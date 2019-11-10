#include "pic18lf26k22.h"
#include "PICTypes.h"
#include "Timer.h"
#include "i2c.h"
#include "SerialOutp.h"
#include <string.h>



#define I2C_1_QENTRYS   5
#define TIMEOUT_VAL     35


#define RTN_CONTINUE    0
#define RTN_RESTART     1
#define RTN_SUCCESS     2


#define I2C_STATE_GETJOB                 0
#define I2C_STATE_TRY_RESTART            1
#define I2C_STATE_SENDSTART              2
#define I2C_STATE_START_COMPLETE         3
#define I2C_STATE_WRITE_SLAVEADDR        4
#define I2C_STATE_WRITE_CMDREG           5
#define I2C_STATE_WRITE_CMDREG2          6
#define I2C_STATE_WAIT_WAIT              7
#define I2C_STATE_DO_RSEN                8
#define I2C_STATE_WRITE_SLAVEADDR_RW     9
#define I2C_STATE_READ_THE_DATA          10
#define I2C_STATE_WAIT_STOP              11
#define I2C_STATE_DO_WRITEONLY           12

#define I2C_WRITE_SUBSTATE_START         0
#define I2C_WRITE_SUBSTATE_WAITCOMPLETE  1
#define I2C_WRITE_SUBSTATE_IDLE_ACK      2
#define I2C_WRITE_SUBSTATE_SUCCESS       3
#define I2C_WRITE_SUBSTATE_NEED_RESTART  4

#define I2C_READ_SUBSTATE_START          0
#define I2C_READ_SUBSTATE_BF             1
#define I2C_READ_SUBSTATE_WAIT_RCEN      2
#define I2C_READ_SUBSTATE_DO_RESTART     3
#define I2C_READ_SUBSTATE_ACKEN          4
#define I2C_READ_SUBSTATE_SUCCESS        5


typedef struct
{
    u8       *ic_dptr;            // destination for Read Data
    u8       *ic_wdata;           // Source ptr to Write Data
    u8       *ic_complptr;        // completion Ptr
    I2CCMDS  *ic_cmdptr;          // Ptr to the Job List
} II2C;




static II2C     i2c_1_data[I2C_1_QENTRYS];
static u8       i2c_1_num_qitems;
static u8       i2c_1_inn_qindex;
static u8       i2c_1_out_qindex;
static II2C    *i2c_1_activeitem;
static u8       i2c_1_state;
static u8       i2c_1_starttime;
static u8      *i2c_1_lcdptr;
static u8      *i2c_1_lcwdata;
static u8       i2c_1_lcnumbytes;
static u8       i2c_1_lcslave;
static u8       i2c_1_lccmd;
static u8       i2c_1_lccmd2;
static u8       i2c_1_lccmdtype;
static I2CCMDS *i2c_1_lclistptr;
static bool     i2c_1_useCmdReg2;

static u8       i2c_1_write_substate;
static u8       i2c_1_write_data;
static u8       i2c_1_read_substate;



static u8  i2c_1_do_write_substate( void );
static u8  i2c_1_do_read_substate ( void );




void I2C_1master_Init( void )
{
    memset( (void *)i2c_1_data, 0, sizeof(i2c_1_data) );

    i2c_1_num_qitems = 0;
    i2c_1_inn_qindex = 0;
    i2c_1_out_qindex = 0;
      
    i2c_1_state = I2C_STATE_GETJOB;
}



bool I2C_1master_SendCmd( u8 *dptr, u8 *wptr, u8 *complptr, I2CCMDS *listptr )
{
    II2C  *lqitem;
    bool   retv = FALSE;

    if( i2c_1_num_qitems != I2C_1_QENTRYS )
    {
        retv   = TRUE;
        lqitem = &i2c_1_data[i2c_1_inn_qindex];
        ++i2c_1_num_qitems;
        if( ++i2c_1_inn_qindex == I2C_1_QENTRYS ) { i2c_1_inn_qindex = 0; }

        lqitem->ic_dptr     = dptr;
        lqitem->ic_wdata    = wptr;
        lqitem->ic_complptr = complptr;
        lqitem->ic_cmdptr   = listptr;
        
        if( complptr ) { *complptr = I2C_COMPLETION_BUSY; }
    }

    return retv;
}



void I2C_1master_Process( void )
{
    u8  tmpb;

    if( i2c_1_state != I2C_STATE_GETJOB )                                         // If the state machine is active
    {
        if( TIM_GetDeltaB( i2c_1_starttime ) >= TIMEOUT_VAL )                     // Safety valve: too long to complete this job ?
        {
            PIC_REGISTER SSP1CON2bits.PEN = 1;                                    //     Send Stop!   Terminates I2C protocol machine

            SER_Print8( "TO! i2c_1_state: ", i2c_1_state );                       //     Message out to debug port (optional)
            SER_Print8( "SSP1CON2: ", PIC_REGISTER SSP1CON2 );                    //     Message out to debug port (optional)

            i2c_1_state = I2C_STATE_GETJOB;                                       //     state machine transitions to:  Looking For New Job
            --i2c_1_num_qitems;                                                   //     Item is no longer on the Q, total number decreases by 1
            if( ++i2c_1_out_qindex == I2C_1_QENTRYS ) { i2c_1_out_qindex = 0; }   //     Where next item will come from.  adjusts to 0 if wrapped
            
            if( i2c_1_activeitem->ic_complptr != 0 )                              //     Is there a valid Completion Pointer ?
                *i2c_1_activeitem->ic_complptr = I2C_COMPLETION_TIMEOUT;          //         signal a non-zero to that address to indicate completion

            return;                                                               //     return immediately
        }
    }

    switch( i2c_1_state )
    {
    case I2C_STATE_GETJOB:

        if( i2c_1_num_qitems == 0 ) { return; }                                   // If nothing on the Q, jump out

        i2c_1_activeitem     = &i2c_1_data[i2c_1_out_qindex];                     // Got something. ActiveItem is at out_qindex
        i2c_1_lclistptr      = i2c_1_activeitem->ic_cmdptr;                       // makes local copy of the Command List
        i2c_1_starttime      = TIM_GetTickB();                                    // Initialize the "safety valve" ticker
        i2c_1_write_substate = I2C_WRITE_SUBSTATE_START;                          // Make sure write_substate is initialized for each job
        i2c_1_read_substate  = I2C_READ_SUBSTATE_START;                           // Make sure read_substate is initialized for each job
        FALL_THRU;                                                                // Keep on going, no need to break

    case I2C_STATE_TRY_RESTART:

        i2c_1_lcslave    = i2c_1_lclistptr->ct_slaveaddr;                         // Local Copy, Slave Address
        i2c_1_lccmd      = i2c_1_lclistptr->ct_cmdreg;                            // Local Copy, Command Register
        i2c_1_lccmd2     = i2c_1_lclistptr->ct_cmdreg2;                           // Local Copy, Command Register #2
        i2c_1_lcnumbytes = i2c_1_lclistptr->ct_numbytes;                          // Local Copy, Number of Bytes
        i2c_1_lccmdtype  = i2c_1_lclistptr->ct_cmdtype;                           // Local Copy, Command Type
        i2c_1_useCmdReg2 = FALSE;
        
        if( i2c_1_lccmdtype & I2C_CMDTYPE_CMDREG2 )
        {
            i2c_1_lccmdtype &= ~I2C_CMDTYPE_CMDREG2;
            i2c_1_useCmdReg2 = TRUE;
        }
        
        i2c_1_lcdptr     = i2c_1_activeitem->ic_dptr;                             // Local Copy, Data Pointer
        i2c_1_lcwdata    = i2c_1_activeitem->ic_wdata;                            // Local Copy, Pointer to Write Data
        i2c_1_state      = I2C_STATE_SENDSTART;                                   // transition to SENDSTART
        FALL_THRU;                                                                // no need to break

    case I2C_STATE_SENDSTART:

        if((( PIC_REGISTER SSP1CON2 & 0x1F ) | ( PIC_REGISTER SSP1STATbits.R_W )) != 0 ) { return; }    // Must wait for IDLE

        i2c_1_state = I2C_STATE_START_COMPLETE;                                   // Have IDLE, new state will be START_COMPLETE
        PIC_REGISTER SSP1CON2bits.SEN = 1;                                        // START!
        FALL_THRU;                                                                // no need to break

    case I2C_STATE_START_COMPLETE:

        if( PIC_REGISTER SSP1CON2bits.SEN != 0 ) { return; }                      // Looking for SEN to transition to 0

        i2c_1_state      = I2C_STATE_WRITE_SLAVEADDR;                             // Got it!  Transition to WRITE_SLAVEADDR
        i2c_1_write_data = i2c_1_lcslave;                                         // This is the byte which will be written
        FALL_THRU;                                                                // no need to break

    case I2C_STATE_WRITE_SLAVEADDR:
            
        if( (tmpb = i2c_1_do_write_substate()) == RTN_CONTINUE ) { return; }      // Keep calling write_substate while return value says to CONTINUE
            
        if( tmpb == RTN_RESTART )                                                 // Test for the RESTART return value
        {                                                                         //    RESTART indicates some error was detected
            i2c_1_state = I2C_STATE_TRY_RESTART;                                  //    Change state machine to TRY_RESTART.  Means 'try again'
            return;                                                               //    Immediately jump out
        }
                                                                                  // ELSE return indicates SUCCESS!
        i2c_1_state      = I2C_STATE_WRITE_CMDREG;                                //    new state is WRITE_CMDREG
        i2c_1_write_data = i2c_1_lccmd;                                           //    this is the byte which will be written
   //SER_Print8( "CMDREG1: ", i2c_1_lccmd );
        FALL_THRU;                                                                //    no need to break

    case I2C_STATE_WRITE_CMDREG:

        if( (tmpb = i2c_1_do_write_substate()) == RTN_CONTINUE ) { return; }      // Keep calling write_substate while return value says to CONTINUE

        if( tmpb == RTN_RESTART )                                                 // Test for the RESTART return value
        {                                                                         //    RESTART indicates some error was detected
            i2c_1_state = I2C_STATE_TRY_RESTART;                                  //    Change state machine to TRY_RESTART.  Means 'try again'
            return;                                                               //    Immediately jump out
        }
                                                                                  // ELSE return indicates SUCCESS!
        if( i2c_1_useCmdReg2 == FALSE )
        {
            if( i2c_1_lccmdtype == I2C_CMDTYPE_WRITEONLY )                        // Check to see if this job is WRITE-ONLY
            {                                                                     //    Means nothing to read, need only to write some registers
                i2c_1_state          = I2C_STATE_DO_WRITEONLY;                    //    new state is DO_WRITEONLY
                i2c_1_write_data     = *i2c_1_lcwdata ++;                         //    Get the byte to write, and increment pointer thru this list
                return;                                                           //    return immediately
            }
            
            i2c_1_state = I2C_STATE_DO_RSEN;                                      // ELSE gonna do a read, new state is DO_RSEN
            PIC_REGISTER SSP1CON2bits.RSEN = 1;                                   // Bang!  Hit the RSEN bit.
        }
        else
        {
            i2c_1_state      = I2C_STATE_WRITE_CMDREG2;
            i2c_1_write_data = i2c_1_lccmd2;
            return;
    //SER_Print8( "CMDREG2: ", i2c_1_lccmd2 );        
        }

        FALL_THRU;                                                                // no need to break
        
    case I2C_STATE_WRITE_CMDREG2:

        if( i2c_1_state == I2C_STATE_WRITE_CMDREG2 )
        {
            if( (tmpb = i2c_1_do_write_substate()) == RTN_CONTINUE ) { return; }  // Keep calling write_substate while return value says to CONTINUE

            if( tmpb == RTN_RESTART )                                             // Test for the RESTART return value
            {                                                                     //    RESTART indicates some error was detected
                i2c_1_state = I2C_STATE_TRY_RESTART;                              //    Change state machine to TRY_RESTART.  Means 'try again'
                return;                                                           //    Immediately jump out
            }
                                                                                  // ELSE return indicates SUCCESS!
            if( i2c_1_lccmdtype == I2C_CMDTYPE_WRITEONLY )                        // Check to see if this job is WRITE-ONLY
            {                                                                     //    Means nothing to read, need only to write some registers
                i2c_1_state          = I2C_STATE_DO_WRITEONLY;                    //    new state is DO_WRITEONLY
                i2c_1_write_data     = *i2c_1_lcwdata ++;                         //    Get the byte to write, and increment pointer thru this list
          //SER_Print8( "WRITEONLY: ", i2c_1_write_data );
                return;                                                           //    return immediately
            }

            i2c_1_state = I2C_STATE_DO_RSEN;                                      // ELSE gonna do a read, new state is DO_RSEN
            PIC_REGISTER SSP1CON2bits.RSEN = 1;                                   // Bang!  Hit the RSEN bit.
        }
        FALL_THRU;            

    case I2C_STATE_DO_RSEN:

        if( PIC_REGISTER SSP1CON2bits.RSEN != 0 )                                 // Looking for RSEN transition to 0
        {
            if( PIC_REGISTER PIR2bits.BCL1IF != 0 )                               // not 0 yet, check for Bus Collision
            {
                PIC_REGISTER PIR2bits.BCL1IF  = 0;                                //     Bummer, got one.  Clear collision Bit
                PIC_REGISTER SSP1CON2bits.PEN = 1;                                //     Send STOP
                i2c_1_state = I2C_STATE_TRY_RESTART;                              //     Let's try again
            }
            return;                                                               // Did not receive RSEN=0, return a look again
        }

        i2c_1_state      = I2C_STATE_WRITE_SLAVEADDR_RW;                          // Got RSEN=0.  Awesome!  Transition the state
        i2c_1_write_data = i2c_1_lcslave | 1;                                     // IMPORTANT!!  The 'OR 1' turns the bus around, so Read is Enabled
        FALL_THRU;                                                                // no need to break

    case I2C_STATE_WRITE_SLAVEADDR_RW:

        if( (tmpb = i2c_1_do_write_substate()) == RTN_CONTINUE ) { return; }      // Keep calling write_substate while return value says to CONTINUE

        if( tmpb == RTN_RESTART )                                                 // Test for the RESTART return value
        {                                                                         //    RESTART indicates some error was detected
            i2c_1_state = I2C_STATE_TRY_RESTART;                                  //    Change state machine to TRY_RESTART.  Means 'try again'
            return;                                                               //    Immediately jump out
        }
                                                                                  // ELSE return indicates SUCCESS
        i2c_1_state = I2C_STATE_READ_THE_DATA;                                    // new state = READ_THE_DATA
        FALL_THRU;                                                                // Fall through, start the read immediately

    case I2C_STATE_READ_THE_DATA:

        if( (tmpb = i2c_1_do_read_substate()) == RTN_CONTINUE ) { return; }       // Keep calling read_substate while return value says to CONTINUE

        if( tmpb == RTN_RESTART )                                                 // Test for the RESTART return value
        {                                                                         //    RESTART indicates some error was detected
            i2c_1_state = I2C_STATE_TRY_RESTART;                                  //    Change state machine to TRY_RESTART.  Means 'try again'
            return;                                                               //    Immediately jump out
        }

        PIC_REGISTER SSP1CON2bits.PEN = 1;                                        // Got it!  Can now Stop I2C transaction
        i2c_1_state = I2C_STATE_WAIT_STOP;                                        // transition state to looking for PEN to transition
        FALL_THRU;                                                                // Fall thru and immediately check

    case I2C_STATE_WAIT_STOP:

        if( PIC_REGISTER SSP1CON2bits.PEN != 0) { return; }                       // Spin here until the transition on PEN occurs

        tmpb = i2c_1_lclistptr->ct_numbytes;                                      // save numbytes before incrementing to new command field
        i2c_1_lclistptr++;                                                        // will point to the new Command Field
        if( i2c_1_lclistptr->ct_slaveaddr != 0xff )                               // New command list is Valid if Non FF
        {
            if( i2c_1_lccmdtype == I2C_CMDTYPE_RW )                               // Valid!  is this a 'RW' command ?
                i2c_1_activeitem->ic_dptr += tmpb;                                //     YES:  increment dptr, where read data goes
            else                                                                  // Else it's a WRITEONLY Job
                i2c_1_activeitem->ic_wdata += tmpb;                               //     increment where write data is coming from

            i2c_1_starttime = TIM_GetTickB();                                     // Start timer for the new transaction
            i2c_1_state     = I2C_STATE_TRY_RESTART;                              // Let's hit it again!
            return;                                                               // Hop out of state machine, back in a flash
        }

        i2c_1_state = I2C_STATE_GETJOB;                                           // All Done, new state is: Look for Another Job on the list
        --i2c_1_num_qitems;                                                       // Active Jobs on the Queue is reduced by 1
        if( ++i2c_1_out_qindex == I2C_1_QENTRYS ) { i2c_1_out_qindex = 0; }       // Where next Active item will come from.  adjusts to 0 if wrapped
        
        if( i2c_1_activeitem->ic_complptr != 0 )                                  // Is there a valid Completion Pointer ?
            *i2c_1_activeitem->ic_complptr = I2C_COMPLETION_OK;                   //    signal a non-zero to that address to indicate completion
        return;                                                                   // Get out of the i2c thread now

    case I2C_STATE_DO_WRITEONLY:

        if( (tmpb = i2c_1_do_write_substate()) == RTN_CONTINUE ) { return; }      // Keep calling write_substate while return value says to CONTINUE

        if( tmpb == RTN_RESTART )                                                 // Test for the RESTART return value
        {                                                                         //    RESTART indicates some error was detected
            i2c_1_state = I2C_STATE_TRY_RESTART;                                  //    Change state machine to TRY_RESTART.  Means 'try again'
            return;                                                               //    Immediately jump out
        }

        if( --i2c_1_lcnumbytes == 0 )                                             // Have all the bytes been sent ?
        {
            PIC_REGISTER SSP1CON2bits.PEN = 1;                                    //     Yes!, Send STOP
            i2c_1_state = I2C_STATE_WAIT_STOP;                                    //     Transition state to WAIT_STOP
            return;                                                               //     hop out, returns to WAIT_STOP quickly
        }

        i2c_1_write_data = *i2c_1_lcwdata++;                                      // Assign the new byte to write, and increment through the list
   //SER_Print8( "wdata: ", i2c_1_write_data );        
        i2c_1_do_write_substate();                                                // kick off write state machine.  No worries about return val
    }
}




static u8 i2c_1_do_write_substate( void )
{
    volatile u8 dummy;

    switch( i2c_1_write_substate )
    {
    case I2C_WRITE_SUBSTATE_START:

        if((( PIC_REGISTER SSP1CON2 & 0x1F ) | ( PIC_REGISTER SSP1STATbits.R_W )) != 0 )        // Wait for IDLE
            return RTN_CONTINUE;

        dummy = PIC_REGISTER SSP1BUF;                                 // dummy read
        PIC_REGISTER SSP1BUF = i2c_1_write_data;                      // transmits 1 byte

        if( PIC_REGISTER SSP1CON1bits.WCOL != 0 )                     // write collision ?
        {
            dummy = PIC_REGISTER SSP1BUF;                             //    dummy read
            PIC_REGISTER SSP1CON1bits.WCOL = 0;                       //    clear collision
            return RTN_CONTINUE;                                      //    Stay in this state, will attempt write again
        }
        
        i2c_1_write_substate = I2C_WRITE_SUBSTATE_WAITCOMPLETE;       // Success on write, transition to next state
        FALL_THRU;

    case I2C_WRITE_SUBSTATE_WAITCOMPLETE:

        if( PIC_REGISTER SSP1STATbits.BF != 0 )                       // Looking for 'Buffer Full Status Bit' to be 0
        {
            if( PIC_REGISTER PIR2bits.BCL1IF != 0 )                   // Always check for bus collision, and if set:
            {
               PIC_REGISTER PIR2bits.BCL1IF  = 0;                     //       Clr collisionBit
               PIC_REGISTER SSP1CON2bits.PEN = 1;                     //       Send STOP!
               i2c_1_write_substate = I2C_WRITE_SUBSTATE_START;       //       Will re-send the byte
               return RTN_RESTART;                                    //       transition states for attempt to resend
            }
            return RTN_CONTINUE;                                      //  stay here, keep looking for BF to be 0
        }
        
        i2c_1_write_substate = I2C_WRITE_SUBSTATE_IDLE_ACK;           //  BF cleared!  Now look for IDLE ACK
        FALL_THRU;

    case I2C_WRITE_SUBSTATE_IDLE_ACK:

        if((( PIC_REGISTER SSP1CON2 & 0x1F ) | ( PIC_REGISTER SSP1STATbits.R_W )) != 0 )        // standard wait for IDLE
            return RTN_CONTINUE;

        if( PIC_REGISTER SSP1CON2bits.ACKSTAT != 0 )                  // 1 = Acknowledge was not received
        {
            PIC_REGISTER SSP1CON2bits.PEN = 1;                        //      Send STOP!
            i2c_1_write_substate = I2C_WRITE_SUBSTATE_START;          //      Will re-send the byte
            return RTN_RESTART;                                       //      transition states for attempt to resend
        }

        i2c_1_write_substate = I2C_WRITE_SUBSTATE_START;              // ACK good!  write_substate goes back to START
        break;                                                        // get out
    }
    
    return RTN_SUCCESS;                                               // write substate done, byte transfer succeeded
}




static bool i2c_1_bus_collision( void )
{
    if( PIC_REGISTER PIR2bits.BCL1IF != 0 )                           // perform check for Bus Collision
    {
       PIC_REGISTER PIR2bits.BCL1IF  = 0;                             //    Got one, clear the Indicator
       PIC_REGISTER SSP1CON2bits.PEN = 1;                             //    I2C Stop
       i2c_1_read_substate = I2C_READ_SUBSTATE_START;                 //    State machine will start from the beginning
       return TRUE;                                                   //    TRUE says:  "yes we have a bus collision"
    }
    return FALSE;                                                     // "no, there was not a bus collision"
}


//
//   64Mhz Oscillator, 400kHz I2C:  1 character read takes about 34.2us.   PIN toggle whenever RCEN=1, on scope
//
static u8 i2c_1_do_read_substate( void )
{
    switch( i2c_1_read_substate )
    {
    case I2C_READ_SUBSTATE_START:

        PIC_REGISTER SSP1CON2bits.RCEN = 1;                                   // Enables Receive Mode for I2C
        i2c_1_read_substate = I2C_READ_SUBSTATE_BF;                           // Transition the state machine
        FALL_THRU;                                                            // no need to break

    case I2C_READ_SUBSTATE_BF:

        if( PIC_REGISTER SSP1STATbits.BF == 0 )                               // will stay in this state until BF transitions to 1
        {            
            if( i2c_1_bus_collision() == TRUE )                               // Check for Bus Collision 
                return RTN_RESTART;                                           //     Got one, transition to RESTART
            else
                return RTN_CONTINUE;                                          //     No BF and No BCL, stay in state SUBSTATE_BF
        }

        i2c_1_read_substate = I2C_READ_SUBSTATE_WAIT_RCEN;                    // Got BF!  next state is WAIT_RCEN
       *i2c_1_lcdptr++      = PIC_REGISTER SSP1BUF;                           // Read the byte from SSP1BUF, store in the data pointer
        i2c_1_lcnumbytes--;                                                   // Decrements Number of Bytes received
        FALL_THRU;                                                            // OK to Fall through

    case I2C_READ_SUBSTATE_WAIT_RCEN:

        if( PIC_REGISTER SSP1CON2bits.RCEN != 0 ) { return RTN_CONTINUE; }    // spin in this state until RCEN=0: indicates Receive Idle

        if( i2c_1_bus_collision() == TRUE ) { return RTN_RESTART; }           // If Bus Collision here, transition to RESTART
        
        if( i2c_1_lcnumbytes == 0 )                                           // When num bytes is 0, the Job is finished
        {
            PIC_REGISTER SSP1CON2bits.ACKDT = 1;                              //    Done: Send Not Acknowledge
            if( i2c_1_lcslave == 0xA0 )
            {
                i2c_1_read_substate = I2C_READ_SUBSTATE_START;                //    We are:  state goes back to START
                return RTN_SUCCESS;                                           //    Indicates Read State Machine is complete
            }
        }
        else
        {
            PIC_REGISTER SSP1CON2bits.ACKDT = 0;                              //    Not done yet:  Send Acknowledge
        }

        PIC_REGISTER SSP1CON2bits.ACKEN = 1;                                  // Initiate Acknowledge sequence and xmit ACKDT bit
        i2c_1_read_substate = I2C_READ_SUBSTATE_ACKEN;                        // state changes.
        FALL_THRU;                                                            // OK to fall through

    case I2C_READ_SUBSTATE_ACKEN:

        if( PIC_REGISTER SSP1CON2bits.ACKEN != 0 ) { return RTN_CONTINUE; }   // Spin in this state until ACKEN=0, meaning ACK sequence idle 

        if( i2c_1_lcnumbytes == 0 )                                           // Check to see if we're done
        {
            i2c_1_read_substate = I2C_READ_SUBSTATE_START;                    //    We are:  state goes back to START
            return RTN_SUCCESS;                                               //    Indicates Read State Machine is complete
        }
                                                                              // ELSE there are still characters to read
        PIC_REGISTER SSP1CON2bits.RCEN = 1;                                   //    Initiate by poking RCEN
        i2c_1_read_substate            = I2C_READ_SUBSTATE_BF;                //    next state will be looking for transition on BF
        FALL_THRU;                                                            //    falls out of the switch (mainly to avoid compiler warning)
    }

    return RTN_CONTINUE;                                                      // only time switch statement is broken, send CONTINUE
}


