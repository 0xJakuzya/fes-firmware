#include "protocol.h"

#include <stddef.h>
#include "lwip/sockets.h"

// преобразование чисел 55 AA → 0xAA55
static uint16_t read_u16_le(const uint8_t *data)
{
    return (uint16_t)data[0] | ((uint16_t)data[1] << 8);
}

static void write_u16_le(uint8_t *data, uint16_t value)
{
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8);
}

// расчет crc
static uint16_t crc16_ccitt_update(uint16_t crc, const uint8_t *data, size_t length)
{
    for (size_t i = 0; i < length; ++i) {
        crc ^= (uint16_t)data[i] << 8;
        for (uint8_t bit = 0; bit < 8; ++bit) {
            crc = (crc & 0x8000) != 0
                ? (uint16_t)((crc << 1) ^ 0x1021)
                : (uint16_t)(crc << 1);
        }
    }
    return crc;
}

// читает из TCP ровно указанное количество байтов
static bool receive_all(int socket, uint8_t *buffer, size_t length)
{
    size_t received = 0;
    while (received < length) {
        int result = recv(socket, buffer + received, length - received, 0);
        received += (size_t)result;
        if (result <= 0) {
            return false;
        }
    }
    return true;
}

// отправляет переданный буфер
static bool send_all(int socket, const uint8_t *buffer, size_t length)
{
    size_t sent = 0;
    while (sent < length) {
        int result = send(socket, buffer + sent, length - sent, 0);
        sent += (size_t)result;
        if (result <= 0) {
            return false;
        }
    }
    return true;
}

// ищет начало пакета в TCP-потоке
static bool find_signature(int socket, bool *invalid_signature)
{
    const uint8_t signature_low = (uint8_t)PROTOCOL_SIGNATURE;
    const uint8_t signature_high = (uint8_t)(PROTOCOL_SIGNATURE >> 8);
    uint8_t byte;
    bool have_low = false;

    *invalid_signature = false;
    while (receive_all(socket, &byte, 1)) {
        if (have_low && byte == signature_high) {
            return true;
        }
        if (have_low || byte != signature_low) {
            *invalid_signature = true;
        }
        have_low = byte == signature_low;
    }
    return false;
}

// отправка пакета
bool protocol_send_packet(int socket, uint8_t msg_type, uint16_t sequence_id,
                            const uint8_t *payload, uint16_t payload_length)
{
    // проверка максимальной длины payload
    if (payload_length > PROTOCOL_MAX_PAYLOAD_SIZE) {
        return false;
    }

    uint8_t signature[PROTOCOL_SIGNATURE_SIZE];
    uint8_t header[PROTOCOL_HEADER_SIZE];
    uint8_t crc_bytes[PROTOCOL_CRC_SIZE];

    write_u16_le(signature, PROTOCOL_SIGNATURE);
    header[0] = msg_type;
    write_u16_le(&header[1], sequence_id);
    write_u16_le(&header[3], payload_length);

    uint16_t crc = crc16_ccitt_update(0xFFFF, header, sizeof(header));
    crc = crc16_ccitt_update(crc, payload, payload_length);
    write_u16_le(crc_bytes, crc);

    return send_all(socket, signature, sizeof(signature)) &&
        send_all(socket, header, sizeof(header)) &&
        send_all(socket, payload, payload_length) &&
        send_all(socket, crc_bytes, sizeof(crc_bytes));
}

bool protocol_send_error(int socket, uint16_t sequence_id, protocol_error_t error)
{
    uint8_t error_code = (uint8_t)error;
    return protocol_send_packet(
        socket,
        PROTOCOL_MSG_ERROR,
        sequence_id,
        &error_code,
        sizeof(error_code)
    );
}

bool protocol_receive_packet(int socket, protocol_packet_t *packet)
{
    while (true) {
        bool invalid_signature;
        if (!find_signature(socket, &invalid_signature)) {
            return false;
        }
        if (invalid_signature &&
            !protocol_send_error(
                socket,
                0,
                PROTOCOL_ERROR_INVALID_SIGNATURE)) {
            return false;
        }

        uint8_t header[PROTOCOL_HEADER_SIZE];
        if (!receive_all(socket, header, sizeof(header))) {
            return false;
        }

        packet->msg_type = header[0];
        packet->sequence_id = read_u16_le(&header[1]);
        packet->payload_length = read_u16_le(&header[3]);
        if (packet->payload_length > PROTOCOL_MAX_PAYLOAD_SIZE) {
            if (!protocol_send_error(
                    socket,
                    packet->sequence_id,
                    PROTOCOL_ERROR_INVALID_LENGTH)) {
                return false;
            }
            continue;
        }

        uint8_t crc_bytes[PROTOCOL_CRC_SIZE];
        if (!receive_all(socket, packet->payload, packet->payload_length) ||
            !receive_all(socket, crc_bytes, sizeof(crc_bytes))) {
            return false;
        }

        uint16_t expected_crc = crc16_ccitt_update(
            0xFFFF,
            header,
            sizeof(header)
        );
        expected_crc = crc16_ccitt_update(
            expected_crc,
            packet->payload,
            packet->payload_length
        );
        if (read_u16_le(crc_bytes) != expected_crc) {
            if (!protocol_send_error(
                    socket,
                    packet->sequence_id,
                    PROTOCOL_ERROR_INVALID_CRC)) {
                return false;
            }
            continue;
        }
        return true;
    }
}
