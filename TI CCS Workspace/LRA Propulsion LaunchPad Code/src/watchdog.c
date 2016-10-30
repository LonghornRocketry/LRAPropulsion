/*
 * watchdog.c
 *
 *  Created on: Oct 29, 2016
 *      Author: james
 */

#include <stdint.h>

#include "debug.h"
#include "watchdog.h"

#define WATCHDOG_TIME_MS 250

static bool watchdog_alive = false;
static uint32_t watchdog_last_petted = 0;

void watchdog_pet() {
	watchdog_alive = true;
	watchdog_last_petted = debug_clock_us();
}

bool watchdog_is_alive() {

	if (!watchdog_alive) return false;

	uint32_t time_ms = (debug_clock_us() - watchdog_last_petted) / 1000;

	if(time_ms > WATCHDOG_TIME_MS) {
		watchdog_alive = false;
		return false;
	} else {
		return true;
	}
}



