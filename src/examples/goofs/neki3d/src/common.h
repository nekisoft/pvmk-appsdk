//common.h
//Common type definitions
//Bryan E. Topp <betopp@betopp.com> 2024
#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>

//Fixed-point math type - 24.8 bits of precision
typedef int fix24p8_t;
#define FV(floatval) ((fix24p8_t)(256 * floatval))


#endif //COMMON_H
