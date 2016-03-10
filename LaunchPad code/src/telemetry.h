#ifndef TELEMETRY_H
#define TELEMETRY_H

#include <stdint.h>
#include <stdbool.h>

struct telemetry_packet {
	uint32_t timestamp;
	uint16_t transducer_val[4];
};

typedef struct telemetry_packet telem_packet_t;

extern bool telemetry_new_packet;
extern telem_packet_t telemetry_packet;
extern struct uip_udp_conn* telemetry_udp_conn;

void telemetry_init(void);
void telemetry_periodic(void);

#endif /*TELEMETRY_H*/
