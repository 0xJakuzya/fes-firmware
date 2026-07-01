#include "utils.h"
#include "pca9685.h"     
#include "stimulation.h" 
#include "esp_timer.h"

// read little-endian u16
uint16_t read_u16(const uint8_t *data)
{
    return (uint16_t)(data[0] | ((uint16_t)data[1] << 8));
}

// write little-endian u16
void write_u16(uint8_t *data, uint16_t value)
{
    data[0] = (uint8_t)(value & 0xFF);
    data[1] = (uint8_t)(value >> 8);
}

int64_t heartbeat_now_ms(void)
{
    return esp_timer_get_time() / 1000;
}

// clamp value to [low, high]
uint16_t clamp_u16(uint16_t value, uint16_t low, uint16_t high)
{
    if (value < low) return low;
    if (value > high) return high;
    return value;
}

// map intensity 0..255 -> pwm 0..4095
uint16_t intensity_to_pwm(uint8_t intensity)
{
    return (uint16_t)(((uint32_t)intensity * PCA9685_PWM_MAX) / FES_INTENSITY_MAX);
}

// pick waveform phase by elapsed time
int compute_phase(int64_t elapsed, uint32_t pw, uint32_t dead)
{
    // phase 0: positive pulse
    if (elapsed < (int64_t)pw)                     return 0;
    // phase 1: dead time
    if (elapsed < (int64_t)pw + dead)              return 1;
    // phase 2: negative pulse
    if (elapsed < (int64_t)2 * pw + dead)          return 2;
    // phase 3: dead time
    if (elapsed < (int64_t)2 * pw + 2 * dead)      return 3;
    // phase 4: inter-pulse pause
    return 4;
}

// clamp pulse width so it fits the period
uint32_t effective_pulse_width(uint32_t pw, uint32_t dead, uint32_t period)
{
    // if active phases exceed period --> shrink pw
    if (2 * pw + 2 * dead > period) {
        pw = (period - 2 * dead) / 2;
    }
    return pw;
}
