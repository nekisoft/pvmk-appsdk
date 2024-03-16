//pvmkoslib/limits.h
//Numeric limits definitions for using picolibc
//Bryan E. Topp <betopp@betopp.com> 2024
#ifndef _LIMITS_H
#define _LIMITS_H

#define WINT_MAX (__WINT_MAX__)
#define WINT_MIN (-__WINT_MAX__ - 1)


#define SSIZE_MAX (__SIZE_MAX__/2)

#define SIZE_MAX (__SIZE_MAX__)

#define SHRT_MAX (__SHRT_MAX__)
#define SHRT_MIN (-__SHRT_MAX__-1)
#define USHRT_MAX ((SHRT_MAX*2)+1)
#define USHRT_MIN (0)


#define PTRDIFF_MAX (__PTRDIFF_MAX__)
#define PTRDIFF_MIN (-__PTRDIFF_MAX__ - 1)

#define LONG_MAX (__LONG_MAX__)
#define LONG_MIN (-__LONG_MAX__-1)
#define ULONG_MAX ((2ul*(__LONG_MAX__))+1ul) //GCC doesn't give us this?

#define LLONG_MAX (__LONG_LONG_MAX__)
#define LLONG_MIN (-__LONG_LONG_MAX__-1)
#define ULLONG_MAX (18446744073709551615ULL) //GCC doesn't give us this?


#define INTPTR_MAX (__INTPTR_MAX__)
#define INTPTR_MIN (-__INTPTR_MAX__-1)

#define UINTPTR_MAX (__UINTPTR_MAX__)


#define INT8_MAX (__INT8_MAX__)
#define INT8_MIN (-__INT8_MAX__ - 1)

#define INT16_MAX (__INT16_MAX__)
#define INT16_MIN (-__INT16_MAX__ - 1)

#define INT32_MAX (__INT32_MAX__)
#define INT32_MIN (-__INT32_MAX__ - 1)

#define INT64_MAX (__INT64_MAX__)
#define INT64_MIN (-__INT64_MAX__ - 1)


#define UINT8_MAX (__UINT8_MAX__)
#define UINT16_MAX (__UINT16_MAX__)
#define UINT32_MAX (__UINT32_MAX__)
#define UINT64_MAX (__UINT64_MAX__)


#define INTMAX_MAX (__INTMAX_MAX__)
#define INTMAX_MIN (-__INTMAX_MAX__ - 1)

#define UINTMAX_MAX (__UINTMAX_MAX__)

#define INT_LEAST8_MAX (__INT_LEAST8_MAX__)
#define INT_LEAST8_MIN (-__INT_LEAST8_MAX__ - 1)

#define INT_LEAST16_MAX (__INT_LEAST16_MAX__)
#define INT_LEAST16_MIN (-__INT_LEAST16_MAX__ - 1)

#define INT_LEAST32_MAX (__INT_LEAST32_MAX__)
#define INT_LEAST32_MIN (-__INT_LEAST32_MAX__ - 1)

#define INT_LEAST64_MAX (__INT_LEAST64_MAX__)
#define INT_LEAST64_MIN (-__INT_LEAST64_MAX__ - 1)


#define UINT_LEAST8_MAX (__UINT_LEAST8_MAX__)
#define UINT_LEAST16_MAX (__UINT_LEAST16_MAX__)
#define UINT_LEAST32_MAX (__UINT_LEAST32_MAX__)
#define UINT_LEAST64_MAX (__UINT_LEAST64_MAX__)


#define INT_FAST8_MAX (__INT_FAST8_MAX__)
#define INT_FAST8_MIN (-__INT_FAST8_MAX__ - 1)

#define INT_FAST16_MAX (__INT_FAST16_MAX__)
#define INT_FAST16_MIN (-__INT_FAST16_MAX__ - 1)

#define INT_FAST32_MAX (__INT_FAST32_MAX__)
#define INT_FAST32_MIN (-__INT_FAST32_MAX__ - 1)

#define INT_FAST64_MAX (__INT_FAST64_MAX__)
#define INT_FAST64_MIN (-__INT_FAST64_MAX__ - 1)


#define UINT_FAST8_MAX (__UINT_FAST8_MAX__)
#define UINT_FAST16_MAX (__UINT_FAST16_MAX__)
#define UINT_FAST32_MAX (__UINT_FAST32_MAX__)
#define UINT_FAST64_MAX (__UINT_FAST64_MAX__)

#define INT_MAX (__INT_MAX__)
#define INT_MIN (-__INT_MAX__-1)
#define UINT_MAX ((__INT_MAX__ * 2U) + 1U)

#define SCHAR_MAX (__SCHAR_MAX__)
#define SCHAR_MIN (-__SCHAR_MAX__-1)
#define UCHAR_MAX ((__SCHAR_MAX__*2)+1)

#ifdef __CHAR_UNSIGNED__
	#define CHAR_MAX UCHAR_MAX
	#define CHAR_MIN 0
#else
	#define CHAR_MAX SCHAR_MAX
	#define CHAR_MIN SCHAR_MIN
#endif

#define CHAR_BIT 8 //ya better hope huh

#define MB_LEN_MAX 4

#define PATH_MAX 1023

#endif //_LIMITS_H
