#ifndef HEARTBEAT_H
#define HEARTBEAT_H

#include <stdbool.h>
#include <stdint.h>

#include "protocol.h"

#define HEARTBEAT_DEFAULT_INTERVAL_MS 1000
#define HEARTBEAT_DEFAULT_TIMEOUT_MS  3000
#define HEARTBEAT_SET_PAYLOAD_SIZE    4

typedef struct {
    uint16_t interval_ms;
    uint16_t timeout_ms;
    uint8_t next_seq_id;
    uint8_t pending_seq_id;
    int64_t last_ping_ms;
    bool waiting_ack;
    bool connected;
} heartbeat_state_t;

void heartbeat_init(void);
bool heartbeat_handle_command(int socket, uint8_t seq_id, const request_t *request);
bool heartbeat_service(int socket);
void heartbeat_handle_response(const protocol_frame_t *frame);
bool heartbeat_is_connected(void);

#endif
