#ifndef PACKET_RECEIVER_H
#define PACKET_RECEIVER_H

#include <stdint.h>

/* -------------------
		PACKET FORMAT
		
		byte [0] 		- packet type (uint8_t)
		byte [1..n] - data (cast to packet-specific struct)
		
   ------------------- */


#define PACKET_BASIC 0

__packed struct packet_basic {
	uint8_t stand_arm;
	uint16_t solenoid_states;
};

void process_packet(uint8_t* data, uint16_t len);



#endif /*PACKET_RECEIVER_H*/
