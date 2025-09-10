//proj.c
//Projection/3D drawing functions
//Bryan E. Topp <betopp@betopp.com> 2025

#include "proj.h"
#include <stdlib.h>

//Currently set camera position
static int32_t proj_campos[3];

//Currently set camera radius/FOV
static int32_t proj_camrad;

//Enqueued cards to draw, with their coordinates already reduced to screen-space
typedef struct proj_element_s
{
	//Depth of the element for sorting - higher is further away
	int depth;
	
	//Image file to draw
	images_file_t imf;
	
	//Screen position to draw the bottom-center of the image
	int sx, sy;
	
	//Screen height in pixels of the image when scaled
	int sh;
	
} proj_element_t;
#define PROJ_ELEMENT_MAX 256
static proj_element_t proj_element_table[PROJ_ELEMENT_MAX];
static int proj_element_used;

//Comparator for sorting elements in depth order
int proj_element_comparator(const void *a, const void *b)
{
	const proj_element_t *ea = (const proj_element_t *)a;
	const proj_element_t *eb = (const proj_element_t *)b;
	return eb->depth - ea->depth;
}

void proj_cam(int32_t cx8, int32_t cy8, int32_t cz8, int32_t fov)
{
	proj_campos[0] = cx8;
	proj_campos[1] = cy8;
	proj_campos[2] = cz8;
	proj_camrad = fov;
}

void proj_card(images_file_t imf, int32_t vx8, int32_t vy8, int32_t vz8, int32_t vh8)
{
	if(proj_camrad < 1)
		proj_camrad = 1;
	
	int vx_rel = vx8 - proj_campos[0];
	int vy_rel = vy8 - proj_campos[1];
	int vz_rel = vz8 - proj_campos[2];
	
	int sy = 1024 * ((proj_camrad * 360) - vy_rel ) / ((proj_camrad * 1536) + vy_rel);	
	int sx = 320 +  (vx_rel * (1024 + sy) / (proj_camrad * 1024));
	int sh =        (   vh8 * (1024 + sy) / (proj_camrad * 1024));
	
	sy -=           (vz_rel * (1024 + sy) / (proj_camrad * (1024))); //not really correct but whatever
	
	sy+=(sh/8);
	
	if(proj_element_used >= PROJ_ELEMENT_MAX)
	{
		//Whoopsie doodle
		return;
	}
	
	proj_element_t *eptr = &(proj_element_table[proj_element_used]);
	eptr->imf = imf;
	eptr->sh = sh;
	eptr->sx = sx;
	eptr->sy = sy;
	eptr->depth = vy_rel - vz_rel;
	
	proj_element_used++;
}

void proj_draw(void)
{
	//Sort elements by depth
	qsort(proj_element_table, proj_element_used, sizeof(proj_element_table[0]), &proj_element_comparator);
	
	//Draw them all
	for(int ee = 0; ee < proj_element_used; ee++)
	{
		const proj_element_t *eptr = &(proj_element_table[ee]);
		images_card(eptr->imf, eptr->sx, eptr->sy, eptr->sh);
	}
	
	//Clear queue
	proj_element_used = 0;
}

