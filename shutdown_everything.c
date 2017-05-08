/**
 * This test program aims to put the LoRaBug in the lowest power state possible.
 *
 * This will probably not work at all without being rewritten for the
 * new LoRaMAC-node library.
 *
 * TODO: Rewrite using new LoRaMAC-node library.
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

/* TI-RTOS Header files */
// #include <ti/drivers/I2C.h>
#include <ti/drivers/PIN.h>
#include <ti/drivers/SPI.h>
// #include <ti/drivers/UART.h>
// #include <ti/drivers/Watchdog.h>

/* Board Header files */
#include "Board.h"

#define TASKSTACKSIZE   512

Task_Struct task0Struct;
Char task0Stack[TASKSTACKSIZE];

/* Pin driver handle */
static PIN_Handle ledPinHandle;
static PIN_State ledPinState;

/*
 * Application LED pin configuration table:
 *   - All LEDs board LEDs are off.
 */
PIN_Config ledPinTable[] = {
    Board_LED0 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
    Board_LED1 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
    PIN_TERMINATE
};


/*
 *  ======== heartBeatFxn ========
 *  Toggle the Board_LED0. The Task_sleep is determined by arg0 which
 *  is configured for the heartBeat Task instance.
 */
Void heartBeatFxn(UArg arg0, UArg arg1)
{
	int i = 2;
	hal_init();
	writeReg(0x01, 0x00);
	Task_sleep((UInt)arg0);
	writeReg(0x01, 0x00);

    while (1) {
        Task_sleep((UInt)arg0);
        uint_t state = PIN_getOutputValue(Board_LED0);
        PIN_setOutputValue(ledPinHandle, Board_LED0, !state);
        hal_pin_rxtx(state?1:0);

        if (!state && (i-- == 0)) {
        	PIN_setOutputValue(ledPinHandle, Board_LED0, 0);
        	PIN_setOutputValue(ledPinHandle, Board_LED1, 0);
        	hal_sleep();
        	Power_shutdown(NULL, 0);
        }
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
    // Board_initUART();
    // Board_initWatchdog();

    /* Construct heartBeat Task  thread */
    Task_Params_init(&taskParams);
    taskParams.arg0 = 1000000 / Clock_tickPeriod;
    taskParams.stackSize = TASKSTACKSIZE;
    taskParams.stack = &task0Stack;
    Task_construct(&task0Struct, (Task_FuncPtr)heartBeatFxn, &taskParams, NULL);

    /* Open LED pins */
    ledPinHandle = PIN_open(&ledPinState, ledPinTable);
    if(!ledPinHandle) {
        System_abort("Error initializing board LED pins\n");
    }

//    PIN_setOutputValue(ledPinHandle, Board_LED1, 1);

    /* Start BIOS */
    BIOS_start();

    return (0);
}
