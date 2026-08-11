#ifndef ROASTER_APPLICATION_STATE_MACHINE_H
#define ROASTER_APPLICATION_STATE_MACHINE_H

#include <stdbool.h>
#include <stdint.h>

#include "domain/roaster_types.h"

struct roaster_state_machine {
	enum roaster_state state;
	uint64_t entered_ms;
	uint32_t fault_flags;
};

void roaster_state_machine_init(struct roaster_state_machine *machine,
				uint64_t now_ms);
void roaster_state_transition(struct roaster_state_machine *machine,
			      enum roaster_state next_state, uint64_t now_ms,
			      uint32_t fault_flags);
bool roaster_state_allows_heating(enum roaster_state state);
const char *roaster_state_name(enum roaster_state state);

#endif
