/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c)  OpenBOR Team
 */

#ifndef DEPENDS_H
#define DEPENDS_H

#include "../gamelib/types.h"
#include "../globals.h"

#ifndef COMPILED_SCRIPT
#define COMPILED_SCRIPT 1
#endif

typedef const char *LPCSTR;
typedef char *LPSTR;
typedef s32 HRESULT;
typedef u32 DWORD;
typedef u32 ULONG;
typedef s32 LONG;
typedef char CHAR;
typedef float FLOAT;
typedef double DOUBLE;

#ifdef WII
#include <gcbool.h>
#else
typedef int BOOL;
#endif

#ifdef VOID
#undef VOID
#endif
typedef void VOID;

#ifndef NULL
#define NULL 0
#endif

#ifdef S_OK
#undef S_OK
#endif
#define S_OK   ((HRESULT)0)

#ifdef E_FAIL
#undef E_FAIL
#endif
#define E_FAIL ((HRESULT)-1)

#ifdef FAILED
#undef FAILED
#endif
#define FAILED(status) (((HRESULT)(status))<0)

#ifdef SUCCEEDED
#undef SUCCEEDED
#endif
#define SUCCEEDED(status) (((HRESULT)(status))>=0)

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

#define MAX_STR_LEN    128
#define MAX_STR_VAR_LEN    64

//#pragma pack(4)

#endif
