#ifndef __DEBUG_H__
#define __DEBUG_H__
// debug.h
// Utilities for outputting debug data and timing stuff


void debug_init(uint32_t sysClkFreq);

void debug_print(char* str);
void debug_print_u32(uint32_t val);
uint32_t debug_clock_us(void);

#endif /*__DEBUG_H__*/
