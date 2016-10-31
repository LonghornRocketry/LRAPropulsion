// networking.c - application side of the uIP stack

#include <string.h>

#include "uip/uip.h"
#include "uip/uip_arp.h"

#include "networking.h"
#include "debug.h"
#include "telemetry.h"
#include "packet_receiver.h"

struct uip_udp_conn *packet_receive_conn;
struct uip_udp_conn *telemetry_send_conn;

uint8_t remote_ip[4] = {10, 0, 0, 1};
#define remote_port    6240

void networking_init() {
	//start packet receiver UDP listen
	uip_ipaddr_t listen_addr;
	uip_ipaddr(&listen_addr, 0, 0, 0, 0);
	packet_receive_conn = uip_udp_new(&listen_addr, 0); //make remote port 0 for listen
	uip_udp_bind(packet_receive_conn, HTONS(remote_port));
	
	//open UDP socket for sending telemetry
	telemetry_send_conn = uip_udp_new((void*) remote_ip, HTONS(remote_port));
	
	debug_print("networking_init() complete\r\n");
}

//called by uIP during networking_periodic when it has something to tell us about TCP stuff
void uip_appcall() {
	debug_print("uip_appcall!!\r\n");
}

//called by uIP during networking_periodic when it has something to tell us about UDP stuff
void uip_udp_appcall() {
	
	//check for new data in our sockets
	if(uip_newdata()) {
		if(uip_udp_conn == packet_receive_conn) {
			//we got a packet! process it!
			process_packet((uint8_t*) uip_appdata, uip_len);
		}
	}
	
	//check if uIP is polling a UDP connection for data
	if(uip_poll()) {
		//poll telemetry.c for a new packet to send
		if(uip_udp_conn == telemetry_send_conn && telemetry_new_packet()) {
			telemetry_packet_t *packet = telemetry_get_packet();
			memcpy(uip_appdata, packet, sizeof(telemetry_packet_t));
			uip_udp_send(sizeof(telemetry_packet_t));
		}
	}
	
}

void uip_log(char* str) {
	debug_print("UIP: ");
	debug_print(str);
	debug_print("\r\n");
}
