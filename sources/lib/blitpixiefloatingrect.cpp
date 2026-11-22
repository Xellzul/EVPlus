/* blitpixiefloatingrect.cpp -- BlitPixieFloatingRect (win 0x00471d30), the direct-colour translucent rectangle copy. Notes: Research/notes/blitpixiefloatingrect.md */
#include "spriteworld.h"


// FUNCTION: EVNOVA 0x00471d30
// MAC: _BlitPixieFloatingRect i386 0x000be4f8 ppc 0x000c9e7c
void BlitPixieFloatingRect(unsigned char *srcPixels, unsigned char *dstPixels,
        int32_t srcRowBytes, int32_t dstRowBytes, unsigned short byteWidth,
        unsigned short numRows, unsigned short level)
{
    SpriteBlend blend = {};
    blend.overBias = 32;
    blend.alpha = 32;
    blend.alphaR = blend.alphaG = blend.alphaB = level;

    while (numRows--) {
        BlitPixieTranslucentCopy(dstPixels, srcPixels, byteWidth, blend);

        srcPixels += srcRowBytes;
        dstPixels += dstRowBytes;
    }
}
