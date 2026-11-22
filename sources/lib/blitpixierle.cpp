/* blitpixierle.cpp -- the RLE sprite blitters (win 0x00471e10-0x004730d0): one token walker, a span copy per
   blitter. Notes: Research/notes/blitpixierle.md */
#include "spriteworld.h"

namespace {

enum class Clip {
    None,
    Exact,
    PastRight   /* BlitPixieRLEClipped: the last column counted inclusive */
};

/* the part of a span at x inside the clip: lead bytes skipped, count drawn; false when none is */
template <Clip kClip>
inline bool ClipSpan(int32_t x, int32_t width, const Rect *clip, int32_t &lead, int32_t &count)
{
    lead = 0;
    count = width;
    if constexpr (kClip != Clip::None) {
        // BUG: PastRight's last column is inclusive but clip->right is exclusive, so the right trim leaves
        // one byte past the clip edge (a partial pixel at 16bpp), and a span ending exactly on the left edge
        // is skipped; the translucent/add-over clipped blitters clip exactly
        int32_t last = kClip == Clip::Exact ? x + width : x + width - 1;
        if (!(x < clip->right && last > clip->left))
            return false;
        if (x < clip->left)
            lead = clip->left - x;
        count -= lead;
        if (last > clip->right)
            count -= last - clip->right;
    }
    return true;
}

/* Draws one RLE frame, its pixel runs through copySpan(dst, src, bytes); clip is in bytes and rows of
   the frame. False on an unknown token. */
template <Clip kClip, typename CopySpan>
bool WalkRLE(unsigned char *tokens, unsigned char *dst, int32_t dstRowBytes, const Rect *clip,
             CopySpan copySpan)
{
    unsigned char *rowStart = dst;
    int32_t x = 0;
    int32_t row = 0;
    uint32_t opcode;

    do {
        uint32_t token = *(uint32_t *)tokens;
        opcode = token >> kTokenShift;
        tokens += 4;
        int32_t width = token & kCountMask;
        int32_t lead, count;

        switch (opcode) {
        case kDrawPixelsToken:
            if (ClipSpan<kClip>(x, width, clip, lead, count))
                copySpan(dst + lead, tokens + lead, count);
            tokens += width;
            tokens += (-(intptr_t)tokens) & 3;
            x += width;
            dst += width;
            break;

        case kSkipPixelsToken:
            x += width;
            dst += width;
            break;

        case kSingleColorToken:
            if (ClipSpan<kClip>(x, width, clip, lead, count))
                BlitPixieMemSet(dst + lead, *(uint32_t *)tokens, count);
            tokens += 4;
            x += width;
            dst += width;
            break;

        case kLineStartToken:
            if constexpr (kClip != Clip::None) {
                if (row < clip->top)
                    tokens += width;
                else if (row >= clip->bottom)
                    return true;
                row++;
            }
            dst = rowStart;
            x = 0;
            rowStart += dstRowBytes;
            break;

        case kEndShapeToken:
            break;

        default:
            return false;
        }
    } while (opcode != kEndShapeToken);
    return true;
}

} // namespace

// FUNCTION: EVNOVA 0x00471e10
// MAC: _BlitPixieRLE i386 0x000bddd0 ppc 0x000c9820
void BlitPixieRLE(unsigned char *rleTokens, unsigned char *dstPixel, int32_t dstRowBytes)
{
    WalkRLE<Clip::None>(rleTokens, dstPixel, dstRowBytes, nullptr, BlitPixieMemCopy);
}

// FUNCTION: EVNOVA 0x00471e90
// MAC: _BlitPixieRLEClipped i386 0x000bde9c ppc 0x000c9938
void BlitPixieRLEClipped(unsigned char *rleTokens, unsigned char *dstPixel, int32_t dstRowBytes,
        Rect *srcBlitRect)
{
    WalkRLE<Clip::PastRight>(rleTokens, dstPixel, dstRowBytes, srcBlitRect, BlitPixieMemCopy);
}

// FUNCTION: EVNOVA 0x00472900
// MAC: _BlitPixieRLETranslucent i386 0x000c1568 ppc 0x000cd4fc
void BlitPixieRLETranslucent(unsigned char *rleTokens, unsigned char *dstPixel, int32_t dstRowBytes,
        const SpriteBlend &blend)
{
    WalkRLE<Clip::None>(rleTokens, dstPixel, dstRowBytes, nullptr,
        [&blend](unsigned char *dst, unsigned char *src, int32_t bytes) {
            BlitPixieTranslucentCopy(dst, src, bytes, blend);
        });
}

// FUNCTION: EVNOVA 0x00472b30
// MAC: _BlitPixieRLEAddOver i386 0x000c24bf ppc 0x000ce7f0
void BlitPixieRLEAddOver(unsigned char *rleTokens, unsigned char *dstPixel, int32_t dstRowBytes)
{
    WalkRLE<Clip::None>(rleTokens, dstPixel, dstRowBytes, nullptr, BlitPixieAddOverCopy);
}

// FUNCTION: EVNOVA 0x00472d70
// MAC: _BlitPixieRLETranslucentClipped i386 0x000c18c0 ppc 0x000cd93c
void BlitPixieRLETranslucentClipped(unsigned char *rleTokens, unsigned char *dstPixel,
        int32_t dstRowBytes, Rect *srcBlitRect, const SpriteBlend &blend)
{
    if (!WalkRLE<Clip::Exact>(rleTokens, dstPixel, dstRowBytes, srcBlitRect,
            [&blend](unsigned char *dst, unsigned char *src, int32_t bytes) {
                BlitPixieTranslucentCopy(dst, src, bytes, blend);
            }))
        DebugStr("\runknown token");
}

// FUNCTION: EVNOVA 0x004730d0
// MAC: _BlitPixieRLEAddOverClipped i386 0x000c25e7 ppc 0x000ce954
void BlitPixieRLEAddOverClipped(unsigned char *rleTokens, unsigned char *dstPixel,
        int32_t dstRowBytes, Rect *srcBlitRect)
{
    if (!WalkRLE<Clip::Exact>(rleTokens, dstPixel, dstRowBytes, srcBlitRect, BlitPixieAddOverCopy))
        DebugStr("\runknown token");
}
