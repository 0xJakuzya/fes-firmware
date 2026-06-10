#ifndef WIFI_H
#define WIFI_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

void wifi_init(void);
bool wifi_save_password(const char *password);
bool wifi_set_password(const uint8_t *password, size_t length);

#endif
