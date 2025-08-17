//pvmkoslib.c
//OS support library for picolibc when running under PVMK
//Bryan E. Topp <betopp@betopp.com> 2024

#define _POSIX_C_SOURCE 200809L

//PVMK system call definitions
#include <sc.h>

#include <stdarg.h>
#include <libgen.h>
#include <sys/time.h>
#include <sys/dirent.h>
#include <stddef.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <utime.h>
#include <sys/times.h>

#include "cdfs.h"

//Table of open files on the CD filesystem
typedef struct _user_file_s
{
	int valid;
	uint32_t ino;
	int pos;
} _user_file_t;
#define USER_FILE_MIN 4
#define USER_FILE_MAX 256
_user_file_t _user_file_table[USER_FILE_MAX];
uint32_t _user_cwd;


//Define this to placate cxx things that expect dynamic linking
int __dso_handle = 0;

//Translates a PVMK system-call error return into a picolibc errno value
static int _pvmk_sysret_errno(int sysret)
{
	if(sysret >= 0)
		return 0;
	
	switch(sysret)
	{
		case -_SC_EPERM:        return EPERM;
		case -_SC_ENOENT:       return ENOENT;
		case -_SC_ESRCH:        return ESRCH;
		case -_SC_EIO:          return EIO;
		case -_SC_ENXIO:        return ENXIO;
		case -_SC_E2BIG:        return E2BIG;
		case -_SC_ECHILD:       return ECHILD;
		case -_SC_EAGAIN:       return EAGAIN;
		case -_SC_ENOMEM:       return ENOMEM;
		case -_SC_EFAULT:       return EFAULT;
		case -_SC_EINVAL:       return EINVAL;
		case -_SC_EFBIG:        return EFBIG;
		case -_SC_ENOSPC:       return ENOSPC;
		case -_SC_EROFS:        return EROFS;
		case -_SC_ENAMETOOLONG: return ENAMETOOLONG;
		case -_SC_ENOSYS:       return ENOSYS;
		default:                return ENOSYS;
	}
}

//Non-variadic version of openat.
static int _openatm(int fd, const char *path, int flags, mode_t mode)
{		
	(void)flags;
	
	//If they didn't specify a type of file, default to a "regular file" mode.
	if((mode & S_IFMT) == 0)
		mode |= S_IFREG;
	
	//Find a spot for the new file
	int newfd = -1;
	for(int uu = USER_FILE_MIN; uu < USER_FILE_MAX; uu++)
	{
		if(_user_file_table[uu].valid == 0)
		{
			newfd = uu;
			break;
		}
	}
	if(newfd < 0)
	{
		//No room
		errno = EMFILE;
		return -1;
	}
	
	//Pathname lookup
	uint32_t dir_ino = _cdfs_rootino();
	if(path[0] != '/')
	{
		//They passed a relative path - check the given file descriptor for what it's relative to
		if(fd == AT_FDCWD)
		{
			//They passed CWD as their starting point
			dir_ino = _user_cwd;
		}
		else
		{
			if(fd < USER_FILE_MIN)
			{
				//They passed stdin/stdout/stderr
				errno = ENOTDIR;
				return -1;
			}
			if(fd >= USER_FILE_MAX)
			{
				//They passed a bad file descriptor
				errno = EBADF;
				return -1;
			}
			dir_ino = _user_file_table[fd].ino;
		}
	}
	
	while(*path != '\0')
	{
		while(*path == '/')
			path++;
		
		char component_buf[256] = {0};
		int component_len = 0;
		for(const char *cc = path; *cc != '/' && *cc != '\0'; cc++)
		{
			component_buf[component_len] = *cc;
			component_len++;
			path++;
			if(component_len+1 >= (int)sizeof(component_buf))
				break;
		}
		
		int component_lookup = _cdfs_search(dir_ino, component_buf);
		if(component_lookup < 0)
		{
			errno = _pvmk_sysret_errno(component_lookup);
			return -1;
		}
		
		dir_ino = component_lookup;
	}
	
	_user_file_table[newfd].ino = dir_ino;
	_user_file_table[newfd].pos = 0;
	_user_file_table[newfd].valid = 1;
	return newfd;
}


//argenv buffer - aligned enough to refer to a char*, but ultimately storing strings
//static char *argenv_buffer[4096 / sizeof(void*)];

//Called from crt0 to call main
void _pvmk_callmain(void)
{
	//Force references to be held to Picolibc stdin/stdout/stderr.
	//Don't know why this is causing problems for us...
	volatile const void * volatile a = NULL;
	a = &stdin;
	a = &stdout;
	a = &stderr;
	a = stdin;
	a = stdout;
	a = stderr;
	(void)a;
	
	//Default arguments/environments if something goes wrong
	int argc = 1;
	char **argv = (char*[]){ "argv0",         NULL };
	char **envp = (char*[]){ "PVMK=external", "HOME=/", "PWD=/", NULL };
	
	//Try to load arguments/environment
	//int nread = _sc_env_load(argenv_buffer, sizeof(argenv_buffer));
	//SDK update - our linker makes a buffer, and whoever loads us can fill it up.
	//Separate arg/env syscalls have been eliminated.
	extern char _argenv_start[];
	extern char _argenv_end[];
	
	_argenv_end[-1] = '\0'; //Make sure they didn't overrun us
	
	//Still need to build pointers at the end
	char **argenv_buffer = (char**)_argenv_start;
	
	//Simulate the "nread" return value - count how many bytes are filled before the rest is NULs.
	int nread = (intptr_t)_argenv_end - (intptr_t)_argenv_start;
	while(nread > 0 && _argenv_start[nread-1] == '\0')
		nread--;
	
	if(nread > 0)
	{
		//Run through the strings we read. Count them and make sure there's room to build pointer arrays.
		//Each of the two pointer arrays ends, when a zero-length string is encountered.
		int nstrings = 0;
		int zerolen = 0;
		char *remaining = _argenv_start;
		char *stop = _argenv_end; 
		while(zerolen < 2 && remaining < stop)
		{
			if(strlen(remaining) == 0)
				zerolen++;

			nstrings++;
			remaining += strlen(remaining) + 1;
		}
		
		//See if we have enough room...
		int ptr_idx = (nread + (sizeof(void*)) - 1) / sizeof(void*); //Array index
		int ptr_capacity = ((intptr_t)_argenv_end - (intptr_t)_argenv_start) / sizeof(void*);
		if(ptr_idx + nstrings < ptr_capacity)
		{
			//Alright, have room for the pointers
			remaining = _argenv_start;
			argv = &(argenv_buffer[ptr_idx]);
			argc = 0;
			while(remaining < stop)
			{
				if(remaining[0] == '\0')
				{
					argenv_buffer[ptr_idx] = NULL;
					ptr_idx++;
					
					remaining++;
					break;
				}
				else
				{
					argenv_buffer[ptr_idx] = remaining;
					ptr_idx++;
					
					remaining += strlen(remaining) + 1;
					argc++;
				}
			}
			
			envp = &(argenv_buffer[ptr_idx]);
			while(remaining < stop)
			{
				if(remaining[0] == '\0')
				{
					argenv_buffer[ptr_idx] = NULL;
					ptr_idx++;
					
					remaining++;
					break;
				}
				else
				{
					argenv_buffer[ptr_idx] = remaining;
					ptr_idx++;
					
					remaining += strlen(remaining) + 1;
				}
			}
		}
	}
		
	//Set aside initial environment location
	extern char **environ;
	environ = envp;
	
	//Set signal actions to default
	//for(int ss = 0; ss < 64; ss++)
	//{
	//	signal(ss, SIG_DFL);
	//}
	//_sc_sig_mask(SIG_SETMASK, 0); //none masked
	
	//Validate filesystem on card
	_cdfs_init();
	_user_cwd = _cdfs_rootino();
	
	//Call constructors
	extern void __libc_init_array();
	__libc_init_array();
	
	//Call main
	extern int main();
	int main_returned = main(argc, argv, envp);
	
	//If main returns, call exit with its return value.
	exit(main_returned);
	
	//Exit shouldn't return
	_exit(main_returned);
	abort();
	while(1){}
}

int __aeabi_atexit (void *arg, void (*func) (void *), void *d)
{
	extern int __cxa_atexit();
	return __cxa_atexit (func, arg, d);
}


//Non-variadic version of open.
int _openm(const char *path, int flags, mode_t mode)
{
	//open is equivalent to openat with fd of AT_FDCWD
	return _openatm(AT_FDCWD, path, flags, mode);
}

int open(const char *path, int flags, ...)
{
	//Kill variadic parameter and call non-variadic version.
	mode_t mode = 0;
	if(flags & O_CREAT)
	{
		va_list ap;
		va_start(ap, flags);
		mode = va_arg(ap, mode_t);
		va_end(ap);
	}
	
	return _openm(path, flags, mode);
}

int openat(int fd, const char *path, int flags, ...)
{
	//Kill variadic parameter and call non-variadic version.
	mode_t mode = 0;
	if(flags & O_CREAT)
	{
		va_list ap;
		va_start(ap, flags);
		mode = va_arg(ap, mode_t);
		va_end(ap);
	}
	
	return _openatm(fd, path, flags, mode);
}

int close(int fd)
{
	if(fd < USER_FILE_MIN)
	{
		//Don't allow closing stdin/stdout/stderr
		errno = ENOSYS;
		return -1;
	}
	
	if(fd >= USER_FILE_MAX)
	{
		//Out of range
		errno = EBADF;
		return -1;
	}
	
	if(!_user_file_table[fd].valid)
	{
		//No file open here...
		errno = EBADF;
		return -1;
	}
	
	//Clear this file entry
	memset(&(_user_file_table[fd]), 0, sizeof(_user_file_table[fd]));
	return 0;
}

ssize_t read(int fd, void *buf, size_t nbyte)
{
	if(fd < USER_FILE_MIN)
	{
		if(fd == 0)
		{
			//stdin just ends
			return 0;
		}
		else if(fd == 1 || fd == 2)
		{
			//Can't read back from stdout/stderr
			return 0;
		}
		else
		{
			errno = EBADF;
			return -1;
		}
	}
	else if(fd >= USER_FILE_MAX || !_user_file_table[fd].valid)
	{
		//Bad file descriptor
		errno = EBADF;
		return -1;
	}
	else 
	{
		//User-level file read
		int nread = _cdfs_read(_user_file_table[fd].ino, _user_file_table[fd].pos, buf, nbyte);
		if(nread < 0)
		{
			errno = _pvmk_sysret_errno(nread);
			return -1;
		}
		
		_user_file_table[fd].pos += nread;
		return nread;
	}
}

ssize_t write(int fd, const void *buf, size_t nbyte)
{
	if(fd < USER_FILE_MIN)
	{
		//Do a write system-call to operate on system-level files (stdin/stdout/stderr)
		if(fd == 0)
		{
			//Can't write to stdin
			return 0;
		}
		if(fd == 1 || fd == 2)
		{
			//Print out stdout/stderr to text-mode console
			for(size_t bb = 0; bb < nbyte; bb++)
			{
				char twochar[2];
				twochar[0] = ((const char*)buf)[bb];
				twochar[1] = '\0';
				_sc_print(twochar);
			}
			return nbyte;
		}
		else
		{
			errno = EBADF;
			return -1;
		}
	}
	else
	{
		//User-level file write
		int nwritten = _cdfs_write(_user_file_table[fd].ino, _user_file_table[fd].pos, buf, nbyte);
		if(nwritten < 0)
		{
			errno = _pvmk_sysret_errno(nwritten);
			return -1;
		}
		
		_user_file_table[fd].pos += nwritten;
		return nwritten;
	}
}

off_t lseek(int fd, off_t offset, int whence)
{
	if(fd < USER_FILE_MIN)
	{
		//Can't seek stdin/stdout/whatever
		if(fd == 0 || fd == 1 || fd == 2)
		{
			errno = ESPIPE;
			return -1;
		}
		else
		{
			errno = EBADF;
			return -1;
		}
	}
	else if(fd >= USER_FILE_MAX || !_user_file_table[fd].valid)
	{
		//Bad file descriptor
		errno = EBADF;
		return -1;
	}
	else
	{
		//Seek in the CD-based file
		struct stat st = {0};
		int stat_result = fstat(fd, &st);
		if(stat_result < 0)
			return stat_result;
		
		int whence_val = 0;
		switch(whence)
		{
			case SEEK_SET: whence_val = 0; break;
			case SEEK_CUR: whence_val = _user_file_table[fd].pos; break;
			case SEEK_END: whence_val = st.st_size; break; 
			default: errno = EINVAL; return -1;
		}
		
		_user_file_table[fd].pos = whence_val + offset;
		if(_user_file_table[fd].pos < 0)
			_user_file_table[fd].pos = 0;
		
		return _user_file_table[fd].pos;
	}
}

void _exit(int status)
{
	//System update - _sc_exit is now a private call, games shouldn't use it.
	//Reset the process instead of exiting.
	//_sc_exit(status, 0);
	(void)status;
	while(1)
	{
		extern char _BSS_START[];
		extern char _BSS_END[];
		for(char *ii = _BSS_START; ii < _BSS_END; ii++)
		{
			*ii = 0x00;
		}
		
		extern void _start(void);
		_start();
	}
}

//Implementation of sbrk allowing growing the process from the size as-linked up to 24MBytes in total
static size_t _sbrk_top_fake;
void *sbrk(ptrdiff_t nbytes)
{
	if(_sbrk_top_fake == 0)
	{
		//Assume we can start using memory after the process as-linked
		extern uint8_t _LINK_END[]; //From linker script
		_sbrk_top_fake = (size_t)(uintptr_t)_LINK_END; //What our process can allocate beyond
		if(_sbrk_top_fake % 16)
		{
			//Discard a few bytes immediately after the image as linked, to 16-byte align
			_sbrk_top_fake += 16;
			_sbrk_top_fake -= _sbrk_top_fake % 16;
		}
	}
	
	//Return value will always be the old "fake" top of process
	void *retval = (void*)_sbrk_top_fake;
	
	//Always allow giving back memory
	if(nbytes < 0)
	{
		_sbrk_top_fake += nbytes; //nbytes is negative
		return retval;
	}
	
	//Can we service the request?
	//Assume that we have 24MBytes to work with (Neki32 standard) in our process.
	size_t wanted_top = _sbrk_top_fake + nbytes;
	extern uint8_t _HEAP_END[]; //From linker script
	if(wanted_top <= (size_t)(uintptr_t)_HEAP_END)
	{
		//Can provide this
		_sbrk_top_fake = wanted_top;
		return retval;
	}
	
	//Needed more memory than we're allowed
	errno = ENOMEM;
	return (void*)(-1);
}

int fstat(int fd, struct stat *out)
{
	
	
	if(fd < USER_FILE_MIN)
	{
		/*
		//Performing a stat on an open file is the way our kernel works natively.
		_sc_stat_t _sc_stat_buf = {0};
		int stat_err = _sc_stat(fd, &_sc_stat_buf, sizeof(_sc_stat_buf));
		if(stat_err < 0)
		{
			errno = _pvmk_sysret_errno(stat_err);
			return -1;
		}
		//Convert kernel stat struct to libc struct.
		//Todo - is it worth defining these to be the same?
		//Seems like there might be reasons not to, but I can't come up with any.
		memset(out, 0, sizeof(*out));
		out->st_dev = _sc_stat_buf.part;
		out->st_ino = _sc_stat_buf.ino;
		out->st_mode = _sc_stat_buf.mode;
		out->st_size = _sc_stat_buf.size;
		out->st_rdev = _sc_stat_buf.rdev;
		out->st_blocks = _sc_stat_buf.used / 512;
		*/
		
		out->st_dev = 0;
		out->st_ino = 0;
		out->st_mode = S_IFCHR;
		out->st_size = 0;
		out->st_rdev = fd;
		out->st_blocks = 0;
		return 0;
	}
	else if(fd >= USER_FILE_MAX || !_user_file_table[fd].valid)
	{
		//Bad file descriptor
		errno = EBADF;
		return -1;
	}
	else
	{
		//Stat the file on the CD
		int cdstat = _cdfs_stat(_user_file_table[fd].ino, out);
		if(cdstat < 0)
		{
			errno = _pvmk_sysret_errno(cdstat);
			return -1;
		}
	}

	return 0;	
}

int fstatat(int at_fd, const char *path, struct stat *out, int flag)
{
	//Our kernel has the concept of "open for stat", without needing permission for read nor write nor exec.
	//Try to open the given path for stat.
	int oflag = O_CLOEXEC;
	if(flag & AT_SYMLINK_NOFOLLOW)
		oflag |= O_NOFOLLOW;
	
	int fd = _openatm(at_fd, path, oflag, 0);
	if(fd < 0)
		return -1; //_openatm sets errno
	
	//Got the file open for stat. Stat it.
	int fstat_err = fstat(fd, out); //can set errno on failure
	int saved_errno = errno;
	
	//Either success or failure, just close the file and return what happened.
	close(fd);
	
	errno = saved_errno;
	return fstat_err;
}

int stat(const char *path, struct stat *out)
{
	return fstatat(AT_FDCWD, path, out, 0);
}

int lstat(const char *path, struct stat *out)
{
	return fstatat(AT_FDCWD, path, out, AT_SYMLINK_NOFOLLOW);
}

int faccessat(int fd, const char *path, int mode, int flag)
{
	int check_access = 0;
	if(mode & R_OK)
		check_access |= O_RDONLY;
	if(mode & W_OK)
		check_access |= O_WRONLY;
	if(mode & X_OK)
		check_access |= O_EXEC;
	
	int testfd = _openatm(fd, path, check_access | flag | O_CLOEXEC, 0);
	if(testfd < 0)
		return -1; //_openatm sets error
	
	close(testfd);
	return 0;
}

int access(const char *path, int mode)
{
	return faccessat(AT_FDCWD, path, mode, 0);
}

int eaccess(const char *path, int mode)
{
	return faccessat(AT_FDCWD, path, mode, AT_EACCESS);
}

int mkdirat(int fd, const char *path, mode_t mode)
{
	//Make "mode" refer to the given mode, as a directory.
	//Make sure no conflicting mode bits were set.
	mode |= S_IFDIR;
	if(!S_ISDIR(mode))
	{
		errno = EINVAL;
		return -1;
	}
	
	int made = _openatm(fd, path, O_CLOEXEC | O_CREAT | O_EXCL, mode);
	if(made == -1)
		return -1;
	
	close(made);
	return 0;
}

int mkdir(const char *path, mode_t mode)
{
	return mkdirat(AT_FDCWD, path, mode);
}

int usleep(int microsec)
{
	int at_entry = _sc_getticks();
	int target = at_entry + ( (microsec+999) / 1000 );
	while(_sc_getticks() < target) { _sc_pause(); }
	return 0;
}

unsigned sleep(unsigned sec)
{
	int at_entry = _sc_getticks();
	int target = at_entry + ( sec * 1000 );
	while(_sc_getticks() < target) { _sc_pause(); }
	return 0;	
}


static uint64_t _timeofday_setting = 0;

int gettimeofday(struct timeval *tp, void *tzp_void)
{
	struct timezone *tzp = (struct timezone *)tzp_void;
	
	int ticks = _sc_getticks();
	
	tp->tv_sec = ticks / 1000;
	tp->tv_usec = (ticks % 1000) * 1000;
	
	tp->tv_sec += _timeofday_setting;
	
	if(tzp != NULL)
	{
		tzp->tz_minuteswest = 0;
		tzp->tz_dsttime = 0;
	}
	
	return 0;
}

int settimeofday(const struct timeval *tp, const struct timezone *tzp)
{
	_timeofday_setting = tp->tv_sec - (_sc_getticks() / 1000);
	_timeofday_setting += tzp->tz_minuteswest * 60;
	return 0;
}

int clock_gettime(clockid_t clock_id, struct timespec *tp)
{
	//Don't care about the domain because we only support one clock source anyway
	(void)clock_id;
	int ticks = _sc_getticks();
	tp->tv_sec = ticks / 1000;
	tp->tv_nsec = (ticks % 1000) * 1000l * 1000l;
	return 0;
}

int clock_getres(clockid_t clock_id, struct timespec *tp)
{
	//Always 1ms ticks
	(void)clock_id;
	tp->tv_sec = 0;
	tp->tv_nsec = 1000l * 1000l;
	return 0;
}

struct _DIR_s
{
	int fd; //Underlying file descriptor
	char buf[256]; //Space 
};

DIR *fdopendir(int fd)
{
	//Reopen with read permissions, in case the caller's FD was only open for stat()
	int newfd = openat(fd, ".", O_RDONLY | O_DIRECTORY | O_CLOEXEC);
	if(newfd < 0)
		return NULL; //openat sets errno
	
	//Put in a DIR structure
	DIR *retval = malloc(sizeof(DIR));
	if(retval == NULL)
	{
		close(newfd);
		errno = ENOMEM;
		return NULL;
	}
	memset(retval, 0, sizeof(*retval));
	retval->fd = newfd;
	return retval;
}

int fdclosedir(DIR *dirp)
{
	close(dirp->fd);
	free(dirp);
	return 0;
}

DIR *opendir(const char *filename)
{
	int fd = open(filename, O_RDONLY | O_DIRECTORY | O_CLOEXEC);
	if(fd < 0)
		return NULL;
	
	DIR *retval = fdopendir(fd);
	close(fd);
	return retval;
}

struct dirent *readdir(DIR *dirp)
{
	//Ask kernel to read the directory, getting a directory entry
	struct _dirent_storage d = {0};
	int read_result = read(dirp->fd, &d, sizeof(d));
	if(read_result <= 0)
	{
		return NULL; //read sets errno
	}

	//Use the dirent buffer in the DIR structure, and clear it
	memset(dirp->buf, 0, sizeof(dirp->buf));
	struct dirent *retval = (struct dirent*)(dirp->buf);
	
	//Copy the inode number and name from the directory entry to the dirent buffer
	memcpy(dirp->buf, &d, sizeof(dirp->buf));
	
	//Stat the file to find its type, and put in the dirent buffer
	struct stat st = {0};
	if(fstatat(dirp->fd, d.d_name, &st, AT_SYMLINK_NOFOLLOW) >= 0)
	{
		retval->d_type = st.st_mode & S_IFMT;
	}
	
	return retval;
}

int closedir(DIR *dirp)
{
	fdclosedir(dirp);
	return 0;
}

mode_t umask(mode_t numask)
{
	(void)numask;
	return 0;
}

//This is a recent and nonstandard addition from the FreeBSD API. 
//It solves an important problem though ("unlink exactly this file I've been looking at") so we copy it.
int funlinkat(int dfd, const char *path, int fd, int flag)
{
	(void)dfd;
	(void)path;
	(void)fd;
	(void)flag;
	return -1;
	errno = EROFS;
	
	/*
	(void)flag;
	
	if(dfd == AT_FDCWD)
		dfd = -1;
	
	//We unlink by using the "relink" system call, which atomically removes one link and makes another to an inode.
	//We pass NULL for the "new" link, making it into an "unlink" operation.
	//This requires that we pass a directory and single filename, not an entire path though.
	//If the path has slashes in it, we need to resolve what exactly which dirent we're unlinking.
	if(strchr(path, '/'))
	{
		char path_copy[256] = {0};
		strncpy(path_copy, path, sizeof(path_copy)-1);
		if(path_copy[sizeof(path_copy)-2] != '\0')
		{
			errno = ENAMETOOLONG;
			return -1;
		}
		
		int resolved_dfd = openat(dfd, dirname(path_copy), O_RDONLY | O_EXEC);
		if(resolved_dfd < 0)
			return -1;
		
		strncpy(path_copy, path, sizeof(path_copy)-1);
		int result = _sc_relink(resolved_dfd, basename(path_copy), -1, NULL, fd);
		close(resolved_dfd);
		
		if(result < 0)
		{
			errno = -result;
			return -1;
		}
		
		return 0;
	}
	else
	{
		int result = _sc_relink(dfd, path, -1, NULL, fd);
		if(result < 0)
		{
			errno = -result;
			return -1;
		}
		return 0;
	}
	*/
}

int unlinkat(int dfd, const char *path, int flag)
{
	int fd = openat(dfd, path, O_RDWR | O_NOFOLLOW);
	if(fd < 0)
		return -1;
	
	int result = funlinkat(dfd, path, fd, flag);
	int saved_errno = errno;
	
	close(fd);
	
	errno = saved_errno;
	return result;
}

int unlink(const char *path)
{
	return unlinkat(AT_FDCWD, path, 0);
}

int execl(const char *path, const char *arg, ...)
{
	char *ptrs[32] = {0};
	
	ptrs[0] = (char*)arg;
	
	va_list ap;
	va_start(ap, arg);
	for(int aa = 1; aa < 32; aa++)
	{
		char * const argptr = va_arg(ap, char *);
		if(argptr == NULL)
			break;
		ptrs[aa] = argptr;
	}
	va_end(ap);
	
	return execv(path, ptrs);
}


int execv(const char *path, char *const argv[])
{
	//Reset pending process image
	_sc_mexec_append(NULL, 0);
	
	//Try to open the file
	int fd = open(path, O_RDONLY);
	if(fd < 0)
	{
		//Failed to open the file - open sets errno
		return -1;
	}
	
	//Read the 4kbyte header
	static uint8_t first4k[4096];
	memset(first4k, 0, sizeof(first4k));
	int first4k_read = 0;
	while(1)
	{
		int nread = read(fd, first4k + first4k_read, sizeof(first4k) - first4k_read);
		if(nread <= 0)
		{
			//Failed to read the first 4KBytes (header)
			errno = ENOEXEC;
			return -1;
		}

		first4k_read += nread;
		if(first4k_read >= 4096)
			break;
	}
	
	//Look through the header for the fields we expect
	int entry = 0;
	int argenv_start = 0;
	int argenv_end = 0;
	for(int ff = 0; ff < 4096/16; ff++)
	{
		char field_name[8] = {0};
		uint64_t field_val = 0;
		memcpy(field_name, first4k + (ff*16) + 0, 8);
		memcpy(&field_val, first4k + (ff*16) + 8, 8);
		
		if(!memcmp(field_name, "NNEARM32", 8))
			entry = field_val;
		
		if(!memcmp(field_name, "ARGENVST", 8))
			argenv_start = field_val;
		
		if(!memcmp(field_name, "ARGENVED", 8))
			argenv_end = field_val;
	}
	
	if(entry != 0x1000)
	{
		errno = ENOEXEC;
		return -1;
	}
	
	//Append the header to the pending process image (just takes up space)
	_sc_mexec_append(first4k, sizeof(first4k));
	
	//Load the rest of the file
	int nloaded = sizeof(first4k);
	uint8_t buf[256];
	while(1)
	{
		int nread = read(fd, buf, sizeof(buf));
		if(nread <= 0)
		{
			//Done loading
			break;
		}
		
		//Loaded more
		int appended = _sc_mexec_append(buf, nread);
		if(appended > 0)
		{
			nloaded += appended;
		}
	}
	
	//Stuff in the arguments/environments and BSS...
	
	//Zeroes before the arg/env buffer
	while(nloaded < argenv_start)
	{
		int to_append = argenv_start - nloaded;
		if(to_append > 1048576)
			to_append = 1048576;
		
		int appended = _sc_mexec_append(NULL, to_append); //append zeroes
		if(appended > 0)
			nloaded += appended;
		else
			break;
	}
	
	//The arg/env buffer itself
	char *const * argv_next = argv;
	char *const * envp_next = environ;
	while(nloaded < argenv_end)
	{
		const char *string_to_append = "";
		if(argv_next != NULL && *argv_next != NULL)
		{
			string_to_append = *argv_next;
			argv_next++;
		}
		else if(argv_next != NULL)
		{
			string_to_append = "";
			argv_next = NULL;
		}
		else if(envp_next != NULL && *envp_next != NULL)
		{
			string_to_append = *envp_next;
			envp_next++;
		}
		else if(envp_next != NULL)
		{
			string_to_append = "";
			envp_next = NULL;
		}
		else
		{
			break;
		}
		
		int to_append = strlen(string_to_append) + 1;
		if(nloaded + to_append > argenv_end)
			to_append = argenv_end - nloaded;
		
		int appended = _sc_mexec_append(string_to_append, to_append);
		if(appended > 0)
			nloaded += appended;
		else
			break;
	}
	
	//Zeroes after the arg/env buffer
	const int total = 24*1024*1024;
	while(nloaded < total)
	{		
		int to_append = total - nloaded;
		if(to_append > 1048576)
			to_append = 1048576;
		
		int appended = _sc_mexec_append(NULL, to_append); //append zeroes
		if(appended > 0)
			nloaded += appended;
		else
			break;
	}
	
	_sc_mexec_apply();
	return 0;
}

int isatty(int fd)
{
	//Just say that stdin/stdout/stderr are ttys and nothing else is
	if(fd == 0 || fd == 1 || fd == 2)
	{
		return 1;
	}
	
	if(fd < 0 || fd >= USER_FILE_MAX)
		errno = EBADF;

	return 0;
}

int fcntl(int fd, int cmd, ...)
{
	//Nope
	(void)fd;
	(void)cmd;
	
	errno = ENOSYS;
	return -1;
}

char *getcwd(char *buf, size_t size)
{
	//My general plan here is to pass the pwd through an environment variable
	//I haven't exactly implemented it yet
	
	if(size <= 0)
	{
		errno = EINVAL;
		return NULL;
	}
	
	const char *envpath = getenv("PWD");
	if(envpath == NULL)
		envpath = "/";
	
	if(strlen(envpath) >= size)
	{
		errno = ERANGE;
		return NULL;
	}
	
	strcpy(buf, envpath);	
	return buf;
}

int rename(const char *from, const char *to)
{
	//We only support ISO9660 filesystems and don't support modifying directories
	(void)from;
	(void)to;
	errno = ENOSYS;
	return -1;
}

int rmdir(const char *path)
{
	//We only support ISO9660 filesystems and don't support modifying directories
	(void)path;
	errno = ENOSYS;
	return -1;	
}

int utime(const char *file, const struct utimbuf *timep)
{
	//In theory we could modify a RockRidge structure in-place to change file times.
	//We don't support this at the moment.
	(void)file;
	(void)timep;
	errno = ENOSYS;
	return -1;
}

clock_t times(struct tms *tp)
{
	//The system doesn't expose this to userland
	if(tp != NULL)
	{
		tp->tms_utime = 0;
		tp->tms_stime = 0;
		tp->tms_cutime = 0;
		tp->tms_cstime = 0;
	}
	errno = ENOSYS;
	return (clock_t)(-1);
}

int chdir(const char *path)
{
	//Not implemented yet...
	//We could keep our own pwd file descriptor and reflect it in the environment as well, to preserve it
	(void)path;
	errno = ENOSYS;
	return -1;
}
