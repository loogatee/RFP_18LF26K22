#ifndef SERIALDBG_H
#define	SERIALDBG_H




#define SERD_OTYPE_ONECHAR    0x01
#define SERD_OTYPE_ROM        0x02
#define SERD_OTYPE_RAM        0x03
#define SERD_OTYPE_16         0x04
#define SERD_OTYPE_16N        0x05
#define SERD_OTYPE_8          0x06
#define SERD_OTYPE_8N         0x07





void SerialDbg_Init( void );
void SerialDbg_ProcessOutput( void );
void SerialDbg_ProcessInput ( void );
void SerialDbg_SignalCmdDone( void );

void SER_PrintCH    ( char ch );
void SER_PrintSTR   ( const char *pstr );
void SER_PrintSTRram( char *pstr );
void SER_Print16    ( const char *pstr, u16 val );
void SER_Print16N   ( const char *pstr, u16 val );
void SER_Print8     ( const char *pstr,  u8 val );
void SER_Print8N    ( const char *pstr,  u8 val );
void SerialDbg_Sout ( u8 otype, char *sptr, u8 *completionptr, u16 aval );



#endif
