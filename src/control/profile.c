#include "control/profile.h"

#include <errno.h>
#include <stddef.h>

static int32_t limit_max_target(const struct roast_limits *limits)
{
	return limits->max_target_mdeg_c > 0 ? limits->max_target_mdeg_c :
					      ROASTER_PROFILE_DEFAULT_MAX_TARGET_MDEG_C;
}

static uint32_t limit_max_duration(const struct roast_limits *limits)
{
	return limits->max_profile_duration_ms > 0 ?
		       limits->max_profile_duration_ms :
		       ROASTER_PROFILE_DEFAULT_MAX_DURATION_MS;
}

static uint16_t limit_max_heater(const struct roast_limits *limits)
{
	if (limits->max_heater_power_permille == 0U) {
		return ROASTER_PROFILE_DEFAULT_MAX_HEATER_PERMILLE;
	}

	return roaster_clamp_permille(limits->max_heater_power_permille);
}

int profile_validate(const struct roast_profile *profile, uint32_t *fault_flags)
{
	uint32_t faults = 0U;

	if (profile == NULL) {
		if (fault_flags != NULL) {
			*fault_flags = ROAST_FAULT_PROFILE_INVALID;
		}
		return -EINVAL;
	}

	if (profile->point_count < 2U ||
	    profile->point_count > ROASTER_PROFILE_MAX_POINTS) {
		faults |= ROAST_FAULT_PROFILE_INVALID;
	}

	if (limit_max_heater(&profile->limits) > ROASTER_PERMILLE_MAX) {
		faults |= ROAST_FAULT_PROFILE_INVALID;
	}

	for (uint16_t i = 0U; i < profile->point_count; i++) {
		const struct roast_profile_point *point = &profile->points[i];

		if (point->temperature_mdeg_c > limit_max_target(&profile->limits) ||
		    point->time_ms > limit_max_duration(&profile->limits)) {
			faults |= ROAST_FAULT_PROFILE_INVALID;
		}

		if (i == 0U) {
			if (point->time_ms != 0U) {
				faults |= ROAST_FAULT_PROFILE_INVALID;
			}
			continue;
		}

		if (point->time_ms <= profile->points[i - 1U].time_ms) {
			faults |= ROAST_FAULT_PROFILE_INVALID;
		}
	}

	if (fault_flags != NULL) {
		*fault_flags = faults;
	}

	return faults == 0U ? 0 : -EINVAL;
}

int profile_target_at(const struct roast_profile *profile, uint32_t elapsed_ms,
		      int32_t *target_mdeg_c)
{
	uint32_t faults;

	if (target_mdeg_c == NULL) {
		return -EINVAL;
	}

	if (profile_validate(profile, &faults) != 0) {
		(void)faults;
		return -EINVAL;
	}

	if (elapsed_ms <= profile->points[0].time_ms) {
		*target_mdeg_c = profile->points[0].temperature_mdeg_c;
		return 0;
	}

	for (uint16_t i = 1U; i < profile->point_count; i++) {
		const struct roast_profile_point *a = &profile->points[i - 1U];
		const struct roast_profile_point *b = &profile->points[i];

		if (elapsed_ms <= b->time_ms) {
			const int64_t dt = (int64_t)b->time_ms - (int64_t)a->time_ms;
			const int64_t elapsed = (int64_t)elapsed_ms - (int64_t)a->time_ms;
			const int64_t temp_delta =
				(int64_t)b->temperature_mdeg_c -
				(int64_t)a->temperature_mdeg_c;

			*target_mdeg_c =
				a->temperature_mdeg_c + (int32_t)((temp_delta * elapsed) / dt);
			return 0;
		}
	}

	*target_mdeg_c = profile->points[profile->point_count - 1U].temperature_mdeg_c;
	return 0;
}
