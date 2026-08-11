#ifndef ROASTER_SAFETY_FAULTS_H
#define ROASTER_SAFETY_FAULTS_H

#include <stdbool.h>
#include <stdint.h>

#include "domain/roaster_types.h"

const char *roast_fault_name(uint32_t fault_flag);
bool roast_fault_flags_require_latch(uint32_t fault_flags);

#endif
