#include <stdint.h>
#include <stdbool.h>

#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/ssi.h"
#include "driverlib/pin_map.h"
#include "inc/hw_memmap.h"

#include "thermocouple.h"
#include "debug.h"

tc_data_t tc_data;

void thermocouple_init() {
	//enable the port for the SPI pins
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
	while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA)) {}
		
	//enable SSI0 for SPI comm
	SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI0);
	while(!SysCtlPeripheralReady(SYSCTL_PERIPH_SSI0)) {}
		
	//configure PA7 as a digital out for CS line
	GPIOPinTypeGPIOOutput(GPIO_PORTA_BASE, GPIO_PIN_7);
		
	//configure PA2 (SCLK) and PA5 (MISO) pins for comm
	GPIOPinConfigure(GPIO_PA2_SSI0CLK);
	GPIOPinConfigure(GPIO_PA5_SSI0XDAT1);
	GPIOPinTypeSSI(GPIO_PORTA_BASE, GPIO_PIN_2 | GPIO_PIN_5);
		
	//configure the SPI port (SSI0). Sample on rising edge
	SSIClockSourceSet(SSI0_BASE, SSI_CLOCK_SYSTEM);
	SSIConfigSetExpClk(SSI0_BASE, 120000000, SSI_FRF_MOTO_MODE_1, SSI_MODE_MASTER, 5000000, 8); // SPI, SCLK idle low, sample on falling edge, 5MHz, 8 bits per frame
	SSIEnable(SSI0_BASE);	
		
	debug_print("thermocouple_init() complete\r\n");
}

void thermocouple_periodic() {
	//1. set CS low
	GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_7, 0);
	//2. wait 100 ns
	
	for(volatile int i=0; i < 8; i++) {}
	
	//3. transfer 32 bits
	SSIDataPut(SSI0_BASE, 0x00);
	SSIDataPut(SSI0_BASE, 0x00);
	SSIDataPut(SSI0_BASE, 0x00);
	SSIDataPut(SSI0_BASE, 0x00);
	
	//Wait for transaction to complete
	while(SSIBusy(SSI0_BASE)){}
		
	// set CS high again
	GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_7, GPIO_PIN_7);
	
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
	tc_data.tc_val = *((int16_t *) &raw_tc_val) >> 2;
	
	//more dirty tricks for a 12-bit number
	uint32_t raw_int_temp = (data & 0x0000fff0) >> 4;
	raw_int_temp = raw_int_temp << 4; //align to 16 bits
	tc_data.internal_temp = *((int16_t *) &raw_int_temp) >> 4;
	
	tc_data.fault = (data >> 16) & 0x1;
	tc_data.short_to_vcc = (data >> 2) & 0x1;
	tc_data.short_to_gnd = (data >> 1) & 0x1;
	tc_data.open_circuit = (data >> 0) & 0x1;
	

		debug_print("TC VAL: ");
		debug_print_u32(tc_data.tc_val);
		debug_print("\r\nINT TEMP: ");
		debug_print_u32(tc_data.internal_temp);
		debug_print("\r\nFAULT: ");
		debug_print_u32(tc_data.fault);
		debug_print("\r\nSHORT_VCC: ");
		debug_print_u32(tc_data.short_to_vcc);
		debug_print("\r\nSHORT_GND: ");
		debug_print_u32(tc_data.short_to_gnd);
		debug_print("\r\nOPEN_CIRCUIT: ");
		debug_print_u32(tc_data.open_circuit);
		debug_print("\r\n\r\n");

}
