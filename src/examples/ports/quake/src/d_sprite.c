/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// d_sprite.c: software top-level rasterization driver module for drawing
// sprites

#include "quakedef.h"
#include "d_local.h"

#include <stdint.h>

static int		sprite_height;
static int		minindex, maxindex;
static sspan_t	*sprite_spans;

#if	!id386

/*
=====================
D_SpriteDrawSpans
=====================
*/
void D_SpriteDrawSpans (sspan_t *pspan)
{
	int			count, spancount, izistep;
	int			izi;
	pixel_t		*pbase;//, *pdest;
	pixel_t  *pdest; 
	fixed16_t	s, t, snext, tnext, sstep, tstep;
	fix32_t		sdivz, tdivz, zi, sdivz8stepu, tdivz8stepu, zi8stepu;
	fix16_t		z;
	int			spancountminus1;
	byte		btemp;
	short		*pz;

	sstep = 0;	// keep compiler happy
	tstep = 0;	// ditto

	pbase = cacheblock;

	sdivz8stepu = d_sdivzstepu * 8;
	tdivz8stepu = d_tdivzstepu * 8;
	zi8stepu = d_zistepu * 8;

// z buffer holds 1/z scaled by 2^31, top half only
	izistep = (int)(d_zistepu >> 1);

	do
	{
		pdest = ((pixel_t *)d_viewbuffer + (screenwidth * pspan->v) + (pspan->u));
		pz = d_pzbuffer + (d_zwidth * pspan->v) + pspan->u;

		count = pspan->count;

		if (count <= 0)
			goto NextSpan;

	// calculate the initial s/z, t/z, 1/z, s, and t and clamp
		sdivz = d_sdivzorigin + pspan->v * d_sdivzstepv + pspan->u * d_sdivzstepu;
		tdivz = d_tdivzorigin + pspan->v * d_tdivzstepv + pspan->u * d_tdivzstepu;
		zi = d_ziorigin + pspan->v * d_zistepv + pspan->u * d_zistepu;
		z = Fix32_Recip (zi);	//1/zi, which is view z, in 16.16
		izi = (int)(zi >> 1);

		s = Fix32_MulToFix16 (sdivz, z) + sadjust;
		if (s > bbextents)
			s = bbextents;
		else if (s < 0)
			s = 0;

		t = Fix32_MulToFix16 (tdivz, z) + tadjust;
		if (t > bbextentt)
			t = bbextentt;
		else if (t < 0)
			t = 0;

		do
		{
		// calculate s and t at the far end of the span
			if (count >= 8)
				spancount = 8;
			else
				spancount = count;

			count -= spancount;

			if (count)
			{
			// calculate s/z, t/z, zi->fixed s and t at far end of span,
			// calculate s and t steps across span by shifting
				sdivz += sdivz8stepu;
				tdivz += tdivz8stepu;
				zi += zi8stepu;
				z = Fix32_Recip (zi);

				snext = Fix32_MulToFix16 (sdivz, z) + sadjust;
				if (snext > bbextents)
					snext = bbextents;
				else if (snext < 8)
					snext = 8;	// prevent round-off error on <0 steps from
								//  from causing overstepping & running off the
								//  edge of the texture

				tnext = Fix32_MulToFix16 (tdivz, z) + tadjust;
				if (tnext > bbextentt)
					tnext = bbextentt;
				else if (tnext < 8)
					tnext = 8;	// guard against round-off error on <0 steps

				sstep = (snext - s) >> 3;
				tstep = (tnext - t) >> 3;
			}
			else
			{
			// calculate s/z, t/z, zi->fixed s and t at last pixel in span (so
			// can't step off polygon), clamp, calculate s and t steps across
			// span by division, biasing steps low so we don't run off the
			// texture
				spancountminus1 = spancount - 1;
				sdivz += d_sdivzstepu * spancountminus1;
				tdivz += d_tdivzstepu * spancountminus1;
				zi += d_zistepu * spancountminus1;
				z = Fix32_Recip (zi);
				snext = Fix32_MulToFix16 (sdivz, z) + sadjust;
				if (snext > bbextents)
					snext = bbextents;
				else if (snext < 8)
					snext = 8;	// prevent round-off error on <0 steps from
								//  from causing overstepping & running off the
								//  edge of the texture

				tnext = Fix32_MulToFix16 (tdivz, z) + tadjust;
				if (tnext > bbextentt)
					tnext = bbextentt;
				else if (tnext < 8)
					tnext = 8;	// guard against round-off error on <0 steps

				if (spancount > 1)
				{
					sstep = Fix16_Mul (snext - s, r_spanrecip[spancount - 1]);
					tstep = Fix16_Mul (tnext - t, r_spanrecip[spancount - 1]);
				}
			}

			do
			{
				btemp = *(pbase + (s >> 16) + (t >> 16) * cachewidth);
				if (btemp != 255)
				{
					if (*pz <= (izi >> 16))
					{
						*pz = izi >> 16;
						*pdest = d_8to16table[btemp];
					}
				}

				izi += izistep;
				pdest++;
				pz++;
				s += sstep;
				t += tstep;
			} while (--spancount > 0);

			s = snext;
			t = tnext;

		} while (count > 0);

NextSpan:
		pspan++;

	} while (pspan->count != DS_SPAN_LIST_END);
}

#endif


/*
=====================
D_SpriteScanLeftEdge
=====================
*/
void D_SpriteScanLeftEdge (void)
{
	int			i, v, itop, ibottom, lmaxindex;
	emitpoint_t	*pvert, *pnext;
	sspan_t		*pspan;
	float		du, dv, vtop, vbottom, slope;
	fixed16_t	u, u_step;

	pspan = sprite_spans;
	i = minindex;
	if (i == 0)
		i = r_spritedesc.nump;

	lmaxindex = maxindex;
	if (lmaxindex == 0)
		lmaxindex = r_spritedesc.nump;

	vtop = ceil (r_spritedesc.pverts[i].v);

	do
	{
		pvert = &r_spritedesc.pverts[i];
		pnext = pvert - 1;

		vbottom = ceil (pnext->v);

		if (vtop < vbottom)
		{
			du = pnext->u - pvert->u;
			dv = pnext->v - pvert->v;
			slope = du / dv;
			u_step = (int)(slope * 0x10000);
		// adjust u to ceil the integer portion
			u = (int)((pvert->u + (slope * (vtop - pvert->v))) * 0x10000) +
					(0x10000 - 1);
			itop = (int)vtop;
			ibottom = (int)vbottom;

			for (v=itop ; v<ibottom ; v++)
			{
				pspan->u = u >> 16;
				pspan->v = v;
				u += u_step;
				pspan++;
			}
		}

		vtop = vbottom;

		i--;
		if (i == 0)
			i = r_spritedesc.nump;

	} while (i != lmaxindex);
}


/*
=====================
D_SpriteScanRightEdge
=====================
*/
void D_SpriteScanRightEdge (void)
{
	int			i, v, itop, ibottom;
	emitpoint_t	*pvert, *pnext;
	sspan_t		*pspan;
	float		du, dv, vtop, vbottom, slope, uvert, unext, vvert, vnext;
	fixed16_t	u, u_step;

	pspan = sprite_spans;
	i = minindex;

	vvert = r_spritedesc.pverts[i].v;
	if (vvert < r_refdef.fvrecty_adj)
		vvert = r_refdef.fvrecty_adj;
	if (vvert > r_refdef.fvrectbottom_adj)
		vvert = r_refdef.fvrectbottom_adj;

	vtop = ceil (vvert);

	do
	{
		pvert = &r_spritedesc.pverts[i];
		pnext = pvert + 1;

		vnext = pnext->v;
		if (vnext < r_refdef.fvrecty_adj)
			vnext = r_refdef.fvrecty_adj;
		if (vnext > r_refdef.fvrectbottom_adj)
			vnext = r_refdef.fvrectbottom_adj;

		vbottom = ceil (vnext);

		if (vtop < vbottom)
		{
			uvert = pvert->u;
			if (uvert < r_refdef.fvrectx_adj)
				uvert = r_refdef.fvrectx_adj;
			if (uvert > r_refdef.fvrectright_adj)
				uvert = r_refdef.fvrectright_adj;

			unext = pnext->u;
			if (unext < r_refdef.fvrectx_adj)
				unext = r_refdef.fvrectx_adj;
			if (unext > r_refdef.fvrectright_adj)
				unext = r_refdef.fvrectright_adj;

			du = unext - uvert;
			dv = vnext - vvert;
			slope = du / dv;
			u_step = (int)(slope * 0x10000);
		// adjust u to ceil the integer portion
			u = (int)((uvert + (slope * (vtop - vvert))) * 0x10000) +
					(0x10000 - 1);
			itop = (int)vtop;
			ibottom = (int)vbottom;

			for (v=itop ; v<ibottom ; v++)
			{
				pspan->count = (u >> 16) - pspan->u;
				u += u_step;
				pspan++;
			}
		}

		vtop = vbottom;
		vvert = vnext;

		i++;
		if (i == r_spritedesc.nump)
			i = 0;

	} while (i != maxindex);

	pspan->count = DS_SPAN_LIST_END;	// mark the end of the span list 
}


/*
=====================
D_SpriteCalculateGradients
=====================
*/
void D_SpriteCalculateGradients (void)
{
	fix16_t		p_normal[3], p_saxis[3], p_taxis[3], p_temp1[3];
	fix16_t		svpn[3], svright[3], svup[3];
	fix32_t		distinv, denom;

//The sprite's own axes are floats (they come from the entity's angles), so
//convert them here, once per sprite rather than once per span.
	Fix16_FromVec3 (r_spritedesc.vpn, svpn);
	Fix16_FromVec3 (r_spritedesc.vright, svright);
	Fix16_FromVec3 (r_spritedesc.vup, svup);

	p_normal[0] = Fix16_Dot3 (svpn, vright_f);
	p_normal[1] = Fix16_Dot3 (svpn, vup_f);
	p_normal[2] = Fix16_Dot3 (svpn, vpn_f);

	p_saxis[0] = Fix16_Dot3 (svright, vright_f);
	p_saxis[1] = Fix16_Dot3 (svright, vup_f);
	p_saxis[2] = Fix16_Dot3 (svright, vpn_f);

	p_taxis[0] = -Fix16_Dot3 (svup, vright_f);
	p_taxis[1] = -Fix16_Dot3 (svup, vup_f);
	p_taxis[2] = -Fix16_Dot3 (svup, vpn_f);

	denom = -Fix16_Dot3 (modelorg_f, svpn);
	if (!denom)
		denom = 1;	//keep the divide in range; the float code produced inf here
	distinv = Fix16_Recip32 ((fix16_t)denom);

	d_sdivzstepu = Fix16_Mul16ToFix32 (p_saxis[0], xscaleinv_f);
	d_tdivzstepu = Fix16_Mul16ToFix32 (p_taxis[0], xscaleinv_f);

	d_sdivzstepv = -Fix16_Mul16ToFix32 (p_saxis[1], yscaleinv_f);
	d_tdivzstepv = -Fix16_Mul16ToFix32 (p_taxis[1], yscaleinv_f);

	d_zistepu = Fix16_MulToFix32 (Fix16_Mul (p_normal[0], xscaleinv_f), distinv);
	d_zistepv = -Fix16_MulToFix32 (Fix16_Mul (p_normal[1], yscaleinv_f), distinv);

	d_sdivzorigin = Fix32_FromFix16 (p_saxis[2])
			- Fix16_MulToFix32 (xcenter_f, d_sdivzstepu)
			- Fix16_MulToFix32 (ycenter_f, d_sdivzstepv);
	d_tdivzorigin = Fix32_FromFix16 (p_taxis[2])
			- Fix16_MulToFix32 (xcenter_f, d_tdivzstepu)
			- Fix16_MulToFix32 (ycenter_f, d_tdivzstepv);
	d_ziorigin = Fix16_MulToFix32 (p_normal[2], distinv)
			- Fix16_MulToFix32 (xcenter_f, d_zistepu)
			- Fix16_MulToFix32 (ycenter_f, d_zistepv);

	TransformVectorFix (modelorg_f, p_temp1);

	sadjust = Fix16_Dot3 (p_temp1, p_saxis) -
			(-(cachewidth >> 1) << 16);
	tadjust = Fix16_Dot3 (p_temp1, p_taxis) -
			(-(sprite_height >> 1) << 16);

// -1 (-epsilon) so we never wander off the edge of the texture
	bbextents = (cachewidth << 16) - 1;
	bbextentt = (sprite_height << 16) - 1;
}


/*
=====================
D_DrawSprite
=====================
*/
void D_DrawSprite (void)
{
	int			i, nump;
	float		ymin, ymax;
	emitpoint_t	*pverts;
	static sspan_t		spans[MAXHEIGHT+1]; //pvmk - made static to squelch dangling-pointer warning

	sprite_spans = spans;

// find the top and bottom vertices, and make sure there's at least one scan to
// draw
	ymin = 999999.9;
	ymax = -999999.9;
	pverts = r_spritedesc.pverts;

	for (i=0 ; i<r_spritedesc.nump ; i++)
	{
		if (pverts->v < ymin)
		{
			ymin = pverts->v;
			minindex = i;
		}

		if (pverts->v > ymax)
		{
			ymax = pverts->v;
			maxindex = i;
		}

		pverts++;
	}

	ymin = ceil (ymin);
	ymax = ceil (ymax);

	if (ymin >= ymax)
		return;		// doesn't cross any scans at all

	cachewidth = r_spritedesc.pspriteframe->width;
	sprite_height = r_spritedesc.pspriteframe->height;
	cacheblock = (pixel_t *)&r_spritedesc.pspriteframe->pixels[0];

// copy the first vertex to the last vertex, so we don't have to deal with
// wrapping
	nump = r_spritedesc.nump;
	pverts = r_spritedesc.pverts;
	pverts[nump] = pverts[0];

	D_SpriteCalculateGradients ();
	D_SpriteScanLeftEdge ();
	D_SpriteScanRightEdge ();
	D_SpriteDrawSpans (sprite_spans);
}

