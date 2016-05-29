#include <stdint.h>
#include <stdbool.h>

#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "inc/hw_memmap.h"
#include "inc/tm4c1294ncpdt.h"

#include "solenoid.h"
#include "debug.h"
#include "main.h"

/*
PIN MAPPINGS

SOL 0  - PK0
SOL 1  - PK1
SOL 2  - PK2
SOL 3  - PK3
SOL 4  - PK4
SOL 5  - PK5
SOL 6  - PK6
SOL 7  - PK7

SOL 8  - PL0
SOL 9  - PL1

SOL PWR SENSE (input) - PC7
*/

static volatile bool solenoids_enabled = false;

void solenoid_init() {
	
	// Enable GPIO Port K clock
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOK);
	while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOK)){};
	
	// Enable GPIO Port L clock
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOL);
	while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOL)){};
	
	// Enable GPIO Port C clock
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
	while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOC)){};
		
	// Configure PC7 as an input
	GPIOPinTypeGPIOInput(GPIO_PORTC_BASE, GPIO_PIN_7);
		
	debug_print("solenoid_init() complete\r\n");
	
}

void enable_solenoids() {

	//clear any data (turn off solenoids) before we enable outputs
	GPIOPinWrite(GPIO_PORTK_BASE, 0xFF, 0x00);
	GPIOPinWrite(GPIO_PORTL_BASE, 0x03, 0x00);
	
	// configure the solenoid pins as outputs
	GPIOPinTypeGPIOOutput(GPIO_PORTK_BASE, 0xFF);
	GPIOPinTypeGPIOOutput(GPIO_PORTL_BASE, 0x03);

	solenoids_enabled = true;
}

void disable_solenoids() {
	//clear any data (turn off solenoids)
	GPIOPinWrite(GPIO_PORTK_BASE, 0xFF, 0x00);
	GPIOPinWrite(GPIO_PORTL_BASE, 0x03, 0x00);
	
	// configure the solenoid GPIOs as inputs, putting them in High-Z state
	GPIOPinTypeGPIOInput(GPIO_PORTK_BASE, 0xFF);
	GPIOPinTypeGPIOInput(GPIO_PORTL_BASE, 0x03);
	
	solenoids_enabled = false;
}

void solenoid_periodic() {
	if (stand_enabled) {
		
		if(!solenoids_enabled) enable_solenoids();
		
		//write solenoids 0-7
		GPIOPinWrite(GPIO_PORTK_BASE, 0xFF, (solenoid_state & 0xFF));
		
		//write solenoids 8-9
		GPIOPinWrite(GPIO_PORTL_BASE, 0x03, (solenoid_state >> 8) & 0x03);
		
	} else {
		disable_solenoids();
	}
}

bool are_solenoids_powered() {
	return GPIOPinRead(GPIO_PORTC_BASE, GPIO_PIN_7);
}
