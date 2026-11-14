// COMPILE: -lang c++ -bool off
/* blitpixieaddovercopy.c -- BlitPixieAddOverCopy (win 0x00473b60), inside the un-anchored BlitPixie block. Notes: Research/notes/blitpixieaddovercopy.md */
#include "macport.h"
#include "shipobj.h"

EXTERN_C_BEGIN

/* BlitPixieAddOverCopy (mac _BlitPixieAddOverCopy). STATE: MATCH 100.0%, byte-identical. OPEN: none. */
// FUNCTION: EVNOVA 0x00473b60
// MAC: _BlitPixieAddOverCopy i386 0x000c177d ppc 0x000cd7d4
void BlitPixieAddOverCopy(unsigned char *dstPixels, unsigned char *srcPixels,
                  int32_t byteWidth)
{
    int32_t i;
    uint32_t *sp;
    uint32_t *dp;

    sp = (uint32_t *)srcPixels;
    dp = (uint32_t *)dstPixels;
    for (i = 0; i < (byteWidth >> 2); i++) {
        *dp |= *sp++;
        dp++;
    }

    srcPixels = (unsigned char *)sp;
    dstPixels = (unsigned char *)dp;

    if (byteWidth & 2) {
        unsigned short *wordDst = (unsigned short *)dstPixels;
        *wordDst = *wordDst | *(unsigned short *)srcPixels;
    }

    /* BUG: the tails never advance srcPixels and dstPixels, so a width of 4n+3 re-ORs the short's low byte */
    if (byteWidth & 1)
        *dstPixels = *dstPixels | *srcPixels;
}

EXTERN_C_END
