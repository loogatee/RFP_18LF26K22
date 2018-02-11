#include <xc.h>
#include "pic18lf26k22.h"
#include "PICRegs.h"
#include "PICTypes.h"
#include "Timer.h"



void PICRegs_Init_Ports( void )
{
    ADCON0 = 0x00;              // ADC is disabled and consumes no operating current
    ADCON1 = 0x00;              // N/A when ADON=0
    ADCON2 = 0x00;              // N/A when ADON=0
    SLRCON = 0x00;              // Slew at the standard rate (PORTA, PORTB, PORTC)

    ANSELA = 0x00;              // Digital input buffers Enabled (A0,A1,A2,A3,A5)
    LATA   = 0x00;              // outputs all initially LOW
    TRISA  = 0xFF;              // Init to Inputs for starters

    TRISAbits.TRISA0 = 0;       // NC
    TRISAbits.TRISA1 = 0;       // NC
    TRISAbits.TRISA2 = 0;       // NC
    TRISAbits.TRISA3 = 0;       // NC
    TRISAbits.TRISA4 = 0;       // NC
    TRISAbits.TRISA5 = 0;       // NC
    TRISAbits.TRISA6 = 0;       // NC
    TRISAbits.TRISA7 = 0;       // NC

    ANSELB = 0x00;              // Digital input buffers Enabled (B0..B5)
    LATB   = 0x00;              // outputs all initially LOW
    TRISB  = 0xFF;              // Init to Inputs for starters

    TRISBbits.TRISB0 = 0;       // NC
    TRISBbits.TRISB1 = 0;       // Output, Scope probe (1ms TMR0 output)
    TRISBbits.TRISB2 = 0;       // NC
    TRISBbits.TRISB3 = 0;       // NC
    TRISBbits.TRISB4 = 0;       // NC
    TRISBbits.TRISB5 = 0;       // NC
    TRISBbits.TRISB6 = 0;       // NC
    TRISBbits.TRISB7 = 0;       // NC

    ANSELC = 0x00;              // Digital input buffers Enabled (C2..C7)
    LATC   = 0x00;              // outputs all initially LOW
    TRISC  = 0xFF;              // Init to Inputs for starters

    TRISCbits.TRISC0 = 0;       // NC
    TRISCbits.TRISC1 = 0;       // Output, Scope probe
    TRISCbits.TRISC2 = 0;       // NC
    TRISCbits.TRISC3 = 0;       // NC
    TRISCbits.TRISC4 = 0;       // NC
    TRISCbits.TRISC5 = 0;       // NC
    TRISCbits.TRISC6 = 0;       // NC
    TRISCbits.TRISC7 = 0;       // NC
}

void PICRegs_Init_Oscillator( void )
{
    OSCCONbits.IDLEN  = 0;                 // Device enters Sleep mode on SLEEP instruction
    OSCCONbits.IRCF   = 7;                 // HFINTOSC - (16 MHz)
    OSCCONbits.SCS    = 0;                 // Primary clock (determined by FOSC in CONFIG1H)
    
    OSCCON2bits.MFIOSEL = 0;               // MFINTOSC is not used
    OSCCON2bits.SOSCGO  = 0;               // Secondary oscillator is Off if no other sources are requesting it
    OSCCON2bits.PRISD   = PIC_OSC_PRISD;   // Oscillator drive: OFF for Internal, ON for External
            
    OSCTUNEbits.PLLEN = 1;                 // PLL enabled
}

void PICRegs_Init_PortBPullups( void )
{
  //WPUB             = 0xFF;     // Pull-up Enabled on all B pins
  //INTCON2bits.RBPU = 0;        // PortB Pull-Ups Enabled (pin must be Input AND WPUB bit must be set)
    
    WPUB             = 0x00;     // Pull-up Disabled on all B pins
    INTCON2bits.RBPU = 1;        // All PORTB pull-ups are disabled
}

void PICRegs_Init_Interrupts( void )
{
    RCONbits.IPEN   = 1;         // Enable priority levels on interrupts
    INTCONbits.GIE  = 1;         // Enables all high-priority interrupts
    INTCONbits.PEIE = 1;         // Enables all low-priority peripheral interrupts
}


void PICRegs_Init_TMR0( void )
{
    T0CONbits.T0PS    = 0;       // Prescaler Select Bits: N/A
    T0CONbits.PSA     = 1;       // TMR0 clock input bypasses prescaler
    T0CONbits.T0SE    = 0;       // Increment on Low-to-High transition on TOCKI pin
    T0CONbits.T0CS    = 0;       // Internal instruction cycle clock (CLK0)
    T0CONbits.T08BIT  = 0;       // 16-bit Timer/Counter
    T0CONbits.TMR0ON  = 0;       // Not Enabled yet

    TMR0H = TIM0_HVAL;           // Timer0 Register High Byte
    TMR0L = TIM0_LVAL;           // Timer0 Register Low Byte
}

void PICRegs_Init_TMR0_Intrs( void )
{
  //INTCON2bits.TMR0IP = 1;      // High Priority Interrupt
    INTCON2bits.TMR0IP = 0;      // Low Priority Interrupt
    INTCONbits.TMR0IF  = 0;      // Clears TMR0 Overflow Flag
    INTCONbits.TMR0IE  = 1;      // Enables the TMR0 overflow interrupt
}
