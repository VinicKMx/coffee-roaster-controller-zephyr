#ifndef ROASTER_DOMAIN_ROASTER_TYPES_H
#define ROASTER_DOMAIN_ROASTER_TYPES_H

#include <stdbool.h>
#include <stdint.h>

#define ROASTER_PERMILLE_MAX 1000U
#define ROASTER_PROFILE_MAX_POINTS 16U
#define ROASTER_PROFILE_NAME_LEN 32U

enum roaster_state {
	ROASTER_STATE_BOOT = 0,
	ROASTER_STATE_SELF_TEST,
	ROASTER_STATE_IDLE,
	ROASTER_STATE_PREHEAT,
	ROASTER_STATE_READY,
	ROASTER_STATE_ROASTING,
	ROASTER_STATE_COOLING,
	ROASTER_STATE_COMPLETE,
	ROASTER_STATE_FAULT,
};

enum roaster_control_mode {
	ROASTER_CONTROL_MANUAL_POWER = 0,
	ROASTER_CONTROL_TEMPERATURE_HOLD,
	ROASTER_CONTROL_PROFILE_TRACKING,
};

enum temperature_sample_flags {
	TEMPERATURE_SAMPLE_VALID = 1u << 0,
	TEMPERATURE_SAMPLE_STALE = 1u << 1,
	TEMPERATURE_SAMPLE_OPEN_SENSOR = 1u << 2,
	TEMPERATURE_SAMPLE_SHORT_SENSOR = 1u << 3,
	TEMPERATURE_SAMPLE_OUT_OF_RANGE = 1u << 4,
};

struct temperature_sample {
	int32_t temperature_mdeg_c;
	uint64_t timestamp_ms;
	uint32_t flags;
};

struct roast_profile_point {
	uint32_t time_ms;
	int32_t temperature_mdeg_c;
};

struct roast_limits {
	int32_t max_target_mdeg_c;
	uint32_t max_profile_duration_ms;
	uint16_t max_heater_power_permille;
	int32_t max_ror_mdeg_c_per_min;
};

struct roast_profile {
	uint32_t id;
	char name[ROASTER_PROFILE_NAME_LEN];
	uint16_t point_count;
	struct roast_profile_point points[ROASTER_PROFILE_MAX_POINTS];
	struct roast_limits limits;
};

enum roast_fault_flag {
	ROAST_FAULT_TEMP_SENSOR_INVALID = 1u << 0,
	ROAST_FAULT_TEMP_SENSOR_STALE = 1u << 1,
	ROAST_FAULT_OVERTEMPERATURE = 1u << 2,
	ROAST_FAULT_EXCESSIVE_ROR = 1u << 3,
	ROAST_FAULT_HEATER_STUCK_ON = 1u << 4,
	ROAST_FAULT_HEATER_OPEN = 1u << 5,
	ROAST_FAULT_FAN_FAILURE = 1u << 6,
	ROAST_FAULT_MAINS_FAILURE = 1u << 7,
	ROAST_FAULT_WATCHDOG_RECOVERY = 1u << 8,
	ROAST_FAULT_PROFILE_INVALID = 1u << 9,
	ROAST_FAULT_INTERNAL_CONTROL_ERROR = 1u << 10,
	ROAST_FAULT_STOP_ASSERTED = 1u << 11,
};

enum roast_fault_severity {
	ROAST_FAULT_SEVERITY_WARNING = 0,
	ROAST_FAULT_SEVERITY_RECOVERABLE,
	ROAST_FAULT_SEVERITY_LATCHED,
	ROAST_FAULT_SEVERITY_EMERGENCY,
};

struct roast_telemetry {
	uint64_t elapsed_ms;
	int32_t temperature_mdeg_c;
	int32_t target_mdeg_c;
	int32_t ror_mdeg_c_per_min;
	uint16_t heater_request_permille;
	uint16_t heater_actual_permille;
	uint16_t fan_permille;
	uint32_t state;
	uint32_t fault_flags;
};

enum roast_event_type {
	ROAST_EVENT_CHARGE = 0,
	ROAST_EVENT_DRY_END,
	ROAST_EVENT_FIRST_CRACK,
	ROAST_EVENT_SECOND_CRACK,
	ROAST_EVENT_DROP,
	ROAST_EVENT_CUSTOM_MARK,
};

static inline uint16_t roaster_clamp_permille(uint16_t value)
{
	return value > ROASTER_PERMILLE_MAX ? ROASTER_PERMILLE_MAX : value;
}

#endif
