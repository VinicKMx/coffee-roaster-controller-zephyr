#ifndef ROASTER_HARDWARE_TEMPERATURE_H
#define ROASTER_HARDWARE_TEMPERATURE_H

#include "domain/roaster_types.h"

int temperature_sensor_init(void);
int temperature_sensor_read(struct temperature_sample *sample);

#endif
