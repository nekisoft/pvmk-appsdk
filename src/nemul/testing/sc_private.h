//sc_private.h
//System-call library plus internal-only features that games shouldn't use.
//Bryan E. Topp <betopp@betopp.com> 2024
#ifndef _SC_PRIVATE_H
#define _SC_PRIVATE_H

//Public system calls - applications should only include sc.h.
#include "sc.h"

//Further internal-use-only system calls.
//These will generally be denied to games before they are exec'd.

// _sc_getpid //
//Returns the process ID of the calling process.
//Does not fail.
SYSCALL_DECL int _sc_getpid(void)
	#define _SC_GETPID_N 0x03
	{ return _SC(_SC_GETPID_N, 0, 0, 0, 0, 0); }

// _sc_getppid //
//Returns the process ID of the parent of the calling process.
//Does not fail. Note that the PPID of PID 1 is 0, and no other process has a PPID of 0.
SYSCALL_DECL int _sc_getppid(void)
	#define _SC_GETPPID_N 0x04
	{ return _SC(_SC_GETPPID_N, 0, 0, 0, 0, 0); }

// _sc_fork //
//Creates a copy of the calling process, as a child of the caller.
//The caller and the new child both return from this system-call in an identical state, aside from the return value.
//Returns the PID of the child process in the caller. Returns 0 in the child process.
//Returns a negative error number on failure (an error returns in the parent; no child is created).
SYSCALL_DECL int _sc_fork(void)
	#define _SC_FORK_N 0x05
	{ return _SC(_SC_FORK_N, 0, 0, 0, 0, 0); }

//Bits of status information returned by wait call
#define _SC_STATUS_EXITCODE_MASK      0xFFu
#define _SC_STATUS_EXITCODE_SHIFT 0
#define _SC_STATUS_STOPSIG_MASK     0xFF00u
#define _SC_STATUS_STOPSIG_SHIFT 8
#define _SC_STATUS_TERMSIG_MASK   0xFF0000u
#define _SC_STATUS_TERMSIG_SHIFT 16
#define _SC_STATUS_CONTINUED_BIT 0x1000000u
#define _SC_STATUS_EXITED_BIT    0x2000000u
#define _SC_STATUS_SIGNALED_BIT  0x4000000u
#define _SC_STATUS_STOPPED_BIT   0x8000000u
#define _SC_STATUS_COREDUMP_BIT 0x10000000u

//Types of ID defined for calls which take a Process ID
#define _SC_IDTYPE_ALL 0x0 //All processes
#define _SC_IDTYPE_PID 0x1 //Process ID
#define _SC_IDTYPE_PTREE 0x10 //Process ID and its entire subtree of descendants

//Options passed to wait system-call
#define _SC_WNOWAIT 0x1 //Don't complete the waiting upon this child (leave status info)
#define _SC_WSTOPPED 0x2 //Look for stopped children
#define _SC_WEXITED 0x4 //Look for exited children
#define _SC_WCONTINUED 0x8 //Look for continued children

//Information returned by kernel from a successful call to _sc_wait.
typedef struct _sc_wait_s
{
	int status; //Wait status
	int pid; //Other process
} _sc_wait_t;

// _sc_wait //
//Checks whether another process has changed state.
//Fills in an _sc_wait_t structure with its details, if any.
//Returns the size of the structure filled, or a negative error number.
//Note that this doesn't actually block - use _sc_pause to do the waiting.
SYSCALL_DECL int _sc_wait(int idtype, int id, int options, _sc_wait_t *buf, int len)
	#define _SC_WAIT_N 0x06
	{ return _SC(_SC_WAIT_N, idtype, id, options, buf, len); }

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
	
// _sc_sig_send //
//Sends a signal to the given process, thread, or process group.
//Returns a nonnegative value on success or a negative error number.
SYSCALL_DECL int _sc_sig_send(int idtype, int id, int sig)
	#define _SC_SIG_SEND_N 0x21
	{ return _SC(_SC_SIG_SEND_N, idtype, id, sig, 0, 0); }
	
// _sc_mem_sbrk //
//Changes the size of the calling process's memory space, adding "req" new bytes.
//(The memory space cannot ever be reduced, except by calling exec() with a smaller image.)
//The new bytes are appended at the old end-of-process address. The OLD end-of-process is returned.
//A negative error number is returned on failure (probably, -_SC_ENOMEM).
SYSCALL_DECL int _sc_mem_sbrk(int req)
	#define _SC_MEM_SBRK_N 0x40
	{ return _SC(_SC_MEM_SBRK_N, req, 0, 0, 0, 0); }
	
// _sc_deny_syscall //
//Locks out the given system call, preventing it from being used by this process or any children.
//Returns 0 on success or a negative error number.
SYSCALL_DECL int _sc_deny_syscall(int callnum)
	#define _SC_DENY_SYSCALL_N 0x70
	{ return _SC(_SC_DENY_SYSCALL_N, callnum, 0, 0, 0, 0); }

// _sc_nvm_ident //
//Looks up the given nonvolatile memory record and configures it for saving and loading.
//Ensures the record exists, is fully allocated, and is writable.
//Names can be between 1 and 63 characters long, alphanumeric characters and underscores only, NUL-terminated.
//Returns 0 on success or a negative error number.
SYSCALL_DECL int _sc_nvm_ident(const char *name)
	#define _SC_NVM_IDENT_N 0x80
	{ return _SC(_SC_NVM_IDENT_N, name, 0, 0, 0, 0); }

// _sc_nvm_delete //
//Erases any nonvolatile memory entry with the configured name.
//Returns 0 on success or a negative error number.
SYSCALL_DECL int _sc_nvm_delete(void)
	#define _SC_NVM_DELETE_N 0x83
	{ return _SC(_SC_NVM_DELETE_N, 0, 0, 0, 0, 0); }

// _sc_nvm_enum //
//Enumerates nonvolatile memory names by index (greater than 0).
//Fills the given buffer at name_buf with the name of the file.
//Returns nonnegative on success or a negative error number.
//Returns -_SC_ENOSPC if idx is more than the number of records supported.
//Returns -_SC_ENOENT is idx is valid but unused at the moment.
SYSCALL_DECL int _sc_nvm_enum(int idx, char *name_buf, int name_max)
	#define _SC_NVM_ENUM_N 0x84
	{ return _SC(_SC_NVM_ENUM_N, idx, name_buf, name_max, 0, 0); }

// _sc_rom_read //
//Reads block from boot ROM
SYSCALL_DECL int _sc_rom_read(int off, void *buf, int len)
	#define _SC_ROM_READ_N 0xB1
	{ return _SC(_SC_ROM_READ_N, off, buf, len, 0, 0); }
	
// _sc_rom_write //
//Writes block to boot ROM
SYSCALL_DECL int _sc_rom_write(int off, const void *buf, int len)
	#define _SC_ROM_WRITE_N 0xB2
	{ return _SC(_SC_ROM_WRITE_N, off, buf, len, 0, 0); }

// _sc_halt //
//Shuts down the system, returning to power-off mode (bootloader ROM).
SYSCALL_DECL void _sc_halt(void)
	#define  _SC_HALT_N 0xFE
	{ _SC(_SC_HALT_N, 0, 0, 0, 0, 0); }


#endif //_SC_PRIVATE_H
