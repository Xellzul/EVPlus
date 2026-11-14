// COMPILE: -lang c++ -bool off
/* swstickyerror.c -- SWClearStickyError (win 0x004794a0, donor SpriteWorldUtils.c line 1820) and SWSetStickyIfError (win 0x004794b0, donor line 1838). Notes: Research/notes/swstickyerror.md */
#include "macport.h"
#include "shipobj.h"
#include "spriteworld.h"


/* SWClearStickyError (mac _SWClearStickyError). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004794a0
// MAC: _SWClearStickyError i386 0x000a7a36 ppc 0x000b2834
void SWClearStickyError(void)
{
    gSWStickyError = 0;
}

/* SWSetStickyIfError (mac _SWSetStickyIfError). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004794b0
// MAC: _SWSetStickyIfError i386 0x000a6f77 ppc 0x000b1ba4
void SWSetStickyIfError(short errNum)
{
    if ((int)errNum != noErr)
        gSWStickyError = errNum;
}

