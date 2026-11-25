/* drawbeam.cpp -- the beam and lightning shapes of SWBeams.c (win 0x00479fe0, 0x0047a410), as lines handed to a
   BeamLines (the GPU view draws them). Notes: Research/notes/drawbeam.md */
#include "spriteworld.h"

// FUNCTION: EVNOVA 0x00479fe0
// MAC: _DrawBeam i386 0x000c3b3c ppc 0x000cfe70
void DrawBeamLines(BeamLines &out, Point srcPoint, Point dstPoint, uint32_t color, short halfWidth,
                   uint32_t coronaColor, short fadeStep)
{
    Point   ptA;
    Point   ptB;
    Boolean longerHoriz;
    short   i;
    short   shade;
    Rect    clipRect;

    if (srcPoint.h == dstPoint.h && srcPoint.v == dstPoint.v)
        return;

    clipRect = out.bounds;

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
        out.Line(ptA, ptB, color, shade, shade, 0, 20 - i * 10);

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
            out.Line(ptA, ptB, color, shade, shade, 0, 20 - i * 10);
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
            out.Line(ptA, ptB, coronaColor, shade, 8 - i * 2, i * 2, 8 + i * 6);

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
            out.Line(ptA, ptB, coronaColor, shade, 8 - i * 2, i * 2, 8 + i * 6);

            shade -= fadeStep;
            i++;
        }
        while (shade > 2);
    }
}

// FUNCTION: EVNOVA 0x0047a410
// MAC: _DrawLightningBeam i386 0x000c3f9d ppc 0x000d01fc
void DrawLightningLines(BeamLines &out, Point srcPoint, Point dstPoint, uint32_t color, short width,
                        short segPercent, short jitter, short shade)
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

    clipRect = out.bounds;

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
                    out.Line(ptA, ptB, color, shade2, shade2, 0, rampEnd);

                    if (i > 0)
                    {
                        ptA.v = lastV - i;
                        ptB.v = newV - i;
                        out.Line(ptA, ptB, color, shade2, shade2, 0, rampEnd);
                    }
                }
                else
                {
                    ptA.h += i;
                    ptB.h += i;
                    out.Line(ptA, ptB, color, shade2, shade2, 0, rampEnd);

                    if (i > 0)
                    {
                        ptA.h = lastH - i;
                        ptB.h = newH - i;
                        out.Line(ptA, ptB, color, shade2, shade2, 0, rampEnd);
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
