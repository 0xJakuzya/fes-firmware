#include "wifi.h"
#include "tcp_server.h"
#include "hw_module.h"
#include "device_state.h"
#include "heartbeat.h"
#include "safety.h"

#include "esp_err.h"

void app_main(void)
{
    wifi_init();
    ESP_ERROR_CHECK(hw_module_init());  // PCA9685 + движок стимуляции, выходы LOW
    device_state_init();
    heartbeat_init();
    safety_init();
    tcp_server_start();
}
