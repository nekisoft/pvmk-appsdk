//our_err.h
//Stuff that BSD utilities depend on, which Windows does not have
//Bryan E. Topp <betopp@betopp.com> 2025
#ifndef OUR_ERR_H
#define OUR_ERR_H

#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

#ifndef __dead2
	#define __dead2
#endif

#ifndef __restrict
	#define __restrict
#endif

#ifndef __unused
	#define __unused
#endif

#ifndef nitems
	#define nitems(jj) ((int)(((int)sizeof(jj)/sizeof(jj[0]))))
#endif

#define OUR_MAX(a,b) (((a)>(b))?(a):(b))

#define our_errx our_err
#define our_warnx our_warn


#ifndef _UID_T_DECLARED
	typedef int uid_t;
	#define _UID_T_DECLARED
	#define NO_USERS 1
#endif

#ifndef _GID_T_DECLARED
	typedef int gid_t;
	#define _GID_T_DECLARED
	#define NO_GROUPS 1
#endif

#ifndef O_BINARY
	#define O_BINARY 0
#endif

void our_errc(int nn, int ee, const char *ss, ...) __attribute__((noreturn));
void our_err(int nn, const char *ss, ...) __attribute__((noreturn));
void our_warn(const char *ss, ...);
int our_setenv(const char *key, const char *val, int overwrite);
size_t our_strlcat(char *dst, const char *str, size_t dstsize);
size_t our_strlcpy(char *dst, const char *str, size_t dstsize);
int our_rpmatch(const char *rr);
void *our_reallocf(void *ptr, size_t size);
void our_strmode(mode_t mode, char *bp);
int our_gmtime_s(struct tm *dst, const time_t *src);
int our_localtime_s(struct tm *dst, const time_t *src);
int our_fchdir(int fd);
int our_lstat(const char *path, struct stat *sb);
int our_fchown(int fd, uid_t owner, gid_t group);
int our_lchown(const char *path, uid_t owner, gid_t group);
int our_utimes(const char *path, const struct timeval *times);
int our_lutimes(const char *path, const struct timeval *times);
int our_mknod(const char *path, mode_t mode, dev_t dev);
int our_mkfifo(const char *path, mode_t mode);
int our_fchmod(int fd, mode_t mode);
int our_lchmod(const char *path, mode_t mode);
int our_readlink(const char *path, char *buf, size_t bufsiz);
int our_symlink(const char *name1, const char *name2);
int our_link(const char *name1, const char *name2);
int our_chown(const char *path, uid_t owner, gid_t group);
void our_timespec_to_timeval(struct timeval *tv_out, const struct timespec *ts_in);
int our_mkdir(const char *path, mode_t mode);
mode_t our_getmode(const void *set, mode_t mode);
void *our_setmode(const char *mode_str);
char *our_realpath(const char *path, char *resolved);
uid_t our_geteuid(void);
const char *our_user_from_uid(uid_t uid, int nouser);
const char *our_group_from_gid(gid_t gid, int nogroup);
int our_fsync(int fd);

#ifndef TIMESPEC_TO_TIMEVAL
#define TIMESPEC_TO_TIMEVAL our_timespec_to_timeval
#endif

typedef uint64_t u_long;
typedef uint64_t nlink_t;

#ifndef EX_USAGE
#define EX_USAGE 1
#endif

//typedef uint32_t sig_atomic_t;
#include <signal.h>

#ifndef O_NONBLOCK
#define O_NONBLOCK 0
#endif

#ifndef O_NOFOLLOW
#define O_NOFOLLOW 0
#endif

#ifndef O_CLOEXEC
#define O_CLOEXEC 0
#endif

#ifndef O_DIRECTORY
#define O_DIRECTORY 0
#endif

#ifndef S_ISUID
#define S_ISUID 0
#endif

#ifndef S_ISGID
#define S_ISGID 0
#endif

#ifndef S_ISTXT
#define S_ISTXT 0
#endif

#ifndef S_ISVTX
#define S_ISVTX 0
#endif

#ifndef LINE_MAX
#define LINE_MAX 1024
#endif

#ifndef QUAD_MAX
#define QUAD_MAX LLONG_MAX
#endif

#endif //OUR_ERR_H

