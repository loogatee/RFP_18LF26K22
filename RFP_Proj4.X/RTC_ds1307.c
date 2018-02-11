#include "pic18lf26k22.h"
#include "PicTypes.h"
#include "Timer.h"
#include "i2c.h"
#include "SerialOutp.h"
#include "Misc.h"
#include "RTC.h"
#include <string.h>



#define STATE_GET_TIME     0
#define STATE_SHOW_TIME    1
#define STATE_DO_WAIT      2


static u8   rtc_rbuf[10];           // for returned data from the I2C driver
static u8   rtc_i2c1_rcompl;        // location where I2C driver will use to signal completion, reads
static u8   rtc_i2c1_wcompl;        // completion pointer, writes
static char rtc_sdat0[27];          // used to build up Date string that is presented to user
static u8   rtc_loop_state;         // state machine for the showtime_loop
static u16  rtc_loop_timer;         // timer counter for the showtime_loop


static I2CCMDS rtc_registers[] =
{
    { 0xD0, 0x00, 0x07, I2C_CMDTYPE_RW },        // RTC: read the registers
    { 0xff, 0xff, 0xff, 0xff           },        // Terminate the List
};

static const char *gWeeks[7] =
{
   (const char *)"Sun  ",     // 1
   (const char *)"Mon  ",     // 2
   (const char *)"Tue  ",
   (const char *)"Wed  ",
   (const char *)"Thu  ",
   (const char *)"Fri  ",
   (const char *)"Sat  "      // 7
};


//
// { SlaveAddr, CmdReg, NumBytes, CmdType }
//
static I2CCMDS rtc_regs_w0[2] =
{
    { 0xD0, 0x00, 0x07, I2C_CMDTYPE_WRITEONLY },        // RTC: write the registers
    { 0xff, 0xff, 0xff, 0xff                  },        // Terminate the List
};

static I2CCMDS rtc_regs_w1[2] =
{
    { 0xD0, 0x00, 0x01, I2C_CMDTYPE_WRITEONLY },        // RTC: write the registers
    { 0xff, 0xff, 0xff, 0xff                  },        // Terminate the List
};

                            //    80, mins,  hrs, wkday,  day,  mon,  yr
static u8  rtc_wdata0[7]    = { 0x80, 0x38, 0x12,  0x03, 0x31, 0x01, 0x17 };
static u8  rtc_wdata1[1]    = { 0 };
static u8  rtc_wdata2[7]    = { 0x80, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00 };




void RTC_Init( void )
{
    strcpy(rtc_sdat0, "xxx  xx/xx/xxxx   xx:xx:xx");
    rtc_loop_state = STATE_GET_TIME;
}

void RTC_GetTime( void )
{
    rtc_rbuf[0] = 0;
    I2C_1master_SendCmd( rtc_rbuf, 0, &rtc_i2c1_rcompl, rtc_registers );
}

void RTC_SetTime_Canned( void )
{
    I2C_1master_SendCmd( 0, rtc_wdata0, 0,                rtc_regs_w0);
    I2C_1master_SendCmd( 0, rtc_wdata1, &rtc_i2c1_wcompl, rtc_regs_w1);
}

void RTC_SetTime( u8 *TA )
{
    u8 i;
    
    for( i=0; i < 6; ++i )
        rtc_wdata2[i+1] = TA[i];
    
    I2C_1master_SendCmd( 0, rtc_wdata2, 0,                rtc_regs_w0);
    I2C_1master_SendCmd( 0, rtc_wdata1, &rtc_i2c1_wcompl, rtc_regs_w1);
}

//
//   OSC=64Mhz, I2C=400kHz:  time=371.5us
//   OSC=64Mhz, I2C=100kHz:  time=1.094ms
//
//   OSC=32Mhz, I2C=400kHz:  time=544.20us
//   OSC=32Mhz, I2C=100kHz:  time=1.209ms
//
u8 RTC_ShowTime( void )
{
    u8  dow;
    u8  retv = rtc_i2c1_rcompl;

    if( retv != I2C_COMPLETION_BUSY )
    {
        if( retv == I2C_COMPLETION_TIMEOUT )
        {
            SER_PrintSTR( "Timeout RTC!!" );
            SER_Print8  ( "    buf[0]: ", rtc_rbuf[0] );
        }
        else                                               // ELSE completed OK
        {
            dow = (rtc_rbuf[3] & 0x07) - 1;
            if( dow > 7 ) { dow = 0; }                     // covers the case of receiving bad data
            
            strcpy( rtc_sdat0, gWeeks[dow] );              // Day of Week: mon, tue, etc
            
            BtoHnz( rtc_rbuf[5], &rtc_sdat0[ 5] );         // Month
            BtoHnz( rtc_rbuf[4], &rtc_sdat0[ 8] );         // Day
            BtoHnz(        0x20, &rtc_sdat0[11] );         // 20something
            BtoHnz( rtc_rbuf[6], &rtc_sdat0[13] );         // year
            BtoHnz( rtc_rbuf[2], &rtc_sdat0[18] );         // Hours, military format
            BtoHnz( rtc_rbuf[1], &rtc_sdat0[21] );         // minutes
            BtoHnz( rtc_rbuf[0], &rtc_sdat0[24] );         // seconds

            SER_PrintSTRram( rtc_sdat0 );
        }
    }
    return retv;
}

//
//   OSC=64Mhz, I2C=400kHz:  time=475.7
//   OSC=64Mhz, I2C=100kHz:  time=1.328ms
//
//   OSC=32Mhz, I2C=400kHz:  time=708.5us
//   OSC=32Mhz, I2C=100kHz:  time=1.447ms
//
u8 RTC_SetComplete( void )
{
    u8  retv = rtc_i2c1_wcompl;

    if( retv != I2C_COMPLETION_BUSY )
    {
        if( retv == I2C_COMPLETION_TIMEOUT )
        {
            SER_PrintSTR( "Timeout RTC!!" );
        }
        else
        {
            SER_PrintSTR( "Set Done" );
        }
    }
    return retv;
}





void RTC_ShowTime_Loop( void )
{   
    switch( rtc_loop_state )
    {
    case STATE_GET_TIME:
        
        rtc_loop_timer = TIM_GetTickW();
        RTC_GetTime();
        rtc_loop_state = STATE_SHOW_TIME;
        break;
        
    case STATE_SHOW_TIME:

        if( RTC_ShowTime() )
        {
            SER_PrintSTR( "\n\r" );
            rtc_loop_state = STATE_DO_WAIT;
        }
        break;
        
    case STATE_DO_WAIT:
        
        if( TIM_GetDeltaW(rtc_loop_timer) >= 500 )
            rtc_loop_state = STATE_GET_TIME;
        break;
    }
}
