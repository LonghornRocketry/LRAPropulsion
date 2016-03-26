#include <string.h>

#include "uip/uip.h"

#include "telemetry.h"
#include "transducer.h"
#include "debug.h"
#include "main.h"

bool new_packet = false;
telemetry_packet_t packet;

void telemetry_init() {
	debug_print("telemetry_init() complete!\r\n");
}

void telemetry_periodic() {
	// TODO: implement this as a timer
	if(!new_packet) {
		
		//build a new packet
		packet.timestamp = systick_clock;
		memcpy(packet.transducer_val, transducer_val, sizeof(transducer_val));
		packet.loops_per_second = loops_per_second;
		//queue it up for sending
		new_packet = true;
		
	}
}

bool telemetry_new_packet() {
	return new_packet;
}

telemetry_packet_t* telemetry_get_packet() {
	
	//mark the packet as sent
	new_packet = false;
	return &packet;
	
}
