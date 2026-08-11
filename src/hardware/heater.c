#include "hardware/heater.h"

#include <errno.h>

#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "domain/roaster_types.h"

LOG_MODULE_REGISTER(heater, LOG_LEVEL_INF);

#ifndef CONFIG_ROASTER_HEATER_BURST_WINDOW_MS
#define CONFIG_ROASTER_HEATER_BURST_WINDOW_MS 1000
#endif

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

static uint16_t requested_permille;
static uint16_t actual_permille;

static int heater_write(bool enabled)
{
#if HEATER_HAS_GPIO
	if (!device_is_ready(heater_gpio.port)) {
		return -ENODEV;
	}

	return gpio_pin_set_dt(&heater_gpio, enabled ? 1 : 0);
#else
	ARG_UNUSED(enabled);
	return -ENODEV;
#endif
}

bool heater_output_available(void)
{
#if HEATER_HAS_GPIO
	return device_is_ready(heater_gpio.port);
#else
	return false;
#endif
}

int heater_init(void)
{
	requested_permille = 0U;
	actual_permille = 0U;

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
}

int heater_set_demand(uint16_t permille)
{
	requested_permille = roaster_clamp_permille(permille);
	return 0;
}

void heater_force_off(void)
{
	requested_permille = 0U;
	actual_permille = 0U;
	(void)heater_write(false);
}

void heater_service(uint64_t now_ms)
{
	if (requested_permille == 0U || !heater_output_available()) {
		actual_permille = 0U;
		(void)heater_write(false);
		return;
	}

	const uint32_t window_ms = CONFIG_ROASTER_HEATER_BURST_WINDOW_MS;
	const uint32_t position_ms = (uint32_t)(now_ms % window_ms);
	const uint32_t on_ms = ((uint32_t)requested_permille * window_ms) /
			       ROASTER_PERMILLE_MAX;
	const bool enabled = position_ms < on_ms;

	actual_permille = enabled ? ROASTER_PERMILLE_MAX : 0U;
	(void)heater_write(enabled);
}

uint16_t heater_get_requested(void)
{
	return requested_permille;
}

uint16_t heater_get_actual(void)
{
	return actual_permille;
}
