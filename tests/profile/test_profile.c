#include <zephyr/ztest.h>

#include "control/profile.h"

ZTEST(profile_tests, test_linear_interpolation)
{
	struct roast_profile profile = {
		.id = 1U,
		.name = "test",
		.point_count = 2U,
		.points = {
			{ .time_ms = 0U, .temperature_mdeg_c = 25000 },
			{ .time_ms = 10000U, .temperature_mdeg_c = 125000 },
		},
	};
	int32_t target = 0;

	zassert_ok(profile_target_at(&profile, 5000U, &target));
	zassert_equal(target, 75000);
}

ZTEST(profile_tests, test_validation_rejects_non_monotonic_time)
{
	struct roast_profile profile = {
		.id = 1U,
		.name = "bad",
		.point_count = 2U,
		.points = {
			{ .time_ms = 0U, .temperature_mdeg_c = 25000 },
			{ .time_ms = 0U, .temperature_mdeg_c = 125000 },
		},
	};
	uint32_t faults = 0U;

	zassert_not_equal(profile_validate(&profile, &faults), 0);
	zassert_true((faults & ROAST_FAULT_PROFILE_INVALID) != 0U);
}

ZTEST_SUITE(profile_tests, NULL, NULL, NULL, NULL, NULL);
