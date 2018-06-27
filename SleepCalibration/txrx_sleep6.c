/**
 * This test program emits a "Hello World! - ##" message for some number of times, goes to sleep for 3 sec,
 * and then starts again.
 *
 * This program is intended to use all sleep features of the LoRaWan
 * lib HAL layer without doing any trickery in this routine.
 *
 * Use this program as the main tester for sleeping.
 *
 * This program is confirmed to reach about 250nA during the Task_Sleep
 * section on June 26, 2018.
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

/* Radio Events */
#define EVENT_TXDONE           Event_Id_00
#define EVENT_RXDONE           Event_Id_01
#define EVENT_TXTIMEOUT        Event_Id_02
#define EVENT_RXTIMEOUT        Event_Id_03
#define EVENT_RXERROR          Event_Id_04
#define EVENT_CADDONE_DETECT   Event_Id_05
#define EVENT_CADDONE_NODETECT Event_Id_06

static Event_Struct radioEventsStruct;
static Event_Handle radioEvents;

#define USE_BAND_915
#define USE_MODEM_LORA
//#define USE_MODEM_FSK

#if defined( USE_BAND_433 )

#define RF_FREQUENCY                                434000000 // Hz

#elif defined( USE_BAND_780 )

#define RF_FREQUENCY                                780000000 // Hz

#elif defined( USE_BAND_868 )

#define RF_FREQUENCY                                868000000 // Hz

#elif defined( USE_BAND_915 )

//#define RF_FREQUENCY                                915000000 // Hz
#define RF_FREQUENCY                                902000000 // Hz

#else
    #error "Please define a frequency band in the compiler options."
#endif

//#define TX_OUTPUT_POWER                             14        // dBm
#define TX_OUTPUT_POWER                             20        // dBm

#if defined( USE_MODEM_LORA )

#define LORA_BANDWIDTH                              0         // [0: 125 kHz,
                                                              //  1: 250 kHz,
                                                              //  2: 500 kHz,
                                                              //  3: Reserved]
#define LORA_SPREADING_FACTOR                       7         // [SF7..SF12]
//#define LORA_SPREADING_FACTOR                       9         // [SF7..SF12]
#define LORA_CODINGRATE                             1         // [1: 4/5,
                                                              //  2: 4/6,
                                                              //  3: 4/7,
                                                              //  4: 4/8]
#define LORA_PREAMBLE_LENGTH                        8         // Same for Tx and Rx
//#define LORA_PREAMBLE_LENGTH                        20         // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT                         5         // Symbols - Up to 1023 symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON                  false
#define LORA_IQ_INVERSION_ON                        false

#elif defined( USE_MODEM_FSK )

#define FSK_FDEV                                    25e3      // Hz
#define FSK_DATARATE                                50e3      // bps
#define FSK_BANDWIDTH                               50e3      // Hz
#define FSK_AFC_BANDWIDTH                           83.333e3  // Hz
#define FSK_PREAMBLE_LENGTH                         5         // Same for Tx and Rx
#define FSK_FIX_LENGTH_PAYLOAD_ON                   false

#else
    #error "Please define a modem in the compiler options."
#endif

#define RX_TIMEOUT_VALUE                            0 // Radio should be in TX continuous anyways
//#define RX_TIMEOUT_VALUE                            1000
//#define RX_TIMEOUT_VALUE                            5000
#define BUFFER_SIZE                                 64 // Define the payload size here - MAX 265, since hardware FIFO is 256


uint16_t BufferSize = BUFFER_SIZE;
uint8_t Buffer[BUFFER_SIZE];

int8_t RssiValue = 0;
int8_t SnrValue = 0;

/*!
 * Radio events function pointer
 */
static RadioEvents_t RadioEvents;

/*!
 * \brief Function to be executed on Radio Tx Done event
 */
void OnTxDone( void );

/*!
 * \brief Function to be executed on Radio Rx Done event
 */
void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr );

/*!
 * \brief Function executed on Radio Tx Timeout event
 */
void OnTxTimeout( void );

/*!
 * \brief Function executed on Radio Rx Timeout event
 */
void OnRxTimeout( void );

/*!
 * \brief Function executed on Radio Rx Error event
 */
void OnRxError( void );

/*!
 * \brief CAD Done callback prototype.
 *
 * \param [IN] channelDetected    Channel Activity detected during the CAD
 */
static void OnCadDone ( bool channelActivityDetected );


void OnTxDone( void )
{
    printf("OnTxDone\n");
    Radio.Sleep( );
    Event_post(radioEvents, EVENT_TXDONE);
}

void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr )
{
    printf("OnRxDone - RSSI=%d, SNR=%d\n", rssi, snr);
//    uartputs("OnRxDone\n");
//    Radio.Sleep( );
    BufferSize = size;
    memcpy( Buffer, payload, BufferSize );
    RssiValue = rssi;
    SnrValue = snr;
    hexdump(payload, size);
//    uarthexdump(payload, size);
    Event_post(radioEvents, EVENT_RXDONE);
}

void OnTxTimeout( void )
{
    printf("OnTxTimeout\n");
    Radio.Sleep( );
    Event_post(radioEvents, EVENT_TXTIMEOUT);
}

void OnRxTimeout( void )
{
    printf("OnRxTimeout\n");
    Radio.Sleep( );
    Event_post(radioEvents, EVENT_RXTIMEOUT);
}

void OnRxError( void )
{
//    Radio.Sleep( );
    printf("OnRxError\n");
//    uartputs("OnRxError\n");
    Event_post(radioEvents, EVENT_RXERROR);
}

static void OnCadDone( bool channelActivityDetected )
{
    Radio.Sleep( );
    printf("OnCadDone - %s\n", channelActivityDetected ? "Detected" : "NotDetected");
    Event_post(radioEvents, channelActivityDetected ? EVENT_CADDONE_DETECT : EVENT_CADDONE_NODETECT);
}

#define TASKSTACKSIZE   2048

Task_Struct task0Struct;
Char task0Stack[TASKSTACKSIZE];

void maintask(UArg arg0, UArg arg1)
{
    unsigned int pktcount = 0;

    // Target board initialization
    BoardInitMcu();
    BoardInitPeriph();

    Event_construct(&radioEventsStruct, NULL);
    radioEvents = Event_handle(&radioEventsStruct);

    // Radio initialization
    RadioEvents.TxDone    = OnTxDone;
    RadioEvents.RxDone    = OnRxDone;
    RadioEvents.TxTimeout = OnTxTimeout;
    RadioEvents.RxTimeout = OnRxTimeout;
    RadioEvents.RxError   = OnRxError;
    RadioEvents.CadDone   = OnCadDone;

    Radio.Init(&RadioEvents);
    printf("# Radio init\n");

    Radio.SetChannel(RF_FREQUENCY);
    printf("# Set channel to %u\n", RF_FREQUENCY);

#if defined( USE_MODEM_LORA )

    Radio.SetTxConfig(MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                      LORA_SPREADING_FACTOR,
                      LORA_CODINGRATE,
                      LORA_PREAMBLE_LENGTH,
                      LORA_FIX_LENGTH_PAYLOAD_ON,
                      true,
                      0, 0, LORA_IQ_INVERSION_ON, 3000);

    Radio.SetRxConfig(MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                      LORA_CODINGRATE,
                      0, LORA_PREAMBLE_LENGTH,
                      LORA_SYMBOL_TIMEOUT,
                      LORA_FIX_LENGTH_PAYLOAD_ON, 0, true, 0, 0,
                      LORA_IQ_INVERSION_ON, true);
    printf("# Radio set TX and RX config\n");

#elif defined( USE_MODEM_FSK )

    Radio.SetTxConfig( MODEM_FSK, TX_OUTPUT_POWER, FSK_FDEV, 0,
            FSK_DATARATE, 0,
            FSK_PREAMBLE_LENGTH, FSK_FIX_LENGTH_PAYLOAD_ON,
            true, 0, 0, 0, 3000 );

    Radio.SetRxConfig( MODEM_FSK, FSK_BANDWIDTH, FSK_DATARATE,
            0, FSK_AFC_BANDWIDTH, FSK_PREAMBLE_LENGTH,
            0, FSK_FIX_LENGTH_PAYLOAD_ON, 0, true,
            0, 0,false, true );

#else
#error "Please define a frequency band in the compiler options."
#endif


    while(1) {
        pktcount = 0;
        while (pktcount < 1)
        {
            UInt events;
            char buf[64];
            snprintf(buf, sizeof(buf), "Hello World! - %u", pktcount++);
            printf("Sending: \"%s\"\n", buf);
    //        uartprintf("Sending: \"%s\"\n", buf);;
            Radio.Send(buf, strlen(buf));
            // There is no race condition here because events are persistent flags
            events = Event_pend(radioEvents, Event_Id_NONE, EVENT_TXDONE | EVENT_TXTIMEOUT, BIOS_WAIT_FOREVER);
            Task_sleep(TIME_MS * 20);
        }

        printf("Sleeping radio\n");
        Radio.Sleep(); // This is probably not needed, since it put the radio into slee by default

        printf("Sleeping now for 3 sec\n");
        Task_sleep(TIME_MS * 3000);
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
    Board_initUART();
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
