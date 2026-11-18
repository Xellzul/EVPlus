// COMPILE: -lang c++ -bool off
/* swdepthok.c -- sw_depth_ok (win 0x00478f10), in the SpriteWorldUtils.c block. Notes: Research/notes/swdepthok.md */
#include "macport.h"
#include "shipobj.h"
#include "spriteworld.h"

// FUNCTION: EVNOVA 0x00478f10
// MAC: _sw_depth_ok i386 0x000b7f97 ppc 0x000c3b44
Boolean sw_depth_ok(SpriteWorldPtr spriteWorldP)
{
    return 1;
}
