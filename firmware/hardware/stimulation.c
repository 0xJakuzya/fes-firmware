#include "stimulation.h"
#include "pulse.h"
#include "pca9685.h"
#include "utils.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_timer.h"
#include "esp_task_wdt.h"

static channel_state_t s_params[FES_CHANNEL_COUNT];
static runtime_t       s_runtime[FES_CHANNEL_COUNT];
static SemaphoreHandle_t           s_mutex;
static TaskHandle_t                s_task;

bool stimulation_start_channel(uint8_t channel)
{
    if (channel >= FES_CHANNEL_COUNT) return false;
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    bool ok = s_params[channel].status != CH_DISABLED;
    if (ok) s_params[channel].status = CH_RUNNING;
    xSemaphoreGive(s_mutex);
    return ok;
}

bool stimulation_stop_channel(uint8_t channel)
{
    if (channel >= FES_CHANNEL_COUNT) return false;
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    if (s_params[channel].status == CH_RUNNING) s_params[channel].status = CH_READY;
    xSemaphoreGive(s_mutex);
    return true;
}

static bool set_param_locked(uint8_t channel, channel_field_t field, uint16_t value, uint16_t low, uint16_t high)
{
    if (channel >= FES_CHANNEL_COUNT) return false;
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    uint16_t clamped = clamp_u16(value, low, high);
    switch (field) {
    case FIELD_INTENSITY:
        s_params[channel].intensity = (uint8_t)clamped;
        if (clamped == 0) {
            s_params[channel].status = CH_DISABLED;
        } else if (s_params[channel].status == CH_DISABLED) {
            s_params[channel].status = CH_READY;
        }
        break;
    case FIELD_FREQUENCY:     s_params[channel].frequency_hz   = (uint8_t)clamped; break;
    case FIELD_PULSE_WIDTH:   s_params[channel].pulse_width_us = clamped;          break;
    }
    xSemaphoreGive(s_mutex);
    return true;
}

bool stimulation_set_intensity(uint8_t channel, uint8_t intensity)
{
    return set_param_locked(channel, FIELD_INTENSITY, intensity, FES_INTENSITY_MIN, FES_INTENSITY_MAX);
}

bool stimulation_set_frequency(uint8_t channel, uint8_t frequency_hz)
{
    return set_param_locked(channel, FIELD_FREQUENCY, frequency_hz, FES_FREQUENCY_MIN_HZ, FES_FREQUENCY_MAX_HZ);
}

bool stimulation_set_pulse_width(uint8_t channel, uint16_t pulse_width_us)
{
    return set_param_locked(channel, FIELD_PULSE_WIDTH, pulse_width_us, FES_PULSE_WIDTH_MIN_US, FES_PULSE_WIDTH_MAX_US);
}

void stimulation_stop_all(void)
{
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    for (int i = 0; i < FES_CHANNEL_COUNT; ++i) {
        if (s_params[i].status == CH_RUNNING) s_params[i].status = CH_READY;
    }
    xSemaphoreGive(s_mutex);
}

bool stimulation_set_param(uint8_t channel, param_t param, uint16_t value)
{
    switch (param) {
    case INTENSITY:   return stimulation_set_intensity(channel, (uint8_t)value);
    case FREQUENCY:   return stimulation_set_frequency(channel, (uint8_t)value);
    case PULSE_WIDTH: return stimulation_set_pulse_width(channel, value);
    default:                     return false;
    }
}

bool stimulation_get_channel(uint8_t channel, channel_state_t *out)
{
    if (channel >= FES_CHANNEL_COUNT) return false;
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    *out = s_params[channel];
    xSemaphoreGive(s_mutex);
    return true;
}

static void stimulation_task(void *arg)
{
    (void)arg;
    pca9685_init();
    esp_task_wdt_add(NULL);
    for (;;) {
        esp_task_wdt_reset();
        xSemaphoreTake(s_mutex, portMAX_DELAY);

        for (int i = 0; i < FES_CHANNEL_COUNT; ++i) {
            s_runtime[i].intensity      = s_params[i].intensity;
            s_runtime[i].frequency_hz   = s_params[i].frequency_hz;
            s_runtime[i].pulse_width_us = s_params[i].pulse_width_us;
            s_runtime[i].status         = s_params[i].status;
        }

        xSemaphoreGive(s_mutex);
        int64_t now = esp_timer_get_time();

        for (int i = 0; i < FES_CHANNEL_COUNT; ++i) {
            pulse_service_channel(&s_runtime[i], now);
        }
        vTaskDelay(1);
    }
}

void stimulation_start(void)
{
    s_mutex = xSemaphoreCreateMutex();
    int64_t now = esp_timer_get_time();

    for (int i = 0; i < FES_CHANNEL_COUNT; ++i) {
        s_params[i] = (channel_state_t){
            .frequency_hz   = FES_DEFAULT_FREQUENCY_HZ,
            .pulse_width_us = FES_DEFAULT_PULSE_WIDTH_US,
            .status         = CH_DISABLED,
        };
        s_runtime[i] = (runtime_t){
            .in1 = FES_CH_IN1(i), .in2 = FES_CH_IN2(i),
            .period_start_us = now, .current_phase = PULSE_PHASE_OFF,
        };
    }

    xTaskCreatePinnedToCore(stimulation_task, "stim", FES_TASK_STACK_SIZE, NULL,
        FES_TASK_PRIORITY, &s_task, FES_TASK_CORE);
}
