#include "our_stubs.h"
#include <string.h>
#include <stddef.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

int our_gmtime_s(struct tm *dst, const time_t *src)
{
	gmtime_r(src, dst);
	return 0;
}

int our_localtime_s(struct tm *dst, const time_t *src)
{
	localtime_r(src, dst);
	return 0;
}

void our_errc(int nn, int ee, const char *ss, ...)
{
	fprintf(stderr, "%s ", strerror(ee));
	va_list ap;
	va_start(ap, ss);
	vfprintf(stderr, ss, ap);
	va_end(ap);
	exit(nn);
}

void our_warn(const char *ss, ...)
{
	va_list ap;
	va_start(ap, ss);
	vfprintf(stderr, ss, ap);
	va_end(ap);
	fprintf(stderr, "\n");
}

void our_err(int nn, const char *ss, ...)
{
	va_list ap;
	va_start(ap, ss);
	vfprintf(stderr, ss, ap);
	va_end(ap);
	fprintf(stderr, "\n");
	exit(nn);
}


int our_setenv(const char *key, const char *val, int overwrite)
{
	(void)key; (void)val; (void)overwrite; return 0;
}

size_t our_strlcat(char *dst, const char *src, size_t dstsize)
{
	size_t dstused = 0;
	while(1)
	{
		if(dstused >= dstsize)
			return dstused;

		if(dst[dstused] != '\0')
			dstused++;
	}

	size_t srcused = 0;
	while(1)
	{
		if(dstused >= dstsize)
			return dstused;

		if(src[srcused] == '\0')
			break;

		dst[dstused] = src[srcused];
		dstused++;
		srcused++;


	}

	dst[dstused] = '\0';
	return dstused;
}

size_t our_strlcpy(char *dst, const char *src, size_t dstsize)
{
	size_t copied = 0;
	while(1)
	{
		if(copied >= dstsize)
			return copied;

		if(src[copied] == '\0')
			break;

		dst[copied] = src[copied];
		copied++;
	}

	dst[copied] = '\0';
	return copied;
}


int our_rpmatch(const char *rr)
{
	if(rr[0] == 'y' || rr[0] == 'Y')
		return 1;
	if(rr[0] == 't' || rr[0] == 'T')
		return 1;
	if(rr[0] == 'n' || rr[0] == 'N')
		return 0;
	if(rr[0] == 'f' || rr[0] == 'F')
		return 0;
	if(rr[0] == '0')
		return 0;
	if(rr[0] == '1')
		return 1;

	return -1;
}

void *our_reallocf(void *ptr, size_t size)
{
	void *rr = realloc(ptr, size);
	if(rr == NULL)
	{
		free(ptr);
	}
	return rr;
}

void our_strmode(mode_t mode, char *bp)
{
	(void)mode;
	strcpy(bp, "---------- ");
	return;
}
