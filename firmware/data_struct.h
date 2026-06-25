#ifndef DATA_STRUCT_H
#define DATA_STRUCT_H

#include <stdint.h>

#define PROTOCOL_MAX_PAYLOAD_SIZE 128

// struct protocol for request packet 
typedef struct {
    uint8_t seq_id;
    uint8_t com_id;
    uint8_t payload_length;
    uint8_t payload[PROTOCOL_MAX_PAYLOAD_SIZE - 1]; 
} protocol_request_t;

// msg_type
typedef enum {
    MSG_TYPE_COMMAND = 0xCC,
    MSG_TYPE_DATA    = 0xDD,
    MSG_TYPE_ERROR   = 0xEE,
} protocol_msg_type_t;

// command_id
typedef enum {
    COM_GET_INFO          = 0x03,
    COM_SET_WIFI_PASSWORD = 0x04,
    COM_SET_CHANNEL_PARAM = 0x05,
    COM_SET_ALL_PARAM     = 0x06,
    COM_GET_CHANNEL       = 0x07,
    COM_START             = 0x08,
    COM_STOP              = 0x09,
    COM_ALARM             = 0x0A,
    COM_RESET_ERRORS      = 0x0B,
    COM_HEARTBEAT         = 0x0C,
    COM_SET_HEARTBEAT     = 0x0D,
    COM_GET_TELEMETRY     = 0x0E,
    COM_GET_BATTERY       = 0x0F,
} protocol_command_t;

// ask/error response
typedef enum {
    RESULT_OK             = 0x00,
    RESULT_INVALID_PARAM  = 0x01,
    RESULT_INVALID_LENGTH = 0x02,
    RESULT_INVALID_TYPE   = 0x03,
    RESULT_UNKNOWN_CMD    = 0x04,
    RESULT_INVALID_CRC    = 0x05,
} protocol_result_t;

typedef enum {
    STIM_PARAM_INTENSITY   = 0x00,
    STIM_PARAM_FREQUENCY   = 0x01,
    STIM_PARAM_PULSE_WIDTH = 0x02,
} stim_param_t;

// get_info
typedef struct {
    uint8_t major;
    uint8_t minor;
    uint8_t patch;
} version_t;

typedef struct {
    version_t firmware_version;
    version_t protocol_version;
    uint8_t   channel_count;
    uint8_t   status;
} device_info_t;

// stimulation parameters (3 type)
typedef struct {
    uint8_t  intensity;
    uint8_t  frequency_hz; 
    uint16_t pulse_width_us; 
} stimulation_channel_state_t;

#endif
