idf_component_register(
    SRCS
        "main.c"
        "network/tcp_server.c"
        "network/protocol.c"
        "network/wifi.c"
        "commands.c"
        "device_info.c"
        "pca9685.c"
        "stimulation.c"

    INCLUDE_DIRS
        "."
        "network"

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
