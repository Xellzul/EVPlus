/* burgersavefile.cpp -- BurgerLib MMMemoryGeneric's cleared allocators. Notes: Research/notes/burgersavefile.md */

#include "burger.h"

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
