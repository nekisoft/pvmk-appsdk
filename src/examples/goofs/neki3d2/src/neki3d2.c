//neki3d2.c
//A fast fixed-point 3D renderer for Neki32.
//Bryan E. Topp <betopp@betopp.com> 2026
//
//Design, in order of importance:
//
//  - No floating point in the frame loop. 16.16 fixed point for positions, screen space and
//    texture coordinates, with 64-bit intermediates only where a product needs them. The
//    only float is the 256-entry sine table built once at startup.
//
//  - No divide in the pixel loop. A triangle gets plane equations for 1/z, u/z and v/z once
//    (a few divides per triangle); each pixel is then six adds. Perspective correctness comes
//    from evaluating those planes exactly at both ends of every run of eight pixels and
//    interpolating affinely between them - the same trade Quake's span code makes, and the
//    reason a textured floor does not cost a reciprocal per pixel.
//
//  - Cull before setup, bound before testing, depth-test before the texture fetch. The
//    inside test is three edge functions advanced by constants (three adds, three compares).
//
//  - The framebuffer is cleared with 32-bit stores and the texture is RGB565, sampled as one
//    16-bit load and one 16-bit store per pixel.
//
//  - Lighting is per face: eight pre-shaded copies of the texture are built at startup, so a
//    lit pixel costs the same as an unlit one.
//
//Geometry and texture are procedural, so the example needs no assets and runs anywhere.
//
//The scene is a tumbling cube above a chequered floor, on a fixed camera path, so a run is
//deterministic and two builds can be compared frame for frame.

#include <sc.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#define FBW 320
#define FBH 240
#define NFB 3

//--- fixed point -------------------------------------------------------------
typedef int32_t f16;   //16.16
typedef int64_t f32;   //wide intermediate

#define F16(n)  ((f16)((n) * 65536))

//A 32.32 reciprocal from one 32-bit divide. The obvious (1<<48)/w is a 64-bit divide,
//which costs about a hundred instructions on ARMv5 and showed up as 31% of the frame.
static inline f32 recip32(f16 w)
{
	if(w <= 0)
		return 0;
	return ((f32)(0xFFFFFFFFu / (uint32_t)w)) << 16;
}
#define M16(a,b) ((f16)(((f32)(a) * (f32)(b)) >> 16))

//--- framebuffers and depth --------------------------------------------------
static uint16_t fbs[NFB][FBH][FBW];
static uint16_t (*draw_fb)[FBW];   //the buffer being rendered into this frame
static int32_t zbuf[FBH][FBW];     //1/z in 16.16; 0 means infinitely far

//--- texture ----------------------------------------------------------------
#define TEXW 64
#define TEXH 64
#define LIGHTLEVELS 8
static uint16_t tex[LIGHTLEVELS][TEXH][TEXW];

static inline uint16_t rgb565(int r, int g, int b)
{
	return (uint16_t)(((r & 0x1F) << 11) | ((g & 0x3F) << 5) | (b & 0x1F));
}

static void build_texture(void)
{
	//A chequerboard with a bright border and a red stripe: enough to see perspective and the
	//light levels in a screenshot. Each light level is a separate copy, so the pixel loop
	//never does a multiply for shading.
	for(int level = 0; level < LIGHTLEVELS; level++)
	{
		for(int y = 0; y < TEXH; y++)
		{
			for(int x = 0; x < TEXW; x++)
			{
				int r, g, b;
				if(((x ^ y) & 8) != 0)
				{
					r = 26; g = 22; b = 12;
				}
				else
				{
					r = 10; g = 14; b = 24;
				}
				
				if(x >= 28 && x < 36 && y >= 16 && y < 48)
				{
					r = 30; g = 5; b = 5;
				}
				const int scale = 2 + (level * 4); //2..30
				tex[level][y][x] = rgb565((r * scale) / 30, (g * scale) / 30, (b * scale) / 30);
			}
		}
	}
}

//--- geometry ----------------------------------------------------------------
#define FLOOR_LIGHT (LIGHTLEVELS / 2)
#define MAXV 32
#define MAXTRI 48

typedef struct vert_s
{
	f16 x, y, z;
	int32_t u, v;   //texel coordinates, 16.16
} vert_t;

typedef struct tri_s
{
	int v[3];
	int light;
} tri_t;

static vert_t verts[MAXV];
static tri_t tris[MAXTRI];
static int nverts, ntris;

static void add_vert(f16 x, f16 y, f16 z, int u, int v)
{
	verts[nverts].x = x;
	verts[nverts].y = y;
	verts[nverts].z = z;
	verts[nverts].u = (int32_t)u << 16;
	verts[nverts].v = (int32_t)v << 16;
	nverts++;
}

static void add_tri(int a, int b, int c, int light)
{
	tris[ntris].v[0] = a;
	tris[ntris].v[1] = b;
	tris[ntris].v[2] = c;
	tris[ntris].light = light;
	ntris++;
}

static void build_geometry(void)
{
	//A one-unit cube above a chequered floor. Each face has its own four vertices: a shared
	//corner cannot carry the two different texture coordinates the two faces meeting there
	//need, and a cube built from eight vertices samples one texture row across a face.
	const f16 s = F16(1) / 2;
	const int top = LIGHTLEVELS - 1;
	const int side = LIGHTLEVELS - 2;
	const int low = LIGHTLEVELS / 2;

	//Per face: four corners in clockwise order seen from outside, and the light level.
	const struct { f16 x[4], y[4], z[4]; int light; } faces[6] =
	{
		{ { -s,  s,  s, -s }, {  s,  s, -s, -s }, { -s, -s,  s,  s }, top  }, //+Y top
		{ { -s,  s,  s, -s }, { -s, -s, -s, -s }, { -s, -s,  s,  s }, low  }, //-Y bottom
		{ { -s,  s,  s, -s }, { -s, -s,  s,  s }, {  s,  s,  s,  s }, side }, //+Z front
		{ {  s, -s, -s,  s }, { -s, -s,  s,  s }, { -s, -s, -s, -s }, side }, //-Z back
		{ {  s,  s,  s,  s }, { -s, -s,  s,  s }, { -s,  s,  s, -s }, side }, //+X right
		{ { -s, -s, -s, -s }, { -s, -s,  s,  s }, {  s,  s, -s, -s }, side }, //-X left
	};
	const int uvc[4][2] = { {0, 0}, {TEXW, 0}, {TEXW, TEXH}, {0, TEXH} };

	for(int ff = 0; ff < 6; ff++)
	{
		const int base = nverts;
		for(int cc = 0; cc < 4; cc++)
			add_vert(faces[ff].x[cc], faces[ff].y[cc], faces[ff].z[cc],
				uvc[cc][0], uvc[cc][1]);
		add_tri(base + 0, base + 1, base + 2, faces[ff].light);
		add_tri(base + 0, base + 2, base + 3, faces[ff].light);
	}

	//Ground plane, four units square, one unit below the cube's centre. The texture repeats
	//four times across it.
	const int base = nverts;
	add_vert(F16(-16), F16(-1), F16(-16), 0, 0);
	add_vert(F16( 16), F16(-1), F16(-16), TEXW * 8, 0);
	add_vert(F16( 16), F16(-1), F16( 16), TEXW * 8, TEXH * 8);
	add_vert(F16(-16), F16(-1), F16( 16), 0, TEXH * 8);
	add_tri(base + 0, base + 2, base + 1, FLOOR_LIGHT);
	add_tri(base + 0, base + 3, base + 2, FLOOR_LIGHT);
}

//--- clipping and projection -------------------------------------------------
#define NEARZ (F16(1) / 4)

typedef struct svert_s
{
	int x, y;      //screen pixels, 16.16
	f16 w;         //1/z, 16.16
	f16 sz, tz;    //u/z and v/z, 16.16
} svert_t;

static f16 sin_tab[256];
static f16 cos_tab[256];

static void build_trig(void)
{
	//The only floating point in the program, and it runs 256 times before the first frame.
	for(int ii = 0; ii < 256; ii++)
	{
		const double a = (double)ii * 6.28318530717958647692 / 256.0;
		sin_tab[ii] = (f16)(sin(a) * 65536.0);
		cos_tab[ii] = (f16)(cos(a) * 65536.0);
	}
}

static int project(const vert_t *v, svert_t *out)
{
	//A clipped vertex lands exactly on the near plane; it is still projectable.
	if(v->z <= 0)
		return 0;

	//Perspective divide, once per vertex. Screen y grows downwards, so world y is negated.
	const int cx = FBW / 2;
	const int cy = FBH / 2;
	const f16 sx = (f16)(((f32)v->x * 220) / v->z) + cx;
	const f16 sy = cy - (f16)(((f32)v->y * 220) / v->z);
	out->x = sx << 16;
	out->y = sy << 16;

	out->w = (f16)(((f32)1 << 32) / v->z);
	out->sz = (f16)(((f32)v->u * out->w) >> 16);
	out->tz = (f16)(((f32)v->v * out->w) >> 16);
	return 1;
}

//Sutherland-Hodgman against z >= NEARZ. Returns 0..4 output vertices.
static int clip_near(const vert_t in[3], vert_t out[4])
{
	int n = 0;
	for(int ii = 0; ii < 3; ii++)
	{
		const vert_t *a = &(in[ii]);
		const vert_t *b = &(in[(ii + 1) % 3]);
		const int a_in = (a->z > NEARZ);
		const int b_in = (b->z > NEARZ);

		if(a_in)
			out[n++] = *a;

		if(a_in != b_in)
		{
			const f32 num = (f32)NEARZ - a->z;
			const f32 den = (f32)b->z - a->z;
			vert_t m;
			m.x = (f16)(a->x + (f16)(((f32)(b->x - a->x) * num) / den));
			m.y = (f16)(a->y + (f16)(((f32)(b->y - a->y) * num) / den));
			m.z = NEARZ;
			m.u = (int32_t)(a->u + (int32_t)(((f32)(b->u - a->u) * num) / den));
			m.v = (int32_t)(a->v + (int32_t)(((f32)(b->v - a->v) * num) / den));
			out[n++] = m;
		}
	}
	return n;
}

//--- rasteriser --------------------------------------------------------------
static long stat_tris;
static int64_t stat_pixels;
static long stat_runs;

typedef struct plane_s
{
	f16 a, b, c;   //q = a*x + b*y + c, with x and y in 16.16
} plane_t;

static plane_t make_plane(const svert_t *v0, const svert_t *v1, const svert_t *v2,
	f16 q0, f16 q1, f16 q2, f32 det)
{
	plane_t p;
	//Coordinates are 16.16, so a slope per raw unit has to be scaled by 65536 to become a
	//slope per screen pixel. Getting this wrong makes every coefficient round to zero and
	//the whole triangle sample one texel, which looks like a flat-coloured polygon.
	//Scaling the numerator by 65536 to convert "per raw unit" to "per pixel" overflows
	//64 bits for a floor triangle near the camera. Divide the determinant by 65536 instead:
	//the result is the same slope per pixel, and the truncated division only costs a
	//fraction of one 16.16 unit.
	const f32 detpx = det >> 16;
	if(detpx == 0)
	{
		p.a = p.b = p.c = 0;
		return p;
	}
	p.a = (f16)(((f32)(q1 - q0) * (v2->y - v0->y) -
	             (f32)(q2 - q0) * (v1->y - v0->y)) / detpx);
	p.b = (f16)(((f32)(v1->x - v0->x) * (q2 - q0) -
	             (f32)(v2->x - v0->x) * (q1 - q0)) / detpx);
	p.c = (f16)(q0 - (f16)(((f32)p.a * v0->x + (f32)p.b * v0->y) >> 16));
	return p;
}

static inline f16 plane_at(const plane_t *p, int x, int y)
{
	return (f16)(((f32)p->a * x + (f32)p->b * y) >> 16) + p->c;
}

//Reciprocals of 1..15, used to divide by the length of a run without a divide.
static const f16 spanrecip[16] =
{
	0,
	F16(1), F16(1) / 2, F16(1) / 3, F16(1) / 4, F16(1) / 5,
	F16(1) / 6, F16(1) / 7, F16(1) / 8, F16(1) / 9, F16(1) / 10,
	F16(1) / 11, F16(1) / 12, F16(1) / 13, F16(1) / 14, F16(1) / 15
};

static void raster_tri(const svert_t *a, const svert_t *b, const svert_t *c, int light)
{
	svert_t vv[3];
	vv[0] = *a;
	vv[1] = *b;
	vv[2] = *c;

	f32 det = (f32)(vv[1].x - vv[0].x) * (vv[2].y - vv[0].y) -
	          (f32)(vv[2].x - vv[0].x) * (vv[1].y - vv[0].y);
	if(det == 0)
		return;
	if(det < 0)
	{
		const svert_t tmp = vv[1];
		vv[1] = vv[2];
		vv[2] = tmp;
		det = -det;
	}

	const plane_t pw = make_plane(&vv[0], &vv[1], &vv[2], vv[0].w, vv[1].w, vv[2].w, det);
	const plane_t pu = make_plane(&vv[0], &vv[1], &vv[2], vv[0].sz, vv[1].sz, vv[2].sz, det);
	const plane_t pv = make_plane(&vv[0], &vv[1], &vv[2], vv[0].tz, vv[1].tz, vv[2].tz, det);

	int minx = vv[0].x >> 16, maxx = minx;
	int miny = vv[0].y >> 16, maxy = miny;
	for(int ii = 1; ii < 3; ii++)
	{
		const int x = vv[ii].x >> 16;
		const int y = vv[ii].y >> 16;
		if(x < minx) minx = x;
		if(x > maxx) maxx = x;
		if(y < miny) miny = y;
		if(y > maxy) maxy = y;
	}
	if(minx < 0) minx = 0;
	if(miny < 0) miny = 0;
	if(maxx >= FBW) maxx = FBW - 1;
	if(maxy >= FBH) maxy = FBH - 1;
	if(minx > maxx || miny > maxy)
		return;

	//Edge functions in 16.16, advanced by a constant per pixel.
	const int ex0 = vv[1].x - vv[0].x, ey0 = vv[1].y - vv[0].y;
	const int ex1 = vv[2].x - vv[1].x, ey1 = vv[2].y - vv[1].y;
	const int ex2 = vv[0].x - vv[2].x, ey2 = vv[0].y - vv[2].y;

	for(int y = miny; y <= maxy; y++)
	{
		const int yfix = y << 16;
		f16 wrow = plane_at(&pw, minx << 16, yfix);
		f16 urow = plane_at(&pu, minx << 16, yfix);
		f16 vrow = plane_at(&pv, minx << 16, yfix);

		f32 e0 = (f32)ex0 * (yfix - vv[0].y) - (f32)ey0 * ((minx << 16) - vv[0].x);
		f32 e1 = (f32)ex1 * (yfix - vv[1].y) - (f32)ey1 * ((minx << 16) - vv[1].x);
		f32 e2 = (f32)ex2 * (yfix - vv[2].y) - (f32)ey2 * ((minx << 16) - vv[2].x);

		int x = minx;
		while(x <= maxx)
		{
			if(e0 < 0 || e1 < 0 || e2 < 0)
			{
				x++;
				//Edge function e = ex*(y-y0) - ey*(x-x0): a step in x subtracts ey.
				e0 -= (f32)ey0 << 16;
				e1 -= (f32)ey1 << 16;
				e2 -= (f32)ey2 << 16;
				wrow += pw.a;
				urow += pu.a;
				vrow += pv.a;
				continue;
			}

			//A run of up to eight inside pixels. Planes are evaluated exactly at both ends
			//and texture coordinates are interpolated affinely inside: two reciprocals per
			//run instead of one per pixel.
			const int x0 = x;
			int n = 0;
			stat_runs++;
			while(n < 8 && x <= maxx && e0 >= 0 && e1 >= 0 && e2 >= 0)
			{
				n++;
				x++;
				//Edge function e = ex*(y-y0) - ey*(x-x0): a step in x subtracts ey.
				e0 -= (f32)ey0 << 16;
				e1 -= (f32)ey1 << 16;
				e2 -= (f32)ey2 << 16;
				wrow += pw.a;
				urow += pu.a;
				vrow += pv.a;
			}

			const f16 w_end = (f16)(wrow - pw.a);
			const f16 u_end = (f16)(urow - pu.a);
			const f16 v_end = (f16)(vrow - pv.a);
			const f16 w_start = (f16)(w_end - (f16)(pw.a * (n - 1)));
			const f16 u_plane_start = (f16)(u_end - (f16)(pu.a * (n - 1)));
			const f16 v_plane_start = (f16)(v_end - (f16)(pv.a * (n - 1)));

			//A 32.32 reciprocal, not 16.16: 1/w for a distant surface is a small number, and
			//rounding it to whole 16.16 units turns every texture coordinate on the polygon
			//into the same value. The division happens twice per run of up to eight pixels.
			const f32 rw_start = recip32(w_start);
			int32_t u = (int32_t)(((f32)u_plane_start * rw_start) >> 32);
			int32_t v = (int32_t)(((f32)v_plane_start * rw_start) >> 32);
			int32_t du = 0, dv = 0;
			if(n > 1)
			{
				const f32 rw_end = recip32(w_end);
				const int32_t u_last = (int32_t)(((f32)u_end * rw_end) >> 32);
				const int32_t v_last = (int32_t)(((f32)v_end * rw_end) >> 32);
				du = M16(u_last - u, spanrecip[n - 1]);
				dv = M16(v_last - v, spanrecip[n - 1]);
			}

			//Three edge functions are already inside for the whole run, so the pixel loop is
			//a depth compare, a wrap, a load and a store. w is linear in x, so it is an add.
			f16 w = w_start;
			for(int pp = 0; pp < n; pp++)
			{
				const int px = x0 + pp;
				if(w > zbuf[y][px])
				{
					const int tu = (u >> 16) & (TEXW - 1);
					const int tv = (v >> 16) & (TEXH - 1);
					draw_fb[y][px] = tex[light][tv][tu];
					zbuf[y][px] = w;
					stat_pixels++;
				}
				u += du;
				v += dv;
				w += pw.a;
			}
		}
	}
	stat_tris++;
}

//--- frame ------------------------------------------------------------------
static int back = 0;

static void clear_frame(void)
{
	uint32_t *fb = (uint32_t *)&(draw_fb[0][0]);
	for(int ii = 0; ii < (FBW * FBH) / 2; ii++)
		fb[ii] = 0x000F000F; //dark blue sky
	for(int y = 0; y < FBH; y++)
	{
		uint32_t *z = (uint32_t *)&(zbuf[y][0]);
		for(int x = 0; x < FBW / 2; x++)
			z[x] = 0;
	}
}

static void render_scene(int yawidx)
{
	const f16 ca = cos_tab[yawidx & 255];
	const f16 sa = sin_tab[yawidx & 255];

	for(int tt = 0; tt < ntris; tt++)
	{
		vert_t in[3];
		for(int kk = 0; kk < 3; kk++)
		{
			const int vi = tris[tt].v[kk];
			//Rotate about Y and push the world in front of the camera
			const f16 x = verts[vi].x;
			const f16 z = verts[vi].z;
			in[kk].x = M16(x, ca) - M16(z, sa);
			in[kk].z = M16(x, sa) + M16(z, ca) + F16(4);
			in[kk].y = verts[vi].y;
			in[kk].u = verts[vi].u;
			in[kk].v = verts[vi].v;
		}

		vert_t poly[4];
		int np = clip_near(in, poly);
		if(np < 3)
			continue;

		svert_t sp[4];
		int projected = 0;
		for(int ii = 0; ii < np; ii++)
			if(project(&poly[ii], &sp[ii]))
				projected++;
		if(projected != np)
			continue;

		//Backface cull in screen space. Screen y is flipped, so a front face has negative
		//area by this ordering.
		const f32 area = (f32)(sp[1].x - sp[0].x) * (sp[2].y - sp[0].y) -
		                 (f32)(sp[2].x - sp[0].x) * (sp[1].y - sp[0].y);
		//The floor is a single plane seen from above; its winding is the opposite of the
		//cube's screen-space winding, so it is exempt from the cull rather than given a
		//separate vertex order.
		if(area >= 0 && tris[tt].light != FLOOR_LIGHT)
			continue;

		raster_tri(&sp[0], &sp[1], &sp[2], tris[tt].light);
		if(np == 4)
			raster_tri(&sp[0], &sp[2], &sp[3], tris[tt].light);
	}
}

int app_main(int argc, const char **argv)
{
	(void)argc;
	(void)argv;
	setvbuf(stdout, NULL, _IOLBF, 0);

	build_trig();
	build_texture();
	build_geometry();

	int frame = 0;
	int yaw = 0;
	for(;;)
	{
		draw_fb = fbs[back];
		clear_frame();
		stat_tris = 0;
		stat_pixels = 0;
		stat_runs = 0;

		render_scene(yaw);

		//Enqueue and carry on: three buffers mean one can be on screen, one waiting at the
		//next blank, and one being drawn. This is a benchmark, so it runs as fast as the
		//machine allows and lets the display drop frames rather than throttling the renderer.
		_sc_gfx_flip(_SC_GFX_MODE_320X240_16BPP, fbs[back]);

		//The buffer just enqueued is on screen; any other is free to draw into next.
		back = (back + 1) % NFB;

		frame++;
		if((frame % 60) == 0)
		{
			printf("[EVENT] FRAME %d tris %ld runs %ld px %ld\n",
				frame, stat_tris, stat_runs, (long)stat_pixels);
			fflush(stdout);
		}

		yaw += 2;
	}
	return 0;
}

int main(int argc, const char **argv)
{
	return _sc_shim_main(app_main, argc, argv);
}
