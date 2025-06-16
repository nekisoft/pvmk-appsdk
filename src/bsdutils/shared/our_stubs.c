#include "our_stubs.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <stddef.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <errno.h>

int our_lutimes(const char *path, const struct timeval *times)
{
#ifdef S_IFLNK
	return lutimes(path, times);
#else
	(void)path;
	(void)times;
	errno = ENOSYS;
	return -1;
#endif
}

int our_lchmod(const char *path, mode_t mode)
{
#ifdef S_IFLNK
	return lchmod(path, mode);
#else
	(void)path;
	(void)mode;
	errno = ENOSYS;
	return -1;
#endif
}

int our_lstat(const char *path, struct stat *sb)
{
#ifdef S_IFLNK
	return lstat(path, sb);
#else
	(void)path;
	(void)sb;
	errno = ENOSYS;
	return -1;
#endif
}

int our_gmtime_s(struct tm *dst, const time_t *src)
{
	memcpy(dst, gmtime(src), sizeof(*dst));
	return 0;
}

int our_localtime_s(struct tm *dst, const time_t *src)
{
	memcpy(dst, localtime(src), sizeof(*dst));
	return 0;
}

int our_fchdir(int fd)
{
	(void)fd;
	errno = ENOSYS;
	return -1;
}

int our_link(const char *name1, const char *name2)
{
#ifdef S_IFLNK
	return link(name1, name2);
#else
	(void)name1; (void)name2;
	errno = ENOSYS;
	return -1;
#endif
}

int our_mknod(const char *path, mode_t mode, dev_t dev)
{
#ifdef S_IFLNK
	return mknod(path, mode, dev);
#else
	(void)path; (void)mode; (void)dev;
	errno = ENOSYS;
	return -1;
#endif
}

int our_mkfifo(const char *path, mode_t mode)
{
#ifdef S_IFLNK
	return mkfifo(path, mode);
#else
	(void)path; (void)mode;
	errno = ENOSYS;
	return -1;
#endif
}

int our_fchown(int fd, uid_t owner, gid_t group)
{
#ifdef NO_USERS
	(void)fd;
	(void)owner;
	(void)group;
	errno = ENOSYS;
	return -1;
#else
	return fchown(fd, owner, group);
#endif
}

int our_lchown(const char *path, uid_t owner, gid_t group)
{
#ifdef NO_USERS
	(void)path;
	(void)owner;
	(void)group;
	errno = ENOSYS;
	return -1;
#else
	return lchown(path, owner, group);
#endif
}

int our_chown(const char *path, uid_t owner, gid_t group)
{
#ifdef NO_USERS
	(void)path;
	(void)owner;
	(void)group;
	errno = ENOSYS;
	return -1;
#else
	return chown(path, owner, group);
#endif
}

int our_readlink(const char *path, char *buf, size_t buflen)
{
#ifdef S_IFLNK
	return readlink(path, buf, buflen);
#else
	(void)path;
	(void)buf;
	(void)buflen;
	errno = ENOSYS;
	return -1;
#endif
}

int our_symlink(const char *path1, const char *path2)
{
#ifdef S_IFLNK
	return symlink(path1, path2);
#else
	(void)path1;
	(void)path2;
	errno = ENOSYS;
	return -1;
#endif
}

int our_utimes(const char *path, const struct timeval *times)
{
#ifdef st_atime
	return utimes(path, times);
#else
	(void)path;
	(void)times;
	errno = ENOSYS;
	return -1;
#endif
}

int our_fchmod(int fd, mode_t mode)
{
#ifdef S_IFLNK
	return fchmod(fd, mode);
#else
	(void)fd;
	(void)mode;
	errno = ENOSYS;
	return -1;
#endif
}

int our_mkdir(const char *path, mode_t mode)
{
#ifdef S_IFLNK
	return mkdir(path, mode);
#else
	return mkdir(path); (void)mode;
#endif
}

void *our_setmode(const char *mode_str)
{
#ifdef S_IFLNK
	return setmode(mode_str);
#else
	return NULL; (void)mode_str;
#endif
}

mode_t our_getmode(const void *set, mode_t mode)
{
#ifdef S_IFLNK
	return getmode(set, mode);
#else
	return mode; (void)set;
#endif
}

char *our_realpath(const char *path, char *resolved)
{
#ifdef __MINGW32__
	return _fullpath(resolved, path, PATH_MAX);
#else
	return realpath(path, resolved);
#endif
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
