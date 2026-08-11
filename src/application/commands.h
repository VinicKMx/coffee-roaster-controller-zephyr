#ifndef ROASTER_APPLICATION_COMMANDS_H
#define ROASTER_APPLICATION_COMMANDS_H

#include <stdint.h>

enum roaster_command_type {
	ROASTER_COMMAND_START_MANUAL = 0,
	ROASTER_COMMAND_SET_MANUAL_POWER,
	ROASTER_COMMAND_STOP,
	ROASTER_COMMAND_ACK_FAULT,
};

struct roaster_command {
	enum roaster_command_type type;
	uint16_t value_permille;
};

struct roaster_command roaster_command_start_manual(void);
struct roaster_command roaster_command_set_manual_power(uint16_t permille);
struct roaster_command roaster_command_stop(void);
struct roaster_command roaster_command_ack_fault(void);

#endif
