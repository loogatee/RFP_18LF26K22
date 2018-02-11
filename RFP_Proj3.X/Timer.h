#ifndef _TIMER_H
#define _TIMER_H

#include "PICRegs.h"

//---------------------------------------------------------------------
// Internal Clock
//   FOSC = 64,000,000
//
// External Clock
//   FOSC = 32,000,000
//---------------------------------------------------------------------



//---------------------------------------------------------------------
//   Timer0:  desire 1ms ticker
//
//   64Mhz:
//      Input = Fosc/4 = 64Mhz/4
//      1ms   = 1/.001 = 1000
//      ( 64Mhz ) / ( 4 * 1000 ) = 16000
//      65536 - 16000 = 49536 = 0xC180
//           use scope to dial in:  0xC160
//           factors in latency and inaccuracy of oscillator
//
//   32Mhz:
//      Input = Fosc/4 = 32Mhz/4
//      1ms   = 1/.001 = 1000
//      ( 32Mhz ) / ( 4 * 1000 ) = 8000
//      65536 - 8000 = 57536 = 0xE0C0
//          use scope to dial in:  0xE0CB
//          factors in latency and inaccuracy of oscillator
//---------------------------------------------------------------------
#if defined(USE_INTERNAL_OSC_64Mhz)
    #define  TIM0_HVAL       0xC1
    #define  TIM0_LVAL       0x60
#elif defined(USE_EXTERNAL_OSC_32Mhz)
    #define  TIM0_HVAL       0xE0
    #define  TIM0_LVAL       0xCB
#endif




void TIM_Init          ( void );
u8   TIM_GetTickB      ( void );
u16  TIM_GetTickW      ( void );
u8   TIM_GetDeltaB     ( u8  old_time );
u16  TIM_GetDeltaW     ( u16 old_time );
void TIM_1ms_Interrupt ( void );




#endif
