#include "tcp_server.h"
#include "command_handler.h"
#include "config.h"
#include "protocol.h"

#include <errno.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/inet.h"
#include "lwip/sockets.h"

static const char *TAG = "tcp_server";

// обслуживание tcp-клиента
static void handle_client(int client_socket)
{
    // отправляет клиенту информацию об устройстве
    if (!command_handler_send_device_info(client_socket, 0)) {
        return; 
    }
    
    // packet принимает структуру данных protocol_packet_t в protocol.h
    protocol_packet_t packet;
    
    // принимает пакет в переменную packet и обрабатывает до конца соединения
    while (protocol_receive_packet(client_socket, &packet) &&
           command_handler_process(client_socket, &packet)) {
    }
}

// tcp_server_task запускает tcp-сервер и последовательно обслуживает клиентов
static void tcp_server_task(void *parameter)
{
    (void)parameter;

    // создаем сокет с IPv4, TCP
    int server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    // создаем структуру для настройки адреса сервера
    struct sockaddr_in address = {        
        .sin_family = AF_INET,
        .sin_port = htons(TCP_SERVER_PORT), // 5000
        .sin_addr.s_addr = htonl(INADDR_ANY),
    };

    // bind привязывает сокет к адресу и порту
    bind(server_socket, (struct sockaddr *)&address, sizeof(address));
    
    // listen переводит сокет в режим ожидания подключений
    listen(server_socket, 1);
    ESP_LOGI(TAG, "TCP server listening on port %d", TCP_SERVER_PORT);

    // бесконечный цикл в котором мы ожидаем подключения клиента для обслуживания
    while (true) {
        struct sockaddr_in client_address;
        socklen_t client_address_length = sizeof(client_address);
        int client_socket = accept(
            server_socket,
            (struct sockaddr *)&client_address,
            &client_address_length);

        ESP_LOGI(TAG, "Client connected: %s:%u",
                 inet_ntoa(client_address.sin_addr),
                 ntohs(client_address.sin_port));

        handle_client(client_socket); // кидаем версию прошивки после рукопожатия
        shutdown(client_socket, SHUT_RDWR);
        close(client_socket);

        ESP_LOGI(TAG, "Client disconnected: %s:%u",
                 inet_ntoa(client_address.sin_addr),
                 ntohs(client_address.sin_port));
    }
}

void tcp_server_start(void)
{
    xTaskCreate(tcp_server_task, "tcp_server", 4096, NULL, 5, NULL);
}
