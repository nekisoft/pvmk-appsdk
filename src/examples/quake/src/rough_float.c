//rough_float.c
//Rough float routines for machines lacking FPU
//Bryan E. Topp <betopp@betopp.com> 2025

#include <stdint.h>
#include <stdio.h>

//Rough floating point type - processing with integer ops
typedef union rf_u
{
	float f;
	uint32_t i;
	struct { //Depends on the C compiler assigning bits LSBit-first
		uint32_t ma : 23; //Mantissa
		uint32_t ex : 8;  //Exponent
		uint32_t sg : 1;  //Sign
	} p;
} rf_t;

float rf_add(float a, float b)
{
	return a+b;
}

float rf_sub(float a, float b)
{
	return a-b;
}

float rf_mul(float a, float b)
{
	rf_t ra = { .f = a };
	rf_t rb = { .f = b };

	//Add the exponents
	int expsum = ((ra.i & 0x7F800000u) >> 23) + ((rb.i & 0x7F800000u) >> 23); //add biased exponents
	expsum -= 0x7F; //remove one of the biases
	if(expsum < 20) //flush-to-zero if it's even close
		return 0; 
		
	
	//Multiply the mantissa bits
	uint32_t ma = ((ra.i & 0x7FFFFF) | 0x800000u) >> 8; //1.15
	uint32_t mb = ((rb.i & 0x7FFFFF) | 0x800000u) >> 8; //1.15
	uint32_t manprod = ma * mb; //2.30

	manprod >>= 7; //cut down to 23 bits of fraction
	if(manprod >= 0x1000000u) //if we end up with more than "1" as our integer part, shift down
	{
		manprod >>= 1;
		expsum++;
	}
	if(manprod < 0x800000u) //if we end up with less than "1" as our integer part, shift up
	{
		manprod <<= 1;
		expsum--;
	}
	manprod &= 0x7FFFFFu; //whole part is now 1, will be implied, cut it off
	
	//Build the result
	rf_t rr = { .p = { .sg = ra.p.sg ^ rb.p.sg, .ma = manprod, .ex = expsum } };
	return rr.f;
}

float rf_sq(float a)
{
	rf_t ra = { .f = a };

	int expsum = ((ra.i & 0x7F800000u) >> 22); //Double the exponent
	expsum -= 0x7F; //remove one of the biases
	if(expsum < 20) //flush-to-zero if it's even close
		return 0; 
		
	//Multiply the mantissa bits
	uint32_t ma = ((ra.i & 0x7FFFFF) | 0x800000u) >> 8; //1.15
	uint32_t manprod = ma * ma; //2.30

	manprod >>= 7; //cut down to 23 bits of fraction
	if(manprod >= 0x1000000u) //if we end up with more than "1" as our integer part, shift down
	{
		manprod >>= 1;
		expsum++;
	}
	if(manprod < 0x800000u) //if we end up with less than "1" as our integer part, shift up
	{
		manprod <<= 1;
		expsum--;
	}
	manprod &= 0x7FFFFFu; //whole part is now 1, will be implied, cut it off
	
	//Build the result
	rf_t rr = { .p = { .sg = 0, .ma = manprod, .ex = expsum } };
	return rr.f;	
}



//Test routine
#if ROUGH_FLOAT_TEST
int main(void)
{
	float vals[] = { 4.5f, 88.0f, 0.0f, 1.0f, 999.0f, 32.0f, 1.5f, 777.7777f, -666.666f };
	for(int ai = 0; vals[ai] != -666.666f; ai++)
	{
		for(int bi = 0; vals[bi] != -666.666f; bi++)
		{
			float test_a = vals[ai];
			float test_b = vals[bi];
			printf("test a=%f b=%f\n", test_a, test_b);
			
			float mul_ref = test_a * test_b;
			float mul_our = rf_mul(test_a, test_b);
			printf("\ta*b=%f approx=%f\n", mul_ref, mul_our);
			
			float dev = fabs( (mul_our - mul_ref)/mul_ref );
			printf("\tdev=%f%%\n", dev*100);
		}
	}
	
	return 0;
}
#endif