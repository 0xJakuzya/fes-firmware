#ifndef STIMULATION_H
#define STIMULATION_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#include "protocol.h"  // stim_param_t

// Движок генерации биполярных импульсов для 8 каналов ФЭС (порт логики из
// ШУЕ_КОД.txt). Приватен за hw_module: прямые вызовы только из аппаратного слоя.
// Массив параметров каналов защищён мьютексом; рабочая задача читает снимок.

typedef struct {
    uint8_t  intensity;      // 0..255 (0 = канал выключен)
    uint8_t  frequency_hz;   // 1..100
    uint16_t pulse_width_us; // 100..15000 (на фазу)
} stimulation_channel_state_t;

// Инициализирует состояние каналов и создаёт задачу движка (PCA9685 должен быть
// уже инициализирован). Вывод изначально выключен (running = false).
esp_err_t stimulation_start(void);

// Глобальный разрешающий флаг вывода. false → все выходы LOW (параметры сохранены).
void stimulation_set_running(bool running);

// Аварийный сброс: немедленно выключить вывод всех каналов (running = false).
void stimulation_stop_all(void);

// Сеттеры одного канала (0..7). Значения клампятся в допустимый диапазон.
// Возвращают false при неверном индексе канала.
bool stimulation_set_intensity(uint8_t channel, uint8_t intensity);
bool stimulation_set_frequency(uint8_t channel, uint8_t frequency_hz);
bool stimulation_set_pulse_width(uint8_t channel, uint16_t pulse_width_us);

// Применить параметр param к каналу channel. Обёртка над сеттерами выше.
bool stimulation_set_param(uint8_t channel, stim_param_t param, uint16_t value);

// Применить параметр ко всем каналам.
void stimulation_set_all(stim_param_t param, uint16_t value);

// Снимок состояния одного канала. false при неверном индексе.
bool stimulation_get_channel(uint8_t channel, stimulation_channel_state_t *out);

#endif
