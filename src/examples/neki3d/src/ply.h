//ply.h
//PLY loader
//Bryan E. Topp <betopp@betopp.com> 2024
#ifndef PLY_H
#define PLY_H

#include "common.h"

//Contents of PLY file
typedef struct ply_s 
{
	fix24p8_t *verts;
	int nverts;
	
	fix24p8_t *idxs;
	int nidxs;
} ply_t;

//Loads a PLY file.
ply_t *ply_load(const char *filename);

//Discards a loaded PLY file.
void ply_free(ply_t *ply);

#endif //PLY_H
