#ifndef _PICRegs_H_
#define _PICRegs_H_


//
//  Internal Oscillator operates at 16Mhz, uses 4xPLL to achieve 64Mhz
//  External Oscillator is 8Mhz, uses 4xPLL to achieve 32Mhz
//  Use only 1 of the following 2 defines
//
#define USE_INTERNAL_OSC_64Mhz
//#define USE_EXTERNAL_OSC_32Mhz

#if defined(USE_INTERNAL_OSC_64Mhz)
    #define PIC_OSC_FOSC     8
    #define PIC_OSC_PRISD    0
#elif defined(USE_EXTERNAL_OSC_32Mhz)
    #define PIC_OSC_FOSC     3
    #define PIC_OSC_PRISD    1
#endif



#define PIC_Set_C1(val)        LATCbits.LATC1 = val


#define PIC_Toggle_B1()        LATBbits.LATB1 = !LATBbits.LATB1
#define PIC_Toggle_C1()        LATCbits.LATC1 = !LATCbits.LATC1


#define PIC_TMR0_START()       T0CONbits.TMR0ON = 1



void PICRegs_Init_Ports       ( void );
void PICRegs_Init_Oscillator  ( void );
void PICRegs_Init_PortBPullups( void );
void PICRegs_Init_Interrupts  ( void );
void PICRegs_Init_TMR0        ( void );
void PICRegs_Init_EUSART1     ( void );


#endif


