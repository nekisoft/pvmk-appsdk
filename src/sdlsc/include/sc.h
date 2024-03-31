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
#define _SC_BTNIDX_L       0
#define _SC_BTNIDX_UP      1
#define _SC_BTNIDX_R       2
#define _SC_BTNIDX_LEFT    3
#define _SC_BTNIDX_DOWN    4
#define _SC_BTNIDX_RIGHT   5
#define _SC_BTNIDX_X       6
#define _SC_BTNIDX_Y       7
#define _SC_BTNIDX_Z       8
#define _SC_BTNIDX_A       9
#define _SC_BTNIDX_B      10
#define _SC_BTNIDX_C      11
#define _SC_BTNIDX_START  12
#define _SC_BTNBIT_L       (1u << _SC_BTNIDX_L)
#define _SC_BTNBIT_UP      (1u << _SC_BTNIDX_UP)
#define _SC_BTNBIT_R       (1u << _SC_BTNIDX_R)
#define _SC_BTNBIT_LEFT    (1u << _SC_BTNIDX_LEFT)
#define _SC_BTNBIT_DOWN    (1u << _SC_BTNIDX_DOWN)
#define _SC_BTNBIT_RIGHT   (1u << _SC_BTNIDX_RIGHT)
#define _SC_BTNBIT_X       (1u << _SC_BTNIDX_X)
#define _SC_BTNBIT_Y       (1u << _SC_BTNIDX_Y)
#define _SC_BTNBIT_Z       (1u << _SC_BTNIDX_Z)
#define _SC_BTNBIT_A       (1u << _SC_BTNIDX_A)
#define _SC_BTNBIT_B       (1u << _SC_BTNIDX_B)
#define _SC_BTNBIT_C       (1u << _SC_BTNIDX_C)
#define _SC_BTNBIT_START   (1u << _SC_BTNIDX_START)
typedef struct _sc_input_s { char format; char flags; short buttons; } _sc_input_t __attribute__((aligned(4)));
int _sc_input(_sc_input_t *buffer_ptr, int bytes_per_event, int bytes_max);

//Waiting
void _sc_pause(void);
int _sc_getticks(void);

//Error codes used in the shim
#define _SC_EINVAL       22 //Invalid argument.
#define _SC_EAGAIN       11 //Resource unavailable, try again (may be the same value as EWOULDBLOCK).
#define _SC_ENOMEM       12 //Not enough space.

#if 0
#define _SC_EPERM         1 //Operation not permitted.
#define _SC_ENOENT        2 //No such file or directory.
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
#define _SC_ENOSPC       28 //No space left on device.
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
#define _SC_ENOSYS       38 //Functionality not supported.
#define _SC_ENOTEMPTY    39 //Directory not empty.
#define _SC_ELOOP        40 //Too many levels of symbolic links.
#define _SC_ENOMSG       42 //No message of the desired type.
#define _SC_EIDRM        43 //Identifier removed.
#endif

#endif //_SDLSC_H
