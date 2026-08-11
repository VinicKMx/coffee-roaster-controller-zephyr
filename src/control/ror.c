#include "control/ror.h"

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>

void ror_init(struct ror_calculator *calculator, uint32_t window_ms)
{
	if (calculator == NULL) {
		return;
	}

	calculator->count = 0U;
	calculator->next = 0U;
	calculator->window_ms = window_ms == 0U ? ROR_DEFAULT_WINDOW_MS : window_ms;
}

static bool point_in_window(const struct ror_calculator *calculator,
			    const struct ror_point *point, uint64_t now_ms)
{
	return point->timestamp_ms <= now_ms &&
	       (now_ms - point->timestamp_ms) <= calculator->window_ms;
}

int ror_add_sample(struct ror_calculator *calculator, uint64_t timestamp_ms,
		   int32_t temperature_mdeg_c, int32_t *ror_mdeg_c_per_min)
{
	uint64_t first_timestamp_ms = UINT64_MAX;
	size_t n = 0U;

	if (calculator == NULL || ror_mdeg_c_per_min == NULL) {
		return -EINVAL;
	}

	calculator->samples[calculator->next].timestamp_ms = timestamp_ms;
	calculator->samples[calculator->next].temperature_mdeg_c = temperature_mdeg_c;
	calculator->next = (calculator->next + 1U) % ROR_SAMPLE_CAPACITY;
	if (calculator->count < ROR_SAMPLE_CAPACITY) {
		calculator->count++;
	}

	for (size_t i = 0U; i < calculator->count; i++) {
		const struct ror_point *point = &calculator->samples[i];

		if (point_in_window(calculator, point, timestamp_ms) &&
		    point->timestamp_ms < first_timestamp_ms) {
			first_timestamp_ms = point->timestamp_ms;
		}
	}

	if (first_timestamp_ms == UINT64_MAX) {
		return -EAGAIN;
	}

	double sum_x = 0.0;
	double sum_y = 0.0;
	double sum_xx = 0.0;
	double sum_xy = 0.0;

	for (size_t i = 0U; i < calculator->count; i++) {
		const struct ror_point *point = &calculator->samples[i];

		if (!point_in_window(calculator, point, timestamp_ms)) {
			continue;
		}

		const double x_ms = (double)(point->timestamp_ms - first_timestamp_ms);
		const double y_mdeg_c = (double)point->temperature_mdeg_c;

		sum_x += x_ms;
		sum_y += y_mdeg_c;
		sum_xx += x_ms * x_ms;
		sum_xy += x_ms * y_mdeg_c;
		n++;
	}

	if (n < 2U) {
		return -EAGAIN;
	}

	const double denominator = ((double)n * sum_xx) - (sum_x * sum_x);
	if (denominator == 0.0) {
		return -EAGAIN;
	}

	const double slope_mdeg_c_per_ms =
		(((double)n * sum_xy) - (sum_x * sum_y)) / denominator;
	const double ror = slope_mdeg_c_per_ms * 60000.0;

	*ror_mdeg_c_per_min = (int32_t)(ror >= 0.0 ? ror + 0.5 : ror - 0.5);
	return 0;
}
