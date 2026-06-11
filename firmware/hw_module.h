#ifndef HW_MODULE_H
#define HW_MODULE_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#include "stimulation.h"  // stimulation_channel_state_t, stim_param_t

// Абстракция «Аппаратного модуля». Сейчас реализована поверх stimulation+pca9685
// внутри того же ESP32; граница спроектирована под возможный вынос на отдельный
// MCU (тогда меняется только реализация hw_module.c, транспорт не затрагивается).
// Телеметрия и реальный детект ошибок каналов — Этап 2 (сейчас заглушки).

typedef enum {
    HW_RESULT_OK = 0,
    HW_RESULT_INVALID_PARAM,
    HW_RESULT_VALIDATION_FAILED,
} hw_result_t;

// Инициализирует PCA9685 и движок стимуляции; выходы в безопасном LOW.
esp_err_t hw_module_init(void);

// Валидация загруженных параметров до запуска стимуляции.
hw_result_t hw_module_validate(void);

// Запуск выполнения программы стимуляции. false, если защёлкнута ошибка.
bool hw_module_start(void);

// Остановка стимуляции (параметры сохраняются).
bool hw_module_stop(void);

// Аварийная блокировка всех каналов (высший приоритет) + защёлка ошибки.
void hw_module_alarm(void);

// Снятие защёлки ошибки (после устранения причины верхним уровнем).
bool hw_module_reset_error(void);

// Установка одного параметра канала (значение клампится). false при неверном
// канале/параметре.
bool hw_module_set_channel(uint8_t channel, stim_param_t param, uint16_t value);

// Установка параметра всех каналов.
void hw_module_set_all(stim_param_t param, uint16_t value);

// Снимок состояния канала.
bool hw_module_get_channel(uint8_t channel, stimulation_channel_state_t *out);

#endif
