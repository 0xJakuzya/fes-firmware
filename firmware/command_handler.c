#include "command_handler.h"
#include "config.h"
#include "device_info.h"
#include "device_state.h"
#include "heartbeat.h"
#include "hw_module.h"
#include "safety.h"
#include "tcp_server.h"
#include "wifi.h"

#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ----------------------------------------------------------------------------
// Хелперы отправки
// ----------------------------------------------------------------------------

// Систематическое подтверждение: тот же sequence_id + код результата.
static bool send_ack(uint16_t sequence_id, uint8_t orig_type, protocol_result_t result)
{
    uint8_t payload[2] = { orig_type, (uint8_t)result };
    return tcp_server_send_packet(PROTOCOL_MSG_ACK, sequence_id, payload, sizeof(payload));
}

bool command_handler_send_device_info(int socket, uint16_t sequence_id)
{
    (void)socket;
    device_info_t info = device_info_get();
    uint8_t payload[DEVICE_INFO_PAYLOAD_SIZE];
    device_info_serialize(&info, (uint8_t)device_state_get(), payload);
    return tcp_server_send_packet(PROTOCOL_MSG_GET_INFO, sequence_id, payload, sizeof(payload));
}

// ----------------------------------------------------------------------------
// Валидация длины payload по типу команды
// ----------------------------------------------------------------------------

static bool has_valid_payload_length(const protocol_packet_t *packet)
{
    switch (packet->msg_type) {
    case PROTOCOL_MSG_GET_INFO:
    case PROTOCOL_MSG_HEARTBEAT:
    case PROTOCOL_MSG_VALIDATE:
    case PROTOCOL_MSG_START:
    case PROTOCOL_MSG_STOP:
    case PROTOCOL_MSG_ALARM:
    case PROTOCOL_MSG_RESET_ERROR:
        return packet->payload_length == 0;
    case PROTOCOL_MSG_SET_WIFI_PASSWORD:
        return packet->payload_length >= WIFI_PASSWORD_MIN_LENGTH &&
               packet->payload_length <= WIFI_PASSWORD_MAX_LENGTH;
    case PROTOCOL_MSG_SET_HEARTBEAT_CONFIG:
        return packet->payload_length == 4;
    case PROTOCOL_MSG_SET_CHANNEL:
        return packet->payload_length == 4;
    case PROTOCOL_MSG_SET_ALL:
        return packet->payload_length == 3;
    case PROTOCOL_MSG_GET_CHANNEL:
        return packet->payload_length == 1;
    default:
        return true;
    }
}

// ----------------------------------------------------------------------------
// Обработчики команд
// ----------------------------------------------------------------------------

static bool handle_set_wifi_password(uint16_t seq, const protocol_packet_t *p)
{
    uint8_t saved = wifi_set_password(p->payload, p->payload_length);
    bool sent = tcp_server_send_packet(
        PROTOCOL_MSG_SET_WIFI_PASSWORD, seq, &saved, sizeof(saved));
    if (saved && sent) {
        vTaskDelay(pdMS_TO_TICKS(500));
        esp_restart();
    }
    return sent;
}

static bool handle_set_heartbeat_config(uint16_t seq, const protocol_packet_t *p)
{
    uint16_t interval_ms = protocol_read_u16_le(&p->payload[0]);
    uint16_t timeout_ms = protocol_read_u16_le(&p->payload[2]);
    heartbeat_configure(interval_ms, timeout_ms);
    return send_ack(seq, PROTOCOL_MSG_SET_HEARTBEAT_CONFIG, RESULT_OK);
}

static bool handle_set_channel(uint16_t seq, const protocol_packet_t *p)
{
    uint8_t channel = p->payload[0];
    uint8_t param = p->payload[1];
    uint16_t value = protocol_read_u16_le(&p->payload[2]);
    if (param > STIM_PARAM_PULSE_WIDTH ||
        !hw_module_set_channel(channel, (stim_param_t)param, value)) {
        return send_ack(seq, PROTOCOL_MSG_SET_CHANNEL, RESULT_INVALID_PARAM);
    }
    return send_ack(seq, PROTOCOL_MSG_SET_CHANNEL, RESULT_OK);
}

static bool handle_set_all(uint16_t seq, const protocol_packet_t *p)
{
    uint8_t param = p->payload[0];
    uint16_t value = protocol_read_u16_le(&p->payload[1]);
    if (param > STIM_PARAM_PULSE_WIDTH) {
        return send_ack(seq, PROTOCOL_MSG_SET_ALL, RESULT_INVALID_PARAM);
    }
    hw_module_set_all((stim_param_t)param, value);
    return send_ack(seq, PROTOCOL_MSG_SET_ALL, RESULT_OK);
}

static bool handle_get_channel(uint16_t seq, const protocol_packet_t *p)
{
    uint8_t channel = p->payload[0];
    stimulation_channel_state_t state;
    if (!hw_module_get_channel(channel, &state)) {
        return send_ack(seq, PROTOCOL_MSG_GET_CHANNEL, RESULT_INVALID_PARAM);
    }
    uint8_t payload[4] = {
        state.intensity,
        state.frequency_hz,
        (uint8_t)(state.pulse_width_us & 0xFF),
        (uint8_t)(state.pulse_width_us >> 8),
    };
    return tcp_server_send_packet(PROTOCOL_MSG_GET_CHANNEL, seq, payload, sizeof(payload));
}

static bool handle_validate(uint16_t seq)
{
    protocol_result_t result =
        hw_module_validate() == HW_RESULT_OK ? RESULT_OK : RESULT_VALIDATION_FAILED;
    return send_ack(seq, PROTOCOL_MSG_VALIDATE, result);
}

static bool handle_start(uint16_t seq)
{
    if (hw_module_validate() != HW_RESULT_OK) {
        return send_ack(seq, PROTOCOL_MSG_START, RESULT_VALIDATION_FAILED);
    }
    if (!hw_module_start()) {
        return send_ack(seq, PROTOCOL_MSG_START, RESULT_INVALID_STATE);
    }
    device_state_set(STATE_RUNNING);
    bool sent = send_ack(seq, PROTOCOL_MSG_START, RESULT_OK);
    tcp_server_push_status();
    return sent;
}

static bool handle_stop(uint16_t seq)
{
    hw_module_stop();
    device_state_set(STATE_READY_FOR_COMMANDS);
    bool sent = send_ack(seq, PROTOCOL_MSG_STOP, RESULT_OK);
    tcp_server_push_status();
    return sent;
}

static bool handle_reset_error(uint16_t seq)
{
    hw_module_reset_error();
    device_state_set(STATE_READY_FOR_COMMANDS);
    bool sent = send_ack(seq, PROTOCOL_MSG_RESET_ERROR, RESULT_OK);
    tcp_server_push_status();
    return sent;
}

// ----------------------------------------------------------------------------
// Диспетчер
// ----------------------------------------------------------------------------

bool command_handler_process(int socket, const protocol_packet_t *packet)
{
    uint16_t seq = packet->sequence_id;
    uint8_t type = packet->msg_type;

    // Аварийная остановка — абсолютный приоритет, до любых проверок
    if (type == PROTOCOL_MSG_ALARM) {
        safety_emergency_stop(SAFETY_REASON_COMMAND);
        return send_ack(seq, type, RESULT_OK);
    }

    if (!has_valid_payload_length(packet)) {
        return protocol_send_error(socket, seq, PROTOCOL_ERROR_INVALID_LENGTH);
    }

    if (!device_state_command_allowed(type)) {
        return send_ack(seq, type, RESULT_INVALID_STATE);
    }

    switch (type) {
    case PROTOCOL_MSG_GET_INFO:
        return command_handler_send_device_info(socket, seq);
    case PROTOCOL_MSG_SET_WIFI_PASSWORD:
        return handle_set_wifi_password(seq, packet);
    case PROTOCOL_MSG_SET_HEARTBEAT_CONFIG:
        return handle_set_heartbeat_config(seq, packet);
    case PROTOCOL_MSG_HEARTBEAT:
        heartbeat_feed();
        return send_ack(seq, type, RESULT_OK);
    case PROTOCOL_MSG_SET_CHANNEL:
        return handle_set_channel(seq, packet);
    case PROTOCOL_MSG_SET_ALL:
        return handle_set_all(seq, packet);
    case PROTOCOL_MSG_GET_CHANNEL:
        return handle_get_channel(seq, packet);
    case PROTOCOL_MSG_VALIDATE:
        return handle_validate(seq);
    case PROTOCOL_MSG_START:
        return handle_start(seq);
    case PROTOCOL_MSG_STOP:
        return handle_stop(seq);
    case PROTOCOL_MSG_RESET_ERROR:
        return handle_reset_error(seq);
    default:
        return true;  // неизвестный тип — игнорируем, соединение остаётся живым
    }
}
