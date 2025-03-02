//winstubs.h
//Windows stubs for OKSH
//Bryan E. Topp <betopp@betopp.com> 2025
#ifndef WINSTUBS_H
#define WINSTUBS_H

#if defined(__MINGW32__) || defined(WINNT)

#define NORUSAGE 1
#define NOIOCTL 1
#define NOTERMIOS 1
#define NOPASSWD 1
#define NOFLOCK 1
#define NOSYSWAIT 1

#include <sys/types.h>
#include <signal.h>
#include <setjmp.h>

typedef int uid_t;
typedef int gid_t;
typedef jmp_buf sigjmp_buf;
#define sigsetjmp(X,Y) _setjmp(X,Y)
#define siglongjmp(X,Y) longjmp(X,Y)

int kill(int pp, int ss);
int killpg(int gg, int ss);
int sigprocmask(int how, const sigset_t *set, sigset_t *oset);
#define SIG_BLOCK 1
#define SIG_UNBLOCK 2
#define SIG_SETMASK 3

void sigemptyset(sigset_t *s);
void sigaddset(sigset_t *s, int sig);
const char *strsignal(int ss);

#define SIGTSTP 18
#define SIGCONT 19
#define SIGCHLD 20
#define SIGTTIN 21
#define SIGTTOU 22

#define WEXITSTATUS(x) 0
#define WTERMSIG(x) 0
#define WIFSIGNALED(x) 0
#define WIFSTOPPED(x) 0
#define WSTOPSIG(x) 0
#define WCOREDUMP(x) 0
#define WNOHANG 0
#define WUNTRACED 0


int getppid(void);
int getsid(int pid);
int getpgid(int pid);
int getpgrp(void);
int getegid(void);
int getuid(void);
int geteuid(void);
int getgid(void);
int setpgid(int pp, int gg);
void setresgid(int rgid, int egid, int sgid);
void setresuid(int ruid, int euid, int suid);
void setgroups(int ngroups, int *pgroups);

int readlink(const char *path, char *bufptr, int buflen);

int fcntl(int fd, int op, ...);
#define F_SETFD 1
#define F_GETFL 2
#define F_SETFL 3
#define F_DUPFD 4
#define FD_CLOEXEC 1
#define S_ISVTX 0
#define S_ISUID 0
#define S_ISGID 0
#define S_ISLNK(x) 0
#define S_ISSOCK(x) 0

#define lstat stat
#define pipe(ff) _pipe(ff, 512, O_BINARY)

//Bogus values from my FreeBSD machine
#define SIGALRM 22
#define SIGWINCH 23
#define VEOF 0
#define VERASE 3
#define VKILL 5
#define VINTR 8
#define VQUIT 9
#define VMIN 16
#define VTIME 17
#define NCCS 20
#define ICANON 0x100
#define ICRNL 0x100
#define ECHO 0x8
#define INLCR 0x40
#define _POSIX_VDISABLE 0xFF
#define ISIG 0x80

#define TCSADRAIN 1

struct winsize {
	unsigned short ws_row;
	unsigned short ws_col;
	unsigned short ws_xpixel;
	unsigned short ws_ypixel;
};

struct sigaction {
	void (*sa_handler)(int);
	sigset_t sa_mask;
	int sa_flags;
};

void sigaction(int sig, const struct sigaction *sa, struct sigaction *saout);
void sigsuspend(sigset_t *s);

struct termios {
	int c_iflag;
	int c_oflag;
	int c_cflag;
	int c_lflag;
	int c_cc[NCCS];
	int c_ispeed;
	int c_ospeed;
};

int tcsetattr(int fd, int action, const struct termios *t);
int tcgetattr(int fd, struct termios *t);
int tcsetpgrp(int fd, int grp);
int tcgetpgrp(int fd);

void nice(int n);

int fork(void);

void gethostname(char *buf, size_t len);
int alarm(int timeout);
char *ttyname(int fd);

#define MAXLOGNAME 10

#define O_NONBLOCK 0x100000


#define _CS_PATH 1
size_t confstr(int which, char *buf, size_t len);

#endif //#if defined(__MINGW32__) || defined(WINNT)
#endif //WINSTUBS_H