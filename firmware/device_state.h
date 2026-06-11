#ifndef DEVICE_STATE_H
#define DEVICE_STATE_H

#include <stdbool.h>
#include <stdint.h>

// Конечный автомат состояний транспортного модуля (сценарий из требований,
// Таблица 1.1). Гейтинг команд по текущему состоянию.
typedef enum {
    STATE_DISCONNECTED = 0,    // нет TCP-клиента
    STATE_CONNECTED,           // TCP установлен, рукопожатие отправлено, ждём heartbeat
    STATE_READY_FOR_COMMANDS,  // получен первый валидный heartbeat
    STATE_RUNNING,             // программа стимуляции выполняется
    STATE_ERROR,               // авария/таймаут/ошибка канала; нужен Reset Error
} device_state_t;

void device_state_init(void);
device_state_t device_state_get(void);
void device_state_set(device_state_t state);

// Разрешена ли команда msg_type в текущем состоянии.
bool device_state_command_allowed(uint8_t msg_type);

#endif
