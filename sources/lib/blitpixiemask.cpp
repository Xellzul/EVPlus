/* blitpixiemask.cpp -- BlitPixieMask (win 0x00471a70), the per-row masked blit BlitPixieMaskDrawProc calls. Notes: Research/notes/blitpixiemask.md */
/* The row strides are pre-adjusted once at the top, on the PARAMETER slots, so the
   per-row advance adds only the leftover stride. The source pixels are pre-masked by
   the caller, which is why an OR suffices; both counters are unsigned. */
#include "shipobj.h"

// FUNCTION: EVNOVA 0x00471a70
// MAC: _BlitPixieMask i386 0x000c0f75 ppc 0x000cce04
void BlitPixieMask(unsigned char *srcPixels, unsigned char *dstPixels,
        unsigned char *maskPixels, uint32_t srcRowBytes,
        uint32_t dstRowBytes, unsigned short byteWidth,
        unsigned short numRows)
{
    uint32_t i;

    srcRowBytes -= byteWidth;
    dstRowBytes -= byteWidth;

    while (numRows--) {
        for (i = 0; i < byteWidth; i++) {
            *dstPixels = (*dstPixels & *maskPixels++) | *srcPixels++;
            dstPixels++;
        }

        srcPixels += srcRowBytes;
        dstPixels += dstRowBytes;
        maskPixels += srcRowBytes;
    }
}
