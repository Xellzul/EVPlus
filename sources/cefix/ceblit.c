/* ceblit.c -- EV Nova CE: the blitters CE put into the CopyBits and CopyMask tables, and the true row stride
   of a new GWorld. CE drew through GDI+ (DrawImage, HighQualityBicubic); the same is done here in plain C:
   - A picture drawn at its own size is converted pixel by pixel. Going down to 16-bit colour the low bits
     are dropped, as the game's own 24-bit copy does and Wine's GDI+; Microsoft's GDI+ dithers there.
   - A scaled picture is resampled with the kernel that Microsoft's HighQualityBicubic was measured to use
     (see Kernel below). Unlike GDI+, a picture's edge is made from its edge pixels only: GDI+ mixes in
     what lay outside the picture as transparent, which leaves a rim of the old destination.
   Rectangles are taken relative to the first pixel of each pixel map, bounds not applied, as CE gave them
   to GDI+. Notes: Research/cefix-plan.md */

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "macport.h"
#include "cefix.h"

#define kAlpha1555          0x8000      /* bit 15 of a 1555 pixel */
#define kAlpha1555High      0x80        /* the same bit, in the pixel's high byte */
#define kOpaque             255

#define kWeightBits         14          /* a pixel's weights add up to 1 << kWeightBits */
#define kWeightOne          (1 << kWeightBits)
#define kRowBits            6           /* fraction bits kept between the two passes */
#define kKernelReach        2.0         /* the cubic's half width, in source pixels at full size */
#define kPixelHalf          0.5

typedef struct WinBitmap {              /* Win32 BITMAP */
    int32_t bmType;
    int32_t bmWidth;
    int32_t bmHeight;
    int32_t bmWidthBytes;
    unsigned short bmPlanes;
    unsigned short bmBitsPixel;
    void *bmBits;
} WinBitmap;

__declspec(dllimport) int __stdcall GetObjectA(void *object, int size, void *out);

/* -- pixels: 16-bit x1r5g5b5 (bit 15 = opaque where a mask was applied), 24-bit R G B, 32-bit B G R x ------ */

typedef struct Color {
    int r;
    int g;
    int b;
    int a;
} Color;

static inline int Expand5(int v)
{
    return (v << 3) | (v >> 2);         /* what GDI+ makes of a 5-bit channel */
}

static inline void ReadPixel(const unsigned char *p, int pixelSize, int alpha, Color *c)
{
    unsigned int v;

    c->a = kOpaque;
    switch (pixelSize) {
    case 16:
        v = *(const unsigned short *)p;
        c->r = Expand5((v >> 10) & 31);
        c->g = Expand5((v >> 5) & 31);
        c->b = Expand5(v & 31);
        if (alpha && (v & kAlpha1555) == 0)
            c->r = c->g = c->b = c->a = 0;
        break;
    case 24:
        c->r = p[0];
        c->g = p[1];
        c->b = p[2];
        break;
    default:
        c->b = p[0];
        c->g = p[1];
        c->r = p[2];
        break;
    }
}

static inline void WritePixel(unsigned char *p, int pixelSize, int r, int g, int b)
{
    if (pixelSize == 16) {
        *(unsigned short *)p = ((r >> 3) << 10) | ((g >> 3) << 5) | (b >> 3);
    } else {
        p[0] = b;
        p[1] = g;
        p[2] = r;
        p[3] = kOpaque;
    }
}

static inline unsigned char *PixelAt(PixMapPtr pix, int x, int y)
{
    return (unsigned char *)pix->baseAddr + y * pix->rowBytes + x * (pix->pixelSize / 8);
}

/* -- the same size ----------------------------------------------------------------------------------------- */

static void CopySameSize(PixMapPtr src, PixMapPtr dst, const Rect *srcRect, const Rect *dstRect, int alpha)
{
    const unsigned char *in;
    unsigned char *out;
    Color c;
    int srcStep;
    int dstStep;
    int width;
    int height;
    int sx;
    int sy;
    int dx;
    int dy;
    int skip;
    int x;
    int y;

    sx = srcRect->left;
    sy = srcRect->top;
    dx = dstRect->left;
    dy = dstRect->top;
    width = dstRect->right - dstRect->left;
    height = dstRect->bottom - dstRect->top;
    /* what lies outside either pixel map is not drawn, as with GDI+ */
    skip = 0;
    if (sx < 0)
        skip = -sx;
    if (dx < 0 && -dx > skip)
        skip = -dx;
    sx += skip;
    dx += skip;
    width -= skip;
    skip = 0;
    if (sy < 0)
        skip = -sy;
    if (dy < 0 && -dy > skip)
        skip = -dy;
    sy += skip;
    dy += skip;
    height -= skip;
    if (width > src->width - sx)
        width = src->width - sx;
    if (width > dst->width - dx)
        width = dst->width - dx;
    if (height > src->height - sy)
        height = src->height - sy;
    if (height > dst->height - dy)
        height = dst->height - dy;
    if (width <= 0 || height <= 0)
        return;

    srcStep = src->pixelSize / 8;
    dstStep = dst->pixelSize / 8;
    for (y = 0; y < height; y++) {
        in = PixelAt(src, sx, sy + y);
        out = PixelAt(dst, dx, dy + y);
        if (src->pixelSize == 24 && dst->pixelSize == 16) {
            for (x = 0; x < width; x++) {
                *(unsigned short *)out = ((in[0] >> 3) << 10) | ((in[1] >> 3) << 5) | (in[2] >> 3);
                in += 3;
                out += 2;
            }
        } else if (src->pixelSize == 32 && dst->pixelSize == 16) {
            for (x = 0; x < width; x++) {
                *(unsigned short *)out = ((in[2] >> 3) << 10) | ((in[1] >> 3) << 5) | (in[0] >> 3);
                in += 4;
                out += 2;
            }
        } else {
            for (x = 0; x < width; x++) {
                ReadPixel(in, src->pixelSize, alpha, &c);
                if (c.a != 0)
                    WritePixel(out, dst->pixelSize, c.r, c.g, c.b);
                in += srcStep;
                out += dstStep;
            }
        }
    }
}

/* -- scaled ------------------------------------------------------------------------------------------------ */

/* Microsoft's HighQualityBicubic, as measured (a one-pixel line and an edge at 13 sizes from 4x down to
   1/4, 8-bit output; this reproduces it to within half a step of 255 down to half size): the cubic
   convolution kernel with a = -1,
       k(t) = |t|^3 - 2|t|^2 + 1               for |t| < 1
       k(t) = -(|t|^3 - 5|t|^2 + 8|t| - 4)     for 1 <= |t| < 2
   made 1 / scale times as wide when the picture shrinks, and taken over the whole width of each source
   pixel rather than at its centre. KernelArea is the kernel's integral from 0 to t, so a source pixel's
   weight is the difference of two of them. */
static double KernelArea(double t)
{
    double sign;
    double t2;

    sign = 1.0;
    if (t < 0) {
        sign = -1.0;
        t = -t;
    }
    if (t >= kKernelReach)
        return sign * 0.5;
    t2 = t * t;
    if (t < 1.0)
        return sign * (t2 * t2 / 4.0 - 2.0 * t2 * t / 3.0 + t);
    return sign * (-(t2 * t2 / 4.0 - 5.0 * t2 * t / 3.0 + 4.0 * t2 - 4.0 * t) - 10.0 / 12.0);
}

/* the source pixels one destination pixel is made of: `count' of them from `first' on (which may lie
   outside the picture: the edge pixel stands in), with weights that add up to kWeightOne */
typedef struct Taps {
    int first;
    int count;
} Taps;

typedef struct Axis {
    Taps *taps;                         /* one per destination pixel */
    int *weights;                       /* maxCount for each */
    int maxCount;
    int lowest;                         /* the source pixels all of them need, inside the picture */
    int highest;
} Axis;

static int ClampInt(int v, int low, int high)
{
    return v < low ? low : v > high ? high : v;
}

/* `count' destination pixels made of the source pixels from `start' on, `length' of them, in a picture
   `limit' wide */
static int MakeAxis(Axis *axis, int count, int start, int length, int limit)
{
    double scale;
    double stretch;
    double centre;
    double weight;
    int *w;
    int first;
    int last;
    int sum;
    int largest;
    int i;
    int k;

    scale = (double)count / length;
    stretch = scale < 1.0 ? 1.0 / scale : 1.0;
    axis->maxCount = (int)ceil(2.0 * (kKernelReach * stretch + kPixelHalf)) + 2;
    axis->taps = malloc(count * sizeof(Taps));
    axis->weights = malloc((size_t)count * axis->maxCount * sizeof(int));
    if (axis->taps == NULL || axis->weights == NULL)
        return 0;
    axis->lowest = limit - 1;
    axis->highest = 0;
    for (i = 0; i < count; i++) {
        /* where the destination pixel's centre lies in the source, source pixel k covering k - 1/2 .. k + 1/2 */
        centre = start + (i + kPixelHalf) / scale - kPixelHalf;
        first = (int)floor(centre - kKernelReach * stretch - kPixelHalf);
        last = (int)ceil(centre + kKernelReach * stretch + kPixelHalf);
        if (last - first + 1 > axis->maxCount)
            last = first + axis->maxCount - 1;
        w = axis->weights + (size_t)i * axis->maxCount;
        sum = 0;
        largest = 0;
        for (k = first; k <= last; k++) {
            weight = KernelArea((k + kPixelHalf - centre) / stretch) - KernelArea((k - kPixelHalf - centre) / stretch);
            w[k - first] = (int)floor(weight * kWeightOne + 0.5);
            sum += w[k - first];
            if (w[k - first] > w[largest])
                largest = k - first;
        }
        w[largest] += kWeightOne - sum;         /* rounding must not change the brightness */
        axis->taps[i].first = first;
        axis->taps[i].count = last - first + 1;
        if (ClampInt(first, 0, limit - 1) < axis->lowest)
            axis->lowest = ClampInt(first, 0, limit - 1);
        if (ClampInt(last, 0, limit - 1) > axis->highest)
            axis->highest = ClampInt(last, 0, limit - 1);
    }
    return 1;
}

static void FreeAxis(Axis *axis)
{
    free(axis->taps);
    free(axis->weights);
}

/* srcRect of SRC resampled into dstRect of DST. With `alpha' a source pixel without bit 15 is transparent:
   the picture is resampled with its coverage and laid over what DST holds. */
static void CopyScaled(PixMapPtr src, PixMapPtr dst, const Rect *srcRect, const Rect *dstRect, int alpha)
{
    Axis across;
    Axis down;
    Color c;
    Color old;
    unsigned char *line;                /* one source row as R G B A, coverage already applied */
    short *rows;                        /* the source rows needed, each resampled across */
    short *row;
    const unsigned char *in;
    const int *w;
    unsigned char *out;
    int dstWidth;
    int dstHeight;
    int channels;
    int span;
    int sum[4];
    int x;
    int y;
    int k;
    int ch;
    int column;
    int outX;
    int outY;

    dstWidth = dstRect->right - dstRect->left;
    dstHeight = dstRect->bottom - dstRect->top;
    if (dstWidth <= 0 || dstHeight <= 0 || srcRect->right <= srcRect->left || srcRect->bottom <= srcRect->top
            || src->width <= 0 || src->height <= 0)
        return;
    memset(&across, 0, sizeof(across));
    memset(&down, 0, sizeof(down));
    line = NULL;
    rows = NULL;
    channels = alpha ? 4 : 3;
    if (!MakeAxis(&across, dstWidth, srcRect->left, srcRect->right - srcRect->left, src->width)
            || !MakeAxis(&down, dstHeight, srcRect->top, srcRect->bottom - srcRect->top, src->height))
        goto done;
    span = across.highest - across.lowest + 1;
    line = malloc((size_t)span * 4);
    rows = malloc((size_t)(down.highest - down.lowest + 1) * dstWidth * channels * sizeof(short));
    if (line == NULL || rows == NULL)
        goto done;

    /* across: every source row that is needed */
    for (y = down.lowest; y <= down.highest; y++) {
        in = PixelAt(src, across.lowest, y);
        for (x = 0; x < span; x++) {
            ReadPixel(in, src->pixelSize, alpha, &c);
            line[x * 4 + 0] = c.r;
            line[x * 4 + 1] = c.g;
            line[x * 4 + 2] = c.b;
            line[x * 4 + 3] = c.a;
            in += src->pixelSize / 8;
        }
        row = rows + (size_t)(y - down.lowest) * dstWidth * channels;
        for (x = 0; x < dstWidth; x++) {
            w = across.weights + (size_t)x * across.maxCount;
            for (ch = 0; ch < channels; ch++)
                sum[ch] = 0;
            for (k = 0; k < across.taps[x].count; k++) {
                column = ClampInt(across.taps[x].first + k, 0, src->width - 1) - across.lowest;
                for (ch = 0; ch < channels; ch++)
                    sum[ch] += w[k] * line[column * 4 + ch];
            }
            for (ch = 0; ch < channels; ch++)
                row[x * channels + ch] = (sum[ch] + (1 << (kWeightBits - kRowBits - 1))) >> (kWeightBits - kRowBits);
        }
    }

    /* down, into the destination */
    for (y = 0; y < dstHeight; y++) {
        outY = dstRect->top + y;
        if (outY < 0 || outY >= dst->height)
            continue;
        w = down.weights + (size_t)y * down.maxCount;
        for (x = 0; x < dstWidth; x++) {
            outX = dstRect->left + x;
            if (outX < 0 || outX >= dst->width)
                continue;
            for (ch = 0; ch < channels; ch++)
                sum[ch] = 0;
            for (k = 0; k < down.taps[y].count; k++) {
                row = rows + (size_t)(ClampInt(down.taps[y].first + k, 0, src->height - 1) - down.lowest)
                             * dstWidth * channels + x * channels;
                for (ch = 0; ch < channels; ch++)
                    sum[ch] += w[k] * row[ch];
            }
            for (ch = 0; ch < channels; ch++)
                sum[ch] = ClampInt((sum[ch] + (1 << (kWeightBits + kRowBits - 1))) >> (kWeightBits + kRowBits),
                                   0, kOpaque);
            out = PixelAt(dst, outX, outY);
            if (alpha) {
                if (sum[3] == 0)
                    continue;
                if (sum[3] < kOpaque) {
                    ReadPixel(out, dst->pixelSize, 0, &old);
                    sum[0] = ClampInt(sum[0] + old.r * (kOpaque - sum[3]) / kOpaque, 0, kOpaque);
                    sum[1] = ClampInt(sum[1] + old.g * (kOpaque - sum[3]) / kOpaque, 0, kOpaque);
                    sum[2] = ClampInt(sum[2] + old.b * (kOpaque - sum[3]) / kOpaque, 0, kOpaque);
                }
            }
            WritePixel(out, dst->pixelSize, sum[0], sum[1], sum[2]);
        }
    }

done:
    free(line);
    free(rows);
    FreeAxis(&across);
    FreeAxis(&down);
}

static void Blit(PixMapPtr src, PixMapPtr dst, const Rect *srcRect, const Rect *dstRect, int alpha)
{
    if (dstRect->right - dstRect->left == srcRect->right - srcRect->left
            && dstRect->bottom - dstRect->top == srcRect->bottom - srcRect->top)
        CopySameSize(src, dst, srcRect, dstRect, alpha);
    else
        CopyScaled(src, dst, srcRect, dstRect, alpha);
}

void CE_Blit16(PixMapPtr src, PixMapPtr dst, Rect *srcRect, Rect *dstRect)
{
    Blit(src, dst, srcRect, dstRect, 0);
}

void CE_Blit24(PixMapPtr src, PixMapPtr dst, Rect *srcRect, Rect *dstRect)
{
    Blit(src, dst, srcRect, dstRect, 0);
}

void CE_Blit32(PixMapPtr src, PixMapPtr dst, Rect *srcRect, Rect *dstRect)
{
    Blit(src, dst, srcRect, dstRect, 0);
}

/* A black mask pixel makes its source pixel opaque, written into the source itself as CE
   does; maskRect is not read. */
void CE_Blit16Mask(PixMapPtr src, PixMapPtr mask, PixMapPtr dst, Rect *srcRect,
                   Rect *maskRect, Rect *dstRect)
{
    unsigned char *pixels;
    unsigned char *maskPixels;
    int32_t x;
    int32_t y;

    pixels = src->baseAddr;
    maskPixels = mask->baseAddr;
    for (y = 0; y < src->height; y++) {
        for (x = 0; x < src->width; x++) {
            if (maskPixels[y * mask->rowBytes + x * 2] == 0)
                pixels[y * src->rowBytes + x * 2 + 1] |= kAlpha1555High;
        }
    }
    Blit(src, dst, srcRect, dstRect, 1);
}

/* NewGWorld records depth / 8 * width as the row stride, but the DIB section pads each row
   to 4 bytes: take the stride the bitmap really has, so odd widths draw straight. */
short CE_NewGWorld(CGrafPtr *out, int depth, const Rect *bounds, void *theGDH)
{
    WinBitmap bm;
    short err;

    err = NewGWorld(out, depth, bounds, theGDH);
    GetObjectA((*out)->bitmap, sizeof(bm), &bm);
    (*out)->pix.rowBytes = bm.bmWidthBytes;
    return err;
}
