#ifndef STIMULATION_H
#define STIMULATION_H

#include <stdbool.h>
#include <stdint.h>

#include "protocol.h" 

typedef struct {
    uint8_t  intensity;     
    uint8_t  frequency_hz;  
    uint16_t pulse_width_us; 
} stimulation_channel_state_t;

void stimulation_start(void);

void stimulation_set_running(bool running);
void stimulation_stop_all(void);

bool stimulation_set_intensity(uint8_t channel, uint8_t intensity);
bool stimulation_set_frequency(uint8_t channel, uint8_t frequency_hz);
bool stimulation_set_pulse_width(uint8_t channel, uint16_t pulse_width_us);

bool stimulation_set_param(uint8_t channel, stim_param_t param, uint16_t value);
void stimulation_set_all(stim_param_t param, uint16_t value);

bool stimulation_get_channel(uint8_t channel, stimulation_channel_state_t *out);

#endif
