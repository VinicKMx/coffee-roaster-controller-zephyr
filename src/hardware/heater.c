#include "hardware/heater.h"

#include <errno.h>

#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>

#include "domain/roaster_types.h"

LOG_MODULE_REGISTER(heater, LOG_LEVEL_INF);

#ifndef CONFIG_ROASTER_HEATER_BURST_WINDOW_MS
#define CONFIG_ROASTER_HEATER_BURST_WINDOW_MS 1000
#endif

#ifndef CONFIG_ROASTER_MAINS_HALF_CYCLE_US
#define CONFIG_ROASTER_MAINS_HALF_CYCLE_US 8333
#endif

#ifndef CONFIG_ROASTER_ROBOTDYN_DIMMER_MIN_FIRE_DELAY_US
#define CONFIG_ROASTER_ROBOTDYN_DIMMER_MIN_FIRE_DELAY_US 150
#endif

#ifndef CONFIG_ROASTER_ROBOTDYN_DIMMER_GATE_PULSE_US
#define CONFIG_ROASTER_ROBOTDYN_DIMMER_GATE_PULSE_US 100
#endif

#ifndef CONFIG_ROASTER_ROBOTDYN_DIMMER_ZC_MIN_INTERVAL_US
#define CONFIG_ROASTER_ROBOTDYN_DIMMER_ZC_MIN_INTERVAL_US 3000
#endif

#define ROBOTDYN_ZC_STALE_MS 250U

#if defined(CONFIG_ROASTER_HEATER_ROBOTDYN_DIMMER)
#define ROBOTDYN_ZC_NODE DT_ALIAS(heater_zc0)
#define ROBOTDYN_PSM_NODE DT_ALIAS(heater_psm0)
#if DT_NODE_HAS_STATUS(ROBOTDYN_ZC_NODE, okay) && DT_NODE_HAS_STATUS(ROBOTDYN_PSM_NODE, okay)
#define ROBOTDYN_HAS_GPIOS 1
#else
#define ROBOTDYN_HAS_GPIOS 0
#endif
#else
#define ROBOTDYN_HAS_GPIOS 0
#endif

#if defined(CONFIG_ROASTER_HEATER_ROBOTDYN_DIMMER)
#if ROBOTDYN_HAS_GPIOS
static const struct gpio_dt_spec heater_zc_gpio = GPIO_DT_SPEC_GET(ROBOTDYN_ZC_NODE, gpios);
static const struct gpio_dt_spec heater_psm_gpio = GPIO_DT_SPEC_GET(ROBOTDYN_PSM_NODE, gpios);
static struct gpio_callback heater_zc_callback;
static struct k_timer heater_fire_timer;
static struct k_timer heater_gate_pulse_timer;
static uint64_t last_zc_us;
static volatile uint32_t detected_half_cycle_us = CONFIG_ROASTER_MAINS_HALF_CYCLE_US;
#endif
#else
#if DT_NODE_HAS_STATUS(DT_ALIAS(heater0), okay)
#define HEATER_NODE DT_ALIAS(heater0)
#define HEATER_HAS_GPIO 1
#elif DT_NODE_HAS_STATUS(DT_ALIAS(led0), okay)
#define HEATER_NODE DT_ALIAS(led0)
#define HEATER_HAS_GPIO 1
#else
#define HEATER_HAS_GPIO 0
#endif

#if HEATER_HAS_GPIO
static const struct gpio_dt_spec heater_gpio = GPIO_DT_SPEC_GET(HEATER_NODE, gpios);
#endif
#endif

static atomic_t requested_permille;
static atomic_t actual_permille;
#if ROBOTDYN_HAS_GPIOS
static atomic_t last_zc_ms;
#endif

static uint16_t atomic_get_permille(atomic_t *value)
{
	atomic_val_t raw = atomic_get(value);

	if (raw < 0) {
		return 0U;
	}

	return roaster_clamp_permille((uint16_t)raw);
}

static void atomic_set_permille(atomic_t *value, uint16_t permille)
{
	atomic_set(value, roaster_clamp_permille(permille));
}

static int heater_write(bool enabled)
{
#if defined(CONFIG_ROASTER_HEATER_ROBOTDYN_DIMMER)
#if ROBOTDYN_HAS_GPIOS
	if (!device_is_ready(heater_psm_gpio.port)) {
		return -ENODEV;
	}

	return gpio_pin_set_dt(&heater_psm_gpio, enabled ? 1 : 0);
#else
	ARG_UNUSED(enabled);
	return -ENODEV;
#endif
#else
#if HEATER_HAS_GPIO
	if (!device_is_ready(heater_gpio.port)) {
		return -ENODEV;
	}

	return gpio_pin_set_dt(&heater_gpio, enabled ? 1 : 0);
#else
	ARG_UNUSED(enabled);
	return -ENODEV;
#endif
#endif
}

bool heater_output_available(void)
{
#if defined(CONFIG_ROASTER_HEATER_ROBOTDYN_DIMMER)
#if ROBOTDYN_HAS_GPIOS
	return device_is_ready(heater_zc_gpio.port) && device_is_ready(heater_psm_gpio.port);
#else
	return false;
#endif
#else
#if HEATER_HAS_GPIO
	return device_is_ready(heater_gpio.port);
#else
	return false;
#endif
#endif
}

#if ROBOTDYN_HAS_GPIOS
static uint32_t robotdyn_fire_delay_us(uint16_t permille)
{
	const uint32_t min_delay_us = CONFIG_ROASTER_ROBOTDYN_DIMMER_MIN_FIRE_DELAY_US;
	const uint32_t gate_pulse_us = CONFIG_ROASTER_ROBOTDYN_DIMMER_GATE_PULSE_US;
	const uint32_t half_cycle_us = detected_half_cycle_us;
	uint32_t max_delay_us = min_delay_us;

	if (half_cycle_us > gate_pulse_us) {
		max_delay_us = half_cycle_us - gate_pulse_us;
	}

	if (max_delay_us < min_delay_us) {
		max_delay_us = min_delay_us;
	}

	const uint32_t span_us = max_delay_us - min_delay_us;

	return max_delay_us -
	       (((uint32_t)roaster_clamp_permille(permille) * span_us) / ROASTER_PERMILLE_MAX);
}

static void heater_gate_pulse_expired(struct k_timer *timer)
{
	ARG_UNUSED(timer);
	(void)heater_write(false);
}

static void heater_fire_expired(struct k_timer *timer)
{
	ARG_UNUSED(timer);

	const uint16_t demand = atomic_get_permille(&requested_permille);

	if (demand == 0U || !heater_output_available()) {
		atomic_set_permille(&actual_permille, 0U);
		(void)heater_write(false);
		return;
	}

	(void)heater_write(true);
	atomic_set_permille(&actual_permille, demand);
	k_timer_start(&heater_gate_pulse_timer,
		      K_USEC(CONFIG_ROASTER_ROBOTDYN_DIMMER_GATE_PULSE_US), K_NO_WAIT);
}

static bool zc_interval_valid(uint64_t interval_us)
{
	return interval_us >= CONFIG_ROASTER_ROBOTDYN_DIMMER_ZC_MIN_INTERVAL_US &&
	       interval_us <= 12000U;
}

static void heater_zero_crossed(const struct device *port, struct gpio_callback *callback,
				uint32_t pins)
{
	ARG_UNUSED(port);
	ARG_UNUSED(callback);
	ARG_UNUSED(pins);

	const uint64_t now_us = k_cyc_to_us_floor64(k_cycle_get_64());

	if (last_zc_us != 0U) {
		const uint64_t interval_us = now_us - last_zc_us;

		if (interval_us < CONFIG_ROASTER_ROBOTDYN_DIMMER_ZC_MIN_INTERVAL_US) {
			return;
		}

		if (zc_interval_valid(interval_us)) {
			detected_half_cycle_us =
			    ((detected_half_cycle_us * 7U) + (uint32_t)interval_us) / 8U;
		}
	}

	last_zc_us = now_us;
	atomic_set(&last_zc_ms, (atomic_val_t)k_uptime_get_32());
	(void)heater_write(false);

	const uint16_t demand = atomic_get_permille(&requested_permille);
	if (demand == 0U) {
		atomic_set_permille(&actual_permille, 0U);
		k_timer_stop(&heater_fire_timer);
		k_timer_stop(&heater_gate_pulse_timer);
		return;
	}

	k_timer_start(&heater_fire_timer, K_USEC(robotdyn_fire_delay_us(demand)), K_NO_WAIT);
}
#endif

int heater_init(void)
{
	atomic_set_permille(&requested_permille, 0U);
	atomic_set_permille(&actual_permille, 0U);

#if defined(CONFIG_ROASTER_HEATER_ROBOTDYN_DIMMER)
#if ROBOTDYN_HAS_GPIOS
	if (!device_is_ready(heater_zc_gpio.port) || !device_is_ready(heater_psm_gpio.port)) {
		return -ENODEV;
	}

	int ret = gpio_pin_configure_dt(&heater_psm_gpio, GPIO_OUTPUT_INACTIVE);
	if (ret != 0) {
		return ret;
	}

	ret = gpio_pin_configure_dt(&heater_zc_gpio, GPIO_INPUT);
	if (ret != 0) {
		(void)heater_write(false);
		return ret;
	}

	k_timer_init(&heater_fire_timer, heater_fire_expired, NULL);
	k_timer_init(&heater_gate_pulse_timer, heater_gate_pulse_expired, NULL);
	gpio_init_callback(&heater_zc_callback, heater_zero_crossed, BIT(heater_zc_gpio.pin));

	ret = gpio_add_callback(heater_zc_gpio.port, &heater_zc_callback);
	if (ret != 0) {
		(void)heater_write(false);
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&heater_zc_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		(void)gpio_remove_callback(heater_zc_gpio.port, &heater_zc_callback);
		(void)heater_write(false);
		return ret;
	}

	LOG_INF("heater output initialized for RobotDyn phase-angle dimmer");
	return 0;
#else
	LOG_WRN("RobotDyn dimmer backend selected without heater-zc0/heater-psm0 aliases");
	return -ENODEV;
#endif
#else
#if HEATER_HAS_GPIO
	if (!device_is_ready(heater_gpio.port)) {
		return -ENODEV;
	}

	int ret = gpio_pin_configure_dt(&heater_gpio, GPIO_OUTPUT_INACTIVE);
	if (ret != 0) {
		return ret;
	}

	LOG_INF("heater output initialized as time-proportioning GPIO");
	return 0;
#else
	LOG_WRN("no heater0 or led0 alias found; heater output unavailable");
	return -ENODEV;
#endif
#endif
}

int heater_set_demand(uint16_t permille)
{
	atomic_set_permille(&requested_permille, permille);
	return 0;
}

void heater_force_off(void)
{
	atomic_set_permille(&requested_permille, 0U);
	atomic_set_permille(&actual_permille, 0U);
#if ROBOTDYN_HAS_GPIOS
	k_timer_stop(&heater_fire_timer);
	k_timer_stop(&heater_gate_pulse_timer);
#endif
	(void)heater_write(false);
}

void heater_service(uint64_t now_ms)
{
	const uint16_t requested = atomic_get_permille(&requested_permille);

	if (requested == 0U || !heater_output_available()) {
		atomic_set_permille(&actual_permille, 0U);
		(void)heater_write(false);
		return;
	}

#if ROBOTDYN_HAS_GPIOS
	const uint32_t last_ms = (uint32_t)atomic_get(&last_zc_ms);

	if (last_ms == 0U || (uint32_t)(now_ms - last_ms) > ROBOTDYN_ZC_STALE_MS) {
		atomic_set_permille(&actual_permille, 0U);
		(void)heater_write(false);
		return;
	}

	atomic_set_permille(&actual_permille, requested);
#else
	const uint32_t window_ms = CONFIG_ROASTER_HEATER_BURST_WINDOW_MS;
	const uint32_t position_ms = (uint32_t)(now_ms % window_ms);
	const uint32_t on_ms = ((uint32_t)requested * window_ms) / ROASTER_PERMILLE_MAX;
	const bool enabled = position_ms < on_ms;

	atomic_set_permille(&actual_permille, enabled ? ROASTER_PERMILLE_MAX : 0U);
	(void)heater_write(enabled);
#endif
}

uint16_t heater_get_requested(void)
{
	return atomic_get_permille(&requested_permille);
}

uint16_t heater_get_actual(void)
{
	return atomic_get_permille(&actual_permille);
}
