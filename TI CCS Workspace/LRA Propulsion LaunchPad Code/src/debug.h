#ifndef __DEBUG_H__
#define __DEBUG_H__
// debug.h
// Utilities for outputting debug data and timing stuff


void debug_init(uint32_t sysClkFreq);

void debug_print(char* str);
void debug_print_u32(uint32_t val);
void debug_print_mac(uint8_t* mac);
void debug_print_ip(uint8_t* ip);

void debug_delay_us(uint32_t count);

uint32_t debug_clock_us(void);
uint32_t debug_clock_cycles(void);

void uip_log(char* str);

#endif /*__DEBUG_H__*/
