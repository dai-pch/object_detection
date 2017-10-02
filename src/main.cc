/*
 * Empty C++ Application
 */

#include "lwip/tcp.h"
#include "lwip/tcp_impl.h"
#include "netif/xadapter.h"

#include "cycque.h"
#include "image_store.h"
#include "detect.h"
#include "singleton.h"
#include "config.h"

extern volatile int TcpFastTmrFlag;
extern volatile int TcpSlowTmrFlag;

int main()
{
	auto& ODNetif = get_instance<struct netif>();
	/* receive and process packets */
	while (1) {
		if (TcpFastTmrFlag) {
			tcp_fasttmr();
			TcpFastTmrFlag = 0;
		}
		if (TcpSlowTmrFlag) {
			tcp_slowtmr();
			TcpSlowTmrFlag = 0;
		}
		xemacif_input(&ODNetif);
	}
	return 0;
}
