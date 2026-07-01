#ifndef PULSE_H
#define PULSE_H

#include <stdint.h>
#include "stimulation.h"

void pulse_service_channel(runtime_t *c, int64_t now);

#endif
