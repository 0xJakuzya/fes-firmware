#include "tcp_server.h"
#include "commands.h"
#include "heartbeat.h"
#include "protocol.h"
#include "stimulation.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/sockets.h"
#include <sys/time.h>

static bool tcp_set_receive_timeout(int client_socket)
{
    struct timeval timeout = {
        .tv_sec = 0,
        .tv_usec = TCP_CLIENT_RECV_TIMEOUT_MS * 1000,
    };

    return setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO,
        &timeout, sizeof(timeout)) == 0;
}

static bool tcp_handle_frame(int client_socket, const protocol_frame_t *frame)
{
    if (frame->msg_type == MSG_TYPE_COMMAND) {
        request_t request;

        if (!protocol_request_from_frame(frame, &request)) {
            return protocol_send_error(client_socket, frame->seq_id, RESULT_INVALID_LENGTH);
        }

        return command_handler(client_socket, &request);
    }

    if (frame->msg_type == MSG_TYPE_DATA || frame->msg_type == MSG_TYPE_ERROR) {
        heartbeat_handle_response(frame);
        return true;
    }

    return protocol_send_error(client_socket, frame->seq_id, RESULT_INVALID_TYPE);
}

static void tcp_client(int client_socket)
{
    if (!tcp_set_receive_timeout(client_socket)) {
        return;
    }

    heartbeat_init();

    if (!get_info(client_socket, 0)) { // handshake
        return;
    }

    while (true) {
        protocol_frame_t frame;
        protocol_recv_result_t result;

        if (!heartbeat_service(client_socket)) {
            break;
        }

        result = protocol_receive_frame(client_socket, &frame);
        if (result == PROTOCOL_RECV_WAIT) {
            vTaskDelay(1);
            continue;
        }

        if (result != PROTOCOL_RECV_OK) {
            break;
        }

        if (!tcp_handle_frame(client_socket, &frame)) {
            break;
        }
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
        tcp_client(client_socket);
        stimulation_stop_all();
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
