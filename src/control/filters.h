#ifndef ROASTER_CONTROL_FILTERS_H
#define ROASTER_CONTROL_FILTERS_H

#include <stdbool.h>
#include <stdint.h>

struct temperature_filter {
	int32_t value_mdeg_c;
	uint16_t alpha_permille;
	bool initialized;
};

void temperature_filter_init(struct temperature_filter *filter, uint16_t alpha_permille);
int32_t temperature_filter_update(struct temperature_filter *filter,
				  int32_t input_mdeg_c);

#endif
