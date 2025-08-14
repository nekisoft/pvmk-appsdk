//png2bigscroll.c
//Converts PNG image to chunks for "bigscroll" example
//Bryan E. Topp <betopp@betopp.com> 2025

#include <stdio.h>
#include <stdint.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

int main(int argc, const char **argv)
{
	if(argc != 3)
	{
		printf("usage: %s <png input file> <output prefix>\n", argv[0]);
		return -1;
	}
	
	//Load incoming image
	int bigx = 0;
	int bigy = 0;
	int bign = 0;
	const uint8_t *bigpic = stbi_load(argv[1], &bigx, &bigy, &bign, 3);
	if(bigpic == NULL)
	{
		printf("Failed to load input image %s.\n", argv[1]);
		return -1;
	}
	
	//Spit out in 64x64 chunks, RGB565
	for(int chunky = 0; chunky < bigy / 64; chunky++)
	{
		const uint8_t *chunky_source = bigpic + (3 * bigx * 64 * chunky);
		for(int chunkx = 0; chunkx < bigx / 64; chunkx++)
		{
			const uint8_t *chunkx_source = chunky_source + (3 * 64 * chunkx);
			
			char chunkfilename[4096] = {0};
			snprintf(chunkfilename, sizeof(chunkfilename)-1, "%s.%4.4d.%4.4d.565", argv[2], chunky, chunkx);
			FILE *chunkfile = fopen(chunkfilename, "wb");
			if(chunkfile == NULL)
			{
				printf("Failed to open %s for writing.\n", chunkfilename);
				return -1;
			}
			
			for(int rr = 0; rr < 64; rr++)
			{
				const uint8_t *row_source = chunkx_source + (3 * bigx * rr);
				for(int cc = 0; cc < 64; cc++)
				{
					const uint8_t *col_source = row_source + (3 * cc);
					
					uint8_t r = col_source[0];
					uint8_t g = col_source[1];
					uint8_t b = col_source[2];
					
					uint16_t packed = 0;
					packed |= (r >> 3) <<  0;
					packed |= (g >> 2) <<  5;
					packed |= (b >> 3) << 11;
					
					fwrite(&packed, 1, 2, chunkfile);
				}
			}
			
			fclose(chunkfile);
		}
	}
	
	printf("All done\n");
	return 0;
}
