#include "control/controller.h"

#include <stddef.h>

static uint16_t apply_slew(uint16_t previous, uint16_t target, uint16_t up_per_s,
			   uint16_t down_per_s, uint32_t dt_ms)
{
	const uint32_t up_step = ((uint32_t)up_per_s * dt_ms) / 1000U;
	const uint32_t down_step = ((uint32_t)down_per_s * dt_ms) / 1000U;

	if (target > previous) {
		const uint32_t delta = (uint32_t)target - previous;
		return previous + (uint16_t)(delta > up_step ? up_step : delta);
	}

	const uint32_t delta = (uint32_t)previous - target;
	return previous - (uint16_t)(delta > down_step ? down_step : delta);
}

void controller_reset(struct controller_state *state)
{
	if (state == NULL) {
		return;
	}

	pid_reset(&state->pid);
	state->previous_output_permille = 0U;
}

uint16_t controller_temperature_hold_update(const struct controller_config *config,
					    struct controller_state *state,
					    int32_t target_mdeg_c,
					    int32_t actual_mdeg_c, uint32_t dt_ms)
{
	if (config == NULL || state == NULL) {
		return 0U;
	}

	int32_t pid_output =
		pid_update(&config->pid, &state->pid, target_mdeg_c, actual_mdeg_c, dt_ms);
	if (pid_output < 0) {
		pid_output = 0;
	}

	uint32_t output = (uint32_t)pid_output + config->feed_forward_permille;
	if (output > config->max_output_permille) {
		output = config->max_output_permille;
	}
	if (output > ROASTER_PERMILLE_MAX) {
		output = ROASTER_PERMILLE_MAX;
	}

	const uint16_t slewed =
		apply_slew(state->previous_output_permille, (uint16_t)output,
			   config->max_increase_permille_per_s,
			   config->max_decrease_permille_per_s, dt_ms);
	state->previous_output_permille = slewed;

	return slewed;
}
