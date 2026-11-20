/* blitpixierect.cpp -- BlitPixieRect (win 0x00471cf0). Notes: Research/notes/blitpixierect.md */
#include "spriteworld.h"

// FUNCTION: EVNOVA 0x00471cf0
// MAC: _BlitPixieRect i386 0x000be4a0 ppc 0x000c9df0
void BlitPixieRect(unsigned char *srcPixels, unsigned char *dstPixels,
        int32_t srcRowBytes, int32_t dstRowBytes, unsigned short byteWidth,
        unsigned short numRows)
{
    while (numRows--) {
        BlitPixieMemCopy(dstPixels, srcPixels, byteWidth);

        srcPixels += srcRowBytes;
        dstPixels += dstRowBytes;
    }
}
