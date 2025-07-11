/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing#kwsys for details.  */
#ifndef cmsys_DynamicLoader_hxx
#define cmsys_DynamicLoader_hxx

#include <cmsys/Configure.hxx>

#include <string>

#if defined(__hpux)
#  include <dl.h>
#elif defined(_WIN32) && !defined(__CYGWIN__)
#  include <windows.h>
#elif defined(__APPLE__)
#  include <AvailabilityMacros.h>
#  if MAC_OS_X_VERSION_MAX_ALLOWED < 1030
#    include <mach-o/dyld.h>
#  endif
#elif defined(__BEOS__)
#  include <be/kernel/image.h>
#endif

namespace cmsys {
/** \class DynamicLoader
 * \brief Portable loading of dynamic libraries or dll's.
 *
 * DynamicLoader provides a portable interface to loading dynamic
 * libraries or dll's into a process.
 *
 * Directory currently works with Windows, Apple, HP-UX and Unix (POSIX)
 * operating systems
 *
 * \warning dlopen on *nix system works the following way:
 * If filename contains a slash ("/"), then it is interpreted as a (relative
 * or absolute) pathname.  Otherwise, the dynamic linker searches for the
 * library as follows : see ld.so(8) for further details):
 * Whereas this distinction does not exist on Win32. Therefore ideally you
 * should be doing full path to guarantee to have a consistent way of dealing
 * with dynamic loading of shared library.
 *
 * \warning the Cygwin implementation do not use the Win32 HMODULE. Put extra
 * condition so that we can include the correct declaration (POSIX)
 */

class cmsys_EXPORT DynamicLoader
{
public:
// Ugly stuff for library handles
// They are different on several different OS's
#if defined(__hpux)
  typedef shl_t LibraryHandle;
#elif defined(_WIN32) && !defined(__CYGWIN__)
  typedef HMODULE LibraryHandle;
#elif defined(__APPLE__)
#  if MAC_OS_X_VERSION_MAX_ALLOWED < 1030
  typedef NSModule LibraryHandle;
#  else
  typedef void* LibraryHandle;
#  endif
#elif defined(__BEOS__)
  typedef image_id LibraryHandle;
#else // POSIX
  typedef void* LibraryHandle;
#endif

  // Return type from DynamicLoader::GetSymbolAddress.
  typedef void (*SymbolPointer)();

  enum OpenFlags
  {
    // Search for dependent libraries beside the library being loaded.
    //
    // This is currently only supported on Windows.
    SearchBesideLibrary = 0x00000001,

    // Make loaded symbols visible globally
    //
    // This is currently only supported on *nix systems.
    RTLDGlobal = 0x00000002,

    AllOpenFlags = SearchBesideLibrary | RTLDGlobal
  };

  /** Load a dynamic library into the current process.
   * The returned LibraryHandle can be used to access the symbols in the
   * library. The optional second argument is a set of flags to use when
   * opening the library. If unrecognized or unsupported flags are specified,
   * the library is not opened. */
  static LibraryHandle OpenLibrary(std::string const&);
  static LibraryHandle OpenLibrary(std::string const&, int);

  /** Attempt to detach a dynamic library from the
   * process.  A value of true is returned if it is successful. */
  static int CloseLibrary(LibraryHandle);

  /** Find the address of the symbol in the given library. */
  static SymbolPointer GetSymbolAddress(LibraryHandle, std::string const&);

  /** Return the default module prefix for the current platform.  */
  static char const* LibPrefix() { return "@KWSYS_DynamicLoader_PREFIX@"; }

  /** Return the default module suffix for the current platform.  */
  static char const* LibExtension() { return "@KWSYS_DynamicLoader_SUFFIX@"; }

  /** Return the last error produced from a calls made on this class. */
  static char const* LastError();
}; // End Class: DynamicLoader

} // namespace cmsys

#endif
