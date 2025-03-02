
#define NO_CURSES 1
#define HAVE_SETRESGID 1
#define HAVE_SETRESUID 1

#define HAVE_TIMERCLEAR 1
#define HAVE_CONFSTR 1

#ifdef __MINGW32__
	#include "winstubs.h"
#else
	#define HAVE_ST_MTIMESPEC 1
	#define HAVE_SIGLIST 1
#endif //windows

