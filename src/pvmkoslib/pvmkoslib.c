//pvmkoslib.c
//OS support library for picolibc when running under PVMK
//Bryan E. Topp <betopp@betopp.com> 2024

#define _POSIX_C_SOURCE 200809L

//PVMK system call definitions
#include <sc.h>

#include <stdarg.h>
#include <sys/time.h>
#include <sys/dirent.h>
#include <stddef.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

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
		case -_SC_EINTR:        return EINTR;
		case -_SC_EIO:          return EIO;
		case -_SC_ENXIO:        return ENXIO;
		case -_SC_E2BIG:        return E2BIG;
		case -_SC_ENOEXEC:      return ENOEXEC;
		case -_SC_EBADF:        return EBADF;
		case -_SC_ECHILD:       return ECHILD;
		case -_SC_EAGAIN:       return EAGAIN;
		case -_SC_ENOMEM:       return ENOMEM;
		case -_SC_EACCES:       return EACCES;
		case -_SC_EFAULT:       return EFAULT;
		case -_SC_EBUSY:        return EBUSY;
		case -_SC_EEXIST:       return EEXIST;
		case -_SC_EXDEV:        return EXDEV;
		case -_SC_ENODEV:       return ENODEV;
		case -_SC_ENOTDIR:      return ENOTDIR;
		case -_SC_EISDIR:       return EISDIR;
		case -_SC_EINVAL:       return EINVAL;
		case -_SC_ENFILE:       return ENFILE;
		case -_SC_EMFILE:       return EMFILE;
		case -_SC_ENOTTY:       return ENOTTY;
		case -_SC_ETXTBSY:      return EBUSY; // ?
		case -_SC_EFBIG:        return EFBIG;
		case -_SC_ENOSPC:       return ENOSPC;
		case -_SC_ESPIPE:       return ESPIPE;
		case -_SC_EROFS:        return EROFS;
		case -_SC_EMLINK:       return EMLINK;
		case -_SC_EPIPE:        return EPIPE;
		case -_SC_EDOM:         return EDOM;
		case -_SC_ERANGE:       return ERANGE;
		case -_SC_EDEADLK:      return EDEADLK;
		case -_SC_ENAMETOOLONG: return ENAMETOOLONG;
		case -_SC_ENOLCK:       return ENOLCK;
		case -_SC_ENOSYS:       return ENOSYS;
		case -_SC_ENOTEMPTY:    return ENOTEMPTY;
		case -_SC_ELOOP:        return ELOOP;
		case -_SC_ENOMSG:       return ENOMSG;
		case -_SC_EIDRM:        return EIDRM;
		default:                return ENOSYS;
	}
}

//Non-variadic version of openat.
static int _openatm(int fd, const char *path, int flags, mode_t mode)
{
	if(fd == AT_FDCWD)
		fd = -1;
	
	int fd_ret = -ENOSYS;
	
	//If they didn't specify a type of file, default to a "regular file" mode.
	if((mode & S_IFMT) == 0)
		mode |= S_IFREG;
	
	
	//Translate "flag" from picolibc definition to PVMK system call definition
	int sysflags = 0;
	if(flags & O_CLOEXEC ) sysflags |= _SC_OPEN_CLOEXEC;
	if(flags & O_EXCL    ) sysflags |= _SC_OPEN_EXCL;
	if(flags & O_CREAT   ) sysflags |= _SC_OPEN_CREAT;
	if(flags & O_EXEC    ) sysflags |= _SC_OPEN_X;
	if(flags & O_NOFOLLOW) sysflags |= _SC_OPEN_NOFOLLOW;
	
	if( (flags & O_ACCMODE) == O_RDONLY) sysflags |= _SC_OPEN_R;
	if( (flags & O_ACCMODE) == O_WRONLY) sysflags |= _SC_OPEN_W;
	if( (flags & O_ACCMODE) == O_RDWR  ) sysflags |= _SC_OPEN_R | _SC_OPEN_W;
	
	//Translate "mode" from picolibc definition to PVMK system call definition
	int sysmode = mode & 0777;
	switch(mode & S_IFMT)
	{
		case S_IFREG: sysmode |= _SC_S_IFREG; break;
		case S_IFDIR: sysmode |= _SC_S_IFDIR; break;
		case S_IFLNK: sysmode |= _SC_S_IFLNK; break;
		default: errno = EINVAL; return -1;
	}
	
	fd_ret = _sc_open(fd, path, sysflags, sysmode, 0);
	if(fd_ret < 0)
	{
		errno = _pvmk_sysret_errno(fd_ret);
		return -1;
	}
	
	//Alright, we've got the file open.
	
	//Set our user-level nonblocking flag for it.
	//if(fd_ret >= 0 && fd_ret < FD_NONBLOCKING_MAX)
	//	_fd_nonblocking[fd_ret] = (flags & O_NONBLOCK) ? 1 : 0;
	
	//If we only wanted to open a directory, make sure it is.
	if(flags & O_DIRECTORY)
	{
		_sc_stat_t st = {0};
		int st_err = _sc_stat(fd_ret, &st, sizeof(st));
		if(st_err < 0)
		{
			//Failed to stat the file we just opened...?
			errno = _pvmk_sysret_errno(st_err);
			_sc_close(fd_ret);
			fd_ret = -1;
		}
		
		if(!S_ISDIR(st.mode))
		{
			//Wanted to only open a directory, but this isn't one.
			errno = ENOTDIR;
			_sc_close(fd_ret);
			fd_ret = -1;
		}
	}
	
	//If we wanted to append, go to the end of the file
	if(flags & O_APPEND)
	{
		_sc_seek(fd_ret, 0, SEEK_END);
	}
			
	return fd_ret;
}


//argenv buffer - aligned enough to refer to a char*, but ultimately storing strings
static char *argenv_buffer[4096 / sizeof(void*)];

//Called from crt0 to call main
void _pvmk_callmain(void)
{
	//Tell kernel about what system-call interface we expect
	//Todo - handle older versions if we lack some system-calls or something
	int32_t our_version = 1;
	int32_t version_to_use = _sc_version(our_version);
	(void)version_to_use;
	
	//Default arguments/environments if something goes wrong
	int argc = 1;
	char **argv = (char*[]){ "argv0",         NULL };
	char **envp = (char*[]){ "PVMK=external", NULL };
	
	//Try to load arguments/environment
	int nread = _sc_env_load(argenv_buffer, sizeof(argenv_buffer));
	if(nread > 0)
	{
		//Ensure we're NUL-terminated
		((char*)argenv_buffer)[sizeof(argenv_buffer)-1] = 0;
		
		//Run through the strings we read. Count them and make sure there's room to build pointer arrays.
		//Each of the two pointer arrays ends, when a zero-length string is encountered.
		int nstrings = 0;
		int zerolen = 0;
		char *remaining = (char*)argenv_buffer;
		char *stop = ((char*)argenv_buffer) + sizeof(argenv_buffer);
		while(zerolen < 2 && remaining < stop)
		{
			if(strlen(remaining) == 0)
				zerolen++;

			nstrings++;
			remaining += strlen(remaining) + 1;
		}
		
		//See if we have enough room...
		int ptr_idx = (nread + (sizeof(void*)) - 1) / sizeof(void*); //Array index
		int ptr_capacity = sizeof(argenv_buffer) / sizeof(argenv_buffer[0]);
		if(ptr_idx + nstrings < ptr_capacity)
		{
			//Alright, have room for the pointers
			remaining = (char*)argenv_buffer;
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
	int result = _sc_close(fd);
	if(result < 0)
	{
		errno = _pvmk_sysret_errno(result);
		return -1;
	}
	return 0;
}

ssize_t read(int fd, void *buf, size_t nbyte)
{
	//Pain in the ass here.
	//Everybody agrees that read() can return less data than requested, for a number of reasons.
	//Doom decides to just barf if that happens, though. I defer to Doom.
	size_t nread = 0;
	char *bufb = (char*)buf;
	while(nbyte > 0)
	{
		ssize_t result = _sc_read(fd, bufb, nbyte);
		if(result == -_SC_EAGAIN)
		{
			_sc_pause();
			continue;
		}
		
		if( (result < 0) && (nread == 0) )
		{
			errno = _pvmk_sysret_errno(result);
			return -1;
		}
		
		if(result <= 0)
		{
			return nread;
		}
		
		nbyte -= result;
		nread += result;
		bufb += result;
		
	}
	return nread;
}

ssize_t write(int fd, const void *buf, size_t nbyte)
{
	size_t nwritten = 0;
	const char *bufb = (const char*)buf;
	while(nbyte > 0)
	{
		size_t to_write = nbyte;
		if(to_write > 65536)
			to_write = 65536;
		
		ssize_t result = _sc_write(fd, bufb, to_write);
		if(result == -_SC_EAGAIN)
		{
			_sc_pause();
			continue;
		}
		
		if( (result < 0) && (nwritten == 0) )
		{
			errno = _pvmk_sysret_errno(result);
			return -1;
		}
		
		if(result <= 0)
		{
			return nwritten;
		}
		
		nbyte -= result;
		nwritten += result;
		bufb += result;
	}
	return nwritten;
}

off_t lseek(int fd, off_t offset, int whence)
{
	//Translate "whence" from picolibc definition to PVMK system call definition
	int syswhence = 0;
	switch(whence)
	{
		case SEEK_SET: syswhence = _SC_SEEK_SET; break;
		case SEEK_CUR: syswhence = _SC_SEEK_CUR; break;
		case SEEK_END: syswhence = _SC_SEEK_END; break;
		default:       errno = EINVAL; return -1;
	}
	
	int result = _sc_seek(fd, offset, syswhence);
	if(result < 0)
	{
		errno = _pvmk_sysret_errno(result);
		return -1;
	}
	else
	{
		return result;
	}
}

void _exit(int status)
{
	_sc_exit(status, 0);
}

//Annoyingly, the picolibc malloc isn't configurable in the following ways:
//1. Can't handle an sbrk that only goes up
//2. Asks for dumb shit like 40 bytes at a time
//So we'll manage a real and a phony sbrk, allocate big chunks, and drip them out to callers.
static size_t _sbrk_top_real;
static size_t _sbrk_top_fake;
void *sbrk(ptrdiff_t nbytes)
{
	while(1)
	{
		if(_sbrk_top_real == 0 && _sbrk_top_fake == 0)
		{
			//First call, no reserved memory available for the caller.
			//Grab a big chunk. This gives us the old size-of-process, and some memory to work with.
			int initial_request = 0x1337;
			int result = _sc_mem_sbrk(initial_request);
			if(result < 0)
			{
				//System doesn't have room for what we're asking for
				errno = _pvmk_sysret_errno(result);
				return (void*)(-1);
			}
			
			//We now know the "real" top-of-process, which is our new size.
			//The "fake" top-of-process - visible to callers - is how it was before this call.
			_sbrk_top_real = result + initial_request;
			_sbrk_top_fake = result;
		}
		
		//Return value will always be the old "fake" top of process
		void *retval = (void*)_sbrk_top_fake;
		
		//Always allow giving back memory
		if(nbytes < 0)
		{
			_sbrk_top_fake += nbytes; //nbytes is negative
			return retval;
		}
		
		//Can we service the request already?
		size_t wanted_top = _sbrk_top_fake + nbytes;
		if(wanted_top <= _sbrk_top_real)
		{
			//Can provide this, without another real hit to _sc_mem_sbrk.
			_sbrk_top_fake = wanted_top;
			return retval;
		}
		
		//Need to actually get more memory.
		int request_more = nbytes;
		if(request_more < 0xbff00)
			request_more = 0xbff00; //Kinda arbitrary but big-ish
		
		int real_result = _sc_mem_sbrk(request_more);
		if(real_result < 0)
		{
			//Needed more memory but the system couldn't provide it.
			errno = _pvmk_sysret_errno(real_result);
			return (void*)(-1);
		}
		
		//The old real top-of-process, where more memory was added, should be what we expect.
		if((size_t)real_result != _sbrk_top_real)
		{
			//Someone else has adjusted the real size of the process, underneath us.
			//Have to give up and reset.
			_sbrk_top_real = 0;
			_sbrk_top_fake = 0;
		}
		else
		{
			//We now know the real top-of-process has been expanded by what we asked for.
			//We can try again to raise the fake top-of-process.
			_sbrk_top_real += request_more;
		}
	}
}

int fstat(int fd, struct stat *out)
{
	//Performing a stat on an open file is the way our kernel works natively.
	_sc_stat_t _sc_stat_buf = {0};
	int stat_err = _sc_stat(fd, &_sc_stat_buf, sizeof(_sc_stat_buf));
	if(stat_err < 0)
	{
		errno = -stat_err;
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

	//Either success or failure, just close the file and return what happened.
	_sc_close(fd);
	return fstat_err;
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
	
	tzp->tz_minuteswest = 0;
	tzp->tz_dsttime = 0;
	
	return 0;
}

int settimeofday(const struct timeval *tp, const struct timezone *tzp)
{
	_timeofday_setting = tp->tv_sec - (_sc_getticks() / 1000);
	_timeofday_setting += tzp->tz_minuteswest * 60;
	return 0;
}

struct _DIR_s
{
	int fd; //Underlying file descriptor
	char buf[512]; //Space 
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
	_sc_dirent_t d = {0};
	int read_result = _sc_read(dirp->fd, &d, sizeof(d));
	if(read_result < 0)
	{
		errno = -read_result;
		return NULL;
	}
	if(read_result == 0)
	{
		return NULL;
	}

	//Use the dirent buffer in the DIR structure, and clear it
	memset(dirp->buf, 0, sizeof(dirp->buf));
	struct dirent *retval = (struct dirent*)(dirp->buf);
	
	//Copy the inode number and name from the directory entry to the dirent buffer
	retval->d_ino = d.ino;
	strncpy(dirp->buf + sizeof(struct dirent), d.name, sizeof(dirp->buf) - sizeof(struct dirent) - 1);
	
	//Stat the file to find its type, and put in the dirent buffer
	struct stat st = {0};
	if(fstatat(dirp->fd, d.name, &st, AT_SYMLINK_NOFOLLOW) >= 0)
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

