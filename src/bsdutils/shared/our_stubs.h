//our_err.h
//Stuff that BSD utilities depend on, which Windows does not have
//Bryan E. Topp <betopp@betopp.com> 2025
#ifndef OUR_ERR_H
#define OUR_ERR_H

#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>
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

#ifndef _UID_T_DECLARED
	typedef int uid_t;
	#define _UID_T_DECLARED
#endif

#ifndef _GID_T_DECLARED
	typedef int gid_t;
	#define _GID_T_DECLARED
#endif

typedef uint64_t u_long;
typedef uint64_t nlink_t;

//typedef uint32_t sig_atomic_t;
#include <signal.h>

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

#ifndef LINE_MAX
#define LINE_MAX 1024
#endif

#ifndef QUAD_MAX
#define QUAD_MAX LLONG_MAX
#endif

#endif //OUR_ERR_H

