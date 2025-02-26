/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c)  OpenBOR Team
 */

/*
 * This library is used for calculating how much memory is available/used.
 * Certain platforms offer physical memory statistics, we obviously wrap
 * around those functions.  For platforms where we can't retrieve this
 * information we then calculate the estimated sizes based on a few key
 * variables and symbols.  These estimated values should tolerable.......
 */

/////////////////////////////////////////////////////////////////////////////
// Libraries


#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include "../globals.h"
#include "../utils.h"
#include "ram.h"

int systemRam = 16*1024*1024;
int stackSize = 64*1024;

/////////////////////////////////////////////////////////////////////////////
//  Functions

u64 getFreeRam(int byte_size)
{
    struct mallinfo mi = mallinfo();
    return (systemRam - (mi.arena + stackSize)) / byte_size;
}

void setSystemRam()
{
	
}

u64 getSystemRam(int byte_size)
{
    return systemRam / byte_size;
}

u64 getUsedRam(int byte_size)
{
    return (systemRam - getFreeRam(BYTES)) / byte_size;
}

void getRamStatus(int byte_size)
{
  u64 system_ram = getSystemRam(byte_size);
  u64 free_ram = getFreeRam(byte_size);
  u64 used_ram = getUsedRam(byte_size);

  printf("Total Ram: %11"PRIu64" Bytes ( %5"PRIu64" MB )\n Free Ram: %11"PRIu64" Bytes ( %5"PRIu64" MB )\n Used Ram: %11"PRIu64" Bytes ( %5"PRIu64" MB )\n\n",
           system_ram, system_ram >> 20,
           free_ram, free_ram >> 20,
           used_ram, used_ram >> 20);
}

