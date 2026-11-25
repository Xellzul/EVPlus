/* swcreatepixelmaskfromgworld.cpp -- SWCreatePixelMaskFromGWorld (win 0x00479250, donor SpriteWorldUtils.c line 930). Notes: Research/notes/swcreatepixelmaskfromgworld.md */
/* The win exe has a four-byte stub here (`or eax,-1 / ret'), so a PICT sprite whose
   mask ID is its own picture failed to load. This is the mac's 16-bit path
   (_SWCreateMaskAndFixImage, reached because the default transparent colour is white):
   pixels of the transparent colour become clear in the mask and black in the image. */
#include "spriteworld.h"

#undef kClear
#undef kSolid
#define kClear  0xFFFF
#define kSolid  0x0000

RGBColor gSWTransparentColor = {0xFFFF, 0xFFFF, 0xFFFF};  /* white */

// FUNCTION: EVNOVA 0x00479250
// MAC: _SWCreatePixelMaskFromGWorld i386 0x000a875d ppc 0x000b36b8
short SWCreatePixelMaskFromGWorld(CGrafPtr pictureGWorld, CGrafPtr maskGWorld,
                                  Rect *rectP)
{
    PixMapPtr pictPix;
    PixMapPtr maskPix;
    char *baseAddr;
    char *maskAddr;
    int32_t rowBytes;
    int32_t width, height;
    int32_t x, y;
    unsigned short transparent;
    unsigned short *dst;
    unsigned short *mask;

    pictPix = GetCWindowPixMap(pictureGWorld);
    maskPix = GetCWindowPixMap(maskGWorld);
    if (GetPixDepth(pictPix) != 16 || GetPixDepth(maskPix) != 16)
        return -1;
    if (GetPixRowBytes(pictPix) != GetPixRowBytes(maskPix))
        return -1;

    if (rectP == NULL)
        rectP = GetWindowPortRect(pictureGWorld);
    width = rectP->right - rectP->left;
    height = rectP->bottom - rectP->top;

    transparent = Color2Index(&gSWTransparentColor) & kAlphaMask16;

    baseAddr = GetPixBaseAddr(pictPix);
    maskAddr = GetPixBaseAddr(maskPix);
    rowBytes = GetPixRowBytes(pictPix);
    baseAddr += rectP->top * rowBytes + rectP->left * 2;
    maskAddr += rectP->top * rowBytes + rectP->left * 2;
    rowBytes -= width * 2;

    dst = (unsigned short *)baseAddr;
    mask = (unsigned short *)maskAddr;
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            if ((*dst & kAlphaMask16) == transparent) {
                *dst++ = 0;
                *mask++ = kClear;
            } else {
                dst++;
                *mask++ = kSolid;
            }
        }
        dst = (unsigned short *)((char *)dst + rowBytes);
        mask = (unsigned short *)((char *)mask + rowBytes);
    }
    return noErr;
}
