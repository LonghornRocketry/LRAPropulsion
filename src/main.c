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
#include "networking.h"

bool led_on = false;
void sys_tick() {
	if (led_on) {
		GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, 0);
	} else {
		GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, GPIO_PIN_0);
	}
  led_on = !led_on;
}


int main(void)
{
	// Set the system clock to the full 120MHz
	uint32_t sysClkFreq = SysCtlClockFreqSet(SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480, 120000000);
	
	// Enable the GPIO port that is used for the on-board LED.
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);

	// Check if the peripheral access is enabled.
	while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPION))
	{
	}
	while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF))
	{
	}

	GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_0);
	GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_1);
	
	//configure the two PORTF LEDs as ethernet indicators
	GPIOPinConfigure(GPIO_PF0_EN0LED0);
	GPIOPinConfigure(GPIO_PF4_EN0LED1);
	GPIOPinTypeEthernetLED(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_4);
  
	// Set up the SysTick timer and its interrupts
	SysTickPeriodSet(10000000);
	SysTickIntRegister(sys_tick);
	SysTickIntEnable();
	SysTickEnable(); 
	
	debug_init(sysClkFreq);
	networking_init(sysClkFreq);
	
	while(1) {
		networking_periodic();
	}
}
