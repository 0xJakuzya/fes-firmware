#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#define TCP_SERVER_PORT           5000

#define TRANSPORT_TASK_STACK_SIZE 4096
#define TRANSPORT_TASK_PRIORITY   5
#define TRANSPORT_TASK_CORE       0

void tcp_server_start(void);

#endif
