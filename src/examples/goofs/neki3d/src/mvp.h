//mvp.h
//Model/View/Projection manipulation
//Bryan E. Topp <betopp@betopp.com> 2024
#ifndef MVP_H
#define MVP_H

#include "common.h"

//Resets the model/view/projection matrix
void mvp_ident(void);

//Multiplies a matrix into the current model/view/projection matrix
void mvp_mult(const fix24p8_t mm44[4][4]);

//Computes a projection matrix ala gluPerspective and multiplies it in
void mvp_persp(fix24p8_t fovy, fix24p8_t aspect, fix24p8_t znear, fix24p8_t zfar);

//Rotates the MVP matrix
void mvp_rotate(fix24p8_t angle, fix24p8_t x, fix24p8_t y, fix24p8_t z);

//Translates the MVP matrix
void mvp_translate(fix24p8_t x, fix24p8_t y, fix24p8_t z);

//Transforms a vertex using the mvp matrix
void mvp_xform(const fix24p8_t *in, fix24p8_t *out);

#endif //MVP_H
