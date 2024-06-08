//mvp.c
//Model/View/Projection manipulation
//Bryan E. Topp <betopp@betopp.com> 2024

#include "mvp.h"
#include "trig.h"

#include <string.h>

//Model/View/Projection matrix
static fix24p8_t mvp[4][4];

//Resets the model/view/projection matrix
void mvp_ident(void)
{
	static const fix24p8_t ident[4][4] =
	{
		{ FV(1),     0,     0,     0 },
		{     0, FV(1),     0,     0 },
		{     0,     0, FV(1),     0 },
		{     0,     0,     0, FV(1) },
	};
	memcpy(mvp, ident, sizeof(mvp));
}

//Multiplies a matrix into the current model/view/projection matrix
void mvp_mult(const fix24p8_t mm44[4][4])
{
	//There's a faster way to do this, yeah, but I don't care
	fix24p8_t result[4][4];
	for(int row = 0; row < 4; row++)
	{
		for(int col = 0; col < 4; col++)
		{
			int64_t accum = 0;
			for(int step = 0; step < 4; step++)
			{
				accum += mvp[row][step] * mm44[step][col];
			}
			result[row][col] = accum / FV(1);
		}
	}
	memcpy(mvp, result, sizeof(mvp));
}

//Computes a projection matrix ala gluPerspective and multiplies it in
void mvp_persp(fix24p8_t fovy, fix24p8_t aspect, fix24p8_t znear, fix24p8_t zfar)
{
	(void)fovy;
	fix24p8_t f = FV(1); //cotangent(fovy / 2)
	
	fix24p8_t za = (int64_t)FV(1) * (zfar + znear) / (znear - zfar);
	fix24p8_t zb =     ((int64_t)2 * zfar * znear) / (znear - zfar);
	
	fix24p8_t pp[4][4] = 
	{
		{ FV(1) * f / aspect, 0,      0,  0 },
		{                  0, f,      0,  0 },
		{                  0, 0,     za, zb },
		{                  0, 0, FV(-1),  0 },
	};
	
	mvp_mult(pp);
}

void mvp_rotate(fix24p8_t angle, fix24p8_t x, fix24p8_t y, fix24p8_t z)
{
	const fix24p8_t c = trig_cosd(angle); //.8
	const fix24p8_t ic = FV(1) - c;       //.8
	const fix24p8_t s = trig_sind(angle); //.8
	
	const int64_t xx = x * x; //.16
	const int64_t xy = x * y; //.16
	const int64_t xz = x * z; //.16
	const int64_t yy = y * y; //.16
	const int64_t yz = y * z; //.16
	const int64_t zz = z * z; //.16
	
	const int64_t xs = x * s;     //.16
	const int64_t ys = y * s;     //.16
	const int64_t zs = z * s;     //.16
	const int64_t c1 = FV(1) * c; //.16
	
	const int64_t m11 = (xx * ic) + (FV(1) * c1); //.24
	const int64_t m21 = (xy * ic) + (FV(1) * zs); //.24
	const int64_t m31 = (xz * ic) - (FV(1) * ys); //.24
	
	const int64_t m12 = (xy * ic) - (FV(1) * zs); //.24
	const int64_t m22 = (yy * ic) + (FV(1) * c1); //.24
	const int64_t m32 = (yz * ic) + (FV(1) * xs); //.24
	
	const int64_t m13 = (xz * ic) + (FV(1) * ys); //.24
	const int64_t m23 = (yz * ic) - (FV(1) * xs); //.24
	const int64_t m33 = (zz * ic) + (FV(1) * c1); //.24
	
	const fix24p8_t mm[4][4] = 
	{
		{ m11 / FV(FV(1)), m12 / FV(FV(1)), m13 / FV(FV(1)),     0 },
		{ m21 / FV(FV(1)), m22 / FV(FV(1)), m23 / FV(FV(1)),     0 },
		{ m31 / FV(FV(1)), m32 / FV(FV(1)), m33 / FV(FV(1)),     0 },
		{               0,               0,               0, FV(1) },
	};
	
	mvp_mult(mm);
}

void mvp_translate(fix24p8_t x, fix24p8_t y, fix24p8_t z)
{
	const fix24p8_t mm[4][4] = 
	{
		{ FV(1),     0,     0,     x },
		{     0, FV(1),     0,     y },
		{     0,     0, FV(1),     z },
		{     0,     0,     0, FV(1) },
	};
	
	mvp_mult(mm);
}

//Transforms a vertex using the mvp matrix
void mvp_xform(const fix24p8_t *in, fix24p8_t *out)
{
	int64_t temp[4] = {0,0,0,0};
	for(int dd = 0; dd < 4; dd++)
	{
		for(int ss = 0; ss < 4; ss++)
		{
			temp[dd] += in[ss] * mvp[dd][ss];
		}
	}
	for(int dd = 0; dd < 4; dd++)
	{
		out[dd] = temp[dd] / FV(1);
	}
}
