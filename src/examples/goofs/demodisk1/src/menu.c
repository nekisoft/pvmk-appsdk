//menu.c
//Menu for Demo Disk 1
//Bryan E. Topp <betopp@betopp.com> 2025

#include "font.h"
#include "fbs.h"
#include "pads.h"
#include "images.h"
#include "trigfunc.h"
#include <unistd.h>
#include <fcntl.h>
#include <stddef.h>

void gamelist(void)
{
	const char *titles[][2] = 
	{
		{ "FreeDoom Phase 1",    "sub/ports/freedoom1" },
		{ "FreeDoom Phase 2",    "sub/ports/freedoom2" },
		{ "LibreQuake",          "sub/ports/quake"     },
		{ "Beats of Rage",       "sub/ports/bor"       },
		{ "CyberDogs",           "sub/ports/cdogs"     },
		{ "Cylindrix",           "sub/ports/cylindrix" },
		{ "In Pursuit of GREED", "sub/ports/greed"     },
		{ "Chasin' Gators",      "sub/goofs/invade"    },
		{0}
	};
	
	
	int sel = 0;
	while(1)
	{
		int ticks = _sc_getticks();
		int tickphase = ticks % 65536;
		
		images_draw(IMF_BG3, 0, 0);
		
		const char *heading = "Game Demos";
		font_draw(FS_SCIFI, heading, 0x0000, 50+64, 64);
		font_draw(FS_SCIFI, heading, 0xFFFF, 50+62, 62);
		
		const char *bottomtext[] = 
		{
			"", //"At any time:",
			" - Hold START+MODE+DOWN to POWER OFF.",
			" - Hold UP alone to POWER ON again.",
			NULL,
		};
		
		for(int mm = 0; bottomtext[mm] != NULL; mm++)
		{		
			font_draw(FS_SCIFI, bottomtext[mm], 0x0000, 64, 364+(32*mm));
			font_draw(FS_SCIFI, bottomtext[mm], 0x2222, 62, 362+(32*mm));
		}
		
		
		for(int oo = 0; titles[oo][0] != NULL; oo++)
		{
			int xbump = (oo == sel) ? trigfunc_cos8(tickphase*64)/32 : 0;
			int color = (oo == sel) ? 0xEEEE : 0x8888;
			font_draw(FS_SCIFI, titles[oo][0], 0, 50+128 + xbump+2, 128+(32*oo)+2);
			font_draw(FS_SCIFI, titles[oo][0], color, 50+128 + xbump, 128+(32*oo));
		}
		
		fbs_flip();
		
		
		
		if(pads_detect(PAD_ANY, BTNBIT_B))
			return;
		
		if(pads_detect(PAD_ANY, BTNBIT_UP))
			sel--;
		if(pads_detect(PAD_ANY, BTNBIT_DOWN))
			sel++;
		
		if(sel < 0)
			sel = 0;
		if(titles[sel][0] == NULL)
			sel--;
		
		if(pads_detect(PAD_ANY, BTNBIT_A | BTNBIT_START))
		{
			if(titles[sel][1] != NULL)
			{
				chdir(titles[sel][1]);
				execv("boot.nne", (char *const[]){"boot.nne", NULL});
			}
		}
		
	}
	
	
	
}

void message(void)
{
	while(1)
	{
		
		images_draw(IMF_BG2, 0, 0);
		
		
		static const char *lines[] = 
		{
			"Thank you for your interest in Neki32!",
			"",
			"This system is the result of many long",
			"experiments in game development. I've",
			"found what worked, kept the best, and",
			"thrown out everything else.",
			"",
			"With any luck, I'm not alone - and other",
			"programmers will find and appreciate",
			"this development style too.",
			"",
			"I hope you enjoy using your Neki32.",
			"           - Bryan E. Topp, 2025",
			NULL
		};
		
		for(int ll = 0; lines[ll] != NULL; ll++)
		{
			font_draw(FS_SCIFI, lines[ll], 0x0000, 64, (ll+1)*32);
			font_draw(FS_SCIFI, lines[ll], 0xFFFF, 64-2, (ll+1)*32-2);
		}
		
		fbs_flip();
		
		
		if(pads_detect(PAD_ANY, BTNBIT_B))
			break;
	}
}

void intro(void)
{

	//Intro menu
	while(1)
	{
		int ticks = _sc_getticks();
		int tickphase=ticks % 65536;
		
		images_draw(IMF_BG1, 0, 0);
		images_draw(IMF_LOGO, (256 + trigfunc_sin8(tickphase*8))/4, (256+trigfunc_cos8(tickphase*32))/4);
		
		int xscroll = -(ticks/8) % 640;
		int fontcol = ((1u << 0) | (1u << 6) | (1u << 11)) * ((256+trigfunc_cos8(tickphase*64))/16);
		int fontcol2 = ((1u << 0) | (1u << 6) | (1u << 11)) * ((256+trigfunc_cos8((tickphase+32768/64)*64))/16);
		const char *textline = "Neki32 Demo Disk    Volume 1    Press Start";
		font_draw(FS_SCIFI, textline, fontcol, xscroll+32, 280);
		font_draw(FS_SCIFI, textline, fontcol, xscroll+32+640, 280);
		font_draw(FS_SCIFI, textline, fontcol2, xscroll+32, 280+32);
		font_draw(FS_SCIFI, textline, fontcol2, xscroll+32+640, 280+32);
		
		const char *corp = "Nekisoft Pty Ltd 2025";
		font_draw(FS_SCIFI, corp, 0x3333, 32, 440);
		font_draw(FS_SCIFI, corp, 0xbeef, 32-2, 440-2);
		
		fbs_flip();
		
		if(pads_detect(PAD_ANY, BTNBIT_START) || pads_detect(PAD_ANY, BTNBIT_A))
			break;
	}
}

void mmenu(void)
{
	
	
	//Main menu
	int sel = 0;
	const char *options[] = 
	{
		"Game Demos",
		//"About Neki32",
		"A Message from Bryan",
		NULL
	};
	void (*optptrs[])(void) = 
	{
		gamelist,
		//NULL,
		message,
		NULL,
	};
	
	while(1)
	{
		int ticks = _sc_getticks();
		int tickphase = ticks % 65536;
		
		
		images_draw(IMF_BG1, 0, 0);
		
		font_draw(FS_SCIFI, "Neki32 Demo Disk Volume 1", 0x0000, 64, 64);
		font_draw(FS_SCIFI, "Neki32 Demo Disk Volume 1", 0xFFFF, 62, 62);
		
		for(int oo = 0; options[oo] != NULL; oo++)
		{
			int xbump = (oo == sel) ? trigfunc_cos8(tickphase*64)/32 : 0;
			int color = (oo == sel) ? 0xEEEE : 0x8888;
			font_draw(FS_SCIFI, options[oo], 0, 128 + xbump+2, 128+(32*oo)+2);
			font_draw(FS_SCIFI, options[oo], color, 128 + xbump, 128+(32*oo));
		}
		
		fbs_flip();
		
		if(pads_detect(PAD_ANY, BTNBIT_B))
			return;
		
		if(pads_detect(PAD_ANY, BTNBIT_UP))
			sel--;
		if(pads_detect(PAD_ANY, BTNBIT_DOWN))
			sel++;
		
		if(sel < 0)
			sel = 0;
		if(options[sel] == NULL)
			sel--;
		
		if(pads_detect(PAD_ANY, BTNBIT_A) || pads_detect(PAD_ANY, BTNBIT_START))
		{
			if(optptrs[sel] != NULL)
			{
				(*(optptrs[sel]))();
			}
		}
		
	}
}

int main(void)
{
	while(1)
	{
		intro();
		mmenu();
	}
}
