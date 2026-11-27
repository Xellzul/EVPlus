/* swcreategworldfrompictresource.cpp -- SWCreateGWorldFromPictResource (win 0x00478f30, donor SpriteWorldUtils.c line 356). Notes: Research/notes/swcreategworldfrompictresource.md */
/* The fork's fourth parameter is a byte flag choosing between GetPicture and GetPictureNoGamma, which load
   the same picture here. The error arm is a flat resNotFound, -192. */
#include "spriteworld.h"



// FUNCTION: EVNOVA 0x00478f30
// MAC: _SWCreateGWorldFromPictResource i386 0x000a7f51 ppc 0x000b2d8c
short SWCreateGWorldFromPictResource(SpriteWorldPtr destSpriteWorld,
                                     CGrafPtr *pictGWorldP,
                                     short pictResID,
                                     Boolean useGetPicture)
{
    short err = noErr;
    PicHandle newPictH;

    if (useGetPicture)
        newPictH = GetPicture(pictResID);
    else
        newPictH = GetPictureNoGamma(pictResID);

    if (newPictH != 0) {
        err = SWCreateGWorldFromPict(destSpriteWorld, pictGWorldP, newPictH);

        QDReleaseResource(newPictH);
    } else {
        err = resNotFound;
    }


    return err;
}
