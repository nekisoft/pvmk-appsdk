/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
 * are also available at https://curl.se/docs/copyright.html.
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 * SPDX-License-Identifier: curl
 *
 ***************************************************************************/

#include <cm3p/kwiml/abi.h>

#define BUILDING_LIBCURL 1

/* Default SSL backend */
#define CURL_DEFAULT_SSL_BACKEND ""

/* disables alt-svc */
#undef CURL_DISABLE_ALTSVC 

/* disables cookies support */
#undef CURL_DISABLE_COOKIES 

/* disables Basic authentication */
#undef CURL_DISABLE_BASIC_AUTH 

/* disables Bearer authentication */
#undef CURL_DISABLE_BEARER_AUTH 

/* disables Digest authentication */
#undef CURL_DISABLE_DIGEST_AUTH 

/* disables Kerberos authentication */
#undef CURL_DISABLE_KERBEROS_AUTH 

/* disables negotiate authentication */
#undef CURL_DISABLE_NEGOTIATE_AUTH 

/* disables aws-sigv4 */
#undef CURL_DISABLE_AWS 

/* disables DICT */
#undef CURL_DISABLE_DICT

/* disables DNS-over-HTTPS */
#undef CURL_DISABLE_DOH 

/* disables FILE */
#undef CURL_DISABLE_FILE 

/* disables form api */
#undef CURL_DISABLE_FORM_API

/* disables FTP */
#undef CURL_DISABLE_FTP 

/* disables curl_easy_options API for existing options to curl_easy_setopt */
#undef CURL_DISABLE_GETOPTIONS

/* disables GOPHER */
#undef CURL_DISABLE_GOPHER

/* disables headers-api support */
#undef CURL_DISABLE_HEADERS_API

/* disables HSTS support */
#undef CURL_DISABLE_HSTS 

/* disables HTTP */
#undef CURL_DISABLE_HTTP

/* disabled all HTTP authentication methods */
#undef CURL_DISABLE_HTTP_AUTH 

/* disables IMAP */
#undef CURL_DISABLE_IMAP 

/* disables LDAP */
#define CURL_DISABLE_LDAP  1

/* disables LDAPS */
#undef CURL_DISABLE_LDAPS 

/* disables --libcurl option from the curl tool */
#undef CURL_DISABLE_LIBCURL_OPTION 

/* disables MIME support */
#undef CURL_DISABLE_MIME 

/* disables local binding support */
#undef CURL_DISABLE_BINDLOCAL 

/* disables MQTT */
#undef CURL_DISABLE_MQTT 

/* disables netrc parser */
#undef CURL_DISABLE_NETRC 

/* disables NTLM support */
#undef CURL_DISABLE_NTLM 

/* disables date parsing */
#undef CURL_DISABLE_PARSEDATE 

/* disables POP3 */
#undef CURL_DISABLE_POP3 

/* disables built-in progress meter */
#undef CURL_DISABLE_PROGRESS_METER 

/* disables proxies */
#undef CURL_DISABLE_PROXY 

/* disables IPFS from the curl tool */
#undef CURL_DISABLE_IPFS 

/* disables RTSP */
#undef CURL_DISABLE_RTSP 

/* disables SHA-52/256 hash algorithm */
#undef CURL_DISABLE_SHA52_256 

/* disabled shuffle DNS feature */
#undef CURL_DISABLE_SHUFFLE_DNS 

/* disables SMB */
#undef CURL_DISABLE_SMB 

/* disables SMTP */
#undef CURL_DISABLE_SMTP 

/* disabled WebSockets */
#undef CURL_DISABLE_WEBSOCKETS 

/* disables use of socketpair for curl_multi_poll */
#undef CURL_DISABLE_SOCKETPAIR 

/* disables TELNET */
#undef CURL_DISABLE_TELNET 

/* disables TFTP */
#undef CURL_DISABLE_TFTP 

/* disables verbose strings */
#undef CURL_DISABLE_VERBOSE_STRINGS 

/* disables unsafe CA bundle search on Windows from the curl tool */
#undef CURL_DISABLE_CA_SEARCH 

/* safe CA bundle search (within the curl tool directory) on Windows */
#undef CURL_CA_SEARCH_SAFE 

/* to make a symbol visible */
#undef CURL_EXTERN_SYMBOL 
/* Ensure using CURL_EXTERN_SYMBOL is possible */
#ifndef CURL_EXTERN_SYMBOL
#define CURL_EXTERN_SYMBOL
#endif

/* Allow SMB to work on Windows */
#undef USE_WIN32_CRYPTO 

/* Use Windows LDAP implementation */
#undef USE_WIN32_LDAP 

/* Define if you want to enable IPv6 support */
#undef USE_IPV6 

/* Define to  if you have the alarm function. */
#define HAVE_ALARM 1

/* Define to  if you have the arc4random function. */
#undef HAVE_ARC4RANDOM 

/* Define to  if you have the <arpa/inet.h> header file. */
#define HAVE_ARPA_INET_H 1

/* Define to  if you have _Atomic support. */
#undef HAVE_ATOMIC 

/* Define to  if you have the `fnmatch' function. */
#undef HAVE_FNMATCH 

/* Define to  if you have the `basename' function. */
#undef HAVE_BASENAME 

/* Define to  if bool is an available type. */
#define HAVE_BOOL_T  1

/* Define to  if you have the __builtin_available function. */
#undef HAVE_BUILTIN_AVAILABLE 

/* Define to  if you have the clock_gettime function and monotonic timer. */
#undef HAVE_CLOCK_GETTIME_MONOTONIC 

/* Define to  if you have the clock_gettime function and raw monotonic timer.
   */
#undef HAVE_CLOCK_GETTIME_MONOTONIC_RAW 

/* Define to  if you have the `closesocket' function. */
#undef HAVE_CLOSESOCKET 

/* Define to  if you have the `CloseSocket' function. */
#undef HAVE_CLOSESOCKET_CAMEL 

/* Define to  if you have the <dirent.h> header file. */
#define HAVE_DIRENT_H  1

/* Define to  if you have the `opendir' function. */
#define HAVE_OPENDIR 1

/* Define to  if you have the fcntl function. */
#undef HAVE_FCNTL 

/* Define to  if you have the <fcntl.h> header file. */
#define HAVE_FCNTL_H 1

/* Define to  if you have a working fcntl O_NONBLOCK function. */
#define HAVE_FCNTL_O_NONBLOCK 1

/* Define to  if you have the freeaddrinfo function. */
#undef HAVE_FREEADDRINFO 

/* Define to  if you have the fseeko function. */
#undef HAVE_FSEEKO 

/* Define to  if you have the fseeko declaration. */
#undef HAVE_DECL_FSEEKO 

/* Define to  if you have the ftruncate function. */
#undef HAVE_FTRUNCATE 

/* Define to  if you have a working getaddrinfo function. */
#undef HAVE_GETADDRINFO 

/* Define to  if the getaddrinfo function is threadsafe. */
#undef HAVE_GETADDRINFO_THREADSAFE 

/* Define to  if you have the `geteuid' function. */
#undef HAVE_GETEUID 

/* Define to  if you have the `getppid' function. */
#undef HAVE_GETPPID 

/* Define to  if you have the gethostbyname_r function. */
#undef HAVE_GETHOSTBYNAME_R 

/* gethostbyname_r() takes 3 args */
#undef HAVE_GETHOSTBYNAME_R_3 

/* gethostbyname_r() takes 5 args */
#undef HAVE_GETHOSTBYNAME_R_5 

/* gethostbyname_r() takes 6 args */
#undef HAVE_GETHOSTBYNAME_R_6 

/* Define to  if you have the gethostname function. */
#undef HAVE_GETHOSTNAME 

/* Define to  if you have a working getifaddrs function. */
#undef HAVE_GETIFADDRS 

/* Define to  if you have the `getpass_r' function. */
#undef HAVE_GETPASS_R 

/* Define to  if you have the `getpeername' function. */
#undef HAVE_GETPEERNAME 

/* Define to  if you have the `getsockname' function. */
#undef HAVE_GETSOCKNAME 

/* Define to  if you have the `if_nametoindex' function. */
#undef HAVE_IF_NAMETOINDEX 

/* Define to  if you have the `getpwuid' function. */
#undef HAVE_GETPWUID 

/* Define to  if you have the `getpwuid_r' function. */
#undef HAVE_GETPWUID_R 

/* Define to  if you have the `getrlimit' function. */
#undef HAVE_GETRLIMIT 

/* Define to  if you have the `gettimeofday' function. */
#undef HAVE_GETTIMEOFDAY 

/* Define to  if you have a working glibc-style strerror_r function. */
#undef HAVE_GLIBC_STRERROR_R 

/* Define to  if you have a working gmtime_r function. */
#undef HAVE_GMTIME_R 

/* if you have the gssapi libraries */
#undef HAVE_GSSAPI 

/* Define to  if you have the <gssapi/gssapi_generic.h> header file. */
#undef HAVE_GSSAPI_GSSAPI_GENERIC_H 

/* Define to  if you have the <gssapi/gssapi.h> header file. */
#undef HAVE_GSSAPI_GSSAPI_H 

/* if you have the GNU gssapi libraries */
#undef HAVE_GSSGNU 

/* Define to  if you have the <ifaddrs.h> header file. */
#undef HAVE_IFADDRS_H 

/* Define to  if you have a IPv6 capable working inet_ntop function. */
#undef HAVE_INET_NTOP 

/* Define to  if you have a IPv6 capable working inet_pton function. */
#undef HAVE_INET_PTON 

/* Define to  if symbol `sa_family_t' exists */
#undef HAVE_SA_FAMILY_T 

/* Define to  if symbol `ADDRESS_FAMILY' exists */
#undef HAVE_ADDRESS_FAMILY 

/* Define to  if you have the ioctlsocket function. */
#undef HAVE_IOCTLSOCKET 

/* Define to  if you have the IoctlSocket camel case function. */
#undef HAVE_IOCTLSOCKET_CAMEL 

/* Define to  if you have a working IoctlSocket camel case FIONBIO function.
   */
#undef HAVE_IOCTLSOCKET_CAMEL_FIONBIO 

/* Define to  if you have a working ioctlsocket FIONBIO function. */
#undef HAVE_IOCTLSOCKET_FIONBIO 

/* Define to  if you have a working ioctl FIONBIO function. */
#undef HAVE_IOCTL_FIONBIO 

/* Define to  if you have a working ioctl SIOCGIFADDR function. */
#undef HAVE_IOCTL_SIOCGIFADDR 

/* Define to  if you have the <io.h> header file. */
#undef HAVE_IO_H 

/* Define to  if you have the lber.h header file. */
#undef HAVE_LBER_H 

/* Use LDAPS implementation */
#undef HAVE_LDAP_SSL 

/* Define to  if you have the ldap_ssl.h header file. */
#undef HAVE_LDAP_SSL_H 

/* Define to  if you have the `ldap_url_parse' function. */
#undef HAVE_LDAP_URL_PARSE 

/* Define to  if you have the <libgen.h> header file. */
#undef HAVE_LIBGEN_H 

/* Define to  if you have the `idn2' library (-lidn2). */
#undef HAVE_LIBIDN2 

/* Define to  if you have the idn2.h header file. */
#undef HAVE_IDN2_H 

/* if zlib is available */
#undef HAVE_LIBZ 

/* if brotli is available */
#undef HAVE_BROTLI 

/* if zstd is available */
#undef HAVE_ZSTD 

/* Define to  if you have the <locale.h> header file. */
#undef HAVE_LOCALE_H 

/* Define to  if the compiler supports the 'long long' data type. */
#if KWIML_ABI_SIZEOF_LONG_LONG
#  define HAVE_LONGLONG 
#endif

/* Define to  if you have the 'suseconds_t' data type. */
#undef HAVE_SUSECONDS_T 

/* Define to  if you have the MSG_NOSIGNAL flag. */
#undef HAVE_MSG_NOSIGNAL 

/* Define to  if you have the <netdb.h> header file. */
#define HAVE_NETDB_H 1

/* Define to  if you have the <netinet/in.h> header file. */
#define HAVE_NETINET_IN_H 1

/* Define to  if you have the <netinet/in6.h> header file. */
#undef HAVE_NETINET_IN6_H 

/* Define to  if you have the <netinet/tcp.h> header file. */
#undef HAVE_NETINET_TCP_H 

/* Define to  if you have the <netinet/udp.h> header file. */
#undef HAVE_NETINET_UDP_H 

/* Define to  if you have the <linux/tcp.h> header file. */
#undef HAVE_LINUX_TCP_H 

/* Define to  if you have the <net/if.h> header file. */
#undef HAVE_NET_IF_H 

/* if you have an old MIT gssapi library, lacking GSS_C_NT_HOSTBASED_SERVICE */
#undef HAVE_OLD_GSSMIT 

/* Define to  if you have the `pipe' function. */
#undef HAVE_PIPE 

/* Define to  if you have the `eventfd' function. */
#undef HAVE_EVENTFD 

/* If you have poll */
#undef HAVE_POLL 

/* If you have realpath */
#undef HAVE_REALPATH 

/* Define to  if you have the <poll.h> header file. */
#undef HAVE_POLL_H 

/* Define to  if you have a working POSIX-style strerror_r function. */
#undef HAVE_POSIX_STRERROR_R 

/* Define to  if you have the <pthread.h> header file */
#undef HAVE_PTHREAD_H 

/* Define to  if you have the <pwd.h> header file. */
#undef HAVE_PWD_H 

/* Define to  if OpenSSL has the `SSL_set0_wbio` function. */
#undef HAVE_SSL_SET0_WBIO 

/* Define to  if you have the recv function. */
#define HAVE_RECV 1

/* Define to  if you have the select function. */
#define HAVE_SELECT 1

/* Define to  if you have the sched_yield function. */
#undef HAVE_SCHED_YIELD 

/* Define to  if you have the send function. */
#define HAVE_SEND 1

/* Define to  if you have the sendmsg function. */
#undef HAVE_SENDMSG 

/* Define to  if you have the sendmmsg function. */
#undef HAVE_SENDMMSG 

/* Define to  if you have the 'fsetxattr' function. */
#undef HAVE_FSETXATTR 

/* fsetxattr() takes 5 args */
#undef HAVE_FSETXATTR_5 

/* fsetxattr() takes 6 args */
#undef HAVE_FSETXATTR_6 

/* Define to  if you have the `setlocale' function. */
#undef HAVE_SETLOCALE 

/* Define to  if you have the `setmode' function. */
#undef HAVE_SETMODE 

/* Define to  if you have the `_setmode' function. */
#undef HAVE__SETMODE 

/* Define to  if you have the `setrlimit' function. */
#undef HAVE_SETRLIMIT 

/* Define to  if you have a working setsockopt SO_NONBLOCK function. */
#undef HAVE_SETSOCKOPT_SO_NONBLOCK 

/* Define to  if you have the sigaction function. */
#undef HAVE_SIGACTION 

/* Define to  if you have the siginterrupt function. */
#undef HAVE_SIGINTERRUPT 

/* Define to  if you have the signal function. */
#undef HAVE_SIGNAL 

/* Define to  if you have the sigsetjmp function or macro. */
#undef HAVE_SIGSETJMP 

/* Define to  if you have the `snprintf' function. */
#undef HAVE_SNPRINTF 

/* Define to  if struct sockaddr_in6 has the sin6_scope_id member */
#undef HAVE_SOCKADDR_IN6_SIN6_SCOPE_ID 

/* Define to  if you have the `socket' function. */
#define HAVE_SOCKET 1

/* Define to  if you have the <proto/bsdsocket.h> header file. */
#undef HAVE_PROTO_BSDSOCKET_H 

/* Define to  if you have the socketpair function. */
#undef HAVE_SOCKETPAIR 

/* Define to  if you have the <stdatomic.h> header file. */
#undef HAVE_STDATOMIC_H 

/* Define to  if you have the <stdbool.h> header file. */
#define HAVE_STDBOOL_H 1

/* Define to  if you have the strcasecmp function. */
#undef HAVE_STRCASECMP 

/* Define to  if you have the strcmpi function. */
#undef HAVE_STRCMPI 

/* Define to  if you have the strdup function. */
#undef HAVE_STRDUP 

/* Define to  if you have the strerror_r function. */
#undef HAVE_STRERROR_R 

/* Define to  if you have the stricmp function. */
#undef HAVE_STRICMP 

/* Define to  if you have the <strings.h> header file. */
#undef HAVE_STRINGS_H 

/* Define to  if you have the <stropts.h> header file. */
#undef HAVE_STROPTS_H 

/* Define to  if you have the strtok_r function. */
#undef HAVE_STRTOK_R 

/* Define to  if you have the strtoll function. */
#undef HAVE_STRTOLL 

/* Define to  if you have the memrchr function. */
#undef HAVE_MEMRCHR 

/* if struct sockaddr_storage is defined */
#undef HAVE_STRUCT_SOCKADDR_STORAGE 

/* Define to  if you have the timeval struct. */
#define HAVE_STRUCT_TIMEVAL 1

/* Define to  if you have the <sys/eventfd.h> header file. */
#undef HAVE_SYS_EVENTFD_H 

/* Define to  if you have the <sys/filio.h> header file. */
#undef HAVE_SYS_FILIO_H 

/* Define to  if you have the <sys/ioctl.h> header file. */
#undef HAVE_SYS_IOCTL_H 

/* Define to  if you have the <sys/param.h> header file. */
#undef HAVE_SYS_PARAM_H 

/* Define to  if you have the <sys/poll.h> header file. */
#undef HAVE_SYS_POLL_H 

/* Define to  if you have the <sys/resource.h> header file. */
#undef HAVE_SYS_RESOURCE_H 

/* Define to  if you have the <sys/select.h> header file. */
#undef HAVE_SYS_SELECT_H 

/* Define to  if you have the <sys/socket.h> header file. */
#undef HAVE_SYS_SOCKET_H 

/* Define to  if you have the <sys/sockio.h> header file. */
#undef HAVE_SYS_SOCKIO_H 

/* Define to  if you have the <sys/stat.h> header file. */
#undef HAVE_SYS_STAT_H 

/* Define to  if you have the <sys/time.h> header file. */
#undef HAVE_SYS_TIME_H 

/* Define to  if you have the <sys/types.h> header file. */
#undef HAVE_SYS_TYPES_H 

/* Define to  if you have the <sys/un.h> header file. */
#undef HAVE_SYS_UN_H 

/* Define to  if you have the <sys/utime.h> header file. */
#undef HAVE_SYS_UTIME_H 

/* Define to  if you have the <termios.h> header file. */
#undef HAVE_TERMIOS_H 

/* Define to  if you have the <termio.h> header file. */
#undef HAVE_TERMIO_H 

/* Define to  if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to  if you have the `utime' function. */
#undef HAVE_UTIME 

/* Define to  if you have the `utimes' function. */
#undef HAVE_UTIMES 

/* Define to  if you have the <utime.h> header file. */
#undef HAVE_UTIME_H 

/* Define this symbol if your OS supports changing the contents of argv */
#undef HAVE_WRITABLE_ARGV 

/* Define this if time_t is unsigned */
#undef HAVE_TIME_T_UNSIGNED 

/* Define to  if _REENTRANT preprocessor symbol must be defined. */
#undef NEED_REENTRANT 

/* cpu-machine-OS */
#define CURL_OS "unknown"

/*
 Note: SIZEOF_* variables are fetched with CMake through check_type_size().
 As per CMake documentation on CheckTypeSize, C preprocessor code is
 generated by CMake into SIZEOF_*_CODE. This is what we use in the
 following statements.

 Reference: https://cmake.org/cmake/help/latest/module/CheckTypeSize.html
*/

/* The size of `int', as computed by sizeof. */
#define SIZEOF_INT KWIML_ABI_SIZEOF_INT

/* The size of `short', as computed by sizeof. */
#define SIZEOF_SHORT KWIML_ABI_SIZEOF_SHORT

/* The size of `long', as computed by sizeof. */
#define SIZEOF_LONG KWIML_ABI_SIZEOF_LONG

/* The size of `long long', as computed by sizeof. */
#define SIZEOF_LONG_LONG KWIML_ABI_SIZEOF_LONG_LONG

/* The size of `__int64', as computed by sizeof. */
#if KWIML_ABI_SIZEOF___INT64
#  define SIZEOF___INT64 KWIML_ABI_SIZEOF___INT64
#endif

/* The size of `long long', as computed by sizeof. */
/* The size of `off_t', as computed by sizeof. */
/* The size of `curl_off_t', as computed by sizeof. */
/* The size of `curl_socket_t', as computed by sizeof. */
/* The size of `size_t', as computed by sizeof. */
/* The size of `ssize_t', as computed by sizeof. */
/* The size of `time_t', as computed by sizeof. */

/* Define to  if you have the ANSI C header files. */
#define STDC_HEADERS  1

/* Define if you want to enable c-ares support */
#undef USE_ARES 

/* Define if you want to enable POSIX threaded DNS lookup */
#undef USE_THREADS_POSIX 

/* Define if you want to enable Win32 threaded DNS lookup */
#undef USE_THREADS_WIN32 

/* if GnuTLS is enabled */
#undef USE_GNUTLS 

/* if Secure Transport is enabled */
#undef USE_SECTRANSP 

/* if SSL session export support is available */
#undef USE_SSLS_EXPORT 

/* if mbedTLS is enabled */
#undef USE_MBEDTLS 

/* if BearSSL is enabled */
#undef USE_BEARSSL 

/* if Rustls is enabled */
#undef USE_RUSTLS 

/* if wolfSSL is enabled */
#undef USE_WOLFSSL 

/* if wolfSSL has the wolfSSL_DES_ecb_encrypt function. */
#undef HAVE_WOLFSSL_DES_ECB_ENCRYPT 

/* if wolfSSL has the wolfSSL_BIO_new function. */
#undef HAVE_WOLFSSL_BIO 

/* if wolfSSL has the wolfSSL_BIO_set_shutdown function. */
#undef HAVE_WOLFSSL_FULL_BIO 

/* if libssh is in use */
#undef USE_LIBSSH 

/* if libssh2 is in use */
#undef USE_LIBSSH2 

/* if wolfssh is in use */
#undef USE_WOLFSSH 

/* if libpsl is in use */
#undef USE_LIBPSL 

/* if you want to use OpenLDAP code instead of legacy ldap implementation */
#undef USE_OPENLDAP 

/* if OpenSSL is in use */
#undef USE_OPENSSL 

/* if AmiSSL is in use */
#undef USE_AMISSL 

/* if librtmp/rtmpdump is in use */
#undef USE_LIBRTMP 

/* if GSASL is in use */
#undef USE_GSASL 

/* if libuv is in use */
#undef USE_LIBUV 

/* Define to  if you have the <uv.h> header file. */
#undef HAVE_UV_H 

/* Define to  if you do not want the OpenSSL configuration to be loaded
   automatically */
#undef CURL_DISABLE_OPENSSL_AUTO_LOAD_CONFIG 

/* to enable NGHTTP2  */
#undef USE_NGHTTP2 

/* to enable NGTCP2 */
#undef USE_NGTCP2 

/* to enable NGHTTP3  */
#undef USE_NGHTTP3 

/* to enable quiche */
#undef USE_QUICHE 

/* to enable openssl + nghttp3 */
#undef USE_OPENSSL_QUIC 

/* Define to  if you have the quiche_conn_set_qlog_fd function. */
#undef HAVE_QUICHE_CONN_SET_QLOG_FD 

/* to enable msh3 */
#undef USE_MSH3 

/* if Unix domain sockets are enabled  */
#undef USE_UNIX_SOCKETS 

/* to enable SSPI support */
#undef USE_WINDOWS_SSPI 

/* to enable Windows SSL  */
#undef USE_SCHANNEL 

/* if Watt-32 is in use */
#undef USE_WATT32 

/* enable multiple SSL backends */
#undef CURL_WITH_MULTI_SSL 

/* Number of bits in a file offset, on hosts where this is settable. */
#define _FILE_OFFSET_BITS 64

/* Define for large files, on AIX-style hosts. */
#define _LARGE_FILES 1

/* define this if you need it to compile thread-safe code */
#undef _THREAD_SAFE

/* Define to empty if `const' does not conform to ANSI C. */
#undef const 

/* Type to use in place of in_addr_t when system does not provide it. */
#undef in_addr_t

/* Define to `unsigned int' if <sys/types.h> does not define. */
#undef size_t 

/* Define to  if you have the mach_absolute_time function. */
#undef HAVE_MACH_ABSOLUTE_TIME 

/* to enable Windows IDN */
#undef USE_WIN32_IDN 

/* to enable Apple IDN */
#undef USE_APPLE_IDN 

/* Define to  if OpenSSL has the SSL_CTX_set_srp_username function. */
#undef HAVE_OPENSSL_SRP 

/* Define to  if GnuTLS has the gnutls_srp_verifier function. */
#undef HAVE_GNUTLS_SRP 

/* Define to  to enable TLS-SRP support. */
#undef USE_TLS_SRP 

/* Define to  to query for HTTPSRR when using DoH */
#undef USE_HTTPSRR 

/* if ECH support is available */
#undef USE_ECH 

/* Define to  if you have the wolfSSL_CTX_GenerateEchConfig function. */
#undef HAVE_WOLFSSL_CTX_GENERATEECHCONFIG

/* Define to  if you have the SSL_set_ech_config_list function. */
#undef HAVE_SSL_SET_ECH_CONFIG_LIST


#define SIZEOF_CURL_OFF_T 8
