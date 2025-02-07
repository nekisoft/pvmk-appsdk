//pvmkoslib/stddef.h
//Standard type definitions for using picolibc
//Bryan E. Topp <betopp@betopp.com> 2024
#ifndef _STDDEF_H
#define _STDDEF_H

#if defined(__cplusplus)
	#define NULL nullptr
	typedef decltype(nullptr) nullptr_t;
	#define offsetof(type, memberdesignator) __builtin_offsetof(type, memberdesignator)
#else
	#define NULL ((void*)(0))
	#define offsetof(type, memberdesignator) ((size_t)(&(((type*)0)->memberdesignator)))
#endif

#define _SIZE_T_DECLARED
typedef __SIZE_TYPE__ size_t;

typedef __PTRDIFF_TYPE__ ptrdiff_t;
typedef __WINT_TYPE__ wint_t;

#ifndef __cplusplus
typedef __WCHAR_TYPE__ wchar_t;
#endif

typedef __UINT64_TYPE__ max_align_t __attribute__((aligned(16)));

//What the everlasting fuck, why do I need this
//What are the picolibc guys smoking that ino_t can be 16 bits
typedef __UINT32_TYPE__ __ino_t;
typedef __UINT64_TYPE__ __ino64_t;
#define __machine_ino_t_defined 1

#endif //_STDDEF_H

#undef __need_size_t
#undef __need_ptrdiff_t
#undef __need_wint_t
#undef __need_wchar_t
#undef __need_max_align_t
#undef __need_NULL
#undef __need_mbstate_t

#undef __need_ino_t
#undef __need_ino64_t
