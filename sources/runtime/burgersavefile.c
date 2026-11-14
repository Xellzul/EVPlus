// COMPILE: -lang c++ -bool off
/* burgersavefile.c -- BurgerLib MMMemoryGeneric's cleared allocators. Notes: Research/notes/burgersavefile.md */

#include <stdio.h>
#include <stdlib.h>
#include "burgertypes.h"
#include "burger.h"
#include "burgerplat.h"
#include "win32.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Allocate a handle and clear what it points at. */
/* AllocAHandleClear. STATE: MATCH 100%, exe 23 / obj 23 instrs, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f2970
void **AllocAHandleClear(ULong size)
{
    void **h;

    h = AllocAHandle2(size, 0);
    if (h != 0)
        FastMemSet(*h, 0, size);
    return h;
}

/* AllocAPointerClear (BurgerLib MMMemoryGeneric). STATE: MATCH 100.0%, exe 21 / obj 21,
   byte-identical yes. OPEN: none */
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
