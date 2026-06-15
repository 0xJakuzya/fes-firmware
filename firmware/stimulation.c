#include "stimulation.h"
#include "config.h"
#include "pca9685.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_timer.h"

#define PHASE_OFF   (-2)  
#define PHASE_RESET (-1)

typedef struct {
    uint8_t  in1;           
    uint8_t  in2;           
    uint8_t  intensity;     
    uint8_t  frequency_hz;
    uint16_t pulse_width_us;
    int64_t  period_start_us;
    int8_t   current_phase;
} stim_runtime_t;

static stimulation_channel_state_t s_params[FES_CHANNEL_COUNT];  
static bool s_running;                                           
static stim_runtime_t s_runtime[FES_CHANNEL_COUNT];              
static SemaphoreHandle_t s_mutex;
static TaskHandle_t s_task;


static uint16_t clamp_u16(uint16_t value, uint16_t low, uint16_t high)
{
    if (value < low) {
        return low;
    }
    if (value > high) {
        return high;
    }
    return value;
}

static uint16_t intensity_to_pwm(uint8_t intensity)
{
    return (uint16_t)(((uint32_t)intensity * PCA9685_PWM_MAX) / 255U);
}

static int compute_phase(int64_t elapsed, uint32_t pw, uint32_t dead)
{
    if (elapsed < (int64_t)pw) {
        return 0;  // прямая фаза
    }
    if (elapsed < (int64_t)pw + dead) {
        return 1;  // пауза 1 
    }
    if (elapsed < (int64_t)2 * pw + dead) {
        return 2;  // обратная фаза
    }
    if (elapsed < (int64_t)2 * pw + 2 * dead) {
        return 3;  // пауза 2 
    }
    return 4;  
}

static void apply_phase(stim_runtime_t *c, int phase, uint16_t pwm_value)
{
    switch (phase) {
    case 0:  // Forward: IN1=HIGH, IN2=PWM → ток OUT1→OUT2
        pca9685_set_pwm(c->in1, PCA9685_PWM_MAX);
        pca9685_set_pwm(c->in2, pwm_value);
        break;
    case 1:  // пауза 1: IN2=LOW (coast)
        pca9685_set_pwm(c->in2, 0);
        break;
    case 2:  // Reverse: IN1=LOW, IN2=PWM → ток OUT2→OUT1
        pca9685_set_pwm(c->in1, 0);
        pca9685_set_pwm(c->in2, pwm_value);
        break;
    case 3:  // пауза 2: IN2=LOW (coast)
        pca9685_set_pwm(c->in2, 0);
        break;
    case 4:  // ожидание конца периода: IN2=LOW
        pca9685_set_pwm(c->in2, 0);
        break;
    default:
        break;
    }
}

static void service_channel(stim_runtime_t *c, int64_t now, bool running)
{
    // Канал выключен (стоп/интенсивность 0) → удерживать выходы в LOW
    if (!running || c->intensity == 0) {
        if (c->current_phase != PHASE_OFF) {
            pca9685_set_off(c->in1);
            pca9685_set_off(c->in2);
            c->current_phase = PHASE_OFF;
        }
        c->period_start_us = now; 
        return;
    }

    uint32_t period = 1000000UL / c->frequency_hz;
    uint32_t pw = c->pulse_width_us;
    uint32_t dead = FES_DEAD_TIME_US;

    if (2 * pw + 2 * dead > period) {
        pw = (period - 2 * dead) / 2;
    }

    int64_t elapsed = now - c->period_start_us;
    uint16_t pwm_value = intensity_to_pwm(c->intensity);

    int phase = compute_phase(elapsed, pw, dead);
    if (phase != c->current_phase) {
        apply_phase(c, phase, pwm_value);
        c->current_phase = phase;
    }

    if (elapsed >= (int64_t)period) {
        c->period_start_us = now;
        c->current_phase = PHASE_RESET;
    }
}

static void stimulation_task(void *arg)
{
    (void)arg;
    for (;;) {
      
        xSemaphoreTake(s_mutex, portMAX_DELAY);
        bool running = s_running;
        for (int i = 0; i < FES_CHANNEL_COUNT; ++i) {
            s_runtime[i].intensity = s_params[i].intensity;
            s_runtime[i].frequency_hz = s_params[i].frequency_hz;
            s_runtime[i].pulse_width_us = s_params[i].pulse_width_us;
        }
        xSemaphoreGive(s_mutex);

        int64_t now = esp_timer_get_time();
        for (int i = 0; i < FES_CHANNEL_COUNT; ++i) {
            service_channel(&s_runtime[i], now, running);
        }

        vTaskDelay(1); 
    }
}


void stimulation_start(void)
{
    s_mutex = xSemaphoreCreateMutex();
    int64_t now = esp_timer_get_time();

    for (int i = 0; i < FES_CHANNEL_COUNT; ++i) {
        s_params[i].intensity = 0;
        s_params[i].frequency_hz = FES_DEFAULT_FREQUENCY_HZ;
        s_params[i].pulse_width_us = FES_DEFAULT_PULSE_WIDTH_US;

        s_runtime[i].in1 = FES_CH_IN1(i);
        s_runtime[i].in2 = FES_CH_IN2(i);
        s_runtime[i].intensity = 0;
        s_runtime[i].frequency_hz = FES_DEFAULT_FREQUENCY_HZ;
        s_runtime[i].pulse_width_us = FES_DEFAULT_PULSE_WIDTH_US;
        s_runtime[i].period_start_us = now;
        s_runtime[i].current_phase = PHASE_OFF;
    }
    s_running = false;

    xTaskCreatePinnedToCore(stimulation_task, "stim", FES_TASK_STACK_SIZE, NULL,
        FES_TASK_PRIORITY, &s_task, FES_TASK_CORE);
}

void stimulation_set_running(bool running)
{
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    s_running = running;
    xSemaphoreGive(s_mutex);
}

void stimulation_stop_all(void)
{
    stimulation_set_running(false);
}

bool stimulation_set_intensity(uint8_t channel, uint8_t intensity)
{
    if (channel >= FES_CHANNEL_COUNT) {
        return false;
    }
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    s_params[channel].intensity = (uint8_t)clamp_u16(intensity, FES_INTENSITY_MIN, FES_INTENSITY_MAX);
    xSemaphoreGive(s_mutex);
    return true;
}

bool stimulation_set_frequency(uint8_t channel, uint8_t frequency_hz)
{
    if (channel >= FES_CHANNEL_COUNT) {
        return false;
    }
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    s_params[channel].frequency_hz = (uint8_t)clamp_u16(frequency_hz, FES_FREQUENCY_MIN_HZ, FES_FREQUENCY_MAX_HZ);
    xSemaphoreGive(s_mutex);
    return true;
}

bool stimulation_set_pulse_width(uint8_t channel, uint16_t pulse_width_us)
{
    if (channel >= FES_CHANNEL_COUNT) {
        return false;
    }
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    s_params[channel].pulse_width_us = clamp_u16(pulse_width_us, FES_PULSE_WIDTH_MIN_US, FES_PULSE_WIDTH_MAX_US);
    xSemaphoreGive(s_mutex);
    return true;
}

bool stimulation_set_param(uint8_t channel, stim_param_t param, uint16_t value)
{
    switch (param) {
    case STIM_PARAM_INTENSITY:
        return stimulation_set_intensity(channel, (uint8_t)value);
    case STIM_PARAM_FREQUENCY:
        return stimulation_set_frequency(channel, (uint8_t)value);
    case STIM_PARAM_PULSE_WIDTH:
        return stimulation_set_pulse_width(channel, value);
    default:
        return false;
    }
}

void stimulation_set_all(stim_param_t param, uint16_t value)
{
    for (uint8_t ch = 0; ch < FES_CHANNEL_COUNT; ++ch) {
        stimulation_set_param(ch, param, value);
    }
}

bool stimulation_get_channel(uint8_t channel, stimulation_channel_state_t *out)
{
    if (channel >= FES_CHANNEL_COUNT || out == NULL) {
        return false;
    }
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    *out = s_params[channel];
    xSemaphoreGive(s_mutex);
    return true;
}
