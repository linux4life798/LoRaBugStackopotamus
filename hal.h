

//! \file
#ifndef _hal_h_
#define _hal_h_


#include <ti/drivers/PIN.h>
//#include "../lmic_16/oslmic.h"

//================================================================================
//================================================================================

typedef uint8_t u1_t;

#define DELAY_MS(i)    Task_sleep(((i) * 1000) / Clock_tickPeriod)
#define DELAY_US(i)    Task_sleep(((i) * 1) / Clock_tickPeriod)


//extern void printu(char* msg);




void hal_pin_nss (u1_t val);
void hal_pin_rxtx (u1_t val);
void hal_pin_rst (u1_t val);
void hal_pin_int_handler(PIN_Handle handle, PIN_Id pinId);
//static void hal_io_check();
bool hal_io_get_dio0();

void hal_spi_init();
u1_t hal_spi (u1_t outval);

//void hal_time_init();
//ostime_t hal_ticks ();
//void hal_waitUntil (ostime_t time);
//u1_t hal_checkTimer (ostime_t targettime);
//void hal_sleep ();

//void hal_disableIRQs ();
//void hal_enableIRQs ();

void hal_init ();
void hal_io_init();

void hal_failed ();
//void hal_failed_msg(char* msg);

void hal_sleep();


void writeReg (u1_t addr, u1_t data );
u1_t readReg (u1_t addr);


#endif // _hal_h_
