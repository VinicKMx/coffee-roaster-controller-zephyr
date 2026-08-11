#include "safety/interlocks.h"

#include <stddef.h>

void interlocks_init(struct hardware_interlocks *interlocks)
{
	if (interlocks == NULL) {
		return;
	}

	interlocks->stop_pressed = false;
	interlocks->airflow_ok = true;
}
