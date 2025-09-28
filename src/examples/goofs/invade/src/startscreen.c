//startscreen.c
//Press-Start Screen for Invaders game

#include "renderer.h"
#include "input.h"
#include "assets.h"

void startscreen(void)
{
	Sprite *bg = load_png_sprite("png/screens/startScreen.png");
	
	while(1)
	{
		renderer_clear(COLOR_BLACK);
		draw_sprite(bg, 0, 0);
		renderer_present();
		
		input_update();
		if(input_is_pressed(INPUT_START))
			break;
	}		
	
	
	free_sprite(bg);
	
}
