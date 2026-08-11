#include "safety/safety_manager.h"

#include <stddef.h>

#include "application/state_machine.h"
#include "safety/faults.h"

#ifndef CONFIG_ROASTER_HARD_MAX_TEMP_MDEG_C
#define CONFIG_ROASTER_HARD_MAX_TEMP_MDEG_C 260000
#endif

static uint32_t latched_faults;

void safety_manager_init(void)
{
	latched_faults = 0U;
}

void safety_manager_clear_latched(void)
{
	latched_faults = 0U;
}

uint32_t safety_manager_latched_faults(void)
{
	return latched_faults;
}

static int32_t resolved_hard_limit(const struct safety_limits *limits)
{
	if (limits != NULL && limits->hard_max_mdeg_c > 0) {
		return limits->hard_max_mdeg_c;
	}

	return CONFIG_ROASTER_HARD_MAX_TEMP_MDEG_C;
}

static uint32_t evaluate_faults(const struct safety_limits *limits,
				const struct safety_inputs *inputs)
{
	uint32_t faults = 0U;
	const uint32_t temp_flags = inputs->temperature.flags;

	if ((temp_flags & TEMPERATURE_SAMPLE_VALID) == 0U ||
	    (temp_flags & (TEMPERATURE_SAMPLE_OPEN_SENSOR |
			   TEMPERATURE_SAMPLE_SHORT_SENSOR |
			   TEMPERATURE_SAMPLE_OUT_OF_RANGE)) != 0U) {
		faults |= ROAST_FAULT_TEMP_SENSOR_INVALID;
	}

	if ((temp_flags & TEMPERATURE_SAMPLE_STALE) != 0U) {
		faults |= ROAST_FAULT_TEMP_SENSOR_STALE;
	}

	if ((temp_flags & TEMPERATURE_SAMPLE_VALID) != 0U &&
	    inputs->temperature.temperature_mdeg_c > resolved_hard_limit(limits)) {
		faults |= ROAST_FAULT_OVERTEMPERATURE;
	}

	if (limits != NULL && limits->max_ror_mdeg_c_per_min > 0 &&
	    inputs->ror_mdeg_c_per_min > limits->max_ror_mdeg_c_per_min) {
		faults |= ROAST_FAULT_EXCESSIVE_ROR;
	}

	if (inputs->stop_pressed) {
		faults |= ROAST_FAULT_STOP_ASSERTED;
	}

	if (limits != NULL && limits->require_airflow && !inputs->airflow_ok) {
		faults |= ROAST_FAULT_FAN_FAILURE;
	}

	if (inputs->watchdog_recovered) {
		faults |= ROAST_FAULT_WATCHDOG_RECOVERY;
	}

	if (inputs->heater_request_permille == 0U && inputs->heater_current_when_off) {
		faults |= ROAST_FAULT_HEATER_STUCK_ON;
	}

	if (inputs->heater_request_permille >= 800U && inputs->heater_open_when_on) {
		faults |= ROAST_FAULT_HEATER_OPEN;
	}

	return faults;
}

static bool state_latches_faults(enum roaster_state state)
{
	return state != ROASTER_STATE_BOOT && state != ROASTER_STATE_SELF_TEST;
}

void safety_manager_evaluate(const struct safety_limits *limits,
			     const struct safety_inputs *inputs,
			     struct safety_decision *decision)
{
	if (decision == NULL) {
		return;
	}

	decision->heater_permitted = false;
	decision->heater_allowed_permille = 0U;
	decision->fault_flags = ROAST_FAULT_INTERNAL_CONTROL_ERROR;

	if (inputs == NULL) {
		latched_faults |= ROAST_FAULT_INTERNAL_CONTROL_ERROR;
		return;
	}

	const uint32_t current_faults = evaluate_faults(limits, inputs);
	if (state_latches_faults(inputs->state) &&
	    roast_fault_flags_require_latch(current_faults)) {
		latched_faults |= (current_faults & ~ROAST_FAULT_STOP_ASSERTED);
	}

	const uint32_t blocking_faults = latched_faults | current_faults;
	const bool state_allows_heating = roaster_state_allows_heating(inputs->state);

	decision->fault_flags = blocking_faults;
	decision->heater_permitted =
		blocking_faults == 0U && state_allows_heating && !inputs->stop_pressed;
	decision->heater_allowed_permille =
		decision->heater_permitted ?
			roaster_clamp_permille(inputs->heater_request_permille) :
			0U;
}
