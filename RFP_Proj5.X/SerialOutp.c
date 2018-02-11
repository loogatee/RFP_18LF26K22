#include "pic18lf26k22.h"
#include "PICTypes.h"
#include "SerialOutp.h"
#include "Misc.h"
#include "Cmds.h"



#define SERO_STATE_GETJOB      0
#define SERO_STATE_DOCHARS     1

#define SERO_SQENTRYS          34




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
static u8     serd_ostate_machine;             //   holds state of Serial Output machine
static SERI  *serd_active_item;                //   pointer to currently active item
static SERI   serd_data_items[SERO_SQENTRYS];  //   Serial Job Data Items
static char   serd_databuf[7];                 //   data buffer for value conversion






void SerialOutp_Init( void )
{
    serd_num_qitems     = 0;
    serd_inn_qindex     = 0;
    serd_out_qindex     = 0;
    serd_ostate_machine = SERO_STATE_GETJOB;
}


//---------------------------------------------------------------------------------------
//  Queues up a Job on the 'Serial Jobs Output List' (Implemented as a Circular Q)
//  This is the 'Front End' or Producer of Serial Data.   The idea is to stash the
//  parameters quickly in a Queue, then let the 'Back End' (the Consumer) process the
//  data off the Queue, and perform the printing.
//  The data is destined for EUSART1.
//
//  Parameters:
//
//        otype - The type of output requested.  see SERO_TYPE_ defines
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
void SER_Send( u8 otype, char *sptr, u8 *completionptr, u16 aval )
{
    SERI  *lqitem;                                                              // Pointer to Array Element where the data will go

    if( serd_num_qitems != SERO_SQENTRYS )                                      // Proceed if the Queue is not full
    {
        lqitem = &serd_data_items[serd_inn_qindex];                             // item will go in at index serd_inn_qindex
        ++serd_num_qitems;                                                      // Queue size is increased by 1
        if( ++serd_inn_qindex == SERO_SQENTRYS ) { serd_inn_qindex = 0; }       // where the next item will go:  wrap if necessary

        lqitem->sr_dval.w  = aval;                                              // aval goes into element on the Q
        lqitem->sr_otype   = otype;                                             // otype goes into element on the Q
        lqitem->sr_sptr    = sptr;                                              // sptr goes into element on the Q
        lqitem->sr_compPtr = completionptr;                                     // completionptr goes into element on the Q
        
        if( completionptr != 0 ) { *completionptr = 0; }                        // if pointer is valid, store 0, indicating Not Done
    }
                                                                                // Else the Q is full.  Effectively tosses the data
}



void SerialOutp_Process( void )
{
    char  end_of_string;
    u8    lotype;

    switch( serd_ostate_machine )
    {
    case SERO_STATE_GETJOB:                                                             // Looking for items on the Queue

        if( serd_num_qitems == 0 ) { return; }                                          // return IMMEDIATELY if Queue is empty

        serd_active_item   = &serd_data_items[serd_out_qindex];                         // Item to operate on is at 'serd_out_qindex'
        serd_ostate_machine = SERO_STATE_DOCHARS;                                      // switch state to 'Processing Characters'

        switch( serd_active_item->sr_otype )                                            // sr_otype dictates the action
        {
        case SERO_TYPE_ONECHAR:   serd_databuf[0] = serd_active_item->sr_dval.b[0];     // Character to print is placed in databuf
                                  serd_databuf[1] = 0;                                  // string terminator
                                  serd_active_item->sr_sptr = serd_databuf;             // sptr point to the data in databuf
                                  break;
                                    
        case SERO_TYPE_16:        ItoH(serd_active_item->sr_dval.w, serd_databuf);       // 8-bit to Hex Data Conversion.  Place in databuf
                                  serd_databuf[4] = ASCII_CARRIAGERETURN;                // tack on <CR> at the end
                                  serd_databuf[5] = ASCII_LINEFEED;                      // tack on <LF> at the end
                                  serd_databuf[6] = 0;                                   // this is the string terminator
                                  break;
                                  
        case SERO_TYPE_16N:       ItoH(serd_active_item->sr_dval.w, serd_databuf);       // 8-bit to Hex Data Conversion.  Place in databuf
                                  break;
                                    
        case SERO_TYPE_8:         BtoH(serd_active_item->sr_dval.b[0], serd_databuf);    // 8-bit to Hex Data Conversion.  Place in databuf
                                  serd_databuf[2] = ASCII_CARRIAGERETURN;                // tack on <CR> at the end
                                  serd_databuf[3] = ASCII_LINEFEED;                      // tack on <LF> at the end
                                  serd_databuf[4] = 0;                                   // this is the string terminator
                                  break;
                                  
        case SERO_TYPE_8N:        BtoH(serd_active_item->sr_dval.b[0], serd_databuf);    // 8-bit to Hex Data Conversion.  Place in databuf
                                  break;                          
        }
        
        return;                                                                          // could FALL_THRU, but don't be a CPU hog!


    case SERO_STATE_DOCHARS:                                                        // Actively printing out characters

        if( PIC_REGISTER TXSTA1bits.TRMT == 0 ) { return; }                         // return IMMEDIATELY if TRMT indicates busy

        lotype = serd_active_item->sr_otype;                                        // local assign for optimization
        if( lotype == SERO_TYPE_RAM )                                               // Characters coming from RAM ?
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
            if( lotype >= SERO_TYPE_16 )                                            // IF any of these types, still stuff to print
            {                                                                       //    ordering in SERD_OTYPE is important!!
                serd_active_item->sr_otype = SERO_TYPE_RAM;                         // change type to RAM
                serd_active_item->sr_sptr  = (char *)serd_databuf;                  // data is in serd_databuf (which is in RAM)
            }
            else                                                                    // ELSE this print job is done
            {
                serd_ostate_machine = SERO_STATE_GETJOB;                            // switch state:  look for another job
                --serd_num_qitems;                                                  // Can now decrement Queue size by 1
                if( ++serd_out_qindex == SERO_SQENTRYS ) { serd_out_qindex = 0; }   // index to next element in the Circular Q.  Wrap if necessary

                if( serd_active_item->sr_compPtr != 0 )                             // Is there a valid Completion Pointer ?
                    *serd_active_item->sr_compPtr = 1;                              //    signal a 1 to that address to indicate completion
            }
        }
    }
}









void SER_PrintCH( char ch )
{
    SER_Send( SERO_TYPE_ONECHAR, 0, 0, (u16)ch );
}

void SER_PrintSTR( const char *pstr )
{
    SER_Send( SERO_TYPE_ROM, (char *)pstr, 0, 0 );
}

void SER_PrintSTRram( char *pstr )
{
    SER_Send( SERO_TYPE_RAM, (char *)pstr, 0, 0 );
}

void SER_Print16( const char *pstr, u16 val )
{
    SER_Send( SERO_TYPE_16, (char *)pstr, 0, val );
}

void SER_Print16N( const char *pstr, u16 val )
{
    SER_Send( SERO_TYPE_16N, (char *)pstr, 0, val );
}

void SER_Print8( const char *pstr, u8 val )
{
    SER_Send( SERO_TYPE_8, (char *)pstr, 0, (u16)val );
}

void SER_Print8N( const char *pstr, u8 val )
{
    SER_Send( SERO_TYPE_8N, (char *)pstr, 0, (u16)val );
}

