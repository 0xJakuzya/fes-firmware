#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>

uint16_t read_u16(const uint8_t *data);
void     write_u16(uint8_t *data, uint16_t value);

uint16_t clamp_u16(uint16_t value, uint16_t low, uint16_t high);
uint16_t intensity_to_pwm(uint8_t intensity);
int      compute_phase(int64_t elapsed, uint32_t pw, uint32_t dead);
uint32_t effective_pulse_width(uint32_t pw, uint32_t dead, uint32_t period);

#endif
