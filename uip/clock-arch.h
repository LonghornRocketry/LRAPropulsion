#ifndef __CLOCK_ARCH_H__
#define __CLOCK_ARCH_H__

#include <stdint.h>
#include "main.h"

//how many clock ticks are in one second?
#define CLOCK_CONF_SECOND       SYSTICK_FREQ

//clock time to use for system ticks (output of clock_time())
typedef uint32_t clock_time_t;

#endif // __CLOCK_ARCH_H__
