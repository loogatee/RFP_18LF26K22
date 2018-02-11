#include <xc.h>
#include "pic18lf26k22.h"
#include "Configbits.h"
#include "PICTypes.h"
#include "PICRegs.h"
#include "Timer.h"
#include "SerialDbg.h"
#include "Cmds.h"




void main( void )
{
    u8  btime;
    
    PICRegs_Init_Oscillator();                // 64Mhz if Internal, 32Mhz if External
    PICRegs_Init_Ports();                     // All the tris settings: inputs and outputs
    PICRegs_Init_PortBPullups();              // Weak pullups initialized: only affects PORTB inputs
    PICRegs_Init_Interrupts();                // High and Low Priority Intrs Enabled
    PICRegs_Init_EUSART1();                   // Inits RS232/#1 to 8bits, no parity, 9600 baud
    PICRegs_Init_TMR0();                      // 1ms, Interrupt Priority, Interrupt Enabled
    
    SerialDbg_Init();                         // Inits variables and structures for the Serial Port Debug print feature
    TIM_Init();                               // Inits the Timer routines
    CMDS_Init();                              // Inits variables and structs for the Commands Processor

    PIC_TMR0_START();                         // Timer starts to count

    btime = TIM_GetTickB();                   // init timer counter
    while( TIM_GetDeltaB( btime ) < 100 )     // spin for 100ms
        ;
    
    CMDS_DisplayVersion();                    // display the Version
    
    while( 1 )
    {
        SerialDbg_ProcessOutput();            // Output on the Serial Debug Channel
        SerialDbg_ProcessInput ();            // Input on the Serial Debug Channel
        CMDS_Process();                       // commands coming from Input on the Serial Channel
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
    ;
}






