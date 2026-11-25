/* swinvertgworld.cpp -- SWInvertGWorld (win 0x004791b0, donor SpriteWorldUtils.c line 506) and SWInvertRect (win 0x004791d0, donor line 518). Notes: Research/notes/swinvertgworld.md */
#include "spriteworld.h"

// FUNCTION: EVNOVA 0x004791b0
// MAC: _SWInvertGWorld i386 0x000a703a ppc 0x000b1c60
void SWInvertGWorld(CGrafPtr gWorldP)
{
    SWInvertRect(gWorldP, GetWindowPortRect(gWorldP));
}

// FUNCTION: EVNOVA 0x004791d0
// MAC: _SWInvertRect i386 0x000a6fa2 ppc 0x000b1bc4
void SWInvertRect(CGrafPtr gWorldP, Rect *rectP)
{
    CGrafPtr saveGWorld;

    GetPort(&saveGWorld);
    SetPort(gWorldP);

    InvertRect(rectP);

    SetPort(saveGWorld);
}

