//menuvn.c
//VN-style menu-driven game example
//Bryan E. Topp <betopp@betopp.com> 2024

#include <sc.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "font.xbm"

//Screens we show
typedef enum screen_idx_e
{
	SI_FIRST = 0,
	
	SI_IMPL,
	SI_VNS,
	
	SI_MAX
} screen_idx_t;

//Option available on a screen
typedef struct screen_opt_s
{
	const char *label;
	void (*func)(int parm);
	int parm;
} screen_opt_t;

//Screen we're currently showing
int curscr;

//Line of text we're currently showing
int curline;

//Character we've drawn
int curchar;

//Tick when advancing the next character
int advtick;

//Selected option
int selection;

//Switches screen
void swscr(int parm)
{
	curscr = parm;
	if(curscr < 0 || curscr >= SI_MAX)
		curscr = 0;
	
	curline = 0;
	curchar = 0;
	selection = 0;
}

//Definition of each screen
typedef struct screen_s
{
	const char *heading; //Heading to show over menu/text
	const char *background; //Name of background image
	const char **exposition; //Lines of exposition, NULL-terminated
	screen_opt_t **options; //Options for the player, NULL-terminated
} screen_t;
static const screen_t screen_table[SI_MAX] = 
{
	[SI_FIRST] = 
	{
		.heading = "VN-style Example",
		.background = "first.scr",
		.exposition = (const char*[]) {
			"This is an example of a Visual Novel type program.",
			"Screens are shown with text and menu options for the player.",
			NULL
		},
		.options = (screen_opt_t*[]) {
			&(screen_opt_t){ .label = "Learn about the implementation",   .func = swscr, .parm = SI_IMPL },
			&(screen_opt_t){ .label = "What the heck is a visual novel?", .func = swscr, .parm = SI_VNS  },
			NULL
		},
	},
	
	[SI_IMPL] = 
	{
		.heading = "About this example",
		.background = "impl.scr",
		.exposition = (const char*[]) {
			"This example is implemented totally in C.",
			"It makes use of many designated initializers and constant tables.",
			"See? Objects ain't shit.",
			NULL
		},
		.options = (screen_opt_t*[]) {
			&(screen_opt_t){ .label = "Oh, cool.", .func = swscr, .parm = SI_FIRST },
			NULL
		},
	},
	
	[SI_VNS] = 
	{
		.heading = "About VNs",
		.background = "vns.scr",
		.exposition = (const char*[]) {
			"Visual Novels are video games, and they're art.",
			"Basically they involve lots of text and menu options.",
			"Usually there's also kawaii anime desu~~~.",
			"Don't let that stop you though.",
			NULL
		},
		.options = (screen_opt_t*[]) {
			&(screen_opt_t){ .label = "Gross. Get me out of here.", .func = swscr, .parm = SI_FIRST },
			NULL
		},
	},
};


//Information about each loaded background
typedef struct bgload_s
{
	char filename[32]; //File name being loaded here
	uint16_t data[480][640]; //Image data loaded
	int progress; //How many bytes have been loaded already
	int fd; //File descriptor for this background image
} bgload_t;
#define BGLOAD_MAX 16
bgload_t bgload_table[BGLOAD_MAX];

//Next background we evict to load a new one
int bgload_next;

//Starts loading the given background, if it's not already loaded
void bgload_enq(const char *name)
{
	//See if this background is already enqueued
	int idx = -1;
	for(int bb = 0; bb < BGLOAD_MAX; bb++)
	{
		if(!strcmp(name, bgload_table[bb].filename))
		{
			//Exists in the table already
			idx = bb;
			break;
		}
	}
	
	if(idx == -1)
	{
		//This background doesn't exist in our table.
		//Need to evict a loaded background to make room for it.
		idx = bgload_next;
		bgload_next = (bgload_next + 1) % BGLOAD_MAX;
		
		if(bgload_table[idx].fd > 0)
		{
			close(bgload_table[idx].fd);
			bgload_table[idx].fd = -1;
		}
		
		bgload_table[idx].progress = 0;
		strncpy(bgload_table[idx].filename, name, sizeof(bgload_table[idx].filename)-1);
	}
}

//Advances loading of a background image, if any aren't fully loaded
void bgload_tick(void)
{
	//Look for a background image which is enqueued but not fully loaded.
	int idx = bgload_next;
	for(int attempt = 0; attempt < BGLOAD_MAX; attempt++)
	{
		idx++;
		if(idx >= BGLOAD_MAX)
			idx -= BGLOAD_MAX;
		
		if(bgload_table[idx].progress >= (int)sizeof(bgload_table[idx].data))
		{
			//Fully loaded
			continue;
		}
		
		if(bgload_table[idx].filename[0] == '\0')
		{
			//Nothing enqueued
			continue;
		}
		
		if(bgload_table[idx].fd <= 0)
		{
			//File isn't open... try to open it
			bgload_table[idx].fd = open(bgload_table[idx].filename, O_RDONLY);
			if(bgload_table[idx].fd < 0)
			{
				//Failed to open. Remove from queue.
				bgload_table[idx].filename[0] = '\0';
				return;
			}
			else
			{
				//Successfully opened. Load more next time.
				return;
			}
		}
		
		//Keep loading
		char *dstptr = ((char*)(bgload_table[idx].data)) + bgload_table[idx].progress;
		int nloaded = read(bgload_table[idx].fd, dstptr, 65536);
		if(nloaded < 0)
		{
			//Error loading
			close(bgload_table[idx].fd);
			bgload_table[idx].fd = -1;
			
			bgload_table[idx].filename[0] = '\0';
			
			return;
		}
		else if(nloaded == 0)
		{
			//File finished...?
			bgload_table[idx].progress = sizeof(bgload_table[idx].data);
			return;
		}
		else
		{
			//Loaded some more
			bgload_table[idx].progress += nloaded;
			return;
		}
	}
}

//Framebuffers
uint16_t fbs[3][480][640];

//Which framebuffer is to be drawn to
int fbnext;

//Presents the just-drawn framebuffer and selects a new one to draw to, with triple-buffering.
void fbflip(void)
{
	//Enqueue the flip to our new buffer and find out what we're currently displaying
	int displayed = _sc_gfx_flip(_SC_GFX_MODE_VGA_16BPP, &(fbs[fbnext][0][0]));
	if(displayed < 0)
	{
		//Error enqueuing flip
		return;
	}
	
	//One of our buffers is displayed and, potentially, a different one is now enqueued.
	//Find the third buffer, which is now free for us to render to.
	uint16_t *displayed_ptr = (uint16_t*)(uintptr_t)(displayed);
	uint16_t *enqueued_ptr = (uint16_t*)(&(fbs[fbnext][0][0]));
	for(int ff = 0; ff < 3; ff++)
	{
		//Check the next buffer...
		uint16_t *candidate_ptr = (uint16_t*)(uintptr_t)(&(fbs[ff][0][0]));
		
		if(candidate_ptr == displayed_ptr)
		{
			//This framebuffer is currently displayed, as returned by _sc_gfx_flip
			continue;
		}
		
		if(candidate_ptr == enqueued_ptr)
		{
			//This framebuffer is the one we just requested to display, as we called _sc_gfx_flip
			continue;
		}
		
		//This framebuffer is free - it's neither currently-displayed nor about to be.
		fbnext = ff;
		return;
	}
}

//Draws font
void drawchr(int x, int y, char ch)
{
	for(int rr = 0; rr < 16; rr++)
	{
		uint8_t inpx = font_bits[ch + (rr * 128)];
		for(int cc = 0; cc < 8; cc++)
		{
			if(!(inpx & (1u << cc)))
			{
				fbs[fbnext][y + rr][x + cc] = 0xFFFF;
				fbs[fbnext][y + rr+1][x + cc+1] = 0x00;
			}				
		}	
	}
}

int main(void)
{
	
	while(1)
	{
		//Load any known backgrounds which aren't fully loaded
		bgload_enq(screen_table[curscr].background);
		bgload_tick();
		
		//If we're on a screen that references other screens, preload those too
		for(int oo = 0; screen_table[curscr].options[oo] != NULL; oo++)
		{
			if(screen_table[curscr].options[oo]->func == swscr)
			{
				bgload_enq(screen_table[screen_table[curscr].options[oo]->parm].background);
			}
		}
		
		//Draw background from loaded background cache
		const char *bgname = screen_table[curscr].background;
		for(int bb = 0; bb < BGLOAD_MAX; bb++)
		{
			if(strcmp(bgload_table[bb].filename, bgname) == 0)
			{
				memcpy(fbs[fbnext], bgload_table[bb].data, sizeof(fbs[fbnext]));
				break;
			}
		}
		
		//Compute where the text starts
		int txy = 400;
		txy -= curline * 16;
		
		int txx = 16;
		
		//Darken image
		for(int yy = txy - 4; yy < 480; yy++)
		{
			for(int xx = 0; xx < 640; xx++)
			{
				fbs[fbnext][yy][xx] &= ~(1u << 0);
				fbs[fbnext][yy][xx] &= ~(1u << 5);
				fbs[fbnext][yy][xx] &= ~(1u << 11);
				fbs[fbnext][yy][xx] >>= 1;
			}
		}
		
		//Draw complete lines
		for(int ll = 0; ll < curline; ll++)
		{
			for(int cc = 0; cc < (int)strlen(screen_table[curscr].exposition[ll]); cc++)
			{
				drawchr(txx + cc * 8, txy + ll * 16, screen_table[curscr].exposition[ll][cc]);
			}
		}
		
		//Draw partial line
		if(screen_table[curscr].exposition[curline] != NULL)
		{
			for(int cc = 0; cc < curchar; cc++)
			{
				drawchr(txx + cc * 8, txy + curline * 16, screen_table[curscr].exposition[curline][cc]);
			}
		}
		
		//Advance displayed text if it's time
		if(advtick < _sc_getticks())
		{
			if(screen_table[curscr].exposition[curline] != NULL)
			{
				curchar++;
				advtick += 100;
				if(curchar >= (int)strlen(screen_table[curscr].exposition[curline]))
				{
					curchar = 0;
					curline++;
					advtick += 500;
				}
			}
			
		}
		
		//Show options
		if(screen_table[curscr].exposition[curline] == NULL)
		{
			for(int pp = 0; screen_table[curscr].options[pp] != NULL; pp++)
			{
				for(int cc = 0; cc < (int)strlen(screen_table[curscr].options[pp]->label); cc++)
				{
					drawchr(
						32 + txx + cc * 8, 
						txy + (curline + 1 + pp) * 16, 
						screen_table[curscr].options[pp]->label[cc]
					);
				}
			}
			
			drawchr(
				16 + txx,
				txy + (curline + 1 + selection) * 16, 
				'='
			);
			drawchr(
				20 + txx,
				txy + (curline + 1 + selection) * 16, 
				'>'
			);
		}
		
		//Present the frame
		fbflip();
		
		//Read inputs
		_sc_input_t input = {0};
		static int lastbuttons = ~0u;
		while(_sc_input(&input, sizeof(input), sizeof(input)) > 0)
		{
			if(input.format != 'A')
				continue;
			
			if(input.buttons & _SC_BTNBIT_C)
			{
				//Fast-forward
				advtick = 0;
			}
			
			int pressed = input.buttons & ~lastbuttons;
			
			if(pressed & _SC_BTNBIT_A)
			{
				//If we're not seeing options yet, advance by a line
				if(screen_table[curscr].exposition[curline] != NULL)
				{
					curline++;
					curchar = 0;
				}
				else
				{
					//Otherwise, select the given option
					if(screen_table[curscr].options[selection]->func)
					{
						(*(screen_table[curscr].options[selection]->func))
							(screen_table[curscr].options[selection]->parm);
					}
				}
			}
			
			if(pressed & _SC_BTNBIT_DOWN)
			{
				//Next option
				selection++;
				if(screen_table[curscr].options[selection] == NULL)
					selection--;
			}
			
			if(pressed & _SC_BTNBIT_UP)
			{
				//Previous option
				selection--;
				if(selection < 0)
					selection++;
			}
			
			
			lastbuttons = input.buttons;
		}
		
	}
	return 0;
}
