#include "safety/faults.h"

const char *roast_fault_name(uint32_t fault_flag)
{
	switch (fault_flag) {
	case ROAST_FAULT_TEMP_SENSOR_INVALID:
		return "TEMP_SENSOR_INVALID";
	case ROAST_FAULT_TEMP_SENSOR_STALE:
		return "TEMP_SENSOR_STALE";
	case ROAST_FAULT_OVERTEMPERATURE:
		return "OVERTEMPERATURE";
	case ROAST_FAULT_EXCESSIVE_ROR:
		return "EXCESSIVE_ROR";
	case ROAST_FAULT_HEATER_STUCK_ON:
		return "HEATER_STUCK_ON";
	case ROAST_FAULT_HEATER_OPEN:
		return "HEATER_OPEN";
	case ROAST_FAULT_FAN_FAILURE:
		return "FAN_FAILURE";
	case ROAST_FAULT_MAINS_FAILURE:
		return "MAINS_FAILURE";
	case ROAST_FAULT_WATCHDOG_RECOVERY:
		return "WATCHDOG_RECOVERY";
	case ROAST_FAULT_PROFILE_INVALID:
		return "PROFILE_INVALID";
	case ROAST_FAULT_INTERNAL_CONTROL_ERROR:
		return "INTERNAL_CONTROL_ERROR";
	case ROAST_FAULT_STOP_ASSERTED:
		return "STOP_ASSERTED";
	default:
		return "UNKNOWN";
	}
}

bool roast_fault_flags_require_latch(uint32_t fault_flags)
{
	return (fault_flags & ~ROAST_FAULT_STOP_ASSERTED) != 0U;
}
