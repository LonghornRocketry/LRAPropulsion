#ifndef MAIN_H
#define MAIN_H

#include <stdint.h>

//set the system tick frequency to 1 kHz
#define SYSTICK_FREQ 1000

extern volatile uint32_t systick_clock;
extern volatile uint32_t loops_per_second;

extern volatile uint8_t stand_enabled;
extern volatile uint16_t solenoid_state;

#endif /*MAIN_H*/
