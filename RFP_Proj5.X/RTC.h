#ifndef RTC_H
#define RTC_H

#include "i2c.h"

#define RTC_COMPLETION_BUSY       I2C_COMPLETION_BUSY
#define RTC_COMPLETION_OK         I2C_COMPLETION_OK
#define RTC_COMPLETION_TIMEOUT    I2C_COMPLETION_TIMEOUT


void  RTC_Init(void);
void  RTC_GetTime(void);
void  RTC_SetTime(u8 *TA);
void  RTC_SetTime_Canned(void);
u8    RTC_ShowTime(void);
u8    RTC_SetComplete(void);
void  RTC_ShowTime_Loop(void);




#endif