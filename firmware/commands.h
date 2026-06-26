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

bool command_handler(int socket, const request_t *request);
bool get_info(int socket, uint8_t seq_id);

#endif
