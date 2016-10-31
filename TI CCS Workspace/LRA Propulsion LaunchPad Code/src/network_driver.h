#ifndef __NETWORKING_H__
#define __NETWORKING_H__
#include <stdint.h>
#include <stdbool.h>

#define IPADDR0 10
#define IPADDR1 0
#define IPADDR2 0
#define IPADDR3 2

#define NETMASK0 255
#define NETMASK1 0
#define NETMASK2 0
#define NETMASK3 0

#define MACADDR {0x00, 0x26, 0x12, 0x62, 0x46, 0x24}

#define TX_BUFFER_SIZE 1536
#define RX_BUFFER_SIZE 1536

void network_driver_init(uint32_t sysClkFreq);
void network_driver_periodic(void);

#endif /* __NETWORKING_H__*/
