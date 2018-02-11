#ifndef PICTYPES_H
#define	PICTYPES_H

#define TRUE         1
#define FALSE        0

#define FALL_THRU
#define PIC_REGISTER




typedef signed char     s8;
typedef unsigned char   u8;
typedef int             s16;
typedef unsigned short  u16;
typedef long            s32;
typedef unsigned long   u32;
typedef unsigned char   bool;


union uW2B
{
    u16  w;
    u8   b[2];
};

union W4B
{
    u32   lw;
    u8    b[4];
};

typedef union uW2B   UW2B;







#endif

