// COMPILE: -lang c++ -bool off
/* blitpixietranslucentcopy.c -- BlitPixieTranslucentCopy (win 0x004736c0). Notes: Research/notes/blitpixietranslucentcopy.md */
#include "macport.h"
#include "shipobj.h"
#include "spriteworld.h"

#define SW_RED(c, k)    ((((c) & 0x7c007c00) >> 5) * (k) & 0x7c007c00)
#define SW_GREEN(c, k)  (((c) & 0x3e003e0) * (k) >> 5 & 0x3e003e0)
#define SW_BLUE(c, k)   (((c) & 0x1f001f) * (k) >> 5 & 0x1f001f)

// FUNCTION: EVNOVA 0x004736c0
// MAC: _BlitPixieTranslucentCopy i386 0x000c1110 ppc 0x000ccfe0
void BlitPixieTranslucentCopy(unsigned char *dstPixels, unsigned char *srcPixels,
        int32_t byteWidth)
{
    int32_t numLongs;
    int32_t i;
    uint32_t pixel;
    uint32_t src;
    uint32_t dst;
    unsigned char *s;
    uint32_t *sp;
    unsigned char *d;
    uint32_t *dp;

    d = dstPixels;
    s = srcPixels;

    dp = (uint32_t *)d;
    sp = (uint32_t *)s;
    for (i = 0, numLongs = byteWidth >> 2; i < numLongs; i++) {
        if (overBias < 32) {
            src = SW_RED(*sp, overBias);
            src |= SW_GREEN(*sp, overBias);
            src |= SW_BLUE(*sp, overBias);
            src |= SW_RED(biasColor, 32 - overBias);
            src |= SW_GREEN(biasColor, 32 - overBias);
            src |= SW_BLUE(biasColor, 32 - overBias);
        } else
            src = *sp;

        dst = *dp;
        pixel = SW_RED(src, alphaR) | overR;
        pixel |= SW_GREEN(src, alphaG) | overG;
        pixel |= SW_BLUE(src, alphaB) | overB;
        pixel |= SW_RED(dst, alpha);
        pixel |= SW_GREEN(dst, alpha);
        pixel |= SW_BLUE(dst, alpha);
        *dp = pixel;

        dp++;
        sp++;
    }

    s = (unsigned char *)sp;
    d = (unsigned char *)dp;
    if (byteWidth & 2) {
        unsigned short *wordSrc = (unsigned short *)s,
                       *wordDst = (unsigned short *)d;

        if (overBias < 32) {
            src = SW_RED(*wordSrc, overBias);
            src |= SW_GREEN(*wordSrc, overBias);
            src |= SW_BLUE(*wordSrc, overBias);
            src |= SW_RED(biasColor, 32 - overBias);
            src |= SW_GREEN(biasColor, 32 - overBias);
            src |= SW_BLUE(biasColor, 32 - overBias);
        } else
            src = *wordSrc;

        pixel = SW_RED(src, alphaR) | overR;
        pixel |= SW_GREEN(src, alphaG) | overG;
        pixel |= SW_BLUE(src, alphaB) | overB;
        pixel |= SW_RED(*wordDst, alpha);
        pixel |= SW_GREEN(*wordDst, alpha);
        pixel |= SW_BLUE(*wordDst, alpha);
        *wordDst++ = pixel;
        wordSrc++;

        s = (unsigned char *)wordSrc;
        d = (unsigned char *)wordDst;
    }

    /* BUG: blends a single byte with the 16-bit channel masks, so the result is meaningless */
    if (byteWidth & 1) {
        if (overBias < 32) {
            src = SW_RED(*s, overBias);
            src |= SW_GREEN(*s, overBias);
            src |= SW_BLUE(*s, overBias);
            src |= SW_RED(biasColor, 32 - overBias);
            src |= SW_GREEN(biasColor, 32 - overBias);
            src |= SW_BLUE(biasColor, 32 - overBias);
        } else
            src = *s;

        pixel = SW_RED(src, alphaR) | overR;
        pixel |= SW_GREEN(src, alphaG) | overG;
        pixel |= SW_BLUE(src, alphaB) | overB;
        pixel |= SW_RED(*d, alpha);
        pixel |= SW_GREEN(*d, alpha);
        pixel |= SW_BLUE(*d, alpha);
        *d = pixel;
    }
}
