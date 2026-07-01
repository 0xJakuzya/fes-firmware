#include "protocol.h"
#include <errno.h>
#include <string.h>
#include "lwip/sockets.h"

// parse bytes
static protocol_recv_result_t recv_all(int socket, uint8_t *buffer, size_t len)
{
    for (size_t n = 0; n < len; ) {
        int r = recv(socket, buffer + n, len - n, 0);
        if (r > 0) {
            n += (size_t)r;
            continue;
        }
        if (r == 0) {
            return PROTOCOL_RECV_CLOSED;
        }
        if ((errno == EAGAIN || errno == EWOULDBLOCK) && n == 0) {
            return PROTOCOL_RECV_WAIT;
        }
        return PROTOCOL_RECV_ERROR;
    }
    return PROTOCOL_RECV_OK;
}

// send bytes
static bool send_all(int socket, const uint8_t *buffer, size_t len)
{
    for (size_t n = 0; n < len; ) {
        int r = send(socket, buffer + n, len - n, 0);
        if (r <= 0) return false;
        n += (size_t)r;
    }
    return true;
}

protocol_recv_result_t protocol_receive_frame(int socket, protocol_frame_t *frame)
{
    uint8_t header[PROTOCOL_HEADER_SIZE]; // msg_type, seq_id, payload_length

    if (frame == NULL) {
        return PROTOCOL_RECV_ERROR;
    }

    protocol_recv_result_t result = recv_all(socket, header, sizeof(header));
    if (result != PROTOCOL_RECV_OK) {
        return result;
    }

    frame->msg_type = header[0];
    frame->seq_id = header[1];
    frame->payload_length = header[2];

    if (frame->payload_length > PROTOCOL_MAX_PAYLOAD_SIZE) {
        return PROTOCOL_RECV_ERROR;
    }

    if (frame->payload_length == 0) {
        return PROTOCOL_RECV_OK;
    }

    return recv_all(socket, frame->payload, frame->payload_length);
}

bool protocol_request_from_frame(const protocol_frame_t *frame, request_t *request)
{
    if (frame == NULL || request == NULL) {
        return false;
    }

    if (frame->msg_type != MSG_TYPE_COMMAND || frame->payload_length == 0) {
        return false;
    }

    request->seq_id = frame->seq_id;
    request->command_id = frame->payload[0];
    request->payload_length = frame->payload_length - 1;

    if (request->payload_length > 0) {
        memcpy(request->payload, &frame->payload[1], request->payload_length);
    }

    return true;
}

// parsing protocol packet
bool protocol_receive_request(int socket, request_t *request)
{
    protocol_frame_t frame;

    if (request == NULL) {
        return false;
    }

    if (protocol_receive_frame(socket, &frame) != PROTOCOL_RECV_OK) {
        return false;
    }

    // if !commands --> false
    if (frame.msg_type != MSG_TYPE_COMMAND) {
        protocol_send_error(socket, frame.seq_id, RESULT_INVALID_TYPE);
        return false;
    }

    if (!protocol_request_from_frame(&frame, request)) {
        protocol_send_error(socket, frame.seq_id, RESULT_INVALID_LENGTH);
        return false;
    }

    return true;
}

bool protocol_send_command(int socket, uint8_t seq_id, uint8_t command_id, const uint8_t *payload, uint8_t payload_length)
{
    uint8_t buffer[PROTOCOL_HEADER_SIZE + PROTOCOL_MAX_PAYLOAD_SIZE];

    if (payload_length > (PROTOCOL_MAX_PAYLOAD_SIZE - 1)) {
        return false;
    }

    if (payload_length > 0 && payload == NULL) {
        return false;
    }

    uint8_t frame_payload_length = payload_length + 1;

    buffer[0] = MSG_TYPE_COMMAND;
    buffer[1] = seq_id;
    buffer[2] = frame_payload_length;
    buffer[PROTOCOL_HEADER_SIZE] = command_id;

    if (payload_length > 0) {
        memcpy(&buffer[PROTOCOL_HEADER_SIZE + 1], payload, payload_length);
    }

    return send_all(socket, buffer, PROTOCOL_HEADER_SIZE + frame_payload_length);
}

// data response
bool protocol_send_response(int socket, uint8_t seq_id, const uint8_t *payload, uint8_t payload_length)
{
    uint8_t buffer[PROTOCOL_HEADER_SIZE + PROTOCOL_MAX_PAYLOAD_SIZE]; // create array 131 bytes (3 bytes header + 128 bytes payload) 
    
    // if payload > 128 --> error
    if (payload_length > PROTOCOL_MAX_PAYLOAD_SIZE) {
        return false;
    }
    
    // if payload = 0 or payload is NULL --> error
    if (payload_length > 0 && payload == NULL) {
        return false;
    }

    buffer[0] = MSG_TYPE_DATA; // 0xDD - DATA msg_type
    buffer[1] = seq_id;
    buffer[2] = payload_length;

    if (payload_length > 0) {
        memcpy(&buffer[PROTOCOL_HEADER_SIZE], payload, payload_length);
    }

    return send_all(socket, buffer, PROTOCOL_HEADER_SIZE + payload_length);
}

bool protocol_send_error(int socket, uint8_t seq_id, result_t result)
{
    uint8_t buffer[PROTOCOL_RESULT_SIZE] = {
        MSG_TYPE_ERROR, // 0xEE
        seq_id,
        PROTOCOL_RESULT_PAYLOAD_SIZE,
        (uint8_t)result, 
    };

    return send_all(socket, buffer, sizeof(buffer));
}

bool protocol_send_ack(int socket, uint8_t seq_id)
{
    uint8_t buffer[PROTOCOL_RESULT_SIZE] = {
        MSG_TYPE_DATA, // 0xDD
        seq_id,
        PROTOCOL_RESULT_PAYLOAD_SIZE,
        (uint8_t)RESULT_OK,
    };

    return send_all(socket, buffer, sizeof(buffer));
}
