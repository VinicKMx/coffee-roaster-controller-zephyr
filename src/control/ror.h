#ifndef ROASTER_CONTROL_ROR_H
#define ROASTER_CONTROL_ROR_H

#include <stddef.h>
#include <stdint.h>

#define ROR_SAMPLE_CAPACITY 16U
#define ROR_DEFAULT_WINDOW_MS 30000U

struct ror_point {
	uint64_t timestamp_ms;
	int32_t temperature_mdeg_c;
};

struct ror_calculator {
	struct ror_point samples[ROR_SAMPLE_CAPACITY];
	size_t count;
	size_t next;
	uint32_t window_ms;
};

void ror_init(struct ror_calculator *calculator, uint32_t window_ms);
int ror_add_sample(struct ror_calculator *calculator, uint64_t timestamp_ms,
		   int32_t temperature_mdeg_c, int32_t *ror_mdeg_c_per_min);

#endif
