#include "application/state_machine.h"

#include <stddef.h>

void roaster_state_machine_init(struct roaster_state_machine *machine,
				uint64_t now_ms)
{
	if (machine == NULL) {
		return;
	}

	machine->state = ROASTER_STATE_BOOT;
	machine->entered_ms = now_ms;
	machine->fault_flags = 0U;
}

void roaster_state_transition(struct roaster_state_machine *machine,
			      enum roaster_state next_state, uint64_t now_ms,
			      uint32_t fault_flags)
{
	if (machine == NULL || machine->state == next_state) {
		return;
	}

	machine->state = next_state;
	machine->entered_ms = now_ms;
	machine->fault_flags = fault_flags;
}

bool roaster_state_allows_heating(enum roaster_state state)
{
	return state == ROASTER_STATE_PREHEAT || state == ROASTER_STATE_READY ||
	       state == ROASTER_STATE_ROASTING;
}

const char *roaster_state_name(enum roaster_state state)
{
	switch (state) {
	case ROASTER_STATE_BOOT:
		return "BOOT";
	case ROASTER_STATE_SELF_TEST:
		return "SELF_TEST";
	case ROASTER_STATE_IDLE:
		return "IDLE";
	case ROASTER_STATE_PREHEAT:
		return "PREHEAT";
	case ROASTER_STATE_READY:
		return "READY";
	case ROASTER_STATE_ROASTING:
		return "ROASTING";
	case ROASTER_STATE_COOLING:
		return "COOLING";
	case ROASTER_STATE_COMPLETE:
		return "COMPLETE";
	case ROASTER_STATE_FAULT:
		return "FAULT";
	default:
		return "UNKNOWN";
	}
}
