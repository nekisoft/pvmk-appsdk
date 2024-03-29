/*
 * OpenBOR - https://www.chronocrash.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c)  OpenBOR Team
 */
 

//Hacked up by betopp to use STB_Image, reducing library dependencies
#define STB_IMAGE_IMPLEMENTATION 1
#define STBI_ONLY_PNG 1
#define STBI_NO_THREAD_LOCALS 1
#include "stb_image.h"

#include "../gamelib/types.h"
#include "../gamelib/screen.h"
#include <assert.h>

s_screen *pngToScreen(const void *data)
{	
	//Unused stuff throwing warnings...
	(void)stbi__mul2shorts_valid(0,0);
	(void)stbi__addints_valid(0,0);
	
	int w, h, n;
	void *pxdata = stbi_load_from_memory(data, 4*1024*1024, &w, &h, &n, 4);
	if(pxdata == NULL)
	{
		//Failed to load image data
		return NULL;
	}
	
	s_screen *retval = allocscreen(w, h, PIXEL_32);
	if(retval == NULL)
	{
		//Failed to allocate screen buffer
		stbi_image_free(pxdata);
		return NULL;
	}
	
	uint32_t *srcline = (uint32_t *)pxdata;
	uint32_t *dstline = (uint32_t *)retval->data;
	for(int yy = 0; yy < h; yy++)
	{
		memcpy(dstline, srcline, sizeof(dstline[0])*w);
		srcline += w;
		dstline += w;
	}
	
	stbi_image_free(pxdata);
	return retval;
}

void savepng(const char *filename, s_screen *screen, u8 *pal) {(void)filename; (void)screen; (void)pal;}