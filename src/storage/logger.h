#ifndef ROASTER_STORAGE_LOGGER_H
#define ROASTER_STORAGE_LOGGER_H

#include "domain/roaster_types.h"

void logger_emit_csv_header(void);
void logger_emit_telemetry(const struct roast_telemetry *telemetry);

#endif
