//ply.c
//PLY loader
//Bryan E. Topp <betopp@betopp.com> 2024

#include "ply.h"

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

ply_t *ply_load(const char *filename)
{
	ply_t *retval = calloc(1, sizeof(ply_t));
	if(retval == NULL)
		return NULL;
	
	FILE *plyfile = fopen(filename, "rb");
	if(plyfile == NULL)
	{
		free(retval);
		return NULL;
	}
	
	//Read PLY header
	int num = 0;
	int nverts = 0;
	int nfaces = 0;
	while(1)
	{
		char linebuf[256] = {0};
		if(fgets(linebuf, sizeof(linebuf)-1, plyfile) == NULL)
			break;
		
		if(strstr(linebuf, "end_header") != NULL)
			break;
		
		if(sscanf(linebuf, " element vertex %d ", &num) == 1)
		{
			nverts = num;
			continue;
		}
		
		if(sscanf(linebuf, " element face %d ", &num) == 1)
		{
			nfaces = num;
			continue;
		}
	}
	
	//Allocate space for verts/indexes
	retval->nverts = nverts;
	retval->verts = calloc(nverts * 8, sizeof(retval->verts[0]));
	if(retval->verts == NULL)
	{
		free(retval);
		fclose(plyfile);
		return NULL;
	}
	
	retval->nidxs = nfaces * 3;
	retval->idxs = calloc(nfaces * 3, sizeof(retval->idxs[0]));
	if(retval->idxs == NULL)
	{
		free(retval->verts);
		free(retval);
		fclose(plyfile);
		return NULL;
	}

	//Read vertex data
	for(int vv = 0; vv < nverts; vv++)
	{
		float x, y, z, nx, ny, nz, s, t;
		
		fread(&x,  1, sizeof(x),  plyfile);
		fread(&y,  1, sizeof(y),  plyfile);
		fread(&z,  1, sizeof(z),  plyfile);
		fread(&nx, 1, sizeof(nx), plyfile);
		fread(&ny, 1, sizeof(ny), plyfile);
		fread(&nz, 1, sizeof(nz), plyfile);
		fread(&s,  1, sizeof(s),  plyfile);
		fread(&t,  1, sizeof(t),  plyfile);
		
		retval->verts[ (vv * 8) + 0 ] = 256 * x;
		retval->verts[ (vv * 8) + 1 ] = 256 * y;
		retval->verts[ (vv * 8) + 2 ] = 256 * z;
		retval->verts[ (vv * 8) + 3 ] = 256 * nx;
		retval->verts[ (vv * 8) + 4 ] = 256 * ny;
		retval->verts[ (vv * 8) + 5 ] = 256 * nz;
		retval->verts[ (vv * 8) + 6 ] = 256 * s;
		retval->verts[ (vv * 8) + 7 ] = 256 * t;
	}
	
	//Read index data
	for(int ff = 0; ff < nfaces; ff++)
	{
		unsigned char facetype = 0;
		fread(&facetype, 1, sizeof(facetype), plyfile);
		if(facetype != 3)
		{
			//Not a triangle, skip
			for(int ii = 0; ii < facetype; ii++)
			{
				int dummy = 0;
				fread(&dummy, 1, sizeof(dummy), plyfile);
			}
		}
		else
		{
			//Triangle
			int va, vb, vc;
			fread(&va, 1, sizeof(va), plyfile);
			fread(&vb, 1, sizeof(vb), plyfile);
			fread(&vc, 1, sizeof(vc), plyfile);
			
			retval->idxs[ (3 * ff) + 0 ] = va;
			retval->idxs[ (3 * ff) + 1 ] = vb;
			retval->idxs[ (3 * ff) + 2 ] = vc;
		}
	}
	
	return retval;
}

void ply_free(ply_t *ply)
{
	if(ply->verts != NULL)
		free(ply->verts);
	
	if(ply->idxs != NULL)
		free(ply->idxs);
	
	free(ply);
}

