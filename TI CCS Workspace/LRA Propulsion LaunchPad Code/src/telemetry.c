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

// NOTE: a telemetry packet is sent every time uIP polls the socket for data.
// this rate is controlled by the uIP poll timer in network_driver.c
// (right now it's 20 Hz, or 50 ms).

void telemetry_periodic() {
	
	// build a new packet if the last one just got sent
	if(!new_packet) {
		
		//build a new packet
		
		packet.timestamp = systick_clock;


		packet.mainloop_rate = loops_per_second;
		packet.stand_armed = stand_enabled;
		packet.solenoids_powered = are_solenoids_powered();
		packet.solenoid_state = solenoid_state;
		
		memcpy(&packet.tc_data, &tc_data, sizeof(tc_data));
		for(int i=0; i<16; i++) {
			packet.transducer_val[i] = transducer_val[i];
		}
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
