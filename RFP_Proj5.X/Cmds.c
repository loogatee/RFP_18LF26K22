#include "pic18lf26k22.h"
#include "PICTypes.h"
#include "Cmds.h"
#include "SerialOutp.h"
#include "SerialInp.h"
#include "Misc.h"
#include "Version.h"
#include "RTC.h"
#include "Flash.h"
#include <string.h>



#define  CMDSM_WAITFORLINE       0
#define  CMDSM_MEMDUMP           1
#define  CMDSM_RTC_RDONE         2
#define  CMDSM_RTC_TSDONE        3
#define  CMDSM_RTC_TXDONE        4
#define  CMDSM_FLASH_RDONE       5
#define  CMDSM_FLASH_WDONE       6
#define  CMDSM_FLASH_EDONE       7

#define  DO_INIT                 0
#define  DO_PROCESS              1


static bool   cmds_input_ready;
static char  *cmds_InpPtr;
static u8     cmds_state_machine;
static u8     cmds_completion;
static u16    cmds_word1;
static u8     cmds_byte1;
static u8     cmds_TA[8];
static UW2B   cmds_addr;

static volatile u8     cmds_xtest;

static bool cmds_FR( u8 state );
static bool cmds_FW( u8 state );
static bool cmds_FE( u8 state );
static bool cmds_R ( void );
static bool cmds_T ( void );
static bool cmds_MD( u8 state );
static bool cmds_TR( u8 state );
static bool cmds_TS( u8 state );
static bool cmds_TX( u8 state );



void CMDS_Init(void)
{
    cmds_input_ready   = FALSE;
    cmds_state_machine = CMDSM_WAITFORLINE;
    
    cmds_xtest         = 0x98;
    (u8)*((u8 *)0xB00) = 0xBC;
    (u8)*((u8 *)0xB01) = 0xDE;
    (u8)*((u8 *)0xB02) = 0xAF;
}


void CMDS_Process(void)
{
    char  X0          = cmds_InpPtr[0];
    char  X1          = cmds_InpPtr[1];
    bool  signal_done = TRUE;
    
    switch( cmds_state_machine )
    {
    case CMDSM_WAITFORLINE:
        
        if( cmds_input_ready == FALSE ) { return; }
        cmds_input_ready = FALSE;
        
        if     ( X0 == 'f' && X1 == 'e' )  signal_done = cmds_FE( DO_INIT );             // Flash Erase (8 bytes)
        else if( X0 == 'f' && X1 == 'r' )  signal_done = cmds_FR( DO_INIT );             // Flash Read
        else if( X0 == 'f' && X1 == 'w' )  signal_done = cmds_FW( DO_INIT );             // Flash Write
        else if( X0 == 'm' && X1 == 'd' )  signal_done = cmds_MD( DO_INIT );             // Memory Dump
        else if( X0 == 'r' )               signal_done = cmds_R();                       // Read: Ports, Latches, Direction, Memory
        else if( X0 == 't' && X1 == 'r' )  signal_done = cmds_TR( DO_INIT );             // Time Read
        else if( X0 == 't' && X1 == 's' )  signal_done = cmds_TS( DO_INIT );             // Time Set
        else if( X0 == 't' && X1 == 'x' )  signal_done = cmds_TX( DO_INIT );             // Time Set Canned:  Tue 01/31/2017   12:38:00
        else if( X0 == 't' )               signal_done = cmds_T();                       // Test: t1, t2, etc
        else if( X0 == 'v' )               signal_done = CMDS_DisplayVersion();          // Display Version

        break;

    case CMDSM_MEMDUMP:      signal_done = cmds_MD( DO_PROCESS );    break;
    case CMDSM_FLASH_RDONE:  signal_done = cmds_FR( DO_PROCESS );    break;
    case CMDSM_FLASH_WDONE:  signal_done = cmds_FW( DO_PROCESS );    break;
    case CMDSM_FLASH_EDONE:  signal_done = cmds_FE( DO_PROCESS );    break;
    case CMDSM_RTC_RDONE:    signal_done = cmds_TR( DO_PROCESS );    break;
    case CMDSM_RTC_TSDONE:   signal_done = cmds_TS( DO_PROCESS );    break;
    case CMDSM_RTC_TXDONE:   signal_done = cmds_TX( DO_PROCESS );    break;
    }

    if( signal_done == TRUE ) { SerialInp_SignalCmdDone(); }
}


bool CMDS_DisplayVersion(void)
{
    SER_PrintSTR("\r\n");
    SER_Print8N (VERSION_STR, VERSION_MINOR);
    SER_PrintSTR(", ");
    SER_PrintSTR(VERSION_DATE);
    
    return TRUE;
}


void CMDS_SetInputStr(char *StrInp)
{
    cmds_InpPtr      = StrInp;
    cmds_input_ready = TRUE;
}


static bool cmds_FE( u8 state )
{
    bool retv = FALSE;
    
    switch( state )
    {
    case DO_INIT:
        
        cmds_addr.w = HtoU16( &cmds_InpPtr[3] );
        Flash_Erase8(cmds_addr);
        cmds_state_machine = CMDSM_FLASH_EDONE;
        break;
        
    case DO_PROCESS:

        if( Flash_EraseComplete() != FLASH_COMPLETION_BUSY )
        {
            cmds_state_machine = CMDSM_WAITFORLINE;
            retv = TRUE;
        }
        break;
    }
    
    return retv;
}


//
// fr XXXX    where msb <= 0x7F
// Reads 8 bytes at XXXX\r\n" );
//
static bool cmds_FR( u8 state )
{
    bool retv = FALSE;

    switch( state )
    {
    case DO_INIT:
        
        cmds_addr.w = HtoU16( &cmds_InpPtr[3] );
        Flash_GetMem16(cmds_addr);
        cmds_state_machine = CMDSM_FLASH_RDONE;
        cmds_byte1 = 0;
        
        break;
        
    case DO_PROCESS:

        if( Flash_ShowMem16() != FLASH_COMPLETION_BUSY )
        {
            cmds_addr.w += 16;
            
            if( ++cmds_byte1 == 4 )
            {
                ItoH( cmds_addr.w, &cmds_InpPtr[3] );
                cmds_state_machine = CMDSM_WAITFORLINE;
                retv = TRUE;
            }
            else
            {
                Flash_GetMem16(cmds_addr);
            }
        }
        break;
    }
    
    return retv;
}

//
//  0123456789012345678901234
//  fw 0400 1203310117223344
//
static bool cmds_FW( u8 state )
{
    char wbuf[3];
    u8   i,k;
    bool retv = FALSE;
    
    wbuf[2] = 0;
    
    switch( state )
    {
    case DO_INIT:
        
        cmds_addr.w = HtoU16( &cmds_InpPtr[3] );

        for( i=8,k=0; i < 24; i += 2 )
        {
            wbuf[0]      = cmds_InpPtr[i];
            wbuf[1]      = cmds_InpPtr[i+1];
            cmds_TA[k++] = (u8)HtoU16( wbuf );
        }
        
        Flash_Write8( cmds_addr, cmds_TA );
        cmds_state_machine = CMDSM_FLASH_WDONE;
        break;
        
    case DO_PROCESS:

        if( Flash_WriteComplete() != FLASH_COMPLETION_BUSY )
        {
            cmds_state_machine = CMDSM_WAITFORLINE;
            retv = TRUE;
        }
        break;
    }
    
    return retv;
}


static bool cmds_MD( u8 state )
{
    u8   i;
    bool retv = FALSE;
    
    switch( state )
    {
    case DO_INIT:
        
        cmds_word1         = HtoU16( &cmds_InpPtr[3] ) & 0x0FFF;
        cmds_byte1         = 0;
        cmds_state_machine = CMDSM_MEMDUMP;
        cmds_completion    = 1;
        FALL_THRU;
        
    case DO_PROCESS:
    
        if( cmds_completion == 1 )
        {
            SER_Print16N( "0x", cmds_word1 );
            SER_Print8N( ": ", (u8)*((u8 *)cmds_word1++) );

            for( i=0; i < 15; i++ )
            {
                SER_Print8N( " ", (u8)*((u8 *)cmds_word1++) );
            }

            SER_Send( SERO_TYPE_ROM, (char *)"\r\n", &cmds_completion, 0 );

            if( ++cmds_byte1 == 4 )
            {
                cmds_state_machine = CMDSM_WAITFORLINE;
                ItoH( cmds_word1, &cmds_InpPtr[2] );
                retv = TRUE;
            }
        }
        break;
    }
    
    return retv;
}


static bool cmds_R( void )
{
    u16  tmpw;
    
    if( cmds_InpPtr[1] == 'p' )
    {
        SER_Print8( "PA ", (u8)PORTA );
        SER_Print8( "PB ", (u8)PORTB );
        SER_Print8( "PC ", (u8)PORTC );
    }
    else if( cmds_InpPtr[1] == 'd' )
    {
        SER_Print8( "DA ", (u8)TRISA );
        SER_Print8( "DB ", (u8)TRISB );
        SER_Print8( "DC ", (u8)TRISC );
    }
    else if( cmds_InpPtr[1] == 'l' )
    {
        SER_Print8( "LA ", (u8)LATA );
        SER_Print8( "LB ", (u8)LATB );
        SER_Print8( "LC ", (u8)LATC );
    }
    else if( cmds_InpPtr[1] == 'm' )
    {
        tmpw = HtoU16( &cmds_InpPtr[3] ) & 0x0FFF;
        SER_Print16N( "0x", tmpw );
        SER_Print8( ": ", (u8)*((u8 *)tmpw) );
    }
    
    return TRUE;
}


static bool cmds_TR( u8 state )
{
    bool retv = FALSE;
    
    switch( state )
    {
    case DO_INIT:
        
        RTC_GetTime();
        cmds_state_machine = CMDSM_RTC_RDONE;
        break;
        
    case DO_PROCESS:

        if( RTC_ShowTime() != RTC_COMPLETION_BUSY )
        {
            cmds_state_machine = CMDSM_WAITFORLINE;
            retv = TRUE;
        }
        break;
    }
    
    return retv;
}


//
//  012345678901234567890
//  st 38 12 03 31 01 17
//
static bool cmds_TS( u8 state )
{
    u8  i,k;
    bool retv = FALSE;
    
    if( strlen(cmds_InpPtr) == 2 )
    {
        SER_PrintSTR( "st mins hrs wkday day mon yr\r\n" );
        return TRUE;
    }
    
    switch( state )
    {
    case DO_INIT:
        
        for( i=3,k=0; i < 20; i += 3 )
        {
            cmds_InpPtr[i+2] = 0;
            cmds_TA[k++]     = (u8)HtoU16( &cmds_InpPtr[i] );
        }

        RTC_SetTime( cmds_TA );
        cmds_state_machine = CMDSM_RTC_TSDONE;
        break;
        
    case DO_PROCESS:

        if( RTC_SetComplete() != RTC_COMPLETION_BUSY )
        {
            cmds_state_machine = CMDSM_WAITFORLINE;
            retv = TRUE;
        }
        break;    
        
    }
    
    return retv;
}


static bool cmds_TX( u8 state )
{
    bool retv = FALSE;
    
    switch( state )
    {
    case DO_INIT:
        
        RTC_SetTime_Canned();
        cmds_state_machine = CMDSM_RTC_TXDONE;
        break;
        
    case DO_PROCESS:

        if( RTC_SetComplete() != RTC_COMPLETION_BUSY )
        {
            cmds_state_machine = CMDSM_WAITFORLINE;
            retv = TRUE;
        }
        break;
    }
    
    return retv;
}


static bool cmds_T( void )
{
    int  tmpI;
    u16  tmp16;
    
    if( cmds_InpPtr[1] == '1' )
    {
        tmpI = AtoI("452");
        SER_Print16( "452: 0x", (u16)tmpI );
        
        tmpI = AtoI("-4392");
        if( tmpI == -4392 )
            SER_PrintSTR( "-4392 Good\r\n" );
        else
            SER_PrintSTR( "Conversion did not yield -4392\r\n" );
    }
    else if( cmds_InpPtr[1] == '2' )
    {
        tmp16 = (u16)*((u16 *)0xB00);
        SER_Print16( "*0xB00 = ", tmp16 );
    }
    
    return TRUE;
}

