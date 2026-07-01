#include "heartbeat.h"

#include "commands.h"
#include "stimulation.h"
#include "utils.h"

#include <stddef.h>

static heartbeat_state_t s_heartbeat;

void heartbeat_init(void)
{
    s_heartbeat = (heartbeat_state_t){
        .interval_ms = HEARTBEAT_DEFAULT_INTERVAL_MS,
        .timeout_ms = HEARTBEAT_DEFAULT_TIMEOUT_MS,
        .next_seq_id = 1,
        .last_ping_ms = heartbeat_now_ms(),
    };
}

bool heartbeat_handle_command(int socket, uint8_t seq_id, const request_t *request)
{
    if (request == NULL) {
        return false;
    }

    if (request->payload_length == 0) {
        s_heartbeat.connected = true;
        return protocol_send_ack(socket, seq_id);
    }

    if (request->payload_length != HEARTBEAT_SET_PAYLOAD_SIZE) {
        return protocol_send_error(socket, seq_id, RESULT_INVALID_LENGTH);
    }

    uint16_t interval_ms = read_u16(&request->payload[0]);
    uint16_t timeout_ms = read_u16(&request->payload[2]);

    if (interval_ms == 0 || timeout_ms == 0) {
        return protocol_send_error(socket, seq_id, RESULT_INVALID_PARAM);
    }

    s_heartbeat.interval_ms = interval_ms;
    s_heartbeat.timeout_ms = timeout_ms;
    s_heartbeat.waiting_ack = false;
    s_heartbeat.connected = true;
    s_heartbeat.last_ping_ms = heartbeat_now_ms();

    return protocol_send_ack(socket, seq_id);
}

bool heartbeat_service(int socket)
{
    int64_t now_ms = heartbeat_now_ms();

    if (s_heartbeat.waiting_ack) {
        if (now_ms - s_heartbeat.last_ping_ms >= s_heartbeat.timeout_ms) {
            stimulation_stop_all();
            s_heartbeat.connected = false;
            s_heartbeat.waiting_ack = false;
            s_heartbeat.last_ping_ms = now_ms;
        }
        return true;
    }

    if (now_ms - s_heartbeat.last_ping_ms < s_heartbeat.interval_ms) {
        return true;
    }

    uint8_t seq_id = s_heartbeat.next_seq_id++;
    if (!protocol_send_command(socket, seq_id, SET_HEARTBEAT, NULL, 0)) {
        return false;
    }

    s_heartbeat.pending_seq_id = seq_id;
    s_heartbeat.last_ping_ms = now_ms;
    s_heartbeat.waiting_ack = true;

    return true;
}

void heartbeat_handle_response(const protocol_frame_t *frame)
{
    if (frame == NULL || !s_heartbeat.waiting_ack) {
        return;
    }

    if (frame->msg_type != MSG_TYPE_DATA ||
        frame->seq_id != s_heartbeat.pending_seq_id ||
        frame->payload_length != PROTOCOL_RESULT_PAYLOAD_SIZE ||
        frame->payload[0] != RESULT_OK) {
        return;
    }

    s_heartbeat.waiting_ack = false;
    s_heartbeat.connected = true;
}

bool heartbeat_is_connected(void)
{
    return s_heartbeat.connected;
}
