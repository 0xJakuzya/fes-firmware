#include "protocol.h"
#include <string.h>
#include "lwip/sockets.h"

// parse bytes
static bool recv_all(int socket, uint8_t *buffer, size_t len)
{
    for (size_t n = 0; n < len; ) {
        int r = recv(socket, buffer + n, len - n, 0);
        if (r <= 0) return false;
        n += (size_t)r;
    }
    return true;
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

// parsing protocol packet
bool protocol_receive_request(int socket, request_t *request)
{
    uint8_t header[PROTOCOL_HEADER_SIZE]; // msg_type, seq_id, payload_length
    uint8_t raw[PROTOCOL_MAX_PAYLOAD_SIZE]; // create 128 bytes array for payload

    uint8_t seq_id; // request id
    uint8_t payload_length; // len(payload)
    uint8_t msg_type;

    if (request == NULL) {
        return false;
    }

    // parsing header
    if (!recv_all(socket, header, sizeof(header))) {
        return false;
    }

    msg_type = header[0];
    seq_id = header[1];
    payload_length = header[2];

    // if !commands --> false
    if (msg_type != MSG_TYPE_COMMAND) {
        protocol_send_error(socket, seq_id, RESULT_INVALID_TYPE);
        return false;
    }

    if (payload_length == 0 || payload_length > PROTOCOL_MAX_PAYLOAD_SIZE) {
        protocol_send_error(socket, seq_id, RESULT_INVALID_LENGTH);
        return false;
    }

    // parsing payload
    if (!recv_all(socket, raw, payload_length)) {
        return false;
    }

    request->seq_id = seq_id;  // id request
    request->command_id = raw[0]; // command_id
    request->payload_length = payload_length - 1; // len(payload) without command_id

    if (request->payload_length > 0) {
        memcpy(request->payload, &raw[1], request->payload_length);
    }

    return true;
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
