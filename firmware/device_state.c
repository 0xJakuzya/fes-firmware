#include "device_state.h"
#include "protocol.h"

// Состояние — одно машинное слово, чтение/запись атомарны на ESP32.
static volatile device_state_t s_state = STATE_DISCONNECTED;

void device_state_init(void)
{
    s_state = STATE_DISCONNECTED;
}

device_state_t device_state_get(void)
{
    return s_state;
}

void device_state_set(device_state_t state)
{
    s_state = state;
}

bool device_state_command_allowed(uint8_t msg_type)
{
    device_state_t state = s_state;

    switch (msg_type) {
    // Всегда допустимы (информация, конфигурация, контроль связи, аварийная остановка)
    case PROTOCOL_MSG_GET_INFO:
    case PROTOCOL_MSG_SET_WIFI_PASSWORD:
    case PROTOCOL_MSG_HEARTBEAT:
    case PROTOCOL_MSG_SET_HEARTBEAT_CONFIG:
    case PROTOCOL_MSG_GET_CHANNEL:
    case PROTOCOL_MSG_ALARM:
        return true;

    // Снятие ошибки — только из ERROR
    case PROTOCOL_MSG_RESET_ERROR:
        return state == STATE_ERROR;

    // Параметры стимуляции и валидация — после готовности (heartbeat получен)
    case PROTOCOL_MSG_SET_CHANNEL:
    case PROTOCOL_MSG_SET_ALL:
    case PROTOCOL_MSG_VALIDATE:
        return state == STATE_READY_FOR_COMMANDS || state == STATE_RUNNING;

    // Запуск — только из готовности
    case PROTOCOL_MSG_START:
        return state == STATE_READY_FOR_COMMANDS;

    // Остановка — только во время выполнения
    case PROTOCOL_MSG_STOP:
        return state == STATE_RUNNING;

    default:
        return false;
    }
}
