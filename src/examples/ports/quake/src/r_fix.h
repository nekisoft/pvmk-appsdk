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

//Defined with the reciprocals below
static inline fix16_t Fix16_Recip16 (fix16_t a);

//Multiply, saturating instead of wrapping. Used where the product can be far
//outside the representable range and only its sign and rough size matter - the
//slope of a nearly horizontal edge, for instance.
static inline fix16_t Fix16_MulSat(fix16_t a, fix16_t b)
{
	fix32_t p = ((fix32_t)a * (fix32_t)b) >> 16;

	if (p > 0x7FFFFFFF)
		return (fix16_t)0x7FFFFFFF;
	if (p < -0x7FFFFFFF - 1)
		return (fix16_t)(-0x7FFFFFFF - 1);
	return (fix16_t)p;
}

//Divide. Exact, and left as a 64-bit divide: a<<16 does not fit in 32 bits for
//any a above half a unit, which is most of them. Callers are per-edge and
//per-clip, not per pixel, so this one is not worth an approximation.
static inline fix16_t Fix16_Div(fix16_t a, fix16_t b)
{
	return (fix16_t)((((fix32_t)a) << 16) / (fix32_t)b);
}

//Divide, saturating instead of wrapping. Edge slopes divide by the height of
//the edge, which is nearly zero for a horizontal edge; the float build got a
//huge slope there and then clamped the result to the view rect, and so must
//we, rather than overflow into a wrong-signed coordinate. A reciprocal keeps
//the 64-bit divide out of it entirely, and the result is clamped either way.
static inline fix16_t Fix16_DivSat(fix16_t a, fix16_t b)
{
	if (b == 0)
		return (a < 0) ? (fix16_t)(-0x7FFFFFFF - 1) : (fix16_t)0x7FFFFFFF;

	return Fix16_MulSat(a, Fix16_Recip16(b));
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

//----------------------------------------------------------------------------
//Reciprocals
//
//The renderer divides by the distance to a plane once per surface and once per
//span chunk, and by the edge height once per edge. Written as an integer
//division that is 64 bits over 32, which on ARMv5 is __udivmoddi4's bit loop -
//a hundred instructions and change, every time. Measured on the console it was
//14% of a frame, more than the rasteriser itself.
//
//So: a reciprocal by Newton-Raphson instead. Normalise the divisor to
//[2^31, 2^32), take an 8-bit seed from a table, and refine it twice - each step
//doubles the bits, so 8 -> 16 -> 32. Six multiplies and no division.
//
//The seed is 2^31 / (256 + index) for the index being the top eight bits of the
//normalised divisor below its leading one, which is (1<<39)/(256+index).
static const uint32_t fix_recip_tab[256] =
{
	2147483648u, 2139127680u, 2130836488u, 2122609320u, 2114445438u, 2106344115u, 2098304633u, 2090326289u,
	2082408386u, 2074550241u, 2066751180u, 2059010539u, 2051327664u, 2043701910u, 2036132644u, 2028619239u,
	2021161080u, 2013757560u, 2006408080u, 1999112051u, 1991868891u, 1984678028u, 1977538899u, 1970450946u,
	1963413621u, 1956426384u, 1949488702u, 1942600049u, 1935759908u, 1928967768u, 1922223125u, 1915525484u,
	1908874354u, 1902269252u, 1895709703u, 1889195237u, 1882725390u, 1876299706u, 1869917734u, 1863579030u,
	1857283155u, 1851029676u, 1844818167u, 1838648207u, 1832519380u, 1826431275u, 1820383490u, 1814375623u,
	1808407283u, 1802478078u, 1796587627u, 1790735550u, 1784921474u, 1779145029u, 1773405851u, 1767703582u,
	1762037865u, 1756408351u, 1750814694u, 1745256552u, 1739733588u, 1734245470u, 1728791868u, 1723372457u,
	1717986918u, 1712634934u, 1707316192u, 1702030384u, 1696777203u, 1691556350u, 1686367527u, 1681210440u,
	1676084798u, 1670990316u, 1665926709u, 1660893698u, 1655891006u, 1650918360u, 1645975491u, 1641062131u,
	1636178018u, 1631322890u, 1626496491u, 1621698566u, 1616928864u, 1612187138u, 1607473140u, 1602786629u,
	1598127366u, 1593495113u, 1588889636u, 1584310703u, 1579758086u, 1575231558u, 1570730897u, 1566255880u,
	1561806289u, 1557381909u, 1552982525u, 1548607926u, 1544257904u, 1539932252u, 1535630765u, 1531353242u,
	1527099483u, 1522869291u, 1518662469u, 1514478826u, 1510318170u, 1506180312u, 1502065065u, 1497972245u,
	1493901668u, 1489853154u, 1485826524u, 1481821601u, 1477838209u, 1473876177u, 1469935331u, 1466015504u,
	1462116526u, 1458238233u, 1454380460u, 1450543045u, 1446725826u, 1442928645u, 1439151345u, 1435393770u,
	1431655765u, 1427937179u, 1424237860u, 1420557659u, 1416896428u, 1413254020u, 1409630292u, 1406025099u,
	1402438301u, 1398869755u, 1395319325u, 1391786871u, 1388272257u, 1384775350u, 1381296015u, 1377834120u,
	1374389535u, 1370962129u, 1367551776u, 1364158347u, 1360781718u, 1357421763u, 1354078359u, 1350751385u,
	1347440720u, 1344146244u, 1340867839u, 1337605387u, 1334358772u, 1331127879u, 1327912594u, 1324712805u,
	1321528399u, 1318359266u, 1315205296u, 1312066382u, 1308942414u, 1305833287u, 1302738895u, 1299659134u,
	1296593901u, 1293543092u, 1290506605u, 1287484342u, 1284476201u, 1281482084u, 1278501893u, 1275535531u,
	1272582903u, 1269643912u, 1266718465u, 1263806469u, 1260907830u, 1258022457u, 1255150260u, 1252291148u,
	1249445032u, 1246611823u, 1243791434u, 1240983779u, 1238188770u, 1235406323u, 1232636354u, 1229878778u,
	1227133513u, 1224400476u, 1221679586u, 1218970763u, 1216273925u, 1213588993u, 1210915890u, 1208254536u,
	1205604855u, 1202966770u, 1200340205u, 1197725085u, 1195121335u, 1192528880u, 1189947649u, 1187377568u,
	1184818564u, 1182270568u, 1179733506u, 1177207310u, 1174691910u, 1172187236u, 1169693221u, 1167209796u,
	1164736894u, 1162274448u, 1159822392u, 1157380661u, 1154949189u, 1152527912u, 1150116765u, 1147715687u,
	1145324612u, 1142943480u, 1140572228u, 1138210795u, 1135859120u, 1133517142u, 1131184802u, 1128862041u,
	1126548799u, 1124245018u, 1121950641u, 1119665609u, 1117389866u, 1115123355u, 1112866020u, 1110617806u,
	1108378657u, 1106148519u, 1103927337u, 1101715058u, 1099511628u, 1097316994u, 1095131103u, 1092953904u,
	1090785345u, 1088625374u, 1086473940u, 1084330994u, 1082196484u, 1080070361u, 1077952576u, 1075843080u,
};

//Reciprocal of a normalised divisor: n is in [2^31, 2^32), and the result is
//2^31 / (n / 2^31), i.e. the reciprocal of n/2^31 in 0.32.
static inline uint32_t Fix_RecipNorm (uint32_t n)
{
	uint32_t r = fix_recip_tab[(n >> 23) - 256];
	int i;

	for (i = 0; i < 2; i++)
	{
		//r = r * (2 - (n/2^31) * r), all in 0.32
		uint32_t nr = (uint32_t)(((uint64_t)n * (uint64_t)r) >> 31);
		int64_t e = (int64_t)0x100000000LL - (int64_t)nr;
		r = (uint32_t)(((uint64_t)r * (uint64_t)e) >> 31);
	}

	return r;
}

//Reciprocal of a 16.16 value, as 32.32: 1/(a/2^16) * 2^32 = 2^48/a. This is
//1/distance-to-plane, which the renderer needs per surface.
static inline fix32_t Fix16_Recip32 (fix16_t a)
{
	uint32_t n;
	int s, sh;
	uint32_t r;

	if (a == 0)
		return 0;
	if (a < 0)
		return -Fix16_Recip32 ((fix16_t)-a); //a plane distance can point away

	n = (uint32_t)a;
	s = __builtin_clz (n);
	n <<= s;

	r = Fix_RecipNorm (n);

	//2^48/a is r scaled by 2^(s-14); the leading bit lands in the same place
	//either way, so only the shift direction has to be chosen
	sh = s - 14;
	if (sh >= 0)
		return (fix32_t)r << sh;
	return (fix32_t)(r >> -sh);
}

//Reciprocal of a 32.32 value, as 16.16. This is 1/(1/z), i.e. z, which is what
//the texture mapper needs to scale s/z and t/z back to s and t. Called once per
//span chunk, so it has to be cheap.
static inline fix16_t Fix32_Recip (fix32_t v)
{
	uint64_t n;
	int s, sh;
	uint32_t r;

	if (v == 0)
		return 0;
	if (v < 0)
		return -Fix32_Recip (-v);

	//Normalise to 64 bits, then keep the top 32 of them: the low half cannot
	//move a 16.16 result
	n = (uint64_t)v;
	s = __builtin_clzll (n);
	n <<= s;

	r = Fix_RecipNorm ((uint32_t)(n >> 32));

	sh = s - 46;
	if (sh >= 0)
		return (fix16_t)((uint64_t)r << sh);
	return (fix16_t)(r >> -sh);
}

//Reciprocal of a 16.16 value as 16.16, saturating: only good to 16 bits, for
//dividing by something that may be close to zero.
static inline fix16_t Fix16_Recip16 (fix16_t a)
{
	uint32_t n;
	int s, sh;
	uint32_t r;
	fix32_t out;

	if (a == 0)
		return (fix16_t)0x7FFFFFFF;

	if (a < 0)
		return (fix16_t)(-Fix16_Recip16 ((fix16_t)-a));

	n = (uint32_t)a;
	s = __builtin_clz (n);
	n <<= s;

	r = Fix_RecipNorm (n);

	sh = s - 30;
	if (sh >= 0)
		out = (fix32_t)r << sh;
	else
		out = (fix32_t)(r >> -sh);

	if (out > 0x7FFFFFFF)
		return (fix16_t)0x7FFFFFFF;
	return (fix16_t)out;
}


//1/z in 32.32 (the form zi is carried around in) to the z-buffer's
//"1/z << 31" int. Only the top half is stored, as it always has been.
static inline int Fix32_ToZBuf(fix32_t v)
{
	return (int)(v >> 1);
}

#endif //R_FIX_H
