#ifndef ROASTER_SAFETY_INTERLOCKS_H
#define ROASTER_SAFETY_INTERLOCKS_H

#include <stdbool.h>

struct hardware_interlocks {
	bool stop_pressed;
	bool airflow_ok;
};

void interlocks_init(struct hardware_interlocks *interlocks);

#endif
