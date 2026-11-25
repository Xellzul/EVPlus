/* blitpixieinterface.cpp -- SpriteWorld's BlitPixieInterface.c, tu-map object O12: the sprite draw and collision procs. Notes: Research/notes/blitpixieinterface.md */
#include "spriteworld.h"

namespace {

/* dst clipped to frame, src trimmed with it; false when nothing is left */
bool ClipToFrame(const Rect &frame, Rect &src, Rect &dst)
{
    if (dst.top < frame.top) {
        src.top += frame.top - dst.top;
        dst.top = frame.top;
    }
    if (dst.bottom > frame.bottom) {
        src.bottom -= dst.bottom - frame.bottom;
        dst.bottom = frame.bottom;
    }
    if (dst.bottom <= dst.top)
        return false;
    if (dst.left < frame.left) {
        src.left += frame.left - dst.left;
        dst.left = frame.left;
    }
    if (dst.right > frame.right) {
        src.right -= dst.right - frame.right;
        dst.right = frame.right;
    }
    return dst.right > dst.left;
}

/* a rect copy between two frames */
struct RectBlit {
    Rect src;
    Rect dst;
    int32_t srcRowBytes;
    int32_t dstRowBytes;
    int width;      /* pixels per row */
    int rows;
};

/* Asserts the frames (BlitPixieInterface.c from `line' on), clips the rects to the destination frame and,
   with interlace on an interlaced frame, takes every other row; false when nothing is left. */
bool PrepareRectBlit(SWFrameRec *srcFrameP, SWFrameRec *dstFrameP, const Rect *srcRect, const Rect *dstRect,
                     int line, bool mask, bool interlace, RectBlit &blit)
{
    if (srcFrameP->framePort == 0)
        SW_ASSERT_FAIL("BlitPixieInterface.c", line, line + 2);
    if (mask) {
        line++;
        if (srcFrameP->maskPort == 0)
            SW_ASSERT_FAIL("BlitPixieInterface.c", line, line + 2);
    }
    if (dstFrameP->framePort == 0)
        SW_ASSERT_FAIL("BlitPixieInterface.c", line + 1, line + 3);
    if (!(srcFrameP->isFrameLocked && dstFrameP->isFrameLocked))
        SW_ASSERT_FAIL("BlitPixieInterface.c", line + 2, line + 4);
    if (!(dstFrameP->frameDepth >= 8))
        SW_ASSERT_FAIL("BlitPixieInterface.c", line + 3, line + 5);
    if (!(srcFrameP->frameDepth == dstFrameP->frameDepth))
        SW_ASSERT_FAIL("BlitPixieInterface.c", line + 4, line + 6);

    const Rect &frame = dstFrameP->frameRect;
    blit.src = *srcRect;
    blit.dst = *dstRect;
    blit.srcRowBytes = srcFrameP->frameRowBytes;
    blit.dstRowBytes = dstFrameP->frameRowBytes;
    if (!ClipToFrame(frame, blit.src, blit.dst))
        return false;

    blit.width = blit.dst.right - blit.dst.left;
    blit.rows = blit.dst.bottom - blit.dst.top;

    if (interlace && dstFrameP->interlacingIsOn) {
        if (((blit.dst.top - frame.top) & 1) == (unsigned char)dstFrameP->skipOddLines) {
            blit.src.top++;
            blit.dst.top++;
            blit.rows--;
        }
        blit.rows = (blit.rows >> 1) + (blit.rows & 1);
        blit.srcRowBytes += blit.srcRowBytes;
        blit.dstRowBytes += blit.dstRowBytes;
    }
    return blit.width > 0 && blit.rows > 0;
}

/* an RLE frame drawn into a frame */
struct RLEBlit {
    unsigned char *tokens;
    unsigned char *dst;         /* where the frame's top left lands */
    uint32_t dstRowBytes;
    bool clipped;
    Rect clip;                  /* when clipped: the part drawn, in bytes and rows of the RLE frame */
};

/* Asserts the frames (BlitPixieInterface.c from `line' on) and clips the sprite to the destination frame;
   false when nothing is left. The clip is converted to bytes with the destination frame's pixel shift. */
bool PrepareRLEBlit(SWFrameRec *srcFrameP, SWFrameRec *dstFrameP, const Rect *srcRect, const Rect *dstRect,
                    int line, RLEBlit &blit)
{
    if (srcFrameP->rleTokenStart == 0)
        SW_ASSERT_FAIL("BlitPixieInterface.c", line, line + 2);
    if (dstFrameP->framePort == 0)
        SW_ASSERT_FAIL("BlitPixieInterface.c", line + 1, line + 3);
    if (!(srcFrameP->isFrameLocked && dstFrameP->isFrameLocked))
        SW_ASSERT_FAIL("BlitPixieInterface.c", line + 2, line + 4);
    if (!(srcFrameP->frameDepth >= 8))
        SW_ASSERT_FAIL("BlitPixieInterface.c", line + 3, line + 5);

    const Rect &frame = dstFrameP->frameRect;
    Rect src = *srcRect;
    Rect dst = *dstRect;
    if (dst.left > frame.right || dst.right < frame.left || dst.bottom < frame.top || dst.top > frame.bottom)
        return false;

    src.right -= src.left;
    src.bottom -= src.top;
    src.left = 0;
    src.top = 0;
    if (!ClipToFrame(frame, src, dst))
        return false;

    blit.tokens = (unsigned char *)srcFrameP->rleTokenStart;
    blit.dst = (unsigned char *)SW_PIXELPTR(dstFrameP, dst.left, dst.top);
    blit.dstRowBytes = dstFrameP->frameRowBytes;
    blit.clipped = src.right - src.left != srcFrameP->frameRect.right - srcFrameP->frameRect.left ||
                   src.bottom - src.top != srcFrameP->frameRect.bottom - srcFrameP->frameRect.top;
    if (blit.clipped) {
        blit.dst -= (src.top * blit.dstRowBytes);
        blit.dst -= SW_PIXELBYTES(dstFrameP, src.left);
        src.left = SW_PIXELBYTES(dstFrameP, src.left);
        src.right = SW_PIXELBYTES(dstFrameP, src.right);
    }
    blit.clip = src;
    return true;
}

} // namespace

// FUNCTION: EVNOVA 0x004703e0
// MAC: _BlitPixieRectDrawProc i386 0x000ba16f ppc 0x000c6154
void BlitPixieRectDrawProc(SWFrameRec *srcFrameP, SWFrameRec *dstFrameP,
                           Rect *srcRect, Rect *dstRect)
{
    RectBlit blit;
    if (!PrepareRectBlit(srcFrameP, dstFrameP, srcRect, dstRect, 155, false, true, blit))
        return;

    BlitPixieRect(
        (unsigned char *)SW_PIXELPTR(srcFrameP, blit.src.left, blit.src.top),
        (unsigned char *)SW_PIXELPTR(dstFrameP, blit.dst.left, blit.dst.top),
        blit.srcRowBytes,
        blit.dstRowBytes,
        SW_PIXELBYTES(dstFrameP, blit.width),
        blit.rows);
}

// FUNCTION: EVNOVA 0x00470960
// MAC: _BlitPixieMaskDrawProc i386 0x000ba6c5 ppc 0x000c6644
void BlitPixieMaskDrawProc(SWFrameRec *srcFrameP, SWFrameRec *dstFrameP,
                           Rect *srcRect, Rect *dstRect)
{
    RectBlit blit;
    if (!PrepareRectBlit(srcFrameP, dstFrameP, srcRect, dstRect, 283, true, true, blit))
        return;

    int32_t srcBaseOffset = SW_ROWBYTES(srcFrameP, blit.src.top) + SW_PIXELBYTES(srcFrameP, blit.src.left);

    BlitPixieMask(
        (unsigned char *)(srcFrameP->frameBaseAddr + srcBaseOffset),
        (unsigned char *)SW_PIXELPTR(dstFrameP, blit.dst.left, blit.dst.top),
        (unsigned char *)(srcFrameP->maskBaseAddr + srcBaseOffset),
        blit.srcRowBytes,
        blit.dstRowBytes,
        SW_PIXELBYTES(dstFrameP, blit.width),
        blit.rows);
}

// FUNCTION: EVNOVA 0x00470c20
// MAC: _BlitPixiePartialMaskDrawProc i386 0x000ba98b ppc 0x000c68c4
void BlitPixiePartialMaskDrawProc(SWFrameRec *srcFrameP, SWFrameRec *dstFrameP,
                                  Rect *srcRect, Rect *dstRect)
{
    RectBlit blit;
    if (!PrepareRectBlit(srcFrameP, dstFrameP, srcRect, dstRect, 355, true, true, blit))
        return;

    int32_t srcBaseOffset = SW_ROWBYTES(srcFrameP, blit.src.top) + SW_PIXELBYTES(srcFrameP, blit.src.left);

    BlitPixiePartialMask(
        (unsigned char *)(srcFrameP->frameBaseAddr + srcBaseOffset),
        (unsigned char *)SW_PIXELPTR(dstFrameP, blit.dst.left, blit.dst.top),
        (unsigned char *)(srcFrameP->maskBaseAddr + srcBaseOffset),
        blit.srcRowBytes,
        blit.dstRowBytes,
        SW_PIXELBYTES(dstFrameP, blit.width),
        blit.rows);
}

// FUNCTION: EVNOVA 0x00470ee0
// MAC: _BlitPixieRLEDrawProc i386 0x000bb12d ppc 0x000c6fc4
void BlitPixieRLEDrawProc(SWFrameRec *srcFrameP, SWFrameRec *dstFrameP,
                          Rect *srcRect, Rect *dstRect)
{
    RLEBlit blit;
    if (!PrepareRLEBlit(srcFrameP, dstFrameP, srcRect, dstRect, 565, blit))
        return;

    if (blit.clipped)
        BlitPixieRLEClipped(blit.tokens, blit.dst, blit.dstRowBytes, &blit.clip);
    else
        BlitPixieRLE(blit.tokens, blit.dst, blit.dstRowBytes);
}

// FUNCTION: EVNOVA 0x004711e0
// MAC: _BlitPixieRLETranslucentDrawProc i386 0x000bb3fc ppc 0x000c7234
void BlitPixieRLETranslucentDrawProc(SWFrameRec *srcFrameP, SWFrameRec *dstFrameP,
                                     Rect *srcRect, Rect *dstRect)
{
    RLEBlit blit;
    if (!PrepareRLEBlit(srcFrameP, dstFrameP, srcRect, dstRect, 640, blit))
        return;

    SpriteBlend blend = SpriteBlend::Of(*gSWCurrentSpriteBeingDrawn);
    switch (blend.kind) {
    case SpriteBlend::Kind::Plain:
        if (blit.clipped)
            BlitPixieRLEClipped(blit.tokens, blit.dst, blit.dstRowBytes, &blit.clip);
        else
            BlitPixieRLE(blit.tokens, blit.dst, blit.dstRowBytes);
        break;
    case SpriteBlend::Kind::AddOver:
        if (blit.clipped)
            BlitPixieRLEAddOverClipped(blit.tokens, blit.dst, blit.dstRowBytes, &blit.clip);
        else
            BlitPixieRLEAddOver(blit.tokens, blit.dst, blit.dstRowBytes);
        break;
    case SpriteBlend::Kind::Translucent:
        if (blit.clipped)
            BlitPixieRLETranslucentClipped(blit.tokens, blit.dst, blit.dstRowBytes, &blit.clip, blend);
        else
            BlitPixieRLETranslucent(blit.tokens, blit.dst, blit.dstRowBytes, blend);
        break;
    case SpriteBlend::Kind::Hidden:
        break;
    }
}

// FUNCTION: EVNOVA 0x00471800
// MAC: _BlitPixieRLECollisionProc i386 0x000bba11 ppc 0x000c7744
Boolean BlitPixieRLECollisionProc(SpritePtr srcSpriteP, SpritePtr dstSpriteP,
                               Rect *srcRect, Rect *dstRect)
{
    Boolean collision;
    SWFrameRec *srcFrameP = srcSpriteP->curFrameP;
    SWFrameRec *dstFrameP = dstSpriteP->curFrameP;

    if (!(srcFrameP->isFrameLocked && dstFrameP->isFrameLocked))
        SW_ASSERT_FAIL("BlitPixieInterface.c", 0x333, 821);
    if (!(srcFrameP->rleTokenStart != 0 && dstFrameP->rleTokenStart != 0))
        SW_ASSERT_FAIL("BlitPixieInterface.c", 0x334, 822);

    if (!(srcRect->right - srcRect->left == dstRect->right - dstRect->left))
        SW_ASSERT_FAIL("BlitPixieInterface.c", 0x336, 824);
    if (!(srcRect->bottom - srcRect->top == dstRect->bottom - dstRect->top))
        SW_ASSERT_FAIL("BlitPixieInterface.c", 0x337, 825);

    srcRect->left = SW_PIXELBYTES(srcFrameP, srcRect->left);
    srcRect->right = SW_PIXELBYTES(srcFrameP, srcRect->right);
    dstRect->left = SW_PIXELBYTES(dstFrameP, dstRect->left);
    dstRect->right = SW_PIXELBYTES(dstFrameP, dstRect->right);

    collision = BlitPixieRLECollision(
                    (unsigned char *)srcFrameP->rleTokenStart,
                    (unsigned char *)dstFrameP->rleTokenStart,
                    srcRect,
                    dstRect);

    return collision;
}

// FUNCTION: EVNOVA 0x00471920
// MAC: _BlitPixieRLEPointCollisionProc i386 0x000bbb59 ppc 0x000c7888
Boolean BlitPixieRLEPointCollisionProc(SpritePtr sprite, Point *pt, Rect *rect)
{
    Rect r;
    Point p;
    SWFrameRec *fr;

    fr = sprite->curFrameP;
    if (!fr->isFrameLocked)
        SW_ASSERT_FAIL("BlitPixieInterface.c", 0x356, 856);
    if (fr->rleTokenStart == 0)
        SW_ASSERT_FAIL("BlitPixieInterface.c", 0x357, 857);
    r.left = rect->left << fr->pixelShift;
    r.right = rect->right << fr->pixelShift;
    r.top = rect->top;
    r.bottom = rect->bottom;
    p.h = pt->h << fr->pixelShift;
    p.v = pt->v;
    return BlitPixieRLECollisionWithPoint((unsigned char *)fr->rleTokenStart, &p, &r);
}


// FUNCTION: EVNOVA 0x004719d0
// MAC: _SWGetValueFromColor i386 0x000bd4a0 ppc 0x000c8e14
uint32_t SWGetValueFromColor(FramePtr dstFrameP, RGBColor *color)
{
    CGrafPtr saveGWorld;
    void *saveGDevice;
    uint32_t pixelValue;

    if (dstFrameP == 0)
        SW_ASSERT_FAIL("BlitPixieInterface.c", 1929, 1927);
    if (dstFrameP->framePort == 0)
        SW_ASSERT_FAIL("BlitPixieInterface.c", 1930, 1928);

    GetPort(&saveGWorld);
    saveGDevice = GetGWorldDevice(saveGWorld);
    SetPort(dstFrameP->framePort);

    pixelValue = Color2Index(color);

    if (dstFrameP->frameDepth < 32)
        pixelValue |= pixelValue << 16;

    SetPort(saveGWorld);
    return pixelValue;
}

