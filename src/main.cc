/*
 * Empty C++ Application
 */

#include "lwip/tcp.h"
#include "lwip/tcp_impl.h"
#include "netif/xadapter.h"

#include "cycque.h"
#include "image_store.h"
#include "detect.h"
#include "event.h"
#include "singleton.h"
#include "config.h"

int main()
{
	auto& ODNetif = get_instance<struct netif>();
	auto& eventm = get_instance<EventManager>();

	return 0;
}
