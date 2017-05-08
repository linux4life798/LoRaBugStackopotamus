/**
 * This test program implements the LoRaMAC-node example
 * PING PONG test program.
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
#include <ti/drivers/UART.h>
// #include <ti/drivers/Watchdog.h>

/* Board Header files */
#include "Board.h"

#include <string.h> // strlen in uartputs
#include "board.h" // The LoRaMac-node/src/boards/LoRaBug/board.h file
#include "radio.h"
#include "io.h"

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
//#define LORA_SPREADING_FACTOR                       7         // [SF7..SF12]
#define LORA_SPREADING_FACTOR                       9         // [SF7..SF12]
#define LORA_CODINGRATE                             1         // [1: 4/5,
                                                              //  2: 4/6,
                                                              //  3: 4/7,
                                                              //  4: 4/8]
#define LORA_PREAMBLE_LENGTH                        8         // Same for Tx and Rx
//#define LORA_PREAMBLE_LENGTH                        20         // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT                         5         // Symbols
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

typedef enum
{
    LOWPOWER,
    RX,
    RX_TIMEOUT,
    RX_ERROR,
    TX,
    TX_TIMEOUT,
}States_t;

#define RX_TIMEOUT_VALUE                            1000
//#define RX_TIMEOUT_VALUE                            5000
#define BUFFER_SIZE                                 64 // Define the payload size here

const uint8_t PingMsg[] = "PING";
const uint8_t PongMsg[] = "PONG";

uint16_t BufferSize = BUFFER_SIZE;
uint8_t Buffer[BUFFER_SIZE];

States_t State = LOWPOWER;

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


void OnTxDone( void )
{
    printf("OnTxDone\n");
    Radio.Sleep( );
    State = TX;
}

void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr )
{
    printf("OnRxDone\n");
    Radio.Sleep( );
    BufferSize = size;
    memcpy( Buffer, payload, BufferSize );
    RssiValue = rssi;
    SnrValue = snr;
    State = RX;
}

void OnTxTimeout( void )
{
    printf("OnTxTimeout\n");
    Radio.Sleep( );
    State = TX_TIMEOUT;
}

void OnRxTimeout( void )
{
    printf("OnRxTimeout\n");
    Radio.Sleep( );
    State = RX_TIMEOUT;
}

void OnRxError( void )
{
    printf("OnRxError\n");
    Radio.Sleep( );
    State = RX_ERROR;
}

#define TASKSTACKSIZE   2048

Task_Struct task0Struct;
Char task0Stack[TASKSTACKSIZE];

void maintask(UArg arg0, UArg arg1)
{
    bool isMaster = true;
    uint8_t i;

    printf("# Main Task Started\n");

    // Target board initialization
    printf("# Board Initialization\n");
    BoardInitMcu();
    BoardInitPeriph();

    // Radio initialization
    RadioEvents.TxDone = OnTxDone;
    RadioEvents.RxDone = OnRxDone;
    RadioEvents.TxTimeout = OnTxTimeout;
    RadioEvents.RxTimeout = OnRxTimeout;
    RadioEvents.RxError = OnRxError;

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

    printf("# Radio.Rx( %u ) - Starting\n", RX_TIMEOUT_VALUE);
    Radio.Rx( RX_TIMEOUT_VALUE);
    printf("# Radio.Rx( %u ) - Finished\n", RX_TIMEOUT_VALUE);

    while (1)
    {
        switch (State)
        {
        case RX:
            if (isMaster == true)
            {
                if (BufferSize > 0)
                {
                    if (strncmp((const char*) Buffer, (const char*) PongMsg, 4)
                            == 0)
                    {
                        // Indicates on a LED that the received frame is a PONG
//                        GpioWrite(&Led1, GpioRead(&Led1) ^ 1);
                        toggleLed(Board_RLED);
//                        uartputs("Got PONG");

                        // Send the next PING frame
                        Buffer[0] = 'P';
                        Buffer[1] = 'I';
                        Buffer[2] = 'N';
                        Buffer[3] = 'G';
                        // We fill the buffer with numbers for the payload
                        for (i = 4; i < BufferSize; i++)
                        {
                            Buffer[i] = i - 4;
                        }
                        DelayMs(1);
                        printf("# Sending PING\n");
                        Radio.Send(Buffer, BufferSize);
                    }
                    else if (strncmp((const char*) Buffer,
                                     (const char*) PingMsg, 4) == 0)
                    { // A master already exists then become a slave
                        isMaster = false;
//                        GpioWrite(&Led2, 1); // Set LED off
                        setLed(Board_GLED, 0);
                        Radio.Rx( RX_TIMEOUT_VALUE);
                    }
                    else // valid reception but neither a PING or a PONG message
                    {    // Set device as master ans start again
                        isMaster = true;
                        Radio.Rx( RX_TIMEOUT_VALUE);
                    }
                }
            }
            else
            {
                if (BufferSize > 0)
                {
                    if (strncmp((const char*) Buffer, (const char*) PingMsg, 4)
                            == 0)
                    {
                        // Indicates on a LED that the received frame is a PING
//                        GpioWrite(&Led1, GpioRead(&Led1) ^ 1);
                        toggleLed(Board_RLED);

                        // Send the reply to the PONG string
                        Buffer[0] = 'P';
                        Buffer[1] = 'O';
                        Buffer[2] = 'N';
                        Buffer[3] = 'G';
                        // We fill the buffer with numbers for the payload
                        for (i = 4; i < BufferSize; i++)
                        {
                            Buffer[i] = i - 4;
                        }
                        DelayMs(1);
                        printf("# Sending PONG\n");
                        Radio.Send(Buffer, BufferSize);
                    }
                    else // valid reception but not a PING as expected
                    {    // Set device as master and start again
                        isMaster = true;
                        Radio.Rx( RX_TIMEOUT_VALUE);
                    }
                }
            }
            State = LOWPOWER;
            break;
        case TX:
            // Indicates on a LED that we have sent a PING [Master]
            // Indicates on a LED that we have sent a PONG [Slave]
//            GpioWrite(&Led2, GpioRead(&Led2) ^ 1);
            toggleLed(Board_GLED);
            Radio.Rx( RX_TIMEOUT_VALUE);
            State = LOWPOWER;
            break;
        case RX_TIMEOUT:
        case RX_ERROR:
            if (isMaster == true)
            {
                // Send the next PING frame
                Buffer[0] = 'P';
                Buffer[1] = 'I';
                Buffer[2] = 'N';
                Buffer[3] = 'G';
                for (i = 4; i < BufferSize; i++)
                {
                    Buffer[i] = i - 4;
                }
                DelayMs(1);
                printf("# Sending PING Starter\n");
                Radio.Send(Buffer, BufferSize);
            }
            else
            {
                Radio.Rx( RX_TIMEOUT_VALUE);
            }
            State = LOWPOWER;
            break;
        case TX_TIMEOUT:
            Radio.Rx( RX_TIMEOUT_VALUE);
            State = LOWPOWER;
            break;
        case LOWPOWER:
        default:
            // Set low power
            break;
        }

//        TimerLowPowerHandler();
        Task_sleep(TIME_MS * 50);
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
    setupuart();

    /* Start BIOS */
    BIOS_start();

    return (0);
}
