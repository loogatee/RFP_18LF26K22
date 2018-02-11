#ifndef SERIALOUTP_H
#define	SERIALOUTP_H




#define SERO_TYPE_ONECHAR    0x01
#define SERO_TYPE_ROM        0x02
#define SERO_TYPE_RAM        0x03
#define SERO_TYPE_16         0x04
#define SERO_TYPE_16N        0x05
#define SERO_TYPE_8          0x06
#define SERO_TYPE_8N         0x07


#define SER_COMPLETION_BUSY  0
#define SER_COMPLETION_OK    1



void SerialOutp_Init(void);
void SerialOutp_Process(void);


void SER_PrintCH(char ch);
void SER_PrintSTR(const char *pstr);
void SER_PrintSTRram(char *pstr);
void SER_Print16(const char *pstr, u16 val);
void SER_Print16N(const char *pstr, u16 val);
void SER_Print8(const char *pstr,  u8 val);
void SER_Print8N(const char *pstr,  u8 val);
void SER_Send(u8 otype, char *sptr, u8 *completionptr, u16 aval);



#endif
