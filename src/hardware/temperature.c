#include "hardware/temperature.h"

#include <errno.h>
#include <stddef.h>

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(temperature, LOG_LEVEL_INF);

#define TEMPERATURE_STALE_TIMEOUT_MS 2000
#define TEMPERATURE_MIN_MDEG_C -50000
#define TEMPERATURE_MAX_MDEG_C 400000

static const struct device *const temp_sensor = DEVICE_DT_GET_ONE(maxim_max31865);
static int64_t last_valid_sample_ms = -1;

static int32_t sensor_value_to_mdeg_c(const struct sensor_value *value)
{
	return (value->val1 * 1000) + (value->val2 / 1000);
}

static void mark_stale_if_needed(struct temperature_sample *sample, int64_t now_ms)
{
	if (last_valid_sample_ms >= 0 &&
	    (now_ms - last_valid_sample_ms) > TEMPERATURE_STALE_TIMEOUT_MS) {
		sample->flags |= TEMPERATURE_SAMPLE_STALE;
	}
}

int temperature_sensor_init(void)
{
	if (!device_is_ready(temp_sensor)) {
		return -ENODEV;
	}

	return 0;
}

int temperature_sensor_read(struct temperature_sample *sample)
{
	struct sensor_value temperature;
	const int64_t now_ms = k_uptime_get();

	if (sample == NULL) {
		return -EINVAL;
	}

	sample->temperature_mdeg_c = 0;
	sample->timestamp_ms = (uint64_t)now_ms;
	sample->flags = 0U;

	if (!device_is_ready(temp_sensor)) {
		sample->flags = TEMPERATURE_SAMPLE_OPEN_SENSOR;
		mark_stale_if_needed(sample, now_ms);
		return -ENODEV;
	}

	int ret = sensor_sample_fetch(temp_sensor);
	if (ret != 0) {
		sample->flags = TEMPERATURE_SAMPLE_OPEN_SENSOR;
		mark_stale_if_needed(sample, now_ms);
		return ret;
	}

	ret = sensor_channel_get(temp_sensor, SENSOR_CHAN_AMBIENT_TEMP, &temperature);
	if (ret != 0) {
		sample->flags = TEMPERATURE_SAMPLE_OPEN_SENSOR;
		mark_stale_if_needed(sample, now_ms);
		return ret;
	}

	sample->temperature_mdeg_c = sensor_value_to_mdeg_c(&temperature);
	sample->flags = TEMPERATURE_SAMPLE_VALID;
	last_valid_sample_ms = now_ms;

	if (sample->temperature_mdeg_c < TEMPERATURE_MIN_MDEG_C ||
	    sample->temperature_mdeg_c > TEMPERATURE_MAX_MDEG_C) {
		sample->flags = TEMPERATURE_SAMPLE_OUT_OF_RANGE;
		return -ERANGE;
	}

	return 0;
}
