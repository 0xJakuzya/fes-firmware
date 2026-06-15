idf_component_register(
    SRCS
        "main.c"
        "wifi.c"
        "tcp_server.c"
        "protocol.c"
        "command_handler.c"
        "device_info.c"
        "pca9685.c"
        "stimulation.c"

    INCLUDE_DIRS
        "."

    PRIV_REQUIRES
        "esp_wifi"
        "esp_event"
        "esp_netif"
        "nvs_flash"
        "freertos"
        "lwip"
        "esp_driver_i2c"
        "esp_timer"
)
