//pvmkoslib/stdarg.h
//Variadic function definitions for using picolibc
//Bryan E. Topp <betopp@betopp.com> 2024
#ifndef _STDARG_H
#define _STDARG_H

//GCC and Clang define vararg handling as builtins.
#define va_start(ap,argn) __builtin_va_start(ap,argn)
#define va_copy(dest,src) __builtin_va_copy(dest,src)
#define va_arg(ap,type) __builtin_va_arg(ap,type)
#define va_end(ap) __builtin_va_end(ap)

typedef __builtin_va_list va_list;

#endif //_STDARG_H
