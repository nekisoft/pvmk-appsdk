//sc.h
//One-file system call library
//Bryan E. Topp <betopp@betopp.com> 2023
#ifndef _SC_H
#define _SC_H

//sc.h
//This file defines the system-call interface on PVMK, the kernel of Neki32.
//When building a C program for PVMK, this file is used to make system-calls from C.
//Additionally, the picolibc implementation uses these calls for its OS interface.
//As the definitions are all in this header, you do not need to link anything else.
//Just include this header file and make sure the definition of "_sc" works.
// -Bryan E. Topp <betopp@betopp.com> 2024

//===
//1. Definition of system-call mechanism.
//===

//Qualifiers used in system-call declarations
//As this is a header-only library, just declare+define everything up front
#define SYSCALL_DECL static inline 

//Implementation of system-call mechanism on each platform
#define _SYSTYPE __INTPTR_TYPE__
#define _SC(n,a,b,c,d,e) _sc((_SYSTYPE)n, (_SYSTYPE)a, (_SYSTYPE)b, (_SYSTYPE)c, (_SYSTYPE)d, (_SYSTYPE)e)
SYSCALL_DECL __INTPTR_TYPE__ _sc(_SYSTYPE num, _SYSTYPE p1, _SYSTYPE p2, _SYSTYPE p3, _SYSTYPE p4, _SYSTYPE p5)
{
	#if defined(SC_H_NO_IMPL)
		(void)num; (void)p1; (void)p2; (void)p3; (void)p4; (void)p5;
		return -38; //-_SC_ENOSYS
	#elif defined(__i386__)
		//386 calling convention:
		//EAX contains call number, parameters in EBX, ECX, EDX, ESI, EDI.
		//A port of PVMK to qemu-system-i386 is available. This is not used in the real Neki32.
		register _SYSTYPE eax __asm("eax") = num;
		register _SYSTYPE ebx __asm("ebx") = p1;
		register _SYSTYPE ecx __asm("ecx") = p2;
		register _SYSTYPE edx __asm("edx") = p3;
		register _SYSTYPE esi __asm("esi") = p4;
		register _SYSTYPE edi __asm("edi") = p5;
		__asm volatile ("int $0x20" : "+a"(eax),"+b"(ebx),"+c"(ecx),"+d"(edx),"+S"(esi),"+D"(edi) : : "memory");
		return eax;
	#elif defined(__arm__)
		//ARM (AArch32) calling convention:
		//r0 contains call number, parameters in r1, r2, r3, r4, r5.
		//This is used on the actual Neki32.
		register _SYSTYPE r0 __asm("r0") = num;
		register _SYSTYPE r1 __asm("r1") = p1;
		register _SYSTYPE r2 __asm("r2") = p2;
		register _SYSTYPE r3 __asm("r3") = p3;
		register _SYSTYPE r4 __asm("r4") = p4;
		register _SYSTYPE r5 __asm("r5") = p5;
		__asm volatile ("udf 0x92" : "+r"(r0),"+r"(r1),"+r"(r2),"+r"(r3),"+r"(r4),"+r"(r5) : : "memory");
		return r0;
	#else
		#error unknown architecture, no syscall mechanism defined
	#endif
}

//Macro to prevent a no-return system-call from returning
#define _DONTRETURN do{ ((volatile int*)0)[0]=0; }while(1)
#define _NORETURN __attribute__((noreturn))

//===
//2. System calls usable by games.
//===

// _sc_none //
//Does nothing.
SYSCALL_DECL void _sc_none(void)
	#define  _SC_NONE_N 0x00
	{ _SC(_SC_NONE_N, 0, 0, 0, 0, 0); }

// _sc_pause //
//Waits until anything happens to the calling process, or has happened since the last call returned.
//This is the only way to actually "block" your process at the kernel level.
//When we say "anything happens", it means "anything relating to a system-call you previously made".
//So, idiomatically, you would write: while(1) { try_my_thing(); if(it_finished()) { break; } else { _sc_pause(); } }
SYSCALL_DECL void _sc_pause(void)
	#define  _SC_PAUSE_N 0x01
	{ _SC(_SC_PAUSE_N, 0, 0, 0, 0, 0); }

// _sc_getticks //
//Returns the number of milliseconds since the system was booted.
//Does not fail.
SYSCALL_DECL int _sc_getticks(void)
	#define _SC_GETTICKS_N 0x02
	{ return _SC(_SC_GETTICKS_N, 0, 0, 0, 0, 0); }

// _sc_exit //
//Generally obliterates the calling process.
//Optionally reports a signal that was responsible for its demise.
SYSCALL_DECL _NORETURN void _sc_exit(int exitcode, int signal)
	#define  _SC_EXIT_N 0x07
	{ _SC(_SC_EXIT_N, exitcode, signal, 0, 0, 0); _DONTRETURN; }

// _sc_env_save //
//Appends the given data to the kernel's argument/environment buffer for the calling process.
//Subsequent calls append to the buffer; call with buf=len=0 to reset the buffer.
//This buffer is preserved across calls to exec() and mexec_apply().
//Conventionally it should contain a series of NUL-terminated argument strings,
//then an extra NUL, then a series of NUL-terminated environment strings.
//Returns the number of bytes written or a negative error number.
SYSCALL_DECL int _sc_env_save(const void *buf, int len)
	#define _SC_ENV_SAVE_N 0x08
	{ return _SC(_SC_ENV_SAVE_N, buf, len, 0, 0, 0); }

// _sc_env_load //
//Reads from the kernel's argument/environment buffer for the calling process.
//Writes the result into the calling process's user memory, usually after an exec() or mexec_apply().
//Unlike _sc_env_save, starts from the beginning each time it's called.
//Returns the number of bytes copied or a negative error number.
SYSCALL_DECL int _sc_env_load(void *buf, int len)
	#define _SC_ENV_LOAD_N 0x09
	{ return _SC(_SC_ENV_LOAD_N, buf, len, 0, 0, 0); }

// _sc_print //
//Prints output to the text-mode screen.
//Prints the sequence of bytes at buf_ptr until a terminating NUL.
//Currently supports very few control sequences.
//Basically only used for debugging.
//Returns the number of bytes printed.
SYSCALL_DECL int _sc_print(const char *buf_ptr)
	#define _SC_PRINT_N 0xB0
	{ return _SC(_SC_PRINT_N, buf_ptr, 0, 0, 0, 0); }
	
//Signal numbers defined in the kernel
#define _SC_SIGZERO 0
#define _SC_SIGHUP 1
#define _SC_SIGINT 2
#define _SC_SIGQUIT 3
#define _SC_SIGILL 4
#define _SC_SIGTRAP 5
#define _SC_SIGABRT 6
#define _SC_SIGEMT 7
#define _SC_SIGFPE 8
#define _SC_SIGKILL 9
#define _SC_SIGBUS 10
#define _SC_SIGSEGV 11
#define _SC_SIGSYS 12
#define _SC_SIGPIPE 13
#define _SC_SIGALRM 14
#define _SC_SIGTERM 15
#define _SC_SIGURG 16
#define _SC_SIGSTOP 17
#define _SC_SIGTSTP 18
#define _SC_SIGCONT 19
#define _SC_SIGCHLD 20
#define _SC_SIGTTIN 21
#define _SC_SIGTTOU 22
#define _SC_SIGXCPU 23
#define _SC_SIGXFSZ 24
#define _SC_SIGVTALRM 25
#define _SC_SIGPROF 26
#define _SC_SIGWINCH 27
#define _SC_SIGINFO 28
#define _SC_SIGUSR1 29
#define _SC_SIGUSR2 30

//Ways to alter the signal mask of a thread.
#define _SC_SIGMASK_BLOCK 0
#define _SC_SIGMASK_UNBLOCK 1
#define _SC_SIGMASK_SETMASK 2

// _sc_sig_mask //
//Alters the signal mask of the calling thread.
//Returns the OLD mask.
SYSCALL_DECL int _sc_sig_mask(int how, int bits)
	#define _SC_SIG_MASK_N 0x20
	{ return _SC(_SC_SIG_MASK_N, how, bits, 0, 0, 0); }

// _sc_sig_return //
//Returns from a signal handler.
SYSCALL_DECL _NORETURN void _sc_sig_return(void)
	#define  _SC_SIG_RETURN_N 0x22
	{ _SC(_SC_SIG_RETURN_N, 0, 0, 0, 0, 0); _DONTRETURN; }
	
//Graphics modes that are used with _sc_gfx_flip.
#define _SC_GFX_MODE_TEXT          0 //No framebuffer supplied; kernel text mode only
#define _SC_GFX_MODE_VGA_16BPP     1 //640x480@60Hz RGB565, 1280 bytes per line
#define _SC_GFX_MODE_320X240_16BPP 2 //320x240@60Hz RGB565, 640 bytes per line
#define _SC_GFX_MODE_MAX           3 //Number of modes supported; mode parameter must be less than this

// _sc_gfx_flip //
//Enqueues a change of the system front-buffer to the given buffer.
//Only takes effect during vertical blanking.
//If mode is 0 (text), buffer must be NULL.
//If mode is nonzero, buffer must be given and large enough for one framebuffer.
//Returns the address of the image currently displayed.
//Can return 0 if the currently-displayed image belongs to another process.
//Returns a negative error number on failure (if nothing was enqueued).
SYSCALL_DECL int _sc_gfx_flip(int mode, const void *buffer)
	#define _SC_GFX_FLIP_N 0x30
	{ return _SC(_SC_GFX_FLIP_N, mode, buffer, 0, 0, 0); }

//Audio modes that are used with _sc_snd_play.
#define _SC_SND_MODE_SILENT      0 //Stops all sounds
#define _SC_SND_MODE_48K_16B_2C  1 //Linear PCM, 48KHz, 16-bit stereo, native byte order, left-then-right
#define _SC_SND_MODE_MAX         2 //Number of modes supported; mode parameter must be less than this

// _sc_snd_play //
//Enqueues audio samples for output.
//Returns 0 on success, or a negative error number.
//Returns -EAGAIN if there wasn't enough space for the chunk - audio buffer is already full.
//The "maxbuf" parameter limits how many bytes will be buffered by the system before -EAGAIN is returned.
//Does not consume partial chunks - the chunk is either enqueued entirely or rejected.
SYSCALL_DECL int _sc_snd_play(int mode, const void *chunk, int chunkbytes, int maxbuf)
	#define _SC_SND_PLAY_N 0x60
	{ return _SC(_SC_SND_PLAY_N, mode, chunk, chunkbytes, maxbuf, 0); }

//Button masks sent in inputs
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

//Format of an input event.
typedef struct _sc_input_s
{
	//Common event header
	char format; //Format of the report - 'A' for first player's buttons, 'B' 'C' 'D' for further players
	char flags; //Unused for the moment
	short buttons; //Buttons depressed, as in _SC_BUTTONS macros
} _sc_input_t __attribute__((aligned(4)));

// _sc_input //
//Reads input events, writing them into the given buffer.
//Returns the number of events filled, or a negative error number.
SYSCALL_DECL int _sc_input(_sc_input_t *buffer_ptr, int bytes_per_event, int bytes_max)
	#define _SC_INPUT_N 0x50
	{ return _SC(_SC_INPUT_N, buffer_ptr, bytes_per_event, bytes_max, 0, 0); }

// _sc_nvm_save //
//Writes data to the configured nonvolatile memory record, overwriting any previous data.
//The contents should be in the buffer at "data", of length "len".
//Writes are atomic and update the whole record each time. No partial writes are possible.
//Returns the number of bytes written on success or a negative error number.
SYSCALL_DECL int _sc_nvm_save(const void *data, int len)
	#define _SC_NVM_SAVE_N 0x81
	{ return _SC(_SC_NVM_SAVE_N, data, len, 0, 0, 0); }

// _sc_nvm_load //
//Reads from the configured nonvolatile memory record into the given buffer.
//The buffer to place the results in is at "buf", of length "len".
//Reads are protected by SHA256; corruption will result in the file being lost (-_SC_ENOENT).
//Returns the number of bytes read on success or a negative error number.
SYSCALL_DECL int _sc_nvm_load(void *buf, int len)
	#define _SC_NVM_LOAD_N 0x82
	{ return _SC(_SC_NVM_LOAD_N, buf, len, 0, 0, 0); }
	
// _sc_disk_read2k //
//Reads one or more 2048-byte sectors from the disk into the given buffer.
//The sector number/count are given in units of 2KByte, i.e. not a byte-offset.
//Returns 0 on success or a negative error number.
//May return -_SC_EAGAIN if the operation has started and will finish later.
SYSCALL_DECL int _sc_disk_read2k(int sector_num, void *buf2k, int nsectors)
	#define _SC_DISK_READ2K_N 0x91
	{ return _SC(_SC_DISK_READ2K_N, sector_num, buf2k, nsectors, 0, 0); }

// _sc_disk_write2k //
//Writes one or more 2048-byte sectors to the disk from the given buffer.
//The sector number/count are given in units of 2KByte, i.e. not a byte-offset.
//Returns 0 on success or a negative error number.
//May return -_SC_EAGAIN if the operation has started and will finish later.
SYSCALL_DECL int _sc_disk_write2k(int sector_num, const void *buf2k, int nsectors)
	#define _SC_DISK_WRITE2K_N 0x92
	{ return _SC(_SC_DISK_WRITE2K_N, sector_num, buf2k, nsectors, 0, 0); }
	
// _sc_mexec_append //
//Appends the given data to the kernel's pending memory image for the calling process.
//Subsequent calls append to the buffer; call with buf=len=0 to reset the buffer.
//The first 4KBytes appended are always inaccessible afterwards and can contain anything.
//The memory at address 0x1000 (+4KBytes in) is where execution starts after _sc_mexec_apply.
//Returns the number of bytes appended on success.
//Returns a negative error number if a failure occurs before any bytes were appended.
SYSCALL_DECL int _sc_mexec_append(const void *buf, int len)
	#define _SC_MEXEC_APPEND_N 0xA1
	{ return _SC(_SC_MEXEC_APPEND_N, buf, len, 0, 0, 0); }

// _sc_mexec_apply //
//Concludes an in-memory exec and replaces the current with the pending image.
//Does not return and does not fail.
//If there is no pending image, the caller exits as though killed by _SC_SIGSEGV.
SYSCALL_DECL _NORETURN void _sc_mexec_apply(void)
	#define  _SC_MEXEC_APPLY_N 0xA2
	{ _SC(_SC_MEXEC_APPLY_N, 0, 0, 0, 0, 0); _DONTRETURN; }

//Error numbers that may be returned by the kernel.
//They are defined positively here, but are returned as negative values by the kernel.
//These attempt to be the same as Linux error numbers, but please don't rely on that.
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
#define _SC_EAGAIN       11 //Resource unavailable, try again (may be the same value as EWOULDBLOCK).
#define _SC_EWOULDBLOCK  11 //Operation would block (may be the same value as EAGAIN).
#define _SC_ENOMEM       12 //Not enough space.
#define _SC_EACCES       13 //Permission denied.
#define _SC_EFAULT       14 //Bad address.
#define _SC_EBUSY        16 //Device or resource busy.
#define _SC_EEXIST       17 //File exists.
#define _SC_EXDEV        18 //Cross-device link.
#define _SC_ENODEV       19 //No such device.
#define _SC_ENOTDIR      20 //Not a directory or a symbolic link to a directory.
#define _SC_EISDIR       21 //Is a directory.
#define _SC_EINVAL       22 //Invalid argument.
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

#endif //_SC_H
