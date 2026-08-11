#ifndef ROASTER_CONTROL_PID_H
#define ROASTER_CONTROL_PID_H

#include <stdbool.h>
#include <stdint.h>

struct pid_config {
	float kp;
	float ki;
	float kd;
	int32_t output_min_permille;
	int32_t output_max_permille;
};

struct pid_state {
	float integral_term;
	int32_t previous_process_mdeg_c;
	bool has_previous_process;
};

void pid_reset(struct pid_state *state);
int32_t pid_update(const struct pid_config *config, struct pid_state *state,
		   int32_t target_mdeg_c, int32_t process_mdeg_c, uint32_t dt_ms);

#endif
