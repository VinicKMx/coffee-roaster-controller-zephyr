#include "application/telemetry.h"

#include <string.h>

void roast_telemetry_clear(struct roast_telemetry *telemetry)
{
	if (telemetry == NULL) {
		return;
	}

	memset(telemetry, 0, sizeof(*telemetry));
	telemetry->state = ROASTER_STATE_BOOT;
}
