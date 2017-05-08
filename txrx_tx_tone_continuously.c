/**
 * This test program emits a continuous tone at the specified frequency.
 *
 * @author Craig Hesling <craig@hesling.com>
 */

#include <stdio.h>

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
//#include <ti/drivers/UART.h>
//#include <ti/drivers/Watchdog.h>

/* Board Header files */
#include "Board.h"

//#include "hal.h"

#define TIME_MS (1000/Clock_tickPeriod)

#include "LoRaRadio/board.h"
#include "radio.h"
#include "io.h"

#define RF_FREQUENCY                                902000000 // Hz
#define TX_OUTPUT_POWER                             20        // 20 dBm
//#define TX_TIMEOUT                                  65535     // seconds (MAX value)
#define TX_TIMEOUT                                  2     // seconds (MAX value)


#define TASKSTACKSIZE   2048

static Task_Struct task0Struct;
static Char task0Stack[TASKSTACKSIZE];

/*!
 * Radio events function pointer
 */
static RadioEvents_t RadioEvents;

/*!
 * \brief Function executed on Radio Tx Timeout event
 */
void OnRadioTxTimeout( void )
{
    printf("Restarting TxContinuousWave\n");
    // Restarts continuous wave transmission when timeout expires
    Radio.SetTxContinuousWave( RF_FREQUENCY, TX_OUTPUT_POWER, TX_TIMEOUT );
}

/*
 *  ======== heartBeatFxn ========
 *  Toggle the Board_LED0. The Task_sleep is determined by arg0 which
 *  is configured for the heartBeat Task instance.
 */
void heartBeatFxn(UArg arg0, UArg arg1)
{
    // Target board initialization
    BoardInitMcu( );
    BoardInitPeriph( );

    // Radio initialization
    RadioEvents.TxTimeout = OnRadioTxTimeout;
    Radio.Init(&RadioEvents);

    Task_sleep(1000 * TIME_MS);
    toggleLed(Board_GLED);
    Radio.SetTxContinuousWave( RF_FREQUENCY, TX_OUTPUT_POWER, TX_TIMEOUT);
    printstate();

    while(1) {
        Task_sleep(1000 * TIME_MS);
        printstate();

        toggleLed(Board_GLED);
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
//    Board_initI2C();
    Board_initSPI();
//    Board_initUART();
//    Board_initWatchdog();
    printf("Hello World!\n");

    /* Construct heartBeat Task  thread */
    Task_Params_init(&taskParams);
    taskParams.arg0 = 1000000 / Clock_tickPeriod;
    taskParams.stackSize = TASKSTACKSIZE;
    taskParams.stack = &task0Stack;
    Task_construct(&task0Struct, (Task_FuncPtr) heartBeatFxn, &taskParams,
                   NULL);

    /* Open and setup pins */
    setuppins();

    /* Open UART */
    setupuart();

    /* Start BIOS */
    BIOS_start();

    return (0);
}
