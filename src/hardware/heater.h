#ifndef ROASTER_HARDWARE_HEATER_H
#define ROASTER_HARDWARE_HEATER_H

#include <stdbool.h>
#include <stdint.h>

int heater_init(void);
int heater_set_demand(uint16_t permille);
void heater_force_off(void);
void heater_service(uint64_t now_ms);
uint16_t heater_get_requested(void);
uint16_t heater_get_actual(void);
bool heater_output_available(void);

#endif
