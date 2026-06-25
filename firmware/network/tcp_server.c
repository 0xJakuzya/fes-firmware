#include "tcp_server.h"
#include "api.h"
#include "protocol.h"
#include "stimulation.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/sockets.h"

static void tcp_handle_client(int client_socket)
{
    protocol_request_t req;
    while (protocol_receive_request(client_socket, &req) &&
           command_handler_process(client_socket, &req)) {
    }
}

static void tcp_server_task(void *parameter)
{
    (void)parameter;

    int server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    struct sockaddr_in address = {
        .sin_family = AF_INET,
        .sin_port = htons(TCP_SERVER_PORT),
        .sin_addr.s_addr = htonl(INADDR_ANY),
    };

    bind(server_socket, (struct sockaddr *)&address, sizeof(address));
    listen(server_socket, 1);

    while (true) {
        struct sockaddr_in client_address;
        socklen_t len = sizeof(client_address);
        int client_socket = accept(server_socket, (struct sockaddr *)&client_address, &len);
        tcp_handle_client(client_socket);
        stimulation_set_running(false); 
        shutdown(client_socket, SHUT_RDWR);
        close(client_socket);
    }
}

void tcp_server_start(void)
{
    xTaskCreatePinnedToCore(tcp_server_task, "tcp_server",
        TRANSPORT_TASK_STACK_SIZE, NULL, TRANSPORT_TASK_PRIORITY, NULL,
        TRANSPORT_TASK_CORE);
}
