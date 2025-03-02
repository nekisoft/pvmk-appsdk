//winstubs.c
//Windows stubs for OKSH
//Bryan E. Topp <betopp@betopp.com> 2025

#include "winstubs.h"

#if defined(__MINGW32__) || defined(WINNT)

#include <errno.h>
#include <string.h>
#include <stdio.h>

int kill(int pp, int ss)
{
	(void)pp;
	(void)ss;
	errno = ENOSYS;
	return -1;
}

int killpg(int gg, int ss)
{
	(void)gg;
	(void)ss;
	errno = ENOSYS;
	return -1;
}

int sigprocmask(int how, const sigset_t *set, sigset_t *oset)
{
	(void)how;
	(void)set;
	(void)oset;
	errno = ENOSYS;
	return -1;
}

void sigemptyset(sigset_t *s)
{
	*s = 0;
}

void sigaddset(sigset_t *s, int sig)
{
	*s |= (1ull << sig);
}

const char *strsignal(int ss)
{
	(void)ss;
	return "NoSignalsUsed";
}

int getppid(void)
{
	return 1;
}

int getsid(int pid)
{
	return pid;
}

int getpgid(int pid)
{
	return pid;
}

int getpgrp(void)
{
	return 1;
}

int getegid(void)
{
	return 1;
}

int getuid(void)
{
	return 1;
}

int geteuid(void)
{
	return 1;
}

int getgid(void)
{
	return 1;
}

int setpgid(int pp, int gg)
{
	(void)pp;
	(void)gg;
	errno = ENOSYS;
	return -1;
}

void setresgid(int rgid, int egid, int sgid)
{
	(void)rgid; (void)egid; (void)sgid;
}

void setresuid(int ruid, int euid, int suid)
{
	(void)ruid; (void)euid; (void)suid;
}

void setgroups(int ngroups, int *pgroups)
{
	(void)ngroups;
	(void)pgroups;
}

int readlink(const char *path, char *bufptr, int buflen)
{
	strncpy(bufptr, path, buflen);
	return strlen(path);
}

int fcntl(int fd, int op, ...)
{
	(void)fd;
	(void)op;
	errno = ENOSYS;
	return -1;
}

void sigaction(int sig, const struct sigaction *sa, struct sigaction *saout)
{
	(void)sig;
	(void)sa;
	(void)saout;
	return;
}

void sigsuspend(sigset_t *s)
{
	(void)s;
}

int tcsetattr(int fd, int action, const struct termios *t)
{
	(void)fd;
	(void)action;
	(void)t;
	errno = ENOSYS;
	return -1;
}

int tcgetattr(int fd, struct termios *t)
{
	(void)fd;
	(void)t;
	errno = ENOSYS;
	return -1;
}

int tcsetpgrp(int fd, int grp)
{
	(void)fd;
	(void)grp;
	errno = ENOSYS;
	return -1;
}

int tcgetpgrp(int fd)
{
	(void)fd;
	return 1;
}

void nice(int n)
{
	(void)n;
}

int fork(void)
{
	errno = ENOSYS;
	return -1;
}

void gethostname(char *buf, size_t len)
{
	snprintf(buf, len-1, "%s", "pvmkhost");
}

int alarm(int timeout)
{
	(void)timeout;
	errno = ENOSYS;
	return -1;
}

char *ttyname(int fd)
{
	(void)fd;
	return "tty";
}


size_t confstr(int which, char *buf, size_t len)
{
	(void)which;
	if(len > 0)
		buf[0] = 0;
	return 0;
}

#endif //#if defined(__MINGW32__) || defined(WINNT)
