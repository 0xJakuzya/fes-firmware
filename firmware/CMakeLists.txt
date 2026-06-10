idf_component_register(
    SRCS 
        "main.c"
        "wifi.c"
        "tcp_server.c"
        "protocol.c"
        "command_handler.c"
        "device_info.c"
    
    INCLUDE_DIRS 
        "."
    
    PRIV_REQUIRES
        "esp_wifi"
        "esp_event"
        "esp_netif"
        "nvs_flash"
        "freertos"
        "lwip"
)
