#include <xc.h>
#include "pic18lf26k22.h"
#include "Configbits.h"
#include "PICTypes.h"
#include "PICRegs.h"




void main( void )
{
    PICRegs_InitOscillator();              // 64Mhz if Internal, 32Mhz if External
    PICRegs_InitPorts();                   // All the tris settings: inputs and outputs
    PICRegs_InitPortBPullups();            // Weak pullups enabled: only affects inputs

    while( 1 )
    {
        PIC_Set_C1(1);
        PIC_Set_C1(0);
        PIC_Set_C1(1);
        PIC_Set_C1(0);
        PIC_Set_C1(1);
        PIC_Set_C1(0);
        PIC_Set_C1(1);
        PIC_Set_C1(0);
    }
}



void interrupt low_priority LowISR( void )
{
    ;
}


void interrupt HighIsr( void )
{
    ;
}






