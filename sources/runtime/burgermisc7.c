// COMPILE: -lang c++ -bool off
/* burgermisc7.c -- burgermisc, exe run 7. Notes: Research/notes/burgermisc.md */

#include <string.h>
#include "burgertypes.h"
#include "burger.h"
#include "burgerplat.h"

#ifdef __cplusplus
extern "C" {
#endif

// FUNCTION: EVNOVA 0x00503dd0
void FastMemSet(void *DestPtr, Word Fill, Word32 Length)
{
    memset(DestPtr, Fill, Length);
}

#ifdef __cplusplus
}
#endif
