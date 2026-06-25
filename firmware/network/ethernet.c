#include "ethernet.h"
#include "esp_eth.h"
#include "esp_eth_mac_esp.h"
#include "esp_eth_phy.h"
#include "esp_event.h"
#include "esp_netif.h"

void ethernet_init(void)
{
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_config_t netif_cfg = ESP_NETIF_DEFAULT_ETH();
    esp_netif_t *eth_netif = esp_netif_new(&netif_cfg);
    esp_netif_dhcpc_stop(eth_netif);
    esp_netif_ip_info_t ip_info = {
        .ip      = { .addr = ESP_IP4TOADDR(ETH_STATIC_IP_A, ETH_STATIC_IP_B, ETH_STATIC_IP_C, ETH_STATIC_IP_D) },
        .gw      = { .addr = ESP_IP4TOADDR(ETH_GATEWAY_A,   ETH_GATEWAY_B,   ETH_GATEWAY_C,   ETH_GATEWAY_D)   },
        .netmask = { .addr = ESP_IP4TOADDR(ETH_NETMASK_A,   ETH_NETMASK_B,   ETH_NETMASK_C,   ETH_NETMASK_D)   },
    };
    esp_netif_set_ip_info(eth_netif, &ip_info);
    eth_esp32_emac_config_t emac_cfg = ETH_ESP32_EMAC_DEFAULT_CONFIG();
    emac_cfg.smi_gpio.mdc_num  = ETH_MDC_GPIO;
    emac_cfg.smi_gpio.mdio_num = ETH_MDIO_GPIO;
    eth_mac_config_t mac_cfg = ETH_MAC_DEFAULT_CONFIG();
    eth_phy_config_t phy_cfg = ETH_PHY_DEFAULT_CONFIG();
    phy_cfg.phy_addr       = ETH_PHY_ADDR;
    phy_cfg.reset_gpio_num = ETH_PHY_RST_GPIO;
    esp_eth_mac_t *mac = esp_eth_mac_new_esp32(&emac_cfg, &mac_cfg);
    esp_eth_phy_t *phy = esp_eth_phy_new_lan87xx(&phy_cfg);
    esp_eth_config_t eth_cfg = ETH_DEFAULT_CONFIG(mac, phy);
    esp_eth_handle_t eth_handle = NULL;
    esp_eth_driver_install(&eth_cfg, &eth_handle);
    esp_netif_attach(eth_netif, esp_eth_new_netif_glue(eth_handle));
    esp_eth_start(eth_handle);
}
