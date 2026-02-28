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

LOG_MODULE_REGISTER(main);

static int my_max31865_spi_read(uint8_t reg, uint8_t *data, size_t len);
static int my_max31865_spi_write(uint8_t reg, uint8_t value) __maybe_unused;
static double calculate_temperature(double resistance, double resistance_0);

#define MAX31865_NODE DT_NODELABEL(max31865)

static const struct spi_dt_spec spi = SPI_DT_SPEC_GET(
	MAX31865_NODE,
	SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_TRANSFER_MSB | SPI_MODE_CPHA
);

const struct device *dev = DEVICE_DT_GET_ONE(maxim_max31865);

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
