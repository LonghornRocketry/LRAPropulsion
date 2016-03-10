#include <stdint.h>
#include <stdbool.h>

#include "driverlib/ssi.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"
#include "inc/hw_memmap.h"

#include "debug.h"
#include "main.h"
#include "transducer.h"

// PORTS:
//		MISO 		PD0 (as SSI2 MISO)	
//    SCLK    PD3 (as SSI2 SCLK)
//    ~CS     PB3 (as GPIO)
//
//		Mux 0   PL0
//    Mux 1   PL1
//    Mux 2   PL2
//    Mux 3   PL3

uint16_t transducer_val[4];

static void clear_receive_fifo() {
	uint32_t dummy_data;
	while(SSIDataGetNonBlocking(SSI2_BASE, &dummy_data) != 0) {}
}

void transducer_init() {
	//enable GPIO port for analog mux control pins
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOL);
	while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOL)){}
		
	//config amux control pins as outputs
	GPIOPinTypeGPIOOutput(GPIO_PORTL_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);
	
	//enable the GPIO port for the SPI pins
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
	while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOD)){}
	
	//enable the SPI port for the ADC
	SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI2);
	while(!SysCtlPeripheralReady(SYSCTL_PERIPH_SSI2)){}
		
	// Configure GPIO PORTD pins for SPI
	GPIOPinConfigure(GPIO_PD0_SSI2XDAT1);
	GPIOPinConfigure(GPIO_PD3_SSI2CLK);
	GPIOPinTypeSSI(GPIO_PORTD_BASE, GPIO_PIN_0 | GPIO_PIN_3);
		
	//enable PB3 for ~CS
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
	while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOB)){}
	
	//set PB3 as an output
	GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, GPIO_PIN_3);

	//set PB3 high so ADC is deselected 
	GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_3, GPIO_PIN_3); 
		
	//configure the SPI port (SSI2). Serial clock inactive low, ADC outputs data on falling edge of CLK
	SSIClockSourceSet(SSI2_BASE, SSI_CLOCK_SYSTEM);
	SSIConfigSetExpClk(SSI2_BASE, 120000000, SSI_FRF_MOTO_MODE_1, SSI_MODE_MASTER, 15000000, 8); // SPI, 15MHz, 8 bits per frame
	SSIEnable(SSI2_BASE);
		
	//reset the ADC (generate clk signals by writing dummy data).
	//the ADC will reset when it receives 8 clock pulses with ~CS (PB3) low.
	GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_3, 0);
		
		//8 cycles of dummy data
		SSIDataPut(SSI2_BASE, 0x00);
		
		//wait for xmit to complete
		while(SSIBusy(SSI2_BASE)){}
			
	GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_3, GPIO_PIN_3);
	
	//wait a bit
	for(volatile int i=0; i<100l; i++) {}
	
	clear_receive_fifo();
	
		debug_print("transducer_init() complete!\r\n");
}

static uint16_t do_adc_read() {

	uint32_t data0, data1, data2;
	
	//bring ~CS low to start the transaction
	GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_3, 0);
	
	//pass in 3 bytes of dummy data to read out 3 bytes of data
	SSIDataPut(SSI2_BASE, 0x00);
	SSIDataPut(SSI2_BASE, 0x00);
	SSIDataPut(SSI2_BASE, 0x00);
	
	//wait for transmit to complete
	while(SSIBusy(SSI2_BASE)){}
		
	//bring ~CS high to end transaction
	GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_3, GPIO_PIN_3);
	
	//read out 3 bytes from the receive FIFO
	SSIDataGet(SSI2_BASE, &data0);
	SSIDataGet(SSI2_BASE, &data1);
	SSIDataGet(SSI2_BASE, &data2);
	
	uint16_t val = ((data0 << 8) & 0xFF00) | ((data1) & 0xFF);
		
	return val;
}

static void set_amux(uint8_t channel) {
	GPIOPinWrite(GPIO_PORTL_BASE, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, channel & 0x0F);
}

void transducer_periodic() {
	
		//convert channel 0
		set_amux(0);
		do_adc_read();
	
		debug_delay_us(3);
	
		//convert chan 1, output chan 0
		set_amux(1);
		transducer_val[0] = do_adc_read();
	
		debug_delay_us(3);
	
		//convert chan 2, output chan 1
		set_amux(2);
		transducer_val[1] = do_adc_read();
	
		debug_delay_us(3);
	
		//convert chan 3, output chan 2
		set_amux(3);
		transducer_val[2] = do_adc_read();
		
		//output chan 3
		debug_delay_us(3);
		transducer_val[3] = do_adc_read();
		
}
