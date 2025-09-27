//images.h
//Loading of images
//Bryan E. Topp <betopp@betopp.com> 2025
#ifndef IMAGES_H
#define IMAGES_H

//Names of all images
typedef enum images_file_e
{
	IMF_NONE = 0,
	
	IMF_BG1,
	IMF_BG2,
	IMF_BG3,
	IMF_LOGO,

	
	IMF_MAX
	
} images_file_t;
	
//Purges all loaded images
void images_purge(void);

//Loads an image
void images_load(images_file_t fn);

//Loads a range of images
void images_loadrange(images_file_t min, images_file_t max);

//Blends an image onto the screen
void images_draw(images_file_t fn, int x, int y);

//Draws a scaled image, x and y specifying bottom-center
void images_card(images_file_t fn, int x, int y, int height);

#endif //IMAGES_H

