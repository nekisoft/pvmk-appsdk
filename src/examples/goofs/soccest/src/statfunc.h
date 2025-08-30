//statfunc.h
//Statistics helpers
//Bryan E. Topp <betopp@betopp.com> 2025
#ifndef STATFUNC_H
#define STATFUNC_H

#include <stdint.h>

//Rolls gaussian probability, 8-bit ranges
uint8_t statfunc_gauss_8b(uint8_t mean, uint8_t stdev);

//Rolls pure random, 8-bit range
uint8_t statfunc_rand_8b(void);

//Rolls pure random, 32-bit range
uint32_t statfunc_rand_32b(void);

#endif //STATFUNC_H
