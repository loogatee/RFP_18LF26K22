#ifndef I2C_H
#define I2C_H



#define I2C_CMDTYPE_RW                   0
#define I2C_CMDTYPE_WRITEONLY            1
#define I2C_CMDTYPE_CMDREG2              0x80


#define I2C_COMPLETION_BUSY              0
#define I2C_COMPLETION_OK                1
#define I2C_COMPLETION_TIMEOUT           2


typedef struct
{
    u8      ct_slaveaddr;         // i2c Slave Address
    u8      ct_cmdreg;            // i2c Command Register
    u8      ct_cmdreg2;           // i2c 2nd Command Register 
    u8      ct_numbytes;          // Number of bytes to Read or Write
    u8      ct_cmdtype;           // I2C_CMDTYPE_xx
} I2CCMDS;




extern void I2C_1master_Init(void);
extern void I2C_1master_Process(void);
extern bool I2C_1master_SendCmd(u8 *dptr, u8 *wptr, u8 *complptr, I2CCMDS *lptr);


//extern void I2C_2master_Init(u8 baudrate);
//extern void I2C_2master_Process(void);
//extern bool I2C_2_SendCmd(u8 *dptr, u8 *wptr, u8 *complptr, I2CCMDS *lptr);


#endif

