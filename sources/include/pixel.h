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

/* A spread pixel has green moved up, so each channel can be multiplied by up to 32 without reaching the
   next one. */
constexpr uint32_t kSpreadMask = 0x01f07c1f;

constexpr uint32_t Spread(uint32_t p)
{
    return (p & kGreen555) << 15 | (p & (kRed555 | kBlue555));
}

constexpr unsigned short Unspread(uint32_t e)
{
    return (unsigned short)((e & (kRed555 | kBlue555)) | (e >> 15 & kGreen555));
}

/* two spread pixels mixed, w/32 of the second, each channel rounded down */
constexpr uint32_t Mix(uint32_t a, uint32_t b, uint32_t w)
{
    return (a * (32 - w) + b * w) >> 5 & kSpreadMask;
}

/* alpha/32 of src over dst */
constexpr unsigned short Blend(uint32_t src, uint32_t dst, uint32_t alpha)
{
    return Unspread(Mix(Spread(dst), Spread(src), alpha));
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
