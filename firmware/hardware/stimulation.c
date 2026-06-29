#include "stimulation.h"
#include "pca9685.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_timer.h"
#include "esp_task_wdt.h"

#define PHASE_OFF   (-2)
#define PHASE_RESET (-1)

static channel_state_t s_params[FES_CHANNEL_COUNT];
static runtime_t       s_runtime[FES_CHANNEL_COUNT];
static SemaphoreHandle_t           s_mutex;
static TaskHandle_t                s_task;

static uint16_t clamp_u16(uint16_t value, uint16_t low, uint16_t high)
{
    if (value < low) return low;
    if (value > high) return high;
    return value;
}

static uint16_t intensity_to_pwm(uint8_t intensity)
{
    return (uint16_t)(((uint32_t)intensity * PCA9685_PWM_MAX) / FES_INTENSITY_MAX);
}

static int compute_phase(int64_t elapsed, uint32_t pw, uint32_t dead)
{
    if (elapsed < (int64_t)pw)                     return 0;
    if (elapsed < (int64_t)pw + dead)              return 1;
    if (elapsed < (int64_t)2 * pw + dead)          return 2;
    if (elapsed < (int64_t)2 * pw + 2 * dead)      return 3;
    return 4;
}

static void apply_phase(runtime_t *c, int phase, uint16_t pwm_value)
{
    switch (phase) {
    case 0:
        pca9685_set_pwm(c->in1, PCA9685_PWM_MAX);
        pca9685_set_pwm(c->in2, pwm_value);
        break;
    case 1:
        pca9685_set_pwm(c->in2, 0);
        break;
    case 2:
        pca9685_set_pwm(c->in1, 0);
        pca9685_set_pwm(c->in2, pwm_value);
        break;
    case 3:
    case 4:
        pca9685_set_pwm(c->in2, 0);
        break;
    default:
        break;
    }
}

static uint32_t effective_pulse_width(uint32_t pw, uint32_t dead, uint32_t period)
{
    if (2 * pw + 2 * dead > period) {
        pw = (period - 2 * dead) / 2;
    }
    return pw;
}

static void service_channel(runtime_t *c, int64_t now)
{
    if (c->status != CH_RUNNING || c->intensity == 0) {
        if (c->current_phase != PHASE_OFF) {
            pca9685_set_off(c->in1);
            pca9685_set_off(c->in2);
            c->current_phase = PHASE_OFF;
        }
        c->period_start_us = now;
        return;
    }

    uint32_t period = FES_PERIOD_BASE_US / c->frequency_hz;
    uint32_t dead   = FES_DEAD_TIME_US;
    uint32_t pw     = effective_pulse_width(c->pulse_width_us, dead, period);

    int64_t  elapsed   = now - c->period_start_us;
    uint16_t pwm_value = intensity_to_pwm(c->intensity);

    int phase = compute_phase(elapsed, pw, dead);
    if (phase != c->current_phase) {
        apply_phase(c, phase, pwm_value);
        c->current_phase = phase;
    }

    if (elapsed >= (int64_t)period) {
        c->period_start_us = now;
        c->current_phase   = PHASE_RESET;
    }
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
            service_channel(&s_runtime[i], now);
        }

        vTaskDelay(1);
    }
}

void stimulation_start(void)
{
    s_mutex = xSemaphoreCreateMutex();
    int64_t now = esp_timer_get_time();

    for (int i = 0; i < FES_CHANNEL_COUNT; ++i) {
        s_params[i].intensity      = 0;
        s_params[i].frequency_hz   = FES_DEFAULT_FREQUENCY_HZ;
        s_params[i].pulse_width_us = FES_DEFAULT_PULSE_WIDTH_US;
        s_params[i].status         = CH_DISABLED;
        s_runtime[i].in1            = FES_CH_IN1(i);
        s_runtime[i].in2            = FES_CH_IN2(i);
        s_runtime[i].intensity      = 0;
        s_runtime[i].frequency_hz   = FES_DEFAULT_FREQUENCY_HZ;
        s_runtime[i].pulse_width_us = FES_DEFAULT_PULSE_WIDTH_US;
        s_runtime[i].status         = CH_DISABLED;
        s_runtime[i].period_start_us = now;
        s_runtime[i].current_phase  = PHASE_OFF;
    }

    xTaskCreatePinnedToCore(stimulation_task, "stim", FES_TASK_STACK_SIZE, NULL,
        FES_TASK_PRIORITY, &s_task, FES_TASK_CORE);
}

bool stimulation_start_channel(uint8_t channel)
{
    if (channel >= FES_CHANNEL_COUNT) return false;
    bool ok = false;
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    if (s_params[channel].status != CH_DISABLED) {
        s_params[channel].status = CH_RUNNING;
        ok = true;
    }
    xSemaphoreGive(s_mutex);
    return ok;
}

bool stimulation_stop_channel(uint8_t channel)
{
    if (channel >= FES_CHANNEL_COUNT) return false;
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    if (s_params[channel].status == CH_RUNNING) {
        s_params[channel].status = CH_READY;
    }
    xSemaphoreGive(s_mutex);
    return true;
}

void stimulation_stop_all(void)
{
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    for (int i = 0; i < FES_CHANNEL_COUNT; ++i) {
        if (s_params[i].status == CH_RUNNING) {
            s_params[i].status = CH_READY;
        }
    }
    xSemaphoreGive(s_mutex);
}

typedef enum {
    FIELD_INTENSITY,
    FIELD_FREQUENCY,
    FIELD_PULSE_WIDTH,
} channel_field_t;

static bool set_param_locked(uint8_t channel, channel_field_t field,
                             uint16_t value, uint16_t low, uint16_t high)
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
    return set_param_locked(channel, FIELD_INTENSITY, intensity,
                            FES_INTENSITY_MIN, FES_INTENSITY_MAX);
}

bool stimulation_set_frequency(uint8_t channel, uint8_t frequency_hz)
{
    return set_param_locked(channel, FIELD_FREQUENCY, frequency_hz,
                            FES_FREQUENCY_MIN_HZ, FES_FREQUENCY_MAX_HZ);
}

bool stimulation_set_pulse_width(uint8_t channel, uint16_t pulse_width_us)
{
    return set_param_locked(channel, FIELD_PULSE_WIDTH, pulse_width_us,
                            FES_PULSE_WIDTH_MIN_US, FES_PULSE_WIDTH_MAX_US);
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
