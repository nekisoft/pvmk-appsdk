//trig.c
//Table-based trig functions
//Bryan E. Topp <betopp@betopp.com> 2024

#include "trig.h"

#include <math.h> //temp

fix24p8_t trig_sinr(fix24p8_t a)
{
	return trig_cosr(a + (TRIG_PI / 2));
}

fix24p8_t trig_cosr(fix24p8_t a)
{
	if(a < 0)
		a *= -1;
	
	//temp
	return FV(cosf( (float)a / (float)FV(1) ));
}

fix24p8_t trig_sind(fix24p8_t a)
{
	return trig_cosd(a + FV(90));
}

fix24p8_t trig_cosd(fix24p8_t a)
{
	if(a < 0)
		a *= -1;
	
	return trig_cosr(a * TRIG_PI / FV(180));
}
