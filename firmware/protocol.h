#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdbool.h>
#include <stdint.h>

// protocol characteristic
#define PROTOCOL_HEADER_SIZE       3 // size header
#define PROTOCOL_MAX_PAYLOAD_SIZE  128 // size payload array

#define PROTOCOL_RESULT_SIZE (PROTOCOL_HEADER_SIZE + PROTOCOL_RESULT_PAYLOAD_SIZE) // ask/error packet size
#define PROTOCOL_RESULT_PAYLOAD_SIZE 1 // size payload ask/error

// msg_type
#define PROTOCOL_TYPE_COMMAND      0xCC // command
#define PROTOCOL_TYPE_DATA         0xDD // data
#define PROTOCOL_TYPE_ERROR        0xEE // error

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
    RESULT_INVALID_TYPE    = 0x03,
} protocol_result_t;

// struct protocol request packet 
typedef struct {
    uint8_t seq_id;
    uint8_t com_id;
    uint8_t payload_length;
    uint8_t payload[PROTOCOL_MAX_PAYLOAD_SIZE - 1]; // without command_id
} protocol_request_t;

bool protocol_receive_request(int socket, protocol_request_t *request);
bool protocol_send_response(int socket, uint8_t seq_id, const uint8_t *payload, uint8_t payload_length);
bool protocol_send_error(int socket, uint8_t seq_id, protocol_result_t result);
bool protocol_send_ack(int socket, uint8_t seq_id);
#endif
