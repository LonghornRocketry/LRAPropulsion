#include <string.h>

#include "uip/uip.h"

#include "telemetry.h"
#include "transducer.h"
#include "solenoid.h"
#include "debug.h"
#include "main.h"

static bool new_packet = false;
static telemetry_packet_t packet;

void telemetry_init() {
	debug_print("telemetry_init() complete\r\n");
}

void telemetry_periodic() {
	// TODO: implement this as a timer
	if(!new_packet) {
		
		//build a new packet
		
		packet.timestamp = systick_clock;
		for(int i=0; i<4; i++) {
			packet.transducer_val[i] = transducer_val[i];
		}
		memcpy(&packet.tc_data, &tc_data, sizeof(tc_data));
		packet.loops_per_second = loops_per_second;
		packet.stand_armed = stand_armed;
		packet.solenoids_powered = are_solenoids_powered();
		packet.solenoid_state = solenoid_state;
		
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
