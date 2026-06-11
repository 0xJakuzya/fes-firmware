#ifndef SAFETY_H
#define SAFETY_H

// Аварийная остановка по трём независимым механизмам (требование безопасности).
// Единая точка, которую дёргают: обработчик команды alarm, монитор heartbeat и
// (Этап 2) колбэк критической ошибки канала от аппаратного модуля.

typedef enum {
    SAFETY_REASON_COMMAND,            // явная команда alarm от верхнего уровня
    SAFETY_REASON_HEARTBEAT_TIMEOUT,  // потеря heartbeat
    SAFETY_REASON_CHANNEL_FAULT,      // критическая ошибка устройства/канала
    SAFETY_REASON_DISCONNECT,         // разрыв TCP-соединения
} safety_reason_t;

void safety_init(void);

// Немедленно перевести каналы в безопасное состояние и обновить состояние
// устройства. Для DISCONNECT — без защёлки ошибки (переход в DISCONNECTED);
// для остальных причин — защёлка ошибки и переход в ERROR с уведомлением.
void safety_emergency_stop(safety_reason_t reason);

#endif
