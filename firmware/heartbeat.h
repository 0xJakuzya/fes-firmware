#ifndef HEARTBEAT_H
#define HEARTBEAT_H

#include <stdint.h>

// Контроль целостности соединения. Верхний уровень периодически шлёт heartbeat;
// при превышении таймаута во время активной стимуляции инициируется аварийная
// остановка. Параметры программируются командой верхнего уровня.

void heartbeat_init(void);

// Настройка интервала отправки и таймаута (мс). Программируется верхним уровнем.
void heartbeat_configure(uint16_t interval_ms, uint16_t timeout_ms);

// Вызывается при приёме heartbeat-пакета. Первый валидный heartbeat переводит
// устройство из CONNECTED в READY_FOR_COMMANDS.
void heartbeat_feed(void);

// Запуск мониторинга при установлении соединения; останов при разрыве.
void heartbeat_start(void);
void heartbeat_stop(void);

#endif
