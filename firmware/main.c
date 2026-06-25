#include "ethernet.h"
#include "pca9685.h"
#include "stimulation.h"
#include "tcp_server.h"
#include "wifi.h"

void app_main(void)
{
    wifi_init();
    pca9685_init();
    stimulation_start();
    tcp_server_start();
}
