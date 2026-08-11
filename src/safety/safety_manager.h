#ifndef ROASTER_SAFETY_MANAGER_H
#define ROASTER_SAFETY_MANAGER_H

#include <stdbool.h>
#include <stdint.h>

#include "domain/roaster_types.h"

struct safety_limits {
	int32_t hard_max_mdeg_c;
	int32_t max_ror_mdeg_c_per_min;
	bool require_airflow;
};

struct safety_inputs {
	enum roaster_state state;
	struct temperature_sample temperature;
	int32_t ror_mdeg_c_per_min;
	uint16_t heater_request_permille;
	bool stop_pressed;
	bool airflow_ok;
	bool watchdog_recovered;
	bool heater_current_when_off;
	bool heater_open_when_on;
};

struct safety_decision {
	bool heater_permitted;
	uint16_t heater_allowed_permille;
	uint32_t fault_flags;
};

void safety_manager_init(void);
void safety_manager_clear_latched(void);
uint32_t safety_manager_latched_faults(void);
void safety_manager_evaluate(const struct safety_limits *limits,
			     const struct safety_inputs *inputs,
			     struct safety_decision *decision);

#endif
