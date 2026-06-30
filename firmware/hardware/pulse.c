#include "pulse.h"
#include "pca9685.h"
#include "utils.h"

#define PHASE_OFF   (-2)
#define PHASE_RESET (-1)

// drive both outputs for the given phase
static void apply_phase(runtime_t *c, int phase, uint16_t pwm_value)
{
    switch (phase) {
    // positive pulse: in1 high, in2 at intensity
    case 0:
        pca9685_set_pwm(c->in1, PCA9685_PWM_MAX);
        pca9685_set_pwm(c->in2, pwm_value);
        break;
    // dead time: in2 off
    case 1:
        pca9685_set_pwm(c->in2, 0);
        break;
    // negative pulse: in1 off, in2 at intensity
    case 2:
        pca9685_set_pwm(c->in1, 0);
        pca9685_set_pwm(c->in2, pwm_value);
        break;
    // dead time / pause: in2 off
    case 3:
    case 4:
        pca9685_set_pwm(c->in2, 0);
        break;
    default:
        break;
    }
}

// advance one channels waveform by current time
void pulse_service_channel(runtime_t *c, int64_t now)
{
    // if not running or no current --> outputs off
    if (c->status != CH_RUNNING || c->intensity == 0) {
        // turn off once
        if (c->current_phase != PHASE_OFF) {
            pca9685_set_off(c->in1);
            pca9685_set_off(c->in2);
            c->current_phase = PHASE_OFF;
        }
        c->period_start_us = now;
        return;
    }

    // period and pulse width
    uint32_t period = FES_PERIOD_BASE_US / c->frequency_hz;
    uint32_t dead   = FES_DEAD_TIME_US;
    uint32_t pw     = effective_pulse_width(c->pulse_width_us, dead, period);

    // time into period + pwm level
    int64_t  elapsed   = now - c->period_start_us;
    uint16_t pwm_value = intensity_to_pwm(c->intensity);

    // apply only on phase change
    int phase = compute_phase(elapsed, pw, dead);
    if (phase != c->current_phase) {
        apply_phase(c, phase, pwm_value);
        c->current_phase = phase;
    }

    // period ended --> restart
    if (elapsed >= (int64_t)period) {
        c->period_start_us = now;
        c->current_phase   = PHASE_RESET;
    }
}
