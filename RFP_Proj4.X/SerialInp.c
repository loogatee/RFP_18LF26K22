#include "pic18lf26k22.h"
#include "PicTypes.h"
#include "SerialInp.h"
#include "SerialOutp.h"
#include "Misc.h"
#include "Cmds.h"


#define SERI_STATE_GETCHARS    0
#define SERI_STATE_WAITDONE    1

#define SERI_MAX_CHARS         21



                                               // INPUT related global variables:
static u8     seri_state_machine;              //   state of Serial machine, Input
static bool   seri_CmdDone;                    //   signal for CMD completion
static u8     seri_cnt;                        //   input: count of chars in buffer
static u8     seri_ch;                         //   input: character just received
static char   seri_dat[SERI_MAX_CHARS];        //   input: data buffer


static const char   *seri_crlf   = "\n\r";     // carriage-return, line-feed
static const char   *seri_prompt = ">> ";      // prompt string
static const char   *seri_bksp   = "\b \b";    // backspace string



void SerialInp_Init( void )
{
    seri_cnt           = 0;
    seri_state_machine = SERI_STATE_GETCHARS;
}



void SerialInp_Process( void )
{   
    switch( seri_state_machine )
    {
    case SERI_STATE_GETCHARS: 

        if( PIC_REGISTER PIR1bits.RC1IF == 0 ) { return; }

        seri_ch = PIC_REGISTER RCREG1;

        if( seri_ch == ASCII_CARRIAGERETURN || seri_ch == ASCII_LINEFEED )
        {
            SER_PrintSTR(seri_crlf);
            seri_dat[seri_cnt] = 0;

            if( seri_cnt > 0 )
            {
                seri_cnt           = 0;
                seri_CmdDone       = FALSE;
                seri_state_machine = SERI_STATE_WAITDONE;
                CMDS_SetInputStr(seri_dat);
            }
            else
            {
                SER_PrintSTR(seri_prompt);
            }
        }
        else if( seri_ch == ASCII_BACKSPACE || seri_ch == ASCII_DELETE )
        {
            if( seri_cnt > 0 )
            {
                SER_PrintSTR(seri_bksp);
                --seri_cnt;
            }
        }
        else if(( seri_ch >= ASCII_SPACE ) && ( seri_ch <= ASCII_TILDE ))
        {
            SER_PrintCH(seri_ch);
            seri_dat[seri_cnt] = seri_ch;
            if( ++seri_cnt >= SERI_MAX_CHARS ) { --seri_cnt; }
        }
        
        break;
        
    case SERI_STATE_WAITDONE:
        
        if( seri_CmdDone == TRUE )
        {
            SER_PrintSTR(seri_crlf);
            SER_PrintSTR(seri_prompt);
            seri_state_machine = SERI_STATE_GETCHARS;
        }
        break;
    }
}

    
void SerialInp_SignalCmdDone( void )
{
    seri_CmdDone = TRUE;
}


