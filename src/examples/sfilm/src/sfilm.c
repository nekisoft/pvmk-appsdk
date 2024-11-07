//sfilm.c
//FMV player compatible with Saturn-style FILM files
//Bryan E. Topp <betopp@betopp.com> 2024

//This example plays an FMV in Sega Saturn FILM format and then launches another executable.
//You can make files like this:
//ffmpeg -i somevideo.mp4 -s 320x240 -ac 1 -ar 24000  -threads 4 output.cpk

//Disk streaming module, to keep IO requests going while we process video
#include "streamer.h"

//System-call definitions from SDK
#include <sc.h>

//Standard library headers from SDK
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

//Lots of optimizations are possible if we know the width of a video line at compile-time
#define SFILM_FORCE_VIDEO_MODE _SC_GFX_MODE_VGA_16BPP

//Video mode we'll use for playing
#if SFILM_FORCE_VIDEO_MODE
	#define video_mode SFILM_FORCE_VIDEO_MODE
#else
	static int video_mode = 0;
#endif

//Framebuffer space (might be 320x240 or 640x480, triple-buffer either way)
//Accessed as uint64_t - 4 pixels at a time
static uint64_t framebuffer_0[640*480/4] __attribute__((aligned(1048576)));
//static uint64_t framebuffer_1[640*480/4] __attribute__((aligned(1048576)));
//static uint64_t framebuffer_2[640*480/4] __attribute__((aligned(1048576)));

//Number of 64-bit words in each video line
#if SFILM_FORCE_VIDEO_MODE == _SC_GFX_MODE_VGA_16BPP
	#define video_pitch 160
#elif SFILM_FORCE_VIDEO_MODE == _SC_GFX_MODE_320X240_16BPP
	#define video_pitch 80
#else
	static int video_pitch = 0;
#endif

//Which framebuffer we're decoding into
static uint64_t *framebuffer_next = framebuffer_0;

//Sample table from FILM header
typedef struct stab_s
{
	uint32_t offset;
	uint32_t length;
	uint32_t info1;
	uint32_t info2;
} stab_t;
#define STAB_MAX 65536 //About 45 minutes at 24hz
static stab_t stab[STAB_MAX];

//Number of entries in sample table, when we exhaust this we're done
static int stab_total;

//Next sample table entry to load
static int stab_next;

//Framerate base value for sample table entries
static int stab_timebase;

//Temporary buffer for audio data
#define AUDIOBUFSZ 4096
static uint16_t audio_buffer[4096][2];

//Cinepak V1 codebook with pixels already doubled
static uint64_t cvid_v1[256][2];

//Cinepack V4 codebook - each byte of a V4 code unpacks into 2x2 pixels, each of these used once
static uint32_t cvid_v4[256][2];

//Dimensions of Cinepak strip we're decoding
static uint16_t strip_x0;
static uint16_t strip_x1;
static uint16_t strip_y0;
static uint16_t strip_y1;

//Strips apparently stack within a frame, even if they all have the same Y range
static uint16_t strip_y_done;

//Decodes a Cinepak y/u/v value into rgb565 that we use for our framebuffer
static uint16_t decode_yuv(unsigned char y, signed char u, signed char v)
{
	int r = (int)y + (v  << 1);
	if(r < 0)
		r = 0;
	if(r > 255)
		r = 255;
	
	int g = (int)y - (u >> 1) - v;
	if(g < 0)
		g = 0;
	if(g > 255)
		g = 255;
	
	int b = (int)y + (u << 1);
	if(b < 0)
		b = 0;
	if(b > 255)
		b = 255;
	
	uint16_t result = 0;
	result |= ((b >> 3) & 0x1F) <<  0;
	result |= ((g >> 2) & 0x3F) <<  5;
	result |= ((r >> 3) & 0x1F) << 11;
	
	return result;
}

//Copies and flips byte order of a series of 4-byte values
static void cpy4be(void *dst, void *src, int bytes)
{
	unsigned char *db = (unsigned char*)dst;
	unsigned char *sb = (unsigned char*)src;
	for(int copied = 0; copied < bytes; copied += 4)
	{
		for(int bb = 0; bb < 4; bb++)
		{
			db[bb] = sb[3-bb];
		}
		db += 4;
		sb += 4;
	}
}

//Copies and flips byte order of a series of 2-byte values
static void cpy2be(void *dst, void *src, int bytes)
{
	unsigned char *db = (unsigned char*)dst;
	unsigned char *sb = (unsigned char*)src;
	for(int copied = 0; copied < bytes; copied += 2)
	{
		for(int bb = 0; bb < 2; bb++)
		{
			db[bb] = sb[1-bb];
		}
		db += 2;
		sb += 2;
	}
}

//Handles an audio chunk
static void sample_audio(const unsigned char *audio_chunk, int payload_len)
{
	//We'll synchronize to the audio playback.
	//Feed the audio data into the kernel's buffer and wait if there's no room.
	
	//Decode the sample into the audio buffer	
	if(payload_len % 2)
	{
		fprintf(stderr, "sfilm: 16-bit audio data has an odd number of bytes, that's bad.\n");
		return;
	}
	
	int nsamples = payload_len / 2;
	if(nsamples > AUDIOBUFSZ/2)
	{
		fprintf(stderr, "sfilm: Too many audio samples in audio chunk.\n");
		return;
	}
	
	for(int ss = 0; ss < nsamples; ss++)
	{
		uint16_t sample = 0;
		
		sample = (sample << 8) | *audio_chunk;
		audio_chunk++;
		
		sample = (sample << 8) | *audio_chunk;
		audio_chunk++;
		
		//Decompress 24KHz mono to 48KHz stereo
		audio_buffer[(ss*2)+0][0] = sample;
		audio_buffer[(ss*2)+0][1] = sample;
		audio_buffer[(ss*2)+1][0] = sample;
		audio_buffer[(ss*2)+1][1] = sample;
	}
	
	//Shove it at the kernel
	while(1)
	{
		int maxbuf = (48000*4)/10;
		if(video_mode == _SC_GFX_MODE_VGA_16BPP)
			maxbuf *= 4;
		
		int enqueued = _sc_snd_play(1, audio_buffer, nsamples*8, maxbuf);
		if(enqueued < 0)
		{
			//No room for this audio yet, pump streaming and try again
			streamer_pump();
		}
		else
		{
			//Enqueued this audio sample for playback
			break;
		}
	}	
}

//Draws a quad of V4 codewords into the framebuffer from the codebook
static void cvid_draw_v4(uint64_t *dest, unsigned char *v4)
{	
	dest[video_pitch*0] = cvid_v4[v4[0]][0] | ((uint64_t)cvid_v4[v4[1]][0] << 32);
	dest[video_pitch*1] = cvid_v4[v4[0]][1] | ((uint64_t)cvid_v4[v4[1]][1] << 32);
	dest[video_pitch*2] = cvid_v4[v4[2]][0] | ((uint64_t)cvid_v4[v4[3]][0] << 32);
	dest[video_pitch*3] = cvid_v4[v4[2]][1] | ((uint64_t)cvid_v4[v4[3]][1] << 32);
}

//Draws a single (pixel-doubled) V1 codeword into the framebuffer from the codebook
static void cvid_draw_v1(uint64_t *dest, unsigned char *v1)
{	
	dest[video_pitch*0] = cvid_v1[v1[0]][0];
	dest[video_pitch*1] = cvid_v1[v1[0]][0];
	dest[video_pitch*2] = cvid_v1[v1[0]][1];
	dest[video_pitch*3] = cvid_v1[v1[0]][1];
}

//Writes a V4 word into the codebook from 6 bytes (Y0123/U/V)
static void cvid_set_v4_color(int code, unsigned char *data)
{
	uint32_t a = decode_yuv(data[0],data[4],data[5]);
	uint32_t b = decode_yuv(data[1],data[4],data[5]);
	uint32_t c = decode_yuv(data[2],data[4],data[5]);
	uint32_t d = decode_yuv(data[3],data[4],data[5]);	
	cvid_v4[code][0] = (a << 0) | (b << 16);
	cvid_v4[code][1] = (c << 0) | (d << 16);
}

//Writes a V1 word into the codebook from 6 bytes (Y0123/U/V)
static void cvid_set_v1_color(int code, unsigned char *data)
{
	uint64_t a = decode_yuv(data[0],data[4],data[5]);
	uint64_t b = decode_yuv(data[1],data[4],data[5]);
	uint64_t c = decode_yuv(data[2],data[4],data[5]);
	uint64_t d = decode_yuv(data[3],data[4],data[5]);
	cvid_v1[code][0] = (a << 0) | (a << 16) | (b << 32) | (b << 48);
	cvid_v1[code][1] = (c << 0) | (c << 16) | (d << 32) | (d << 48);
}

//Writes a V4 word into the codebook from 4 bytes (Y0123)
static void cvid_set_v4_grey(int code, unsigned char *data)
{
	uint32_t a = decode_yuv(data[0],0,0);
	uint32_t b = decode_yuv(data[1],0,0);
	uint32_t c = decode_yuv(data[2],0,0);
	uint32_t d = decode_yuv(data[3],0,0);
	cvid_v4[code][0] = (a << 0) | (b << 16);
	cvid_v4[code][1] = (c << 0) | (d << 16);
}

//Writes a V1 word into the codebook from 4 bytes (Y0123)
static void cvid_set_v1_grey(int code, unsigned char *data)
{
	uint64_t a = decode_yuv(data[0],0,0);
	uint64_t b = decode_yuv(data[1],0,0);
	uint64_t c = decode_yuv(data[2],0,0);
	uint64_t d = decode_yuv(data[3],0,0);
	cvid_v1[code][0] = (a << 0) | (a << 16) | (b << 32) | (b << 48);
	cvid_v1[code][1] = (c << 0) | (c << 16) | (d << 32) | (d << 48);	
}


//Handles a cvid data chunk
static void cvid_datum(unsigned char *data_ptr, int data_len)
{
	uint16_t data_id = 0;
	data_id = (data_id << 8) | data_ptr[0];
	data_id = (data_id << 8) | data_ptr[1];
	
	//uint16_t data_bytes = 0;
	//data_bytes = (data_bytes << 8) | data_ptr[2];
	//data_bytes = (data_bytes << 8) | data_ptr[3];
	
	data_ptr += 4;
	data_len -= 4;

	if(data_id == 0x2000) //List of blocks in 12-bit V4 codebook
	{
		int code = 0;
		while(data_len > 5 && code < 256)
		{	
			cvid_set_v4_color(code, data_ptr);
			code++;
			data_ptr += 6;
			data_len -= 6;
		}
		return;
	}
	
	if(data_id == 0x2200) //List of blocks in 12-bit V1 codebook
	{
		int code = 0;
		while(data_len > 5 && code < 256)
		{	
			cvid_set_v1_color(code, data_ptr);
			code++;
			data_ptr += 6;
			data_len -= 6;
		}
		return;
	}
			
	if(data_id == 0x2400) //List of blocks in 8-bit V4 codebook
	{
		int code = 0;
		while(data_len > 3 && code < 256)
		{	
			cvid_set_v4_grey(code, data_ptr);
			code++;
			data_ptr += 4;
			data_len -= 4;
		}
		return;
	}
		
	if(data_id == 0x2600) //List of blocks in 8-bit V1 codebook
	{
		int code = 0;
		while(data_len > 3 && code < 256)
		{	
			cvid_set_v1_grey(code, data_ptr);
			code++;
			data_ptr += 4;
			data_len -= 4;
		}
		return;
	}
		
	if(data_id == 0x3000) //Vectors used to encode a frame
	{
		int outx = strip_x0;
		int outy = strip_y0 + strip_y_done;
		while(data_len > 0)
		{
			//Read 32-bit vector telling us which blocks are V1, and which are V4
			uint32_t flags = 0;
			flags = (flags << 8) | data_ptr[0];
			flags = (flags << 8) | data_ptr[1];
			flags = (flags << 8) | data_ptr[2];
			flags = (flags << 8) | data_ptr[3];
			data_ptr += 4;
			data_len -= 4;
			
			//Work through the 32 blocks then
			for(int ff = 0; ff < 32; ff++)
			{
				uint64_t *dest = framebuffer_next + (outy * video_pitch) + (outx/4);
				if(flags & 0x80000000u)
				{
					//Block coded as V4
					cvid_draw_v4(dest, data_ptr);
					data_ptr += 4;
					data_len -= 4;
				}
				else
				{
					//Block coded as V1
					cvid_draw_v1(dest, data_ptr);
					data_ptr += 1;
					data_len -= 1;
				}
				flags <<= 1;
				
				outx += 4;
				if(outx >= strip_x1)
				{
					outx = strip_x0;
					outy += 4;
				}
				
				if(data_len <= 0)
					break;
			}
		}
		
		return;
	}
		
	if(data_id == 0x3200) //List of blocks from only the V1 codebook
	{
		int outx = strip_x0;
		int outy = strip_y0 + strip_y_done;
		while(data_len > 0)
		{
			uint64_t *dest = framebuffer_next + (outy * video_pitch) + (outx/4);
	
			//Block coded as V1
			cvid_draw_v1(dest, data_ptr);
			data_ptr += 1;
			data_len -= 1;
	
			outx += 4;
			if(outx >= strip_x1)
			{
				outx = strip_x0;
				outy += 4;
			}
		}
		
		return;
	}
	
	if(data_id == 0x2100) //Partial list of blocks to update 12-bit V4 codebook
	{
		int code = 0;
		while(data_len > 0)
		{
			//Read 32-bit vector telling us which codes are updated
			uint32_t flags = 0;
			flags = (flags << 8) | data_ptr[0];
			flags = (flags << 8) | data_ptr[1];
			flags = (flags << 8) | data_ptr[2];
			flags = (flags << 8) | data_ptr[3];
			data_ptr += 4;
			data_len -= 4;
			
			//Work through updated codes
			for(int ff = 0; ff < 32; ff++)
			{
				if(flags & 0x80000000u)
				{
					//This code is updated
					cvid_set_v4_color(code, data_ptr);
					data_ptr += 6;
					data_len -= 6;
				}
				flags <<= 1;
				code++;
			}
		}	
		
		return;
	}
	
	if(data_id == 0x2300) //Partial list of blocks to update 12-bit V1 codebook
	{
		int code = 0;
		while(data_len > 0)
		{
			//Read 32-bit vector telling us which codes are updated
			uint32_t flags = 0;
			flags = (flags << 8) | data_ptr[0];
			flags = (flags << 8) | data_ptr[1];
			flags = (flags << 8) | data_ptr[2];
			flags = (flags << 8) | data_ptr[3];
			data_ptr += 4;
			data_len -= 4;
			
			//Work through updated codes
			for(int ff = 0; ff < 32; ff++)
			{
				if(flags & 0x80000000u)
				{
					//This code is updated
					cvid_set_v1_color(code, data_ptr);
					data_ptr += 6;
					data_len -= 6;
				}
				flags <<= 1;
				code++;
			}
		}	
		
		return;
	}
	
	if(data_id == 0x2500) //Partial list of blocks to update 8-bit V4 codebook
	{
		int code = 0;
		while(data_len > 0)
		{
			//Read 32-bit vector telling us which codes are updated
			uint32_t flags = 0;
			flags = (flags << 8) | data_ptr[0];
			flags = (flags << 8) | data_ptr[1];
			flags = (flags << 8) | data_ptr[2];
			flags = (flags << 8) | data_ptr[3];
			data_ptr += 4;
			data_len -= 4;
			
			//Work through updated codes
			for(int ff = 0; ff < 32; ff++)
			{
				if(flags & 0x80000000u)
				{
					//This code is updated
					cvid_set_v4_grey(code, data_ptr);
					data_ptr += 4;
					data_len -= 4;
				}
				flags <<= 1;
				code++;
			}
		}	
		
		return;
	}
	
	if(data_id == 0x2700) //Partial list of blocks to update 8-bit V1 codebook
	{
		int code = 0;
		while(data_len > 0)
		{
			//Read 32-bit vector telling us which codes are updated
			uint32_t flags = 0;
			flags = (flags << 8) | data_ptr[0];
			flags = (flags << 8) | data_ptr[1];
			flags = (flags << 8) | data_ptr[2];
			flags = (flags << 8) | data_ptr[3];
			data_ptr += 4;
			data_len -= 4;
			
			//Work through updated codes
			for(int ff = 0; ff < 32; ff++)
			{
				if(flags & 0x80000000u)
				{
					//This code is updated
					cvid_set_v1_grey(code, data_ptr);
					data_ptr += 4;
					data_len -= 4;
				}
				flags <<= 1;
				code++;
			}
		}	
		
		return;
	}
	
	if(data_id == 0x3100) //Partial list of vectors to update frame
	{
		int outx = strip_x0;
		int outy = strip_y0 + strip_y_done;
		uint32_t flags_values = 0;
		int flags_remain = 0;
		while(data_len > 0)
		{
			//Determine if the block is coded or not
			if(flags_remain <= 0)
			{
				//Need more flags
				flags_values = (flags_values << 8) | data_ptr[0];
				flags_values = (flags_values << 8) | data_ptr[1];
				flags_values = (flags_values << 8) | data_ptr[2];
				flags_values = (flags_values << 8) | data_ptr[3];
				flags_remain = 32;
				data_ptr += 4;
				data_len -= 4;
			}
			unsigned int is_coded = (flags_values & 0x80000000u);
			flags_values <<= 1;
			flags_remain--;
			
			if(is_coded)
			{
				uint64_t *dest = framebuffer_next + (outy * video_pitch) + (outx/4);
		
				if(flags_remain <= 0)
				{
					//Need more flags
					flags_values = (flags_values << 8) | data_ptr[0];
					flags_values = (flags_values << 8) | data_ptr[1];
					flags_values = (flags_values << 8) | data_ptr[2];
					flags_values = (flags_values << 8) | data_ptr[3];
					flags_remain = 32;
					data_ptr += 4;
					data_len -= 4;
				}
				unsigned int v4_coded = (flags_values & 0x80000000u);
				flags_values <<= 1;
				flags_remain--;
				
				if(v4_coded)
				{
					//Block coded as V4
					cvid_draw_v4(dest, data_ptr);
					data_ptr += 4;
					data_len -= 4;
				}
				else
				{
					//Block coded as V1
					cvid_draw_v1(dest, data_ptr);
					data_ptr += 1;
					data_len -= 1;
				}
			}	
			
			outx += 4;
			if(outx >= strip_x1)
			{
				outx = strip_x0;
				outy += 4;
			}
		}
		
		return;
	}

	fprintf(stderr, "sfilm: Unknown cvid chunk in strip data\n");
	return;
}

//Handles a cvid strip
static void cvid_strip(unsigned char *strip_ptr, int strip_len)
{
	//Read strip header	
	uint16_t strip_type = 0;
	strip_type = (strip_type << 8) | strip_ptr[0];
	strip_type = (strip_type << 8) | strip_ptr[1];
	
	if(strip_type != 0x1000 && strip_type != 0x1100)
	{
		fprintf(stderr, "sfilm: invalid cvid strip type\n");
		return;
	}
	
	uint16_t strip_datalen = 0;
	strip_datalen = (strip_datalen << 8) | strip_ptr[2];
	strip_datalen = (strip_datalen << 8) | strip_ptr[3];
	
	strip_y0 = 0;
	strip_y0 = (strip_y0 << 8) | strip_ptr[4];
	strip_y0 = (strip_y0 << 8) | strip_ptr[5];
	
	strip_x0 = 0;
	strip_x0 = (strip_x0 << 8) | strip_ptr[6];
	strip_x0 = (strip_x0 << 8) | strip_ptr[7];
	
	strip_y1 = 0;
	strip_y1 = (strip_y1 << 8) | strip_ptr[8];
	strip_y1 = (strip_y1 << 8) | strip_ptr[9];
	
	strip_x1 = 0;
	strip_x1 = (strip_x1 << 8) | strip_ptr[10];
	strip_x1 = (strip_x1 << 8) | strip_ptr[11];
	
	strip_ptr += 12;
	strip_len -= 12;
	
	//Work through cvid data in strip
	while(strip_len > 0)
	{
		uint16_t data_id = 0;
		data_id = (data_id << 8) | strip_ptr[0];
		data_id = (data_id << 8) | strip_ptr[1];
		
		uint16_t data_bytes = 0;
		data_bytes = (data_bytes << 8) | strip_ptr[2];
		data_bytes = (data_bytes << 8) | strip_ptr[3];
		
		if(data_bytes < 4)
		{
			fprintf(stderr, "sfilm: cvid chunk too short\n");
			return;
		}
		
		if(data_bytes > strip_len)
		{
			fprintf(stderr, "sfilm: cvid chunk too long for strip data\n");
			return;
		}
		
		cvid_datum(strip_ptr, data_bytes);
		
		strip_ptr += data_bytes;
		strip_len -= data_bytes;
	}
	
	//Apparently these are supposed to stack vertically even if they give the same Y range
	strip_y_done += strip_y1 - strip_y0;
}

//Handles a video chunk
static void sample_video(unsigned char *frame_ptr, int frame_len)
{
	//Video sample
	//Spew these out as quickly as we can, because they happen occasionally between audio samples.
	//Todo - maybe detect if we're running behind and just skip to the next intracoded frame	
	unsigned char *video_chunk_header = frame_ptr;
	frame_ptr += 12;
	frame_len -= 12;
	
	//Read "deviant cvid" header
	uint8_t flags = video_chunk_header[0];
	(void)flags;
	
	uint32_t cvid_len = 0;
	cvid_len = (cvid_len << 8) | video_chunk_header[1];
	cvid_len = (cvid_len << 8) | video_chunk_header[2];
	cvid_len = (cvid_len << 8) | video_chunk_header[3];
	
	uint16_t coded_width = 0;
	coded_width = (coded_width << 8) | video_chunk_header[4];
	coded_width = (coded_width << 8) | video_chunk_header[5];
	
	uint16_t coded_height = 0;
	coded_height = (coded_height << 8) | video_chunk_header[6];
	coded_height = (coded_height << 8) | video_chunk_header[7];
	
	uint16_t nstrips = 0;
	nstrips = (nstrips << 8) | video_chunk_header[8];
	nstrips = (nstrips << 8) | video_chunk_header[9];
	
	strip_y_done = 0;
	
	(void)video_chunk_header[10];
	(void)video_chunk_header[11];
	
	//Read each data strip in the cvid chunk
	for(uint16_t strip = 0; strip < nstrips; strip++)
	{
		//Read strip header
		unsigned char *video_strip_header = frame_ptr;
		
		uint16_t strip_type = 0;
		strip_type = (strip_type << 8) | video_strip_header[0];
		strip_type = (strip_type << 8) | video_strip_header[1];
		
		uint16_t strip_datalen = 0;
		strip_datalen = (strip_datalen << 8) | video_strip_header[2];
		strip_datalen = (strip_datalen << 8) | video_strip_header[3];
		
		cvid_strip(frame_ptr, strip_datalen);
		
		frame_ptr += strip_datalen;
		frame_len -= strip_datalen;
	}
	
	if(frame_len != 0)
	{
		fprintf(stderr, "sfilm: cvid frame incorrect length\n");
		return;
	}
	
	//Finished processing this cvid frame.
	
	//Time to display

	//Triple-buffer so we don't wait on any video stuff
	//Screw it, single-buffer, who cares
	_sc_gfx_flip(video_mode, framebuffer_0);	
}

//Plays the FMV and then returns
static void dofmv(const char *filename)
{
	//Set up data streaming
	int init_result = streamer_init(filename);
	if(init_result < 0)
	{
		fprintf(stderr, "sfilm: Failed to open file %s\n", filename);
		return;
	}
	
	//Read beginning of FILM header
	char film_magic[4] = {0};
	uint32_t film_headerlen = 0;
	char film_version[4] = {0};
	
	memcpy(film_magic, streamer_get(4), 4);
	cpy4be(&film_headerlen, streamer_get(4), 4);
	memcpy(film_version, streamer_get(4), 4);
	streamer_get(4); //Skip mystery 4 bytes
	
	if(memcmp(film_magic, "FILM", 4) != 0)
	{
		//Not a FILM file
		fprintf(stderr, "sfilm: Failed to find FILM header.\n");
		return;
	}
	
	if(memcmp(film_version, "1.09", 4) != 0)
	{
		//Wrong version FILM file
		fprintf(stderr, "sfilm: Not a v1.09 FILM file.\n");
		return;
	}
	
	//Read chunks from FILM header
	uint32_t header_consumed = 16; //Header length at the very beginning includes what we already parsed
	while(header_consumed < film_headerlen)
	{
		//First 4 bytes are always chunk type, next 4 bytes always length of chunk
		char chunktype[4] = {0};
		memcpy(chunktype, streamer_get(4), 4);
		
		uint32_t chunklen = 0;
		cpy4be(&chunklen, streamer_get(4), 4);
		
		//Purported size of chunk should fit in remaining FILM header space
		if(header_consumed + chunklen > film_headerlen)
		{
			fprintf(stderr, "sfilm: Overlong chunk in FILM header.\n");
			return;
		}
		
		if(chunklen <= 8)
		{
			fprintf(stderr, "sfilm: Invalid size of chunk in FILM header.\n");
			return;
		}
		
		//Double-check how much we've consumed and the intended size of the chunk
		//(We've already read the 4-byte type and 4-byte length at this point)
		uint32_t chunk_streamed = 8;
		
		if(!memcmp(chunktype, "FDSC", 4))
		{
			//Film description
			char fourcc[4] = {0};
			
			uint32_t frameheight = 0;
			uint32_t framewidth = 0;
			
			char bitspp;
			char audio_channel;
			char audio_res;
			char audio_compress;
			uint16_t audio_freq;
			
			memcpy(fourcc,          streamer_get(4), 4); chunk_streamed += 4;
			cpy4be(&frameheight,    streamer_get(4), 4); chunk_streamed += 4;
			cpy4be(&framewidth,     streamer_get(4), 4); chunk_streamed += 4;
			memcpy(&bitspp,         streamer_get(1), 1); chunk_streamed += 1;
			memcpy(&audio_channel,  streamer_get(1), 1); chunk_streamed += 1;
			memcpy(&audio_res,      streamer_get(1), 1); chunk_streamed += 1;
			memcpy(&audio_compress, streamer_get(1), 1); chunk_streamed += 1;
			cpy2be(&audio_freq,     streamer_get(2), 2); chunk_streamed += 2;
			(void)streamer_get(6);                       chunk_streamed += 6;
			
			if(memcmp(fourcc, "cvid", 4) != 0)
			{
				fprintf(stderr, "sfilm: FOURCC code not cvid, unsupported.\n");
				return;
			}
			
			#if SFILM_FORCE_VIDEO_MODE == _SC_GFX_MODE_VGA_16BPP
				if(framewidth != 640 || frameheight != 480)
				{
					fprintf(stderr, "sfilm: Built for only 640x480 video. Cannot play.\n");
					return;
				}
			#elif SFILM_FORCE_VIDEO_MODE == _SC_GFX_MODE_320X240_16BPP
				if(framewidth != 320 || frameheight != 240)
				{
					fprintf(stderr, "sfilm: Built for only 320x240 video. Cannot play.\n");
					return;
				}
			#else
				if(framewidth == 320 && frameheight == 240)
				{
					video_mode = _SC_GFX_MODE_320X240_16BPP;
				}
				else if(framewidth == 640 && frameheight == 480)
				{
					video_mode = _SC_GFX_MODE_VGA_16BPP;
				}
				else
				{
					fprintf(stderr, "sfilm: Video size not 320x240 or 640x480, unsupported.\n");
					return;
				}
				
				video_pitch = ((video_mode == _SC_GFX_MODE_VGA_16BPP)?640:320)/4;
			#endif
			
			
			
			if(audio_channel != 1)
			{
				fprintf(stderr, "sfilm: Found non-mono file, unsupported.\n");
				return;
			}
			
			if(audio_compress != 0)
			{
				fprintf(stderr, "sfilm: Audio compression nonzero, unsupported.\n");
				return;
			}
			
			if(audio_freq != 24000)
			{
				fprintf(stderr, "sfilm: Audio frequency not 24000, unsupported.\n");
				return;
			}
			
			if(audio_res != 16)
			{
				fprintf(stderr, "sfilm: Audio resolution not 16, unsupported.\n");
				return;
			}
		}
		else if(!memcmp(chunktype, "STAB", 4))
		{
			//Sample table
			
			//Load sample table header
			uint32_t framerate_base = 0;
			uint32_t nentries = 0;
			
			cpy4be(&framerate_base, streamer_get(4), 4); chunk_streamed += 4;
			cpy4be(&nentries,       streamer_get(4), 4); chunk_streamed += 4;
			
			if(framerate_base < 1 || framerate_base > 1000)
			{
				fprintf(stderr, "sfilm: Out-of-bounds framerate timebase.\n");
				return;
			}
			
			if(nentries <= 0 || nentries > STAB_MAX)
			{
				fprintf(stderr, "sfilm: Too many samples in sample table (too long video).\n");
				return;
			}
			
			//Set aside values from header
			stab_timebase = framerate_base;
			stab_total = nentries;
			
			//Load body of sample table
			for(int ee = 0; ee < stab_total; ee++)
			{
				cpy4be(&(stab[ee]), streamer_get(sizeof(stab[ee])), sizeof(stab[ee]));
				chunk_streamed += sizeof(stab[ee]);
			}	
		}
		else
		{
			//Unknown header chunk, skip past it
			uint32_t unknown_stuff = chunklen - chunk_streamed;
			
			(void)streamer_get(unknown_stuff);
			chunk_streamed += unknown_stuff;
		}
			
		if(chunk_streamed != chunklen)
		{
			//Code problem, we didn't consume the entire declared length of the header...
			fprintf(stderr, "sfilm: Chunk has unexpected size in FILM header.\n");
			return;
		}
		header_consumed += chunk_streamed;
	}
	
	fprintf(stderr, "sfilm: Read FILM header OK.\n");
	
	//Track how far into the data region we read.
	//Assume sample table entries are sorted, i.e. in-order in the file as well
	uint32_t datapos = 0;
	
	//Work our way through the sample table
	while(stab_next < stab_total)
	{
		//Look at the next sample
		const stab_t *sptr = &(stab[stab_next]);
		stab_next++;
		
		//Samples should be in-order
		if(datapos > sptr->offset)
		{
			//We already read past the sample...?
			fprintf(stderr, "sfilm: Sample table gives sample locations out-of-order, can't play.\n");
			return;
		}
		
		//Allow gaps between them tho
		if(datapos < sptr->offset)
		{
			uint32_t discard = sptr->offset - datapos;
			(void)streamer_get(discard);
			datapos += discard;
		}
		
		//Is it an audio sample or a video sample?
		if(sptr->info1 == 0xFFFFFFFFu)
		{
			//Audio sample
			sample_audio(streamer_get(sptr->length), sptr->length);
			datapos += sptr->length;
		}
		else
		{
			sample_video(streamer_get(sptr->length), sptr->length);
			datapos += sptr->length;
		}
	}
}

//Launches the given executable when we're done with the movie
static void doexec(const char *filename)
{
	//Standard library handles this, including preserving environment across exec
	#if USE_SDLSC
		(void)filename;
		exit(0);
	#else
		while(1) { _sc_gfx_flip(0, NULL); _sc_pause(); } //For debugging, we can hold here at text-mode
		execlp(filename, filename, NULL);
	#endif
}

//Entry point, calls the two major top-level features (does the FMV, does the exec)
int main(int argc, const char **argv)
{
	//Defaults if we aren't given arguments
	const char *streamfile = "DEFAULT.CPK";
	const char *execfile = "BOOT.NNE";
	
	//Can pass arguments to this program to have it play a particular movie and return to a particular executable
	if(argc > 2)
	{
		streamfile = argv[1];
		execfile = argv[2];
	}

	dofmv(streamfile);
	fprintf(stderr, "sfilm: FMV done\n");
	doexec(execfile);
	return 0;
}
