/* copybits.cpp -- CopyBits and CopyMask: dispatch by transfer mode and depth, the 8- and 16-bit-source blitters */
#include "pixel.h"
#include "platform.h"
#include "scaleddraw.h"
#include "uiscale.h"

/* the same blitter-family record the 15-bit pair uses: a bounds Rect at
   +0x00, the pixel base at +0x0c, a 3-byte-entry colour table at +0x10 (this
   is its only reader), and the row stride at +0x20 */

// FUNCTION: EVNOVA 0x004b7230
void CopyBitsCopy8(PixMapPtr src, PixMapPtr dst, Rect *srcRect, Rect *dstRect,
                   int32_t xStep, int32_t yStep, int32_t xFrac, int32_t yFrac)
{
    int32_t width;
    int32_t height;
    int32_t srcRow;
    int32_t dstRow;
    unsigned char *dstP;
    const unsigned char *rgb;
    const unsigned char *rgb2;
    unsigned char *srcP;
    unsigned char *table;
    int32_t n;

    width = dstRect->right - dstRect->left;
    height = dstRect->bottom - dstRect->top;
    srcP = (unsigned char *)src->baseAddr
        + (srcRect->top - src->bounds.top) * src->rowBytes
        + (srcRect->left - src->bounds.left);
    srcRow = src->rowBytes - width;
    table = src->ctab;
    dstP = (unsigned char *)dst->baseAddr
        + (dstRect->top - dst->bounds.top) * dst->rowBytes
        + (dstRect->left - dst->bounds.left) * 2;
    dstRow = dst->rowBytes - width * 2;
    if (width > 0 && height > 0) {
        do {
            n = width;
            if (n > 1) {
                do {
                rgb = table + srcP[0] * 3;
                rgb2 = table + srcP[1] * 3;
                *(uint32_t *)dstP = Pixel::From8(rgb[0], rgb[1], rgb[2])
                    | (uint32_t)Pixel::From8(rgb2[0], rgb2[1], rgb2[2]) << 16;
                srcP += 2;
                dstP += 4;
                } while ((n -= 2) > 1);
            }
            if (n != 0) {
                rgb = table + *srcP * 3;
                *(unsigned short *)dstP = Pixel::From8(rgb[0], rgb[1], rgb[2]);
                srcP++;
                dstP += 2;
            }
            srcP += srcRow;
            dstP += dstRow;
            height--;
        } while (height != 0);
    }
}

// FUNCTION: EVNOVA 0x004b73b0
void CopyBitsOr8(PixMapPtr src, PixMapPtr dst, Rect *srcRect, Rect *dstRect,
                 int32_t xStep, int32_t yStep, int32_t xFrac, int32_t yFrac)
{
    unsigned char *srcP;
    int32_t height;
    int32_t srcRow;
    int32_t dstRow;
    unsigned short *dstP;
    const unsigned char *rgb;
    unsigned char *table;
    int32_t width;
    unsigned int idx;
    int32_t n;

    width = dstRect->right - dstRect->left;
    height = dstRect->bottom - dstRect->top;
    srcP = (unsigned char *)src->baseAddr
        + (srcRect->top - src->bounds.top) * src->rowBytes
        + (srcRect->left - src->bounds.left);
    srcRow = src->rowBytes - width;
    table = src->ctab;
    dstP = (unsigned short *)((unsigned char *)dst->baseAddr
        + (dstRect->top - dst->bounds.top) * dst->rowBytes
        + (dstRect->left - dst->bounds.left) * 2);
    dstRow = dst->rowBytes - width * 2;
    if (width != 0 && height != 0) {
        do {
            n = width;
            do {
                idx = *srcP;
                if (idx != 0) {
                    rgb = table + idx * 3;
                    *dstP = Pixel::From8(rgb[0], rgb[1], rgb[2]);
                }
                srcP++;
                dstP++;
                n--;
            } while (n != 0);
            srcP += srcRow;
            dstP = (unsigned short *)((char *)dstP + dstRow);
            height--;
        } while (height != 0);
    }
}

// FUNCTION: EVNOVA 0x004b74c0
void CopyBitsCopy16(PixMapPtr src, PixMapPtr dst, Rect *srcRect, Rect *dstRect,
                    int32_t xStep, int32_t yStep, int32_t xFrac, int32_t yFrac)
{
    int32_t width;
    int32_t height;
    int32_t srcRow;
    int32_t dstRow;
    unsigned short *srcP;
    unsigned short *dstP;
    int32_t n;
    uint32_t a;
    uint32_t b;

    width = dstRect->right - dstRect->left;
    height = dstRect->bottom - dstRect->top;
    srcP = (unsigned short *)((unsigned char *)src->baseAddr
        + (srcRect->top - src->bounds.top) * src->rowBytes
        + (srcRect->left - src->bounds.left) * 2);
    srcRow = src->rowBytes - width * 2;
    dstP = (unsigned short *)((unsigned char *)dst->baseAddr
        + (dstRect->top - dst->bounds.top) * dst->rowBytes
        + (dstRect->left - dst->bounds.left) * 2);
    dstRow = dst->rowBytes - width * 2;
    if (width != 0 && height != 0) {
        do {
            n = width;
            if (n > 3) {
                do {
                    a = ((uint32_t *)srcP)[0];
                    b = ((uint32_t *)srcP)[1];
                    ((uint32_t *)dstP)[0] = a;
                    ((uint32_t *)dstP)[1] = b;
                    srcP += 4;
                    dstP += 4;
                    n -= 4;
                } while (n > 3);
            }
            if (n > 1) {
                *(uint32_t *)dstP = *(uint32_t *)srcP;
                srcP += 2;
                dstP += 2;
                n -= 2;
            }
            if (n != 0) {
                *dstP = *srcP;
                srcP++;
                dstP++;
            }
            srcP = (unsigned short *)((char *)srcP + srcRow);
            dstP = (unsigned short *)((char *)dstP + dstRow);
            height--;
        } while (height != 0);
    }
}

// FUNCTION: EVNOVA 0x004b7b90
void CopyBitsScale8(PixMapPtr src, PixMapPtr dst, Rect *srcRect, Rect *dstRect,
                    int32_t xStep, int32_t yStep, int32_t xStart, int32_t yStart)
{
    int32_t width;
    int32_t height;
    int32_t srcRowBytes;
    int32_t dstRow;
    unsigned short *dstP;
    const unsigned char *rgb;
    unsigned char *table;
    unsigned char *srcBase;
    unsigned char *srcP;
    int32_t x;
    uint32_t n;

    width = dstRect->right - dstRect->left;
    height = dstRect->bottom - dstRect->top;
    srcRowBytes = src->rowBytes;
    srcBase = (unsigned char *)src->baseAddr
        + (srcRect->top - src->bounds.top) * srcRowBytes
        + (srcRect->left - src->bounds.left);
    table = src->ctab;
    dstP = (unsigned short *)((unsigned char *)dst->baseAddr
        + (dstRect->top - dst->bounds.top) * dst->rowBytes
        + (dstRect->left - dst->bounds.left) * 2);
    dstRow = dst->rowBytes - width * 2;
    if (width != 0 && height != 0) {
        do {
            srcP = srcBase + (yStart >> 16) * srcRowBytes;
            n = width;
            x = xStart;
            do {
                rgb = table + srcP[x >> 16] * 3;
                *dstP = Pixel::From8(rgb[0], rgb[1], rgb[2]);
                dstP++;
                x += xStep;
                n--;
            } while (n != 0);
            yStart += yStep;
            dstP = (unsigned short *)((char *)dstP + dstRow);
            height--;
        } while (height != 0);
    }
}

#define kRedBlue555 0x7c1f

// FUNCTION: EVNOVA 0x004b8300
void CopyBitsAddOver16(PixMapPtr src, PixMapPtr dst, Rect *srcRect, Rect *dstRect,
                       int32_t xStep, int32_t yStep, int32_t xFrac, int32_t yFrac)
{
    int32_t width;
    int32_t height;
    int32_t srcRow;
    int32_t dstRow;
    unsigned short *srcP;
    unsigned short *dstP;
    int32_t n;
    int32_t srcPixel;

    width = dstRect->right - dstRect->left;
    height = dstRect->bottom - dstRect->top;
    srcP = (unsigned short *)((unsigned char *)src->baseAddr
        + (srcRect->top - src->bounds.top) * src->rowBytes
        + (srcRect->left - src->bounds.left) * 2);
    srcRow = src->rowBytes;
    srcRow -= width * 2;
    dstP = (unsigned short *)((unsigned char *)dst->baseAddr
        + (dstRect->top - dst->bounds.top) * dst->rowBytes
        + (dstRect->left - dst->bounds.left) * 2);
    dstRow = dst->rowBytes - width * 2;
    if (width != 0 && height != 0) {
        do {
            n = width;
            do {
                srcPixel = *srcP;
                *dstP = (((srcPixel & kRedBlue555) + (*dstP & kRedBlue555)) & kRedBlue555)
                      + (((srcPixel & kGreen555) + (*dstP & kGreen555)) & kGreen555);
                srcP++;
                dstP++;
                n--;
            } while (n != 0);
            srcP = (unsigned short *)((char *)srcP + srcRow);
            dstP = (unsigned short *)((char *)dstP + dstRow);
            height--;
        } while (height != 0);
    }
}

// FUNCTION: EVNOVA 0x004b8400
void CopyBitsAdMax16(PixMapPtr src, PixMapPtr dst, Rect *srcRect, Rect *dstRect,
                     int32_t xStep, int32_t yStep, int32_t xFrac, int32_t yFrac)
{
    int32_t width;
    int32_t height;
    int32_t srcRow;
    int32_t dstRow;
    unsigned short *srcP;
    unsigned short *dstP;
    int32_t n;
    uint32_t d;
    uint32_t dr;
    uint32_t dg;
    uint32_t db;
    uint32_t g;
    uint32_t r;
    uint32_t b;
    uint32_t s;

    width = dstRect->right - dstRect->left;
    height = dstRect->bottom - dstRect->top;
    srcP = (unsigned short *)((unsigned char *)src->baseAddr
        + (srcRect->top - src->bounds.top) * src->rowBytes
        + (srcRect->left - src->bounds.left) * 2);
    srcRow = src->rowBytes - width * 2;
    dstP = (unsigned short *)((unsigned char *)dst->baseAddr
        + (dstRect->top - dst->bounds.top) * dst->rowBytes
        + (dstRect->left - dst->bounds.left) * 2);
    dstRow = dst->rowBytes - width * 2;
    if (width != 0 && height != 0) {
        do {
            n = width;
            do {
                d = *dstP;
                s = *srcP;
                g = s & kGreen555;
                dg = d & kGreen555;
                r = s & kRed555;
                dr = d & kRed555;
                b = s & kBlue555;
                db = d & kBlue555;
                if (g <= dg)
                    g = dg;
                if (r <= dr)
                    r = dr;
                if (b <= db)
                    b = db;
                *dstP = r + g + b;
                srcP++;
                dstP++;
                n--;
            } while (n != 0);
            srcP = (unsigned short *)((char *)srcP + srcRow);
            dstP = (unsigned short *)((char *)dstP + dstRow);
            height--;
        } while (height != 0);
    }
}

// FUNCTION: EVNOVA 0x004b8520
void CopyBitsNone(PixMapPtr src, PixMapPtr dst, Rect *srcRect, Rect *dstRect,
                  int32_t xStep, int32_t yStep, int32_t xFrac, int32_t yFrac)
{
}

typedef void (*BlitProc)(PixMapPtr srcBits, PixMapPtr dstBits,
                          Rect *srcRect, Rect *dstRect,
                          int32_t xStep, int32_t yStep, int32_t xFrac, int32_t yFrac);

BlitProc copyBitsProcs[5][5] = {             /* 0x00575b28 same-size srcCopy */
    { NULL, NULL, NULL, NULL, NULL },
    { NULL, CopyBitsNone, CopyBitsCopy8, CopyBitsNone, CopyBitsNone },
    { NULL, CopyBitsNone, CopyBitsCopy16, CopyBitsNone, BlitDirect },
    { NULL, CopyBitsNone, BlitDirect, CopyBitsNone, BlitDirect },
    { NULL, CopyBitsNone, BlitDirect, CopyBitsNone, BlitDirect }
};

BlitProc scaleBitsCopyProcs[5][5] = {        /* 0x00575b8c scaled srcCopy */
    { NULL, NULL, NULL, NULL, NULL },
    { NULL, CopyBitsNone, CopyBitsScale8, CopyBitsNone, CopyBitsNone },
    { NULL, CopyBitsNone, BlitDirect, CopyBitsNone, BlitDirect },
    { NULL, CopyBitsNone, BlitDirect, CopyBitsNone, BlitDirect },
    { NULL, CopyBitsNone, BlitDirect, CopyBitsNone, BlitDirect }
};

BlitProc copyBitsAddOverProcs[5][5] = {      /* 0x00575bf0 same-size addOver */
    { NULL, NULL, NULL, NULL, NULL },
    { NULL, CopyBitsNone, CopyBitsNone, CopyBitsNone, CopyBitsNone },
    { NULL, CopyBitsNone, CopyBitsAddOver16, CopyBitsNone, CopyBitsNone },
    { NULL, CopyBitsNone, CopyBitsNone, CopyBitsNone, CopyBitsNone },
    { NULL, CopyBitsNone, CopyBitsNone, CopyBitsNone, CopyBitsNone }
};

BlitProc copyBitsAdMaxProcs[5][5] = {        /* 0x00575c54 same-size adMax */
    { NULL, NULL, NULL, NULL, NULL },
    { NULL, CopyBitsNone, CopyBitsNone, CopyBitsNone, CopyBitsNone },
    { NULL, CopyBitsNone, CopyBitsAdMax16, CopyBitsNone, CopyBitsNone },
    { NULL, CopyBitsNone, CopyBitsNone, CopyBitsNone, CopyBitsNone },
    { NULL, CopyBitsNone, CopyBitsNone, CopyBitsNone, CopyBitsNone }
};

/* a pixel map without pixels (a port drawn through its GPU target): no copy, said once in the log */
static bool NoPixels(const PixMap *pix)
{
    static int said;

    if (pix->baseAddr != NULL)
        return false;
    PlatDebugLogOnce(&said, "CopyBits: a pixel map without pixels (a port with a GPU target) is not copied");
    return true;
}

#define transfer_srcCopy 0
#define transfer_srcOr   1
#define transfer_addOver 34
#define transfer_adMax   37

// FUNCTION: EVNOVA 0x004b8530
void CopyBits(BitMapPtr srcBits, BitMapPtr dstBits, const Rect *srcRect,
              const Rect *dstRect, int32_t mode, void *maskRgn)
{
    Rect dst;
    Rect src;
    int32_t srcWidth;
    int32_t xStep;
    int32_t yStep;
    int32_t xFrac;
    int32_t yFrac;
    PixMapPtr srcPix;
    PixMapPtr dstPix;
    BlitProc proc;
    int32_t scaled;

    xStep = 0;
    yStep = 0;
    proc = NULL;
    if (srcBits == NULL || dstBits == NULL || srcRect == NULL || dstRect == NULL)
        return;
    if (NoPixels((PixMapPtr)srcBits) || NoPixels((PixMapPtr)dstBits))
        return;
    if (UI::Mapping(*(PixMapPtr)srcBits).Scaled() || UI::Mapping(*(PixMapPtr)dstBits).Scaled()) {
        Scaled::CopyBits(*(PixMapPtr)srcBits, *(PixMapPtr)dstBits, *srcRect, *dstRect, mode);
        return;
    }

    src.top = srcRect->top;
    src.left = srcRect->left;
    src.bottom = srcRect->bottom;
    src.right = srcRect->right;
    dst.top = dstRect->top;
    dst.left = dstRect->left;
    dst.bottom = dstRect->bottom;
    dst.right = dstRect->right;

    srcPix = (PixMapPtr)srcBits;
    dstPix = (PixMapPtr)dstBits;

    scaled = 1;
    srcWidth = srcRect->right - srcRect->left;
    if (dstRect->right - dstRect->left - srcWidth == 0
            && dstRect->bottom - dstRect->top == srcRect->bottom - srcRect->top)
        scaled = 0;
    if ((char)scaled) {
        switch (mode) {
        case transfer_srcCopy:
            proc = scaleBitsCopyProcs[srcPix->pixelSize / 8][dstPix->pixelSize / 8];
            break;
        }
        xStep = srcWidth;
        xStep <<= 16;
        xStep /= dstRect->right - dstRect->left;
        yStep = srcRect->bottom - srcRect->top;
        yStep <<= 16;
        yStep /= dstRect->bottom - dstRect->top;
    } else {
        switch (mode) {
        case transfer_srcCopy:
            proc = copyBitsProcs[srcPix->pixelSize / 8][dstPix->pixelSize / 8];
            break;
        case transfer_srcOr:
            proc = CopyBitsOr8;
            break;
        case transfer_addOver:
            proc = copyBitsAddOverProcs[srcPix->pixelSize / 8][dstPix->pixelSize / 8];
            break;
        case transfer_adMax:
            proc = copyBitsAdMaxProcs[srcPix->pixelSize / 8][dstPix->pixelSize / 8];
            break;
        }
    }

    if (src.top < srcPix->bounds.top) {
        dst.top += (((dstRect->bottom - dstRect->top) * (srcPix->bounds.top - src.top)) / (srcRect->bottom - srcRect->top));
        src.top = srcPix->bounds.top;
    }
    if (src.left < srcPix->bounds.left) {
        dst.left += (((dstRect->right - dstRect->left) * (srcPix->bounds.left - src.left)) / (srcRect->right - srcRect->left));
        src.left = srcPix->bounds.left;
    }
    if (src.bottom > srcPix->bounds.bottom) {
        dst.bottom -= (((dstRect->bottom - dstRect->top) * (src.bottom - srcPix->bounds.bottom)) / (srcRect->bottom - srcRect->top));
        src.bottom = srcPix->bounds.bottom;
    }
    if (src.right > srcPix->bounds.right) {
        dst.right -= (((dstRect->right - dstRect->left) * (src.right - srcPix->bounds.right)) / (srcRect->right - srcRect->left));
        src.right = srcPix->bounds.right;
    }

    if (dst.top < dstPix->bounds.top) {
        src.top += (((srcRect->bottom - srcRect->top) * (dstPix->bounds.top - dst.top)) / (dstRect->bottom - dstRect->top));
        dst.top = dstPix->bounds.top;
    }
    if (dst.left < dstPix->bounds.left) {
        src.left += (((srcRect->right - srcRect->left) * (dstPix->bounds.left - dst.left)) / (dstRect->right - dstRect->left));
        dst.left = dstPix->bounds.left;
    }
    if (dst.bottom > dstPix->bounds.bottom) {
        src.bottom -= (((srcRect->bottom - srcRect->top) * (dst.bottom - dstPix->bounds.bottom)) / (dstRect->bottom - dstRect->top));
        dst.bottom = dstPix->bounds.bottom;
    }
    if (dst.right > dstPix->bounds.right) {
        src.right -= (((srcRect->right - srcRect->left) * (dst.right - dstPix->bounds.right)) / (dstRect->right - dstRect->left));
        dst.right = dstPix->bounds.right;
    }

    xFrac = xStep >> 1;
    yFrac = yStep >> 1;

    if (dst.left >= dst.right || dst.top >= dst.bottom)
        return;
    if (src.left < srcPix->bounds.left || src.top < srcPix->bounds.top
            || src.right > srcPix->bounds.right
            || src.bottom > srcPix->bounds.bottom)
        return;
    if (proc == NULL)
        return;                         /* a stretched copy in a mode that cannot stretch */

    proc(srcPix, dstPix, &src, &dst, xStep, yStep, xFrac, yFrac);
}

#if EVPLUS_TEST_PAGES
// FUNCTION: EVNOVA 0x004b92d0
void CopyMask16(PixMapPtr srcBits, PixMapPtr maskBits, PixMapPtr dstBits,
                Rect *srcRect, Rect *maskRect, Rect *dstRect,
                int32_t xStep, int32_t yStep, int32_t xFrac, int32_t yFrac)
{
    int32_t srcRow;
    int32_t dstLeft;
    int32_t dstTop;
    int32_t width;
    int32_t height;
    int32_t maskRow;
    int32_t dstRow;
    unsigned short *srcP;
    unsigned short *maskP;
    unsigned short *dstP;
    int32_t n;

    dstLeft = dstRect->left;
    width = dstRect->right - dstLeft;
    dstTop = dstRect->top;
    height = dstRect->bottom - dstTop;

    srcP = (unsigned short *)((unsigned char *)srcBits->baseAddr
        + (srcRect->top - srcBits->bounds.top) * srcBits->rowBytes
        + (srcRect->left - srcBits->bounds.left) * 2);
    srcRow = srcBits->rowBytes - width * 2;

    maskP = (unsigned short *)((unsigned char *)maskBits->baseAddr
        + (maskRect->top - maskBits->bounds.top) * maskBits->rowBytes
        + (maskRect->left - maskBits->bounds.left) * 2);
    maskRow = maskBits->rowBytes - width * 2;

    dstP = (unsigned short *)((unsigned char *)dstBits->baseAddr
        + (dstTop - dstBits->bounds.top) * dstBits->rowBytes
        + (dstLeft - dstBits->bounds.left) * 2);
    dstRow = dstBits->rowBytes - width * 2;

    if (width != 0 && height != 0) {
        do {
            n = width;
            do {
                if (*maskP == 0)
                    *dstP = *srcP;
                srcP++;
                maskP++;
                dstP++;
            } while (--n != 0);
            srcP = (unsigned short *)((char *)srcP + srcRow);
            maskP = (unsigned short *)((char *)maskP + maskRow);
            dstP = (unsigned short *)((char *)dstP + dstRow);
            height--;
        } while (height != 0);
    }
}

// FUNCTION: EVNOVA 0x004b9400
void CopyMaskNone(PixMapPtr src, PixMapPtr mask, PixMapPtr dst,
                  Rect *srcRect, Rect *maskRect, Rect *dstRect,
                  int32_t xStep, int32_t yStep, int32_t xFrac, int32_t yFrac)
{
}

typedef void (*BlitMaskProc)(PixMapPtr srcBits, PixMapPtr maskBits,
                             PixMapPtr dstBits, Rect *srcRect, Rect *maskRect,
                             Rect *dstRect, int32_t xStep, int32_t yStep,
                             int32_t xFrac, int32_t yFrac);

BlitMaskProc copyMaskProcs[5][5] = {    /* 0x00575cb8 */
    { NULL, NULL, NULL, NULL, NULL },
    { NULL, CopyMaskNone, CopyMaskNone, CopyMaskNone, CopyMaskNone },
    { NULL, CopyMaskNone, CopyMask16, CopyMaskNone, BlitDirectMask16 },
    { NULL, CopyMaskNone, CopyMaskNone, CopyMaskNone, CopyMaskNone },
    { NULL, CopyMaskNone, CopyMaskNone, CopyMaskNone, CopyMaskNone }
};

BlitMaskProc scaleMaskProcs[5][5] = {   /* 0x00575d1c */
    { NULL, NULL, NULL, NULL, NULL },
    { NULL, CopyMaskNone, CopyMaskNone, CopyMaskNone, CopyMaskNone },
    { NULL, CopyMaskNone, BlitDirectMask16, CopyMaskNone, BlitDirectMask16 },
    { NULL, CopyMaskNone, CopyMaskNone, CopyMaskNone, CopyMaskNone },
    { NULL, CopyMaskNone, CopyMaskNone, CopyMaskNone, CopyMaskNone }
};

// FUNCTION: EVNOVA 0x004b9410
void CopyMask(BitMapPtr srcBits, BitMapPtr maskBits, BitMapPtr dstBits,
              Rect *srcRect, Rect *maskRect, Rect *dstRect)
{
    Rect mask;
    Rect src;
    Rect dst;
    int32_t srcWidth;
    int32_t dstWidth;
    int32_t xStep;
    int32_t yStep;
    int32_t yFrac;
    int32_t xFrac;
    int32_t delta;
    BlitMaskProc proc;

    xStep = 0;
    yStep = 0;
    xFrac = 0;
    yFrac = 0;
    if (srcBits == NULL || maskBits == NULL || dstBits == NULL
            || srcRect == NULL || maskRect == NULL || dstRect == NULL)
        return;
    if (NoPixels((PixMapPtr)srcBits) || NoPixels((PixMapPtr)maskBits) || NoPixels((PixMapPtr)dstBits))
        return;
    if (UI::Mapping(*(PixMapPtr)srcBits).Scaled() || UI::Mapping(*(PixMapPtr)maskBits).Scaled()
            || UI::Mapping(*(PixMapPtr)dstBits).Scaled()) {
        Scaled::CopyMask(*(PixMapPtr)srcBits, *(PixMapPtr)maskBits, *(PixMapPtr)dstBits, *srcRect, *maskRect,
                         *dstRect);
        return;
    }

    src.top = srcRect->top;
    src.left = srcRect->left;
    src.bottom = srcRect->bottom;
    src.right = srcRect->right;
    mask.top = maskRect->top;
    mask.left = maskRect->left;
    mask.bottom = maskRect->bottom;
    mask.right = maskRect->right;
    dst.top = dstRect->top;
    dst.left = dstRect->left;
    dst.bottom = dstRect->bottom;
    dst.right = dstRect->right;

    srcWidth = srcRect->right - srcRect->left;
    dstWidth = dstRect->right - dstRect->left;
    if (dstWidth == srcWidth
            && dstRect->bottom - dstRect->top
                   == srcRect->bottom - srcRect->top) {
        proc = copyMaskProcs[((PixMapPtr)srcBits)->pixelSize / 8][((PixMapPtr)dstBits)->pixelSize / 8];
    } else {
        proc = scaleMaskProcs[((PixMapPtr)srcBits)->pixelSize / 8][((PixMapPtr)dstBits)->pixelSize / 8];
        xStep = srcWidth;
        xStep <<= 16;
        xStep /= dstWidth;
        yStep = srcRect->bottom - srcRect->top;
        yStep <<= 16;
        yStep /= dstRect->bottom - dstRect->top;
    }

    if (dst.left < ((PixMapPtr)dstBits)->bounds.left) {
        delta = ((PixMapPtr)dstBits)->bounds.left - dst.left;
        src.left += delta;
        mask.left += delta;
        dst.left += delta;
        xFrac = xStep * delta;
    }
    if (dst.top < ((PixMapPtr)dstBits)->bounds.top) {
        delta = ((PixMapPtr)dstBits)->bounds.top - dst.top;
        src.top += delta;
        mask.top += delta;
        dst.top += delta;
        /* BUG: the top clip scales by xStep, not yStep */
        yFrac = xStep * delta;
    }
    if (dst.right > ((PixMapPtr)dstBits)->bounds.right)
        dst.right -= dst.right - ((PixMapPtr)dstBits)->bounds.right;
    if (dst.bottom > ((PixMapPtr)dstBits)->bounds.bottom)
        dst.bottom -= dst.bottom - ((PixMapPtr)dstBits)->bounds.bottom;

    if (dst.left >= dst.right || dst.top >= dst.bottom)
        return;
    if (src.left < ((PixMapPtr)srcBits)->bounds.left || src.top < ((PixMapPtr)srcBits)->bounds.top
            || src.right > ((PixMapPtr)srcBits)->bounds.right
            || src.bottom > ((PixMapPtr)srcBits)->bounds.bottom)
        return;
    if (mask.left < ((PixMapPtr)maskBits)->bounds.left || mask.top < ((PixMapPtr)maskBits)->bounds.top
            || mask.right > ((PixMapPtr)maskBits)->bounds.right
            || mask.bottom > ((PixMapPtr)maskBits)->bounds.bottom)
        return;

    proc((PixMapPtr)srcBits, (PixMapPtr)maskBits, (PixMapPtr)dstBits, &src, &mask, &dst,
         xStep, yStep, xFrac, yFrac);
}
#endif
