//stdbool.h
//Boolean type definitions for PVMK app SDK when using picolibc
//Bryan E. Topp <betopp@betopp.com> 2024
#ifndef _STDBOOL_H
#define _STDBOOL_H

//Only need to define bool in plain C before C23.
#if !defined(__cplusplus)
	#if __STDC_VERSION__ < 202311L
		typedef _Bool bool;
		#define true 1
		#define false 0
	#endif
#endif

#endif //_STDBOOL_H
