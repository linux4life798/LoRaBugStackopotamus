#ifndef __LEDBOARD_H__
#define __LEDBOARD_H__

/* Board Header files */
#include "Board.h"

#define Board_TRI_RED   Board_HDR_HDIO0
#define Board_TRI_GREEN Board_HDR_HDIO1
#define Board_TRI_BLUE  Board_HDR_HDIO2

#define Board_FET_RED   Board_HDR_ADIO0
#define Board_FET_GREEN Board_HDR_ADIO1
#define Board_FET_BLUE  Board_HDR_ADIO2
#define Board_FET_WHITE Board_HDR_ADIO3

#define Board_BTN_SEL   Board_HDR_ADIO5
#define Board_BTN_NEXT  Board_HDR_ADIO6
#define Board_BTN_PREV  Board_HDR_ADIO7

#endif // __LEDBOARD_H__
