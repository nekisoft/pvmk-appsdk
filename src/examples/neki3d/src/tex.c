//tex.c
//Texture loading
//Bryan E. Topp <betopp@betopp.com> 2024

#include "tex.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG 1
#define STBI_NO_THREAD_LOCALS 1
#include "stb_image.h"

//Texture memory, MIP levels
static fbpx_t textures  [64][256][256];
static fbpx_t textures_1[64][128][128];
static fbpx_t textures_2[64][ 64][ 64];
static fbpx_t textures_3[64][ 32][ 32];
static fbpx_t textures_4[64][ 16][ 16];
static fbpx_t textures_5[64][  8][  8];

//Which textures are allocated
static uint64_t textures_alloc_bitmap = 1;

//Turns loaded color data into RGB565
static fbpx_t tex_makepx(int r8, int g8, int b8)
{
	int b5 = ((b8 >> 3) <<  0) & 0x001F;
	int g6 = ((g8 >> 2) <<  5) & 0x07E0;
	int r5 = ((r8 >> 3) << 11) & 0xF800;
	return r5 | g6 | b5;
}

//Averages four RGB565 colors
static fbpx_t tex_avg(fbpx_t a, fbpx_t b, fbpx_t c, fbpx_t d)
{
	//Color A components
	const int ra = a & 0x001F;
	const int ga = a & 0x07E0;
	const int ba = a & 0xF800;
	
	//Color B components
	const int rb = b & 0x001F;
	const int gb = b & 0x07E0;
	const int bb = b & 0xF800;
	
	//Color C components
	const int rc = c & 0x001F;
	const int gc = c & 0x07E0;
	const int bc = c & 0xF800;
	
	//Color D components
	const int rd = d & 0x001F;
	const int gd = d & 0x07E0;
	const int bd = d & 0xF800;
	
	//Averages
	int rp = (ra + rb + rc + rd) / 4;
	int gp = (ga + gb + gc + gd) / 4;
	int bp = (ba + bb + bc + bd) / 4;
	
	//Put it back together
	return (rp & 0x001F) | (gp & 0x07E0) | (bp & 0xF800);
}

int tex_load(const char *filename)
{
	//Find free texture index for loading
	int newidx = 0;
	while( textures_alloc_bitmap & (1ull << newidx) )
		newidx++;
	
	if(newidx >= 64)
		return 0;
	
	//Load image from disk
	int imx, imy, imn;
	uint8_t *imgdata = stbi_load(filename, &imx, &imy, &imn, 3);
	if(imgdata == NULL)
		return 0;
	
	//Build largest image
	for(int dy = 0; dy < 256; dy++)
	{
		int sy = dy * imy / 256;
		for(int dx = 0; dx < 256; dx++)
		{
			int sx = dx * imx / 256;
			
			int rr = imgdata[ (((imx * sy) + sx) * 3) + 0 ];
			int gg = imgdata[ (((imx * sy) + sx) * 3) + 1 ];
			int bb = imgdata[ (((imx * sy) + sx) * 3) + 2 ];
			
			textures[newidx][dy][dx] = tex_makepx(rr, gg, bb);
		}
	}
	
	stbi_image_free(imgdata);
	
	//Build MIP levels
	for(int dy = 0; dy < 128; dy++)
	{
		for(int dx = 0; dx < 128; dx++)
		{
			fbpx_t ta = textures[newidx][(dy*2)+0][(dx*2)+0];
			fbpx_t tb = textures[newidx][(dy*2)+0][(dx*2)+1];
			fbpx_t tc = textures[newidx][(dy*2)+1][(dx*2)+0];
			fbpx_t td = textures[newidx][(dy*2)+1][(dx*2)+1];
			
			textures_1[newidx][dy][dx] = tex_avg(ta, tb, tc, td);
		}
	}
	
	for(int dy = 0; dy < 64; dy++)
	{
		for(int dx = 0; dx < 64; dx++)
		{
			fbpx_t ta = textures_1[newidx][(dy*2)+0][(dx*2)+0];
			fbpx_t tb = textures_1[newidx][(dy*2)+0][(dx*2)+1];
			fbpx_t tc = textures_1[newidx][(dy*2)+1][(dx*2)+0];
			fbpx_t td = textures_1[newidx][(dy*2)+1][(dx*2)+1];
			
			textures_2[newidx][dy][dx] = tex_avg(ta, tb, tc, td);
		}
	}

	for(int dy = 0; dy < 32; dy++)
	{
		for(int dx = 0; dx < 32; dx++)
		{
			fbpx_t ta = textures_2[newidx][(dy*2)+0][(dx*2)+0];
			fbpx_t tb = textures_2[newidx][(dy*2)+0][(dx*2)+1];
			fbpx_t tc = textures_2[newidx][(dy*2)+1][(dx*2)+0];
			fbpx_t td = textures_2[newidx][(dy*2)+1][(dx*2)+1];
			
			textures_3[newidx][dy][dx] = tex_avg(ta, tb, tc, td);
		}
	}

	for(int dy = 0; dy < 16; dy++)
	{
		for(int dx = 0; dx < 16; dx++)
		{
			fbpx_t ta = textures_3[newidx][(dy*2)+0][(dx*2)+0];
			fbpx_t tb = textures_3[newidx][(dy*2)+0][(dx*2)+1];
			fbpx_t tc = textures_3[newidx][(dy*2)+1][(dx*2)+0];
			fbpx_t td = textures_3[newidx][(dy*2)+1][(dx*2)+1];
			
			textures_4[newidx][dy][dx] = tex_avg(ta, tb, tc, td);
		}
	}

	for(int dy = 0; dy < 8; dy++)
	{
		for(int dx = 0; dx < 8; dx++)
		{
			fbpx_t ta = textures_4[newidx][(dy*2)+0][(dx*2)+0];
			fbpx_t tb = textures_4[newidx][(dy*2)+0][(dx*2)+1];
			fbpx_t tc = textures_4[newidx][(dy*2)+1][(dx*2)+0];
			fbpx_t td = textures_4[newidx][(dy*2)+1][(dx*2)+1];
			
			textures_5[newidx][dy][dx] = tex_avg(ta, tb, tc, td);
		}
	}	
	
	return newidx;
}

void tex_free(int tt)
{
	if(tt <= 0 || tt >= 64)
		return;
	
	textures_alloc_bitmap &= ~(1ull << tt);
}

fbpx_t *tex_data(int tt)
{
	return &(textures_3[tt][0][0]);
}
