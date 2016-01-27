#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"


int main(void)
{
	// Set the system clock to the full 120MHz
	SysCtlClockFreqSet(SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480, 120000000);
	
    //
    // Enable the GPIO port that is used for the on-board LED.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
		SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    //
    // Check if the peripheral access is enabled.
    //
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPION))
    {
    }
		while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF))
    {
    }

    //
    // Enable the GPIO pin for the LED (PN0).  Set the direction as output, and
    // enable the GPIO pin for digital function.
    //
    GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_0);
		GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_1);
		GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_0);
		GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_4);
    //
    // Loop forever.
    //
    while(1)
    {
        //
        // Turn on the LED.
        //
        GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, 1);
				GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, GPIO_PIN_1);
				GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, 1);
				GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_4, GPIO_PIN_4);
        //
        // Delay for a bit.
        //
        for(volatile uint32_t ui32Loop = 0; ui32Loop < 400000; ui32Loop++)
        {
        }

        //
        // Turn off the LED.
        //
       // GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, 0x0);
				GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, 0x0);
				GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, 0x0);
				GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_4, 0);
        //
        // Delay for a bit.
        //
        for(volatile uint32_t ui32Loop = 0; ui32Loop < 400000; ui32Loop++)
        {
        }
    }
}
