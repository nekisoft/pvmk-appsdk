//tex.h
//Texture loading
//Bryan E. Topp <betopp@betopp.com> 2024
#ifndef TEX_H
#define TEX_H

#include "fb.h"

//Loads a texture and returns its index
int tex_load(const char *filename);

//Returns a pointer to texture data
fbpx_t *tex_data(int tt);

//Frees a texture
void tex_free(int tt);

#endif //TEX_H
