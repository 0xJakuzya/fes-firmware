#ifndef DATA_STRUCT_H
#define DATA_STRUCT_H

#include <stdint.h>

#define PROTOCOL_MAX_PAYLOAD_SIZE 128

// struct protocol for request packet 
typedef struct {
    uint8_t seq_id;
    uint8_t command_id;
    uint8_t payload_length;
    uint8_t payload[PROTOCOL_MAX_PAYLOAD_SIZE - 1]; 
} request_t;

// msg_type
typedef enum {
    MSG_TYPE_COMMAND = 0xCC,
    MSG_TYPE_DATA    = 0xDD,
    MSG_TYPE_ERROR   = 0xEE,
} msg_type_t;

// command_id
typedef enum {
    GET_INFO            = 0x00,
    SET_WIFI_PASSWORD   = 0x01,
    SET_CHANNEL_PARAM   = 0x02,
    GET_CHANNEL_PARAM   = 0x03,
    START_CHANNEL       = 0x04,
    STOP_CHANNEL        = 0x05,
    ALARM               = 0x06,
    RESET_ERRORS        = 0x07,
    HEARTBEAT           = 0x08,
    SET_HEARTBEAT       = 0x09,
    GET_IMPEDANCE       = 0x0A,
    GET_BATTERY         = 0x0B,
} command_t;

// ask/error response
typedef enum {
    RESULT_OK             = 0x00,
    RESULT_INVALID_PARAM  = 0x01,
    RESULT_INVALID_LENGTH = 0x02,
    RESULT_INVALID_TYPE   = 0x03,
    RESULT_UNKNOWN_CMD    = 0x04,
} result_t;

typedef enum {
    INTENSITY   = 0x00,
    FREQUENCY   = 0x01,
    PULSE_WIDTH = 0x02,
} param_t;

// stimulation parameters (3 type)
typedef struct {
    uint8_t  intensity;
    uint8_t  frequency_hz; 
    uint16_t pulse_width_us; 
} channel_state_t;

typedef struct {
    uint8_t  in1;
    uint8_t  in2;
    uint8_t  intensity;
    uint8_t  frequency_hz;
    uint16_t pulse_width_us;
    int64_t  period_start_us;
    int8_t   current_phase;
} runtime_t;

typedef enum {
    CH_DISABLED = 0x00,
    CH_READY = 0x01,
    CH_RUNNING = 0x2,
} channel_status_t;

#endif