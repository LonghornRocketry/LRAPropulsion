#ifndef TELEMETRY_H
#define TELEMETRY_H

#include <stdint.h>
#include <stdbool.h>

#include "thermocouple.h"

__packed struct telemetry_packet {
	uint32_t timestamp;
	uint16_t transducer_val[4];
	uint32_t loops_per_second;
	tc_data_t tc_data;
	uint8_t stand_armed;
	uint8_t solenoids_powered;
	uint16_t solenoid_state;
};

typedef struct telemetry_packet telemetry_packet_t;

void telemetry_init(void);
void telemetry_periodic(void);
bool telemetry_new_packet(void);
telemetry_packet_t* telemetry_get_packet(void);

#endif /*TELEMETRY_H*/
