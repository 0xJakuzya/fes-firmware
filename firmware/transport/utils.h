#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>

uint16_t read_u16(const uint8_t *data);
void     write_u16(uint8_t *data, uint16_t value);

#endif
