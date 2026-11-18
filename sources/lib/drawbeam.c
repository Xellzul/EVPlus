// COMPILE: -lang c++ -bool off
/* drawbeam.c -- the SWBeams.c run from DrawTranslucentLine to EraseLightningBeam (win 0x00479590-0x0047b399). Notes: Research/notes/drawbeam.md */
#include <stdlib.h>
#include "macport.h"
#include "spriteworld.h"

// FUNCTION: EVNOVA 0x00479590
// MAC: _DrawTranslucentLine i386 0x000c3259 ppc 0x000cf604
void DrawTranslucentLine(FramePtr dstFrameP, Point srcPoint, Point dstPoint,
                uint32_t color, short srcShade, short dstShade,
                short srcRamp, short dstRamp)
{
    Boolean thereIsALine;
    Boolean longerHoriz;
    Point origSrc, origDst;

    if (dstFrameP == 0)
        return;

    if (dstFrameP == 0 || !dstFrameP->isFrameLocked)
        SW_ASSERT_FAIL("SWBeams.c", 23, 24);

    origSrc = srcPoint;
    origDst = dstPoint;

    thereIsALine = SWClipLineWithRect(&srcPoint, &dstPoint, &dstFrameP->frameRect);

    if (thereIsALine)
    {
        if (ABS(srcPoint.h - dstPoint.h) > ABS(srcPoint.v - dstPoint.v))
            longerHoriz = 1;
        else
            longerHoriz = 0;

        if (longerHoriz)
        {
            srcRamp -= ABS(srcPoint.h - origSrc.h);
            dstRamp -= ABS(dstPoint.h - origDst.h);
        }
        else
        {
            srcRamp -= ABS(srcPoint.v - origSrc.v);
            dstRamp -= ABS(dstPoint.v - origDst.v);
        }

        if (srcShade < 0)
            srcShade = 0;
        if (srcShade > 32)
            srcShade = 32;

        if (dstShade < 0)
            dstShade = 0;
        if (dstShade > 32)
            dstShade = 32;

        if (srcRamp < 0)
            srcRamp = 0;
        if (srcRamp > 32)
            srcRamp = 32;

        if (dstRamp < 0)
            dstRamp = 0;
        if (dstRamp > 32)
            dstRamp = 32;

        BlitTranslucentLine(dstFrameP, srcPoint, dstPoint, color,
                 srcShade, dstShade, srcRamp, dstRamp);
    }
}

// FUNCTION: EVNOVA 0x004797b0
// MAC: _BlitTranslucentLine i386 0x000c2c90 ppc 0x000cf0a0
void BlitTranslucentLine(FramePtr dstFrameP, Point srcPoint, Point dstPoint,
                         uint32_t color, int32_t srcShade, int32_t dstShade,
                         uint32_t srcRamp, uint32_t dstRamp)
{
    uint32_t   alpha;
    short           x1, y1, x2, y2;
    short           dx, dy, sx, sy;
    short           accum;
    short           curX, curY;
    short           shade;

    x1 = srcPoint.h;
    y1 = srcPoint.v;
    x2 = dstPoint.h;
    y2 = dstPoint.v;

    dx = x2 - x1;
    dy = y2 - y1;

    sx = SIGN(dx);
    sy = SIGN(dy);

    dx = ABS(dx);
    dy = ABS(dy);

    curX = x1;
    curY = y1;

    shade = srcShade;
    alpha = srcShade;
    if (srcRamp != 0)
        alpha = dstShade;

    if (alpha != 0)
        *SW_PIXELPTR16(dstFrameP, curX, curY) =
            Trans16Bit(color, *SW_PIXELPTR16(dstFrameP, curX, curY), alpha);

    if (dx > dy)
    {
        accum = dx >> 1;
        while (x1 != x2)
        {
            accum -= dy;
            if (accum < 0)
            {
                accum += dx;
                curY += sy;
            }
            x1 += sx;
            curX += sx;

            if (ABS(x1 - srcPoint.h) < srcRamp)
            {
                if (alpha < shade)
                    alpha++;
            }
            else if (ABS(x1 - dstPoint.h) < dstRamp)
            {
                if (alpha == 0)
                    return;
                alpha--;
            }
            else if (alpha != shade)
                alpha = shade;

            if (alpha != 0)
                *SW_PIXELPTR16(dstFrameP, curX, curY) =
                    Trans16Bit(color, *SW_PIXELPTR16(dstFrameP, curX, curY), alpha);
        }
    }
    else
    {
        accum = dy >> 1;
        while (y1 != y2)
        {
            accum -= dx;
            if (accum < 0)
            {
                accum += dy;
                curX += sx;
            }
            y1 += sy;
            curY += sy;

            if (ABS(y1 - srcPoint.v) < srcRamp)
            {
                if (alpha < shade)
                    alpha++;
            }
            else if (ABS(y1 - dstPoint.v) < dstRamp)
            {
                if (alpha == 0)
                    return;
                alpha--;
            }
            else if (alpha != shade)
                alpha = shade;

            if (alpha != 0)
                *SW_PIXELPTR16(dstFrameP, curX, curY) =
                    Trans16Bit(color, *SW_PIXELPTR16(dstFrameP, curX, curY), alpha);
        }
    }
}

// FUNCTION: EVNOVA 0x00479c50
// MAC: _CopyLine i386 0x000c3936 ppc 0x000cfc64
void CopyLine(FramePtr srcFrameP, FramePtr dstFrameP, Point srcPoint, Point dstPoint)
{
    Boolean thereIsALine;

    if (srcFrameP == 0 || !srcFrameP->isFrameLocked)
        SW_ASSERT_FAIL("SWBeams.c", 214, 211);
    if (dstFrameP == 0 || !dstFrameP->isFrameLocked)
        SW_ASSERT_FAIL("SWBeams.c", 215, 212);

    if (srcPoint.h == dstPoint.h && srcPoint.v == dstPoint.v)
        return;

    thereIsALine = SWClipLineWithRect(&srcPoint, &dstPoint, &dstFrameP->frameRect);

    if (thereIsALine)
        BlitCopyLine(srcFrameP, dstFrameP, srcPoint, dstPoint);
}

// FUNCTION: EVNOVA 0x00479cf0
// MAC: _BlitCopyLine i386 0x000c3483 ppc 0x000cf7f4
void BlitCopyLine(FramePtr srcFrameP, FramePtr dstFrameP, Point srcPoint, Point dstPoint)
{
    short   x1, y1, x2, y2;
    short   dx, dy, sx, sy;
    short   accum;
    short   curX, curY;
    short   DEPTH;

    x1 = srcPoint.h;
    y1 = srcPoint.v;
    x2 = dstPoint.h;
    y2 = dstPoint.v;

    if (x1 == x2 && y1 == y2)
        return;

    DEPTH = 16;

    dx = x2 - x1;
    dy = y2 - y1;

    sx = SIGN(dx);
    sy = SIGN(dy);

    dx = ABS(dx);
    dy = ABS(dy);

    curX = x1;
    curY = y1;

    SET_DEPTH_PIXEL(dstFrameP,x1, y1, GET_DEPTH_PIXEL(srcFrameP,x1, y1))

    if (dx > dy)
    {
        accum = dx >> 1;
        while (x1 != x2)
        {
            accum -= dy;
            if (accum < 0)
            {
                accum += dx;
                curY += sy;
            }
            x1 += sx;
            curX += sx;
            SET_DEPTH_PIXEL(dstFrameP,curX, curY, GET_DEPTH_PIXEL(srcFrameP,curX, curY))
        }
    }
    else
    {
        accum = dy >> 1;
        while (y1 != y2)
        {
            accum -= dx;
            if (accum < 0)
            {
                accum += dy;
                curX += sx;
            }
            y1 += sy;
            curY += sy;
            SET_DEPTH_PIXEL(dstFrameP,curX, curY, GET_DEPTH_PIXEL(srcFrameP,curX, curY))
        }
    }
}


// FUNCTION: EVNOVA 0x00479fe0
// MAC: _DrawBeam i386 0x000c3b3c ppc 0x000cfe70
void DrawBeam(FramePtr dstFrameP, Point srcPoint, Point dstPoint, uint32_t color,
              short halfWidth, uint32_t coronaColor, short fadeStep)
{
    Point   ptA;
    Point   ptB;
    Boolean longerHoriz;
    short   i;
    short   shade;
    Rect    clipRect;

    if (srcPoint.h == dstPoint.h && srcPoint.v == dstPoint.v)
        return;

    clipRect = dstFrameP->frameRect;

    InsetRect(&clipRect, -32, -32);

    if (srcPoint.h < clipRect.left && dstPoint.h < clipRect.left)
        return;
    if (srcPoint.h > clipRect.right && dstPoint.h > clipRect.right)
        return;
    if (srcPoint.v < clipRect.top && dstPoint.v < clipRect.top)
        return;
    if (srcPoint.v > clipRect.bottom && dstPoint.v > clipRect.bottom)
        return;

    if (ABS(srcPoint.h - dstPoint.h) > ABS(srcPoint.v - dstPoint.v))
        longerHoriz = 1;
    else
        longerHoriz = 0;

    for (i = 0; i < halfWidth; i++)
    {
        shade = 32 - i * 16;

        if (longerHoriz)
        {
            ptA.h = srcPoint.h;
            ptA.v = srcPoint.v - i;
        }
        else
        {
            ptA.h = srcPoint.h - i;
            ptA.v = srcPoint.v;
        }
        ptB.h = dstPoint.h + ptA.h - srcPoint.h;
        ptB.v = dstPoint.v + ptA.v - srcPoint.v;
        DrawTranslucentLine(dstFrameP, ptA, ptB, color, shade, shade, 0, 20 - i * 10);

        if (i > 0)
        {
            if (longerHoriz)
            {
                ptA.h = srcPoint.h;
                ptA.v = srcPoint.v + i;
            }
            else
            {
                ptA.h = srcPoint.h + i;
                ptA.v = srcPoint.v;
            }
            ptB.h = dstPoint.h + ptA.h - srcPoint.h;
            ptB.v = dstPoint.v + ptA.v - srcPoint.v;
            DrawTranslucentLine(dstFrameP, ptA, ptB, color, shade, shade, 0, 20 - i * 10);
        }
    }

    i = 0;
    shade = 16;
    if (fadeStep > 0)
    {
        do
        {
            if (longerHoriz)
            {
                ptA.h = srcPoint.h;
                ptA.v = srcPoint.v - halfWidth - i;
            }
            else
            {
                ptA.h = srcPoint.h - halfWidth - i;
                ptA.v = srcPoint.v;
            }
            ptB.h = dstPoint.h + ptA.h - srcPoint.h;
            ptB.v = dstPoint.v + ptA.v - srcPoint.v;
            DrawTranslucentLine(dstFrameP, ptA, ptB, coronaColor, shade, 8 - i * 2, i * 2, 8 + i * 6);

            if (longerHoriz)
            {
                ptA.h = srcPoint.h;
                ptA.v = srcPoint.v + halfWidth + i;
            }
            else
            {
                ptA.h = srcPoint.h + halfWidth + i;
                ptA.v = srcPoint.v;
            }
            ptB.h = dstPoint.h + ptA.h - srcPoint.h;
            ptB.v = dstPoint.v + ptA.v - srcPoint.v;
            DrawTranslucentLine(dstFrameP, ptA, ptB, coronaColor, shade, 8 - i * 2, i * 2, 8 + i * 6);

            shade -= fadeStep;
            i++;
        }
        while (shade > 2);
    }
}

// FUNCTION: EVNOVA 0x0047a410
// MAC: _DrawLightningBeam i386 0x000c3f9d ppc 0x000d01fc
void DrawLightningBeam(FramePtr frameA, FramePtr frameB, Point srcPoint, Point dstPoint,
                       uint32_t color, short width, short segPercent, short jitter,
                       short shade)
{
    int     srcV;
    int     dstH;
    int     dstV;
    int     srcH;
    Rect    bounds;
    Rect    clipRect;
    Point   ptB;
    float   stepV;
    float   curV;
    short   srcPtH;
    short   dstPtH;
    int     dH;
    int     dV;
    Boolean    longerHoriz;
    short   segs;
    short   seg;
    short   i;
    short   lastV;
    short   newH;
    short   newV;
    short   h;
    short   shadeStep;
    short   shade2;
    short   lastH;
    short   absV;
    float   curH;
    int     rampEnd;
    Point   ptA;
    float   stepH;
    short   v;

    srcPtH = srcPoint.h;
    dstPtH = dstPoint.h;

    clipRect = frameA->frameRect;

    InsetRect(&clipRect, -32, -32);

    if (srcPtH < clipRect.left && dstPtH < clipRect.left)
        return;
    if (srcPtH > clipRect.right && dstPtH > clipRect.right)
        return;
    if (srcPoint.v < clipRect.top && dstPoint.v < clipRect.top)
        return;
    if (srcPoint.v > clipRect.bottom && dstPoint.v > clipRect.bottom)
        return;

    dstH = dstPtH;
    srcH = srcPtH;
    dH = srcH - dstH;
    segs = abs(dH);

    dstV = dstPoint.v;
    srcV = srcPoint.v;
    dV = srcV - dstV;
    absV = abs(dV);

    if (segs > absV)
        segs = (segPercent * segs * 0.01);
    else
        segs = (segPercent * absV * 0.01);

    if (segs < 1)
        segs = 1;

    stepH = (dstH - srcH) / (float)segs;
    stepV = (dstV - srcV) / (float)segs;
    curH = srcPtH;
    curV = srcPoint.v;

    lastH = curH;
    lastV = curV;

    dV = ABS(dV);
    dH = ABS(dH);

    if (dH > dV)
        longerHoriz = 1;
    else
        longerHoriz = 0;

    if (dstPtH > srcPtH)
    {
        bounds.left = srcPtH;
        bounds.right = dstPtH;
    }
    else
    {
        bounds.left = dstPtH;
        bounds.right = srcPtH;
    }

    if (dstPoint.v > srcPoint.v)
    {
        bounds.top = srcPoint.v;
        bounds.bottom = dstPoint.v;
    }
    else
    {
        bounds.top = dstPoint.v;
        bounds.bottom = srcPoint.v;
    }

    if (longerHoriz)
        InsetRect(&bounds, 0, -(jitter + width));
    else
        InsetRect(&bounds, -(jitter + width), 0);

    shadeStep = 32 / (width + 1);
    srcPtH = jitter * 2 + 1;
    for (seg = 0; seg < segs - 1; seg++)
    {
        curH += stepH;
        curV += stepV;

        h = curH;
        v = curV;

        if (seg < segs - 2)
        {
            newH = h + Random() % srcPtH - jitter;
            newV = v + Random() % srcPtH - jitter;
        }
        else
        {
            newV = dstPoint.v;
            newH = dstPtH;
        }

        if (newH < bounds.left)
            newH = bounds.left;
        if (newH > bounds.right)
            newH = bounds.right;
        if (newV < bounds.top)
            newV = bounds.top;
        if (newV > bounds.bottom)
            newV = bounds.bottom;

        i = 0;

        if (width > 0)
        {
            do
            {
                ptA.v = lastV;
                ptA.h = lastH;
                ptB.v = newV;
                ptB.h = newH;

                if (seg == segs - 2)
                    rampEnd = 32;
                else
                    rampEnd = 0;

                shade2 = shade - i * shadeStep;
                if (shade2 < 1)
                    shade2 = 1;

                if (longerHoriz)
                {
                    ptA.v += i;
                    ptB.v += i;
                    DrawTranslucentLine(frameA, ptA, ptB, color, shade2, shade2, 0, rampEnd);
                    DrawTranslucentLine(frameB, ptA, ptB, color, shade2, shade2, 0, rampEnd);

                    if (i > 0)
                    {
                        ptA.v = lastV - i;
                        ptB.v = newV - i;
                        DrawTranslucentLine(frameA, ptA, ptB, color, shade2, shade2, 0, rampEnd);
                        DrawTranslucentLine(frameB, ptA, ptB, color, shade2, shade2, 0, rampEnd);
                    }
                }
                else
                {
                    ptA.h += i;
                    ptB.h += i;
                    DrawTranslucentLine(frameA, ptA, ptB, color, shade2, shade2, 0, rampEnd);
                    DrawTranslucentLine(frameB, ptA, ptB, color, shade2, shade2, 0, rampEnd);

                    if (i > 0)
                    {
                        ptA.h = lastH - i;
                        ptB.h = newH - i;
                        DrawTranslucentLine(frameA, ptA, ptB, color, shade2, shade2, 0, rampEnd);
                        DrawTranslucentLine(frameB, ptA, ptB, color, shade2, shade2, 0, rampEnd);
                    }
                }

                i++;
            }
            while (i < width);
        }

        lastV = newV;
        lastH = newH;
    }
}

// FUNCTION: EVNOVA 0x0047ac50
// MAC: _EraseBeam i386 0x000c48b5 ppc 0x000d08c0
void EraseBeam(FramePtr srcFrameP, FramePtr dstFrameP, Point srcPoint, Point dstPoint,
               short halfWidth, short fadeStep)
{
    Point   ptA;
    Point   ptB;
    Boolean longerHoriz;
    short   i;
    short   level;
    Rect    clipRect;

    clipRect = dstFrameP->frameRect;

    InsetRect(&clipRect, -32, -32);

    if (srcPoint.h < clipRect.left && dstPoint.h < clipRect.left)
        return;
    if (srcPoint.h > clipRect.right && dstPoint.h > clipRect.right)
        return;
    if (srcPoint.v < clipRect.top && dstPoint.v < clipRect.top)
        return;
    if (srcPoint.v > clipRect.bottom && dstPoint.v > clipRect.bottom)
        return;

    if (ABS(srcPoint.h - dstPoint.h) > ABS(srcPoint.v - dstPoint.v))
        longerHoriz = 1;
    else
        longerHoriz = 0;

    for (i = 0; i <= halfWidth; i++)
    {
        if (longerHoriz)
        {
            ptA.h = srcPoint.h;
            ptA.v = srcPoint.v - i;
        }
        else
        {
            ptA.h = srcPoint.h - i;
            ptA.v = srcPoint.v;
        }
        ptB.h = dstPoint.h + ptA.h - srcPoint.h;
        ptB.v = dstPoint.v + ptA.v - srcPoint.v;
        CopyLine(srcFrameP, dstFrameP, ptA, ptB);

        if (i > 0)
        {
            if (longerHoriz)
            {
                ptA.h = srcPoint.h;
                ptA.v = srcPoint.v + i;
            }
            else
            {
                ptA.h = srcPoint.h + i;
                ptA.v = srcPoint.v;
            }
            ptB.h = dstPoint.h + ptA.h - srcPoint.h;
            ptB.v = dstPoint.v + ptA.v - srcPoint.v;
            CopyLine(srcFrameP, dstFrameP, ptA, ptB);
        }
    }

    i = 0;
    level = 16;
    if (fadeStep > 0)
    {
        do
        {
            if (longerHoriz)
            {
                ptA.h = srcPoint.h;
                ptA.v = srcPoint.v - halfWidth - i;
            }
            else
            {
                ptA.h = srcPoint.h - halfWidth - i;
                ptA.v = srcPoint.v;
            }
            ptB.h = dstPoint.h + ptA.h - srcPoint.h;
            ptB.v = dstPoint.v + ptA.v - srcPoint.v;
            CopyLine(srcFrameP, dstFrameP, ptA, ptB);

            if (longerHoriz)
            {
                ptA.h = srcPoint.h;
                ptA.v = srcPoint.v + halfWidth + i;
            }
            else
            {
                ptA.h = srcPoint.h + halfWidth + i;
                ptA.v = srcPoint.v;
            }
            ptB.h = dstPoint.h + ptA.h - srcPoint.h;
            ptB.v = dstPoint.v + ptA.v - srcPoint.v;
            CopyLine(srcFrameP, dstFrameP, ptA, ptB);

            level -= fadeStep;
            i++;
        }
        while (level > 2);
    }
}

// FUNCTION: EVNOVA 0x0047afd0
// MAC: _EraseLightningBeam i386 0x000c4be1 ppc 0x000d0ca4
void EraseLightningBeam(FramePtr srcFrameP, FramePtr dstFrameP,
                        Point srcPoint, Point dstPoint,
                        short halfWidth, short segPercent, short jitter)
{
    short   i;
    Rect    clipRect;
    Rect    r;
    Point   tempPt;
    Point   curPt;
    Point   lastPt;
    float   stepH, stepV, curH, curV;
    short   segs;
    short   margin, t;

    clipRect = dstFrameP->frameRect;

    InsetRect(&clipRect, -32, -32);

    if (srcPoint.h < clipRect.left && dstPoint.h < clipRect.left)
        return;
    if (srcPoint.h > clipRect.right && dstPoint.h > clipRect.right)
        return;
    if (srcPoint.v < clipRect.top && dstPoint.v < clipRect.top)
        return;
    if (srcPoint.v > clipRect.bottom && dstPoint.v > clipRect.bottom)
        return;

    tempPt.h = abs(srcPoint.h - dstPoint.h);

    tempPt.v = abs(srcPoint.v - dstPoint.v);
    lastPt = tempPt;
    if (tempPt.h > tempPt.v)
        segs = (segPercent * tempPt.h * 0.01);
    else
        segs = (segPercent * tempPt.v * 0.01);

    if (segs < 1)
        segs = 1;

    stepH = (dstPoint.h - srcPoint.h) / (float)segs;
    stepV = (dstPoint.v - srcPoint.v) / (float)segs;
    curH = srcPoint.h;
    curV = srcPoint.v;

    margin = ((halfWidth + jitter) * 2.5);

    t = stepH;
    t = abs(t) * 2;
    if (margin < t)
        margin = t;

    t = stepV;
    t = abs(t) * 2;
    if (margin < t)
        margin = t;

    for (i = 0; i < segs - 1; i++)
    {
        curH += stepH;
        curV += stepV;

        curPt.h = curH;
        curPt.v = curV;
        lastPt = curPt;
        r.left = r.right = curPt.h;
        r.bottom = r.top = curPt.v;

        InsetRect(&r, -margin, -margin);
        BlitPixieRectDrawProc(srcFrameP, dstFrameP, &r, &r);
    }
}
