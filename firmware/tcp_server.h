#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include <stdbool.h>
#include <stdint.h>

void tcp_server_start(void);

// Дескриптор текущего клиента (-1, если нет соединения).
int tcp_server_get_client_socket(void);

// Потокобезопасная отправка пакета текущему клиенту (под мьютексом отправки).
// Все исходящие ответы/уведомления идут через эту функцию, чтобы избежать
// чередования байтов при отправке из разных задач.
bool tcp_server_send_packet(uint8_t msg_type, uint16_t sequence_id,
                            const uint8_t *payload, uint16_t length);

// Push текущего состояния устройства клиенту (незапрошенное уведомление).
void tcp_server_push_status(void);

#endif
