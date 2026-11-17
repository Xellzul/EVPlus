// COMPILE: -lang c++ -bool off
/* swcreategworldfrompict.c -- SWCreateGWorldFromPict (win 0x00478fa0, donor SpriteWorldUtils.c line 392). Notes: Research/notes/swcreategworldfrompict.md */
/* The port's NewGWorld takes four arguments, not six: with no GDevice list the donor's
   device query and its first arm both go. The `err == noErr' test goes through an int
   copy, because mwcc 418 always narrows a short compare. */
#include "macport.h"
#include "shipobj.h"
#include "spriteworld.h"
#include "cefix.h"


// FUNCTION: EVNOVA 0x00478fa0
// MAC: _SWCreateGWorldFromPict i386 0x000a7dc7 ppc 0x000b2bf4
short SWCreateGWorldFromPict(SpriteWorldPtr destSpriteWorld,
                             CGrafPtr *pictGWorld, void *pictH)
{
    short err;
    CGrafPtr saveGWorld;
    void *saveGDevice;
    CGrafPtr tempGWorld;
    Rect pictRect;
    short depth;

    *pictGWorld = 0;

    GetPort(&saveGWorld);
    saveGDevice = GetGWorldDevice(saveGWorld);

    depth = destSpriteWorld->pixelDepth;

    QDGetPictureBounds((PicHandle)pictH, &pictRect);
    OffsetRect(&pictRect, -pictRect.left, -pictRect.top);

#ifdef CEFIX /* F11.03 */
    err = CE_NewGWorld(&tempGWorld, depth, &pictRect, 0);
#else
    err = NewGWorld(&tempGWorld, depth, &pictRect, 0);
#endif

    if ((int)err == noErr) {
        *pictGWorld = tempGWorld;

        SetPort(tempGWorld);

        EraseRect(&pictRect);
        DrawPicHandle((PicHandle)pictH, &pictRect);
    }

    SetPort(saveGWorld);

    SWSetStickyIfError(err);

    return err;
}
