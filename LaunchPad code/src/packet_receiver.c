#include "packet_receiver.h"
#include "debug.h"

void process_packet(uint8_t* data, uint16_t len) {
	
	uint8_t type = data[0];
	len--; //len is now the length of the packet size
	
	if(type == PACKET_BASIC) {
		
		if (len != sizeof(struct packet_basic)) {
			debug_print("wrong packet size");
		}
		
		struct packet_basic *pkt = (struct packet_basic *) data;
		
		debug_print_u32(pkt->led0);
		
	} else {
		debug_print("packet_receiver got unknown packet type: ");
		debug_print_u32(type);
		debug_print("\r\n");
	}
}
