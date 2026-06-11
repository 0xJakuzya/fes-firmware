#ifndef PCA9685_H
#define PCA9685_H

#include <stdint.h>
#include "esp_err.h"

// Драйвер 16-канального I2C ШИМ-контроллера PCA9685 (ESP-IDF 5.x i2c_master).
// Единственный модуль, работающий с шиной I2C. Используется только из
// аппаратного слоя (hw_module / stimulation) — вызовы должны быть однопоточными.

// Инициализирует шину I2C и устройство, задаёт частоту несущей ШИМ из config.h
// и переводит все 16 выходов в LOW (безопасное состояние). Вызывать однократно.
esp_err_t pca9685_init(void);

// Устанавливает 12-битную скважность канала (0..15). value 0..4095 → off-счётчик
// (on=0). value 0 = выход постоянно LOW.
esp_err_t pca9685_set_pwm(uint8_t channel, uint16_t value);

// Полностью выключает канал (on=0, off=0 → постоянный LOW).
esp_err_t pca9685_set_off(uint8_t channel);

#endif
