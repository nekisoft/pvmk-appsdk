/* config.h.  Generated from build/cmake/config.h.in by cmake configure */
#define __LIBARCHIVE_CONFIG_H_INCLUDED 1
#if defined(__osf__)
# define _OSF_SOURCE
#endif

/*
 * Ensure we have C99-style int64_t, etc, all defined.
 */

/* Define ZLIB_WINAPI if zlib was built on Visual Studio. */
#undef ZLIB_WINAPI

/* Darwin ACL support */
#undef ARCHIVE_ACL_DARWIN

/* FreeBSD ACL support */
#undef ARCHIVE_ACL_FREEBSD

/* FreeBSD NFSv4 ACL support */
#undef ARCHIVE_ACL_FREEBSD_NFS4

/* Linux POSIX.1e ACL support via libacl */
#undef ARCHIVE_ACL_LIBACL

/* Linux NFSv4 ACL support via librichacl */
#undef ARCHIVE_ACL_LIBRICHACL

/* Solaris ACL support */
#undef ARCHIVE_ACL_SUNOS

/* Solaris NFSv4 ACL support */
#undef ARCHIVE_ACL_SUNOS_NFS4

/* MD5 via ARCHIVE_CRYPTO_MD5_LIBC supported. */
#undef ARCHIVE_CRYPTO_MD5_LIBC

/* MD5 via ARCHIVE_CRYPTO_MD5_LIBSYSTEM supported. */
#undef ARCHIVE_CRYPTO_MD5_LIBSYSTEM

/* MD5 via ARCHIVE_CRYPTO_MD5_MBEDTLS supported. */
#undef ARCHIVE_CRYPTO_MD5_MBEDTLS

/* MD5 via ARCHIVE_CRYPTO_MD5_NETTLE supported. */
#undef ARCHIVE_CRYPTO_MD5_NETTLE

/* MD5 via ARCHIVE_CRYPTO_MD5_OPENSSL supported. */
#undef ARCHIVE_CRYPTO_MD5_OPENSSL

/* MD5 via ARCHIVE_CRYPTO_MD5_WIN supported. */
#undef ARCHIVE_CRYPTO_MD5_WIN

/* RMD160 via ARCHIVE_CRYPTO_RMD160_LIBC supported. */
#undef ARCHIVE_CRYPTO_RMD160_LIBC

/* RMD160 via ARCHIVE_CRYPTO_RMD160_NETTLE supported. */
#undef ARCHIVE_CRYPTO_RMD160_NETTLE

/* RMD160 via ARCHIVE_CRYPTO_RMD160_MBEDTLS supported. */
#undef ARCHIVE_CRYPTO_RMD160_MBEDTLS

/* RMD160 via ARCHIVE_CRYPTO_RMD160_OPENSSL supported. */
#undef ARCHIVE_CRYPTO_RMD160_OPENSSL

/* SHA1 via ARCHIVE_CRYPTO_SHA1_LIBC supported. */
#undef ARCHIVE_CRYPTO_SHA1_LIBC

/* SHA1 via ARCHIVE_CRYPTO_SHA1_LIBSYSTEM supported. */
#undef ARCHIVE_CRYPTO_SHA1_LIBSYSTEM

/* SHA1 via ARCHIVE_CRYPTO_SHA1_MBEDTLS supported. */
#undef ARCHIVE_CRYPTO_SHA1_MBEDTLS

/* SHA1 via ARCHIVE_CRYPTO_SHA1_NETTLE supported. */
#undef ARCHIVE_CRYPTO_SHA1_NETTLE

/* SHA1 via ARCHIVE_CRYPTO_SHA1_OPENSSL supported. */
#undef ARCHIVE_CRYPTO_SHA1_OPENSSL

/* SHA1 via ARCHIVE_CRYPTO_SHA1_WIN supported. */
#undef ARCHIVE_CRYPTO_SHA1_WIN

/* SHA256 via ARCHIVE_CRYPTO_SHA256_LIBC supported. */
#undef ARCHIVE_CRYPTO_SHA256_LIBC

/* SHA256 via ARCHIVE_CRYPTO_SHA256_LIBC2 supported. */
#undef ARCHIVE_CRYPTO_SHA256_LIBC2

/* SHA256 via ARCHIVE_CRYPTO_SHA256_LIBC3 supported. */
#undef ARCHIVE_CRYPTO_SHA256_LIBC3

/* SHA256 via ARCHIVE_CRYPTO_SHA256_LIBSYSTEM supported. */
#undef ARCHIVE_CRYPTO_SHA256_LIBSYSTEM

/* SHA256 via ARCHIVE_CRYPTO_SHA256_MBEDTLS supported. */
#undef ARCHIVE_CRYPTO_SHA256_MBEDTLS

/* SHA256 via ARCHIVE_CRYPTO_SHA256_NETTLE supported. */
#undef ARCHIVE_CRYPTO_SHA256_NETTLE

/* SHA256 via ARCHIVE_CRYPTO_SHA256_OPENSSL supported. */
#undef ARCHIVE_CRYPTO_SHA256_OPENSSL

/* SHA256 via ARCHIVE_CRYPTO_SHA256_WIN supported. */
#undef ARCHIVE_CRYPTO_SHA256_WIN

/* SHA384 via ARCHIVE_CRYPTO_SHA384_LIBC supported. */
#undef ARCHIVE_CRYPTO_SHA384_LIBC

/* SHA384 via ARCHIVE_CRYPTO_SHA384_LIBC2 supported. */
#undef ARCHIVE_CRYPTO_SHA384_LIBC2

/* SHA384 via ARCHIVE_CRYPTO_SHA384_LIBC3 supported. */
#undef ARCHIVE_CRYPTO_SHA384_LIBC3

/* SHA384 via ARCHIVE_CRYPTO_SHA384_LIBSYSTEM supported. */
#undef ARCHIVE_CRYPTO_SHA384_LIBSYSTEM

/* SHA384 via ARCHIVE_CRYPTO_SHA384_MBEDTLS supported. */
#undef ARCHIVE_CRYPTO_SHA384_MBEDTLS

/* SHA384 via ARCHIVE_CRYPTO_SHA384_NETTLE supported. */
#undef ARCHIVE_CRYPTO_SHA384_NETTLE

/* SHA384 via ARCHIVE_CRYPTO_SHA384_OPENSSL supported. */
#undef ARCHIVE_CRYPTO_SHA384_OPENSSL

/* SHA384 via ARCHIVE_CRYPTO_SHA384_WIN supported. */
#undef ARCHIVE_CRYPTO_SHA384_WIN

/* SHA512 via ARCHIVE_CRYPTO_SHA512_LIBC supported. */
#undef ARCHIVE_CRYPTO_SHA512_LIBC

/* SHA512 via ARCHIVE_CRYPTO_SHA512_LIBC2 supported. */
#undef ARCHIVE_CRYPTO_SHA512_LIBC2

/* SHA512 via ARCHIVE_CRYPTO_SHA512_LIBC3 supported. */
#undef ARCHIVE_CRYPTO_SHA512_LIBC3

/* SHA512 via ARCHIVE_CRYPTO_SHA512_LIBSYSTEM supported. */
#undef ARCHIVE_CRYPTO_SHA512_LIBSYSTEM

/* SHA512 via ARCHIVE_CRYPTO_SHA512_MBEDTLS supported. */
#undef ARCHIVE_CRYPTO_SHA512_MBEDTLS

/* SHA512 via ARCHIVE_CRYPTO_SHA512_NETTLE supported. */
#undef ARCHIVE_CRYPTO_SHA512_NETTLE

/* SHA512 via ARCHIVE_CRYPTO_SHA512_OPENSSL supported. */
#undef ARCHIVE_CRYPTO_SHA512_OPENSSL

/* SHA512 via ARCHIVE_CRYPTO_SHA512_WIN supported. */
#undef ARCHIVE_CRYPTO_SHA512_WIN

/* AIX xattr support */
#undef ARCHIVE_XATTR_AIX

/* Darwin xattr support */
#undef ARCHIVE_XATTR_DARWIN

/* FreeBSD xattr support */
#undef ARCHIVE_XATTR_FREEBSD

/* Linux xattr support */
#undef ARCHIVE_XATTR_LINUX

/* Version number of bsdcpio */
#undef BSDCPIO_VERSION_STRING

/* Version number of bsdtar */
#undef BSDTAR_VERSION_STRING

/* Version number of bsdcat */
#undef BSDCAT_VERSION_STRING

/* Version number of bsdunzip */
#undef BSDUNZIP_VERSION_STRING

/* Define to 1 if you have the `acl_create_entry' function. */
#undef HAVE_ACL_CREATE_ENTRY

/* Define to 1 if you have the `acl_get_fd_np' function. */
#undef HAVE_ACL_GET_FD_NP

/* Define to 1 if you have the `acl_get_link' function. */
#undef HAVE_ACL_GET_LINK

/* Define to 1 if you have the `acl_get_link_np' function. */
#undef HAVE_ACL_GET_LINK_NP

/* Define to 1 if you have the `acl_get_perm' function. */
#undef HAVE_ACL_GET_PERM

/* Define to 1 if you have the `acl_get_perm_np' function. */
#undef HAVE_ACL_GET_PERM_NP

/* Define to 1 if you have the `acl_init' function. */
#undef HAVE_ACL_INIT

/* Define to 1 if you have the <acl/libacl.h> header file. */
#undef HAVE_ACL_LIBACL_H

/* Define to 1 if the system has the type `acl_permset_t'. */
#undef HAVE_ACL_PERMSET_T

/* Define to 1 if you have the `acl_set_fd' function. */
#undef HAVE_ACL_SET_FD

/* Define to 1 if you have the `acl_set_fd_np' function. */
#undef HAVE_ACL_SET_FD_NP

/* Define to 1 if you have the `acl_set_file' function. */
#undef HAVE_ACL_SET_FILE

/* Define to 1 if you have the `arc4random_buf' function. */
#undef HAVE_ARC4RANDOM_BUF

/* Define to 1 if you have the <attr/xattr.h> header file. */
#undef HAVE_ATTR_XATTR_H

/* Define to 1 if you have the <bcrypt.h> header file. */
#undef HAVE_BCRYPT_H

/* Define to 1 if you have the <bsdxml.h> header file. */
#undef HAVE_BSDXML_H

/* Define to 1 if you have the <bzlib.h> header file. */
#undef HAVE_BZLIB_H

/* Define to 1 if you have the `chflags' function. */
#undef HAVE_CHFLAGS

/* Define to 1 if you have the `chown' function. */
#undef HAVE_CHOWN

/* Define to 1 if you have the `chroot' function. */
#undef HAVE_CHROOT

/* Define to 1 if you have the <copyfile.h> header file. */
#undef HAVE_COPYFILE_H

/* Define to 1 if you have the `ctime_r' function. */
#undef HAVE_CTIME_R

/* Define to 1 if you have the <ctype.h> header file. */
#define HAVE_CTYPE_H 1

/* Define to 1 if you have the `cygwin_conv_path' function. */
#undef HAVE_CYGWIN_CONV_PATH

/* Define to 1 if you have the declaration of `ACE_GETACL', and to 0 if you
   don't. */
#undef HAVE_DECL_ACE_GETACL

/* Define to 1 if you have the declaration of `ACE_GETACLCNT', and to 0 if you
   don't. */
#undef HAVE_DECL_ACE_GETACLCNT

/* Define to 1 if you have the declaration of `ACE_SETACL', and to 0 if you
   don't. */
#undef HAVE_DECL_ACE_SETACL

/* Define to 1 if you have the declaration of `ACL_SYNCHRONIZE', and to 0 if
   you don't. */
#undef HAVE_DECL_ACL_SYNCHRONIZE

/* Define to 1 if you have the declaration of `ACL_TYPE_EXTENDED', and to 0 if
   you don't. */
#undef HAVE_DECL_ACL_TYPE_EXTENDED

/* Define to 1 if you have the declaration of `ACL_TYPE_NFS4', and to 0 if you
   don't. */
#undef HAVE_DECL_ACL_TYPE_NFS4

/* Define to 1 if you have the declaration of `ACL_USER', and to 0 if you
   don't. */
#undef HAVE_DECL_ACL_USER

/* Define to 1 if you have the declaration of `SETACL', and to 0 if you don't.
   */
#undef HAVE_DECL_SETACL

/* Define to 1 if you have the declaration of `strerror_r', and to 0 if you
   don't. */
#undef HAVE_DECL_STRERROR_R

/* Define to 1 if you have the declaration of `XATTR_NOFOLLOW', and to 0 if
   you don't. */
#undef HAVE_DECL_XATTR_NOFOLLOW

/* Define to 1 if you have the <direct.h> header file. */
#undef HAVE_DIRECT_H

/* Define to 1 if you have the <dirent.h> header file, and it defines `DIR'.
   */
#define HAVE_DIRENT_H 1

/* Define to 1 if you have the `dirfd' function. */
#define HAVE_DIRFD 1

/* Define to 1 if you have the <dlfcn.h> header file. */
#undef HAVE_DLFCN_H

/* Define to 1 if you don't have `vprintf' but do have `_doprnt.' */
#undef HAVE_DOPRNT 

/* Define to 1 if nl_langinfo supports D_MD_ORDER */
#undef HAVE_D_MD_ORDER 

/* A possible errno value for invalid file format errors */
#undef HAVE_EFTYPE 

/* A possible errno value for invalid file format errors */
#undef HAVE_EILSEQ

/* Define to 1 if you have the <errno.h> header file. */
#define HAVE_ERRNO_H 1

/* Define to 1 if you have the <expat.h> header file. */
#undef HAVE_EXPAT_H 

/* Define to 1 if you have the <ext2fs/ext2_fs.h> header file. */
#undef HAVE_EXT2FS_EXT2_FS_H 

/* Define to 1 if you have the `extattr_get_file' function. */
#undef HAVE_EXTATTR_GET_FILE

/* Define to 1 if you have the `extattr_list_file' function. */
#undef HAVE_EXTATTR_LIST_FILE

/* Define to 1 if you have the `extattr_set_fd' function. */
#undef HAVE_EXTATTR_SET_FD 

/* Define to 1 if you have the `extattr_set_file' function. */
#undef HAVE_EXTATTR_SET_FILE

/* Define to 1 if EXTATTR_NAMESPACE_USER is defined in sys/extattr.h. */
#undef HAVE_DECL_EXTATTR_NAMESPACE_USER 

/* Define to 1 if you have the declaration of `GETACL', and to 0 if you don't.
   */
#undef HAVE_DECL_GETACL 

/* Define to 1 if you have the declaration of `GETACLCNT', and to 0 if you
   don't. */
#undef HAVE_DECL_GETACLCNT 

/* Define to 1 if you have the `fchdir' function. */
#define HAVE_FCHDIR 1

/* Define to 1 if you have the `fchflags' function. */
#undef HAVE_FCHFLAGS 

/* Define to 1 if you have the `fchmod' function. */
#undef HAVE_FCHMOD 

/* Define to 1 if you have the `fchown' function. */
#undef HAVE_FCHOWN

/* Define to 1 if you have the `fcntl' function. */
#define HAVE_FCNTL  1 /*pvmk - nope, this is used to gate fcntl.h includes too! */

/* Define to 1 if you have the <fcntl.h> header file. */
#define HAVE_FCNTL_H 1

/* Define to 1 if you have the `fdopendir' function. */
#undef HAVE_FDOPENDIR 

/* Define to 1 if you have the `fgetea' function. */
#undef HAVE_FGETEA 

/* Define to 1 if you have the `fgetxattr' function. */
#undef HAVE_FGETXATTR 

/* Define to 1 if you have the `flistea' function. */
#undef HAVE_FLISTEA 

/* Define to 1 if you have the `flistxattr' function. */
#undef HAVE_FLISTXATTR 

/* Define to 1 if you have the `fnmatch' function. */
#undef HAVE_FNMATCH 

/* Define to 1 if you have the <fnmatch.h> header file. */
#undef HAVE_FNMATCH_H 

/* Define to 1 if you have the `fork' function. */
#undef HAVE_FORK 

/* Define to 1 if fseeko (and presumably ftello) exists and is declared. */
#undef HAVE_FSEEKO 

/* Define to 1 if you have the `fsetea' function. */
#undef HAVE_FSETEA 

/* Define to 1 if you have the `fsetxattr' function. */
#undef HAVE_FSETXATTR 

/* Define to 1 if you have the `fstat' function. */
#undef HAVE_FSTAT 

/* Define to 1 if you have the `fstatat' function. */
#undef HAVE_FSTATAT 

/* Define to 1 if you have the `fstatfs' function. */
#undef HAVE_FSTATFS 

/* Define to 1 if you have the `fstatvfs' function. */
#undef HAVE_FSTATVFS 

/* Define to 1 if you have the `ftruncate' function. */
#undef HAVE_FTRUNCATE

/* Define to 1 if you have the `futimens' function. */
#undef HAVE_FUTIMENS

/* Define to 1 if you have the `futimes' function. */
#undef HAVE_FUTIMES

/* Define to 1 if you have the `futimesat' function. */
#undef HAVE_FUTIMESAT

/* Define to 1 if you have the `getea' function. */
#undef HAVE_GETEA

/* Define to 1 if you have the `geteuid' function. */
#undef HAVE_GETEUID

/* Define to 1 if you have the `getgrgid_r' function. */
#undef HAVE_GETGRGID_R

/* Define to 1 if you have the `getgrnam_r' function. */
#undef HAVE_GETGRNAM_R

/* Define to 1 if you have the `getline' function. */
#undef HAVE_GETLINE

/* Define to 1 if you have the `getpid' function. */
#undef HAVE_GETPID 

/* Define to 1 if you have the `getpwnam_r' function. */
#undef HAVE_GETPWNAM_R

/* Define to 1 if you have the `getpwuid_r' function. */
#undef HAVE_GETPWUID_R

/* Define to 1 if you have the `getvfsbyname' function. */
#undef HAVE_GETVFSBYNAME 

/* Define to 1 if you have the `getxattr' function. */
#undef HAVE_GETXATTR 

/* Define to 1 if you have the `gmtime_r' function. */
#undef HAVE_GMTIME_R 

/* Define to 1 if you have the <grp.h> header file. */
#undef HAVE_GRP_H 

/* Define to 1 if you have the `iconv' function. */
#undef HAVE_ICONV 

/* Define to 1 if you have the <iconv.h> header file. */
#undef HAVE_ICONV_H 

/* Define to 1 if you have the <io.h> header file. */
#undef HAVE_IO_H 

/* Define to 1 if you have the <langinfo.h> header file. */
#undef HAVE_LANGINFO_H 

/* Define to 1 if you have the `lchflags' function. */
#undef HAVE_LCHFLAGS 

/* Define to 1 if you have the `lchmod' function. */
#undef HAVE_LCHMOD 

/* Define to 1 if you have the `lchown' function. */
#undef HAVE_LCHOWN 

/* Define to 1 if you have the `lgetea' function. */
#undef HAVE_LGETEA 

/* Define to 1 if you have the `lgetxattr' function. */
#undef HAVE_LGETXATTR 

/* Define to 1 if you have the `acl' library (-lacl). */
#undef HAVE_LIBACL 

/* Define to 1 if you have the `attr' library (-lattr). */
#undef HAVE_LIBATTR 

/* Define to 1 if you have the `bsdxml' library (-lbsdxml). */
#undef HAVE_LIBBSDXML 

/* Define to 1 if you have the `bz2' library (-lbz2). */
#undef HAVE_LIBBZ2 

/* Define to 1 if you have the `b2' library (-lb2). */
#undef HAVE_LIBB2 

/* Define to 1 if you have the <blake2.h> header file. */
#undef HAVE_BLAKE2_H 

/* Define to 1 if you have the `charset' library (-lcharset). */
#undef HAVE_LIBCHARSET 

/* Define to 1 if you have the `crypto' library (-lcrypto). */
#undef HAVE_LIBCRYPTO 

/* Define to 1 if you have the `expat' library (-lexpat). */
#undef HAVE_LIBEXPAT 

/* Define to 1 if you have the `gcc' library (-lgcc). */
#undef HAVE_LIBGCC 

/* Define to 1 if you have the `lz4' library (-llz4). */
#undef HAVE_LIBLZ4 

/* Define to 1 if you have the `lzma' library (-llzma). */
#undef HAVE_LIBLZMA 

/* Define to 1 if you have the `lzmadec' library (-llzmadec). */
#undef HAVE_LIBLZMADEC 

/* Define to 1 if you have the `lzo2' library (-llzo2). */
#undef HAVE_LIBLZO2 

/* Define to 1 if you have the `mbedcrypto' library (-lmbedcrypto). */
#undef HAVE_LIBMBEDCRYPTO 

/* Define to 1 if you have the `nettle' library (-lnettle). */
#undef HAVE_LIBNETTLE 

/* Define to 1 if you have the `pcre' library (-lpcre). */
#undef HAVE_LIBPCRE 

/* Define to 1 if you have the `pcreposix' library (-lpcreposix). */
#undef HAVE_LIBPCREPOSIX 

/* Define to 1 if you have the `pcre2-8' library (-lpcre2-8). */
#undef HAVE_LIBPCRE2 

/* Define to 1 if you have the `pcreposix' library (-lpcre2posix). */
#undef HAVE_LIBPCRE2POSIX 

/* Define to 1 if you have the `xml2' library (-lxml2). */
#undef HAVE_LIBXML2 

/* Define to 1 if you have the <libxml/xmlreader.h> header file. */
#undef HAVE_LIBXML_XMLREADER_H 

/* Define to 1 if you have the <libxml/xmlwriter.h> header file. */
#undef HAVE_LIBXML_XMLWRITER_H 

/* Define to 1 if you have the `z' library (-lz). */
#undef HAVE_LIBZ 

/* Define to 1 if you have the `zstd' library (-lzstd). */
#undef HAVE_LIBZSTD 

/* Define to 1 if you have the ZSTD_compressStream function. */
#undef HAVE_ZSTD_compressStream 

/* Define to 1 if you have the <limits.h> header file. */
#define HAVE_LIMITS_H 1

/* Define to 1 if you have the `link' function. */
#undef HAVE_LINK 

/* Define to 1 if you have the `linkat' function. */
#undef HAVE_LINKAT

/* Define to 1 if you have the <linux/fiemap.h> header file. */
#undef HAVE_LINUX_FIEMAP_H 

/* Define to 1 if you have the <linux/fs.h> header file. */
#undef HAVE_LINUX_FS_H 

/* Define to 1 if you have the <linux/magic.h> header file. */
#undef HAVE_LINUX_MAGIC_H 

/* Define to 1 if you have the <linux/types.h> header file. */
#undef HAVE_LINUX_TYPES_H 

/* Define to 1 if you have the `listea' function. */
#undef HAVE_LISTEA 

/* Define to 1 if you have the `listxattr' function. */
#undef HAVE_LISTXATTR 

/* Define to 1 if you have the `llistea' function. */
#undef HAVE_LLISTEA 

/* Define to 1 if you have the `llistxattr' function. */
#undef HAVE_LLISTXATTR 

/* Define to 1 if you have the <localcharset.h> header file. */
#undef HAVE_LOCALCHARSET_H 

/* Define to 1 if you have the `locale_charset' function. */
#undef HAVE_LOCALE_CHARSET 

/* Define to 1 if you have the <locale.h> header file. */
#undef HAVE_LOCALE_H 

/* Define to 1 if you have the `localtime_r' function. */
#undef HAVE_LOCALTIME_R 

/* Define to 1 if the system has the type `long long int'. */
#undef HAVE_LONG_LONG_INT 

/* Define to 1 if you have the `lsetea' function. */
#undef HAVE_LSETEA 

/* Define to 1 if you have the `lsetxattr' function. */
#undef HAVE_LSETXATTR 

/* Define to 1 if you have the `lstat' function. */
#undef HAVE_LSTAT 

/* Define to 1 if `lstat' has the bug that it succeeds when given the
   zero-length file name argument. */
#undef HAVE_LSTAT_EMPTY_STRING_BUG 

/* Define to 1 if you have the `lutimes' function. */
#undef HAVE_LUTIMES 

/* Define to 1 if you have the <lz4hc.h> header file. */
#undef HAVE_LZ4HC_H

/* Define to 1 if you have the <lz4.h> header file. */
#undef HAVE_LZ4_H 

/* Define to 1 if you have the <lzmadec.h> header file. */
#undef HAVE_LZMADEC_H 

/* Define to 1 if you have the <lzma.h> header file. */
#undef HAVE_LZMA_H 

/* Define to 1 if you have a working `lzma_stream_encoder_mt' function. */
#undef HAVE_LZMA_STREAM_ENCODER_MT 

/* Define to 1 if you have the <lzo/lzo1x.h> header file. */
#undef HAVE_LZO_LZO1X_H 

/* Define to 1 if you have the <lzo/lzoconf.h> header file. */
#undef HAVE_LZO_LZOCONF_H 

/* Define to 1 if you have the <mbedtls/aes.h> header file. */
#undef HAVE_MBEDTLS_AES_H 

/* Define to 1 if you have the <mbedtls/md.h> header file. */
#undef HAVE_MBEDTLS_MD_H 

/* Define to 1 if you have the <mbedtls/pkcs5.h> header file. */
#undef HAVE_MBEDTLS_PKCS5_H 

/* Define to 1 if you have the `mbrtowc' function. */
#undef HAVE_MBRTOWC 

/* Define to 1 if you have the <membership.h> header file. */
#undef HAVE_MEMBERSHIP_H 

/* Define to 1 if you have the `memmove' function. */
#define HAVE_MEMMOVE 1

/* Define to 1 if you have the <memory.h> header file. */
#undef HAVE_MEMORY_H 

/* Define to 1 if you have the `mkdir' function. */
#undef HAVE_MKDIR

/* Define to 1 if you have the `mkfifo' function. */
#undef HAVE_MKFIFO

/* Define to 1 if you have the `mknod' function. */
#undef HAVE_MKNOD

/* Define to 1 if you have the `mkstemp' function. */
#undef HAVE_MKSTEMP

/* Define to 1 if you have the <ndir.h> header file, and it defines `DIR'. */
#undef HAVE_NDIR_H

/* Define to 1 if you have the <nettle/aes.h> header file. */
#undef HAVE_NETTLE_AES_H

/* Define to 1 if you have the <nettle/hmac.h> header file. */
#undef HAVE_NETTLE_HMAC_H

/* Define to 1 if you have the <nettle/md5.h> header file. */
#undef HAVE_NETTLE_MD5_H

/* Define to 1 if you have the <nettle/pbkdf2.h> header file. */
#undef HAVE_NETTLE_PBKDF2_H

/* Define to 1 if you have the <nettle/ripemd160.h> header file. */
#undef HAVE_NETTLE_RIPEMD160_H

/* Define to 1 if you have the <nettle/sha.h> header file. */
#undef HAVE_NETTLE_SHA_H

/* Define to 1 if you have the `nl_langinfo' function. */
#undef HAVE_NL_LANGINFO

/* Define to 1 if you have the `openat' function. */
#undef HAVE_OPENAT

/* Define to 1 if you have the <openssl/evp.h> header file. */
#undef HAVE_OPENSSL_EVP_H

/* Define to 1 if you have the <paths.h> header file. */
#undef HAVE_PATHS_H

/* Define to 1 if you have the <pcreposix.h> header file. */
#undef HAVE_PCREPOSIX_H

/* Define to 1 if you have the <pcre2posix.h> header file. */
#undef HAVE_PCRE2POSIX_H

/* Define to 1 if you have the `pipe' function. */
#define HAVE_PIPE 1

/* Define to 1 if you have the `PKCS5_PBKDF2_HMAC_SHA1' function. */
#undef HAVE_PKCS5_PBKDF2_HMAC_SHA1

/* Define to 1 if you have the `poll' function. */
#undef HAVE_POLL

/* Define to 1 if you have the <poll.h> header file. */
#undef HAVE_POLL_H

/* Define to 1 if you have the `posix_spawnp' function. */
#define HAVE_POSIX_SPAWNP 1

/* Define to 1 if you have the <process.h> header file. */
#undef HAVE_PROCESS_H

/* Define to 1 if you have the <pthread.h> header file. */
#undef HAVE_PTHREAD_H

/* Define to 1 if you have the <pwd.h> header file. */
#undef HAVE_PWD_H

/* Define to 1 if you have the `readdir_r' function. */
#undef HAVE_READDIR_R

/* Define to 1 if you have the `readlink' function. */
#undef HAVE_READLINK

/* Define to 1 if you have the `readlinkat' function. */
#undef HAVE_READLINKAT

/* Define to 1 if you have the `readpassphrase' function. */
#undef HAVE_READPASSPHRASE

/* Define to 1 if you have the <readpassphrase.h> header file. */
#undef HAVE_READPASSPHRASE_H

/* Define to 1 if you have the <regex.h> header file. */
#undef HAVE_REGEX_H

/* Define to 1 if you have the `select' function. */
#undef HAVE_SELECT

/* Define to 1 if you have the `setenv' function. */
#undef HAVE_SETENV

/* Define to 1 if you have the `setlocale' function. */
#undef HAVE_SETLOCALE

/* Define to 1 if you have the `sigaction' function. */
#undef HAVE_SIGACTION

/* Define to 1 if you have the <signal.h> header file. */
#undef HAVE_SIGNAL_H

/* Define to 1 if you have the <spawn.h> header file. */
#define HAVE_SPAWN_H 1

/* Define to 1 if you have the `statfs' function. */
#undef HAVE_STATFS

/* Define to 1 if you have the `statvfs' function. */
#undef HAVE_STATVFS

/* Define to 1 if `stat' has the bug that it succeeds when given the
   zero-length file name argument. */
#undef HAVE_STAT_EMPTY_STRING_BUG

/* Define to 1 if you have the <stdarg.h> header file. */
#define HAVE_STDARG_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the `strchr' function. */
#define HAVE_STRCHR 1

/* Define to 1 if you have the `strnlen' function. */
#define HAVE_STRNLEN 1

/* Define to 1 if you have the `strdup' function. */
#undef HAVE_STRDUP 

/* Define to 1 if you have the `strerror' function. */
#undef HAVE_STRERROR 

/* Define to 1 if you have the `strerror_r' function. */
#undef HAVE_STRERROR_R 

/* Define to 1 if you have the `strftime' function. */
#undef HAVE_STRFTIME 

/* Define to 1 if you have the <strings.h> header file. */
#undef HAVE_STRINGS_H 

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the `strrchr' function. */
#define HAVE_STRRCHR 1

/* Define to 1 if the system has the type `struct statfs'. */
#undef HAVE_STRUCT_STATFS 

/* Define to 1 if `f_iosize' is a member of `struct statfs'. */
#undef HAVE_STRUCT_STATFS_F_IOSIZE 

/* Define to 1 if `f_namemax' is a member of `struct statfs'. */
#undef HAVE_STRUCT_STATFS_F_NAMEMAX 

/* Define to 1 if `f_iosize' is a member of `struct statvfs'. */
#undef HAVE_STRUCT_STATVFS_F_IOSIZE 

/* Define to 1 if `st_birthtime' is a member of `struct stat'. */
#undef HAVE_STRUCT_STAT_ST_BIRTHTIME 

/* Define to 1 if `st_birthtimespec.tv_nsec' is a member of `struct stat'. */
#undef HAVE_STRUCT_STAT_ST_BIRTHTIMESPEC_TV_NSEC 

/* Define to 1 if `st_blksize' is a member of `struct stat'. */
#undef HAVE_STRUCT_STAT_ST_BLKSIZE 

/* Define to 1 if `st_flags' is a member of `struct stat'. */
#undef HAVE_STRUCT_STAT_ST_FLAGS 

/* Define to 1 if `st_mtimespec.tv_nsec' is a member of `struct stat'. */
#undef HAVE_STRUCT_STAT_ST_MTIMESPEC_TV_NSEC 

/* Define to 1 if `st_mtime_n' is a member of `struct stat'. */
#undef HAVE_STRUCT_STAT_ST_MTIME_N 

/* Define to 1 if `st_mtime_usec' is a member of `struct stat'. */
#undef HAVE_STRUCT_STAT_ST_MTIME_USEC 

/* Define to 1 if `st_mtim.tv_nsec' is a member of `struct stat'. */
#undef HAVE_STRUCT_STAT_ST_MTIM_TV_NSEC 

/* Define to 1 if `st_umtime' is a member of `struct stat'. */
#undef HAVE_STRUCT_STAT_ST_UMTIME 

/* Define to 1 if `tm_gmtoff' is a member of `struct tm'. */
#define HAVE_STRUCT_TM_TM_GMTOFF 1

/* Define to 1 if `__tm_gmtoff' is a member of `struct tm'. */
#undef HAVE_STRUCT_TM___TM_GMTOFF 

/* Define to 1 if you have `struct vfsconf'. */
#undef HAVE_STRUCT_VFSCONF 

/* Define to 1 if you have `struct xvfsconf'. */
#undef HAVE_STRUCT_XVFSCONF 

/* Define to 1 if you have the `symlink' function. */
#undef HAVE_SYMLINK 

/* Define to 1 if you have the `sysconf' function. */
#undef HAVE_SYSCONF1

/* Define to 1 if you have the <sys/acl.h> header file. */
#undef HAVE_SYS_ACL_H 

/* Define to 1 if you have the <sys/cdefs.h> header file. */
#undef HAVE_SYS_CDEFS_H 

/* Define to 1 if you have the <sys/dir.h> header file, and it defines `DIR'.
   */
#undef HAVE_SYS_DIR_H 

/* Define to 1 if you have the <sys/ea.h> header file. */
#undef HAVE_SYS_EA_H 

/* Define to 1 if you have the <sys/extattr.h> header file. */
#undef HAVE_SYS_EXTATTR_H 

/* Define to 1 if you have the <sys/ioctl.h> header file. */
#undef HAVE_SYS_IOCTL_H 

/* Define to 1 if you have the <sys/mkdev.h> header file. */
#undef HAVE_SYS_MKDEV_H 

/* Define to 1 if you have the <sys/mount.h> header file. */
#undef HAVE_SYS_MOUNT_H

/* Define to 1 if you have the <sys/ndir.h> header file, and it defines `DIR'.
   */
#undef HAVE_SYS_NDIR_H

/* Define to 1 if you have the <sys/param.h> header file. */
#undef HAVE_SYS_PARAM_H 

/* Define to 1 if you have the <sys/poll.h> header file. */
#undef HAVE_SYS_POLL_H 

/* Define to 1 if you have the <sys/richacl.h> header file. */
#undef HAVE_SYS_RICHACL_H 

/* Define to 1 if you have the <sys/select.h> header file. */
#undef HAVE_SYS_SELECT_H 

/* Define to 1 if you have the <sys/statfs.h> header file. */
#undef HAVE_SYS_STATFS_H

/* Define to 1 if you have the <sys/statvfs.h> header file. */
#undef HAVE_SYS_STATVFS_H 

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H  1


/* Define to 1 if you have the <sys/sysmacros.h> header file. */
#undef HAVE_SYS_SYSMACROS_H 

/* Define to 1 if you have the <sys/time.h> header file. */
#define HAVE_SYS_TIME_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <sys/utime.h> header file. */
#undef HAVE_SYS_UTIME_H 

/* Define to 1 if you have the <sys/utsname.h> header file. */
#undef HAVE_SYS_UTSNAME_H 

/* Define to 1 if you have the <sys/vfs.h> header file. */
#undef HAVE_SYS_VFS_H 

/* Define to 1 if you have <sys/wait.h> that is POSIX.1 compatible. */
#define HAVE_SYS_WAIT_H  1

/* Define to 1 if you have the <sys/xattr.h> header file. */
#undef HAVE_SYS_XATTR_H 

/* Define to 1 if you have the `timegm' function. */
#undef HAVE_TIMEGM 

/* Define to 1 if you have the <time.h> header file. */
#undef HAVE_TIME_H 

/* Define to 1 if you have the `tzset' function. */
#undef HAVE_TZSET 

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to 1 if you have the `unlinkat' function. */
#undef HAVE_UNLINKAT 

/* Define to 1 if you have the `unsetenv' function. */
#undef HAVE_UNSETENV 

/* Define to 1 if the system has the type `unsigned long long'. */
#undef HAVE_UNSIGNED_LONG_LONG 

/* Define to 1 if the system has the type `unsigned long long int'. */
#undef HAVE_UNSIGNED_LONG_LONG_INT 

/* Define to 1 if you have the `utime' function. */
#undef HAVE_UTIME 

/* Define to 1 if you have the `utimensat' function. */
#undef HAVE_UTIMENSAT 

/* Define to 1 if you have the `utimes' function. */
#undef HAVE_UTIMES 

/* Define to 1 if you have the <utime.h> header file. */
#undef HAVE_UTIME_H 

/* Define to 1 if you have the `vfork' function. */
#undef HAVE_VFORK 

/* Define to 1 if you have the `vprintf' function. */
#define HAVE_VPRINTF 1

/* Define to 1 if you have the <wchar.h> header file. */
#define HAVE_WCHAR_H 1

/* Define to 1 if the system has the type `wchar_t'. */
#define HAVE_WCHAR_T  1

/* Define to 1 if you have the `wcrtomb' function. */
#define HAVE_WCRTOMB  1

/* Define to 1 if you have the `wcscmp' function. */
#define HAVE_WCSCMP  1

/* Define to 1 if you have the `wcscpy' function. */
#define HAVE_WCSCPY  1

/* Define to 1 if you have the `wcslen' function. */
#define HAVE_WCSLEN  1

/* Define to 1 if you have the `wctomb' function. */
#define HAVE_WCTOMB 1

/* Define to 1 if you have the <wctype.h> header file. */
#define HAVE_WCTYPE_H 1

/* Define to 1 if you have the <wincrypt.h> header file. */
#undef HAVE_WINCRYPT_H 

/* Define to 1 if you have the <windows.h> header file. */
#undef HAVE_WINDOWS_H 

/* Define to 1 if you have the <winioctl.h> header file. */
#undef HAVE_WINIOCTL_H 

/* Define to 1 if you have _CrtSetReportMode in <crtdbg.h>  */
#undef HAVE__CrtSetReportMode 

/* Define to 1 if you have the `wmemcmp' function. */
#define HAVE_WMEMCMP 1

/* Define to 1 if you have the `wmemcpy' function. */
#define HAVE_WMEMCPY 1

/* Define to 1 if you have the `wmemmove' function. */
#define HAVE_WMEMMOVE 1

/* Define to 1 if you have a working EXT2_IOC_GETFLAGS */
#undef HAVE_WORKING_EXT2_IOC_GETFLAGS 

/* Define to 1 if you have a working FS_IOC_GETFLAGS */
#undef HAVE_WORKING_FS_IOC_GETFLAGS 

/* Define to 1 if you have the <zlib.h> header file. */
#undef HAVE_ZLIB_H 

/* Define to 1 if you have the <zstd.h> header file. */
#undef HAVE_ZSTD_H 

/* Define to 1 if you have the `ctime_s' function. */
#undef HAVE_CTIME_S 

/* Define to 1 if you have the `_fseeki64' function. */
#undef HAVE__FSEEKI64 

/* Define to 1 if you have the `_get_timezone' function. */
#undef HAVE__GET_TIMEZONE 

/* Define to 1 if you have the `gmtime_s' function. */
#undef HAVE_GMTIME_S 

/* Define to 1 if you have the `localtime_s' function. */
#undef HAVE_LOCALTIME_S 

/* Define to 1 if you have the `_mkgmtime' function. */
#undef HAVE__MKGMTIME 

/* Define as const if the declaration of iconv() needs const. */
#define ICONV_CONST 

/* Version number of libarchive as a single integer */
#undef LIBARCHIVE_VERSION_NUMBER 

/* Version number of libarchive */
#undef LIBARCHIVE_VERSION_STRING 

/* Define to 1 if `lstat' dereferences a symlink specified with a trailing
   slash. */
#undef LSTAT_FOLLOWS_SLASHED_SYMLINK 

/* Define to 1 if `major', `minor', and `makedev' are declared in <mkdev.h>.
   */
#undef MAJOR_IN_MKDEV 

/* Define to 1 if `major', `minor', and `makedev' are declared in
   <sysmacros.h>. */
#undef MAJOR_IN_SYSMACROS 

/* Define to 1 if your C compiler doesn't accept -c and -o together. */
#undef NO_MINUS_C_MINUS_O 

/* The size of `wchar_t', as computed by sizeof. */
#define SIZEOF_WCHAR_T 2

/* Define to 1 if strerror_r returns char *. */
#undef STRERROR_R_CHAR_P 

/* Define to 1 if you can safely include both <sys/time.h> and <time.h>. */
#undef TIME_WITH_SYS_TIME 

/*
 * Some platform requires a macro to use extension functions.
 */
#define SAFE_TO_DEFINE_EXTENSIONS 1
#ifdef SAFE_TO_DEFINE_EXTENSIONS
/* Enable extensions on AIX 3, Interix.  */
#ifndef _ALL_SOURCE
# define _ALL_SOURCE 1
#endif
/* Enable GNU extensions on systems that have them.  */
#ifndef _GNU_SOURCE
# define _GNU_SOURCE 1
#endif
/* Enable threading extensions on Solaris.  */
#ifndef _POSIX_PTHREAD_SEMANTICS
# define _POSIX_PTHREAD_SEMANTICS 1
#endif
/* Enable extensions on HP NonStop.  */
#ifndef _TANDEM_SOURCE
# define _TANDEM_SOURCE 1
#endif
/* Enable general extensions on Solaris.  */
#ifndef __EXTENSIONS__
# define __EXTENSIONS__ 1
#endif
#endif /* SAFE_TO_DEFINE_EXTENSIONS */

/* Version number of package */
#define VERSION "4.0.0-rc2-pvmk"

/* Number of bits in a file offset, on hosts where this is settable. */
#define _FILE_OFFSET_BITS 64

/* Define to 1 to make fseeko visible on some hosts (e.g. glibc 2.2). */
#define _LARGEFILE_SOURCE 1

/* Define for large files, on AIX-style hosts. */
#define _LARGE_FILES 1

/* Define to control Windows SDK version */
#ifndef NTDDI_VERSION
#define NTDDI_VERSION 0
#endif // NTDDI_VERSION

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0
#endif // _WIN32_WINNT

#ifndef WINVER
#define WINVER 0
#endif // WINVER

/* Define to empty if `const' does not conform to ANSI C. */
//#define const const@

/* Define to `int' if <sys/types.h> doesn't define. */
#undef gid_t

/* Define to `unsigned long' if <sys/types.h> does not define. */
#undef id_t

/* Define to `int' if <sys/types.h> does not define. */
#undef mode_t

/* Define to `long long' if <sys/types.h> does not define. */
#undef off_t

/* Define to `int' if <sys/types.h> doesn't define. */
#undef pid_t

/* Define to `unsigned int' if <sys/types.h> does not define. */
#undef size_t

/* Define to `int' if <sys/types.h> does not define. */
#undef ssize_t

/* Define to `int' if <sys/types.h> doesn't define. */
#undef uid_t

#include <cm3p/kwiml/int.h>

#ifndef KWIML_INT_HAVE_INT64_T
typedef KWIML_INT_int64_t int64_t;
#endif
#ifndef KWIML_INT_HAVE_INT32_T
typedef KWIML_INT_int32_t int32_t;
#endif
#ifndef KWIML_INT_HAVE_INT16_T
typedef KWIML_INT_int16_t int16_t;
#endif
#ifndef KWIML_INT_HAVE_INT8_T
typedef KWIML_INT_int8_t int8_t;
#endif
#ifndef KWIML_INT_HAVE_INTPTR_T
typedef KWIML_INT_intptr_t intptr_t;
#endif
#ifndef KWIML_INT_HAVE_UINT64_T
typedef KWIML_INT_uint64_t uint64_t;
#endif
#ifndef KWIML_INT_HAVE_UINT32_T
typedef KWIML_INT_uint32_t uint32_t;
#endif
#ifndef KWIML_INT_HAVE_UINT16_T
typedef KWIML_INT_uint16_t uint16_t;
#endif
#ifndef KWIML_INT_HAVE_UINT8_T
typedef KWIML_INT_uint8_t uint8_t;
#endif
#ifndef KWIML_INT_HAVE_UINTPTR_T
typedef KWIML_INT_uintptr_t uintptr_t;
#endif

/* Define to 1 if you have the <stdint.h> header file. */
#ifdef KWIML_INT_HAVE_STDINT_H
# define HAVE_STDINT_H 1
#endif

/* Define to 1 if you have the <inttypes.h> header file. */
#ifdef KWIML_INT_HAVE_INTTYPES_H
# define HAVE_INTTYPES_H 1
#endif


