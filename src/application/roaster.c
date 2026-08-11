#include "application/roaster.h"

#include <errno.h>
#include <stddef.h>
#include <string.h>

#include "application/telemetry.h"
#include "safety/faults.h"

#define SELF_TEST_TIMEOUT_MS 2000U
#define DEFAULT_ROR_LIMIT_MDEG_C_PER_MIN 45000
#define DEFAULT_TEMP_FILTER_ALPHA_PERMILLE 350U

static bool sample_valid(const struct temperature_sample *sample)
{
	return sample != NULL &&
	       (sample->flags & TEMPERATURE_SAMPLE_VALID) != 0U &&
	       (sample->flags & (TEMPERATURE_SAMPLE_STALE |
				 TEMPERATURE_SAMPLE_OPEN_SENSOR |
				 TEMPERATURE_SAMPLE_SHORT_SENSOR |
				 TEMPERATURE_SAMPLE_OUT_OF_RANGE)) == 0U;
}

static uint64_t active_elapsed_ms(const struct roaster_context *roaster, uint64_t now_ms)
{
	if (roaster->roast_started_ms == 0U || now_ms < roaster->roast_started_ms) {
		return 0U;
	}

	switch (roaster->state_machine.state) {
	case ROASTER_STATE_PREHEAT:
	case ROASTER_STATE_READY:
	case ROASTER_STATE_ROASTING:
	case ROASTER_STATE_COOLING:
	case ROASTER_STATE_COMPLETE:
		return now_ms - roaster->roast_started_ms;
	default:
		return 0U;
	}
}

static void update_temperature_model(struct roaster_context *roaster,
				     const struct temperature_sample *sample)
{
	if (!sample_valid(sample)) {
		return;
	}

	const int32_t filtered = temperature_filter_update(
		&roaster->temperature_filter, sample->temperature_mdeg_c);
	(void)ror_add_sample(&roaster->ror, sample->timestamp_ms, filtered,
			     &roaster->ror_mdeg_c_per_min);
}

static uint16_t requested_heater_power(const struct roaster_context *roaster)
{
	if (roaster->state_machine.state != ROASTER_STATE_ROASTING) {
		return 0U;
	}

	if (roaster->control_mode == ROASTER_CONTROL_MANUAL_POWER) {
		return roaster->manual_heater_permille;
	}

	return 0U;
}

static void update_telemetry(struct roaster_context *roaster,
			     const struct roaster_inputs *inputs,
			     const struct roaster_outputs *outputs)
{
	struct roast_telemetry *telemetry = &roaster->telemetry;

	telemetry->elapsed_ms = active_elapsed_ms(roaster, inputs->now_ms);
	telemetry->temperature_mdeg_c = inputs->temperature.temperature_mdeg_c;
	telemetry->target_mdeg_c = roaster->target_mdeg_c;
	telemetry->ror_mdeg_c_per_min = roaster->ror_mdeg_c_per_min;
	telemetry->heater_request_permille = outputs->heater_demand_permille;
	telemetry->heater_actual_permille = inputs->heater_actual_permille;
	telemetry->fan_permille = inputs->airflow_ok ? ROASTER_PERMILLE_MAX : 0U;
	telemetry->state = roaster->state_machine.state;
	telemetry->fault_flags = outputs->fault_flags;
}

void roaster_init(struct roaster_context *roaster, uint64_t now_ms)
{
	if (roaster == NULL) {
		return;
	}

	memset(roaster, 0, sizeof(*roaster));
	roaster_state_machine_init(&roaster->state_machine, now_ms);
	roast_telemetry_clear(&roaster->telemetry);
	ror_init(&roaster->ror, ROR_DEFAULT_WINDOW_MS);
	temperature_filter_init(&roaster->temperature_filter,
				DEFAULT_TEMP_FILTER_ALPHA_PERMILLE);
	controller_reset(&roaster->controller);
	safety_manager_init();

	roaster->safety_limits.hard_max_mdeg_c = CONFIG_ROASTER_HARD_MAX_TEMP_MDEG_C;
	roaster->safety_limits.max_ror_mdeg_c_per_min =
		DEFAULT_ROR_LIMIT_MDEG_C_PER_MIN;
	roaster->safety_limits.require_airflow = false;
	roaster->control_mode = ROASTER_CONTROL_MANUAL_POWER;
}

int roaster_tick(struct roaster_context *roaster, const struct roaster_inputs *inputs,
		 struct roaster_outputs *outputs)
{
	if (roaster == NULL || inputs == NULL || outputs == NULL) {
		return -EINVAL;
	}

	outputs->heater_permit = false;
	outputs->heater_demand_permille = 0U;
	outputs->fault_flags = 0U;

	update_temperature_model(roaster, &inputs->temperature);

	if (roaster->state_machine.state == ROASTER_STATE_BOOT) {
		roaster_state_transition(&roaster->state_machine, ROASTER_STATE_SELF_TEST,
					 inputs->now_ms, 0U);
	}

	if (roaster->state_machine.state == ROASTER_STATE_SELF_TEST) {
		if (sample_valid(&inputs->temperature)) {
			roaster_state_transition(&roaster->state_machine,
						 ROASTER_STATE_IDLE, inputs->now_ms, 0U);
		} else if ((inputs->now_ms - roaster->state_machine.entered_ms) >=
			   SELF_TEST_TIMEOUT_MS) {
			roaster_state_transition(&roaster->state_machine,
						 ROASTER_STATE_FAULT, inputs->now_ms,
						 ROAST_FAULT_TEMP_SENSOR_INVALID);
		}
	}

	uint16_t requested = requested_heater_power(roaster);

	struct safety_inputs safety_inputs = {
		.state = roaster->state_machine.state,
		.temperature = inputs->temperature,
		.ror_mdeg_c_per_min = roaster->ror_mdeg_c_per_min,
		.heater_request_permille = requested,
		.stop_pressed = inputs->stop_pressed,
		.airflow_ok = inputs->airflow_ok,
		.watchdog_recovered = inputs->watchdog_recovered,
		.heater_current_when_off = inputs->heater_current_when_off,
		.heater_open_when_on = inputs->heater_open_when_on,
	};
	struct safety_decision decision;

	safety_manager_evaluate(&roaster->safety_limits, &safety_inputs, &decision);

	if (inputs->stop_pressed &&
	    roaster_state_allows_heating(roaster->state_machine.state)) {
		roaster_state_transition(&roaster->state_machine, ROASTER_STATE_COOLING,
					 inputs->now_ms, decision.fault_flags);
	}

	if (roaster->state_machine.state != ROASTER_STATE_SELF_TEST &&
	    roaster->state_machine.state != ROASTER_STATE_BOOT &&
	    roast_fault_flags_require_latch(decision.fault_flags) &&
	    roaster->state_machine.state != ROASTER_STATE_FAULT) {
		roaster_state_transition(&roaster->state_machine, ROASTER_STATE_FAULT,
					 inputs->now_ms, decision.fault_flags);
	}

	outputs->fault_flags = decision.fault_flags | roaster->state_machine.fault_flags;
	outputs->heater_permit =
		decision.heater_permitted &&
		roaster->state_machine.state != ROASTER_STATE_FAULT &&
		roaster->state_machine.state != ROASTER_STATE_COOLING;
	outputs->heater_demand_permille =
		outputs->heater_permit ? decision.heater_allowed_permille : 0U;

	update_telemetry(roaster, inputs, outputs);
	return 0;
}

int roaster_handle_command(struct roaster_context *roaster,
			   const struct roaster_command *command, uint64_t now_ms)
{
	if (roaster == NULL || command == NULL) {
		return -EINVAL;
	}

	switch (command->type) {
	case ROASTER_COMMAND_START_MANUAL:
		if (roaster->state_machine.state != ROASTER_STATE_IDLE &&
		    roaster->state_machine.state != ROASTER_STATE_COMPLETE) {
			return -EACCES;
		}
		roaster->control_mode = ROASTER_CONTROL_MANUAL_POWER;
		roaster->manual_heater_permille = 0U;
		roaster->roast_started_ms = now_ms;
		roaster_state_transition(&roaster->state_machine,
					 ROASTER_STATE_ROASTING, now_ms, 0U);
		return 0;
	case ROASTER_COMMAND_SET_MANUAL_POWER:
		roaster->manual_heater_permille =
			roaster_clamp_permille(command->value_permille);
		return 0;
	case ROASTER_COMMAND_STOP:
		roaster->manual_heater_permille = 0U;
		if (roaster->state_machine.state != ROASTER_STATE_FAULT) {
			roaster_state_transition(&roaster->state_machine,
						 ROASTER_STATE_COOLING, now_ms, 0U);
		}
		return 0;
	case ROASTER_COMMAND_ACK_FAULT:
		if (roaster->state_machine.state != ROASTER_STATE_FAULT) {
			return -EALREADY;
		}
		safety_manager_clear_latched();
		roaster->state_machine.fault_flags = 0U;
		roaster_state_transition(&roaster->state_machine, ROASTER_STATE_IDLE,
					 now_ms, 0U);
		return 0;
	default:
		return -EINVAL;
	}
}

const struct roast_telemetry *roaster_get_telemetry(const struct roaster_context *roaster)
{
	return roaster == NULL ? NULL : &roaster->telemetry;
}
