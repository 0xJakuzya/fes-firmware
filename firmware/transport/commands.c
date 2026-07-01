#include "commands.h"
#include "heartbeat.h"
#include "stimulation.h"
#include "utils.h"
#include "wifi.h"

bool get_info(int socket, uint8_t seq_id)
{
    uint8_t payload[DEVICE_INFO_PAYLOAD_SIZE] = {
        PROTOCOL_VERSION_MAJOR,
        PROTOCOL_VERSION_MINOR,
        FIRMWARE_VERSION_MAJOR,
        FIRMWARE_VERSION_MINOR,
        FIRMWARE_VERSION_PATCH,
        FES_CHANNEL_COUNT,
    };
    return protocol_send_response(socket, seq_id, payload, sizeof(payload));
}

static bool set_channel_param(int socket, uint8_t seq_id, const request_t *request)
{
    if (request->payload_length != 5) {
        return protocol_send_error(socket, seq_id, RESULT_INVALID_LENGTH);
    } // ждем payload = 5 byte

    uint8_t  channel      = request->payload[0]; // ch_id
    uint8_t  intensity    = request->payload[1]; // intensity
    uint8_t  frequency_hz = request->payload[2]; // frequency
    uint16_t pulse_width  = read_u16(&request->payload[3]); // 16-bit pulse_us

    // set channel params
    if (!stimulation_set_intensity(channel, intensity) ||
        !stimulation_set_frequency(channel, frequency_hz) ||
        !stimulation_set_pulse_width(channel, pulse_width)) {
        return protocol_send_error(socket, seq_id, RESULT_INVALID_PARAM);
    }
    // send RESULT_OK
    return protocol_send_ack(socket, seq_id);
}

static bool get_channel_param(int socket, uint8_t seq_id, const request_t *request)
{
    if (request->payload_length != 1) { // ждем payload = ch_id (1 byte)
        return protocol_send_error(socket, seq_id, RESULT_INVALID_LENGTH);
    }

    channel_state_t state; // будет хранить параметры канала

    uint8_t channel = request->payload[0]; // номер канала
    uint8_t payload[6]; // ch_id, status, intensity, freq, pulse_us(16)

    if (!stimulation_get_channel(channel, &state)) { // берем параметры с канала
        return protocol_send_error(socket, seq_id, RESULT_INVALID_PARAM);
    }

    // собираем ответ для клиента
    payload[0] = channel;            // ch_id
    payload[1] = (uint8_t)state.status; // channel_status
    payload[2] = state.intensity;    // intensity
    payload[3] = state.frequency_hz; // frequency
    write_u16(&payload[4], state.pulse_width_us); // 16-bit pulse_us

    return protocol_send_response(socket, seq_id, payload, sizeof(payload)); // send packet
}

static bool start_channel(int socket, uint8_t seq_id, const request_t *request)
{
    if (request->payload_length != 1) { // ждем payload = ch_id (1 byte)
        return protocol_send_error(socket, seq_id, RESULT_INVALID_LENGTH);
    }

    if (!stimulation_start_channel(request->payload[0])) {
        return protocol_send_error(socket, seq_id, RESULT_INVALID_PARAM);
    }
    return protocol_send_ack(socket, seq_id);
}

static bool stop_channel(int socket, uint8_t seq_id, const request_t *request)
{
    if (request->payload_length != 1) { // ждем payload = ch_id (1 byte)
        return protocol_send_error(socket, seq_id, RESULT_INVALID_LENGTH);
    }

    if (!stimulation_stop_channel(request->payload[0])) {
        return protocol_send_error(socket, seq_id, RESULT_INVALID_PARAM);
    }
    return protocol_send_ack(socket, seq_id);
}

static bool set_wifi_password(int socket, uint8_t seq_id, const request_t *request)
{
    if (!wifi_set_password(request->payload, request->payload_length)) {
        return protocol_send_error(socket, seq_id, RESULT_INVALID_PARAM);
    }
    return protocol_send_ack(socket, seq_id);
}

bool command_handler(int socket, const request_t *request)
{
    switch (request->command_id) {
    case GET_INFO:          return get_info(socket, request->seq_id);
    case SET_WIFI_PASSWORD: return set_wifi_password(socket, request->seq_id, request);
    case SET_CHANNEL_PARAM: return set_channel_param(socket, request->seq_id, request);
    case GET_CHANNEL_PARAM: return get_channel_param(socket, request->seq_id, request);
    case START_CHANNEL:     return start_channel(socket, request->seq_id, request);
    case STOP_CHANNEL:      return stop_channel(socket, request->seq_id, request);
    case SET_HEARTBEAT:     return heartbeat_handle_command(socket, request->seq_id, request);
    default:
        return protocol_send_error(socket, request->seq_id, RESULT_UNKNOWN_CMD);
    }
}
