#include "pic18lf26k22.h"
#include "PicTypes.h"
#include "SerialDbg.h"
#include "Misc.h"
#include "Cmds.h"



#define SERD_OSTATE_GETJOB      0
#define SERD_OSTATE_DOCHARS     1

#define SERD_ISTATE_GETCHARS    0
#define SERD_ISTATE_WAITDONE    1

#define SERDI_MAX_CHARS         16
#define SERDO_SQENTRYS          36

#define ASCII_BACKSPACE         8
#define ASCII_LINEFEED          10
#define ASCII_CARRIAGERETURN    13
#define ASCII_SPACE             32
#define ASCII_TILDE             126
#define ASCII_DELETE            127



typedef struct
{                                   // Items on the Serial Job Queue
    u8      sr_otype;               // Output Type.  See SERD_OTYPE_XX
    char   *sr_sptr;                // pointer to string to be printed
    u8     *sr_compPtr;             // if != 0, sets to 1 on completion
    UW2B    sr_dval;                // data value as 8-bit or 16-bit
} SERI;



                                               // OUTPUT related global variables:
static u8     serd_num_qitems;                 //   Number of Items on the Queue
static u8     serd_inn_qindex;                 //   Item will be deposited here
static u8     serd_out_qindex;                 //   Item will be removed from here
static u8     serd_ostate_machine;             //   holds state of Serial machine, Output
static SERI  *serd_active_item;                //   pointer to currently active item
static SERI   serd_data_items[SERDO_SQENTRYS]; //   Serial Job Data Items
static char   serd_databuf[7];                 //   data buffer for value conversion

                                               // INPUT related global variables:
static u8     serd_istate_machine;             //   state of Serial machine, Input
static bool   serd_CmdDone;                    //   signal for CMD completion
static u8     serd_inp_cnt;                    //   input: count of chars in buffer
static u8     serd_inp_ch;                     //   input: character just received
static char   serd_inp_dat[SERDI_MAX_CHARS];   //   input: data buffer


static const char   *serd_crlf   = "\n\r";     // carriage-return, line-feed
static const char   *serd_prompt = ">> ";      // prompt string
static const char   *serd_bksp   = "\b \b";    // backspace string






void SerialDbg_Init( void )
{
    serd_num_qitems     = 0;
    serd_inn_qindex     = 0;
    serd_out_qindex     = 0;
    serd_active_item    = 0;
    serd_inp_cnt        = 0;
    serd_inp_ch         = 0;
    serd_ostate_machine = SERD_OSTATE_GETJOB;
    serd_istate_machine = SERD_ISTATE_GETCHARS;
}


//---------------------------------------------------------------------------------------
//  Queues up a Job on the 'Serial Debug Output List' (Implemented as a Circular Q)
//  This is the 'Front End' or Producer of Serial Data.   The idea is to stash the
//  parameters quickly in a Queue, then let the 'Back End' (the Consumer) process the
//  data off the Queue, and perform the printing.
//  The data is destined for EUSART1.
//
//  Parameters:
//
//        otype - The type of output requested.  see SERD_OTYPE_ defines
//
//        sptr  - Pointer to string to be printed.   Could be either ROM
//                or RAM ptr. otype will indicate this
//
//        completionptr - IF 0, then there is no completion signal.   IF
//                        non-zero, this address will be signaled with a 1 
//                        to indicate request has been completed.
//
//        aval - Value that can optionally be printed along with sptr.
//               Value can be 8-bit or 16-bit.   otype will control this.
//
void SerialDbg_Sout( u8 otype, char *sptr, u8 *completionptr, u16 aval )
{
    SERI  *lqitem;                                                              // Pointer to Array Element where the data will go

    if( serd_num_qitems != SERDO_SQENTRYS )                                     // Proceed if the Queue is not full
    {
        lqitem          = &serd_data_items[serd_inn_qindex];                    // item will go in at index serd_inn_qindex
        serd_num_qitems = serd_num_qitems + 1;                                  // Queue size is increased by 1
        if( ++ serd_inn_qindex == SERDO_SQENTRYS ) { serd_inn_qindex = 0; }     // where the next item will go:  wrap if necessary

        lqitem->sr_dval.w  = aval;                                              // aval goes into element on the Q
        lqitem->sr_otype   = otype;                                             // otype goes into element on the Q
        lqitem->sr_sptr    = sptr;                                              // sptr goes into element on the Q
        lqitem->sr_compPtr = completionptr;                                     // completionptr goes into element on the Q
        
        if( completionptr != 0 ) { *completionptr = 0; }                        // if pointer is valid, store 0, indicating Not Done
    }
                                                                                // Else the Q is full.  Effectively tosses the data
}



void SerialDbg_ProcessOutput( void )
{
    char  end_of_string;
    u8    lotype;

    switch( serd_ostate_machine )
    {
    case SERD_OSTATE_GETJOB:                                                            // Looking for items on the Queue

        if( serd_num_qitems == 0 ) { return; }                                          // return IMMEDIATELY if Queue is empty

        serd_active_item   = &serd_data_items[serd_out_qindex];                         // Item to operate on is at 'serd_out_qindex'
        serd_ostate_machine = SERD_OSTATE_DOCHARS;                                      // switch state to 'Processing Characters'

        switch( serd_active_item->sr_otype )                                            // sr_otype dictates the action
        {
        case SERD_OTYPE_ONECHAR:  serd_databuf[0] = serd_active_item->sr_dval.b[0];     // Character to print is placed in databuf
                                  serd_databuf[1] = 0;                                  // string terminator
                                  serd_active_item->sr_sptr = serd_databuf;             // sptr point to the data in databuf
                                  break;
                                    
        case SERD_OTYPE_16:       ItoH(serd_active_item->sr_dval.w, serd_databuf);       // 8-bit to Hex Data Conversion.  Place in databuf
                                  serd_databuf[4] = 13;                                  // tack on <CR> at the end
                                  serd_databuf[5] = 10;                                  // tack on <LF> at the end
                                  serd_databuf[6] = 0;                                   // this is the string terminator
                                  break;
                                  
        case SERD_OTYPE_16N:      ItoH(serd_active_item->sr_dval.w, serd_databuf);       // 8-bit to Hex Data Conversion.  Place in databuf
                                  break;
                                    
        case SERD_OTYPE_8:        BtoH(serd_active_item->sr_dval.b[0], serd_databuf);    // 8-bit to Hex Data Conversion.  Place in databuf
                                  serd_databuf[2] = 13;                                  // tack on <CR> at the end
                                  serd_databuf[3] = 10;                                  // tack on <LF> at the end
                                  serd_databuf[4] = 0;                                   // this is the string terminator
                                  break;
                                  
        case SERD_OTYPE_8N:       BtoH(serd_active_item->sr_dval.b[0], serd_databuf);    // 8-bit to Hex Data Conversion.  Place in databuf
                                  break;                          
        }
        
        return;                                                                          // could FALL_THRU, but don't be a CPU hog!


    case SERD_OSTATE_DOCHARS:                                                       // Actively printing out characters

        if( PIC_REGISTER TXSTA1bits.TRMT == 0 ) { return; }                         // return IMMEDIATELY if TRMT indicates busy

        lotype = serd_active_item->sr_otype;                                        // local assign for optimization
        if( lotype == SERD_OTYPE_RAM )                                              // Characters coming from RAM ?
        {
            PIC_REGISTER TXREG1 = *serd_active_item->sr_sptr++;                     // TX reg filled with char from ROM
            end_of_string       = *serd_active_item->sr_sptr;                       // to examine character just past the one printed
        }
        else                                                                        // ELSE characters are in ROM
        {
            PIC_REGISTER TXREG1 = *(const char *)serd_active_item->sr_sptr++;       // TX reg filled with char from ROM
            end_of_string       = *(const char *)serd_active_item->sr_sptr;         // to examine character just past the one printed
        }

        if( end_of_string == 0 )                                                    // Found the string terminator ?
        {
            if( lotype >= SERD_OTYPE_16 )                                           // IF any of these types, still stuff to print
            {                                                                       //    ordering in SERD_OTYPE is important!!
                serd_active_item->sr_otype = SERD_OTYPE_RAM;                        // change type to RAM
                serd_active_item->sr_sptr  = (char *)serd_databuf;                  // data is in serd_databuf (which is in RAM)
            }
            else                                                                    // ELSE this print job is done
            {
                serd_ostate_machine = SERD_OSTATE_GETJOB;                           // switch state:  look for another job
                serd_num_qitems    = serd_num_qitems - 1;                           // Can now decrement Queue size by 1
                if( ++ serd_out_qindex == SERDO_SQENTRYS ) { serd_out_qindex = 0; } // index to next element in the Circular Q.  Wrap if necessary

                if( serd_active_item->sr_compPtr != 0 )                             // Is there a valid Completion Pointer ?
                    *serd_active_item->sr_compPtr = 1;                              //    signal a 1 to that address to indicate completion
            }
        }
    }
}



void SerialDbg_ProcessInput( void )
{   
    switch( serd_istate_machine )
    {
    case SERD_ISTATE_GETCHARS: 

        if( PIC_REGISTER PIR1bits.RC1IF == 0 ) { return; }

        serd_inp_ch = PIC_REGISTER RCREG1;

        if( serd_inp_ch == ASCII_CARRIAGERETURN || serd_inp_ch == ASCII_LINEFEED )
        {
            SER_PrintSTR(serd_crlf);
            serd_inp_dat[serd_inp_cnt] = 0;

            if( serd_inp_cnt > 0 )
            {
                serd_inp_cnt        = 0;
                serd_CmdDone        = FALSE;
                serd_istate_machine = SERD_ISTATE_WAITDONE;
                CMDS_SetInputStr(serd_inp_dat);
            }
            else
            {
                SER_PrintSTR(serd_prompt);
            }
        }
        else if( serd_inp_ch == ASCII_BACKSPACE || serd_inp_ch == ASCII_DELETE )
        {
            if( serd_inp_cnt > 0 )
            {
                SER_PrintSTR(serd_bksp);
                --serd_inp_cnt;
            }
        }
        else if(( serd_inp_ch >= ASCII_SPACE ) && ( serd_inp_ch <= ASCII_TILDE ))
        {
            SER_PrintCH(serd_inp_ch);
            serd_inp_dat[serd_inp_cnt] = serd_inp_ch;

            if( ++serd_inp_cnt >= SERDI_MAX_CHARS ) { --serd_inp_cnt; }
        }
        
        break;
        
    case SERD_ISTATE_WAITDONE:
        
        if( serd_CmdDone == TRUE )
        {
            SER_PrintSTR(serd_crlf);
            SER_PrintSTR(serd_prompt);
            serd_istate_machine = SERD_ISTATE_GETCHARS;
        }
        break;
    }
}

    
void SerialDbg_SignalCmdDone( void )
{
    serd_CmdDone = TRUE;
}








void SER_PrintCH( char ch )
{
    SerialDbg_Sout( SERD_OTYPE_ONECHAR, 0, 0, (u16)ch );
}

void SER_PrintSTR( const char *pstr )
{
    SerialDbg_Sout( SERD_OTYPE_ROM, (char *)pstr, 0, 0 );
}

void SER_PrintSTRram( char *pstr )
{
    SerialDbg_Sout( SERD_OTYPE_RAM, (char *)pstr, 0, 0 );
}

void SER_Print16( const char *pstr, u16 val )
{
    SerialDbg_Sout( SERD_OTYPE_16, (char *)pstr, 0, val );
}

void SER_Print16N( const char *pstr, u16 val )
{
    SerialDbg_Sout( SERD_OTYPE_16N, (char *)pstr, 0, val );
}

void SER_Print8( const char *pstr, u8 val )
{
    SerialDbg_Sout( SERD_OTYPE_8, (char *)pstr, 0, (u16)val );
}

void SER_Print8N( const char *pstr, u8 val )
{
    SerialDbg_Sout( SERD_OTYPE_8N, (char *)pstr, 0, (u16)val );
}

