#include "ethernet.h"
#include "stimulation.h"
#include "tcp_server.h"
#include "wifi.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"

void app_main(void)
{
    nvs_flash_init();
    esp_netif_init();
    esp_event_loop_create_default();
    wifi_init();
    ethernet_init();
    stimulation_start();
    tcp_server_start();
}
