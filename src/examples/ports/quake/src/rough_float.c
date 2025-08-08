//rough_float.c
//Rough float routines for machines lacking FPU
//Bryan E. Topp <betopp@betopp.com> 2025

#include <stdint.h>
#include <stdio.h>

//moved all implementations to rough_float.h so they can be inlined more easily

//Test routine
#if ROUGH_FLOAT_TEST
#include <math.h>
int main(void)
{
	float vals[] = { 4.5f, -88.0f, 0.0f, 1.0f, 999.0f, 32.0f, 1.5f, 777.7777f, -666.666f };
	for(int ai = 0; vals[ai] != -666.666f; ai++)
	{
		for(int bi = 0; vals[bi] != -666.666f; bi++)
		{
			float test_a = vals[ai];
			float test_b = vals[bi];
			printf("test a=%f b=%f\n", test_a, test_b);
			
/*
			float mul_ref = test_a * test_b;
			float mul_our = rf_mul(test_a, test_b);
			printf("\ta*b=%f approx=%f\n", mul_ref, mul_our);

			float dev = fabs( (mul_our - mul_ref)/mul_ref );
			printf("\tdev=%f%%\n", dev*100);
			
			
			float sq_ref = test_a * test_a;
			float sq_our = rf_sq(test_a);
			printf("\ta*a=%f approx=%f\n", sq_ref, sq_our);
			
			float sdev = fabs( (sq_our - sq_ref)/sq_ref );
			printf("\tdev=%f%%\n", sdev*100);
*/			
			
			float sum_ref = test_a + test_b;
			float sum_our = rf_add(test_a, test_b);
			printf("\ta+b=%f approx=%f\n", sum_ref, sum_our);
			
			float pdev = fabs( (sum_our - sum_ref)/sum_ref );
			printf("\tdev=%f%%\n", pdev*100);
			
			float dif_ref = test_a - test_b;
			float dif_our = rf_sub(test_a, test_b);
			printf("\ta-b=%f approx=%f\n", dif_ref, dif_our);
			
			float ddev = fabs( (dif_our - dif_ref)/dif_ref );
			printf("\tdev=%f%%\n", ddev*100);
		}
	}
	
	return 0;
}
#endif
