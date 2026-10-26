//r_fix.h
//Fixed-point arithmetic for the BSP renderer.
//Bryan E. Topp <betopp@betopp.com> 2026
//
//The machine has no FPU, so every float operation is a call into the soft-float
//library. The world renderer does this millions of times per frame, so it works
//in fixed point instead.
//
//Two formats, used for different jobs:
//
//	fix16_t - 32-bit, 16.16. World space, view space, texel space and screen
//	          space. Quake maps are +-4096 units, so 16 integer bits is plenty
//	          and 1/65536 unit is far finer than a pixel.
//
//	fix32_t - 64-bit, 32.32. Perspective quantities: 1/z, s/z, t/z and their
//	          gradients. These need range and precision at the same time - the
//	          1/z gradient across a surface is around 1e-6 per pixel, which
//	          16.16 cannot represent at all - so they get their own format.
//	          They are only touched once per 32-pixel span chunk, not per pixel.

#ifndef R_FIX_H
#define R_FIX_H

#include <stdint.h>

typedef int32_t	fix16_t;	//16.16
typedef int64_t	fix32_t;	//32.32

#define FIX16_ONE	0x10000

//=============================================================================
// fix16 - 16.16

//Integer to fixed. Fine for compile-time constants too.
#define FIX16(x)	((fix16_t)((x) * FIX16_ONE))

static inline fix16_t Fix16_FromInt(int v)
{
	return (fix16_t)(v * FIX16_ONE);
}

//Fixed to integer, rounding toward negative infinity (what >> does, and what
//the renderer wants when it converts a screen coordinate to a scan line).
static inline int Fix16_ToInt(fix16_t v)
{
	return (int)(v >> 16);
}

static inline int Fix16_ToIntRound(fix16_t v)
{
	return (int)((v + 0x8000) >> 16);
}

//IEEE754 float to 16.16, using integer operations only. Used when loading BSP
//data: the floats on disk are converted once, and even that costs no soft-float
//calls. Values out of range saturate rather than wrapping.
static inline fix16_t Fix16_FromWord(uint32_t u)
{
	int32_t ex = (int32_t)((u >> 23) & 0xFF) - 127;	//value is 1.ma * 2^ex
	uint32_t ma = (u & 0x7FFFFFu) | 0x800000u;	//1.23, with the implied 1
	int32_t r;

	if (((u >> 23) & 0xFF) == 0)
		return 0;	//zero or denormal - far below anything we care about

	//Want ma * 2^(ex-23+16) = ma * 2^(ex-7)
	ex -= 7;

	if (ex >= 0)
	{
		if (ex > 7)
			r = 0x7FFFFFFF;	//saturate past 32767 units
		else
			r = (int32_t)(ma << ex);
	}
	else if (ex < -24)
	{
		r = 0;	//below half a count
	}
	else
	{
		r = (int32_t)((ma + (1u << (-ex - 1))) >> (-ex));	//round to nearest
	}

	if (u >> 31)
		return (fix16_t)-r;
	return (fix16_t)r;
}

static inline fix16_t Fix16_FromFloat(float f)
{
	union { float f; uint32_t u; } cvt;
	cvt.f = f;
	return Fix16_FromWord(cvt.u);
}

//Multiply, via a 64-bit intermediate. Needed by everything: 16.16 * 16.16
//does not fit in 32 bits.
static inline fix16_t Fix16_Mul(fix16_t a, fix16_t b)
{
	return (fix16_t)(((fix32_t)a * (fix32_t)b) >> 16);
}

//Divide. b must be non-zero.
static inline fix16_t Fix16_Div(fix16_t a, fix16_t b)
{
	return (fix16_t)((((fix32_t)a) << 16) / (fix32_t)b);
}

//Divide, saturating instead of wrapping. Edge slopes divide by the height of
//the edge, which is nearly zero for a horizontal edge; the float build got a
//huge slope there and then clamped the result to the view rect, and so must
//we, rather than overflow into a wrong-signed coordinate.
static inline fix16_t Fix16_DivSat(fix16_t a, fix16_t b)
{
	fix32_t q;

	if (b == 0)
		return (a < 0) ? -0x7FFFFFFF - 1 : 0x7FFFFFFF;

	q = (((fix32_t)a) << 16) / (fix32_t)b;

	if (q > 0x7FFFFFFF)
		return 0x7FFFFFFF;
	if (q < -0x7FFFFFFF - 1)
		return -0x7FFFFFFF - 1;
	return (fix16_t)q;
}

//Smallest integer not less than v. Screen coordinates are clamped into the
//view rect first, so this never has to think about negatives.
static inline int Fix16_Ceil(fix16_t v)
{
	return (int)((v + 0xFFFF) >> 16);
}

//Dot product of two 3-vectors, accumulated in 64 bits.
static inline fix16_t Fix16_Dot3(const fix16_t a[3], const fix16_t b[3])
{
	fix32_t sum = ((fix32_t)a[0] * (fix32_t)b[0])
		+ ((fix32_t)a[1] * (fix32_t)b[1])
		+ ((fix32_t)a[2] * (fix32_t)b[2]);
	return (fix16_t)(sum >> 16);
}

//Same, with integer components on the left - a node's integer bounding box
//against a clip plane. Integer times 16.16 is already a 16.16 product, so no
//shift on the way out and no conversion of the integer side on the way in.
//Accumulated in 64 bits, then clamped: a short coordinate times a unit normal
//can exceed 32 bits, and a bogus result here silently culls half a map.
static inline fix16_t Fix16_Dot3Ints(int a0, int a1, int a2, const fix16_t b[3])
{
	fix32_t sum = ((fix32_t)a0 * (fix32_t)b[0])
		+ ((fix32_t)a1 * (fix32_t)b[1])
		+ ((fix32_t)a2 * (fix32_t)b[2]);

	if (sum > 0x7fffffff)
		return 0x7fffffff;
	if (sum < -0x7fffffff - 1)
		return -0x7fffffff - 1;
	return (fix16_t)sum;
}

//Back to float, for the one or two places where a value has to be handed back
//to code that still works in floats.
static inline float Fix16_ToFloat(fix16_t v)
{
	return (float)v * (1.0f / (float)FIX16_ONE);
}

//Copy a float vector into a 16.16 one. Used where the renderer takes over from
//the game code at frame setup, not in any inner loop.
static inline void Fix16_FromVec3(const float in[3], fix16_t out[3])
{
	out[0] = Fix16_FromFloat(in[0]);
	out[1] = Fix16_FromFloat(in[1]);
	out[2] = Fix16_FromFloat(in[2]);
}

static inline void Fix16_Sub3(const fix16_t a[3], const fix16_t b[3], fix16_t out[3])
{
	out[0] = a[0] - b[0];
	out[1] = a[1] - b[1];
	out[2] = a[2] - b[2];
}

static inline void Fix16_Copy3(const fix16_t in[3], fix16_t out[3])
{
	out[0] = in[0];
	out[1] = in[1];
	out[2] = in[2];
}

//=============================================================================
// fix32 - 32.32

static inline fix32_t Fix32_FromFix16(fix16_t v)
{
	return ((fix32_t)v) << 16;
}

static inline fix16_t Fix32_ToFix16(fix32_t v)
{
	return (fix16_t)(v >> 16);
}

static inline float Fix32_ToFloat(fix32_t v)
{
	return (float)v * (1.0f / 4294967296.0f);
}

static inline int Fix32_ToInt(fix32_t v)
{
	return (int)(v >> 32);
}

//32.32 * 16.16 giving a 16.16 result - the texture-coordinate case:
//(s/z in 32.32) * (z in 16.16) = s in 16.16.
static inline fix16_t Fix32_MulToFix16(fix32_t a, fix16_t b)
{
	return (fix16_t)((a * (fix32_t)b) >> 32);
}

//16.16 * 32.32 giving 32.32 - the gradient case: a texture axis (16.16) times
//the per-pixel step of a perspective value (32.32).
static inline fix32_t Fix16_MulToFix32(fix16_t a, fix32_t b)
{
	return ((fix32_t)a * b) >> 16;
}

//16.16 * 16.16 giving 32.32, with no shift at all: the raw product of two
//16.16 values is already scaled by 2^32. Used to build the first gradient step
//from two 16.16 inputs without losing the low bits.
static inline fix32_t Fix16_Mul16ToFix32(fix16_t a, fix16_t b)
{
	return (fix32_t)a * (fix32_t)b;
}

//Reciprocal of a 16.16 value, as 32.32: 1/(a/2^16) * 2^32 = 2^48/a. This is
//1/distance-to-plane, which the renderer needs per surface.
static inline fix32_t Fix16_Recip32(fix16_t a)
{
	return ((fix32_t)1 << 48) / (fix32_t)a;
}

//Reciprocal of a 32.32 value, as 16.16. This is 1/(1/z), i.e. z, which is what
//the texture mapper needs to scale s/z and t/z back to s and t. One 64-bit
//divide per span chunk.
static inline fix16_t Fix32_Recip(fix32_t v)
{
	return (fix16_t)(((fix32_t)1 << 48) / v);
}

//1/z in 32.32 (the form zi is carried around in) to the z-buffer's
//"1/z << 31" int. Only the top half is stored, as it always has been.
static inline int Fix32_ToZBuf(fix32_t v)
{
	return (int)(v >> 1);
}

#endif //R_FIX_H
