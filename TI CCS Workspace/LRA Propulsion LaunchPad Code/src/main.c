#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "inc/hw_memmap.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/pin_map.h"

#include "inc/hw_ints.h"

#include "debug.h"
#include "status_led.h"
#include "network_driver.h"
#include "networking.h"
#include "transducer.h"
#include "telemetry.h"
#include "thermocouple.h"
#include "solenoid.h"
#include "main.h"

/*

LRA Propulsion Test Stand

Pin Mappings: https://docs.google.com/spreadsheets/d/1FLoMdekCKLfGqCLHUICYdZ5XRLgRkgJR3m1h6rutWZc/edit#gid=0

*/

//global variable to store tick rate (in Hz) of main loop
volatile uint32_t loops_per_second;

//global variable containing master arm state ("are solenoids enabled"?)
volatile uint8_t stand_enabled;

//global variable containing commanded solenoid sattes
volatile uint16_t solenoid_state;

//1 kHz counter
volatile uint32_t systick_clock = 0;

// This runs off the 20 KHz SysTick timer. It is used to schedule
// timed actions.
void sys_tick() {

	systick_clock++;
	
	//run LED PWM at 20 KHz
		status_led_timer_interrupt();

	// run TC reads at 500 Hz (this is divided among TCs so each will get 100 Hz)
	// 500 Hz @ 20KHz clock = 40 clocks
	if(systick_clock % 40 == 0) {
		thermocouple_start_next_transaction();
	}
	
	// run transducer reads at 20 KHz (TODO: make the rate WAY bigger)
	transducer_periodic();
}


int main(void) {
	// Set the system clock to the full 120MHz
	uint32_t sysClkFreq = SysCtlClockFreqSet(SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480, 120000000);
 
	
	debug_init(sysClkFreq);
	status_led_init();
	network_driver_init(sysClkFreq);
	networking_init();
	telemetry_init();
	transducer_init();
	thermocouple_init();
	solenoid_init();
	
	debug_print("Initialization complete. starting main loop.\r\n");
	
	// Set up the SysTick timer and its interrupts
	SysTickPeriodSet(6000); // 20 kHz
	SysTickIntRegister(sys_tick);
	SysTickIntEnable();
	SysTickEnable(); 
	
	uint32_t loopIterations = 0;
	uint32_t frame_start = systick_clock;
	

	
	while(1) {
		status_led_periodic();
		network_driver_periodic();
		telemetry_periodic();
		solenoid_periodic();
		
	//	debug_print_u32(systick_clock);

		//count loop iterations per second
		loopIterations++;
		if(systick_clock - frame_start >= 1000) {
			loops_per_second = loopIterations;
			loopIterations = 0;
			frame_start = systick_clock;
		}
		
	}
}
