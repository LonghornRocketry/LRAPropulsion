#include <stdbool.h>

#include "packet_receiver.h"
#include "debug.h"

#include "driverlib/gpio.h"
#include "inc/hw_memmap.h"

void process_packet(uint8_t* data, uint16_t len) {
	
	uint8_t type = data[0];
	len--;  //len is now the length of the actual packet
	data++; //advance ptr to start of data
	
	if(type == PACKET_BASIC) {
		
		if (len != sizeof(struct packet_basic)) {
			debug_print("wrong packet size");
		}
		
		struct packet_basic *pkt = (struct packet_basic *) data;
		
		if (pkt->led0) {
			GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, GPIO_PIN_1);
		} else {
			GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, 0);
		}
		
	} else {
		debug_print("packet_receiver got unknown packet type: ");
		debug_print_u32(type);
		debug_print("\r\n");
	}
}
