#include <stdint.h>
#include <stdbool.h>

#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"
#include "inc/hw_memmap.h"
#include "inc/tm4c1294ncpdt.h"

#include "status_led.h"

/* PIN MAPPINGS

PN2     - LED R
PN3     - LED G
PN4     - LED B

PN1     - USR LED 1
PN0     - USR LED 2
PF4     - USR LED 3 (Ethernet LED 1)
PF0     - USR LED 4 (NOT USED HERE, RESERVED FOR SD CARD SPI)
*/

void status_led_init() {
	// Enable the GPIO ports that are used for the on-board LEDs.
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);

	// Check if the peripheral access is enabled.
	while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPION)){}
	while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF)){}

	//configure USR LEDs 1 and 2 as GPIO outputs
	GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_0);
	GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_1);
	
	//configure the PF4 LED as an ethernet indicator
	GPIOPinConfigure(GPIO_PF4_EN0LED1);
	GPIOPinTypeEthernetLED(GPIO_PORTF_BASE, GPIO_PIN_4);
		
	// Enable the GPIO port used for the RGB LED
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
	while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPION)){}
	
	// congigure the RGB LED ports as outputs
	GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4);
	// gimme mo power
	GPIOPadConfigSet(GPIO_PORTN_BASE, GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4, GPIO_STRENGTH_8MA, GPIO_PIN_TYPE_STD);
}

void status_led_periodic() {
	GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_4|GPIO_PIN_3, GPIO_PIN_4|GPIO_PIN_3);
	GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, GPIO_PIN_1);
}
