#include "PICTypes.h"
#include <string.h>




static char hexv[] =  { '0', '1', '2', '3', '4', '5', '6', '7',
                        '8', '9' ,'A', 'B', 'C', 'D', 'E', 'F' };



void ItoH( u16 val, char *S )
{
    s8  k;

    for( k=3; k >= 0; --k )
    {
        S[k]   = hexv[val & 0xF];
        val  >>= 4;
    }

    S[4] = 0;
}

void BtoH( u8 val, char *S )
{
    s8 k;

    for( k=1; k >= 0; --k )
    {
        S[k]   = hexv[val & 0xF];
        val  >>= 4;
    }

    S[2] = 0;
}

void BtoHnz( u8 val, char *S )
{
    s8 k;

    for( k=1; k >= 0; --k )
    {
        S[k]   = hexv[val & 0xF];
        val  >>= 4;
    }
}


int AtoI( const char *p )
{
	int n,f;
    
    n=f=0;

    for( ; ; p++ )
    {
		switch( *p )
        {
		case ' ':
		case '\t':    continue;
		case '-':     f++;
		case '+':     p++;
		}
		break;
	}
    
	while( *p >= '0' && *p <= '9')
		n = n*10 + *p++ - '0';
    
	return(f ? -n : n);
}


u16 HtoU16( char *pstr )
{
    u8    i,j;
    u16   ch,sum;
    
    switch( strlen(pstr) )
    {
        case 1:   j = 0;  break;
        case 2:   j = 4;  break;
        default:  j = 8;  break;
    }

    for( i=0,sum=0; pstr[i] && i < 3; ++i,j-=4 )
    {
        ch = pstr[i];
        
        if     ( ch >= '0' && ch <= '9' ) ch -= '0';
        else if( ch >= 'A' && ch <= 'F' ) ch -= '7';
        else if( ch >= 'a' && ch <= 'f' ) ch -= 'W';
        else                              ch  = 0;

        sum = sum | ( ch << j );
    }
    return sum;
}
