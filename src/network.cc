#include "xparameters.h"
#include "xparameters_ps.h"	/* defines XPAR values */
#include "xscugic.h"
#include "xscutimer.h"
#include "lwipopts.h"
#include "netif/xadapter.h"
#include "netif/xemacpsif.h"
#include "lwip/tcp.h"
#include "lwip/init.h"
#include "network.h"
#include "singleton.h"
#include "event.h"

#if LWIP_DHCP==1
#include "lwip/dhcp.h"
#endif

#define INTC_DEVICE_ID		    XPAR_SCUGIC_SINGLE_DEVICE_ID
#define TIMER_DEVICE_ID		    XPAR_SCUTIMER_DEVICE_ID
#define INTC_BASE_ADDR		    XPAR_SCUGIC_0_CPU_BASEADDR
#define INTC_DIST_BASE_ADDR	    XPAR_SCUGIC_0_DIST_BASEADDR
#define TIMER_IRPT_INTR		    XPAR_SCUTIMER_INTR
#define PLATFORM_EMAC_BASEADDR  XPAR_XEMACPS_0_BASEADDR
#define RESET_RX_CNTR_LIMIT	400

volatile int TcpFastTmrFlag;
volatile int TcpSlowTmrFlag;
static int ResetRxCntr = 0;

//static struct netif ODNetif;

#if LWIP_DHCP==1
volatile int dhcp_timoutcntr = 24;
void dhcp_fine_tmr();
void dhcp_coarse_tmr();
#endif

static XScuTimer TimerInstance;

void
timer_callback(XScuTimer * TimerInstance)
{
	/* we need to call tcp_fasttmr & tcp_slowtmr at intervals specified
	 * by lwIP. It is not important that the timing is absoluetly accurate.
	 */
	static int odd = 1;
#if LWIP_DHCP==1
    static int dhcp_timer = 0;
#endif
    //TcpFastTmrFlag = 1;
    auto& event = get_instance<EventManager>();
    event.emit(E_TcpFastTimmer);

	odd = !odd;
#ifndef USE_SOFTETH_ON_ZYNQ
	ResetRxCntr++;
#endif
	if (odd) {
#if LWIP_DHCP==1
		dhcp_timer++;
		dhcp_timoutcntr--;
#endif
		//TcpSlowTmrFlag = 1;
		event.emit(E_TcpSlowTimmer);
#if LWIP_DHCP==1
		dhcp_fine_tmr();
		if (dhcp_timer >= 120) {
			dhcp_coarse_tmr();
			dhcp_timer = 0;
		}
#endif
	}

	/* For providing an SW alternative for the SI #692601. Under heavy
	 * Rx traffic if at some point the Rx path becomes unresponsive, the
	 * following API call will ensures a SW reset of the Rx path. The
	 * API xemacpsif_resetrx_on_no_rxdata is called every 100 milliseconds.
	 * This ensures that if the above HW bug is hit, in the worst case,
	 * the Rx path cannot become unresponsive for more than 100
	 * milliseconds.
	 */
#ifndef USE_SOFTETH_ON_ZYNQ
	if (ResetRxCntr >= RESET_RX_CNTR_LIMIT) {
		auto& ODNetif = get_instance<struct netif>();
		xemacpsif_resetrx_on_no_rxdata(&ODNetif);
		ResetRxCntr = 0;
	}
#endif
	XScuTimer_ClearInterruptStatus(TimerInstance);
}

void network_setup_timer() {
	int status = XST_SUCCESS;
	XScuTimer_Config *p_timer_config;

	p_timer_config = XScuTimer_LookupConfig(TIMER_DEVICE_ID);
	status = XScuTimer_CfgInitialize(&TimerInstance, p_timer_config,
			p_timer_config->BaseAddr);
	if (status != XST_SUCCESS) {
		xil_printf("In %s: Scutimer config initialization failed...\r\n",
		__func__);
		return;
	}

	status = XScuTimer_SelfTest(&TimerInstance);
	if (status != XST_SUCCESS) {
		xil_printf("In %s: Scutimer Self test failed...\r\n",
		__func__);
		return;
	}

	XScuTimer_EnableAutoReload(&TimerInstance);
	/*
	 * Set for 250 milli seconds timeout.
	 */
	auto timerLoadValue = XPAR_CPU_CORTEXA9_0_CPU_CLK_FREQ_HZ / 8;

	XScuTimer_LoadTimer(&TimerInstance, timerLoadValue);
	return;
}

void network_setup_timer_interrupt() {
	Xil_ExceptionInit();
	XScuGic_DeviceInitialize(INTC_DEVICE_ID);
	/*
	 * Connect the interrupt controller interrupt handler to the hardware
	 * interrupt handling logic in the processor.
	 */
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_IRQ_INT,
			(Xil_ExceptionHandler)XScuGic_DeviceInterruptHandler,
			(void *)INTC_DEVICE_ID);
	/*
	 * Connect the device driver handler that will be called when an
	 * interrupt for the device occurs, the handler defined above performs
	 * the specific interrupt processing for the device.
	 */
	XScuGic_RegisterHandler(INTC_BASE_ADDR, TIMER_IRPT_INTR,
					(Xil_ExceptionHandler)timer_callback,
					(void *)&TimerInstance);
	/*
	 * Enable the interrupt for scu timer.
	 */
	XScuGic_EnableIntr(INTC_DIST_BASE_ADDR, TIMER_IRPT_INTR);

	return;
}

void print_ip(const char *msg, const struct ip_addr *ip)
{
	print(msg);
	xil_printf("%d.%d.%d.%d\n\r", ip4_addr1(ip), ip4_addr2(ip),
			ip4_addr3(ip), ip4_addr4(ip));
}

void print_ip_settings(const struct ip_addr *ip,
		const struct ip_addr *mask, const struct ip_addr *gw)
{
	print_ip("Board IP: ", ip);
	print_ip("Netmask : ", mask);
	print_ip("Gateway : ", gw);
}

bool tcp_fast_callback() {
	tcp_fasttmr();
	return true;
}
bool tcp_slow_callback() {
	tcp_slowtmr();
	return true;
}
bool emac_callback() {
	auto& ODNetif = get_instance<struct netif>();
	xemacif_input(&ODNetif);
	return true;
}

int network_init() {
	network_setup_timer();
	network_setup_timer_interrupt();

	lwip_init();

	ip_addr_t ipaddr, netmask, gw;

	/* the mac address of the board. this should be unique per board */
	unsigned char mac_ethernet_address[] =
		{ 0x00, 0x0a, 0x35, 0x00, 0x01, 0x02 };
	struct netif *default_netif = &get_instance<struct netif>();
	#if LWIP_DHCP==1
		ipaddr.addr = 0;
		gw.addr = 0;
		netmask.addr = 0;
	#else
		/* initliaze IP addresses to be used */
		IP4_ADDR(&ipaddr,  192, 168,   1, 21);
		IP4_ADDR(&netmask, 255, 255, 255,  0);
		IP4_ADDR(&gw,      192, 168,   1,  1);
	#endif

  	/* Add network interface to the netif_list, and set it as default */
	if (!xemac_add(default_netif, &ipaddr, &netmask,
						&gw, mac_ethernet_address,
						PLATFORM_EMAC_BASEADDR)) {
		xil_printf("Error adding N/W interface\n\r");
		return 1;
	}
	netif_set_default(default_netif);

	network_enable_interrupts();
	netif_set_up(default_netif);

#if (LWIP_DHCP==1)
	/* Create a new DHCP client for this interface.
	 * Note: you must call dhcp_fine_tmr() and dhcp_coarse_tmr() at
	 * the predefined regular intervals after starting the client.
	 */
	dhcp_start(default_netif);
	dhcp_timoutcntr = 24;

	while(((default_netif->ip_addr.addr) == 0) && (dhcp_timoutcntr > 0))
		xemacif_input(default_netif);

	if (dhcp_timoutcntr <= 0) {
		if ((default_netif->ip_addr.addr) == 0) {
			xil_printf("DHCP Timeout\r\n");
			xil_printf("Configuring default IP of 192.168.1.10\r\n");
			IP4_ADDR(&(default_netif->ip_addr),  192, 168,   1, 10);
			IP4_ADDR(&(default_netif->netmask), 255, 255, 255,  0);
			IP4_ADDR(&(default_netif->gw),      192, 168,   1,  1);
		}
	}

	ipaddr.addr = default_netif->ip_addr.addr;
	gw.addr = default_netif->gw.addr;
	netmask.addr = default_netif->netmask.addr;
#endif

	print_ip_settings(&ipaddr, &netmask, &gw);

	// register event disposal function
	auto& eventm = get_instance<EventManager>();
	eventm.register_function(E_TcpFastTimmer, tcp_fast_callback);
	eventm.register_function(E_TcpSlowTimmer, tcp_slow_callback);
	// eventm.register_function(E_EMAC, emac_callback);
	return 0;
}

void network_enable_interrupts()
{
	/*
	 * Enable non-critical exceptions.
	 */
	Xil_ExceptionEnableMask(XIL_EXCEPTION_IRQ);
	XScuTimer_EnableInterrupt(&TimerInstance);
	XScuTimer_Start(&TimerInstance);
	return;
}

int start_network()
{

	return 0;
}
