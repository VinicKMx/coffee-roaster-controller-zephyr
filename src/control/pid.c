#include "control/pid.h"

#include <errno.h>
#include <stddef.h>

static float clampf(float value, float min_value, float max_value)
{
	if (value < min_value) {
		return min_value;
	}

	if (value > max_value) {
		return max_value;
	}

	return value;
}

static int32_t round_to_i32(float value)
{
	if (value >= 0.0f) {
		return (int32_t)(value + 0.5f);
	}

	return (int32_t)(value - 0.5f);
}

void pid_reset(struct pid_state *state)
{
	if (state == NULL) {
		return;
	}

	state->integral_term = 0.0f;
	state->previous_process_mdeg_c = 0;
	state->has_previous_process = false;
}

int32_t pid_update(const struct pid_config *config, struct pid_state *state,
		   int32_t target_mdeg_c, int32_t process_mdeg_c, uint32_t dt_ms)
{
	if (config == NULL || state == NULL || dt_ms == 0U ||
	    config->output_min_permille > config->output_max_permille) {
		return 0;
	}

	const float dt_s = (float)dt_ms / 1000.0f;
	const float error_c = ((float)target_mdeg_c - (float)process_mdeg_c) / 1000.0f;
	const float proportional = config->kp * error_c;
	float derivative = 0.0f;

	if (state->has_previous_process) {
		const float process_delta_c =
			((float)process_mdeg_c - (float)state->previous_process_mdeg_c) /
			1000.0f;
		derivative = -config->kd * (process_delta_c / dt_s);
	}

	const float output_without_integral = proportional + derivative;
	const float min_integral =
		(float)config->output_min_permille - output_without_integral;
	const float max_integral =
		(float)config->output_max_permille - output_without_integral;

	state->integral_term += config->ki * error_c * dt_s;
	state->integral_term =
		clampf(state->integral_term, min_integral, max_integral);

	const float output = clampf(output_without_integral + state->integral_term,
				    (float)config->output_min_permille,
				    (float)config->output_max_permille);

	state->previous_process_mdeg_c = process_mdeg_c;
	state->has_previous_process = true;

	return round_to_i32(output);
}
