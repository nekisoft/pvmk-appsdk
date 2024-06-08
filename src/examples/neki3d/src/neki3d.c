//neki3d.c
//Demo 3D texture-mapper for Neki32
//Bryan E. Topp <betopp@betopp.com> 2024

#include <sc.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include "common.h"
#include "ply.h"
#include "mvp.h"
#include "tex.h"
#include "fb.h"
#include "span.h"


int main(int argc, const char **argv)
{
	(void)argc;
	(void)argv;
	
	ply_t *model = ply_load("cube.ply");
	if(model == NULL)
	{
		perror("load cube.ply");
		exit(-1);
	}
	
	int texnum = tex_load("cube.png");
	if(texnum == 0)
	{
		perror("load cube.png");
		exit(-1);
	}
	
	int anim = 0;
	while(1)
	{
		anim = _sc_getticks();
		
		//Prep view matrix
		mvp_ident();
		mvp_persp(FV(90), FV(1.33333), FV(1), FV(65536));
		mvp_translate(0, 0, -1024);
		mvp_rotate(FV(1) * FV(anim) / FV(10), FV(0), FV(1), FV(0));
		mvp_rotate(FV(1) * FV(anim/2) / FV(10), FV(1), FV(0), FV(0));
		
		//Transform some triangles and put into the span buffers
		for(int ii = 0; ii < model->nidxs; ii += 3)
		{
			int ia = model->idxs[ii + 0] * 8;
			int ib = model->idxs[ii + 1] * 8;
			int ic = model->idxs[ii + 2] * 8;
			
			fix24p8_t va[4] = 
			{
				model->verts[ ia + 0 ],
				model->verts[ ia + 1 ],
				model->verts[ ia + 2 ],
				256,
			};
			
			fix24p8_t vb[4] = 
			{
				model->verts[ ib + 0 ],
				model->verts[ ib + 1 ],
				model->verts[ ib + 2 ],
				256,
			};
			
			fix24p8_t vc[4] = 
			{
				model->verts[ ic + 0 ],
				model->verts[ ic + 1 ],
				model->verts[ ic + 2 ],
				256,
			};
			
			mvp_xform(va, va);
			mvp_xform(vb, vb);
			mvp_xform(vc, vc);
			
			span_add_tri(1,
				va, model->verts + ia + 6,
				vb, model->verts + ib + 6,
				vc, model->verts + ic + 6);
		}
		
		//Rasterize spans
		span_finish();
		
		//Present old image and find free buffer for new one
		fb_flip();
		
		//Get input
		_sc_input_t input = {0};
		while(_sc_input(&input, sizeof(input), sizeof(input)) > 0)
		{
			
		}
	}
}
