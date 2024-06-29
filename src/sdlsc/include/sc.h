//sc.h (sdlsc)
//Library to replace PVMK system calls with SDL usage
//Bryan E. Topp <betopp@betopp.com> 2024
#ifndef _SDLSC_H
#define _SDLSC_H

//sdlsc/include/sc.h
//This library can be used in place of the real sc.h to call SDL for graphics/sound/input.
//This way, your port of an application to PVMK can be tested on a desktop.
//See the real sc.h for details on how each of these system-calls works.
//Unlike the real sc.h, this one requires you to link some actual object files.
// -Bryan E. Topp <betopp@betopp.com> 2024

#ifdef _SC_H
#error "You can't use the real sc.h and this shim sc.h in the same build!"
#endif //_SC_H
#if !USE_SDLSC
#error "You should define USE_SDLSC=1 if your build will use the shim sc.h."
#endif

//Replacement for graphics system-calls
#define _SC_GFX_MODE_TEXT          0
#define _SC_GFX_MODE_VGA_16BPP     1
#define _SC_GFX_MODE_320X240_16BPP 2
#define _SC_GFX_MODE_MAX           3
int _sc_gfx_flip(int mode, const void *buffer);

//Replacement for audio system-calls
#define _SC_SND_MODE_SILENT      0
#define _SC_SND_MODE_48K_16B_2C  1
#define _SC_SND_MODE_MAX         2
int _sc_snd_play(int mode, const void *chunk, int chunkbytes, int maxbuf);

//Replacement for input system-calls
#define _SC_BTNIDX_UP      0
#define _SC_BTNIDX_LEFT    1
#define _SC_BTNIDX_DOWN    2
#define _SC_BTNIDX_RIGHT   3
#define _SC_BTNIDX_A       4
#define _SC_BTNIDX_B       5
#define _SC_BTNIDX_C       6
#define _SC_BTNIDX_X       7
#define _SC_BTNIDX_Y       8
#define _SC_BTNIDX_Z       9
#define _SC_BTNIDX_START  10
#define _SC_BTNIDX_MODE   11
#define _SC_BTNBIT_UP      (1u << _SC_BTNIDX_UP)
#define _SC_BTNBIT_LEFT    (1u << _SC_BTNIDX_LEFT)
#define _SC_BTNBIT_DOWN    (1u << _SC_BTNIDX_DOWN)
#define _SC_BTNBIT_RIGHT   (1u << _SC_BTNIDX_RIGHT)
#define _SC_BTNBIT_A       (1u << _SC_BTNIDX_A)
#define _SC_BTNBIT_B       (1u << _SC_BTNIDX_B)
#define _SC_BTNBIT_C       (1u << _SC_BTNIDX_C)
#define _SC_BTNBIT_X       (1u << _SC_BTNIDX_X)
#define _SC_BTNBIT_Y       (1u << _SC_BTNIDX_Y)
#define _SC_BTNBIT_Z       (1u << _SC_BTNIDX_Z)
#define _SC_BTNBIT_START   (1u << _SC_BTNIDX_START)
#define _SC_BTNBIT_MODE    (1u << _SC_BTNIDX_MODE)
typedef struct _sc_input_s { char format; char flags; short buttons; } _sc_input_t __attribute__((aligned(4)));
int _sc_input(_sc_input_t *buffer_ptr, int bytes_per_event, int bytes_max);

//Waiting
void _sc_pause(void);
int _sc_getticks(void);

//NVM
int _sc_nvm_save(const void *buf, int len);
int _sc_nvm_load(void *buf, int len);
int _sc_nvm_ident(const char *name); //ENOSYS
int _sc_nvm_delete(void); //ENOSYS
int _sc_nvm_enum(int idx, char *name_buf, int name_max); //ENOSYS

//Poweroff
int _sc_halt(void);

//Other stuff (just to get our system-menu compiling...)
int _sc_mount(int container_fd, int disk_number); //ENOSYS
int _sc_deny_syscall(int callnum); //ENOSYS
int _sc_deny_filefmt(int mode); //ENOSYS


//Error codes used in the shim
#define _SC_EINVAL       22 //Invalid argument.
#define _SC_EAGAIN       11 //Resource unavailable, try again (may be the same value as EWOULDBLOCK).
#define _SC_ENOMEM       12 //Not enough space.
#define _SC_ENOENT        2 //No such file or directory.
#define _SC_ENOSPC       28 //No space left on device.
#define _SC_ENOSYS       38 //Functionality not supported.

#if 0
#define _SC_EPERM         1 //Operation not permitted.

#define _SC_ESRCH         3 //No such process.
#define _SC_EINTR         4 //Interrupted function.
#define _SC_EIO           5 //I/O error.
#define _SC_ENXIO         6 //No such device or address.
#define _SC_E2BIG         7 //Argument list too long.
#define _SC_ENOEXEC       8 //Executable file format error.
#define _SC_EBADF         9 //Bad file descriptor.
#define _SC_ECHILD       10 //No child processes.

#define _SC_EWOULDBLOCK  11 //Operation would block (may be the same value as EAGAIN).

#define _SC_EACCES       13 //Permission denied.
#define _SC_EFAULT       14 //Bad address.
#define _SC_EBUSY        16 //Device or resource busy.
#define _SC_EEXIST       17 //File exists.
#define _SC_EXDEV        18 //Cross-device link.
#define _SC_ENODEV       19 //No such device.
#define _SC_ENOTDIR      20 //Not a directory or a symbolic link to a directory.
#define _SC_EISDIR       21 //Is a directory.

#define _SC_ENFILE       23 //Too many files open in system.
#define _SC_EMFILE       24 //File descriptor value too large.
#define _SC_ENOTTY       25 //Inappropriate I/O control operation.
#define _SC_ETXTBSY      26 //Text file busy.
#define _SC_EFBIG        27 //File too large.

#define _SC_ESPIPE       29 //Invalid seek.
#define _SC_EROFS        30 //Read-only file system.
#define _SC_EMLINK       31 //Too many links.
#define _SC_EPIPE        32 //Broken pipe.
#define _SC_EDOM         33 //Mathematics argument out of domain of function.
#define _SC_ERANGE       34 //Result too large.
#define _SC_EDEADLK      35 //Resource deadlock would occur.
#define _SC_EDEADLOCK    35 //Resource deadlock would occur.
#define _SC_ENAMETOOLONG 36 //Filename too long.
#define _SC_ENOLCK       37 //No locks available.

#define _SC_ENOTEMPTY    39 //Directory not empty.
#define _SC_ELOOP        40 //Too many levels of symbolic links.
#define _SC_ENOMSG       42 //No message of the desired type.
#define _SC_EIDRM        43 //Identifier removed.
#endif

//File modes (probably not a wise choice to look at anything but the lowest 9 bits... no guarantee the host system agrees)
#define _SC_S_IFMT   ((int)0770000) //Format mask - bits that indicate the type of file
#define _SC_S_IFREG  ((int)0010000) //Format - regular file
#define _SC_S_IFDIR  ((int)0020000) //Format - directory
#define _SC_S_IFBLK  ((int)0030000) //Format - block device (unused)
#define _SC_S_IFCHR  ((int)0040000) //Format - character device
#define _SC_S_IFLNK  ((int)0050000) //Format - symbolic link
#define _SC_S_IFSOCK ((int)0060000) //Format - socket
#define _SC_S_IFIFO  ((int)0070000) //Format - FIFO (named pipe)
#define _SC_S_IFPTY  ((int)0100000) //Format - pseudoterminal (unique to us!)
#define _SC_S_ISUID  ((int)0004000) //Set-UID bit (unused)
#define _SC_S_ISGID  ((int)0002000) //Set-GID bit (unused)
#define _SC_S_ISVTX  ((int)0001000) //Sticky bit (unused)
#define _SC_S_IRWXU  ((int)0000700) //Permission mask - owner
#define _SC_S_IRUSR  ((int)0000400) //Readable by owner
#define _SC_S_IWUSR  ((int)0000200) //Writable by owner
#define _SC_S_IXUSR  ((int)0000100) //Executable by owner
#define _SC_S_IRWXG  ((int)0000070) //Permission mask - group (unused)
#define _SC_S_IRGRP  ((int)0000040) //Readable by owning group (unused)
#define _SC_S_IWGRP  ((int)0000020) //Writable by owning group (unused)
#define _SC_S_IXGRP  ((int)0000010) //Executable by owning group (unused)
#define _SC_S_IRWXO  ((int)0000007) //Permission mask - others (unused)
#define _SC_S_IROTH  ((int)0000004) //Readable by others (unused)
#define _SC_S_IWOTH  ((int)0000002) //Writable by others (unused)
#define _SC_S_IXOTH  ((int)0000001) //Executable by others (unused)



#endif //_SDLSC_H
