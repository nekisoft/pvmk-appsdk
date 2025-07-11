/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#pragma once

#include "cmsys/Configure.hxx" // IWYU pragma: export

#ifdef _MSC_VER
#pragma warning(disable : 4786)
#pragma warning(disable : 4503)
#endif

#ifdef __ICL
#pragma warning(disable : 985)
#pragma warning(disable : 1572) /* floating-point equality test */
#endif

#if defined(__LCC__) && defined(__EDG__)
#if __LCC__ == 123
#pragma diag_suppress 2910 /* excess -Wunused-function in 1.23.x */
#elif __LCC__ == 121
#pragma diag_suppress 2727 /* excess -Wunused-function in 1.21.x */
#include <limits.h>        /* ..._MIN, ..._MAX constants */
#endif
#endif

#undef HAVE_ENVIRON_NOT_REQUIRE_PROTOTYPE
#undef HAVE_UNSETENV
#undef CMake_ENABLE_DEBUGGER
#undef CMake_USE_MACH_PARSER
#undef CMake_USE_XCOFF_PARSER
#undef CMAKE_USE_WMAKE
#define CMake_DEFAULT_RECURSION_LIMIT 16
#define CMAKE_BIN_DIR "/bin"
#define CMAKE_DATA_DIR "/data"
#define CMAKE_DOC_DIR "/doc"

#define CM_FALLTHROUGH cmsys_FALLTHROUGH

#if defined(_WIN32) && !defined(NOMINMAX)
#  define NOMINMAX
#endif

#define CURL_CA_BUNDLE "CURL_CA_BUNDLE"
#define CURL_CA_PATH "CURL_CA_PATH"

#define CMake_STAT_HAS_ST_MTIM 0
#define CMake_STAT_HAS_ST_MTIMESPEC 0

#define KWSYS_ENCODING_DEFAULT_CODEPAGE KWSYS_ENCODING_DEFAULT_CODEPAGE
