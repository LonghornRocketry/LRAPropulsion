#include <stdbool.h>

#include "packet_receiver.h"
#include "main.h"
#include "debug.h"

#include "driverlib/gpio.h"
#include "inc/hw_memmap.h"

void process_packet(uint8_t* data, uint16_t len) {
	
	uint8_t type = data[0];
	len--;  //len is now the length of the actual packet
	data++; //advance ptr to start of data
	
	if(type == PACKET_BASIC) {
		
		if (len != sizeof(struct packet_basic)) {
			debug_print("packet_receiver.c: wrong packet size!\r\n");
			return;
		}
		
		struct packet_basic *pkt = (struct packet_basic *) data;
		
		stand_enabled = pkt->stand_enable;
		solenoid_state = pkt->solenoid_states;
		
		watchdog_pet();

	} else {
		debug_print("packet_receiver got unknown packet type: ");
		debug_print_u32(type);
		debug_print("\r\n");
	}
}
