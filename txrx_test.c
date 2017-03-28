/*
 * Copyright (c) 2015-2016, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  ======== empty_min.c ========
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
//#include <ti/drivers/UART.h>
//#include <ti/drivers/Watchdog.h>

//#include "uart_printf.h"
// UART LOGS???
//#include <uart_logs.h>

/* Board Header files */
#include "Board.h"

//#include "hal.h"

#define TIME_MS (1000/Clock_tickPeriod)

//#include <string.h>
#include "LoRaRadio/board.h"
#include "radio.h"

#define RF_FREQUENCY                                902000000 // Hz
#define TX_OUTPUT_POWER                             20        // 20 dBm
#define TX_TIMEOUT                                  65535     // seconds (MAX value)



#define TASKSTACKSIZE   2048

static Task_Struct task0Struct;
static Char task0Stack[TASKSTACKSIZE];

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

/*!
 * Radio events function pointer
 */
static RadioEvents_t RadioEvents;

/*!
 * \brief Function executed on Radio Tx Timeout event
 */
void OnRadioTxTimeout( void )
{
    // Restarts continuous wave transmission when timeout expires
    Radio.SetTxContinuousWave( RF_FREQUENCY, TX_OUTPUT_POWER, TX_TIMEOUT );
    printf("Restarted TxContinuousWave\n");
}

void setuppins() {
    ledPinHandle = PIN_open(&ledPinState, ledPinTable);
    if (ledPinHandle == NULL) {
        System_abort("Failed to open board LED pins\n");
    }
}

void printstate()
{
    PIN_Status pstatus;
    RadioState_t state = Radio.GetStatus();
    switch (state)
    {
    case RF_IDLE:
        pstatus = PIN_setOutputValue(ledPinHandle, Board_RLED, 0);
        printf("RF_IDLE\n");
        break;
    case RF_RX_RUNNING:
        pstatus = PIN_setOutputValue(ledPinHandle, Board_RLED, 1);
        printf("RF_RX_RUNNING\n");
        break;
    case RF_TX_RUNNING:
        pstatus = PIN_setOutputValue(ledPinHandle, Board_RLED, 1);
        printf("RF_TX_RUNNING\n");
        break;
    case RF_CAD:
        pstatus = PIN_setOutputValue(ledPinHandle, Board_RLED, 1);
        printf("RF_CAD\n");
        break;
    }
    if (pstatus != PIN_SUCCESS) {
        System_abort("Failed to set Red LED value\n");
    }
}

/*
 *  ======== heartBeatFxn ========
 *  Toggle the Board_LED0. The Task_sleep is determined by arg0 which
 *  is configured for the heartBeat Task instance.
 */
void heartBeatFxn(UArg arg0, UArg arg1)
{
    PIN_Status pstatus;
//    asm("  .word 0x4567f123");

    // Target board initialization
    BoardInitMcu( );
    BoardInitPeriph( );

    // Radio initialization
    RadioEvents.TxTimeout = OnRadioTxTimeout;
    Radio.Init(&RadioEvents);

    Task_sleep(1000 * TIME_MS);
    Task_sleep(1000 * TIME_MS);
    pstatus = PIN_setOutputValue(ledPinHandle, Board_GLED,
                                 !PIN_getOutputValue(Board_GLED));
    if (pstatus != PIN_SUCCESS)
    {
        System_abort("Failed to set Red LED value\n");
    }
    Radio.SetTxContinuousWave( RF_FREQUENCY, TX_OUTPUT_POWER, TX_TIMEOUT);
    printstate();

    while(1) {
//        Radio.SetTxContinuousWave( RF_FREQUENCY, TX_OUTPUT_POWER, TX_TIMEOUT );
//        printstate();
        Task_sleep(1000 * TIME_MS);
        printstate();

        pstatus = PIN_setOutputValue(ledPinHandle, Board_GLED, !PIN_getOutputValue(Board_GLED));
        if (pstatus != PIN_SUCCESS) {
            System_abort("Failed to set Red LED value\n");
        }
        //Clock_tickPeriod
    }

}

//void eventProcessorFxn(UArg arg0, UArg arg1)
//{
//
//    Mailbox_pend(WAIT_FOREVER,)
//}

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
    System_printf("Hello World!\n");

//    UART_Params uartParams;
//    UART_Params_init(&uartParams);
////  uartParams.baudRate = 3000000; // WARNING
//    uartParams.baudRate = 115200; // WARNING
//    UART_Handle hUart = UART_open(Board_UART, &uartParams);
//    //Initialize the logger output
////    UartLog_init(hUart);
//    UartPrintf_init()

    /* Construct heartBeat Task  thread */
    Task_Params_init(&taskParams);
    taskParams.arg0 = 1000000 / Clock_tickPeriod;
    taskParams.stackSize = TASKSTACKSIZE;
    taskParams.stack = &task0Stack;
    Task_construct(&task0Struct, (Task_FuncPtr) heartBeatFxn, &taskParams,
                   NULL);

    /* Open LED pins */
    setuppins();

//    PIN_setOutputValue(ledPinHandle, Board_LED1, 1);

    /* Start BIOS */
    BIOS_start();

    return (0);
}
