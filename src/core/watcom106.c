
#include "drally.h"

// not static: the race replay (race_ticklog.c) saves and restores it
unsigned int watcom106_seed = 1;

void srand_watcom106(unsigned int seed){

    watcom106_seed = seed;
}

DR_HOT int rand_watcom106(void){

    watcom106_seed *= 0x41c64e6d;
    watcom106_seed += 0x3039;
 
	return (watcom106_seed>>0x10)&0x7fff;
}

char * strupr_watcom106(char * s){

    char *  p;

    p = s-1;
    while(++p&&*p){

        if((unsigned char)(*p-0x61) <= 0x19) *p -= 0x20;
    }

    return s;
}

char * strlwr_watcom106(char * s){

    char *  p;

    p = s-1;
    while(++p&&*p){

        if((unsigned char)(*p-0x41) <= 0x19) *p += 0x20;
    }

    return s;
}

char * utoa_watcom106(int value, char * str, int radix){

    char    __buff[0x28];
    char *  buff;
    char *  p;

    const char ___Alphabet[] = "0123456789abcdefghijklmnopqrstuvwxyz";

    p = str;
    buff = __buff;
	*buff++ = 0;

    if(radix > 1){
        
        while(value||!*(buff-1)){

            *buff++ = ___Alphabet[value%radix];
            value /= radix;
        }
    }

    while((*p++ = *--buff));

	return str;
}

char * itoa_watcom106(int value, char * str, int radix){

    if((radix == 0xa)&&(value < 0)){

        *str = '-';
        return utoa_watcom106(-value, str+1, radix)-1;
    }

	return utoa_watcom106(value, str, radix);
}
