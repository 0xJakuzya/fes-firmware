#include "tcp.h"
#include "config.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/sockets.h"

static void tcp_task(void *parameter) 
{
    (void)parameter;

    int server_socket = socket(
        AF_INET, 
        SOCK_STREAM, 
        IPPROTO_TCP
    ); 

    struct sockaddr_in address = {
        .sin_family = AF_INET,
        .sin_port = htons(TCP_SERVER_PORT),
        .sin_addr.s_addr = htonl(INADDR_ANY),
    }; 

    bind(server_socket, (struct sockaddr *)&address, sizeof(address));
    listen(server_socket, 1); 

    while (1) { 
        int client_socket = accept(server_socket, NULL, NULL);

        if (client_socket < 0) {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        char buffer[TCP_BUFFER_SIZE];
        int received;

        while ((received = recv(client_socket, buffer, sizeof(buffer), 0)) > 0) {
            if (send(client_socket, buffer, received, 0) <= 0) {
                break;
            }
        }
        
        shutdown(client_socket, SHUT_RDWR);
        close(client_socket);
    }
}

void tcp_start(void)
{
    xTaskCreate(tcp_task, "tcp_server", 4096, NULL, 5, NULL);
}