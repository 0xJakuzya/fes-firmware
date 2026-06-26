#ifndef WIFI_H
#define WIFI_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define WIFI_DEFAULT_PASSWORD    "1234567890"
#define WIFI_SSID                "FES_Device"
#define WIFI_CHANNEL             1
#define WIFI_MAX_CONN            2
#define WIFI_PASSWORD_MIN_LENGTH 8
#define WIFI_PASSWORD_MAX_LENGTH 63

void wifi_init(void);
bool wifi_save_password(const char *password);
bool wifi_set_password(const uint8_t *password, size_t length);

#endif
