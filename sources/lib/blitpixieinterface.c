// COMPILE: -lang c++ -bool off
/* blitpixieinterface.c -- SpriteWorld's BlitPixieInterface.c, tu-map object O12: the sprite draw and collision procs. Notes: Research/notes/blitpixieinterface.md */
#include "macport.h"
#include "shipobj.h"
#include "spriteworld.h"


// FUNCTION: EVNOVA 0x004703e0
// MAC: _BlitPixieRectDrawProc i386 0x000ba16f ppc 0x000c6154
void BlitPixieRectDrawProc(SWFrameRec *srcFrameP, SWFrameRec *dstFrameP,
                           Rect *srcRect, Rect *dstRect)
{
    Rect *frameRectP = &dstFrameP->frameRect;
    Rect srcBlitRect = *srcRect;
    Rect dstBlitRect = *dstRect;
    int32_t srcRowBytes = srcFrameP->frameRowBytes;
    int32_t dstRowBytes = dstFrameP->frameRowBytes;
    int numPixelsPerRow, numRowsToCopy;

    if (srcFrameP->framePort == 0)
        SW_ASSERT_FAIL("BlitPixieInterface.c", 155, 157);
    if (dstFrameP->framePort == 0)
        SW_ASSERT_FAIL("BlitPixieInterface.c", 156, 158);
    if (!(srcFrameP->isFrameLocked && dstFrameP->isFrameLocked))
        SW_ASSERT_FAIL("BlitPixieInterface.c", 157, 159);
    if (!(dstFrameP->frameDepth >= 8))
        SW_ASSERT_FAIL("BlitPixieInterface.c", 158, 160);
    if (!(srcFrameP->frameDepth == dstFrameP->frameDepth))
        SW_ASSERT_FAIL("BlitPixieInterface.c", 159, 161);

    BP_CLIP_RECT(frameRectP, srcBlitRect, dstBlitRect);

    numPixelsPerRow = dstBlitRect.right - dstBlitRect.left;
    numRowsToCopy = dstBlitRect.bottom - dstBlitRect.top;

    if (dstFrameP->interlacingIsOn)
    {
        if (((dstBlitRect.top - frameRectP->top) & 1) ==
                (unsigned char)dstFrameP->skipOddLines)
        {
            srcBlitRect.top++;
            dstBlitRect.top++;
            numRowsToCopy--;
        }

        numRowsToCopy = (numRowsToCopy >> 1) + (numRowsToCopy & 1);

        srcRowBytes += srcRowBytes;
        dstRowBytes += dstRowBytes;
    }

    if (numPixelsPerRow <= 0)
        return;
    if (numRowsToCopy <= 0)
        return;

    BlitPixieRect(
        (unsigned char *)SW_PIXELPTR(srcFrameP, srcBlitRect.left, srcBlitRect.top),
        (unsigned char *)SW_PIXELPTR(dstFrameP, dstBlitRect.left, dstBlitRect.top),
        srcRowBytes,
        dstRowBytes,
        SW_PIXELBYTES(dstFrameP, numPixelsPerRow),
        numRowsToCopy);
}

/* backgroundValue was the 8-bit blitter's palette index to blend toward; the 16-bit one is
   handed `32 - level', an alpha weight out of 32. */

// FUNCTION: EVNOVA 0x00470680
// MAC: _BlitPixieFloatingRectDrawProc i386 0x000ba405 ppc 0x000c63a4
void BlitPixieFloatingRectDrawProc(SWFrameRec *srcFrameP, SWFrameRec *dstFrameP,
                                   Rect *srcRect, Rect *dstRect, short level,
                                   int32_t backgroundValue)
{
    Rect *frameRectP = &dstFrameP->frameRect;
    Rect srcBlitRect = *srcRect;
    Rect dstBlitRect = *dstRect;
    int32_t srcRowBytes = srcFrameP->frameRowBytes;
    int32_t dstRowBytes = dstFrameP->frameRowBytes;
    int numPixelsPerRow, numRowsToCopy;

    if (srcFrameP->framePort == 0)
        SW_ASSERT_FAIL("BlitPixieInterface.c", 222, 224);
    if (dstFrameP->framePort == 0)
        SW_ASSERT_FAIL("BlitPixieInterface.c", 223, 225);
    if (!(srcFrameP->isFrameLocked && dstFrameP->isFrameLocked))
        SW_ASSERT_FAIL("BlitPixieInterface.c", 224, 226);
    if (!(dstFrameP->frameDepth >= 8))
        SW_ASSERT_FAIL("BlitPixieInterface.c", 225, 227);
    if (!(srcFrameP->frameDepth == dstFrameP->frameDepth))
        SW_ASSERT_FAIL("BlitPixieInterface.c", 226, 228);

    BP_CLIP_RECT(frameRectP, srcBlitRect, dstBlitRect);

    numPixelsPerRow = dstBlitRect.right - dstBlitRect.left;
    numRowsToCopy = dstBlitRect.bottom - dstBlitRect.top;

    if (numPixelsPerRow <= 0)
        return;
    if (numRowsToCopy <= 0)
        return;

    BlitPixieFloatingRect(
        (unsigned char *)SW_PIXELPTR(srcFrameP, srcBlitRect.left, srcBlitRect.top),
        (unsigned char *)SW_PIXELPTR(dstFrameP, dstBlitRect.left, dstBlitRect.top),
        srcRowBytes,
        dstRowBytes,
        SW_PIXELBYTES(dstFrameP, numPixelsPerRow),
        numRowsToCopy,
        32 - level);
}

// FUNCTION: EVNOVA 0x00470960
// MAC: _BlitPixieMaskDrawProc i386 0x000ba6c5 ppc 0x000c6644
void BlitPixieMaskDrawProc(SWFrameRec *srcFrameP, SWFrameRec *dstFrameP,
                           Rect *srcRect, Rect *dstRect)
{
    Rect *frameRectP = &dstFrameP->frameRect;
    Rect srcBlitRect = *srcRect;
    Rect dstBlitRect = *dstRect;
    uint32_t srcRowBytes = srcFrameP->frameRowBytes;
    uint32_t dstRowBytes = dstFrameP->frameRowBytes;
    int32_t srcBaseOffset;
    int numPixelsPerRow, numRowsToCopy;

    if (srcFrameP->framePort == 0)
        SW_ASSERT_FAIL("BlitPixieInterface.c", 283, 285);
    if (srcFrameP->maskPort == 0)
        SW_ASSERT_FAIL("BlitPixieInterface.c", 284, 286);
    if (dstFrameP->framePort == 0)
        SW_ASSERT_FAIL("BlitPixieInterface.c", 285, 287);
    if (!(srcFrameP->isFrameLocked && dstFrameP->isFrameLocked))
        SW_ASSERT_FAIL("BlitPixieInterface.c", 286, 288);
    if (!(dstFrameP->frameDepth >= 8))
        SW_ASSERT_FAIL("BlitPixieInterface.c", 287, 289);
    if (!(srcFrameP->frameDepth == dstFrameP->frameDepth))
        SW_ASSERT_FAIL("BlitPixieInterface.c", 288, 290);

    BP_CLIP_RECT(frameRectP, srcBlitRect, dstBlitRect);

    numPixelsPerRow = dstBlitRect.right - dstBlitRect.left;
    numRowsToCopy = dstBlitRect.bottom - dstBlitRect.top;

    if (dstFrameP->interlacingIsOn)
    {
        if (((dstBlitRect.top - frameRectP->top) & 1) ==
                (unsigned char)dstFrameP->skipOddLines)
        {
            srcBlitRect.top++;
            dstBlitRect.top++;
            numRowsToCopy--;
        }

        numRowsToCopy = (numRowsToCopy >> 1) + (numRowsToCopy & 1);

        srcRowBytes += srcRowBytes;
        dstRowBytes += dstRowBytes;
    }

    if (numPixelsPerRow <= 0)
        return;
    if (numRowsToCopy <= 0)
        return;

    srcBaseOffset = SW_ROWBYTES(srcFrameP, srcBlitRect.top) +
                    SW_PIXELBYTES(srcFrameP, srcBlitRect.left);

    BlitPixieMask(
        (unsigned char *)(srcFrameP->frameBaseAddr + srcBaseOffset),
        (unsigned char *)SW_PIXELPTR(dstFrameP, dstBlitRect.left, dstBlitRect.top),
        (unsigned char *)(srcFrameP->maskBaseAddr + srcBaseOffset),
        srcRowBytes,
        dstRowBytes,
        SW_PIXELBYTES(dstFrameP, numPixelsPerRow),
        numRowsToCopy);
}

// FUNCTION: EVNOVA 0x00470c20
// MAC: _BlitPixiePartialMaskDrawProc i386 0x000ba98b ppc 0x000c68c4
void BlitPixiePartialMaskDrawProc(SWFrameRec *srcFrameP, SWFrameRec *dstFrameP,
                                  Rect *srcRect, Rect *dstRect)
{
    Rect *frameRectP = &dstFrameP->frameRect;
    Rect srcBlitRect = *srcRect;
    Rect dstBlitRect = *dstRect;
    int32_t srcRowBytes = srcFrameP->frameRowBytes;
    int32_t dstRowBytes = dstFrameP->frameRowBytes;
    int32_t srcBaseOffset;
    int numPixelsPerRow, numRowsToCopy;

    if (srcFrameP->framePort == 0)
        SW_ASSERT_FAIL("BlitPixieInterface.c", 355, 357);
    if (srcFrameP->maskPort == 0)
        SW_ASSERT_FAIL("BlitPixieInterface.c", 356, 358);
    if (dstFrameP->framePort == 0)
        SW_ASSERT_FAIL("BlitPixieInterface.c", 357, 359);
    if (!(srcFrameP->isFrameLocked && dstFrameP->isFrameLocked))
        SW_ASSERT_FAIL("BlitPixieInterface.c", 358, 360);
    if (!(dstFrameP->frameDepth >= 8))
        SW_ASSERT_FAIL("BlitPixieInterface.c", 359, 361);
    if (!(srcFrameP->frameDepth == dstFrameP->frameDepth))
        SW_ASSERT_FAIL("BlitPixieInterface.c", 360, 362);

    BP_CLIP_RECT(frameRectP, srcBlitRect, dstBlitRect);

    numPixelsPerRow = dstBlitRect.right - dstBlitRect.left;
    numRowsToCopy = dstBlitRect.bottom - dstBlitRect.top;

    if (dstFrameP->interlacingIsOn)
    {
        if (((dstBlitRect.top - frameRectP->top) & 1) ==
                (unsigned char)dstFrameP->skipOddLines)
        {
            srcBlitRect.top++;
            dstBlitRect.top++;
            numRowsToCopy--;
        }

        numRowsToCopy = (numRowsToCopy >> 1) + (numRowsToCopy & 1);

        srcRowBytes += srcRowBytes;
        dstRowBytes += dstRowBytes;
    }

    if (numPixelsPerRow <= 0)
        return;
    if (numRowsToCopy <= 0)
        return;

    srcBaseOffset = SW_ROWBYTES(srcFrameP, srcBlitRect.top) +
                    SW_PIXELBYTES(srcFrameP, srcBlitRect.left);

    BlitPixiePartialMask(
        (unsigned char *)(srcFrameP->frameBaseAddr + srcBaseOffset),
        (unsigned char *)SW_PIXELPTR(dstFrameP, dstBlitRect.left, dstBlitRect.top),
        (unsigned char *)(srcFrameP->maskBaseAddr + srcBaseOffset),
        srcRowBytes,
        dstRowBytes,
        SW_PIXELBYTES(dstFrameP, numPixelsPerRow),
        numRowsToCopy);
}


/* The clipped variant gets srcBlitRect converted from pixels to BYTES first, and the
   conversion uses the DESTINATION frame's pixelShift on the SOURCE rect -- that is the
   donor's own text, not a transcription slip. */

// FUNCTION: EVNOVA 0x00470ee0
// MAC: _BlitPixieRLEDrawProc i386 0x000bb12d ppc 0x000c6fc4
void BlitPixieRLEDrawProc(SWFrameRec *srcFrameP, SWFrameRec *dstFrameP,
                          Rect *srcRect, Rect *dstRect)
{
    Rect *frameRectP = &dstFrameP->frameRect;
    Rect srcBlitRect = *srcRect;
    Rect dstBlitRect = *dstRect;
    unsigned char *dstPixel;
    uint32_t dstRowBytes;

    if (srcFrameP->rleTokenStart == 0)
        SW_ASSERT_FAIL("BlitPixieInterface.c", 565, 567);
    if (dstFrameP->framePort == 0)
        SW_ASSERT_FAIL("BlitPixieInterface.c", 566, 568);
    if (!(srcFrameP->isFrameLocked && dstFrameP->isFrameLocked))
        SW_ASSERT_FAIL("BlitPixieInterface.c", 567, 569);
    if (!(srcFrameP->frameDepth >= 8))
        SW_ASSERT_FAIL("BlitPixieInterface.c", 568, 570);

    if (dstBlitRect.left > frameRectP->right ||
        dstBlitRect.right < frameRectP->left ||
        dstBlitRect.bottom < frameRectP->top ||
        dstBlitRect.top > frameRectP->bottom)
        return;

    srcBlitRect.right -= srcBlitRect.left;
    srcBlitRect.bottom -= srcBlitRect.top;
    srcBlitRect.left = 0;
    srcBlitRect.top = 0;
    BP_CLIP_RECT(frameRectP, srcBlitRect, dstBlitRect);

    if (dstBlitRect.left >= dstBlitRect.right)
        return;
    if (dstBlitRect.top >= dstBlitRect.bottom)
        return;

    dstPixel = (unsigned char *)SW_PIXELPTR(dstFrameP, dstBlitRect.left, dstBlitRect.top);
    dstRowBytes = dstFrameP->frameRowBytes;

    if (srcBlitRect.right - srcBlitRect.left !=
                srcFrameP->frameRect.right - srcFrameP->frameRect.left ||
        srcBlitRect.bottom - srcBlitRect.top !=
                srcFrameP->frameRect.bottom - srcFrameP->frameRect.top)
    {
        dstPixel -= (srcBlitRect.top * dstRowBytes);
        dstPixel -= SW_PIXELBYTES(dstFrameP, srcBlitRect.left);

        srcBlitRect.left = SW_PIXELBYTES(dstFrameP, srcBlitRect.left);
        srcBlitRect.right = SW_PIXELBYTES(dstFrameP, srcBlitRect.right);

        BlitPixieRLEClipped(
            (unsigned char *)srcFrameP->rleTokenStart,
            dstPixel,
            dstRowBytes,
            &srcBlitRect);
    }
    else
    {
        BlitPixieRLE(
            (unsigned char *)srcFrameP->rleTokenStart,
            dstPixel,
            dstRowBytes);
    }
}

/* Sprite fields, described not recovered: +0xa2 is a translucency out of 64 (compared
   against 0, 32 and 64), +0xa4/+0xa6/+0xa8 three 5-bit tint scales compared against 32,
   +0xaa only tested against 0. */

// FUNCTION: EVNOVA 0x004711e0
// MAC: _BlitPixieRLETranslucentDrawProc i386 0x000bb3fc ppc 0x000c7234
void BlitPixieRLETranslucentDrawProc(SWFrameRec *srcFrameP, SWFrameRec *dstFrameP,
                                     Rect *srcRect, Rect *dstRect)
{
    Rect *frameRectP = &dstFrameP->frameRect;
    Rect srcBlitRect = *srcRect;
    Rect dstBlitRect = *dstRect;
    unsigned char *dstPixel;
    uint32_t dstRowBytes;

    if (srcFrameP->rleTokenStart == 0)
        SW_ASSERT_FAIL("BlitPixieInterface.c", 640, 642);
    if (dstFrameP->framePort == 0)
        SW_ASSERT_FAIL("BlitPixieInterface.c", 641, 643);
    if (!(srcFrameP->isFrameLocked && dstFrameP->isFrameLocked))
        SW_ASSERT_FAIL("BlitPixieInterface.c", 642, 644);
    if (!(srcFrameP->frameDepth >= 8))
        SW_ASSERT_FAIL("BlitPixieInterface.c", 643, 645);

    if (dstBlitRect.left > frameRectP->right ||
        dstBlitRect.right < frameRectP->left ||
        dstBlitRect.bottom < frameRectP->top ||
        dstBlitRect.top > frameRectP->bottom)
        return;

    srcBlitRect.right -= srcBlitRect.left;
    srcBlitRect.bottom -= srcBlitRect.top;
    srcBlitRect.left = 0;
    srcBlitRect.top = 0;
    BP_CLIP_RECT(frameRectP, srcBlitRect, dstBlitRect);

    if (dstBlitRect.left >= dstBlitRect.right)
        return;
    if (dstBlitRect.top >= dstBlitRect.bottom)
        return;

    dstPixel = (unsigned char *)SW_PIXELPTR(dstFrameP, dstBlitRect.left, dstBlitRect.top);
    dstRowBytes = dstFrameP->frameRowBytes;

    if (srcBlitRect.right - srcBlitRect.left !=
                srcFrameP->frameRect.right - srcFrameP->frameRect.left ||
        srcBlitRect.bottom - srcBlitRect.top !=
                srcFrameP->frameRect.bottom - srcFrameP->frameRect.top)
    {
        dstPixel -= (srcBlitRect.top * dstRowBytes);
        dstPixel -= SW_PIXELBYTES(dstFrameP, srcBlitRect.left);

        srcBlitRect.left = SW_PIXELBYTES(dstFrameP, srcBlitRect.left);
        srcBlitRect.right = SW_PIXELBYTES(dstFrameP, srcBlitRect.right);

        if (gSWCurrentSpriteBeingDrawn->translucency > 0 ||
            gSWCurrentSpriteBeingDrawn->tintMode > 0 ||
            gSWCurrentSpriteBeingDrawn->tintRed != 32 ||
            gSWCurrentSpriteBeingDrawn->tintGreen != 32 ||
            gSWCurrentSpriteBeingDrawn->tintBlue != 32)
        {
            if (gSWCurrentSpriteBeingDrawn->tintRed == 32 &&
                gSWCurrentSpriteBeingDrawn->tintGreen == 32 &&
                gSWCurrentSpriteBeingDrawn->tintBlue == 32 &&
                gSWCurrentSpriteBeingDrawn->translucency == 32 &&
                gSWCurrentSpriteBeingDrawn->tintMode == 0)
                BlitPixieRLEAddOverClipped((unsigned char *)srcFrameP->rleTokenStart,
                        dstPixel, dstRowBytes, &srcBlitRect);
            else if (gSWCurrentSpriteBeingDrawn->translucency < 64)
                BlitPixieRLETranslucentClipped((unsigned char *)srcFrameP->rleTokenStart,
                        dstPixel, dstRowBytes, &srcBlitRect);
        }
        else
        {
            BlitPixieRLEClipped((unsigned char *)srcFrameP->rleTokenStart,
                    dstPixel, dstRowBytes, &srcBlitRect);
        }
    }
    else
    {
        if (gSWCurrentSpriteBeingDrawn->translucency > 0 ||
            gSWCurrentSpriteBeingDrawn->tintMode > 0 ||
            gSWCurrentSpriteBeingDrawn->tintRed != 32 ||
            gSWCurrentSpriteBeingDrawn->tintGreen != 32 ||
            gSWCurrentSpriteBeingDrawn->tintBlue != 32)
        {
            if (gSWCurrentSpriteBeingDrawn->tintRed == 32 &&
                gSWCurrentSpriteBeingDrawn->tintGreen == 32 &&
                gSWCurrentSpriteBeingDrawn->tintBlue == 32 &&
                gSWCurrentSpriteBeingDrawn->translucency == 32 &&
                gSWCurrentSpriteBeingDrawn->tintMode == 0)
                BlitPixieRLEAddOver((unsigned char *)srcFrameP->rleTokenStart,
                        dstPixel, dstRowBytes);
            else if (gSWCurrentSpriteBeingDrawn->translucency < 64)
                BlitPixieRLETranslucent((unsigned char *)srcFrameP->rleTokenStart,
                        dstPixel, dstRowBytes);
        }
        else
        {
            BlitPixieRLE((unsigned char *)srcFrameP->rleTokenStart,
                    dstPixel, dstRowBytes);
        }
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

