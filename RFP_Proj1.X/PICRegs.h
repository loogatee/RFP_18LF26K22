#ifndef _PICRegs_H_
#define _PICRegs_H_


//
//  Internal Oscillator operates at 16Mhz, uses 4xPLL to achieve 64Mhz
//  External Oscillator is 8Mhz, uses 4xPLL to achieve 32Mhz
//  Use only 1 of the following 2 defines
//
//#define USE_INTERNAL_OSC_64Mhz
#define USE_EXTERNAL_OSC_32Mhz

#if defined(USE_INTERNAL_OSC_64Mhz)
    #define PIC_OSC_FOSC     8
    #define PIC_OSC_PRISD    0
#elif defined(USE_EXTERNAL_OSC_32Mhz)
    #define PIC_OSC_FOSC     3
    #define PIC_OSC_PRISD    1
#endif



#define PIC_Set_C1(val)            LATCbits.LATC1 = val



void PICRegs_InitPorts        ( void );
void PICRegs_InitOscillator   ( void );
void PICRegs_InitPortBPullups ( void );



#endif


