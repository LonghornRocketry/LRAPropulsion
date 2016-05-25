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
//		Mux 0   PH0
//    Mux 1   PH1
//    Mux 2   PH2
//    Mux 3   PH3

volatile uint16_t transducer_val[16];

//This variable keeps track of which channel is "in flight" - 
//what the ADC is converting in between calls to transducer_periodic().
static volatile uint8_t current_channel = 0;

//This variable keeps track of which channel is the one we're reading
//out of the ADC right now over SPI.
static volatile uint8_t last_channel = 0;

//keep track of whether a transaction is currently happening
static volatile bool transaction_in_flight = false;

static void transaction_complete();

// empty the SPI port receive FIFO in preparation for use.
static void clear_receive_fifo() {
	uint32_t dummy_data;
	while(SSIDataGetNonBlocking(SSI2_BASE, &dummy_data) != 0) {}
}

void transducer_init() {
	//enable GPIO port for analog mux control pins
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOH);
	while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOH)){}
		
	//config amux control pins as outputs
	GPIOPinTypeGPIOOutput(GPIO_PORTH_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);
	
	//enable the GPIO port for the SPI pins
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
	while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOD)){}
	
	//enable the SPI port for the ADC (SSI2)
	SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI2);
	while(!SysCtlPeripheralReady(SYSCTL_PERIPH_SSI2)){}
		
	// Configure GPIO PORTD pins for SPI
	GPIOPinConfigure(GPIO_PD0_SSI2XDAT1);
	GPIOPinConfigure(GPIO_PD3_SSI2CLK);
	GPIOPinTypeSSI(GPIO_PORTD_BASE, GPIO_PIN_0 | GPIO_PIN_3);
		
	//enable GPIO Port PB3 (for ~CS)
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
	while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOB)){}
	
	//set PB3 as an output
	GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, GPIO_PIN_3);

	//set PB3 high so ADC is deselected 
	GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_3, GPIO_PIN_3); 
		
	//configure the SPI port (SSI2). Serial clock inactive low, ADC outputs data on falling edge of CLK
	SSIClockSourceSet(SSI2_BASE, SSI_CLOCK_SYSTEM); //use the system clock as a timing source
	SSIConfigSetExpClk(SSI2_BASE, 120000000, SSI_FRF_MOTO_MODE_1, SSI_MODE_MASTER, 15000000, 8); // SPI, 15MHz, 8 bits per frame
	SSIEnable(SSI2_BASE);
		
	//reset the ADC (generate clk signals by writing dummy data).
	//the ADC will reset when it receives 8 clock pulses with ~CS (PB3) low.
	GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_3, 0);
		
		//8 cycles of dummy data
		SSIDataPut(SSI2_BASE, 0x00);
		
		//wait for xmit to complete
		while(SSIBusy(SSI2_BASE)){}
	
	//write ~CS high to end the transaction
	GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_3, GPIO_PIN_3);
	
	//wait a bit
	for(volatile int i=0; i<100l; i++) {}
		
	clear_receive_fifo();
		
	//set transaction_complete() as the SPI interrupt handler
	SSIIntRegister(SSI2_BASE, transaction_complete);
	
	debug_print("transducer_init() complete\r\n");
}

//set the current channel connected to the ADC input pin
//by changing the control pins on the analog mux IC.
static void set_amux(uint8_t channel) {
	GPIOPinWrite(GPIO_PORTH_BASE, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, channel & 0x0F);
}


// starts an ADC conversion, and reads out the data from the
// previously started conversion. You must leave at least 3
// uS between calls for the ADC conversion or this function will 
// return junk data.
static void start_transaction() {
	
	if(transaction_in_flight) return;
	transaction_in_flight = true;
	
	//bring ~CS low to start the transaction
	GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_3, 0);
	
	//pass in 3 bytes of dummy data to read out 3 bytes of data
	SSIDataPut(SSI2_BASE, 0x00);
	SSIDataPut(SSI2_BASE, 0x00);
	SSIDataPut(SSI2_BASE, 0x00);
	
	//arm the "transmit complete" interrupt (TX End of Transmission).
	//this will call transaction_complete() when the SPI transaction finishes.
	SSIIntEnable(SSI2_BASE, SSI_TXEOT);

}

//store the acquired data out of the SPI FIFO.
static void transaction_complete() {
	
	//bring ~CS high to end transaction
	GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_3, GPIO_PIN_3);
	
	uint32_t data0, data1, data2;
	
	//read out 3 bytes from the receive FIFO
	SSIDataGet(SSI2_BASE, &data0);
	SSIDataGet(SSI2_BASE, &data1);
	SSIDataGet(SSI2_BASE, &data2);
	
	uint16_t val = ((data0 << 8) & 0xFF00) | ((data1) & 0xFF);
	
	//store decoded value
	transducer_val[last_channel] = val;
	
	transaction_in_flight = false;
}



//This function uses do_adc_read() for each channel, so it must
//be called with a delay of at least 3 uS between calls.
void transducer_periodic() {
	
	//get ready to start converting the next channel
	last_channel = current_channel;
	current_channel = (current_channel+1) % 16;
	
	//connect the NEXT channel to the ADC.
	set_amux(current_channel);
	
	//start a new readout
	start_transaction();
}
