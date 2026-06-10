#include "command_handler.h"
#include "config.h"
#include "device_info.h"
#include "wifi.h"

#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

bool command_handler_send_device_info(int socket, uint16_t sequence_id)
{
    device_info_t info = device_info_get();
    uint8_t payload[DEVICE_INFO_PAYLOAD_SIZE];
    device_info_serialize(&info, payload);
    return protocol_send_packet(socket, PROTOCOL_MSG_GET_INFO, sequence_id, payload, sizeof(payload));
}

// проверка длинны payload для конкретной команды
static bool has_valid_payload_length(const protocol_packet_t *packet)
{
    if (packet->msg_type == PROTOCOL_MSG_GET_INFO) {
        return packet->payload_length == 0;
    }
    if (packet->msg_type == PROTOCOL_MSG_SET_WIFI_PASSWORD) {
        return packet->payload_length >= WIFI_PASSWORD_MIN_LENGTH &&
            packet->payload_length <= WIFI_PASSWORD_MAX_LENGTH;
    }
    return true;
}

bool command_handler_process(int socket, const protocol_packet_t *packet)
{
    // проверка на payload-пакета
    if (!has_valid_payload_length(packet)) {
        return protocol_send_error(socket, packet->sequence_id, PROTOCOL_ERROR_INVALID_LENGTH);
    }
    // get_inof() возвращает версию прошивки, протокола
    if (packet->msg_type == PROTOCOL_MSG_GET_INFO) {
        return command_handler_send_device_info(socket, packet->sequence_id);
    }
    // установка пароля для Wi-Fi
    if (packet->msg_type == PROTOCOL_MSG_SET_WIFI_PASSWORD) {
        uint8_t saved = wifi_set_password(packet->payload, packet->payload_length);
        bool response_sent = protocol_send_packet(socket, PROTOCOL_MSG_SET_WIFI_PASSWORD, packet->sequence_id, &saved, sizeof(saved)
    );
        if (saved && response_sent) {
            vTaskDelay(pdMS_TO_TICKS(500));
            esp_restart();
        }
        return response_sent;
    }
    return true;
}
