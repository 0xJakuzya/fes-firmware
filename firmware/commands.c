#include "commands.h"
#include "device_info.h"
#include "stimulation.h"

static uint16_t read_u16_le(const uint8_t *data)
{
    return (uint16_t)(data[0] | ((uint16_t)data[1] << 8));
}

static void write_u16_le(uint8_t *data, uint16_t value)
{
    data[0] = (uint8_t)(value & 0xFF);
    data[1] = (uint8_t)(value >> 8);
}

static bool get_info(int socket, uint8_t seq_id)
{
    device_info_t info = device_info_get();
    uint8_t payload[DEVICE_INFO_PAYLOAD_SIZE];
    device_info_serialize(&info, payload);
    return protocol_send_response(socket, seq_id, payload, sizeof(payload));
}

static bool set_channel_param(int socket, uint8_t seq_id,
                              const protocol_request_t *req)
{
    if (req->payload_length != 5) {
        return protocol_send_error(socket, seq_id, RESULT_INVALID_LENGTH);
    }

    uint8_t  channel      = req->payload[0];
    uint8_t  intensity    = req->payload[1];
    uint8_t  frequency_hz = req->payload[2];
    uint16_t pulse_width  = read_u16_le(&req->payload[3]);

    if (!stimulation_set_intensity(channel, intensity) ||
        !stimulation_set_frequency(channel, frequency_hz) ||
        !stimulation_set_pulse_width(channel, pulse_width)) {
        return protocol_send_error(socket, seq_id, RESULT_INVALID_PARAM);
    }

    return protocol_send_ack(socket, seq_id);
}

static bool get_channel_param(int socket, uint8_t seq_id,
                              const protocol_request_t *req)
{
    if (req->payload_length != 1) {
        return protocol_send_error(socket, seq_id, RESULT_INVALID_LENGTH);
    }

    uint8_t channel = req->payload[0];
    stimulation_channel_state_t state;
    if (!stimulation_get_channel(channel, &state)) {
        return protocol_send_error(socket, seq_id, RESULT_INVALID_PARAM);
    }

    uint8_t payload[5];
    payload[0] = COM_GET_CHANNEL;
    payload[1] = state.intensity;
    payload[2] = state.frequency_hz;
    write_u16_le(&payload[3], state.pulse_width_us);
    return protocol_send_response(socket, seq_id, payload, sizeof(payload));
}

static bool handle_start(int socket, uint8_t seq_id)
{
    stimulation_set_running(true);
    return protocol_send_ack(socket, seq_id);
}

static bool handle_stop(int socket, uint8_t seq_id)
{
    stimulation_set_running(false);
    return protocol_send_ack(socket, seq_id);
}

bool command_handler_process(int socket, const protocol_request_t *req)
{
    switch (req->com_id) {
    case COM_GET_INFO:          return get_info(socket, req->seq_id);
    case COM_SET_CHANNEL_PARAM: return set_channel_param(socket, req->seq_id, req);
    case COM_GET_CHANNEL:       return get_channel_param(socket, req->seq_id, req);
    case COM_START:             return handle_start(socket, req->seq_id);
    case COM_STOP:              return handle_stop(socket, req->seq_id);
    default:
        return protocol_send_error(socket, req->seq_id, RESULT_UNKNOWN_CMD);
    }
}
