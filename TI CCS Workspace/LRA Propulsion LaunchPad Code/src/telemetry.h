#ifndef TELEMETRY_H
#define TELEMETRY_H

#include <stdint.h>
#include <stdbool.h>

#include "thermocouple.h"

struct __attribute__ ((packed)) telemetry_packet {
	
	//diagnostic info
	uint32_t timestamp;					//ms, time since poweron
	uint32_t mainloop_rate;			//Hz
	uint32_t transducer_rate;		//Hz
	uint32_t transducer_time;   //uS, time spent total for last ISR execution
	uint32_t thermocouple_rate; //Hz
	uint32_t thermocouple_time; //uS, time spent total for last ISR execution
	//status info
	uint8_t stand_armed;
	uint8_t solenoids_powered;
	uint16_t solenoid_state;
	
	//sensor data
	uint16_t transducer_val[16];
	tc_data_t tc_data[5];
	
};


typedef struct telemetry_packet telemetry_packet_t;

void telemetry_init(void);
void telemetry_periodic(void);
bool telemetry_new_packet(void);
telemetry_packet_t* telemetry_get_packet(void);

#endif /*TELEMETRY_H*/
