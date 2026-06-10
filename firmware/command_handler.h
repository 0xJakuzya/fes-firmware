#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include <stdbool.h>
#include <stdint.h>

#include "protocol.h"

bool command_handler_send_device_info(int socket, uint16_t sequence_id);
bool command_handler_process(int socket, const protocol_packet_t *packet);

#endif
