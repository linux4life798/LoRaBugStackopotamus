/**
 * This test program emits a "Hello World! - ##" message continuously.
 *
 * @author Craig Hesling <craig@hesling.com>
 */

/* XDCtools Header files */
#include <xdc/std.h>
#include <xdc/runtime/System.h>

/* BIOS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Event.h>

/* TI-RTOS Header files */
// #include <ti/drivers/I2C.h>
#include <ti/drivers/PIN.h>
#include <ti/drivers/SPI.h>
#include <ti/drivers/UART.h>
// #include <ti/drivers/Watchdog.h>

/* Board Header files */
#include "Board.h"

#define TIME_MS (1000/Clock_tickPeriod)

#include <string.h> // strlen in uartputs
#include "board.h" // The LoRaMac-node/src/boads/LoRaBug/board.h file
#include "radio.h"
#include "io.h"

#include <spi.h>

#define TASKSTACKSIZE   2048

Task_Struct task0Struct;
Char task0Stack[TASKSTACKSIZE];

static PIN_Handle sxPinHandle;
static PIN_State sxPinState;

static PIN_Handle sxSpiSleepPinHandle;
static PIN_State sxSpiSleepPinState;

PIN_Config sxPinTable[] = {
     Board_SX_RESET | PIN_GPIO_OUTPUT_DIS | PIN_GPIO_LOW,
     Board_SX_NSS   | PIN_GPIO_OUTPUT_DIS | PIN_GPIO_HIGH,
     Board_SX_DIO0  | PIN_INPUT_DIS | PIN_NOPULL,
     Board_SX_DIO1  | PIN_INPUT_DIS | PIN_NOPULL,
     Board_SX_DIO2  | PIN_INPUT_DIS | PIN_NOPULL,
     Board_SX_DIO3  | PIN_INPUT_DIS | PIN_NOPULL,
     Board_SX_DIO4  | PIN_INPUT_DIS | PIN_NOPULL,
     Board_SX_DIO5  | PIN_INPUT_DIS | PIN_NOPULL,
     Board_SX_RF_CTRL1 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW,
     Board_SX_RF_CTRL2 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW,
     PIN_TERMINATE
};

PIN_Config sxSpiSleepPinTable[] = {
     Board_SX_MOSI | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW,
     Board_SX_MISO | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW,
     Board_SX_SCK  | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW,
     PIN_TERMINATE
};

#define READ (0<<7)
#define WRITE (1<<7)

void maintask(UArg arg0, UArg arg1)
{
    Spi_t spi;

    sxPinHandle = PIN_open(&sxPinState, sxPinTable);
    if (sxPinHandle == NULL)
    {
        System_abort("Failed to open board SX pins\n");
    }

    SpiInit(&spi, NC, NC, NC, NC);

    // Wait for radio to stablize
    Task_sleep(TIME_MS * 100);

    PIN_setOutputEnable(sxPinHandle, Board_SX_NSS, 1);

    // Get OP Mode
    {
        PIN_setOutputValue(sxPinHandle, Board_SX_NSS, 0);
        SpiInOut(&spi, READ|0x01);
        uint16_t mode = SpiInOut(&spi, 0);
        PIN_setOutputValue(sxPinHandle, Board_SX_NSS, 1);
        printf("Mode = 0x%X\n", mode);
    }

    // Read silicon version
    {
        PIN_setOutputValue(sxPinHandle, Board_SX_NSS, 0);
        SpiInOut(&spi, READ|0x42);
        uint16_t ver = SpiInOut(&spi, 0);
        PIN_setOutputValue(sxPinHandle, Board_SX_NSS, 1);
        printf("Version = 0x%X\n", ver); // Should be 0x12 for V1b(production)
    }

    // Set HF and Sleep Mode
    {
        PIN_setOutputValue(sxPinHandle, Board_SX_NSS, 0);
        SpiInOut(&spi, WRITE|0x01);
        uint16_t mode = SpiInOut(&spi, 0);
        PIN_setOutputValue(sxPinHandle, Board_SX_NSS, 1);
        printf("Prev Mode = 0x%X\n", mode);
    }


//    int err = Power_shutdown(NULL, 0);
//    if (err == Power_ECHANGE_NOT_ALLOWED) {
//            setLed(Board_GLED, 1);
//            while(1) {
//                Task_sleep(TIME_MS * 5000);
//            }
//    }
//    if (err == Power_EBUSY) {
//        setLed(Board_RLED, 1);
//        while(1) {
//            Task_sleep(TIME_MS * 5000);
//        }
//    }

//    setLed(Board_GLED, 1);
//    setLed(Board_RLED, 1);

    SpiDeInit(&spi);

    sxSpiSleepPinHandle = PIN_open(&sxSpiSleepPinState, sxSpiSleepPinTable);
    if (sxSpiSleepPinHandle == NULL)
    {
        System_abort("Failed to open board SX SPI Sleep pins\n");
    }

    while(1) {
        Task_sleep(TIME_MS * 10000);
    }

}


/*
 *  ======== main ========
 */
int main(void)
{
    Task_Params taskParams;

    /* Call board init functions */
    Board_initGeneral();
    // Board_initI2C();
    Board_initSPI();
//    Board_initUART();
    // Board_initWatchdog();

    /* Construct heartBeat Task  thread */
    Task_Params_init(&taskParams);
    taskParams.arg0 = 1000000 / Clock_tickPeriod;
    taskParams.stackSize = TASKSTACKSIZE;
    taskParams.stack = &task0Stack;
    Task_construct(&task0Struct, (Task_FuncPtr) maintask, &taskParams,
                   NULL);

    /* Open and setup pins */
    setuppins();

    /* Open UART */
//    setupuart();

    /* Start BIOS */
    BIOS_start();

    return (0);
}
