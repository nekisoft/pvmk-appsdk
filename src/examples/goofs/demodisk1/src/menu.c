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

void gamelist(const char *heading, const char *titles[][2])
{
	int sel = 0;
	while(1)
	{
		int ticks = _sc_getticks();
		int tickphase = ticks % 65536;
		
		images_draw(IMF_BG1, 0, 0);
		
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
			font_draw(FS_SCIFI, bottomtext[mm], 0x2234, 62, 362+(32*mm));
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
		
		images_draw(IMF_BG1, 0, 0);
		
		
		static const char *lines[] = 
		{
			"Thank you for your interest in Neki32!",
			"",
			"The Neki32 Video Game System is designed",
			"to be fast, easy, and reliable, for both",
			"gamers and game developers. This card",
			"includes some examples from the Neki32",
			"software development kit, as well as some",
			"ports of old and open-source games.",
			"",
			"More software will be available soon from",
			"Nekisoft Pty Ltd and other developers too!",
			"I hope you enjoy using your Neki32.",
			"                    - Bryan E. Topp, 2025",
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
		images_draw(IMF_LOGO, 120 + trigfunc_sin8(tickphase*8)/8, 100);
		
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

void gamelist_ports(void)
{
	static const char *titles_ports[][2] = 
	{
		{ "FreeDoom Phase 1",    "sub/ports/freedoom1" },
		{ "FreeDoom Phase 2",    "sub/ports/freedoom2" },
		{ "LibreQuake",          "sub/ports/quake"     },
		{ "Beats of Rage",       "sub/ports/bor"       },
		{ "CyberDogs",           "sub/ports/cdogs"     },
		{ "Cylindrix",           "sub/ports/cylindrix" },
		{ "In Pursuit of GREED", "sub/ports/greed"     },
		{0}
	};

	gamelist("Game Ports", titles_ports);
}

void gamelist_goofs(void)
{
	const char *titles_goofs[][2] = 
	{
		{ "Chasin' Gators",      "sub/goofs/invade"    },
		{ "Sausage Tosser",      "sub/goofs/sausage"   },
		{0}
	};	
	
	gamelist("Mini Games", titles_goofs);
}

void gamelist_bits(void)
{
	const char *titles_bits[][2] = 
	{
		{ "Gamepad Test", "sub/sysusage/showinput" },
		{ "3D Test",      "sub/goofs/neki3d" },
		{0}
	};
	
	gamelist("Bits 'n' Bobs", titles_bits);
	
}
void mmenu(void)
{
	
	
	//Main menu
	int sel = 0;
	const char *options[] = 
	{
		"Game Ports",
		"Mini Games",
		"Bits 'n' Bobs",
		"About Neki32",
		NULL
	};
	void (*optptrs[])(void) = 
	{
		gamelist_ports,
		gamelist_goofs,
		gamelist_bits,
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
