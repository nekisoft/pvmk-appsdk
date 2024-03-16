//stdbool.h
//Boolean type definitions for PVMK app SDK when using picolibc
//Bryan E. Topp <betopp@betopp.com> 2024
#ifndef _STDBOOL_H
#define _STDBOOL_H

#ifndef __cplusplus
	typedef _Bool bool;
	#define true 1
	#define false 0
#endif

#endif //_STDBOOL_H
