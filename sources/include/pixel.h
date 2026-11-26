/* pixel.h -- the 16-bit 5-5-5 pixel: packing, channels, blends, rows. */
#pragma once

#include <stddef.h>
#include <stdint.h>
#include "macport.h"

namespace Pixel {

constexpr int kBytes = 2;

constexpr unsigned Red(unsigned p) { return p >> 10 & 0x1f; }
constexpr unsigned Green(unsigned p) { return p >> 5 & 0x1f; }
constexpr unsigned Blue(unsigned p) { return p & 0x1f; }

/* from 5-bit channels */
constexpr unsigned short Pack(unsigned r, unsigned g, unsigned b)
{
    return (unsigned short)(r << 10 | g << 5 | b);
}

/* from 8-bit channels, the low 3 bits dropped */
constexpr unsigned short From8(unsigned r, unsigned g, unsigned b)
{
    return Pack(r >> 3, g >> 3, b >> 3);
}

/* from 0x00RRGGBB, the low 3 bits dropped */
constexpr unsigned short FromRGB24(uint32_t c)
{
    return From8(c >> 16 & 0xff, c >> 8 & 0xff, c & 0xff);
}

/* from 16-bit channels, the low 11 bits dropped */
constexpr unsigned short FromColor(const RGBColor &c)
{
    return Pack(c.red >> 11, c.green >> 11, c.blue >> 11);
}

/* from 16-bit channels, rounded; a channel of 0xf7ff..0xfffe wraps to 0, 0xffff to 1 */
constexpr unsigned short FromColorRounded(const RGBColor &c)
{
    return Pack((c.red + 0x801) >> 11 & 0x1f, (c.green + 0x801) >> 11 & 0x1f, (c.blue + 0x801) >> 11 & 0x1f);
}

/* as opaque ARGB, each channel's top bits repeated in its low ones, as a GPU widens 5 bits */
constexpr uint32_t ToArgb(unsigned p)
{
    return 0xff000000u | (Red(p) << 3 | Red(p) >> 2) << 16 | (Green(p) << 3 | Green(p) >> 2) << 8
           | (Blue(p) << 3 | Blue(p) >> 2);
}

/* each channel through a 32-entry table */
inline unsigned short Map(unsigned p, const unsigned char *table)
{
    return Pack(table[Red(p)], table[Green(p)], table[Blue(p)]);
}

/* from 16-bit channels through the gamma ramp */
inline unsigned short FromColorGamma(const RGBColor &c)
{
    return Pack(gGammaRamp[c.red >> 11], gGammaRamp[c.green >> 11], gGammaRamp[c.blue >> 11]);
}

/* Two pixels in one 32-bit word: one channel of both scaled by k/32, rounded down. */
constexpr uint32_t kRedPair = 0x7c007c00;
constexpr uint32_t kGreenPair = 0x03e003e0;
constexpr uint32_t kBluePair = 0x001f001f;

constexpr uint32_t ScaleRedPair(uint32_t c, uint32_t k) { return ((c & kRedPair) >> 5) * k & kRedPair; }
constexpr uint32_t ScaleGreenPair(uint32_t c, uint32_t k) { return (c & kGreenPair) * k >> 5 & kGreenPair; }
constexpr uint32_t ScaleBluePair(uint32_t c, uint32_t k) { return (c & kBluePair) * k >> 5 & kBluePair; }
constexpr uint32_t ScalePair(uint32_t c, uint32_t k)
{
    return ScaleRedPair(c, k) | ScaleGreenPair(c, k) | ScaleBluePair(c, k);
}

/* Two packed pixels added per channel, each channel clamped at 31: how a sprite's part and the screen's
   combine, as a GPU's additive blend does (the original ORed them). */
inline uint32_t AddPair(uint32_t a, uint32_t b)
{
    constexpr uint32_t kHigh = 0x42104210;      /* each channel's top bit */
    constexpr uint32_t kLow = 0x3def3def;       /* each channel's other bits */

    a &= 0x7fff7fff;
    b &= 0x7fff7fff;
    uint32_t low = (a & kLow) + (b & kLow);
    uint32_t sum = low ^ ((a ^ b) & kHigh);
    uint32_t carry = ((a & b) | ((a | b) & low)) & kHigh;
    return sum | (carry >> 4) * 31;
}

/* row y of 16-bit pixels */
inline unsigned short *Row(const PixMap &pix, int y)
{
    return (unsigned short *)((char *)pix.baseAddr + (ptrdiff_t)y * (ptrdiff_t)pix.rowBytes);
}

inline unsigned short *Row(const SWFrameRec &frame, int y)
{
    return (unsigned short *)(frame.frameBaseAddr + (ptrdiff_t)y * (ptrdiff_t)frame.frameRowBytes);
}

} // namespace Pixel
