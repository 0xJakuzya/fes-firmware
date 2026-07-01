#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdbool.h>
#include <stdint.h>

#define PROTOCOL_MAX_PAYLOAD_SIZE 128
#define PROTOCOL_HEADER_SIZE 3 // size header
#define PROTOCOL_RESULT_SIZE 4 // ask/error packet size (header_size + payload_size)
#define PROTOCOL_RESULT_PAYLOAD_SIZE 1 // size payload ask/error

typedef struct {
    uint8_t seq_id;
    uint8_t command_id;
    uint8_t payload_length;
    uint8_t payload[PROTOCOL_MAX_PAYLOAD_SIZE - 1];
} request_t;

typedef enum {
    MSG_TYPE_COMMAND = 0xCC,
    MSG_TYPE_DATA    = 0xDD,
    MSG_TYPE_ERROR   = 0xEE,
} msg_type_t;

typedef enum {
    RESULT_OK             = 0x00,
    RESULT_INVALID_PARAM  = 0x01,
    RESULT_INVALID_LENGTH = 0x02,
    RESULT_INVALID_TYPE   = 0x03,
    RESULT_UNKNOWN_CMD    = 0x04,
} result_t;

typedef struct {
    uint8_t msg_type;
    uint8_t seq_id;
    uint8_t payload_length;
    uint8_t payload[PROTOCOL_MAX_PAYLOAD_SIZE];
} protocol_frame_t;

typedef enum {
    PROTOCOL_RECV_OK,
    PROTOCOL_RECV_WAIT,
    PROTOCOL_RECV_CLOSED,
    PROTOCOL_RECV_ERROR,
} protocol_recv_result_t;

protocol_recv_result_t protocol_receive_frame(int socket, protocol_frame_t *frame);
bool protocol_request_from_frame(const protocol_frame_t *frame, request_t *request);
bool protocol_receive_request(int socket, request_t *request);
bool protocol_send_command(int socket, uint8_t seq_id, uint8_t command_id, const uint8_t *payload, uint8_t payload_length);
bool protocol_send_response(int socket, uint8_t seq_id, const uint8_t *payload, uint8_t payload_length);
bool protocol_send_error(int socket, uint8_t seq_id, result_t result);
bool protocol_send_ack(int socket, uint8_t seq_id);

#endif
