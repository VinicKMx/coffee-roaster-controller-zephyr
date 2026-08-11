#include "hardware/fan.h"

#include "domain/roaster_types.h"

static uint16_t fan_power_permille;

int fan_set_power(uint16_t permille)
{
	fan_power_permille = roaster_clamp_permille(permille);
	return 0;
}

uint16_t fan_get_power(void)
{
	return fan_power_permille;
}
