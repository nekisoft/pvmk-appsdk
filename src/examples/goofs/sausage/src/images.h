//images.h
//Loading of images
//Bryan E. Topp <betopp@betopp.com> 2025
#ifndef IMAGES_H
#define IMAGES_H

//Names of all images
typedef enum images_file_e
{
	IMF_NONE = 0,
	
	IMF_TITLE_AAA,
		IMF_TITLE_BG,
		IMF_TITLE_SAUSAGE,
		IMF_TITLE_TOSSER,
		IMF_TITLE_SAUSAGES,
	IMF_TITLE_ZZZ,
	
	IMF_GAME_AAA,
		IMF_GAME_BG,
		IMF_GAME_SAUSAGE0,
		IMF_GAME_SAUSAGE1,
		IMF_GAME_SAUSAGE2,
		IMF_GAME_SAUSAGE3,
		IMF_GAME_TARGET0,
		IMF_GAME_TARGET1,
		IMF_GAME_TARGET2,
		IMF_GAME_TARGET3,
		IMF_GAME_LAUNCH0,
		IMF_GAME_LAUNCH1,
		IMF_GAME_LAUNCH2,
		IMF_GAME_LAUNCH3,
		IMF_GAME_LAUNCH4,
		IMF_GAME_LAUNCH5,
		IMF_GAME_LAUNCH6,
		IMF_GAME_LAUNCH7,
		IMF_GAME_DOT,
	IMF_GAME_ZZZ,
	
	IMF_WIN_AAA,
		IMF_WIN_BG,
	IMF_WIN_ZZZ,
	
	IMF_LOSE_AAA,
		IMF_LOSE_BG,
	IMF_LOSE_ZZZ,
	
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

