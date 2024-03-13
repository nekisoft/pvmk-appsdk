//pvmkoslib/float.h
//Floating-point definitions for using picolibc
//Bryan E. Topp <betopp@betopp.com> 2024
#ifndef _FLOAT_H
#define _FLOAT_H

//None of this really matters because we don't support floating-point on this platform.

#define FLT_RADIX 2 

#define DECIMAL_DIG 10

#define FLT_MIN_10_EXP -37
#define DBL_MIN_10_EXP -37
#define LDBL_MIN_10_EXP -37 

#define FLT_MAX_EXP 128
#define DBL_MAX_EXP 1024
#define LDBL_MAX_EXP 1024

#define FLT_MAX_10_EXP (+37)
#define DBL_MAX_10_EXP (+37)
#define LDBL_MAX_10_EXP (+37) 

#define FLT_MAX (1E+37)
#define DBL_MAX (1E+37)
#define LDBL_MAX (1E+37) 

#define FLT_EPSILON (1E-5)
#define DBL_EPSILON (1E-9)
#define LDBL_EPSILON (1E-9) 

#define FLT_MIN (1E-37)
#define DBL_MIN (1E-37)
#define LDBL_MIN (1E-37) 

#endif //_FLOAT_H
