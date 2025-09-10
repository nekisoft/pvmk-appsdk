//trigfunc.h
//Fixed-point trigonometry
//Bryan E. Topp <betopp@betopp.com> 2025
#ifndef TRIGFUNC_H
#define TRIGFUNC_H

#include <stdint.h>

//Returns 0.8 sine of 0.16 angle (65536 = 360 degrees)
int32_t trigfunc_sin8(int32_t input_ang16);

//Returns 0.8 cosine of 0.16 angle (65536 = 360 degrees)
int32_t trigfunc_cos8(int32_t input_ang16);

#endif //TRIGFUNC_H
