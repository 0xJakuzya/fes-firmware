#include "tcp_server.h"
#include "command_handler.h"
#include "config.h"
#include "protocol.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/sockets.h"

static void handle_client(int client_socket)
{
    if (!command_handler_send_device_info(client_socket, 0)) {
        return;
    }

    protocol_packet_t packet;
    while (protocol_receive_packet(client_socket, &packet) &&
           command_handler_process(client_socket, &packet)) {
    }
}

static void tcp_server_task(void *parameter)
{
    (void)parameter;

    int server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    
    if (server_socket < 0) {
        vTaskDelete(NULL);
        return;
    }

    struct sockaddr_in address = {
        .sin_family = AF_INET,
        .sin_port = htons(TCP_SERVER_PORT),
        .sin_addr.s_addr = htonl(INADDR_ANY),
    };

    bind(server_socket, (struct sockaddr *)&address, sizeof(address));
    listen(server_socket, 1); 

    while (true) {
        int client_socket = accept(server_socket, NULL, NULL);
        if (client_socket < 0) {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        handle_client(client_socket);
        shutdown(client_socket, SHUT_RDWR);
        close(client_socket);
    }
}

void tcp_server_start(void)
{
    xTaskCreate(tcp_server_task, "tcp_server", 4096, NULL, 5, NULL);
}
