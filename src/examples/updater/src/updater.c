//updater.c
//Installs system update to boot ROM
//Bryan E. Topp <betopp@betopp.com> 2023

#define _GNU_SOURCE

#include <sc.h>
#include <ctype.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "gfx_font_8x16.xbm"

#define RGBEXPD(eightbit) ((((eightbit) & 0xE0) << 8) | (((eightbit) & 0x1C) << 6) | (((eightbit) & 0x03) << 3))

#include "sha256.h"

//Framebuffers
uint16_t fbs[2][240][320];
int fb_next;

//Text buffer
uint8_t textbuf[14][38];
uint8_t textcol;
uint8_t textrow;

//Image to write
uint8_t image_buf[8*1024*1024];

//Header we write
//Look for the newest kernel in the boot ROM. Each kernel starts with a known header...
typedef struct update_header_s
{
	char magic[8];
	uint64_t date;
	uint64_t size;
	uint8_t sha256[32];
} update_header_t;
static update_header_t update_header = 
{
	.magic = "PVMK_UPD",
	.date = __TIME_UNIX__,
};

static void font_draw_ch(int y, int x, char ch)
{
	for(int rr = 0; rr < 16; rr++)
	{
		if(y + rr < 0)
			continue;
		if(y + rr >= 240)
			break;
		
		for(int cc = 0; cc < 8; cc++)
		{
			if(x + cc < 0)
				continue;
			if(x + cc >= 320)
				break;
			
			if(gfx_font_8x16_bits[(((uint8_t)ch)*16)+rr] & (1<<cc))
			{
				fbs[fb_next][y+rr+8][x+cc+4] = 0x0;
			}
		}
	}	
}

static void clear(uint8_t color)
{
	for(int yy = 0; yy < 240; yy++)
	{
		for(int xx = 0; xx < 320; xx++)
		{
			fbs[fb_next][yy][xx] = RGBEXPD(color);
		}
	}
	
	//memset(fbs[fb_next], color, sizeof(fbs[fb_next]));
}

static void flip(void)
{
	while(_sc_gfx_flip(_SC_GFX_MODE_320X240_16BPP, fbs[fb_next]) != (int)(fbs[fb_next])) { _sc_pause(); }
	fb_next = fb_next ? 0 : 1;
}

static void drawtext(void)
{
	for(unsigned int rr = 0; rr < sizeof(textbuf) / sizeof(textbuf[0]); rr++)
	{
		for(unsigned int cc = 0; cc < sizeof(textbuf[0]) / sizeof(textbuf[0][0]); cc++)
		{
			font_draw_ch(rr * 16, cc * 8, textbuf[rr][cc]);
		}
	}
}

static void text_putc(int ch)
{
	if(isprint(ch))
	{
		textbuf[textrow][textcol] = ch;
		textcol++;
	}
	
	if((ch == '\n') || (textcol >= sizeof(textbuf[0]) / sizeof(textbuf[0][0])))
	{
		textcol = 0;
		textrow++;
		if(textrow >= sizeof(textbuf) / sizeof(textbuf[0]))
		{
			textrow = 0;
		}
	}
}

static void text_puts(const char *str)
{
	while(*str != '\0')
	{
		text_putc(*str);
		str++;
	}
}

static void text_printf(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	char buf[256] = {0};
	vsnprintf(buf, sizeof(buf) - 1, fmt, ap);
	va_end(ap);
	text_puts(buf);
}

static void fliptext(uint8_t bg)
{
	clear(bg);
	drawtext();
	flip();
}

void failure(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	char buf[256];
	buf[sizeof(buf) - 1] = 0;
	vsnprintf(buf, sizeof(buf) - 1, fmt, ap);
	va_end(ap);
	text_puts(buf);
	
	
	fliptext(0x80);
	sleep(1);
	exit(-1);
}

void prompt_em(void)
{
	while(1)
	{
		paint_clear();
		
		const char *updtitle = "Neki32 System Update";
		
		paint_text_ega(320 - (4 * strlen(updtitle)),  96, 11, 0, updtitle);
		paint_rect(320 - (4 * strlen(updtitle)) - 8, 96 - 4, 320 + 8 + (4 * strlen(updtitle)), 96 + 20, 0xEEEE);
		
		const char *upd_str = " A system update is available. ";
		const char *warn_str = "This game may not function correctly without it.";
		const char *start_str = "Press START or A to apply the update.";
		const char *nope_str = "Press MODE to continue without updating.";
		
		int yiter = 128;
		int first = 1;
		const char *strs[] = { upd_str, warn_str, "", start_str, nope_str, NULL };
		for(const char **ss = strs; *ss != NULL; ss++)
		{
			paint_text_ega(320 - (4 * strlen(*ss)), yiter, first ? 0 : 3, first ? 3 : 0, *ss);
			yiter += 32;
			first = 0;
		}
		
		paint_flip();
		
		_sc_input_t input = {0};
		static int lastbuttons = ~0ul;
		while(_sc_input(&input, sizeof(input), sizeof(input)) > 0)
		{
			if(input.format != 'A')
				continue;
			
			inputok = 1; //Valid input, wait for the user
			
			int newbuttons = input.buttons & ~lastbuttons;
			lastbuttons = input.buttons;
			
			for(int bb = 0; bb < 16; bb++)
			{
				if(!(newbuttons & (1u << bb)))
					continue;
				
				switch(bb)
				{
					case _SC_BTNIDX_START:
					case _SC_BTNIDX_A:
						return; //Go ahead
					
					case _SC_BTNIDX_MODE:
						exit(0); //Abort without updating
				}
			}
		}
		
		//If we've been sitting here for a long time with no controller data, just do it
		if(!inputok)
		{
			if(_sc_getticks() > 300000)
			{
				return; //Go ahead
			}
		}
	}			
}
	

int main(int argc, const char **argv)
{
	(void)argc;
	(void)argv;
	
	//There's an update on this card. Ask the player what to do.
	prompt_em();
	
	text_printf("Neki32 System Updater\n");
	text_printf("Version " BUILDVERSION "\n");
	text_printf("At " BUILDDATE "\n");
	text_printf("By " BUILDUSER "\n");
	
	fliptext(0x10);
	
	//Load update image...
	int update_fd = open("pvmk_upd/pvmk_arm.bin", O_RDONLY);
	if(update_fd < 0)
		failure("Cannot open pvmk_upd/pvmk_arm.bin");
	
	ssize_t nread = read(update_fd, image_buf, sizeof(image_buf));
	if(nread < 0)
		failure("Cannot read pvmk_upd/pvmk_arm.bin");
	
	if(nread < 131072)
		failure("Short read of pvmk_upd/pvmk_arm.bin");	
	
	//The update, implicitly, starts at 0x10000.
	//Find where the update actually ends.
	uint32_t sizeof_image = nread;
	update_header.size = sizeof_image;
	
	//Compute the SHA256 of what we'll be writing.
	//Todo - would be safer to store this when building...
	sha256_t sha = {0};
	sha256_init(&sha);
	sha256_update(&sha, image_buf + 65536, sizeof_image - 65536);
	sha256_final(&sha, update_header.sha256);

	//Open the boot ROM for writing
	int romfd = open("/dev/flash", O_RDWR);
	if(romfd < 0)
		failure("Cannot open /dev/flash");
	
	//Updates can be written starting at any 1MByte-aligned position.
	//For now just stick our update at the very end.
	
	//Check the size of the ROM
	uint32_t sizeof_flashrom = 16*1024*1024; //Assume 16MByte if we can't figure out the size
	struct stat st = {0};
	if(fstat(romfd, &st) == 0)
	{
		if(st.st_size > 0)
			sizeof_flashrom = st.st_size; //Know the actual ROM size
	}
	uint32_t free_start = sizeof_flashrom - sizeof_image;
	free_start -= free_start % 1048576;
	
	//The NVMs system uses 4 megabytes starting 1 meg in, and we have to start past that.
	if(free_start < (5*1024*1024))
		failure("No room for update image!");

	text_printf("Writing 0x%X bytes at 0x%X.\n", sizeof_image, free_start);
	fliptext(0x10);
	
	//Write the header at the beginning
	if(lseek(romfd, free_start, SEEK_SET) != (off_t)free_start)
		failure("Failed to seek in /dev/flash");
	
	if(write(romfd, &update_header, sizeof(update_header)) != (int)sizeof(update_header))
		failure("Failed to write header at 0x%X.\n", free_start);
	
	//Write the rest of it at 64KBytes after the beginning of the header.
	//Pull from 64KBytes after the beginning of this process image.
	if(lseek(romfd, free_start + 65536, SEEK_SET) != (off_t)(free_start + 65536))
		failure("Failed to seek in /dev/flash");
	
	for(uint32_t pp = 65536; pp < sizeof_image; pp += 4096)
	{
		if(write(romfd, image_buf + pp, 4096) != 4096)
			failure("Failed write 0x%X -> 0x%X.\n", (uint32_t)pp, (int)lseek(romfd, 0, SEEK_CUR));
		
		if(!(pp%32768))
		{
			text_printf(".");
			fliptext(0x10);
		}

	}
	
	//Clear any old update headers that might be found before the one we just wrote
	text_printf("\nClearing old update info...\n");
	fliptext(0x10);
	
	//At each megabyte, there might be an update info. If there is, it starts with PVMK_UPD.
	//Clear those first bytes so old updates aren't checked by the standby/bootloader.
	#define SIG_SIZE 8
	const char sig_to_clear[SIG_SIZE] = "PVMK_UPD";
	const uint8_t sig_to_write[SIG_SIZE] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
	for(uint32_t off = 0; off < free_start; off += 1024*1024)
	{
		char sig_existing[SIG_SIZE] = {0};
		lseek(romfd, off, SEEK_SET);
		read(romfd, sig_existing, SIG_SIZE);
		
		if(!memcmp(sig_existing, sig_to_clear, SIG_SIZE))
		{
			lseek(romfd, off, SEEK_SET);
			write(romfd, sig_to_write, SIG_SIZE);
		}
	}
	lseek(romfd, 0, SEEK_SET);
	
	//Perfek
	text_printf("\nAll good!\n");
	for(int ff = 0; ff < 5; ff++)
	{
		fliptext(0x18);
		usleep(500000);
	}
	_sc_halt();
	exit(0);
}