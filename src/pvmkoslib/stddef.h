//pvmkoslib/stddef.h
//Standard type definitions for using picolibc
//Bryan E. Topp <betopp@betopp.com> 2024
#ifndef _STDDEF_H
#define _STDDEF_H

#define NULL ((void*)(0))
#define offsetof(type, memberdesignator) ((size_t)(&(((type*)0)->memberdesignator)))
typedef __SIZE_TYPE__ size_t;
typedef __PTRDIFF_TYPE__ ptrdiff_t;
typedef __WINT_TYPE__ wint_t;
typedef __WCHAR_TYPE__ wchar_t;

#endif //_STDDEF_H
