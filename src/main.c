#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>

#include "application/roaster.h"
#include "hardware/heater.h"
#include "hardware/temperature.h"
#include "storage/logger.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#define CONTROL_PERIOD_MS 200U
#define TELEMETRY_PERIOD_MS 1000U

int main(void)
{
	struct roaster_context roaster;
	uint64_t next_telemetry_ms = 0U;

	LOG_INF("coffee roaster controller boot");

	(void)heater_init();
	heater_force_off();

	if (temperature_sensor_init() != 0) {
		LOG_WRN("temperature sensor is not ready; safety will keep heater off");
	}

	roaster_init(&roaster, (uint64_t)k_uptime_get());
	logger_emit_csv_header();

	while (1) {
		const uint64_t now_ms = (uint64_t)k_uptime_get();
		struct temperature_sample sample;
		struct roaster_inputs inputs;
		struct roaster_outputs outputs;

		(void)temperature_sensor_read(&sample);

		inputs.now_ms = now_ms;
		inputs.temperature = sample;
		inputs.heater_actual_permille = heater_get_actual();
		inputs.stop_pressed = false;
		inputs.airflow_ok = true;
		inputs.watchdog_recovered = false;
		inputs.heater_current_when_off = false;
		inputs.heater_open_when_on = false;

		(void)roaster_tick(&roaster, &inputs, &outputs);

		if (!outputs.heater_permit) {
			heater_force_off();
		} else {
			(void)heater_set_demand(outputs.heater_demand_permille);
		}
		heater_service(now_ms);

		if (now_ms >= next_telemetry_ms) {
			logger_emit_telemetry(roaster_get_telemetry(&roaster));
			next_telemetry_ms = now_ms + TELEMETRY_PERIOD_MS;
		}

		k_sleep(K_MSEC(CONTROL_PERIOD_MS));
	}
}
