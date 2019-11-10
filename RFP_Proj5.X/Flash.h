#ifndef FLASH_H
#define FLASH_H

#include "i2c.h"

#define FLASH_COMPLETION_BUSY       I2C_COMPLETION_BUSY
#define FLASH_COMPLETION_OK         I2C_COMPLETION_OK
#define FLASH_COMPLETION_TIMEOUT    I2C_COMPLETION_TIMEOUT


void  Flash_Init(void);
void  Flash_GetMem16(UW2B addr);
u8    Flash_ShowMem16(void);
void  Flash_Write8( UW2B addr, u8 *TA );
u8    Flash_WriteComplete( void );
void  Flash_Erase8( UW2B addr );
u8    Flash_EraseComplete( void );


#endif