#include "tcp.h"
#include "config.h"
#include "device_info.h"
#include "wifi.h"

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
        uint8_t command;

        if (client_socket < 0) {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        while (recv(client_socket, &command, sizeof(command), 0) > 0) {
            if (command == CMD_GET_INFO) {
                device_info_t info = get_info();
                send(client_socket, &info, sizeof(info), 0);
            }

            if (command == CMD_SET_WIFI_PASSWORD) {
                wifi_set_password(client_socket);
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
