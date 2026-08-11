#ifndef ROASTER_CONTROL_PROFILE_H
#define ROASTER_CONTROL_PROFILE_H

#include <stdint.h>

#include "domain/roaster_types.h"

#define ROASTER_PROFILE_DEFAULT_MAX_TARGET_MDEG_C 250000
#define ROASTER_PROFILE_DEFAULT_MAX_DURATION_MS 1800000U
#define ROASTER_PROFILE_DEFAULT_MAX_HEATER_PERMILLE 1000U
#define ROASTER_PROFILE_DEFAULT_MAX_ROR_MDEG_C_PER_MIN 40000

int profile_validate(const struct roast_profile *profile, uint32_t *fault_flags);
int profile_target_at(const struct roast_profile *profile, uint32_t elapsed_ms,
		      int32_t *target_mdeg_c);

#endif
