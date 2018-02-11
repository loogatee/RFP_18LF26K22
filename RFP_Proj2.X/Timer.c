#include <xc.h>
#include "pic18lf26k22.h"
#include "PICTypes.h"
#include "PICRegs.h"
#include "Timer.h"




static u8  TIM_TickB;
static u16 TIM_TickW;



void TIM_Init( void )
{
    TIM_TickB = 0;
    TIM_TickW = 0;
}


u8 TIM_GetTickB( void )
{
    return TIM_TickB;
}


u16 TIM_GetTickW( void )
{
    return TIM_TickW;
}



u8 TIM_GetDeltaB( u8 old_time )
{
u8  cur_time;

    cur_time = TIM_TickB;

    if( cur_time >= old_time )
    {
        return( cur_time - old_time );
    }
    else
    {
        return( ~old_time + 1 + cur_time );
    }
}



u16 TIM_GetDeltaW( u16 old_time )
{
u16 cur_time;

    cur_time = TIM_TickW;

    if( cur_time >= old_time )
    {
        return( cur_time - old_time );
    }
    else
    {
        return( ~old_time + 1 + cur_time );
    }
}




void TIM_1ms_Interrupt( void )
{
    //PIC_Toggle_B1();                // toggle a GPIO line (RB1)
    ++ TIM_TickB;
    ++ TIM_TickW;
}



