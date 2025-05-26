//trig.h
//Table-based trig functions
//Bryan E. Topp <betopp@betopp.com> 2024
#ifndef TRIG_H
#define TRIG_H

#include "common.h"

#define TRIG_PI FV(3.1415926535)

//Sine, radians as input
fix24p8_t trig_sinr(fix24p8_t a);

//Cosine, radians as input
fix24p8_t trig_cosr(fix24p8_t a);

//Sine, degrees as input
fix24p8_t trig_sind(fix24p8_t a);

//Cosine, degrees as input
fix24p8_t trig_cosd(fix24p8_t a);

#endif //TRIG_H

