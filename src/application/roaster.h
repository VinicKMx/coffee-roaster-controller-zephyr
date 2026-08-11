#ifndef ROASTER_APPLICATION_ROASTER_H
#define ROASTER_APPLICATION_ROASTER_H

#include <stdbool.h>
#include <stdint.h>

#include "application/commands.h"
#include "application/state_machine.h"
#include "control/controller.h"
#include "control/filters.h"
#include "control/profile.h"
#include "control/ror.h"
#include "domain/roaster_types.h"
#include "safety/safety_manager.h"

struct roaster_context {
	struct roaster_state_machine state_machine;
	struct roast_telemetry telemetry;
	struct ror_calculator ror;
	struct temperature_filter temperature_filter;
	struct controller_state controller;
	struct safety_limits safety_limits;
	enum roaster_control_mode control_mode;
	uint16_t manual_heater_permille;
	uint64_t roast_started_ms;
	int32_t target_mdeg_c;
	int32_t ror_mdeg_c_per_min;
};

struct roaster_inputs {
	uint64_t now_ms;
	struct temperature_sample temperature;
	uint16_t heater_actual_permille;
	bool stop_pressed;
	bool airflow_ok;
	bool watchdog_recovered;
	bool heater_current_when_off;
	bool heater_open_when_on;
};

struct roaster_outputs {
	bool heater_permit;
	uint16_t heater_demand_permille;
	uint32_t fault_flags;
};

void roaster_init(struct roaster_context *roaster, uint64_t now_ms);
int roaster_tick(struct roaster_context *roaster, const struct roaster_inputs *inputs,
		 struct roaster_outputs *outputs);
int roaster_handle_command(struct roaster_context *roaster,
			   const struct roaster_command *command, uint64_t now_ms);
const struct roast_telemetry *roaster_get_telemetry(const struct roaster_context *roaster);

#endif
