#include "tcp_server.h"
#include "command_handler.h"
#include "config.h"
#include "device_state.h"
#include "heartbeat.h"
#include "protocol.h"
#include "safety.h"

#include <errno.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "lwip/inet.h"
#include "lwip/sockets.h"

static const char *TAG = "tcp_server";

static volatile int s_client_socket = -1;
static SemaphoreHandle_t s_send_mutex;

int tcp_server_get_client_socket(void)
{
    return s_client_socket;
}

bool tcp_server_send_packet(uint8_t msg_type, uint16_t sequence_id,
                            const uint8_t *payload, uint16_t length)
{
    bool result = false;
    xSemaphoreTake(s_send_mutex, portMAX_DELAY);
    int sock = s_client_socket;
    if (sock >= 0) {
        result = protocol_send_packet(sock, msg_type, sequence_id, payload, length);
    }
    xSemaphoreGive(s_send_mutex);
    return result;
}

void tcp_server_push_status(void)
{
    uint8_t state = (uint8_t)device_state_get();
    tcp_server_send_packet(PROTOCOL_MSG_STATUS, 0, &state, sizeof(state));
}

// обслуживание tcp-клиента
static void handle_client(int client_socket)
{
    s_client_socket = client_socket;
    device_state_set(STATE_CONNECTED);

    // рукопожатие: отправляет клиенту расширенную информацию об устройстве
    if (!command_handler_send_device_info(client_socket, 0)) {
        return;
    }

    // запуск контроля целостности соединения (heartbeat)
    heartbeat_start();

    // принимает пакеты и обрабатывает их до конца соединения
    protocol_packet_t packet;
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

        handle_client(client_socket);

        // разрыв соединения: аварийная остановка и сброс состояния
        heartbeat_stop();
        safety_emergency_stop(SAFETY_REASON_DISCONNECT);
        s_client_socket = -1;
        shutdown(client_socket, SHUT_RDWR);
        close(client_socket);

        ESP_LOGI(TAG, "Client disconnected: %s:%u",
                 inet_ntoa(client_address.sin_addr),
                 ntohs(client_address.sin_port));
    }
}

void tcp_server_start(void)
{
    s_send_mutex = xSemaphoreCreateMutex();
    xTaskCreate(tcp_server_task, "tcp_server", 4096, NULL, 5, NULL);
}
