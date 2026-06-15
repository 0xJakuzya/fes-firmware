#include "protocol.h"

#include <string.h>
#include "lwip/sockets.h"

uint16_t protocol_read_u16_le(const uint8_t *d)
{
    return (uint16_t)(d[0] | (d[1] << 8));
}

static void write_u16_le(uint8_t *d, uint16_t v)
{
    d[0] = (uint8_t)v;
    d[1] = (uint8_t)(v >> 8);
}

static uint16_t crc16(uint16_t crc, const uint8_t *data, size_t len)
{
    for (size_t i = 0; i < len; ++i) {
        crc ^= (uint16_t)data[i] << 8;
        for (int b = 0; b < 8; ++b) {
            crc = (crc & 0x8000) ? (uint16_t)((crc << 1) ^ 0x1021) : (uint16_t)(crc << 1);
        }
    }
    return crc;
}

static bool recv_all(int sock, uint8_t *buf, size_t len)
{
    for (size_t n = 0; n < len; ) {
        int r = recv(sock, buf + n, len - n, 0);
        if (r <= 0) return false;
        n += (size_t)r;
    }
    return true;
}

static bool send_all(int sock, const uint8_t *buf, size_t len)
{
    for (size_t n = 0; n < len; ) {
        int r = send(sock, buf + n, len - n, 0);
        if (r <= 0) return false;
        n += (size_t)r;
    }
    return true;
}

static bool find_signature(int sock, bool *invalid)
{
    const uint8_t lo = (uint8_t)PROTOCOL_SIGNATURE, hi = (uint8_t)(PROTOCOL_SIGNATURE >> 8);
    uint8_t byte;
    bool have_lo = false;
    *invalid = false;
    while (recv_all(sock, &byte, 1)) {
        if (have_lo && byte == hi) return true;
        if (have_lo || byte != lo) *invalid = true;
        have_lo = (byte == lo);
    }
    return false;
}

bool protocol_send_packet(int sock, uint8_t msg_type, uint16_t seq,
                          const uint8_t *payload, uint16_t len)
{
    if (len > PROTOCOL_MAX_PAYLOAD_SIZE) return false;
    uint8_t buf[PROTOCOL_SIGNATURE_SIZE + PROTOCOL_HEADER_SIZE +
                PROTOCOL_MAX_PAYLOAD_SIZE + PROTOCOL_CRC_SIZE];
    write_u16_le(buf, PROTOCOL_SIGNATURE);
    buf[2] = msg_type;
    write_u16_le(&buf[3], seq);
    write_u16_le(&buf[5], len);
    if (len) memcpy(&buf[7], payload, len);
    write_u16_le(&buf[7 + len], crc16(0xFFFF, &buf[2], PROTOCOL_HEADER_SIZE + len));

    return send_all(sock, buf, (size_t)(7 + len + 2));
}

bool protocol_send_error(int sock, uint16_t seq, protocol_error_t error)
{
    uint8_t code = (uint8_t)error;
    return protocol_send_packet(sock, PROTOCOL_MSG_ERROR, seq, &code, 1);
}

bool protocol_receive_packet(int sock, protocol_packet_t *p)
{
    for (;;) {
        bool invalid;
        if (!find_signature(sock, &invalid)) return false;
        if (invalid && !protocol_send_error(sock, 0, PROTOCOL_ERROR_INVALID_SIGNATURE)) return false;

        uint8_t header[PROTOCOL_HEADER_SIZE];
        if (!recv_all(sock, header, sizeof(header))) return false;
        p->msg_type = header[0];
        p->sequence_id = protocol_read_u16_le(&header[1]);
        p->payload_length = protocol_read_u16_le(&header[3]);

        if (p->payload_length > PROTOCOL_MAX_PAYLOAD_SIZE) {
            if (!protocol_send_error(sock, p->sequence_id, PROTOCOL_ERROR_INVALID_LENGTH)) return false;
            continue;
        }

        uint8_t crc_bytes[PROTOCOL_CRC_SIZE];
        if (!recv_all(sock, p->payload, p->payload_length) || !recv_all(sock, crc_bytes, sizeof(crc_bytes)))
            return false;

        uint16_t crc = crc16(0xFFFF, header, sizeof(header));
        crc = crc16(crc, p->payload, p->payload_length);
        if (protocol_read_u16_le(crc_bytes) != crc) {
            if (!protocol_send_error(sock, p->sequence_id, PROTOCOL_ERROR_INVALID_CRC)) return false;
            continue;
        }
        return true;
    }
}
