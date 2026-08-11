#include "application/commands.h"

#include "domain/roaster_types.h"

struct roaster_command roaster_command_start_manual(void)
{
	return (struct roaster_command){
		.type = ROASTER_COMMAND_START_MANUAL,
		.value_permille = 0U,
	};
}

struct roaster_command roaster_command_set_manual_power(uint16_t permille)
{
	return (struct roaster_command){
		.type = ROASTER_COMMAND_SET_MANUAL_POWER,
		.value_permille = roaster_clamp_permille(permille),
	};
}

struct roaster_command roaster_command_stop(void)
{
	return (struct roaster_command){
		.type = ROASTER_COMMAND_STOP,
		.value_permille = 0U,
	};
}

struct roaster_command roaster_command_ack_fault(void)
{
	return (struct roaster_command){
		.type = ROASTER_COMMAND_ACK_FAULT,
		.value_permille = 0U,
	};
}
