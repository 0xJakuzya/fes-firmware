#ifndef COMMANDS_H
#define COMMANDS_H

#include <stdbool.h>
#include <stdint.h>

#include "protocol.h"

bool command_handler_process(int socket, const protocol_request_t *req);

#endif
