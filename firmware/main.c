#include "ethernet.h"
#include "stimulation.h"
#include "tcp_server.h"
#include "wifi.h"

void app_main(void)
{
    wifi_init();
    ethernet_init();
    stimulation_start();
    tcp_server_start();
}
