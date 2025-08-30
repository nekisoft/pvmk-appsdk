//statfunc.c
//Statistics helpers
//Bryan E. Topp <betopp@betopp.com> 2025

#include "statfunc.h"
#include <stdlib.h>

uint8_t statfunc_gauss_8b(uint8_t mean, uint8_t stdev)
{
	int accum = 0;
	for(int tt = 0; tt < 256; tt++)
	{
		accum += statfunc_rand_8b();
	}
	accum -= 32768;
	accum *= stdev * 64;
	accum += mean * 65536;
	if(accum < 0)
		return 0;
	if(accum / 65536 > 255)
		return 255;
	
	return accum / 65536;
}

uint8_t statfunc_rand_8b(void)
{
	return statfunc_rand_32b();
}

uint32_t statfunc_rand_32b(void)
{
	uint32_t accum = 0;
	accum += rand();
	accum -= rand();
	accum += rand();
	accum -= rand();
	return accum;
}
