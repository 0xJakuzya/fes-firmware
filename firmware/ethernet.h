#ifndef ETHERNET_H
#define ETHERNET_H

#define ETH_MDC_GPIO        23
#define ETH_MDIO_GPIO       18
#define ETH_PHY_ADDR        1
#define ETH_PHY_RST_GPIO    16

#define ETH_STATIC_IP_A     192
#define ETH_STATIC_IP_B     168
#define ETH_STATIC_IP_C     1
#define ETH_STATIC_IP_D     200

#define ETH_GATEWAY_A       192
#define ETH_GATEWAY_B       168
#define ETH_GATEWAY_C       1
#define ETH_GATEWAY_D       1

#define ETH_NETMASK_A       255
#define ETH_NETMASK_B       255
#define ETH_NETMASK_C       255
#define ETH_NETMASK_D       0

void ethernet_init(void);

#endif
