//streamer.c
//Disk streaming for FMV player
//Bryan E. Topp <betopp@betopp.com> 2024

//For now just use this trivial implementation because we don't need great performance for 320x240 Cinepak
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

FILE *streamfile;
char *streambuf;
int streambuflen;

int streamer_init(const char *filename)
{
	if(streamfile != NULL)
		fclose(streamfile);

	streamfile = fopen(filename, "rb");
	if(streamfile == NULL)
		return -1;
	else
		return 0;
}

void *streamer_get(int nbytes)
{
	if(nbytes > streambuflen)
	{
		streambuf = realloc(streambuf, nbytes);
		streambuflen = nbytes;
	}
	
	int nread = 0;
	if(streamfile != NULL)
		nread = fread(streambuf, 1, nbytes, streamfile);
		
	if(nread < nbytes)
		memset(streambuf + nread, 0x00, nbytes - nread);
	
	return streambuf;
}

void streamer_pump(void)
{

}
