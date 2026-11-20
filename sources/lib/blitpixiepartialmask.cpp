/* blitpixiepartialmask.cpp -- BlitPixiePartialMask (win 0x00471b70), the per-row blit BlitPixiePartialMaskDrawProc calls. Notes: Research/notes/blitpixiepartialmask.md */
#include "shipobj.h"

EXTERN_C_BEGIN

// FUNCTION: EVNOVA 0x00471b70
// MAC: _BlitPixiePartialMask i386 0x000c0fe5 ppc 0x000cce88
void BlitPixiePartialMask(unsigned char *srcPixels, unsigned char *dstPixels,
        unsigned char *maskPixels, uint32_t srcRowBytes, uint32_t dstRowBytes,
        unsigned short byteWidth, unsigned short numRows)
{
    uint32_t i;
    unsigned char maskByte;

    srcRowBytes -= byteWidth;
    dstRowBytes -= byteWidth;

    while (numRows--) {
        for (i = 0; i < byteWidth; i++) {
            maskByte = *maskPixels++;
            *dstPixels++ = (*dstPixels & maskByte) | (*srcPixels++ & ~maskByte);
        }

        srcPixels += srcRowBytes;
        dstPixels += dstRowBytes;
        maskPixels += srcRowBytes;
    }
}

EXTERN_C_END
