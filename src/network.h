#ifndef OD_NETWORK_H
#define OD_NETWORK_H

#include "lwipopts.h"
#include "lwip/netif.h"
#include "lwip/tcp.h"

void init_network();
void network_enable_interrupts();
int start_network();

#endif //OD_NETWORK_H
