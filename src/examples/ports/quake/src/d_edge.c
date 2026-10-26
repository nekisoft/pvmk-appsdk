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
// d_edge.c

#include "quakedef.h"
#include "d_local.h"

static int	miplevel;

float		scale_for_mip;
extern int			screenwidth;
int			ubasestep, errorterm, erroradjustup, erroradjustdown;
int			vstartscan;

// FIXME: should go away
extern void			R_RotateBmodel (void);
extern void			R_TransformFrustum (void);

vec3_t		transformed_modelorg;
fix16_t		transformed_modelorg_f[3];	//16.16, for the span drawer

/*
==============
D_DrawPoly

==============
*/
void D_DrawPoly (void)
{
// this driver takes spans, not polygons
}


/*
=============
D_MipLevelForScale
=============
*/
int D_MipLevelForScale (float scale)
{
	int		lmiplevel;

	if (scale >= d_scalemip[0] )
		lmiplevel = 0;
	else if (scale >= d_scalemip[1] )
		lmiplevel = 1;
	else if (scale >= d_scalemip[2] )
		lmiplevel = 2;
	else
		lmiplevel = 3;

	if (lmiplevel < d_minmip)
		lmiplevel = d_minmip;

	return lmiplevel;
}


/*
==============
D_DrawSolidSurface
==============
*/

// FIXME: clean this up

void D_DrawSolidSurface (surf_t *surf, int color)
{
	espan_t	*span;
	unsigned short	*pdest;
	int		u, u2, pix;
	
	pix = (color<<24) | (color<<16) | (color<<8) | color;
	for (span=surf->spans ; span ; span=span->pnext)
	{
		pdest = (pixel_t *)d_viewbuffer + screenwidth*span->v;
		u = span->u;
		u2 = span->u + span->count - 1;
		((byte *)pdest)[u] = pix;

		if (u2 - u < 8)
		{
			for (u++ ; u <= u2 ; u++)
				((byte *)pdest)[u] = pix;
		}
		else
		{
			for (u++ ; u & 3 ; u++)
				((byte *)pdest)[u] = pix;

			u2 -= 4;
			for ( ; u <= u2 ; u+=4)
				*(int *)((byte *)pdest + u) = pix;
			u2 += 4;
			for ( ; u <= u2 ; u++)
				((byte *)pdest)[u] = pix;
		}
	}
}


/*
==============
D_CalcGradients
==============
*/
void D_CalcGradients (msurface_t *pface)
{
	fix16_t		mipscale;	//1/(1<<miplevel), as 16.16
	fix16_t		p_temp1[3];
	fix16_t		p_saxis[3], p_taxis[3];
	fix32_t		t;

	mipscale = (fix16_t)(FIX16_ONE >> miplevel);

	p_saxis[0] = Fix16_Dot3 (pface->texinfo->fvecs[0], vright_f);
	p_saxis[1] = Fix16_Dot3 (pface->texinfo->fvecs[0], vup_f);
	p_saxis[2] = Fix16_Dot3 (pface->texinfo->fvecs[0], vpn_f);
	p_taxis[0] = Fix16_Dot3 (pface->texinfo->fvecs[1], vright_f);
	p_taxis[1] = Fix16_Dot3 (pface->texinfo->fvecs[1], vup_f);
	p_taxis[2] = Fix16_Dot3 (pface->texinfo->fvecs[1], vpn_f);

//How a texture axis moves per screen pixel, in 32.32. The 16.16 axis times
//the 16.16 pixel scale gives the 32.32 step directly.
	t = Fix16_Mul16ToFix32 (xscaleinv_f, mipscale);
	d_sdivzstepu = Fix16_MulToFix32 (p_saxis[0], t);
	d_tdivzstepu = Fix16_MulToFix32 (p_taxis[0], t);

	t = Fix16_Mul16ToFix32 (yscaleinv_f, mipscale);
	d_sdivzstepv = -Fix16_MulToFix32 (p_saxis[1], t);
	d_tdivzstepv = -Fix16_MulToFix32 (p_taxis[1], t);

	d_sdivzorigin = Fix16_Mul16ToFix32 (p_saxis[2], mipscale)
			- Fix16_MulToFix32 (xcenter_f, d_sdivzstepu)
			- Fix16_MulToFix32 (ycenter_f, d_sdivzstepv);
	d_tdivzorigin = Fix16_Mul16ToFix32 (p_taxis[2], mipscale)
			- Fix16_MulToFix32 (xcenter_f, d_tdivzstepu)
			- Fix16_MulToFix32 (ycenter_f, d_tdivzstepv);

	p_temp1[0] = Fix16_Mul (transformed_modelorg_f[0], mipscale);
	p_temp1[1] = Fix16_Mul (transformed_modelorg_f[1], mipscale);
	p_temp1[2] = Fix16_Mul (transformed_modelorg_f[2], mipscale);

//The float build adds 0.5 here before truncating, but its dot product is the
//low precision one, so it lands below what an exact 16.16 product gives;
//rounding would move every surface half a texel the wrong way.
	sadjust = Fix16_Dot3 (p_temp1, p_saxis) -
			((pface->texturemins[0] << 16) >> miplevel)
			+ Fix16_Mul (pface->texinfo->fvecs[0][3], mipscale);
	tadjust = Fix16_Dot3 (p_temp1, p_taxis) -
			((pface->texturemins[1] << 16) >> miplevel)
			+ Fix16_Mul (pface->texinfo->fvecs[1][3], mipscale);

//
// -1 (-epsilon) so we never wander off the edge of the texture
//
	bbextents = ((pface->extents[0] << 16) >> miplevel) - 1;
	bbextentt = ((pface->extents[1] << 16) >> miplevel) - 1;
}


/*
==============
D_DrawSurfaces
==============
*/
void D_DrawSurfaces (void)
{
	surf_t			*s;
	msurface_t		*pface;
	surfcache_t		*pcurrentcache;
	vec3_t			world_transformed_modelorg;
	vec3_t			local_modelorg;
	fix16_t			world_transformed_modelorg_f[3];
	fix16_t			local_modelorg_f[3];

	currententity = &cl_entities[0];
	TransformVector (modelorg, transformed_modelorg);
	VectorCopy (transformed_modelorg, world_transformed_modelorg);
	TransformVectorFix (modelorg_f, transformed_modelorg_f);
	Fix16_Copy3 (transformed_modelorg_f, world_transformed_modelorg_f);

// TODO: could preset a lot of this at mode set time
	if (r_drawflat.value)
	{
		for (s = &surfaces[1] ; s<surface_p ; s++)
		{
			if (!s->spans)
				continue;

			d_zistepu = s->d_zistepu;
			d_zistepv = s->d_zistepv;
			d_ziorigin = s->d_ziorigin;

			D_DrawSolidSurface (s, (int)s->data & 0xFF);
			D_DrawZSpans (s->spans);
		}
	}
	else
	{
		for (s = &surfaces[1] ; s<surface_p ; s++)
		{
			if (!s->spans)
				continue;

			r_drawnpolycount++;

			d_zistepu = s->d_zistepu;
			d_zistepv = s->d_zistepv;
			d_ziorigin = s->d_ziorigin;

			if (s->flags & SURF_DRAWSKY)
			{
				if (!r_skymade)
				{
					R_MakeSky ();
				}

				D_DrawSkyScans8 (s->spans);
				D_DrawZSpans (s->spans);
			}
			else if (s->flags & SURF_DRAWBACKGROUND)
			{
			// set up a gradient for the background surface that places it
			// effectively at infinity distance from the viewpoint
				d_zistepu = 0;
				d_zistepv = 0;
				d_ziorigin = -(fix32_t)3865470566LL;	//-0.9 in 32.32

				D_DrawSolidSurface (s, (int)r_clearcolor.value & 0xFF);
				D_DrawZSpans (s->spans);
			}
			else if (s->flags & SURF_DRAWTURB)
			{
				pface = s->data;
				miplevel = 0;
				cacheblock = /*(pixel_t *)*/
						((pixel_t *)pface->texinfo->texture +
						pface->texinfo->texture->offsets[0]);
				cachewidth = 64;

				if (s->insubmodel)
				{
				// FIXME: we don't want to do all this for every polygon!
				// TODO: store once at start of frame
					currententity = s->entity;	//FIXME: make this passed in to
												// R_RotateBmodel ()
					VectorSubtract (r_origin, currententity->origin,
							local_modelorg);
							TransformVector (local_modelorg, transformed_modelorg);
							Fix16_FromVec3 (currententity->origin, local_modelorg_f);
					Fix16_Sub3 (r_origin_f, local_modelorg_f, local_modelorg_f);
							TransformVectorFix (local_modelorg_f, transformed_modelorg_f);

					R_RotateBmodel ();	// FIXME: don't mess with the frustum,
										// make entity passed in
				}

				D_CalcGradients (pface);
				Turbulent8 (s->spans);
				D_DrawZSpans (s->spans);

				if (s->insubmodel)
				{
				//
				// restore the old drawing state
				// FIXME: we don't want to do this every time!
				// TODO: speed up
				//
					currententity = &cl_entities[0];
					VectorCopy (world_transformed_modelorg,
							transformed_modelorg);
					Fix16_Copy3 (world_transformed_modelorg_f,
							transformed_modelorg_f);
					VectorCopy (base_vpn, vpn);
					VectorCopy (base_vup, vup);
					VectorCopy (base_vright, vright);
					VectorCopy (base_modelorg, modelorg);
					Fix16_Copy3 (base_vpn_f, vpn_f);
					Fix16_Copy3 (base_vup_f, vup_f);
					Fix16_Copy3 (base_vright_f, vright_f);
					Fix16_Copy3 (base_modelorg_f, modelorg_f);
					R_TransformFrustum ();
				}
			}
			else
			{
				if (s->insubmodel)
				{
				// FIXME: we don't want to do all this for every polygon!
				// TODO: store once at start of frame
					currententity = s->entity;	//FIXME: make this passed in to
												// R_RotateBmodel ()
					VectorSubtract (r_origin, currententity->origin, local_modelorg);
					TransformVector (local_modelorg, transformed_modelorg);
					Fix16_FromVec3 (currententity->origin, local_modelorg_f);
					Fix16_Sub3 (r_origin_f, local_modelorg_f, local_modelorg_f);
					TransformVectorFix (local_modelorg_f, transformed_modelorg_f);

					R_RotateBmodel ();	// FIXME: don't mess with the frustum,
										// make entity passed in
				}

				pface = s->data;
				miplevel = D_MipLevelForScale (s->nearzi * scale_for_mip
				* pface->texinfo->mipadjust);

			// FIXME: make this passed in to D_CacheSurface
				pcurrentcache = D_CacheSurface (pface, miplevel);

				cacheblock = (pixel_t *)pcurrentcache->data;
				cachewidth = pcurrentcache->width;

				D_CalcGradients (pface);

				(*d_drawspans) (s->spans);

				D_DrawZSpans (s->spans);

				if (s->insubmodel)
				{
				//
				// restore the old drawing state
				// FIXME: we don't want to do this every time!
				// TODO: speed up
				//
					currententity = &cl_entities[0];
					VectorCopy (world_transformed_modelorg,
							transformed_modelorg);
					Fix16_Copy3 (world_transformed_modelorg_f,
							transformed_modelorg_f);
					VectorCopy (base_vpn, vpn);
					VectorCopy (base_vup, vup);
					VectorCopy (base_vright, vright);
					VectorCopy (base_modelorg, modelorg);
					Fix16_Copy3 (base_vpn_f, vpn_f);
					Fix16_Copy3 (base_vup_f, vup_f);
					Fix16_Copy3 (base_vright_f, vright_f);
					Fix16_Copy3 (base_modelorg_f, modelorg_f);
					R_TransformFrustum ();
				}
			}
		}
	}
}

