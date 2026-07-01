#ifndef STIMULATION_H
#define STIMULATION_H

#include <stdbool.h>
#include <stdint.h>

#define FES_CHANNEL_COUNT           8
#define FES_DEAD_TIME_US            200
#define FES_PERIOD_BASE_US          1000000UL

#define FES_INTENSITY_MIN           0
#define FES_INTENSITY_MAX           255
#define FES_FREQUENCY_MIN_HZ        1
#define FES_FREQUENCY_MAX_HZ        100
#define FES_PULSE_WIDTH_MIN_US      100
#define FES_PULSE_WIDTH_MAX_US      15000

#define FES_DEFAULT_FREQUENCY_HZ    50
#define FES_DEFAULT_PULSE_WIDTH_US  300

#define FES_CH_IN1(n)  ((uint8_t)(2 * (n)))
#define FES_CH_IN2(n)  ((uint8_t)(2 * (n) + 1))

#define FES_TASK_STACK_SIZE         4096
#define FES_TASK_PRIORITY           10
#define FES_TASK_CORE               1

#define PULSE_PHASE_OFF (-2)

typedef enum {
    INTENSITY   = 0x00,
    FREQUENCY   = 0x01,
    PULSE_WIDTH = 0x02,
} param_t;

typedef enum {
    CH_DISABLED = 0x00,
    CH_READY = 0x01,
    CH_RUNNING = 0x02,
} channel_status_t;

typedef struct {
    uint8_t          intensity;
    uint8_t          frequency_hz;
    uint16_t         pulse_width_us;
    channel_status_t status;
} channel_state_t;

typedef struct {
    uint8_t          in1;
    uint8_t          in2;
    uint8_t          intensity;
    uint8_t          frequency_hz;
    uint16_t         pulse_width_us;
    channel_status_t status;
    int64_t          period_start_us;
    int8_t           current_phase;
} runtime_t;

typedef enum {
    FIELD_INTENSITY,
    FIELD_FREQUENCY,
    FIELD_PULSE_WIDTH,
} channel_field_t;

void stimulation_start(void);
bool stimulation_start_channel(uint8_t channel);
bool stimulation_stop_channel(uint8_t channel);
void stimulation_stop_all(void);
bool stimulation_set_intensity(uint8_t channel, uint8_t intensity);
bool stimulation_set_frequency(uint8_t channel, uint8_t frequency_hz);
bool stimulation_set_pulse_width(uint8_t channel, uint16_t pulse_width_us);
bool stimulation_set_param(uint8_t channel, param_t param, uint16_t value);
bool stimulation_get_channel(uint8_t channel, channel_state_t *out);

#endif
