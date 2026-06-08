#include "wifi.h"
#include "config.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "MAIN";

void app_main(void)
{
    wifi_init();
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}