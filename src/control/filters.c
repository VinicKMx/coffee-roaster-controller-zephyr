#include "control/filters.h"

#include <stddef.h>

void temperature_filter_init(struct temperature_filter *filter, uint16_t alpha_permille)
{
	if (filter == NULL) {
		return;
	}

	filter->value_mdeg_c = 0;
	filter->alpha_permille = alpha_permille > 1000U ? 1000U : alpha_permille;
	filter->initialized = false;
}

int32_t temperature_filter_update(struct temperature_filter *filter,
				  int32_t input_mdeg_c)
{
	if (filter == NULL) {
		return input_mdeg_c;
	}

	if (!filter->initialized) {
		filter->value_mdeg_c = input_mdeg_c;
		filter->initialized = true;
		return filter->value_mdeg_c;
	}

	filter->value_mdeg_c =
		(int32_t)(((int64_t)filter->alpha_permille * input_mdeg_c +
			   (int64_t)(1000U - filter->alpha_permille) *
				   filter->value_mdeg_c) /
			  1000);

	return filter->value_mdeg_c;
}
