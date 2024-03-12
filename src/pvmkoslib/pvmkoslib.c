//pvmkoslib.c
//OS support library for picolibc when running under PVMK
//Bryan E. Topp <betopp@betopp.com> 2024

//PVMK system call definitions
#include <sc.h>

#include <stdarg.h>
#include <stddef.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

//For picolibc...
#undef errno
extern __thread int errno;

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

int open(const char *fn, int flag, ...)
{
	//Translate "flag" from picolibc definition to PVMK system call definition
	int sysflag = 0;
	if(flag & O_CLOEXEC ) sysflag |= _SC_OPEN_CLOEXEC;
	if(flag & O_EXCL    ) sysflag |= _SC_OPEN_EXCL;
	if(flag & O_CREAT   ) sysflag |= _SC_OPEN_CREAT;
	if(flag & O_RDONLY  ) sysflag |= _SC_OPEN_R;
	if(flag & O_WRONLY  ) sysflag |= _SC_OPEN_W;
	if(flag & O_EXEC    ) sysflag |= _SC_OPEN_X;
	if(flag & O_NOFOLLOW) sysflag |= _SC_OPEN_NOFOLLOW;
	
	int mode = 0;
	if(flag & O_CREAT)
	{
		va_list ap;
		va_start(ap, flag);
		mode = va_arg(ap, int);
		va_end(ap);
	}
	
	int result = _sc_open(-1, fn, sysflag, mode, 0);
	if(result < 0)
	{
		errno = _pvmk_sysret_errno(result);
		return -1;
	}
	
	return result;
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
		ssize_t result = _sc_write(fd, bufb, nbyte);
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

void *sbrk(ptrdiff_t nbytes)
{
	if(nbytes < 0)
	{
		errno = EINVAL;
		return (void*)(-1);
	}
	
	if(nbytes >= 1024*1024*1024)
	{
		errno = ENOMEM;
		return (void*)(-1);
	}
	
	int result = _sc_mem_sbrk(nbytes);
	if(result < 0)
	{
		errno = _pvmk_sysret_errno(result);
		return (void*)-1;
	}
	
	return (void*)result;
}

