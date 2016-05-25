#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "network_driver.h"
#include "debug.h"
#include "main.h"
#include "packet_receiver.h"
#include "telemetry.h"

#include "driverlib/sysctl.h"
#include "driverlib/emac.h"
#include "driverlib/interrupt.h"
#include "inc/hw_memmap.h"
#include "inc/hw_emac.h"
#include "inc/hw_ints.h"

#include "uip/uip.h"
#include "uip/uip_arp.h"
#include "uip/uip_timer.h"

static uint8_t mac_addr[] = MACADDR;

static tEMACDMADescriptor txDescriptor[3];
static tEMACDMADescriptor rxDescriptor[3];

static uint8_t active_tx_desc = 0;
static uint8_t active_rx_desc = 0;

static uint8_t rx_buffer[RX_BUFFER_SIZE];
static uint8_t tx_buffer[TX_BUFFER_SIZE];

static volatile bool rx_flag = false;
static volatile bool tx_complete_flag = false;

struct timer periodic_timer_for_uip;
struct timer arp_timer_for_uip;

struct uip_udp_conn *udp_socket;

void ethernet_interrupt_handler() {
	//read and clear interrupt flags
	uint32_t flags = EMACIntStatus(EMAC0_BASE, true);
	EMACIntClear(EMAC0_BASE, flags);
	
	//check if it's an RX interrupt
	if(flags & EMAC_INT_RECEIVE) {
		rx_flag = true;
	}
	
	//check if it's the DMA reporting a finished xmit
	if(flags & EMAC_INT_TRANSMIT) {
		tx_complete_flag = true;
	}
	
}

static void transmit_ethernet_frame() {
	
	//wait until we own the tx descriptor.
	//is this a bad idea? maybe isntead just drop the frame...
	//but in that case, if one run of the event loop happens to generate multiple packets
	//then bad things will happen.......
	// instead, queue and wait for tx complete? something else? /shrug
	while(txDescriptor[active_tx_desc].ui32CtrlStatus & DES0_TX_CTRL_OWN) {};
		
	if(uip_len > TX_BUFFER_SIZE) uip_len = TX_BUFFER_SIZE;
	
	//copy data into the DMA buffer	
	for(uint16_t i = 0; i < uip_len; i++) {
		tx_buffer[i] = uip_buf[i];
	}

	
	txDescriptor[active_tx_desc].ui32Count = (uint32_t)uip_len;
	txDescriptor[active_tx_desc].ui32CtrlStatus = (
		DES0_TX_CTRL_LAST_SEG |
		DES0_TX_CTRL_FIRST_SEG |
		DES0_TX_CTRL_INTERRUPT |
		DES0_TX_CTRL_IP_ALL_CKHSUMS |
		DES0_TX_CTRL_CHAINED |
		DES0_TX_CTRL_OWN
	);
	
	EMACTxDMAPollDemand(EMAC0_BASE);
	
	//advance to the next DMA descriptor
	active_tx_desc++;
	if(active_tx_desc == 3) active_tx_desc = 0;
}

static void receive_ethernet_frame() {
	//copy rx data out of the DMA buffer
	
	//make sure we own the dma thing
	if(rxDescriptor[active_rx_desc].ui32CtrlStatus & DES0_RX_CTRL_OWN) {
		debug_print("don't own dma!\r\n");
		while(1);
	}
		
	//check if it contains a valid packet (it'll have error bit set if it's truncated, or if last frame in pkt
	if(rxDescriptor[active_rx_desc].ui32CtrlStatus & DES0_RX_STAT_ERR) {
		debug_print("dmaRxDescErr!\r\n");
		while(1);
	}
		
	//make sure it's the "last descriptor" (it always should be, because the DMA buffer should be bigger than the biggest 
	//possible eth frame
	if(!(rxDescriptor[active_rx_desc].ui32CtrlStatus & DES0_RX_STAT_LAST_DESC)) {
		debug_print("dmaNotLast!\r\n");
		while(1);
	}
	
	//OK it's probably a good packet
	uint32_t frame_len = (rxDescriptor[active_rx_desc].ui32CtrlStatus & DES0_RX_STAT_FRAME_LENGTH_M) >> DES0_RX_STAT_FRAME_LENGTH_S;

	//copy packet from DMA buffer into uIP buffer
	for(uint16_t i=0; i < frame_len; i++) {
		uip_buf[i] = rx_buffer[i];
	}
	uip_len = frame_len;
	
	//look @ header
	struct uip_eth_hdr *pkt_header = (struct uip_eth_hdr *) uip_buf;
	
	if(pkt_header->type == htons(UIP_ETHTYPE_IP)) {
		//it's an IP packet!
		
		uip_arp_ipin();
		uip_input();
		
		//if uIP generated output, send it
		if(uip_len > 0) {
			uip_arp_out();
			transmit_ethernet_frame();
		}
		uip_len = 0;
		
	} else if (pkt_header->type == htons(UIP_ETHTYPE_ARP)) {
		//it's an ARP packet!
	
		uip_arp_arpin();
		
		//if uIP generated output, send it
		if(uip_len > 0) {
			transmit_ethernet_frame();
		}
		uip_len = 0;
		
	} else if (pkt_header->type == htons(UIP_ETHTYPE_IP6)) {
	}
	//move on to the next DMA descriptor in the chain
	active_rx_desc++;
	if(active_rx_desc == 3) active_rx_desc = 0;

	//give the new active DMA descriptor to the MAC
	rxDescriptor[active_rx_desc].ui32CtrlStatus = DES0_RX_CTRL_OWN;

}

void network_driver_init(uint32_t sysClkFreq) {
	
	// Reset and initialize ethernet MAC & PHY
	SysCtlPeripheralEnable(SYSCTL_PERIPH_EMAC0);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_EPHY0);
	SysCtlPeripheralReset(SYSCTL_PERIPH_EMAC0);
	SysCtlPeripheralReset(SYSCTL_PERIPH_EPHY0);
	
	// Wait for the MAC to be ready
	while(!SysCtlPeripheralReady(SYSCTL_PERIPH_EMAC0)) {}
  
  // Configure the PHY
	EMACPHYConfigSet(EMAC0_BASE,
		EMAC_PHY_TYPE_INTERNAL |  			//use the built-in PHY
		EMAC_PHY_INT_MDIX_EN |   				//auto crossoverification
		EMAC_PHY_AN_100B_T_FULL_DUPLEX  //autonegotiation, advertise 100BASE_T
	);
		
  // Reset the MAC
	EMACReset(EMAC0_BASE);
		
	// Initialize the MAC
	EMACInit(EMAC0_BASE, sysClkFreq, 
		EMAC_BCONFIG_MIXED_BURST |    // use fixed burst mode DMA transfers
		EMAC_BCONFIG_PRIORITY_FIXED,  // use a fixed priority scheme
	  4, 4,													// max number of words in a single transaction (4 rx 4 tx)
		0															// number of 32-bit words to skip between DMA descriptors (lolwat?)
	); 
	
	// Set MAC configuration stuff
	EMACConfigSet(EMAC0_BASE,
		EMAC_CONFIG_FULL_DUPLEX |					//attempt to operate in full-duplex mode
		EMAC_CONFIG_CHECKSUM_OFFLOAD |		//enable ipv4 header and TCP/UDP/etc checksum checking (reported via status fields)
		EMAC_CONFIG_7BYTE_PREAMBLE |			//use a 7 byte preamble at beginning of every frame
		EMAC_CONFIG_IF_GAP_96BITS |				//interframe gap size between transmitted frames 
		EMAC_CONFIG_USE_MACADDR0 |				//which MAC address to use
		EMAC_CONFIG_SA_FROM_DESCRIPTOR |  //control of MAC address insertion (of MACADDR0) or deletion from DMA xmit descriptor fields
		EMAC_CONFIG_BO_LIMIT_1024,				//range limit for random delay after collision
		EMAC_MODE_RX_STORE_FORWARD |			//wait for a full frame in fifo to DMA it
		EMAC_MODE_TX_STORE_FORWARD |		  // same as ^
		EMAC_MODE_TX_THRESHOLD_128_BYTES | //how full the TX fifo needs to be to begin a DMA transfer
		EMAC_MODE_RX_THRESHOLD_128_BYTES,  //how full the RX fifo needs to be to begin a DMA transfer
		0																	//max frame size
	);
	
	// Initialize the DMA descriptors.. I don't really get this part
	txDescriptor[0].ui32Count = (DES1_TX_CTRL_SADDR_INSERT | (TX_BUFFER_SIZE << DES1_TX_CTRL_BUFF1_SIZE_S));
	txDescriptor[0].pvBuffer1 = tx_buffer;
	txDescriptor[0].ui32CtrlStatus = (DES0_TX_CTRL_LAST_SEG | DES0_TX_CTRL_FIRST_SEG | DES0_TX_CTRL_INTERRUPT | DES0_TX_CTRL_CHAINED | DES0_TX_CTRL_IP_ALL_CKHSUMS);
	txDescriptor[0].DES3.pLink = &txDescriptor[1]; //0 -> 1
	
	txDescriptor[1] = txDescriptor[0];
	txDescriptor[1].DES3.pLink = &txDescriptor[2]; //1 -> 2
	
	txDescriptor[2] = txDescriptor[0];
	txDescriptor[2].DES3.pLink = &txDescriptor[0]; //2 -> 0 end the chain
	
	rxDescriptor[0].ui32CtrlStatus = 0;
	rxDescriptor[0].ui32Count = (DES1_RX_CTRL_CHAINED | (RX_BUFFER_SIZE << DES1_RX_CTRL_BUFF1_SIZE_S));
	rxDescriptor[0].pvBuffer1 = rx_buffer;
	rxDescriptor[0].DES3.pLink = &rxDescriptor[1];	//0 -> 1
	
	rxDescriptor[1] = rxDescriptor[0];
	rxDescriptor[1].DES3.pLink = &rxDescriptor[2];  //1 -> 2
	
	rxDescriptor[2] = rxDescriptor[0];
	rxDescriptor[2].DES3.pLink = &rxDescriptor[0]; //2 -> 0 end the chain
	
	//tell the MAC about our nice new descriptors
	EMACTxDMADescriptorListSet(EMAC0_BASE, txDescriptor);
	EMACRxDMADescriptorListSet(EMAC0_BASE, rxDescriptor);
	
	//tell the MAC its MAC address for inbound packet filtering
	EMACAddrSet(EMAC0_BASE, 0, (uint8_t*) mac_addr);
	
	//wait for the PHY to report link up
//	debug_print("waiting link..\r\n");
//	 while((EMACPHYRead(EMAC0_BASE, 0, EPHY_BMSR) & EPHY_BMSR_LINKSTAT) == 0) {} 
//	debug_print("Link up!!1\r\n");
	
	//set up the MAC xmit frame filter
	EMACFrameFilterSet(EMAC0_BASE, 
		 EMAC_FRMFILTER_SADDR |						//rename xmit frames when the source address field doesn't match SA registers
		 EMAC_FRMFILTER_PASS_MULTICAST |  //pass all multicast frames
		 EMAC_FRMFILTER_PASS_NO_CTRL			//don't pass any control frames
	);
	
	//clear pending MAC interrupts
	EMACIntClear(EMAC0_BASE, EMACIntStatus(EMAC0_BASE, false));
		 
	//enable MAC transmitter and receiver
	EMACTxEnable(EMAC0_BASE);
	EMACRxEnable(EMAC0_BASE);
		 
	//enable MAC interrupt, and the MAC RX Packet interrupt source
	IntEnable(INT_EMAC0);
	EMACIntEnable(EMAC0_BASE, EMAC_INT_RECEIVE);
	EMACIntEnable(EMAC0_BASE, EMAC_INT_TRANSMIT);
	EMACIntRegister(EMAC0_BASE, ethernet_interrupt_handler);

	//initialize the uIP stack
	uip_init();
	
	//set the host IP address
	uip_ipaddr_t ipaddr;
	uip_ipaddr(ipaddr, IPADDR0, IPADDR1, IPADDR2, IPADDR3);
	uip_sethostaddr(ipaddr);
	
	debug_print("Set up uIP host addr: ");
	debug_print_ip((void*)&ipaddr);
	debug_print("\r\n");
	
	//set uip's mac address for ARP
	struct uip_eth_addr temp_addr;
	temp_addr.addr[0] = mac_addr[0];
	temp_addr.addr[1] = mac_addr[1];
	temp_addr.addr[2] = mac_addr[2];
	temp_addr.addr[3] = mac_addr[3];
	temp_addr.addr[4] = mac_addr[4];
	temp_addr.addr[5] = mac_addr[5];
	uip_setethaddr(temp_addr);
	
	//initialize uip arp
	uip_arp_init();
	
	//initialize uip timers
	timer_set(&periodic_timer_for_uip, CLOCK_SECOND / 20); //set uIP TCP/UDP poll timer for 20 Hz (50ms) (THE TELEMETRY RATE IS TIED TO THIS..)
	timer_set(&arp_timer_for_uip, CLOCK_SECOND * 10);     //set uIP ARP timer for 10s
	
	//give the first receive DMA descriptor to the MAC (by setting the OWN bit)
	rxDescriptor[0].ui32CtrlStatus = DES0_RX_CTRL_OWN;
	
	debug_print("network_driver_init() complete\r\n");
}

void network_driver_periodic() {
	//handle a received ethernet frame
	if(rx_flag) { 
		receive_ethernet_frame();
		rx_flag = false;
	}
	
	if(tx_complete_flag) {
		tx_complete_flag = false;
	}
	
	if(timer_expired(&periodic_timer_for_uip)) {
		timer_reset(&periodic_timer_for_uip);
		
		//update TCP connections, polling for data to send
		for(uint16_t i=0; i<UIP_CONNS; i++) {
			uip_periodic(i);
			if(uip_len > 0) {
				uip_arp_out();
				transmit_ethernet_frame();
			}
		}
		
		//update UDP connections, looking for data to send.
		//This caused an appcall() with uip_poll()
		for(uint16_t i=0; i < UIP_UDP_CONNS; i++) {
			uip_udp_periodic(i);
			if(uip_len > 0) {
				uip_arp_out();
				transmit_ethernet_frame();
			}
		}
}
	
	if(timer_expired(&arp_timer_for_uip)) {
		timer_reset(&arp_timer_for_uip);
		uip_arp_timer();
	}
	
}

void tcpip_output() {
	debug_print("tcpip_output!\r\n");
}

//function called by uIP to get clock time
uint32_t clock_time() {
	return systick_clock;
}



