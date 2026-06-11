#include "wifi.h"
#include "config.h"

#include <string.h>

#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "nvs.h"
#include "nvs_flash.h"

// загружает сохранённый пароль из флеш-памяти ESP32.
static void load_password(char *password, size_t size)
{
    nvs_handle_t nvs; // объявляем nvs структурой nvs_handle_t (NVS пространство)
    if (nvs_open("config", NVS_READONLY, &nvs) != ESP_OK) {
        return;
    } // открываем конфиг в флеш-памяти устройства только для чтения
    nvs_get_str(nvs, "password", password, &size); // читаем строку с ключом password
    nvs_close(nvs); // заврешаем чтение
}

// вспомогательная функция для сохранения пароля после изменения
bool wifi_save_password(const char *password)
{
    nvs_handle_t nvs;
    if (nvs_open("config", NVS_READWRITE, &nvs) != ESP_OK) {
        return false;
    }
    esp_err_t result = nvs_set_str(nvs, "password", password); // set_str записывает пароль в устройство
    if (result == ESP_OK) {
        result = nvs_commit(nvs); // сохраняет изменения в флеш-памяти устройства
    }
    nvs_close(nvs);
    return result == ESP_OK;
}
// основная функция замены пароля из флеш-памяти
bool wifi_set_password(const uint8_t *password, size_t length)
{
    if (password == NULL || length > WIFI_PASSWORD_MAX_LENGTH) {
        return false;
    } // проверка указателя и предотвращаем переполнения буфера
    char terminated_password[WIFI_PASSWORD_MAX_LENGTH + 1]; // создаем бффер с +1 для эндера
    memcpy(terminated_password, password, length); // копируем байты пароля в локальный буфер
    terminated_password[length] = '\0'; // добавляем эндер
    return wifi_save_password(terminated_password); // сохраняем пароль 
}

void wifi_init(void)
{
    // берем с конфига дефолтный пароль
    char wifi_password[64] = WIFI_DEFAULT_PASSWORD;

    nvs_flash_init(); // иницилизируем nvs-пространство (память)
    load_password(wifi_password, sizeof(wifi_password)); // загружаем пароль
    esp_netif_init(); // иницилизация сетевого стека
    esp_event_loop_create_default(); // цикл обработки ошибок
    esp_netif_create_default_wifi_ap(); // сетев.интерфейс точки доступа
    wifi_init_config_t init_config = WIFI_INIT_CONFIG_DEFAULT(); // иницилизация Wi-Fi драйвера с дефолт конфигом
    esp_wifi_init(&init_config);
    wifi_config_t config = { // кастомная настройка точки доступа
        .ap = {
            .ssid = WIFI_SSID, // имя сети
            .ssid_len = strlen(WIFI_SSID), 
            .channel = WIFI_CHANNEL, // коло-во каналов
            .max_connection = WIFI_MAX_CONN, // ограничение по подключениям
            .authmode = WIFI_AUTH_WPA2_WPA3_PSK, // защита WPA2/WPA3 с паролем
        },
    };
    strcpy((char *)config.ap.password, wifi_password); // копируем пароль в конфиг точки доступа
    // поднимаем точку доступа 
    esp_wifi_set_mode(WIFI_MODE_AP); 
    esp_wifi_set_config(WIFI_IF_AP, &config);
    esp_wifi_start();
}
