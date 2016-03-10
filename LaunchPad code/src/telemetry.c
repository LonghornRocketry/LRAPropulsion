#include <string.h>

#include "uip/uip.h"

#include "telemetry.h"
#include "transducer.h"
#include "debug.h"
#include "main.h"

uint8_t remote_ip[4] = {10, 0, 0, 1};

bool telemetry_new_packet;
telem_packet_t telemetry_packet;
struct uip_udp_conn* telemetry_udp_conn;

void telemetry_init() {
	telemetry_udp_conn = uip_udp_new((void*) remote_ip, HTONS(6240));
	debug_print("telemetry_init() complete!\r\n");
}

void telemetry_periodic() {
	
	if(telemetry_new_packet) {
		// networking hasn't picked up our last packet yet... ;_;
	} else {
		telemetry_packet.timestamp = systick_clock;
		memcpy(telemetry_packet.transducer_val, transducer_val,  sizeof(transducer_val));
		telemetry_new_packet = true;
	}
	
}
