/*
 * SD_test.c
 *
 *  Created on: Oct 26, 2016
 *      Author: kuatt
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "inc/hw_memmap.h"
#include "driverlib/fpu.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/uart.h"
#include "grlib/grlib.h"
#include "utils/cmdline.h"
#include "utils/uartstdio.h"
#include "fatfs/src/ff.h"
#include "fatfs/src/diskio.h"


uint32_t g_ui32SysClock;

//*****************************************************************************
//
// Defines the size of the buffers that hold the path, or temporary data from
// the SD card.  There are two buffers allocated of this size.  The buffer size
// must be large enough to hold the longest expected full path name, including
// the file name, and a trailing null character.
//
//*****************************************************************************
#define PATH_BUF_SIZE           80

//*****************************************************************************
//
// This buffer holds the full path to the current working directory.  Initially
// it is root ("/").
//
//*****************************************************************************
static char g_pcCwdBuf[PATH_BUF_SIZE] = "/";

//*****************************************************************************
//
// A temporary data buffer used when manipulating file paths, or reading data
// from the SD card.
//
//*****************************************************************************
static char g_pcTmpBuf[PATH_BUF_SIZE];

//*****************************************************************************
//
// The following are data structures used by FatFs.
//
//*****************************************************************************
static FATFS g_sFatFs;
static DIR g_sDirObject;
//static FILINFO g_sFileInfo;
static FIL g_sFileObject;

//*****************************************************************************
//
// A structure that holds a mapping between an FRESULT numerical code, and a
// string representation.  FRESULT codes are returned from the FatFs FAT file
// system driver.
//
//*****************************************************************************
typedef struct
{
    FRESULT iFResult;
    char *pcResultStr;
}
tFResultString;

//*****************************************************************************
//
// A macro to make it easy to add result codes to the table.
//
//*****************************************************************************
#define FRESULT_ENTRY(f)        { (f), (#f) }

//*****************************************************************************
//
// A macro that holds the number of result codes.
//
//*****************************************************************************
#define NUM_FRESULT_CODES       (sizeof(g_psFResultStrings) /                 \
                                 sizeof(tFResultString))

//*****************************************************************************
//
// This is the handler for this SysTick interrupt.  FatFs requires a timer tick
// every 10 ms for internal timing purposes.
//
//*****************************************************************************
void
SysTickHandler(void)
{
    //
    // Call the FatFs tick timer.
    //
    disk_timerproc();
}

//*****************************************************************************
//
// This function implements the "cd" command.  It takes an argument that
// specifies the directory to make the current working directory.  Path
// separators must use a forward slash "/".  The argument to cd can be one of
// the following:
//
// * root ("/")
// * a fully specified path ("/my/path/to/mydir")
// * a single directory name that is in the current directory ("mydir")
// * parent directory ("..")
//
// It does not understand relative paths, so dont try something like this:
// ("../my/new/path")
//
// Once the new directory is specified, it attempts to open the directory to
// make sure it exists.  If the new path is opened successfully, then the
// current working directory (cwd) is changed to the new path.
//
//*****************************************************************************
int
Cmd_cd(int argc, char *argv[])
{
    uint_fast8_t ui8Idx;
    FRESULT iFResult;

    //
    // Copy the current working path into a temporary buffer so it can be
    // manipulated.
    //
    strcpy(g_pcTmpBuf, g_pcCwdBuf);

    //
    // If the first character is /, then this is a fully specified path, and it
    // should just be used as-is.
    //
    if(argv[1][0] == '/')
    {
        //
        // Make sure the new path is not bigger than the cwd buffer.
        //
        if(strlen(argv[1]) + 1 > sizeof(g_pcCwdBuf))
        {
         //   UARTprintf("Resulting path name is too long\n");
            return(0);
        }

        //
        // If the new path name (in argv[1])  is not too long, then copy it
        // into the temporary buffer so it can be checked.
        //
        else
        {
            strncpy(g_pcTmpBuf, argv[1], sizeof(g_pcTmpBuf));
        }
    }

    //
    // If the argument is .. then attempt to remove the lowest level on the
    // CWD.
    //
    else if(!strcmp(argv[1], ".."))
    {
        //
        // Get the index to the last character in the current path.
        //
        ui8Idx = strlen(g_pcTmpBuf) - 1;

        //
        // Back up from the end of the path name until a separator (/) is
        // found, or until we bump up to the start of the path.
        //
        while((g_pcTmpBuf[ui8Idx] != '/') && (ui8Idx > 1))
        {
            //
            // Back up one character.
            //
            ui8Idx--;
        }

        //
        // Now we are either at the lowest level separator in the current path,
        // or at the beginning of the string (root).  So set the new end of
        // string here, effectively removing that last part of the path.
        //
        g_pcTmpBuf[ui8Idx] = 0;
    }

    //
    // Otherwise this is just a normal path name from the current directory,
    // and it needs to be appended to the current path.
    //
    else
    {
        //
        // Test to make sure that when the new additional path is added on to
        // the current path, there is room in the buffer for the full new path.
        // It needs to include a new separator, and a trailing null character.
        //
        if(strlen(g_pcTmpBuf) + strlen(argv[1]) + 1 + 1 > sizeof(g_pcCwdBuf))
        {
        //    UARTprintf("Resulting path name is too long\n");
            return(0);
        }

        //
        // The new path is okay, so add the separator and then append the new
        // directory to the path.
        //
        else
        {
            //
            // If not already at the root level, then append a /
            //
            if(strcmp(g_pcTmpBuf, "/"))
            {
                strcat(g_pcTmpBuf, "/");
            }

            //
            // Append the new directory to the path.
            //
            strcat(g_pcTmpBuf, argv[1]);
        }
    }

    //
    // At this point, a candidate new directory path is in chTmpBuf.  Try to
    // open it to make sure it is valid.
    //
    iFResult = f_opendir(&g_sDirObject, g_pcTmpBuf);

    //
    // If it can't be opened, then it is a bad path.  Inform the user and
    // return.
    //
    if(iFResult != FR_OK)
    {
     //   UARTprintf("cd: %s\n", g_pcTmpBuf);
        return((int)iFResult);
    }

    //
    // Otherwise, it is a valid new path, so copy it into the CWD.
    //
    else
    {
        strncpy(g_pcCwdBuf, g_pcTmpBuf, sizeof(g_pcCwdBuf));
    }

    //
    // Return success.
    //
    return(0);
}

//*****************************************************************************
//
// This function implements the "cat" command.  It reads the contents of a file
// and prints it to the console.  This should only be used on text files.  If
// it is used on a binary file, then a bunch of garbage is likely to printed on
// the console.
//
//*****************************************************************************
int
Cmd_cat(int argc, char *argv[])
{
    FRESULT iFResult;
    uint32_t ui32BytesRead;

    //
    // First, check to make sure that the current path (CWD), plus the file
    // name, plus a separator and trailing null, will all fit in the temporary
    // buffer that will be used to hold the file name.  The file name must be
    // fully specified, with path, to FatFs.
    //
    if(strlen(g_pcCwdBuf) + strlen(argv[1]) + 1 + 1 > sizeof(g_pcTmpBuf))
    {
    //    UARTprintf("Resulting path name is too long\n");
        return(0);
    }

    //
    // Copy the current path to the temporary buffer so it can be manipulated.
    //
    strcpy(g_pcTmpBuf, g_pcCwdBuf);

    //
    // If not already at the root level, then append a separator.
    //
    if(strcmp("/", g_pcCwdBuf))
    {
        strcat(g_pcTmpBuf, "/");
    }

    //
    // Now finally, append the file name to result in a fully specified file.
    //
    strcat(g_pcTmpBuf, argv[1]);

    //
    // Open the file for reading.
    //
    iFResult = f_open(&g_sFileObject, g_pcTmpBuf, FA_READ);

    //
    // If there was some problem opening the file, then return an error.
    //
    if(iFResult != FR_OK)
    {
        return((int)iFResult);
    }

    //
    // Enter a loop to repeatedly read data from the file and display it, until
    // the end of the file is reached.
    //
    do
    {
        //
        // Read a block of data from the file.  Read as much as can fit in the
        // temporary buffer, including a space for the trailing null.
        //
        iFResult = f_read(&g_sFileObject, g_pcTmpBuf, sizeof(g_pcTmpBuf) - 1,
                          (UINT *)&ui32BytesRead);

        //
        // If there was an error reading, then print a newline and return the
        // error to the user.
        //
        if(iFResult != FR_OK)
        {
        //    UARTprintf("\n");
            return((int)iFResult);
        }

        //
        // Null terminate the last block that was read to make it a null
        // terminated string that can be used with printf.
        //
        g_pcTmpBuf[ui32BytesRead] = 0;

        //
        // Print the last chunk of the file that was received.
        //
     //   UARTprintf("%s", g_pcTmpBuf);
    }
    while(ui32BytesRead == sizeof(g_pcTmpBuf) - 1);

    //
    // Return success.
    //
    return(0);
}

//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, uint32_t ui32Line)
{
}
#endif

//*****************************************************************************
//
// The program main function.  It performs initialization, then runs a command
// processing loop to read commands from the console.
//
//*****************************************************************************
int
main(void)
{
  //  int nStatus;
    FRESULT iFResult;
  //  tRectangle sRect;

    //
    // Enable the GPIO port that is used for the on-board LED.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);

    //
    // Check if the peripheral access is enabled.
    //
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPION))
    {
    }

    //
    // Enable the GPIO pin for the LED (PN0).  Set the direction as output, and
    // enable the GPIO pin for digital function.
    //
    GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_0);

    volatile uint32_t ui32Loop;
    int counter = 2;


    //
    // Enable lazy stacking for interrupt handlers.  This allows floating-point
    // instructions to be used within interrupt handlers, but at the expense of
    // extra stack usage.
    //
    MAP_FPULazyStackingEnable();

    //
    // Set the system clock to run at 50MHz from the PLL.
    //
    g_ui32SysClock = SysCtlClockFreqSet (SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                           SYSCTL_XTAL_16MHZ,50000000);

    //
    // Enable the peripherals used by this example.
    //
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI3);

    //
    // Configure SysTick for a 100Hz interrupt.  The FatFs driver wants a 10 ms
    // tick.
    //
    MAP_SysTickPeriodSet(g_ui32SysClock / 100);
    MAP_SysTickEnable();
    MAP_SysTickIntEnable();

    //
    // Enable Interrupts
    //
    MAP_IntMasterEnable();
    // Mount the file system, using logical disk 0.
    //

    iFResult = f_mount(0, &g_sFatFs);



    iFResult = f_open(&g_sFileObject, "testFile.txt", (FA_OPEN_ALWAYS | FA_WRITE));


    while(counter > 0){
    		GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, GPIO_PIN_0);
    		for(ui32Loop = 0; ui32Loop < 200000; ui32Loop++)
    		{
    		}

    		//
    		// Turn off the LED.
    		//
    		GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, 0x0);

    		//
    		// Delay for a bit.
    		//
    		for(ui32Loop = 0; ui32Loop < 200000; ui32Loop++)
    		{
    		}
    		counter--;
    }

    if(iFResult != FR_OK)
    {
       return(1);
    }
}








