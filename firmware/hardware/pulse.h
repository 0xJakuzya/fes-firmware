#ifndef PULSE_H
#define PULSE_H

#include <stdint.h>
#include "stimulation.h"

// Начальное значение runtime_t.current_phase: выходы выключены.
#define PULSE_PHASE_OFF (-2)

void pulse_service_channel(runtime_t *c, int64_t now);

#endif
