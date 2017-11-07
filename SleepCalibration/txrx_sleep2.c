/**
 * This properly sleeps the entire board
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

#include <ti/drivers/spi/SPICC26XXDMA.h>

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

// Simple PIN_INPUT_EN | PIN_PULL_DOWN does not work
PIN_Config sxSpiSleepPinTable[] = {
     Board_SX_MOSI | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW,
     Board_SX_MISO | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW,
     Board_SX_SCK  | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW,
     PIN_TERMINATE
};

#define READ (0<<7)
#define WRITE (1<<7)

void print_bin(uint32_t val) {
    int i;
    for (i = 31; i >= 0; i--) {
        if ((i % 4) == 0) {
            printf("%d ", (val & (1<<i)) ? 1 : 0);
        } else {
            printf("%d", (val & (1<<i)) ? 1 : 0);
        }
    }
}

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
//        SPI_Handle spiHandle = spi.Spi;
//        // Get pinHandle
//        PIN_Handle spiPinHandle = ((SPICC26XXDMA_Object *)spiHandle->object)->pinHandle;
//        // Get mosi pin id
//        PIN_Id mosiPinId = ((SPICC26XXDMA_HWAttrsV1 *)spiHandle->hwAttrs)->mosiPin;
//        // Get miso pin id
//        PIN_Id misoPinId = ((SPICC26XXDMA_HWAttrsV1 *)spiHandle->hwAttrs)->misoPin;
//        // Get clk pin id
//        PIN_Id clkPinId = ((SPICC26XXDMA_HWAttrsV1 *)spiHandle->hwAttrs)->clkPin;
//        // Apply low power sleep pull config for MISO - do a pull down only on miso
////        if (PIN_setConfig(spiPinHandle, PIN_BM_ALL, misoPinId | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW) != PIN_SUCCESS) {
////            System_abort("Failed to set pin config for miso\n");
////        }
////        if (PIN_setConfig(spiPinHandle, PIN_BM_ALL, mosiPinId | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW) != PIN_SUCCESS) {
////            System_abort("Failed to set pin config for mosi\n");
////        }
////        if (PIN_setConfig(spiPinHandle, PIN_BM_ALL, clkPinId | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW) != PIN_SUCCESS) {
////            System_abort("Failed to set pin config for sck\n");
////        }
//
//        printf("miso prev config: 0x%X: ", (uint32_t)PIN_getConfig(misoPinId)); print_bin((uint32_t)PIN_getConfig(misoPinId)); printf("\n");
//        printf("mosi prev config: 0x%X: ", (uint32_t)PIN_getConfig(mosiPinId)); print_bin((uint32_t)PIN_getConfig(mosiPinId)); printf("\n");
//        printf("clk prev config:  0x%X: ", (uint32_t)PIN_getConfig(clkPinId)); print_bin((uint32_t)PIN_getConfig(clkPinId)); printf("\n");
//
//        printf("\n");
//
//        if (PIN_setConfig(spiPinHandle, PIN_BM_PULLING, misoPinId | PIN_PULLDOWN) != PIN_SUCCESS) {
//            System_abort("Failed to set pin config for miso\n");
//        }
//        if (PIN_setConfig(spiPinHandle, PIN_BM_GPIO_OUTPUT_EN|PIN_BM_OUTPUT_BUF|PIN_BM_DRVSTR, mosiPinId | PIN_GPIO_OUTPUT_EN | PIN_PUSHPULL | PIN_DRVSTR_MAX) != PIN_SUCCESS) {
//            System_abort("Failed to set pin config for mosi\n");
//        }
//        if (PIN_setConfig(spiPinHandle, PIN_BM_GPIO_OUTPUT_EN|PIN_BM_OUTPUT_BUF|PIN_BM_DRVSTR, clkPinId | PIN_GPIO_OUTPUT_EN | PIN_PUSHPULL | PIN_DRVSTR_MAX) != PIN_SUCCESS) {
//            System_abort("Failed to set pin config for sck\n");
//        }
//
//        printf("miso new config:  0x%X: ", (uint32_t)PIN_getConfig(misoPinId)); print_bin((uint32_t)PIN_getConfig(misoPinId)); printf("\n");
//        printf("mosi new config:  0x%X: ", (uint32_t)PIN_getConfig(mosiPinId)); print_bin((uint32_t)PIN_getConfig(mosiPinId)); printf("\n");
//        printf("clk new config:   0x%X: ", (uint32_t)PIN_getConfig(clkPinId)); print_bin((uint32_t)PIN_getConfig(clkPinId)); printf("\n");
//    }

//    {
//        SPI_Handle spiHandle = spi.Spi;
//        // Get pinHandle
//        PIN_Handle spiPinHandle = ((SPICC26XXDMA_Object *)spiHandle->object)->pinHandle;
//        // Get mosi pin id
//        PIN_Id mosiPinId = ((SPICC26XXDMA_HWAttrsV1 *)spiHandle->hwAttrs)->mosiPin;
//        // Get miso pin id
//        PIN_Id misoPinId = ((SPICC26XXDMA_HWAttrsV1 *)spiHandle->hwAttrs)->misoPin;
//        // Get clk pin id
//        PIN_Id clkPinId = ((SPICC26XXDMA_HWAttrsV1 *)spiHandle->hwAttrs)->clkPin;
//        // Apply low power sleep pull config for MISO - do a pull down only on miso
////        if (PIN_setConfig(spiPinHandle, PIN_BM_ALL, misoPinId | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW) != PIN_SUCCESS) {
////            System_abort("Failed to set pin config for miso\n");
////        }
////        if (PIN_setConfig(spiPinHandle, PIN_BM_ALL, mosiPinId | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW) != PIN_SUCCESS) {
////            System_abort("Failed to set pin config for mosi\n");
////        }
////        if (PIN_setConfig(spiPinHandle, PIN_BM_ALL, clkPinId | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW) != PIN_SUCCESS) {
////            System_abort("Failed to set pin config for sck\n");
////        }
//
//        printf("miso prev config: 0x%X: ", (uint32_t)PIN_getConfig(misoPinId)); print_bin((uint32_t)PIN_getConfig(misoPinId)); printf("\n");
//        printf("mosi prev config: 0x%X: ", (uint32_t)PIN_getConfig(mosiPinId)); print_bin((uint32_t)PIN_getConfig(mosiPinId)); printf("\n");
//        printf("clk prev config:  0x%X: ", (uint32_t)PIN_getConfig(clkPinId)); print_bin((uint32_t)PIN_getConfig(clkPinId)); printf("\n");
//
//        printf("\n");
//
//        if (PIN_setConfig(spiPinHandle, PIN_BM_ALL, misoPinId | PIN_GPIO_OUTPUT_EN | PIN_PUSHPULL | PIN_GPIO_LOW) != PIN_SUCCESS) {
//            System_abort("Failed to set pin config for miso\n");
//        }
//        if (PIN_setConfig(spiPinHandle, PIN_BM_ALL, mosiPinId | PIN_GPIO_OUTPUT_EN | PIN_PUSHPULL | PIN_GPIO_LOW ) != PIN_SUCCESS) {
//            System_abort("Failed to set pin config for mosi\n");
//        }
//        if (PIN_setConfig(spiPinHandle, PIN_BM_ALL, clkPinId | PIN_GPIO_OUTPUT_EN | PIN_PUSHPULL | PIN_GPIO_LOW) != PIN_SUCCESS) {
//            System_abort("Failed to set pin config for sck\n");
//        }
//
//        printf("miso new config:  0x%X: ", (uint32_t)PIN_getConfig(misoPinId)); print_bin((uint32_t)PIN_getConfig(misoPinId)); printf("\n");
//        printf("mosi new config:  0x%X: ", (uint32_t)PIN_getConfig(mosiPinId)); print_bin((uint32_t)PIN_getConfig(mosiPinId)); printf("\n");
//        printf("clk new config:   0x%X: ", (uint32_t)PIN_getConfig(clkPinId)); print_bin((uint32_t)PIN_getConfig(clkPinId)); printf("\n");
//    }


//    SpiDeInit(&spi);
//
//    sxSpiSleepPinHandle = PIN_open(&sxSpiSleepPinState, sxSpiSleepPinTable);
//    if (sxSpiSleepPinHandle == NULL)
//    {
//        System_abort("Failed to open board SX SPI Sleep pins\n");
//    }

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
