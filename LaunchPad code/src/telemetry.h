#ifndef TELEMETRY_H
#define TELEMETRY_H

#include <stdint.h>
#include <stdbool.h>

struct telemetry_packet {
	uint32_t timestamp;
	uint16_t transducer_val[4];
	uint32_t loops_per_second;
};

typedef struct telemetry_packet telemetry_packet_t;

void telemetry_init(void);
void telemetry_periodic(void);
bool telemetry_new_packet(void);
telemetry_packet_t* telemetry_get_packet(void);

#endif /*TELEMETRY_H*/
