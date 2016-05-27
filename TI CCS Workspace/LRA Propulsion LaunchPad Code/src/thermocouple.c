#include <stdint.h>
#include <stdbool.h>

#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/ssi.h"
#include "driverlib/pin_map.h"
#include "inc/hw_memmap.h"
#include "inc/tm4c1294ncpdt.h"

#include "thermocouple.h"
#include "debug.h"

//PIN CONNECTIONS:
//
// PA2 - SCLK (as SSI0Clk)
// PA5 - MISO (as SSI0XDAT1)
//
// PM0 - CS 0
// PM1 - CS 1
// PM2 - CS 2
// PM3 - CS 3
// PM4 - CS 4

/* global variable containing current status of the 5 thermocouples */
volatile tc_data_t tc_data[5];

/* which thermocouple transaction is currently "in flight" */
static uint8_t current_tc = 0;

/* to keep track of whether or not an SPI transaction is happening right now. */
static volatile bool transaction_in_flight = false;

static void read_out_tc_data();


bool thermocouple_busy() {
	return transaction_in_flight;
}

void thermocouple_init() {
	//enable the port for the SPI pins
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
	while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA)) {}
		
	//enable SSI0 for SPI comm
	SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI0);
	while(!SysCtlPeripheralReady(SYSCTL_PERIPH_SSI0)) {}
		
	//configure PA2 (SCLK) and PA5 (MISO) pins for comm
	GPIOPinConfigure(GPIO_PA2_SSI0CLK);
	GPIOPinConfigure(GPIO_PA5_SSI0XDAT1);
    GPIOPinTypeSSI(GPIO_PORTA_BASE, GPIO_PIN_2);
    GPIOPinTypeSSI(GPIO_PORTA_BASE, GPIO_PIN_5);
		
	//configure the SPI port (SSI0). Sample on rising edge
	SSIClockSourceSet(SSI0_BASE, SSI_CLOCK_SYSTEM);
	SSIConfigSetExpClk(SSI0_BASE, 120000000, SSI_FRF_MOTO_MODE_1, SSI_MODE_MASTER, 5000000, 8); // SPI, SCLK idle low, sample on falling edge, 5MHz, 8 bits per frame
	SSIEnable(SSI0_BASE);	
		
	//enable the port for the CS pins
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOM);
	while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOM)) {}
		
	// enable PM0-5 as outputs for the CS pins
	GPIOPinTypeGPIOOutput(GPIO_PORTM_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4);
	
	//make all the ~CSes high
	GPIOPinWrite(GPIO_PORTM_BASE, 0x1F, 0x1F);

	// set read_out_tc_data() as the interrupt handler for SSI0 interrupts
	SSIIntRegister(SSI0_BASE, read_out_tc_data);
		
	debug_print("thermocouple_init() complete\r\n");
}

// starts an SPI read for the the thermocouple denoted by
// tc_id. This sets up an interrupt which will call read_out_tc_data()
// when the transaction completes.
void thermocouple_start_next_transaction() {
	
	//make sure we aren't stepping on an in flight transaction
	if(transaction_in_flight) return;

	//ok we're good to start
	transaction_in_flight = true;
	
	//1. move on to the next thermocouple in line
	current_tc = (current_tc + 1) % 5;
	
	//2. set CS low for our particular thermocouple
	GPIOPinWrite(GPIO_PORTM_BASE, 0x1F, ~(1 << current_tc));
	
	//3. wait 100 ns
	for(volatile int i=0; i < 8; i++) {}
	
	//4. start transfer of 32 bits
	SSIDataPut(SSI0_BASE, 0x00);
	SSIDataPut(SSI0_BASE, 0x00);
	SSIDataPut(SSI0_BASE, 0x00);
	SSIDataPut(SSI0_BASE, 0x00);
	
		
  //5. arm interrupt for SSI transfer complete (SSI TX End Of Transmission).
	//   the interrupt will call read_out_tc_data() when the transaction completes.
	SSIIntEnable(SSI0_BASE, SSI_TXEOT);
}

// finishes the current transaction
// updates tc_data[tc_id] with the next 4 bytes read out of the 
// SPI receive FIFO.
static void read_out_tc_data() {
	
	//Acknowledge interrupt
	SSIIntClear(SSI0_BASE, SSI_TXEOT);
	
	//Disable interrupt
	SSIIntDisable(SSI0_BASE, SSI_TXEOT);
	
	//1. end current transaction by bringing ~CS high
	GPIOPinWrite(GPIO_PORTM_BASE, 0x1F, 0x1F);
	
	//2. read out data from the receive FIFO
	uint32_t data = 0;
	uint32_t buf = 0;
		
	SSIDataGet(SSI0_BASE, &buf);
	data |= ((buf & 0xff) << 24);
	SSIDataGet(SSI0_BASE, &buf);
	data |= ((buf & 0xff) << 16);
	SSIDataGet(SSI0_BASE, &buf);
	data |= ((buf & 0xff) << 8);
	SSIDataGet(SSI0_BASE, &buf);
	data |= ((buf & 0xff) << 0);
		
	//do some dirty tricks to interpret a 14-bit two's complement number
	uint32_t raw_tc_val = (data & 0xfffc0000) >> 18;
	raw_tc_val = raw_tc_val << 2; //align to 16 bits
	tc_data[current_tc].tc_val = *((int16_t *) &raw_tc_val) >> 2;
	
	//more dirty tricks for a 12-bit number
	uint32_t raw_int_temp = (data & 0x0000fff0) >> 4;
	raw_int_temp = raw_int_temp << 4; //align to 16 bits
	tc_data[current_tc].internal_temp = *((int16_t *) &raw_int_temp) >> 4;
	
	tc_data[current_tc].fault = (data >> 16) & 0x1;
	tc_data[current_tc].short_to_vcc = (data >> 2) & 0x1;
	tc_data[current_tc].short_to_gnd = (data >> 1) & 0x1;
	tc_data[current_tc].open_circuit = (data >> 0) & 0x1;
	
	//OK we're done. clear the "in flight" flag.
	transaction_in_flight = false;

}

