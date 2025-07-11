/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing#kwsys for details.  */
#ifndef cmsys_Process_h
#define cmsys_Process_h

#include <cmsys/Configure.h>

/* Redefine all public interface symbol names to be in the proper
   namespace.  These macros are used internally to kwsys only, and are
   not visible to user code.  Use kwsysHeaderDump.pl to reproduce
   these macros after making changes to the interface.  */
#if !defined(KWSYS_NAMESPACE)
#  define kwsys_ns(x) cmsys##x
#  define kwsysEXPORT cmsys_EXPORT
#endif
#if !cmsys_NAME_IS_KWSYS
#  define kwsysProcess kwsys_ns(Process)
#  define kwsysProcess_s kwsys_ns(Process_s)
#  define kwsysProcess_New kwsys_ns(Process_New)
#  define kwsysProcess_Delete kwsys_ns(Process_Delete)
#  define kwsysProcess_SetCommand kwsys_ns(Process_SetCommand)
#  define kwsysProcess_AddCommand kwsys_ns(Process_AddCommand)
#  define kwsysProcess_SetTimeout kwsys_ns(Process_SetTimeout)
#  define kwsysProcess_SetWorkingDirectory                                    \
    kwsys_ns(Process_SetWorkingDirectory)
#  define kwsysProcess_SetPipeFile kwsys_ns(Process_SetPipeFile)
#  define kwsysProcess_SetPipeNative kwsys_ns(Process_SetPipeNative)
#  define kwsysProcess_SetPipeShared kwsys_ns(Process_SetPipeShared)
#  define kwsysProcess_Option_Detach kwsys_ns(Process_Option_Detach)
#  define kwsysProcess_Option_HideWindow kwsys_ns(Process_Option_HideWindow)
#  define kwsysProcess_Option_MergeOutput kwsys_ns(Process_Option_MergeOutput)
#  define kwsysProcess_Option_Verbatim kwsys_ns(Process_Option_Verbatim)
#  define kwsysProcess_Option_CreateProcessGroup                              \
    kwsys_ns(Process_Option_CreateProcessGroup)
#  define kwsysProcess_GetOption kwsys_ns(Process_GetOption)
#  define kwsysProcess_SetOption kwsys_ns(Process_SetOption)
#  define kwsysProcess_Option_e kwsys_ns(Process_Option_e)
#  define kwsysProcess_State_Starting kwsys_ns(Process_State_Starting)
#  define kwsysProcess_State_Error kwsys_ns(Process_State_Error)
#  define kwsysProcess_State_Exception kwsys_ns(Process_State_Exception)
#  define kwsysProcess_State_Executing kwsys_ns(Process_State_Executing)
#  define kwsysProcess_State_Exited kwsys_ns(Process_State_Exited)
#  define kwsysProcess_State_Expired kwsys_ns(Process_State_Expired)
#  define kwsysProcess_State_Killed kwsys_ns(Process_State_Killed)
#  define kwsysProcess_State_Disowned kwsys_ns(Process_State_Disowned)
#  define kwsysProcess_State_e kwsys_ns(Process_State_e)
#  define kwsysProcess_Exception_None kwsys_ns(Process_Exception_None)
#  define kwsysProcess_Exception_Fault kwsys_ns(Process_Exception_Fault)
#  define kwsysProcess_Exception_Illegal kwsys_ns(Process_Exception_Illegal)
#  define kwsysProcess_Exception_Interrupt                                    \
    kwsys_ns(Process_Exception_Interrupt)
#  define kwsysProcess_Exception_Numerical                                    \
    kwsys_ns(Process_Exception_Numerical)
#  define kwsysProcess_Exception_Other kwsys_ns(Process_Exception_Other)
#  define kwsysProcess_Exception_e kwsys_ns(Process_Exception_e)
#  define kwsysProcess_GetState kwsys_ns(Process_GetState)
#  define kwsysProcess_GetExitException kwsys_ns(Process_GetExitException)
#  define kwsysProcess_GetExitCode kwsys_ns(Process_GetExitCode)
#  define kwsysProcess_GetExitValue kwsys_ns(Process_GetExitValue)
#  define kwsysProcess_GetErrorString kwsys_ns(Process_GetErrorString)
#  define kwsysProcess_GetExceptionString kwsys_ns(Process_GetExceptionString)
#  define kwsysProcess_GetStateByIndex kwsys_ns(Process_GetStateByIndex)
#  define kwsysProcess_GetExitExceptionByIndex                                \
    kwsys_ns(Process_GetExitExceptionByIndex)
#  define kwsysProcess_GetExitCodeByIndex kwsys_ns(Process_GetExitCodeByIndex)
#  define kwsysProcess_GetExitValueByIndex                                    \
    kwsys_ns(Process_GetExitValueByIndex)
#  define kwsysProcess_GetExceptionStringByIndex                              \
    kwsys_ns(Process_GetExceptionStringByIndex)
#  define kwsysProcess_GetExitCodeByIndex kwsys_ns(Process_GetExitCodeByIndex)
#  define kwsysProcess_Execute kwsys_ns(Process_Execute)
#  define kwsysProcess_Disown kwsys_ns(Process_Disown)
#  define kwsysProcess_WaitForData kwsys_ns(Process_WaitForData)
#  define kwsysProcess_Pipes_e kwsys_ns(Process_Pipes_e)
#  define kwsysProcess_Pipe_None kwsys_ns(Process_Pipe_None)
#  define kwsysProcess_Pipe_STDIN kwsys_ns(Process_Pipe_STDIN)
#  define kwsysProcess_Pipe_STDOUT kwsys_ns(Process_Pipe_STDOUT)
#  define kwsysProcess_Pipe_STDERR kwsys_ns(Process_Pipe_STDERR)
#  define kwsysProcess_Pipe_Timeout kwsys_ns(Process_Pipe_Timeout)
#  define kwsysProcess_Pipe_Handle kwsys_ns(Process_Pipe_Handle)
#  define kwsysProcess_WaitForExit kwsys_ns(Process_WaitForExit)
#  define kwsysProcess_Interrupt kwsys_ns(Process_Interrupt)
#  define kwsysProcess_Kill kwsys_ns(Process_Kill)
#  define kwsysProcess_KillPID kwsys_ns(Process_KillPID)
#  define kwsysProcess_ResetStartTime kwsys_ns(Process_ResetStartTime)
#endif

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * Process control data structure.
 */
typedef struct kwsysProcess_s kwsysProcess;

/* Platform-specific pipe handle type.  */
#if defined(_WIN32) && !defined(__CYGWIN__)
typedef void* kwsysProcess_Pipe_Handle;
#else
typedef int kwsysProcess_Pipe_Handle;
#endif

/**
 * Create a new Process instance.
 */
kwsysEXPORT kwsysProcess* kwsysProcess_New(void);

/**
 * Delete an existing Process instance.  If the instance is currently
 * executing a process, this blocks until the process terminates.
 */
kwsysEXPORT void kwsysProcess_Delete(kwsysProcess* cp);

/**
 * Set the command line to be executed.  Argument is an array of
 * pointers to the command and each argument.  The array must end with
 * a NULL pointer.  Any previous command lines are removed.  Returns
 * 1 for success and 0 otherwise.
 */
kwsysEXPORT int kwsysProcess_SetCommand(kwsysProcess* cp,
                                        char const* const* command);

/**
 * Add a command line to be executed.  Argument is an array of
 * pointers to the command and each argument.  The array must end with
 * a NULL pointer.  If this is not the first command added, its
 * standard input will be connected to the standard output of the
 * previous command.  Returns 1 for success and 0 otherwise.
 */
kwsysEXPORT int kwsysProcess_AddCommand(kwsysProcess* cp,
                                        char const* const* command);

/**
 * Set the timeout in seconds for the child process.  The timeout
 * period begins when the child is executed.  If the child has not
 * terminated when the timeout expires, it will be killed.  A
 * non-positive (<= 0) value will disable the timeout.
 */
kwsysEXPORT void kwsysProcess_SetTimeout(kwsysProcess* cp, double timeout);

/**
 * Set the working directory for the child process.  The working
 * directory can be absolute or relative to the current directory.
 * Returns 1 for success and 0 for failure.
 */
kwsysEXPORT int kwsysProcess_SetWorkingDirectory(kwsysProcess* cp,
                                                 char const* dir);

/**
 * Set the name of a file to be attached to the given pipe.  Returns 1
 * for success and 0 for failure.
 */
kwsysEXPORT int kwsysProcess_SetPipeFile(kwsysProcess* cp, int pipe,
                                         char const* file);

/**
 * Set whether the given pipe in the child is shared with the parent
 * process.  The default is no for Pipe_STDOUT and Pipe_STDERR and yes
 * for Pipe_STDIN.
 */
kwsysEXPORT void kwsysProcess_SetPipeShared(kwsysProcess* cp, int pipe,
                                            int shared);

/**
 * Specify a platform-specific native pipe for use as one of the child
 * interface pipes.  The native pipe is specified by an array of two
 * descriptors or handles.  The first entry in the array (index 0)
 * should be the read end of the pipe.  The second entry in the array
 * (index 1) should be the write end of the pipe.  If a null pointer
 * is given the option will be disabled.
 *
 * For Pipe_STDIN the native pipe is connected to the first child in
 * the pipeline as its stdin.  After the children are created the
 * write end of the pipe will be closed in the child process and the
 * read end will be closed in the parent process.
 *
 * For Pipe_STDOUT and Pipe_STDERR the pipe is connected to the last
 * child as its stdout or stderr.  After the children are created the
 * write end of the pipe will be closed in the parent process and the
 * read end will be closed in the child process.
 */
kwsysEXPORT void kwsysProcess_SetPipeNative(
  kwsysProcess* cp, int pipe, kwsysProcess_Pipe_Handle const p[2]);

/**
 * Get/Set a possibly platform-specific option.  Possible options are:
 *
 *  kwsysProcess_Option_Detach = Whether to detach the process.
 *         0 = No (default)
 *         1 = Yes
 *
 *  kwsysProcess_Option_HideWindow = Whether to hide window on Windows.
 *         0 = No (default)
 *         1 = Yes
 *
 *  kwsysProcess_Option_MergeOutput = Whether to merge stdout/stderr.
 *                                    No content will be returned as stderr.
 *                                    Any actual stderr will be on stdout.
 *         0 = No (default)
 *         1 = Yes
 *
 *  kwsysProcess_Option_Verbatim = Whether SetCommand and AddCommand
 *                                 should treat the first argument
 *                                 as a verbatim command line
 *                                 and ignore the rest of the arguments.
 *         0 = No (default)
 *         1 = Yes
 *
 *  kwsysProcess_Option_CreateProcessGroup = Whether to place the process in a
 *                                           new process group.  This is
 *                                           useful if you want to send Ctrl+C
 *                                           to the process.  On UNIX, also
 *                                           places the process in a new
 *                                           session.
 *         0 = No (default)
 *         1 = Yes
 */
kwsysEXPORT int kwsysProcess_GetOption(kwsysProcess* cp, int optionId);
kwsysEXPORT void kwsysProcess_SetOption(kwsysProcess* cp, int optionId,
                                        int value);
enum kwsysProcess_Option_e
{
  kwsysProcess_Option_HideWindow,
  kwsysProcess_Option_Detach,
  kwsysProcess_Option_MergeOutput,
  kwsysProcess_Option_Verbatim,
  kwsysProcess_Option_CreateProcessGroup
};

/**
 * Get the current state of the Process instance.  Possible states are:
 *
 *  kwsysProcess_State_Starting  = Execute has not yet been called.
 *  kwsysProcess_State_Error     = Error administrating the child process.
 *  kwsysProcess_State_Exception = Child process exited abnormally.
 *  kwsysProcess_State_Executing = Child process is currently running.
 *  kwsysProcess_State_Exited    = Child process exited normally.
 *  kwsysProcess_State_Expired   = Child process's timeout expired.
 *  kwsysProcess_State_Killed    = Child process terminated by Kill method.
 *  kwsysProcess_State_Disowned  = Child is no longer managed by this object.
 */
kwsysEXPORT int kwsysProcess_GetState(kwsysProcess* cp);
enum kwsysProcess_State_e
{
  kwsysProcess_State_Starting,
  kwsysProcess_State_Error,
  kwsysProcess_State_Exception,
  kwsysProcess_State_Executing,
  kwsysProcess_State_Exited,
  kwsysProcess_State_Expired,
  kwsysProcess_State_Killed,
  kwsysProcess_State_Disowned
};

/**
 * When GetState returns "Exception", this method returns a
 * platform-independent description of the exceptional behavior that
 * caused the child to terminate abnormally.  Possible exceptions are:
 *
 *  kwsysProcess_Exception_None      = No exceptional behavior occurred.
 *  kwsysProcess_Exception_Fault     = Child crashed with a memory fault.
 *  kwsysProcess_Exception_Illegal   = Child crashed with an illegal
 * instruction.
 *  kwsysProcess_Exception_Interrupt = Child was interrupted by user
 * (Cntl-C/Break).
 *  kwsysProcess_Exception_Numerical = Child crashed with a numerical
 * exception.
 *  kwsysProcess_Exception_Other     = Child terminated for another reason.
 */
kwsysEXPORT int kwsysProcess_GetExitException(kwsysProcess* cp);
enum kwsysProcess_Exception_e
{
  kwsysProcess_Exception_None,
  kwsysProcess_Exception_Fault,
  kwsysProcess_Exception_Illegal,
  kwsysProcess_Exception_Interrupt,
  kwsysProcess_Exception_Numerical,
  kwsysProcess_Exception_Other
};

/**
 * When GetState returns "Exited" or "Exception", this method returns
 * the platform-specific raw exit code of the process.  UNIX platforms
 * should use WIFEXITED/WEXITSTATUS and WIFSIGNALED/WTERMSIG to access
 * this value.  Windows users should compare the value to the various
 * EXCEPTION_* values.
 *
 * If GetState returns "Exited", use GetExitValue to get the
 * platform-independent child return value.
 */
kwsysEXPORT int kwsysProcess_GetExitCode(kwsysProcess* cp);

/**
 * When GetState returns "Exited", this method returns the child's
 * platform-independent exit code (such as the value returned by the
 * child's main).
 */
kwsysEXPORT int kwsysProcess_GetExitValue(kwsysProcess* cp);

/**
 * When GetState returns "Error", this method returns a string
 * describing the problem.  Otherwise, it returns NULL.
 */
kwsysEXPORT const char* kwsysProcess_GetErrorString(kwsysProcess* cp);

/**
 * When GetState returns "Exception", this method returns a string
 * describing the problem.  Otherwise, it returns NULL.
 */
kwsysEXPORT const char* kwsysProcess_GetExceptionString(kwsysProcess* cp);

/**
 * Get the current state of the Process instance.  Possible states are:
 *
 *  kwsysProcess_StateByIndex_Starting  = Execute has not yet been called.
 *  kwsysProcess_StateByIndex_Exception = Child process exited abnormally.
 *  kwsysProcess_StateByIndex_Exited    = Child process exited normally.
 *  kwsysProcess_StateByIndex_Error     = Error getting the child return code.
 */
kwsysEXPORT int kwsysProcess_GetStateByIndex(kwsysProcess* cp, int idx);
enum kwsysProcess_StateByIndex_e
{
  kwsysProcess_StateByIndex_Starting = kwsysProcess_State_Starting,
  kwsysProcess_StateByIndex_Exception = kwsysProcess_State_Exception,
  kwsysProcess_StateByIndex_Exited = kwsysProcess_State_Exited,
  kwsysProcess_StateByIndex_Error = kwsysProcess_State_Error
};

/**
 * When GetState returns "Exception", this method returns a
 * platform-independent description of the exceptional behavior that
 * caused the child to terminate abnormally.  Possible exceptions are:
 *
 *  kwsysProcess_Exception_None      = No exceptional behavior occurred.
 *  kwsysProcess_Exception_Fault     = Child crashed with a memory fault.
 *  kwsysProcess_Exception_Illegal   = Child crashed with an illegal
 *                                     instruction.
 *  kwsysProcess_Exception_Interrupt = Child was interrupted by user
 *                                     (Cntl-C/Break).
 *  kwsysProcess_Exception_Numerical = Child crashed with a numerical
 *                                     exception.
 *  kwsysProcess_Exception_Other     = Child terminated for another reason.
 */
kwsysEXPORT int kwsysProcess_GetExitExceptionByIndex(kwsysProcess* cp,
                                                     int idx);

/**
 * When GetState returns "Exited" or "Exception", this method returns
 * the platform-specific raw exit code of the process.  UNIX platforms
 * should use WIFEXITED/WEXITSTATUS and WIFSIGNALED/WTERMSIG to access
 * this value.  Windows users should compare the value to the various
 * EXCEPTION_* values.
 *
 * If GetState returns "Exited", use GetExitValue to get the
 * platform-independent child return value.
 */
kwsysEXPORT int kwsysProcess_GetExitCodeByIndex(kwsysProcess* cp, int idx);

/**
 * When GetState returns "Exited", this method returns the child's
 * platform-independent exit code (such as the value returned by the
 * child's main).
 */
kwsysEXPORT int kwsysProcess_GetExitValueByIndex(kwsysProcess* cp, int idx);

/**
 * When GetState returns "Exception", this method returns a string
 * describing the problem.  Otherwise, it returns NULL.
 */
kwsysEXPORT const char* kwsysProcess_GetExceptionStringByIndex(
  kwsysProcess* cp, int idx);

/**
 * Start executing the child process.
 */
kwsysEXPORT void kwsysProcess_Execute(kwsysProcess* cp);

/**
 * Stop management of a detached child process.  This closes any pipes
 * being read.  If the child was not created with the
 * kwsysProcess_Option_Detach option, this method does nothing.  This
 * is because disowning a non-detached process will cause the child
 * exit signal to be left unhandled until this process exits.
 */
kwsysEXPORT void kwsysProcess_Disown(kwsysProcess* cp);

/**
 * Block until data are available on a pipe, a timeout expires, or the
 * child process terminates.  Arguments are as follows:
 *
 *  data    = If data are read, the pointer to which this points is
 *            set to point to the data.
 *  length  = If data are read, the integer to which this points is
 *            set to the length of the data read.
 *  timeout = Specifies the maximum time this call may block.  Upon
 *            return after reading data, the time elapsed is subtracted
 *            from the timeout value.  If this timeout expires, the
 *            value is set to 0.  A NULL pointer passed for this argument
 *            indicates no timeout for the call.  A negative or zero
 *            value passed for this argument may be used for polling
 *            and will always return immediately.
 *
 * Return value will be one of:
 *
 *   Pipe_None    = No more data will be available from the child process,
 *    ( == 0)       or no process has been executed.  WaitForExit should
 *                  be called to wait for the process to terminate.
 *   Pipe_STDOUT  = Data have been read from the child's stdout pipe.
 *   Pipe_STDERR  = Data have been read from the child's stderr pipe.
 *   Pipe_Timeout = No data available within timeout specified for the
 *                  call.  Time elapsed has been subtracted from timeout
 *                  argument.
 */
kwsysEXPORT int kwsysProcess_WaitForData(kwsysProcess* cp, char** data,
                                         int* length, double* timeout);
enum kwsysProcess_Pipes_e
{
  kwsysProcess_Pipe_None,
  kwsysProcess_Pipe_STDIN,
  kwsysProcess_Pipe_STDOUT,
  kwsysProcess_Pipe_STDERR,
  kwsysProcess_Pipe_Timeout = 255
};

/**
 * Block until the child process terminates or the given timeout
 * expires.  If no process is running, returns immediately.  The
 * argument is:
 *
 *  timeout = Specifies the maximum time this call may block.  Upon
 *            returning due to child termination, the elapsed time
 *            is subtracted from the given value.  A NULL pointer
 *            passed for this argument indicates no timeout for the
 *            call.
 *
 * Return value will be one of:
 *
 *    0 = Child did not terminate within timeout specified for
 *        the call.  Time elapsed has been subtracted from timeout
 *        argument.
 *    1 = Child has terminated or was not running.
 */
kwsysEXPORT int kwsysProcess_WaitForExit(kwsysProcess* cp, double* timeout);

/**
 * Interrupt the process group for the child process that is currently
 * running by sending it the appropriate operating-system specific signal.
 * The caller should call WaitForExit after this returns to wait for the
 * child to terminate.
 *
 * WARNING:  If you didn't specify kwsysProcess_Option_CreateProcessGroup,
 * you will interrupt your own process group.
 */
kwsysEXPORT void kwsysProcess_Interrupt(kwsysProcess* cp);

/**
 * Forcefully terminate the child process that is currently running.
 * The caller should call WaitForExit after this returns to wait for
 * the child to terminate.
 */
kwsysEXPORT void kwsysProcess_Kill(kwsysProcess* cp);

/**
 * Same as kwsysProcess_Kill using process ID to locate process to
 * terminate.
 * @see kwsysProcess_Kill(kwsysProcess* cp)
 */
kwsysEXPORT void kwsysProcess_KillPID(unsigned long);

/**
 * Reset the start time of the child process to the current time.
 */
kwsysEXPORT void kwsysProcess_ResetStartTime(kwsysProcess* cp);

#if defined(__cplusplus)
} /* extern "C" */
#endif

/* If we are building a kwsys .c or .cxx file, let it use these macros.
   Otherwise, undefine them to keep the namespace clean.  */
#if !defined(KWSYS_NAMESPACE)
#  undef kwsys_ns
#  undef kwsysEXPORT
#  if !cmsys_NAME_IS_KWSYS
#    undef kwsysProcess
#    undef kwsysProcess_s
#    undef kwsysProcess_New
#    undef kwsysProcess_Delete
#    undef kwsysProcess_SetCommand
#    undef kwsysProcess_AddCommand
#    undef kwsysProcess_SetTimeout
#    undef kwsysProcess_SetWorkingDirectory
#    undef kwsysProcess_SetPipeFile
#    undef kwsysProcess_SetPipeNative
#    undef kwsysProcess_SetPipeShared
#    undef kwsysProcess_Option_Detach
#    undef kwsysProcess_Option_HideWindow
#    undef kwsysProcess_Option_MergeOutput
#    undef kwsysProcess_Option_Verbatim
#    undef kwsysProcess_Option_CreateProcessGroup
#    undef kwsysProcess_GetOption
#    undef kwsysProcess_SetOption
#    undef kwsysProcess_Option_e
#    undef kwsysProcess_State_Starting
#    undef kwsysProcess_State_Error
#    undef kwsysProcess_State_Exception
#    undef kwsysProcess_State_Executing
#    undef kwsysProcess_State_Exited
#    undef kwsysProcess_State_Expired
#    undef kwsysProcess_State_Killed
#    undef kwsysProcess_State_Disowned
#    undef kwsysProcess_GetState
#    undef kwsysProcess_State_e
#    undef kwsysProcess_Exception_None
#    undef kwsysProcess_Exception_Fault
#    undef kwsysProcess_Exception_Illegal
#    undef kwsysProcess_Exception_Interrupt
#    undef kwsysProcess_Exception_Numerical
#    undef kwsysProcess_Exception_Other
#    undef kwsysProcess_GetExitException
#    undef kwsysProcess_Exception_e
#    undef kwsysProcess_GetExitCode
#    undef kwsysProcess_GetExitValue
#    undef kwsysProcess_GetErrorString
#    undef kwsysProcess_GetExceptionString
#    undef kwsysProcess_Execute
#    undef kwsysProcess_Disown
#    undef kwsysProcess_WaitForData
#    undef kwsysProcess_Pipes_e
#    undef kwsysProcess_Pipe_None
#    undef kwsysProcess_Pipe_STDIN
#    undef kwsysProcess_Pipe_STDOUT
#    undef kwsysProcess_Pipe_STDERR
#    undef kwsysProcess_Pipe_Timeout
#    undef kwsysProcess_Pipe_Handle
#    undef kwsysProcess_WaitForExit
#    undef kwsysProcess_Interrupt
#    undef kwsysProcess_Kill
#    undef kwsysProcess_KillPID
#    undef kwsysProcess_ResetStartTime
#  endif
#endif

#endif
