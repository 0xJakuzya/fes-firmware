#include "ethernet.h"
#include "stimulation.h"
#include "tcp_server.h"
#include "wifi.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"

void app_main(void)
{
    // init nvs storage
    nvs_flash_init();
    // init network stack
    esp_netif_init();
    esp_event_loop_create_default();
    // bring up wifi ap
    wifi_init();
    // bring up ethernet
    ethernet_init();
    // start stimulation task
    stimulation_start();
    // start tcp server
    tcp_server_start();
}
