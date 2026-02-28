#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/gpio.h>
#include <stdio.h>
#include <string.h>

#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>

LOG_MODULE_REGISTER(main);

static int my_max31865_spi_read(uint8_t reg, uint8_t *data, size_t len);
static int my_max31865_spi_write(uint8_t reg, uint8_t value) __maybe_unused;
static double calculate_temperature(double resistance, double resistance_0);

#define MAX31865_NODE DT_NODELABEL(max31865)
#define ZEPHYR_USER_NODE DT_PATH(zephyr_user)
#define TRIAC_DELAY_US 3000
#define TRIAC_PULSE_US 100
#define TRIAC_THREAD_STACK_SIZE 1024
#define TRIAC_THREAD_PRIORITY 5

#if !DT_NODE_HAS_PROP(ZEPHYR_USER_NODE, zc_gpios)
#error "zephyr,user.zc-gpios is not defined in app.overlay"
#endif
#if !DT_NODE_HAS_PROP(ZEPHYR_USER_NODE, fire_gpios)
#error "zephyr,user.fire-gpios is not defined in app.overlay"
#endif

static const struct spi_dt_spec spi = SPI_DT_SPEC_GET(
	MAX31865_NODE,
	SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_TRANSFER_MSB | SPI_MODE_CPHA
);
static const struct gpio_dt_spec zc_gpio = GPIO_DT_SPEC_GET(ZEPHYR_USER_NODE, zc_gpios);
static const struct gpio_dt_spec fire_gpio = GPIO_DT_SPEC_GET(ZEPHYR_USER_NODE, fire_gpios);
static struct gpio_callback zc_cb_data;
static atomic_t zc_pulse_count = ATOMIC_INIT(0);
static atomic_t fire_pulse_count = ATOMIC_INIT(0);
K_SEM_DEFINE(zc_sem, 0, 1000);
K_THREAD_STACK_DEFINE(triac_thread_stack, TRIAC_THREAD_STACK_SIZE);
static struct k_thread triac_thread_data;

const struct device *dev = DEVICE_DT_GET_ONE(maxim_max31865);

static void triac_pulse_thread(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	while (1) {
		k_sem_take(&zc_sem, K_FOREVER);
		k_busy_wait(TRIAC_DELAY_US);
		gpio_pin_set_dt(&fire_gpio, 1);
		k_busy_wait(TRIAC_PULSE_US);
		gpio_pin_set_dt(&fire_gpio, 0);
		atomic_inc(&fire_pulse_count);
	}
}

static void zero_cross_isr(const struct device *port, struct gpio_callback *cb, uint32_t pins)
{
	ARG_UNUSED(port);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);

	atomic_inc(&zc_pulse_count);
	k_sem_give(&zc_sem);
}

int main(void)
{
	uint8_t reg_conf = 0x00 & 0x7F;
	uint8_t reg_fault = 0x07 & 0x7F;
	uint8_t dummy = 0xFF;
	uint8_t recv = 0;

	struct spi_buf tx_buf[] = {
		{ .buf = &reg_conf, .len = 1 },
		{ .buf = &dummy, .len = 1 }
	};
	struct spi_buf rx_buf[] = {
		{ .buf = NULL, .len = 1 },
		{ .buf = &recv, .len = 1 }
	};

	struct spi_buf_set tx = { .buffers = tx_buf, .count = 2 };
	struct spi_buf_set rx = { .buffers = rx_buf, .count = 2 };

	if (!spi_is_ready_dt(&spi)) {
		printk("SPI nao esta pronto.\n");
		return 0;
	}

	int ret = spi_transceive_dt(&spi, &tx, &rx);
	if (ret == 0) {
		printk("Leitura do registrador 0x00(conf): 0x%02X\n", recv);
	} else {
		printk("Erro na transacao SPI: %d\n", ret);
	}

	tx_buf[0].buf = &reg_fault;
	recv = 0;

	ret = spi_transceive_dt(&spi, &tx, &rx);
	if (ret == 0) {
		printk("Leitura do registrador 0x07(fault): 0x%02X\n", recv);
	} else {
		printk("Erro na transacao SPI: %d\n", ret);
	}

	if (!device_is_ready(dev)) {
		printk("MAX31865 nao esta pronto!\n");
		return 0;
	}
	printk("MAX31865 encontrado. Iniciando leitura...\n");

	if (!gpio_is_ready_dt(&zc_gpio)) {
		printk("GPIO do zero-cross nao esta pronto.\n");
		return 0;
	}
	if (!gpio_is_ready_dt(&fire_gpio)) {
		printk("GPIO de disparo nao esta pronto.\n");
		return 0;
	}

	ret = gpio_pin_configure_dt(&zc_gpio, GPIO_INPUT);
	if (ret < 0) {
		printk("Falha ao configurar GPIO zero-cross: %d\n", ret);
		return 0;
	}
	ret = gpio_pin_configure_dt(&fire_gpio, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		printk("Falha ao configurar GPIO disparo: %d\n", ret);
		return 0;
	}

	gpio_init_callback(&zc_cb_data, zero_cross_isr, BIT(zc_gpio.pin));
	ret = gpio_add_callback(zc_gpio.port, &zc_cb_data);
	if (ret < 0) {
		printk("Falha ao registrar callback zero-cross: %d\n", ret);
		return 0;
	}

	ret = gpio_pin_interrupt_configure_dt(&zc_gpio, GPIO_INT_EDGE_RISING);
	if (ret < 0) {
		printk("Falha ao habilitar IRQ zero-cross: %d\n", ret);
		return 0;
	}

	printk("Zero-cross em %s pin %u (RISING).\n", zc_gpio.port->name, zc_gpio.pin);
	printk("Disparo triac em %s pin %u.\n", fire_gpio.port->name, fire_gpio.pin);

	k_thread_create(&triac_thread_data, triac_thread_stack, TRIAC_THREAD_STACK_SIZE,
			triac_pulse_thread, NULL, NULL, NULL,
			TRIAC_THREAD_PRIORITY, 0, K_NO_WAIT);

	k_sleep(K_MSEC(66));
	uint8_t config = 0;
	int err = my_max31865_spi_read(0x00, &config, 1);
	if (err == 0) {
		LOG_INF("Valor do registrador 0x00: 0x%02X", config);
	} else {
		LOG_ERR("Erro na leitura SPI: %d", err);
	}

	k_sleep(K_MSEC(66));
	err = my_max31865_spi_read(0x00, &config, 1);
	if (err == 0) {
		LOG_INF("Valor do registrador 0x00: 0x%02X", config);
	} else {
		LOG_ERR("Erro na leitura SPI: %d", err);
	}

	uint32_t last_zc_count = 0;
	uint32_t last_fire_count = 0;
	int64_t last_zc_ms = k_uptime_get();

	while (1) {
		ret = sensor_sample_fetch(dev);

		err = my_max31865_spi_read(0x00, &config, 1);
		if (err == 0) {
			LOG_INF("Valor do registrador 0x00: 0x%02X", config);
		} else {
			LOG_ERR("Erro na leitura SPI: %d", err);
		}
		err = my_max31865_spi_read(0x07, &config, 1);
		if (err == 0) {
			LOG_INF("Valor do registrador 0x07: 0x%02X", config);
		} else {
			LOG_ERR("Erro na leitura SPI: %d", err);
		}

		uint8_t reg1;
		uint8_t reg2;
		err = my_max31865_spi_read(0x01, &reg1, 1);
		if (err == 0) {
			LOG_INF("Valor do registrador 0x01: 0x%02X", reg1);
		} else {
			LOG_ERR("Erro na leitura SPI: %d", err);
		}
		err = my_max31865_spi_read(0x02, &reg2, 1);
		if (err == 0) {
			LOG_INF("Valor do registrador 0x02: 0x%02X", reg2);
		} else {
			LOG_ERR("Erro na leitura SPI: %d", err);
		}

		uint16_t raw = (reg1 << 8) | reg2;
		uint16_t raw_inv = ((uint16_t)((((raw) >> 8) & 0xff) | (((raw) & 0xff) << 8)));
		LOG_INF("RAW direto: 0x%04X", raw);
		raw_inv = raw_inv >> 1;
		raw = raw >> 1;
		double resistance = (double)raw;
		resistance /= 32768;
		resistance *= 430;
		LOG_INF("resistencia: %f", resistance);
		double temperature = calculate_temperature(resistance, 100.0);
		LOG_INF("temperatura: %f", temperature);

		if (ret < 0) {
			printk("Erro ao buscar amostra: %d\n", ret);
		} else {
			struct sensor_value temp;
			ret = sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &temp);
			if (ret < 0) {
				printk("Erro ao ler temperatura: %d\n", ret);
			} else {
				printk("Temperatura: %d.%06d C\n", temp.val1, temp.val2);
			}
		}

		uint32_t zc_total = (uint32_t)atomic_get(&zc_pulse_count);
		uint32_t zc_delta = zc_total - last_zc_count;
		uint32_t fire_total = (uint32_t)atomic_get(&fire_pulse_count);
		uint32_t fire_delta = fire_total - last_fire_count;
		int64_t now_ms = k_uptime_get();
		int64_t elapsed_ms = now_ms - last_zc_ms;
		if (elapsed_ms > 0) {
			uint32_t hz_x10 = (uint32_t)((10000ULL * zc_delta) / (uint64_t)elapsed_ms);
			printk("ZC: total=%u delta=%u freq=%u.%u Hz | FIRE: total=%u delta=%u\n",
			       zc_total, zc_delta, hz_x10 / 10, hz_x10 % 10, fire_total, fire_delta);
		}
		last_zc_count = zc_total;
		last_fire_count = fire_total;
		last_zc_ms = now_ms;

		k_sleep(K_SECONDS(2));
	}

	return 0;
}

static int my_max31865_spi_read(uint8_t reg, uint8_t *data, size_t len)
{
	reg &= 0x7F;

	uint8_t tx_buf[1] = { reg };
	uint8_t rx_buf[1 + len];

	const struct spi_buf txb = { .buf = tx_buf, .len = 1 };
	const struct spi_buf rxb = { .buf = rx_buf, .len = 1 + len };

	struct spi_buf_set tx = { .buffers = &txb, .count = 1 };
	struct spi_buf_set rx = { .buffers = &rxb, .count = 1 };

	int ret = spi_transceive_dt(&spi, &tx, &rx);
	if (ret == 0) {
		memcpy(data, &rx_buf[1], len);
	}
	return ret;
}

static int my_max31865_spi_write(uint8_t reg, uint8_t value)
{
	uint8_t buf[2] = { reg | 0x80, value };
	const struct spi_buf tx_buf = { .buf = buf, .len = 2 };
	const struct spi_buf_set tx = { .buffers = &tx_buf, .count = 1 };

	return spi_write_dt(&spi, &tx);
}

static double calculate_temperature(double resistance, double resistance_0)
{
	double temperature;

	temperature = (resistance - resistance_0) / (resistance_0 * 3.9080e-3);
	return temperature;
}
