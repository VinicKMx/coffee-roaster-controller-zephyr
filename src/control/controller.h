#ifndef ROASTER_CONTROL_CONTROLLER_H
#define ROASTER_CONTROL_CONTROLLER_H

#include <stdint.h>

#include "control/pid.h"
#include "domain/roaster_types.h"

struct controller_config {
	struct pid_config pid;
	uint16_t feed_forward_permille;
	uint16_t max_output_permille;
	uint16_t max_increase_permille_per_s;
	uint16_t max_decrease_permille_per_s;
};

struct controller_state {
	struct pid_state pid;
	uint16_t previous_output_permille;
};

void controller_reset(struct controller_state *state);
uint16_t controller_temperature_hold_update(const struct controller_config *config,
					    struct controller_state *state,
					    int32_t target_mdeg_c,
					    int32_t actual_mdeg_c, uint32_t dt_ms);

#endif
