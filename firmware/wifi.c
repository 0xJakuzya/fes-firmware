#include "wifi.h"
#include "config.h"

#include <string.h>

#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "nvs.h"
#include "nvs_flash.h"

static void load_password(char *password, size_t size)
{
    nvs_handle_t nvs; 
    if (nvs_open("config", NVS_READONLY, &nvs) != ESP_OK) {
        return;
    }
    nvs_get_str(nvs, "password", password, &size);
    nvs_close(nvs); 
}

bool wifi_save_password(const char *password)
{
    nvs_handle_t nvs;
    if (nvs_open("config", NVS_READWRITE, &nvs) != ESP_OK) {
        return false;
    }
    nvs_set_str(nvs, "password", password);
    nvs_commit(nvs);
    nvs_close(nvs);
    return true;
}

bool wifi_set_password(const uint8_t *password, size_t length)
{
    if (password == NULL || length > WIFI_PASSWORD_MAX_LENGTH) {
        return false;
    }
    char terminated_password[WIFI_PASSWORD_MAX_LENGTH + 1];
    memcpy(terminated_password, password, length); 
    terminated_password[length] = '\0';
    return wifi_save_password(terminated_password); 
}

void wifi_init(void)
{
    char wifi_password[64] = WIFI_DEFAULT_PASSWORD;

    nvs_flash_init(); 
    load_password(wifi_password, sizeof(wifi_password)); 
    esp_netif_init(); 
    esp_event_loop_create_default(); 
    esp_netif_create_default_wifi_ap(); 
    wifi_init_config_t init_config = WIFI_INIT_CONFIG_DEFAULT(); 
    esp_wifi_init(&init_config);
    wifi_config_t config = { 
        .ap = {
            .ssid = WIFI_SSID, 
            .ssid_len = strlen(WIFI_SSID), 
            .channel = WIFI_CHANNEL,
            .max_connection = WIFI_MAX_CONN, 
            .authmode = WIFI_AUTH_WPA2_WPA3_PSK, 
        },
    };
    strcpy((char *)config.ap.password, wifi_password);
    esp_wifi_set_mode(WIFI_MODE_AP); 
    esp_wifi_set_config(WIFI_IF_AP, &config);
    esp_wifi_start();
}
