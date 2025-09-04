//images.h
//Loading of images in soccer game
//Bryan E. Topp <betopp@betopp.com> 2025
#ifndef IMAGES_H
#define IMAGES_H

//Names of all images
typedef enum images_file_e
{
	IMF_NONE = 0,
	
	IMF_TITLE_AAA,
		IMF_TITLE_BG,
		IMF_TITLE_LOGO,
		IMF_TITLE_START,
		IMF_TITLE_BALL,
	IMF_TITLE_ZZZ,
	
	IMF_MAINMENU_AAA,
		IMF_MAINMENU_BG,
		IMF_MAINMENU_BALL,
		IMF_MAINMENU_ITEM1,
		IMF_MAINMENU_ITEM2,
		IMF_MAINMENU_ITEM3,
		IMF_MAINMENU_ITEMBG,
		IMF_MAINMENU_TITLE,
	IMF_MAINMENU_ZZZ,
	
	IMF_TEAMSELECT_AAA,
		IMF_TEAMSELECT_BG,
		IMF_TEAMSELECT_HEADERS,
		IMF_TEAMSELECT_LOGO,
		IMF_TEAMSELECT_SHIRTS,
	IMF_TEAMSELECT_ZZZ,
	
	IMF_PADTEAMS_AAA,
		IMF_PADTEAMS_BG,
		IMF_PADTEAMS_RED,
		IMF_PADTEAMS_BLUE,
		IMF_PADTEAMS_TEAMS,
		IMF_PADTEAMS_LETTERS,
		IMF_PADTEAMS_HEAD,
		IMF_PADTEAMS_PAD,
	IMF_PADTEAMS_ZZZ,
	
	IMF_MATCHOPT_AAA,
		IMF_MATCHOPT_BG,
		IMF_MATCHOPT_CHALK,
		IMF_MATCHOPT_ERASER,
		IMF_MATCHOPT_TITLE,
		IMF_MATCHOPT_VUVU,
	IMF_MATCHOPT_ZZZ,
	
	IMF_CARD_AAA,
		IMF_CARD_CONE,
	IMF_CARD_ZZZ,
	
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

