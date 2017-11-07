/**
 * This properly sleeps the entire board using more Semtech lib IO functions.
 * This depend on the SPI pins being properly configured for sleep mode elsewhere
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

#include <gpio-board.h>
#include <spi.h>

#include <ti/drivers/spi/SPICC26XXDMA.h>
#include <ti/drivers/Power.h>

#define TASKSTACKSIZE   2048

Task_Struct task0Struct;
Char task0Stack[TASKSTACKSIZE];

static PIN_Handle sxPinHandle;
static PIN_State sxPinState;

PIN_Config sxPinTable[] = {
     Board_SX_RESET | PIN_GPIO_OUTPUT_DIS | PIN_GPIO_LOW,
     PIN_TERMINATE
};


//static Power_NotifyObj pwrNotifObj;

#define READ (0<<7)
#define WRITE (1<<7)

///*
// *  ======== spiMosiCorrect ========
// *  This functions is called to notify the us of an imminent transition
// *  in to sleep mode.
// *
// *  @pre    Function assumes that the SPI handle (clientArg) is pointing to a
// *          hardware module which has already been opened.
// */
//static int spiMosiCorrect(unsigned int eventType, uintptr_t eventArg, uintptr_t clientArg);
//{
//    SPICC26XXDMA_Object *object;
//    object = ((SPI_Handle) clientArg)->object;
//
//    /* In slave mode, optionally enable wakeup on CSN assert */
//    if (object->wakeupCallbackFxn) {
//        PIN_setInterrupt(object->pinHandle, object->csnPin | PIN_IRQ_NEGEDGE);
//    }
//    return Power_NOTIFYDONE;
//}

void maintask(UArg arg0, UArg arg1)
{
    sxPinHandle = PIN_open(&sxPinState, sxPinTable);
    if (sxPinHandle == NULL)
    {
        System_abort("Failed to open board SX pins\n");
    }

    BoardInitMcu();
    SX1276SetAntSwLowPower(false);

//    Power_registerNotify(&pwrNotifObj, PowerCC26XX_ENTERING_STANDBY|PowerCC26XX_AWAKE_STANDBY, (Fxn)spiMosiCorrect, (UInt32)NULL);

    // Wait for radio to stabilize
    Task_sleep(TIME_MS * 100);

    GpioMcuWrite(&SX1276.Spi.Nss, 1);

    // Get OP Mode
    {
        GpioMcuWrite(&SX1276.Spi.Nss, 0);
        SpiInOut(&SX1276.Spi, READ|0x01);
        uint16_t mode = SpiInOut(&SX1276.Spi, 0);
        GpioMcuWrite(&SX1276.Spi.Nss, 1);
        printf("Mode = 0x%X\n", mode);
    }

    // Read silicon version
    {
        GpioMcuWrite(&SX1276.Spi.Nss, 0);
        SpiInOut(&SX1276.Spi, READ|0x42);
        uint16_t ver = SpiInOut(&SX1276.Spi, 0);
        GpioMcuWrite(&SX1276.Spi.Nss, 1);
        printf("Version = 0x%X\n", ver); // Should be 0x12 for V1b(production)
    }

    // Set HF and Sleep Mode
    {
        GpioMcuWrite(&SX1276.Spi.Nss, 0);
        SpiInOut(&SX1276.Spi, WRITE|0x01);
        uint16_t mode = SpiInOut(&SX1276.Spi, 0);
        GpioMcuWrite(&SX1276.Spi.Nss, 1);
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

    /* Check out SPICC26XXDMA.h for an example on how to configure pull ups and pull downs on MISO and MOSI
     * OR
     * file:///home/craig/ti/tirtos_cc13xx_cc26xx_2_21_00_06/products/tidrivers_cc13xx_cc26xx_2_21_00_04/docs/doxygen/html/_s_p_i_c_c26_x_x_d_m_a_8h.html
     *
     *  // Open the SPI and perform the transfer
     *  handle = SPI_open(Board_SPI, &params);
     *  // Get pinHandle
     *  pinHandle = ((SPICC26XXDMA_Object *)spiHandle->object)->pinHandle;
     *  // Get miso pin id
     *  misoPinId = ((SPICC26XXDMA_HWAttrsV1 *)spiHandle->hwAttrs)->misoPin;
     *  // Apply low power sleep pull config for MISO
     *  PIN_setConfig(pinHandle, PIN_BM_PULLING, PIN_PULLUP | misoPinId);
     */

//    {
//        SPI_Handle spiHandle = SX1276.Spi;
//        // Get pinHandle
//        PIN_Handle spiPinHandle = ((SPICC26XXDMA_Object *)spiHandle->object)->pinHandle;
//        // Get mosi pin id
//        PIN_Id mosiPinId = ((SPICC26XXDMA_HWAttrsV1 *)spiHandle->hwAttrs)->mosiPin;
//        // Get miso pin id
//        PIN_Id misoPinId = ((SPICC26XXDMA_HWAttrsV1 *)spiHandle->hwAttrs)->misoPin;
//        // Get clk pin id
//        PIN_Id clkPinId = ((SPICC26XXDMA_HWAttrsV1 *)spiHandle->hwAttrs)->clkPin;
//        // Apply low power sleep pull config for MISO - do a pull down only on miso
//        if (PIN_setConfig(spiPinHandle, PIN_BM_ALL, misoPinId | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW) != PIN_SUCCESS) {
//            System_abort("Failed to set pin config for miso\n");
//        }
//        if (PIN_setConfig(spiPinHandle, PIN_BM_ALL, mosiPinId | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW) != PIN_SUCCESS) {
//            System_abort("Failed to set pin config for mosi\n");
//        }
//        if (PIN_setConfig(spiPinHandle, PIN_BM_ALL, clkPinId | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW) != PIN_SUCCESS) {
//            System_abort("Failed to set pin config for sck\n");
//        }
//    }


    SX1276SetAntSwLowPower(true);
//    BoardDeInitMcu();

    printf("Sleeping now\n");

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
