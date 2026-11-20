/* swcreateregionfromgworldandrectfinish.cpp -- SWCreateRegionFromGWorldAndRectFinish (win 0x00478f20, donor SpriteWorldUtils.c line 279). Notes: Research/notes/swcreateregionfromgworldandrectfinish.md */
#include "spriteworld.h"



// FUNCTION: EVNOVA 0x00478f20
// MAC: _SWCreateRegionFromGWorldAndRectFinish i386 0x000a6f90 ppc 0x000b1bb8
void SWCreateRegionFromGWorldAndRectFinish(CGrafPtr tempMaskGWorld)
{
    if (tempMaskGWorld != 0)
        DisposeGWorld(tempMaskGWorld);
}

