/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing#kwsys for details.  */
#ifndef cmsys_Base64_h
#define cmsys_Base64_h

#include <cmsys/Configure.h>

#include <stddef.h> /* size_t */

/* Redefine all public interface symbol names to be in the proper
   namespace.  These macros are used internally to kwsys only, and are
   not visible to user code.  Use kwsysHeaderDump.pl to reproduce
   these macros after making changes to the interface.  */
#if !defined(KWSYS_NAMESPACE)
#  define kwsys_ns(x) cmsys##x
#  define kwsysEXPORT cmsys_EXPORT
#endif
#if !cmsys_NAME_IS_KWSYS
#  define kwsysBase64 kwsys_ns(Base64)
#  define kwsysBase64_Decode kwsys_ns(Base64_Decode)
#  define kwsysBase64_Decode3 kwsys_ns(Base64_Decode3)
#  define kwsysBase64_Encode kwsys_ns(Base64_Encode)
#  define kwsysBase64_Encode1 kwsys_ns(Base64_Encode1)
#  define kwsysBase64_Encode2 kwsys_ns(Base64_Encode2)
#  define kwsysBase64_Encode3 kwsys_ns(Base64_Encode3)
#endif

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * Encode 3 bytes into a 4 byte string.
 */
kwsysEXPORT void kwsysBase64_Encode3(unsigned char const* src,
                                     unsigned char* dest);

/**
 * Encode 2 bytes into a 4 byte string.
 */
kwsysEXPORT void kwsysBase64_Encode2(unsigned char const* src,
                                     unsigned char* dest);

/**
 * Encode 1 bytes into a 4 byte string.
 */
kwsysEXPORT void kwsysBase64_Encode1(unsigned char const* src,
                                     unsigned char* dest);

/**
 * Encode 'length' bytes from the input buffer and store the encoded
 * stream into the output buffer. Return the length of the encoded
 * buffer (output). Note that the output buffer must be allocated by
 * the caller (length * 1.5 should be a safe estimate).  If 'mark_end'
 * is true than an extra set of 4 bytes is added to the end of the
 * stream if the input is a multiple of 3 bytes.  These bytes are
 * invalid chars and therefore they will stop the decoder thus
 * enabling the caller to decode a stream without actually knowing how
 * much data to expect (if the input is not a multiple of 3 bytes then
 * the extra padding needed to complete the encode 4 bytes will stop
 * the decoding anyway).
 */
kwsysEXPORT size_t kwsysBase64_Encode(unsigned char const* input,
                                      size_t length, unsigned char* output,
                                      int mark_end);

/**
 * Decode 4 bytes into a 3 byte string.  Returns the number of bytes
 * actually decoded.
 */
kwsysEXPORT int kwsysBase64_Decode3(unsigned char const* src,
                                    unsigned char* dest);

/**
 * Decode bytes from the input buffer and store the decoded stream
 * into the output buffer until 'length' bytes have been decoded.
 * Return the real length of the decoded stream (which should be equal
 * to 'length'). Note that the output buffer must be allocated by the
 * caller.  If 'max_input_length' is not null, then it specifies the
 * number of encoded bytes that should be at most read from the input
 * buffer. In that case the 'length' parameter is ignored. This
 * enables the caller to decode a stream without actually knowing how
 * much decoded data to expect (of course, the buffer must be large
 * enough).
 */
kwsysEXPORT size_t kwsysBase64_Decode(unsigned char const* input,
                                      size_t length, unsigned char* output,
                                      size_t max_input_length);

#if defined(__cplusplus)
} /* extern "C" */
#endif

/* If we are building a kwsys .c or .cxx file, let it use these macros.
   Otherwise, undefine them to keep the namespace clean.  */
#if !defined(KWSYS_NAMESPACE)
#  undef kwsys_ns
#  undef kwsysEXPORT
#  if !cmsys_NAME_IS_KWSYS
#    undef kwsysBase64
#    undef kwsysBase64_Decode
#    undef kwsysBase64_Decode3
#    undef kwsysBase64_Encode
#    undef kwsysBase64_Encode1
#    undef kwsysBase64_Encode2
#    undef kwsysBase64_Encode3
#  endif
#endif

#endif
