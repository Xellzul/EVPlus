/* blitpixietranslucentcopy.cpp -- SpriteBlend::Of and BlitPixieTranslucentCopy (win 0x004736c0). Notes: Research/notes/blitpixietranslucentcopy.md */
#include "spriteworld.h"

SpriteBlend SpriteBlend::Of(const SpriteRec &sprite)
{
    SpriteBlend blend = {};
    bool tinted = sprite.tintRed != 32 || sprite.tintGreen != 32 || sprite.tintBlue != 32;

    if (sprite.translucency <= 0 && sprite.tintMode <= 0 && !tinted)
        blend.kind = Kind::Plain;
    else if (!tinted && sprite.translucency == 32 && sprite.tintMode == 0)
        blend.kind = Kind::AddOver;
    else if (sprite.translucency < 64)
        blend.kind = Kind::Translucent;
    else
        blend.kind = Kind::Hidden;

    blend.alpha = sprite.translucency;
    blend.biasColor = (sprite.tintColor << 16 & 0xffff0000) | (sprite.tintColor & 0xffff);
    blend.overBias = 32 - sprite.tintMode;

    // BUG (all versions): `x != 0 ? x : 0' is no clamp -- when alpha > tint + 32 the value goes negative,
    // passes the unsigned `> 32' test below and gives garbage over/alpha values instead of 0
    uint32_t red = sprite.tintRed + 32 - blend.alpha != 0 ? sprite.tintRed + 32 - blend.alpha : 0;
    uint32_t green = sprite.tintGreen + 32 - blend.alpha != 0 ? sprite.tintGreen + 32 - blend.alpha : 0;
    uint32_t blue = sprite.tintBlue + 32 - blend.alpha != 0 ? sprite.tintBlue + 32 - blend.alpha : 0;

    blend.alphaR = red;
    blend.alphaG = green;
    blend.alphaB = blue;
    if (red > 32) {
        blend.overR = Pixel::ScaleRedPair(Pixel::kRedPair, red - 32);
        blend.alphaR = 32;
    }
    if (green > 32) {
        blend.overG = Pixel::ScaleGreenPair(Pixel::kGreenPair, green - 32);
        blend.alphaG = 32;
    }
    if (blue > 32) {
        blend.overB = Pixel::ScaleBluePair(Pixel::kBluePair, blue - 32);
        blend.alphaB = 32;
    }
    return blend;
}

/* a pixel pair (or one pixel, or a byte) through the blend */
static inline uint32_t BlendPair(uint32_t src, uint32_t dst, const SpriteBlend &blend)
{
    if (blend.overBias < 32)
        src = Pixel::ScalePair(src, blend.overBias) | Pixel::ScalePair(blend.biasColor, 32 - blend.overBias);

    return (Pixel::ScaleRedPair(src, blend.alphaR) | blend.overR) |
           (Pixel::ScaleGreenPair(src, blend.alphaG) | blend.overG) |
           (Pixel::ScaleBluePair(src, blend.alphaB) | blend.overB) |
           Pixel::ScalePair(dst, blend.alpha);
}

// FUNCTION: EVNOVA 0x004736c0
// MAC: _BlitPixieTranslucentCopy i386 0x000c1110 ppc 0x000ccfe0
void BlitPixieTranslucentCopy(unsigned char *dstPixels, unsigned char *srcPixels,
        int32_t byteWidth, const SpriteBlend &blend)
{
    uint32_t *sp = (uint32_t *)srcPixels;
    uint32_t *dp = (uint32_t *)dstPixels;
    for (int32_t i = 0, numLongs = byteWidth >> 2; i < numLongs; i++, sp++, dp++)
        *dp = BlendPair(*sp, *dp, blend);

    unsigned char *s = (unsigned char *)sp;
    unsigned char *d = (unsigned char *)dp;
    if (byteWidth & 2) {
        *(unsigned short *)d = BlendPair(*(unsigned short *)s, *(unsigned short *)d, blend);
        s += 2;
        d += 2;
    }

    /* BUG: blends a single byte with the 16-bit channel masks, so the result is meaningless */
    if (byteWidth & 1)
        *d = BlendPair(*s, *d, blend);
}
