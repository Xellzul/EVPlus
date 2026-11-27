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
