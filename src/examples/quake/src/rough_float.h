//rough_float.h
//Rough float routines for machines lacking FPU
//Bryan E. Topp <betopp@betopp.com> 2025
#ifndef ROUGH_FLOAT_H
#define ROUGH_FLOAT_H

//Replacements for aeabi float functions, but shittier
float rf_add(float a, float b);
float rf_sub(float a, float b);
float rf_mul(float a, float b);
float rf_sq(float a);

#endif //ROUGH_FLOAT_H
