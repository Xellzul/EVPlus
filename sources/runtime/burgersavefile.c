// COMPILE: -lang c++ -bool off
/* burgersavefile.c -- BurgerLib MMMemoryGeneric's cleared allocators. Notes: Research/notes/burgersavefile.md */

#include <stdio.h>
#include <stdlib.h>
#include "burgertypes.h"
#include "burger.h"
#include "burgerplat.h"

#ifdef __cplusplus
extern "C" {
#endif

/* AllocAPointerClear (BurgerLib MMMemoryGeneric). */
// FUNCTION: EVNOVA 0x004f29c0
void *AllocAPointerClear(uint32_t size)
{
    void *mem;

    mem = AllocAPointer(size);
    if (mem != 0)
        memset(mem, 0, size);
    return mem;
}

#ifdef __cplusplus
}
#endif
