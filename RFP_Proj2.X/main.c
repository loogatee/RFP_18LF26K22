#include <xc.h>
#include "pic18lf26k22.h"
#include "Configbits.h"
#include "PICTypes.h"
#include "PICRegs.h"
#include "Timer.h"




void main( void )
{
    u8  btime;
    u16 wtime;
    
    PICRegs_Init_Oscillator();             // 64Mhz if Internal, 32Mhz if External
    PICRegs_Init_Ports();                  // All the tris settings: inputs and outputs
    PICRegs_Init_PortBPullups();           // Weak pullups enabled: only affects inputs
    PICRegs_Init_Interrupts();             // High and Low Priority Intrs Enabled
    
    TIM_Init();
    
    PICRegs_Init_TMR0();                   // 1ms, Low Priority
    PICRegs_Init_TMR0_Intrs();             // TMR0 Interrupts Enabled, ready to go
    PIC_TMR0_START();                      // Timer starts to count

    btime = TIM_GetTickB();
    wtime = TIM_GetTickW();
    while( 1 )
    {
        if( TIM_GetDeltaB( btime ) >= 20 )      // 20ms timer
        {
            PIC_Toggle_B1();
            btime = TIM_GetTickB();
        }
        
        if( TIM_GetDeltaW( wtime ) >= 1000 )    // 1sec timer
        {
            PIC_Toggle_C1();
            wtime = TIM_GetTickW();
        }
    }
}



void interrupt low_priority LowISR( void )
{
    if( INTCONbits.TMR0IF == 1 )
    {
        TMR0H = TIM0_HVAL;                  // re-load counter, High Byte
        TMR0L = TIM0_LVAL;                  // re-load counter, Low Byte
        INTCONbits.TMR0IF = 0;              // Clear out the overflow flag
        TIM_1ms_Interrupt();                // Only purpose is to increment a couple of counters
    }
}


void interrupt HighIsr( void )
{
#if 0
    if( INTCONbits.TMR0IF == 1)
    {
        TMR0H = TIM0_HVAL;                  // re-load counter, High Byte
        TMR0L = TIM0_LVAL;                  // re-load counter, Low Byte
        INTCONbits.TMR0IF = 0;              // Clear out the overflow flag
        TIM_1ms_Interrupt();                // Only purpose is to increment a couple of counters
    }
#endif
}






