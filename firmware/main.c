#include "wifi.h"
#include "tcp_server.h"

void app_main(void)
{
    wifi_init();
    tcp_server_start();
}
