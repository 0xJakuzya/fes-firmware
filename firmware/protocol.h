#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdbool.h>
#include <stdint.h>

#include <config.h>

typedef enum {
    // Транспорт / управление соединением
    PROTOCOL_MSG_GET_INFO = 0x01,          // рукопожатие (push на connect) + запрос
    PROTOCOL_MSG_SET_WIFI_PASSWORD = 0x02,
    PROTOCOL_MSG_SET_HEARTBEAT_CONFIG = 0x03,  // [interval_ms:2][timeout_ms:2]
    PROTOCOL_MSG_HEARTBEAT = 0x04,             // ping верхнего уровня
    PROTOCOL_MSG_STATUS = 0x05,                // push [state:1]
    PROTOCOL_MSG_ACK = 0x06,                   // push [orig_msg_type:1][result:1]
    // Параметры стимуляции
    PROTOCOL_MSG_SET_CHANNEL = 0x10,           // [channel:1][param:1][value:2]
    PROTOCOL_MSG_SET_ALL = 0x11,               // [param:1][value:2]
    PROTOCOL_MSG_GET_CHANNEL = 0x12,           // [channel:1] → [int:1][freq:1][pw:2]
    PROTOCOL_MSG_VALIDATE = 0x14,              // запрос валидации параметров
    // Управление выполнением
    PROTOCOL_MSG_START = 0x20,
    PROTOCOL_MSG_STOP = 0x21,
    PROTOCOL_MSG_ALARM = 0x22,                 // аварийная остановка (высший приоритет)
    PROTOCOL_MSG_RESET_ERROR = 0x23,
    // Ошибки
    PROTOCOL_MSG_ERROR = 0xFF,                 // push [error_code:1]
} protocol_message_type_t;

typedef enum {
    PROTOCOL_ERROR_INVALID_SIGNATURE = 0x01,
    PROTOCOL_ERROR_INVALID_LENGTH = 0x02,
    PROTOCOL_ERROR_INVALID_CRC = 0x03,
    PROTOCOL_ERROR_INVALID_PARAM = 0x04,
} protocol_error_t;

// Код результата выполнения команды (payload ACK)
typedef enum {
    RESULT_OK = 0x00,
    RESULT_INVALID_STATE = 0x01,      // команда недопустима в текущем состоянии
    RESULT_INVALID_PARAM = 0x02,      // неверный канал/параметр
    RESULT_VALIDATION_FAILED = 0x03,  // валидация параметров не прошла
    RESULT_BUSY = 0x04,
} protocol_result_t;

// Селектор параметра стимуляции (payload SET_CHANNEL / SET_ALL)
typedef enum {
    STIM_PARAM_INTENSITY = 0x00,
    STIM_PARAM_FREQUENCY = 0x01,
    STIM_PARAM_PULSE_WIDTH = 0x02,
} stim_param_t;

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

// Чтение 16-битного значения little-endian (переиспользуется парсерами команд).
uint16_t protocol_read_u16_le(const uint8_t *data);

#endif
