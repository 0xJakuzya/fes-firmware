#ifndef WIFI_H
#define WIFI_H

#include <stdbool.h>

void wifi_init(void);
bool wifi_save_password(const char *password);
void wifi_set_password(int socket);

#endif
