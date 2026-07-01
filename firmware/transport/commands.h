#ifndef COMMANDS_H
#define COMMANDS_H

#include <stdbool.h>
#include <stdint.h>

#include "protocol.h"

#define FIRMWARE_VERSION_MAJOR  1
#define FIRMWARE_VERSION_MINOR  0
#define FIRMWARE_VERSION_PATCH  0

#define PROTOCOL_VERSION_MAJOR  1
#define PROTOCOL_VERSION_MINOR  0

#define DEVICE_INFO_PAYLOAD_SIZE 6

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
    SET_HEARTBEAT       = 0x08,
    GET_IMPEDANCE       = 0x0A,
    GET_BATTERY         = 0x0B,
} command_t;

bool command_handler(int socket, const request_t *request);
bool get_info(int socket, uint8_t seq_id);

#endif
