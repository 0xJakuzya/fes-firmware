#ifndef PCA9685_H
#define PCA9685_H

#include <stdint.h>

void pca9685_init(void);
void pca9685_set_pwm(uint8_t channel, uint16_t value);
void pca9685_set_off(uint8_t channel);

#endif
