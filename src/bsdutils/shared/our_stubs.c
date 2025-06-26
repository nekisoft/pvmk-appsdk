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

#ifndef NO_USERS
#include <pwd.h>
#endif

#ifndef NO_GROUPS
#include <grp.h>
#endif

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
	return strdup(mode_str);
}

mode_t our_getmode(const void *set, mode_t mode)
{
	const char *mode_str = (const char*)set;
	
	//check for numeric mode
	char *numend = NULL;
	int oct = strtoul(mode_str, &numend, 8);
	if(numend != NULL && numend != mode_str)
		return oct;

	while(*mode_str != '\0')
	{
		int do_u = 0;
		int do_g = 0;
		int do_o = 0;
		while(*mode_str != '\0')
		{
			if(*mode_str == 'u')
			{
				do_u = 1;
				mode_str++;
			}
			else if(*mode_str == 'g')
			{
				do_g = 1;
				mode_str++;
			}
			else if(*mode_str == 'o')
			{
				do_o = 1;
				mode_str++;
			}
			else if(*mode_str == 'a')
			{
				do_u = 1;
				do_g = 1;
				do_o = 1;
				mode_str++;
			}
			else
			{
				if(!(do_u || do_g || do_o))
				{
					do_u = 1;
					do_g = 1;
					do_o = 1;
				}
				break;
			}
		}

		int do_plus = 0;
		int do_minus = 0;
		int do_equals = 0;
		while(*mode_str != '\0')
		{
			if(*mode_str == '+')
			{
				do_plus = 1;
				mode_str++;
			}
			else if(*mode_str == '-')
			{
				do_minus = 1;
				mode_str++;
			}
			else if(*mode_str == '=')
			{
				do_equals = 1;
				mode_str++;
			}
			else
			{
				if(do_plus && do_minus)
				{
					do_plus = 0;
					do_minus = 0;
					do_equals = 1;
				}
				if(!(do_plus || do_minus || do_equals))
				{
					do_equals = 1;
				}
				if(do_plus || do_minus)
				{
					do_equals = 0;
				}
				break;
			}
		}

		mode_t mask = 0;
		while(*mode_str != '\0')
		{
			if(*mode_str == 'r')
			{
				if(do_u)
					mask |= S_IRUSR;
				if(do_g)
					mask |= S_IRGRP;
				if(do_o)
					mask |= S_IROTH;

				mode_str++;
			}
			else if(*mode_str == 'w')
			{
				if(do_u)
					mask |= S_IWUSR;
				if(do_g)
					mask |= S_IWGRP;
				if(do_o)
					mask |= S_IWOTH;

				mode_str++;
			}
			else if(*mode_str == 'x')
			{
				if(do_u)
					mask |= S_IXUSR;
				if(do_g)
					mask |= S_IXGRP;
				if(do_o)
					mask |= S_IXOTH;

				mode_str++;
			}
			else
			{
				break;
			}
		}

		if(do_plus)
			mode |= mask;
		if(do_minus)
			mode &= ~mask;
		if(do_equals)
			mode = mask;

		mask = 0;
		if(*mode_str == ',')
			mode_str++;
		else
			break;
	}
}

char *our_realpath(const char *path, char *resolved)
{
#ifdef __MINGW32__
	return _fullpath(resolved, path, PATH_MAX);
#else
	return realpath(path, resolved);
#endif
}

uid_t our_geteuid(void)
{
#ifdef NO_USERS
	return 0;
#else
	return geteuid();
#endif
}

int our_fsync(int fd)
{
#ifdef __MINGW32__
	//nothing
	(void)fd;
#else
	return fsync(fd);
#endif
}

const char *our_user_from_uid(uid_t u, int nouser)
{
#ifdef NO_USERS
	return "";
#else
	static char numbuf[16] = {0};
	snprintf(numbuf, sizeof(numbuf)-1, "%d", u);

	struct passwd *p = getpwuid(u);
	if(p == NULL)
		return nouser?NULL:numbuf;
	else
		return p->pw_name;
	//return user_from_uid(u, nouser); //only works on BSD, not Linux
#endif
}

const char *our_group_from_gid(gid_t g, int nogroup)
{
#ifdef NO_GROUPS
	return "";
#else
	static char numbuf[16] = {0};
	snprintf(numbuf, sizeof(numbuf)-1, "%d", g);

	struct group *p = getgrgid(g);
	if(p == NULL)
		return nogroup?NULL:numbuf;
	else
		return p->gr_name;
	//return group_from_gid(g, nogroup); //only works on BSD, not Linux
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
