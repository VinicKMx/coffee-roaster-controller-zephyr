#include "storage/logger.h"

#include <zephyr/sys/printk.h>

void logger_emit_csv_header(void)
{
	printk("elapsed_ms,temperature_mdeg_c,target_mdeg_c,ror_mdeg_c_per_min,"
	       "heater_request_permille,heater_actual_permille,fan_permille,state,"
	       "fault_flags\n");
}

void logger_emit_telemetry(const struct roast_telemetry *telemetry)
{
	if (telemetry == NULL) {
		return;
	}

	printk("%llu,%d,%d,%d,%u,%u,%u,%u,0x%08x\n",
	       (unsigned long long)telemetry->elapsed_ms, telemetry->temperature_mdeg_c,
	       telemetry->target_mdeg_c, telemetry->ror_mdeg_c_per_min,
	       telemetry->heater_request_permille, telemetry->heater_actual_permille,
	       telemetry->fan_permille, telemetry->state, telemetry->fault_flags);
}
