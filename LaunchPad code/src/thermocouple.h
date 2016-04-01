#ifndef THERMOCOUPLE_H
#define THERMOCOUPLE_H

#include <stdint.h>
#include <stdbool.h>

void thermocouple_init(void);
void thermocouple_periodic(void);

__packed struct thermocouple_data{
	int16_t tc_val;
	int16_t internal_temp;
	bool fault;
	bool short_to_vcc;
	bool short_to_gnd;
	bool open_circuit;
};

typedef struct thermocouple_data tc_data_t;

extern tc_data_t tc_data;

#endif /*THERMOCOUPLE_H*/

