/* swcreategworldfromciconmask.cpp -- SWCreateGWorldFromCIconMask (win 0x00479070, donor SpriteWorldUtils.c line 444). Notes: Research/notes/swcreategworldfromciconmask.md */
#include "spriteworld.h"

#define kWhite555   0x7fff

// FUNCTION: EVNOVA 0x00479070
// MAC: _SWCreateGWorldFromCIconMask i386 0x000a7fbd ppc 0x000b2e0c
short SWCreateGWorldFromCIconMask(SpriteWorldPtr destSpriteWorld,
                                  CGrafPtr *maskGWorldP, Image_t *cIconH)
{
    CGrafPtr saveGWorld;
    Rect bounds;
    CGrafPtr tempGWorldP;
    int32_t srcSkip;
    short err;
    PixMapPtr maskPix;
    PixMapPtr pixBounds;
    short depth;
    int32_t y;
    int32_t width;
    int32_t x;
    unsigned char *src;
    unsigned short *dst;
    int32_t dstSkip;

    GetPort(&saveGWorld);

    depth = destSpriteWorld->pixelDepth;

    SetRect(&bounds, 0, 0, cIconH->Width, cIconH->Height);

    err = NewGWorld(&tempGWorldP, depth, &bounds, 0);

    if ((int)err == noErr) {
        SetPort(tempGWorldP);

        src = cIconH->ImagePtr;
        srcSkip = cIconH->RowBytes - cIconH->Width;

        maskPix = GetCWindowPixMap(tempGWorldP);
        dst = (unsigned short *)GetPixBaseAddr(maskPix);

        pixBounds = GetGWorldPixMap((CGrafPtr)maskPix);
        dstSkip = GetPixRowBytes(maskPix)
                - (pixBounds->bounds.right - pixBounds->bounds.left) * 2;

        y = cIconH->Height;
        width = cIconH->Width;

        do {
            x = width;

            do {
                *dst = (*src == 0) ? 0 : kWhite555;
                dst++;
                src++;
            } while (--x != 0);

            src += srcSkip;
            dst = (unsigned short *)((char *)dst + dstSkip);
        } while (--y != 0);

        InvertRect(&bounds);

        *maskGWorldP = tempGWorldP;
    }

    SetPort(saveGWorld);


    return err;
}
