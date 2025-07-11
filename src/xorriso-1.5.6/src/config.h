/* config.h.in.  Generated from configure.ac by autoheader.  */

/* Hacked up for PVMK application SDK by betopp, 2025 */

#define Libisofs_use_os_dummY 1
#define Libburn_use_sg_dummY 1
#define LIBDAX_MSGS_SINGLE_THREADED 1
#define LIBISO_MSGS_SINGLE_THREADED 1

//Nasty hack for MSW which doesn't define uid_t or gid_t
#include <sys/types.h>
#if !defined(_UID_T_DECLARED) && !defined(__uid_t_defined)
typedef int uid_t;
#define _UID_T_DECLARED int
#endif
#if !defined(_GID_T_DECLARED) && !defined(__gid_t_defined)
typedef int gid_t;
#define _GID_T_DECLARED int
#endif


/* Define if building universal (internal helper macro) */
#undef AC_APPLE_UNIVERSAL_BUILD

/* Define to 1 if you have the <dlfcn.h> header file. */
#undef HAVE_DLFCN_H

/* Define this if eaccess function is available */
#undef HAVE_EACCESS

/* Define to 1 if fseeko (and presumably ftello) exists and is declared. */
#undef HAVE_FSEEKO

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the `acl' library (-lacl). */
#undef HAVE_LIBACL

/* Define to 1 if you have the `bz2' library (-lbz2). */
#undef HAVE_LIBBZ2

/* Define to 1 if you have the `cdio' library (-lcdio). */
#undef HAVE_LIBCDIO

/* Define to 1 if you have the `iconv' library (-liconv). */
#undef HAVE_LIBICONV

/* Define to 1 if you have the `readline' library (-lreadline). */
#undef HAVE_LIBREADLINE

/* Define to 1 if you have the `z' library (-lz). */
#undef HAVE_LIBZ

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define this if timegm function is available */
#define HAVE_TIMEGM 1

/* Define this if tm structure includes a tm_gmtoff entry. */
#undef HAVE_TM_GMTOFF

/* Define to 1 if you have the <unistd.h> header file. */
#undef HAVE_UNISTD_H

/* Whether to apply const qualifier to iconv inbuf */
#undef ICONV_CONST

/* Define to use libbz2 by built-in libjte */
#undef LIBJTE_WITH_LIBBZ2

/* Allow libjte to use zlib */
#undef LIBJTE_WITH_ZLIB

/* Define to the sub-directory in which libtool stores uninstalled libraries.
   */
#undef LT_OBJDIR

/* Define to use statvfs() with libburn stdio */
#undef Libburn_os_has_statvfS

/* Define to use O_DIRECT with -as cdrskin */
#undef Libburn_read_o_direcT

/* Define to use libcdio as system adapter */
#undef Libburn_use_libcdiO

/* Define to compile without OS specific SCSI features */
#define Libburn_use_sg_dummY 1

/* Either timezone or 0 */
#define Libburnia_timezonE 0

/* Define to use ACL capabilities */
#undef Libisofs_with_aaip_acL

/* Define to use Linux xattr capabilities */
#undef Libisofs_with_aaip_xattR

/* Define to use FreeBSD extattr capabilities */
#undef Libisofs_with_freebsd_extattR

/* Define to use Jigdo Template Extraction via libjte */
#undef Libisofs_with_libjtE

/* Define to include Linux sys/xattr.h instead of attr/xattr.h */
#undef Libisofs_with_sys_xattR

/* Define to use compression via zlib */
#undef Libisofs_with_zliB

/* Name of package */
#undef PACKAGE

/* Define to the address where bug reports for this package should be sent. */
#undef PACKAGE_BUGREPORT

/* Define to the full name of this package. */
#undef PACKAGE_NAME

/* Define to the full name and version of this package. */
#undef PACKAGE_STRING

/* Define to the one symbol short name of this package. */
#undef PACKAGE_TARNAME

/* Define to the home page for this package. */
#undef PACKAGE_URL

/* Define to the version of this package. */
#undef PACKAGE_VERSION

/* Define to 1 if you have the ANSI C header files. */
#undef STDC_HEADERS

/* Define to use multi-threading in built-in libjte */
#undef THREADED_CHECKSUMS

/* Version number of package */
#undef VERSION

/* Define WORDS_BIGENDIAN to 1 if your processor stores words with the most
   significant byte first (like Motorola and SPARC, unlike Intel). */
#if defined AC_APPLE_UNIVERSAL_BUILD
# if defined __BIG_ENDIAN__
#  define WORDS_BIGENDIAN 1
# endif
#else
# ifndef WORDS_BIGENDIAN
#  undef WORDS_BIGENDIAN
# endif
#endif

/* Define to allow xorriso to start external filter processes */
#undef Xorriso_allow_external_filterS

/* Define to allow xorriso command -launch_frontend when running under setuid
   */
#undef Xorriso_allow_extf_suiD

/* Define to allow xorriso command -launch_frontend */
#undef Xorriso_allow_launch_frontenD

/* Define to make 64 KB default size for DVD writing */
#undef Xorriso_dvd_obs_default_64K

/* Define to prepare sources for statically linked xorriso */
#define Xorriso_standalonE 1

/* Define to use libedit if not libreadline */
#undef Xorriso_with_editlinE

/* Define to use Jigdo Template Extraction via libjte */
#undef Xorriso_with_libjtE

/* Define to use libreadline */
#undef Xorriso_with_readlinE

/* Enable large inode numbers on Mac OS X 10.5.  */
#ifndef _DARWIN_USE_64_BIT_INODE
# define _DARWIN_USE_64_BIT_INODE 1
#endif

/* Number of bits in a file offset, on hosts where this is settable. */
#define _FILE_OFFSET_BITS 64

/* Define to 1 to make fseeko visible on some hosts (e.g. glibc 2.2). */
#define _LARGEFILE_SOURCE 1

/* Define for large files, on AIX-style hosts. */
#define _LARGE_FILES 1

/* Define to empty if `const' does not conform to ANSI C. */
#undef const

/* Define to `__inline__' or `__inline' if that's what the C compiler
   calls it, or to nothing if 'inline' is not supported under any name.  */
#ifndef __cplusplus
#undef inline
#endif
