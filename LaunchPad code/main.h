#ifndef MAIN_H
#define MAIN_H

#include <stdint.h>

//set the system tick frequency to 1 kHz
#define SYSTICK_FREQ 1000

extern volatile uint32_t systick_clock;
extern volatile uint32_t loops_per_second;

#endif /*MAIN_H*/
