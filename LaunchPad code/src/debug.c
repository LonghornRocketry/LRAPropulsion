#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "debug.h"

#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/timer.h"



void debug_init(uint32_t sysClkFreq) {
	
	//Enable the GPIO pins mapped to to UART0
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
	while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA)){}
		
	//configure the pins for the UART
	GPIOPinConfigure(GPIO_PA0_U0RX);
	GPIOPinConfigure(GPIO_PA1_U0TX);
	GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

	//Enable the UART clock signal
	SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
	
	// Set up for 115200, 8, n, 1
	UARTConfigSetExpClk(UART0_BASE, sysClkFreq, 115200,
											UART_CONFIG_PAR_NONE | UART_CONFIG_STOP_ONE |
											UART_CONFIG_WLEN_8);
	
	//Enable the UART
	UARTEnable(UART0_BASE);
	
	//Enable TIMER0 in full-width count up mode
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
	TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC_UP);
	TimerClockSourceSet(TIMER0_BASE, TIMER_CLOCK_SYSTEM); //internal 16 MHz clock
	TimerEnable(TIMER0_BASE, TIMER_BOTH);

	debug_print("\r\n\r\n----------\r\n\r\ndebug_init() complete\r\n");
}

void debug_print(char* str) {
	char* ptr = str;
	while (*ptr != 0) {
		UARTCharPut(UART0_BASE, *ptr);
		ptr++;
	}
}

void debug_print_u32(uint32_t val) {
	char buf[10];
	snprintf(buf, 10, "%u", val);
	debug_print(buf);
}

void debug_print_mac(uint8_t* mac) {
	char buf[20];
	snprintf(buf, 20, "%x-%x-%x-%x-%x-%x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]); 
	debug_print(buf);
}

void debug_print_ip(uint8_t* ip) {
	char buf[20];
	snprintf(buf, 20, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
	debug_print(buf);
}

void debug_delay_us(uint32_t val) {
	uint32_t start = debug_clock_us();
	while((debug_clock_us() - start) < val) {}
}

uint32_t debug_clock_us() 
{
	return (TimerValueGet(TIMER0_BASE, TIMER_A) / 120); //divide 120 MHz timer by 120 to get 1 MHz
}

uint32_t debug_clock_cycles()
{
	return TimerValueGet(TIMER0_BASE, TIMER_A);
}



