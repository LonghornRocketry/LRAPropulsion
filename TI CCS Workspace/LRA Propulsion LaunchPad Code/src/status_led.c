#include <stdint.h>
#include <stdbool.h>

#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"
#include "inc/hw_memmap.h"
#include "inc/tm4c1294ncpdt.h"

#include "status_led.h"
#include "solenoid.h"
#include "main.h"

/* PIN MAPPINGS

PN2     - LED B
PN3     - LED R
PN4     - LED G

PN1     - USR LED 1
PN0     - USR LED 2
PF4     - USR LED 3 (Ethernet LED 1)
PF0     - USR LED 4 (NOT USED HERE, RESERVED FOR SD CARD SPI)
*/

static volatile uint8_t r, g, b;

static volatile uint8_t brightness_1, brightness_2;

static uint8_t pwm_counter;

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

void status_led_timer_interrupt() {

	pwm_counter++;

	uint8_t r_on = (r > pwm_counter) ? 1:0;
	uint8_t g_on = (g > pwm_counter) ? 1:0;
	uint8_t b_on = (b > pwm_counter) ? 1:0;

	uint8_t rgb_bits = (r_on << 3) | (g_on << 4) | (b_on << 2);

	uint8_t led1_on = (brightness_1 > pwm_counter) ? 1:0;
	uint8_t led2_on = (brightness_2 > pwm_counter) ? 1:0;

	uint8_t portn_bits = (led1_on << 1) | led2_on;

	GPIOPinWrite(GPIO_PORTN_BASE, 0x1F, rgb_bits | portn_bits);
}

void status_led_periodic() {
	if(!are_solenoids_powered()) {
		r = 255;
		g = 0;
		b = 0;
	} else if (!stand_enabled) {
		r = 255;
		g = 150;
		b = 3;
	} else {
		r = 0;
		g = 255;
		b = 3;
	}
	brightness_1 = 3;
	brightness_2 = 3;

}
