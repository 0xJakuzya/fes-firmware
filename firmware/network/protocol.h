#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdbool.h>
#include <stdint.h>

#include "data_struct.h"

#define PROTOCOL_HEADER_SIZE 3 // size header
#define PROTOCOL_RESULT_SIZE 4 // ask/error packet size (header_size + payload_size)
#define PROTOCOL_RESULT_PAYLOAD_SIZE 1 // size payload ask/error

bool protocol_receive_request(int socket, protocol_request_t *request);
bool protocol_send_response(int socket, uint8_t seq_id, const uint8_t *payload, uint8_t payload_length);
bool protocol_send_error(int socket, uint8_t seq_id, protocol_result_t result);
bool protocol_send_ack(int socket, uint8_t seq_id);

#endif
