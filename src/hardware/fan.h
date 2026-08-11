#ifndef ROASTER_HARDWARE_FAN_H
#define ROASTER_HARDWARE_FAN_H

#include <stdint.h>

int fan_set_power(uint16_t permille);
uint16_t fan_get_power(void);

#endif
