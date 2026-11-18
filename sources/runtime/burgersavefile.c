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

/* Allocate a handle and clear what it points at. */
// FUNCTION: EVNOVA 0x004f2970
void **AllocAHandleClear(ULong size)
{
    void **h;

    h = AllocAHandle2(size, 0);
    if (h != 0)
        FastMemSet(*h, 0, size);
    return h;
}

/* AllocAPointerClear (BurgerLib MMMemoryGeneric). */
// FUNCTION: EVNOVA 0x004f29c0
void *AllocAPointerClear(uint32_t size)
{
    void *mem;

    mem = AllocAPointer(size);
    if (mem != 0)
        FastMemSet(mem, 0, size);
    return mem;
}

#ifdef __cplusplus
}
#endif
