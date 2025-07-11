#ifndef _SHARED_H_
#define _SHARED_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <signal.h>
#include <math.h>
#include <limits.h>
#include <zlib.h>

#ifndef PATH_MAX
#ifdef  MAX_PATH
#define PATH_MAX    MAX_PATH
#else
#define PATH_MAX    1024
#endif
#endif

#include "types.h"
#include "macros.h"
#include "cpu/z80.h"
#include "sms.h"
#include "pio.h"
#include "memz80.h"
#include "vdp.h"
#include "render.h"
#include "tms.h"
#include "sound/sn76489.h"
#include "sound/emu2413.h"
#include "sound/ym2413.h"
#include "sound/fmintf.h"
#include "sound/stream.h"
#include "sound/sound.h"
#include "system.h"
#include "error.h"

#include "state.h"
#include "fileio.h"
#include "loadrom.h"
//#include "unzip/unzip.h"


#define LSB_FIRST 1

#endif /* _SHARED_H_ */
