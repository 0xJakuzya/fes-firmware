#ifndef PCA9685_H
#define PCA9685_H

#include <stdint.h>

#define PCA9685_SDA_GPIO        14
#define PCA9685_SCL_GPIO        15

#define PCA9685_I2C_FREQ_HZ     400000
#define PCA9685_I2C_ADDR        0x40
#define PCA9685_OSC_HZ          25000000
#define PCA9685_PWM_FREQ_HZ     1000
#define PCA9685_PWM_MAX         4095
#define PCA9685_I2C_TIMEOUT_MS  50

#define PCA9685_REG_MODE1       0x00
#define PCA9685_REG_MODE2       0x01
#define PCA9685_REG_LED0_ON_L   0x06
#define PCA9685_REG_PRESCALE    0xFE

#define PCA9685_MODE1_RESTART   0x80
#define PCA9685_MODE1_AI        0x20
#define PCA9685_MODE1_SLEEP     0x10
#define PCA9685_PRESCALE_MIN    3

void pca9685_init(void);
void pca9685_set_pwm(uint8_t channel, uint16_t value);
void pca9685_set_off(uint8_t channel);

#endif
