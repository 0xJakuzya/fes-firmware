#include "wifi.h"
#include "tcp.h"

void app_main(void)
{
    wifi_init();
    tcp_start();
}