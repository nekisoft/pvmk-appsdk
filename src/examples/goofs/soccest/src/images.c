//images.c
//Loading of images in soccer game
//Bryan E. Topp <betopp@betopp.com> 2025

#include "images.h"
#include "fbs.h"

//Actual file loading code from STB_Image
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

//Filenames for each image
static const char *images_file_names[IMF_MAX] = 
{
	[IMF_TITLE_BG]    = "screens/title/bg.png",
	[IMF_TITLE_LOGO]  = "screens/title/logo.png",
	[IMF_TITLE_START] = "screens/title/start.png",
	[IMF_TITLE_BALL]  = "screens/title/ball.png",
	
	[IMF_MAINMENU_BG]     = "screens/mainmenu/bg.png",
	[IMF_MAINMENU_BALL]   = "screens/mainmenu/ball.png",
	[IMF_MAINMENU_ITEM1]  = "screens/mainmenu/item1.png",
	[IMF_MAINMENU_ITEM2]  = "screens/mainmenu/item2.png",
	[IMF_MAINMENU_ITEM3]  = "screens/mainmenu/item3.png",
	[IMF_MAINMENU_ITEMBG] = "screens/mainmenu/itembg.png",
	[IMF_MAINMENU_TITLE]  = "screens/mainmenu/title.png",
	
	[IMF_TEAMSELECT_BG]      = "screens/teamselect/bg.png",
	[IMF_TEAMSELECT_HEADERS] = "screens/teamselect/headers.png",
	[IMF_TEAMSELECT_LOGO]    = "screens/teamselect/logo.png",
	[IMF_TEAMSELECT_SHIRTS]  = "screens/teamselect/shirts.png",
	
	[IMF_PADTEAMS_BG]      = "screens/padteams/bg.png",
	[IMF_PADTEAMS_RED]     = "screens/padteams/red.png",
	[IMF_PADTEAMS_BLUE]    = "screens/padteams/blue.png",
	[IMF_PADTEAMS_TEAMS]   = "screens/padteams/teams.png",
	[IMF_PADTEAMS_LETTERS] = "screens/padteams/letters.png",
	[IMF_PADTEAMS_HEAD]    = "screens/padteams/head.png",
	[IMF_PADTEAMS_PAD]     = "screens/padteams/pad.png",
	
	[IMF_MATCHOPT_BG]     = "screens/matchopt/bg.png",
	[IMF_MATCHOPT_CHALK]  = "screens/matchopt/chalk.png",
	[IMF_MATCHOPT_ERASER] = "screens/matchopt/eraser.png",
	[IMF_MATCHOPT_TITLE]  = "screens/matchopt/title.png",
	[IMF_MATCHOPT_VUVU]   = "screens/matchopt/vuvu.png",
	
	[IMF_CARD_CONE]       = "cards/cone.png",
	[IMF_CARD_BALL0]       = "cards/ball0.png",
	[IMF_CARD_BALL1]       = "cards/ball1.png",
	[IMF_CARD_BALL2]       = "cards/ball2.png",
	[IMF_CARD_BALL3]       = "cards/ball3.png",
	[IMF_CARD_BALLSH]      = "cards/ballsh.png",
};

//Information about each file if loaded
typedef struct images_info_s
{
	uint16_t *pixels;
	int x;
	int y;
} images_info_t;
images_info_t images_info[IMF_MAX];

void images_purge(void)
{
	for(int ii = 0; ii < IMF_MAX; ii++)
	{
		if(images_info[ii].pixels != NULL)
			free(images_info[ii].pixels);
		
		memset(&(images_info[ii]), 0, sizeof(images_info[ii]));
	}
}

void images_load(images_file_t fn)
{
	if(images_info[fn].pixels != NULL)
		return; //Already loaded
	
	int xx = 0;
	int yy = 0;
	int nn = 0;
	uint8_t *data = stbi_load(images_file_names[fn], &xx, &yy, &nn, 4);
	if(data == NULL)
	{
		//Failed to load image
		return;
	}
	
	images_info[fn].pixels = (uint16_t*)malloc(sizeof(uint16_t)*xx*yy);
	if(images_info[fn].pixels == NULL)
	{
		//Out of memory
		stbi_image_free(data);
		return;
	}
	
	for(int pp = 0; pp < (xx*yy); pp++)
	{
		uint16_t rgb565 = 0;
		rgb565 |= ((data[ (pp*4) + 2 ]) >> 3) <<  0;
		rgb565 |= ((data[ (pp*4) + 1 ]) >> 2) <<  5;
		rgb565 |= ((data[ (pp*4) + 0 ]) >> 3) << 11;
		if(data[ (pp*4) + 3 ] < 127)
			rgb565 = 0;
		else if(rgb565 == 0)
			rgb565 = 1;
		
		images_info[fn].pixels[pp] = rgb565;
	}
	stbi_image_free(data);
	
	images_info[fn].x = xx;
	images_info[fn].y = yy;
	return; //Success
}

void images_loadrange(images_file_t min, images_file_t max)
{
	for(int rr = (int)min; rr <= (int)max; rr++)
	{
		images_load(rr);
	}
}

void images_draw(images_file_t fn, int x, int y)
{
	if(images_info[fn].pixels == NULL)
		return;
	
	uint16_t *image_line = images_info[fn].pixels;
	for(int yy = y; yy < y + images_info[fn].y; yy++)
	{
		if(yy < 0)
		{
			image_line += images_info[fn].x;
			continue;
		}
		
		if(yy >= SCREENY)
			break;
		
		uint16_t *image_pixel = image_line;
		for(int xx = x; xx < x + images_info[fn].x; xx++)
		{
			if(xx < 0)
			{
				image_pixel++;
				continue;
			}
			
			if(xx >= SCREENX)
				break;
			
			if(*image_pixel)
				BACKBUF[yy][xx] = *image_pixel;
			
			image_pixel++;
		}
		image_line += images_info[fn].x;
	}
	
}

void images_card(images_file_t fn, int x, int y, int height)
{
	if(images_info[fn].pixels == NULL)
		return;
	
	//Assume image scales 1:1 - compute width given desired height
	int image_height = images_info[fn].y;
	int image_width = images_info[fn].x;
	int width = image_width * height / image_height;
	
	//2D bresenham to scale the image
	int yfrac = 0;
	uint16_t *image_line = images_info[fn].pixels;
	for(int yy = y - height; yy < y; yy++)
	{
		yfrac += image_height;
		while(yfrac > height)
		{
			image_line += images_info[fn].x;
			yfrac -= height;
		}
		
		if(yy < 0)
			continue;
		if(yy >= SCREENY)
			break;
		
		int xfrac = 0;
		uint16_t *image_pixel = image_line;
		for(int xx = x - (width/2); xx < x + (width/2); xx++)
		{
			xfrac += image_width;
			while(xfrac > width)
			{
				image_pixel++;
				xfrac -= width;
			}
			
			if(xx < 0)
				continue;
			if(xx >= SCREENX)
				break;
			
			if(*image_pixel)
				BACKBUF[yy][xx] = *image_pixel;
		}
	}
}
