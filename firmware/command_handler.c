#include "command_handler.h"
#include "config.h"
#include "device_info.h"
#include "stimulation.h"
#include "wifi.h"

#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static bool send_ack(int socket, uint16_t seq, uint8_t orig_type, protocol_result_t result)
{
    uint8_t payload[2] = { orig_type, (uint8_t)result };
    return protocol_send_packet(socket, PROTOCOL_MSG_ACK, seq, payload, sizeof(payload));
}

bool command_handler_send_device_info(int socket, uint16_t sequence_id)
{
    device_info_t info = device_info_get();
    uint8_t payload[DEVICE_INFO_PAYLOAD_SIZE];
    device_info_serialize(&info, payload);
    return protocol_send_packet(socket, PROTOCOL_MSG_GET_INFO, sequence_id, payload, sizeof(payload));
}

static bool has_valid_payload_length(const protocol_packet_t *p)
{
    switch (p->msg_type) {
    case PROTOCOL_MSG_GET_INFO:
    case PROTOCOL_MSG_START:
    case PROTOCOL_MSG_STOP:
        return p->payload_length == 0;
    case PROTOCOL_MSG_SET_WIFI_PASSWORD:
        return p->payload_length >= WIFI_PASSWORD_MIN_LENGTH &&
               p->payload_length <= WIFI_PASSWORD_MAX_LENGTH;
    case PROTOCOL_MSG_SET_CHANNEL:
        return p->payload_length == 4;
    case PROTOCOL_MSG_SET_ALL:
        return p->payload_length == 3;
    case PROTOCOL_MSG_GET_CHANNEL:
        return p->payload_length == 1;
    default:
        return true;
    }
}

static bool handle_set_wifi_password(int socket, uint16_t seq, const protocol_packet_t *p)
{
    uint8_t saved = wifi_set_password(p->payload, p->payload_length);
    bool sent = protocol_send_packet(socket, PROTOCOL_MSG_SET_WIFI_PASSWORD, seq, &saved, sizeof(saved));
    if (saved && sent) {
        vTaskDelay(pdMS_TO_TICKS(500));
        esp_restart();
    }
    return sent;
}

static bool handle_set_channel(int socket, uint16_t seq, const protocol_packet_t *p)
{
    uint8_t channel = p->payload[0];
    uint8_t param = p->payload[1];
    uint16_t value = protocol_read_u16_le(&p->payload[2]);
    if (param > STIM_PARAM_PULSE_WIDTH || !stimulation_set_param(channel, (stim_param_t)param, value)) {
        return send_ack(socket, seq, PROTOCOL_MSG_SET_CHANNEL, RESULT_INVALID_PARAM);
    }
    return send_ack(socket, seq, PROTOCOL_MSG_SET_CHANNEL, RESULT_OK);
}

static bool handle_set_all(int socket, uint16_t seq, const protocol_packet_t *p)
{
    uint8_t param = p->payload[0];
    uint16_t value = protocol_read_u16_le(&p->payload[1]);
    if (param > STIM_PARAM_PULSE_WIDTH) {
        return send_ack(socket, seq, PROTOCOL_MSG_SET_ALL, RESULT_INVALID_PARAM);
    }
    stimulation_set_all((stim_param_t)param, value);
    return send_ack(socket, seq, PROTOCOL_MSG_SET_ALL, RESULT_OK);
}

static bool handle_get_channel(int socket, uint16_t seq, const protocol_packet_t *p)
{
    uint8_t channel = p->payload[0];
    stimulation_channel_state_t state;
    if (!stimulation_get_channel(channel, &state)) {
        return send_ack(socket, seq, PROTOCOL_MSG_GET_CHANNEL, RESULT_INVALID_PARAM);
    }
    uint8_t payload[4] = {
        state.intensity,
        state.frequency_hz,
        (uint8_t)(state.pulse_width_us & 0xFF),
        (uint8_t)(state.pulse_width_us >> 8),
    };
    return protocol_send_packet(socket, PROTOCOL_MSG_GET_CHANNEL, seq, payload, sizeof(payload));
}

bool command_handler_process(int socket, const protocol_packet_t *packet)
{
    uint16_t seq = packet->sequence_id;
    uint8_t type = packet->msg_type;

    if (!has_valid_payload_length(packet)) {
        return protocol_send_error(socket, seq, PROTOCOL_ERROR_INVALID_LENGTH);
    }

    switch (type) {
    case PROTOCOL_MSG_GET_INFO:
        return command_handler_send_device_info(socket, seq);
    case PROTOCOL_MSG_SET_WIFI_PASSWORD:
        return handle_set_wifi_password(socket, seq, packet);
    case PROTOCOL_MSG_SET_CHANNEL:
        return handle_set_channel(socket, seq, packet);
    case PROTOCOL_MSG_SET_ALL:
        return handle_set_all(socket, seq, packet);
    case PROTOCOL_MSG_GET_CHANNEL:
        return handle_get_channel(socket, seq, packet);
    case PROTOCOL_MSG_START:
        stimulation_set_running(true);
        return send_ack(socket, seq, PROTOCOL_MSG_START, RESULT_OK);
    case PROTOCOL_MSG_STOP:
        stimulation_set_running(false);
        return send_ack(socket, seq, PROTOCOL_MSG_STOP, RESULT_OK);
    default:
        return true;
    }
}
