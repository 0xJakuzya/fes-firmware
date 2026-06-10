#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdbool.h>
#include <stdint.h>

#include <config.h>

typedef enum {
    PROTOCOL_MSG_GET_INFO = 0x01,
    PROTOCOL_MSG_SET_WIFI_PASSWORD = 0x02,
    PROTOCOL_MSG_ERROR = 0xFF,
} protocol_message_type_t;

typedef enum {
    PROTOCOL_ERROR_INVALID_SIGNATURE = 0x01,
    PROTOCOL_ERROR_INVALID_LENGTH = 0x02,
    PROTOCOL_ERROR_INVALID_CRC = 0x03,
} protocol_error_t;

enum {
    PROTOCOL_SIGNATURE_SIZE = 2,
    PROTOCOL_HEADER_SIZE = 5,
    PROTOCOL_CRC_SIZE = 2,
};

typedef struct {
    uint8_t msg_type;
    uint16_t sequence_id;
    uint16_t payload_length;
    uint8_t payload[PROTOCOL_MAX_PAYLOAD_SIZE];
} protocol_packet_t;

bool protocol_receive_packet(int socket, protocol_packet_t *packet);
bool protocol_send_packet(int socket, uint8_t msg_type, uint16_t sequence_id,
                            const uint8_t *payload, uint16_t payload_length);
bool protocol_send_error(int socket, uint16_t sequence_id, protocol_error_t error);

#endif
