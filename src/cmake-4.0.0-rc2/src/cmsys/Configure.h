/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing#kwsys for details.  */
#ifndef cmsys_Configure_h
#define cmsys_Configure_h

/* If we are building a kwsys .c or .cxx file, let it use the kwsys
   namespace.  When not building a kwsys source file these macros are
   temporarily defined inside the headers that use them.  */
#if defined(KWSYS_NAMESPACE)
#  define kwsys_ns(x) cmsys##x
#  define kwsysEXPORT cmsys_EXPORT
#endif

/* Disable some warnings inside kwsys source files.  */
#if defined(KWSYS_NAMESPACE)
#  if defined(__INTEL_COMPILER)
#    pragma warning(disable : 1572) /* floating-point equality test */
#  endif
#  if defined(__sgi) && !defined(__GNUC__)
#    pragma set woff 3970 /* pointer to int conversion */
#    pragma set woff 3968 /* 64 bit conversion */
#  endif
#endif

/* Whether kwsys namespace is "kwsys".  */
#define cmsys_NAME_IS_KWSYS 0 /* @KWSYS_NAME_IS_KWSYS@ */

/* Setup the export macro.  */
#if 0 /*@KWSYS_BUILD_SHARED@*/
#  if defined(_WIN32) || defined(__CYGWIN__)
#    if defined(cmsys_EXPORTS)
#      define cmsys_EXPORT __declspec(dllexport)
#    else
#      define cmsys_EXPORT __declspec(dllimport)
#    endif
#  elif __GNUC__ >= 4
#    define cmsys_EXPORT __attribute__((visibility("default")))
#  else
#    define cmsys_EXPORT
#  endif
#else
#  define cmsys_EXPORT
#endif

/* Enable warnings that are off by default but are useful.  */
#if !defined(cmsys_NO_WARNING_ENABLE)
#  if defined(_MSC_VER)
#    pragma warning(default : 4263) /* no override, call convention differs   \
                                     */
#  endif
#endif

/* Disable warnings that are on by default but occur in valid code.  */
#if !defined(cmsys_NO_WARNING_DISABLE)
#  if defined(_MSC_VER)
#    pragma warning(disable : 4097) /* typedef is synonym for class */
#    pragma warning(disable : 4127) /* conditional expression is constant */
#    pragma warning(disable : 4244) /* possible loss in conversion */
#    pragma warning(disable : 4251) /* missing DLL-interface */
#    pragma warning(disable : 4305) /* truncation from type1 to type2 */
#    pragma warning(disable : 4309) /* truncation of constant value */
#    pragma warning(disable : 4514) /* unreferenced inline function */
#    pragma warning(disable : 4706) /* assignment in conditional expression   \
                                     */
#    pragma warning(disable : 4710) /* function not inlined */
#    pragma warning(disable : 4786) /* identifier truncated in debug info */
#  endif
#endif

/* MSVC 6.0 in release mode will warn about code it produces with its
   optimizer.  Disable the warnings specifically for this
   configuration.  Real warnings will be revealed by a debug build or
   by other compilers.  */
#if !defined(cmsys_NO_WARNING_DISABLE_BOGUS)
#  if defined(_MSC_VER) && (_MSC_VER < 1300) && defined(NDEBUG)
#    pragma warning(disable : 4701) /* Variable may be used uninitialized. */
#    pragma warning(disable : 4702) /* Unreachable code.  */
#  endif
#endif

#endif
