//pngscr.c
//Converts PNG to fullscreen 16bpp 640x480 image
//Bryan E. Topp <betopp@betopp.com> 2024

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

int main(int argc, const char **argv)
{
	if(argc < 3)
	{
		printf("Usage: %s <input png> <output scr>\n", argv[0]);
		exit(-1);
	}
	
	int im_x, im_y, im_n;
	uint8_t *png32 = stbi_load(argv[1], &im_x, &im_y, &im_n, 4);
	if(png32 == NULL)
	{
		printf("Failed to load %s\n", argv[1]);
		exit(-1);
	}
	
	FILE *outf = fopen(argv[2], "wb");
	if(outf == NULL)
	{
		printf("Failed to open %s\n", argv[2]);
		exit(-1);
	}
	
	uint8_t *srcline = png32;
	for(int yy = 0; yy < 480; yy++)
	{
		uint8_t *srcptr = srcline;
		for(int xx = 0; xx < 640; xx++)
		{
			uint16_t outpx = 0;
			if(yy < im_y && xx < im_x)
			{
				outpx |= (srcptr[2] >> 3) <<  0;
				outpx |= (srcptr[1] >> 2) <<  5;
				outpx |= (srcptr[0] >> 3) << 11;
			}
			fwrite(&outpx, 1, 2, outf);
			
			srcptr += 4;
		}
		srcline += im_x * 4;
	}
	
	fclose(outf);
	exit(0);
}