/* atplatlock2.c -- atplatlock, exe run 2: the bit test of the atomic bit set. The exe's run also holds the
   layer's recursive lock and the atomic add, set and clear (InterlockedCompareExchange looked up at run
   time, the lock where old Windows lacks it); their users were the thread table and the timers.
   Notes: Research/notes/atplatlock.md */

#include "burgertypes.h"
#include "burger.h"

// FUNCTION: EVNOVA 0x005060d0
unsigned char AtomicMaskTest(ULong *word, ULong bit)
{
    return (*word & (1 << bit)) ? true : false;
}
