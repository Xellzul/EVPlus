/* blitdirect.cpp -- CopyBits and CopyMask with a direct-colour (16, 24 or 32-bit) source: at the same size
   converted pixel by pixel, stretched with a bicubic kernel (the one GDI+'s HighQualityBicubic uses). A
   picture's edge is made from its edge pixels only. Rectangles are relative to the first pixel
   of each pixel map, bounds not applied. */

#include <math.h>
#include <algorithm>
#include <vector>
#include "macport.h"

#define kAlpha1555          0x8000      /* bit 15 of a 1555 pixel */
#define kAlpha1555High      0x80        /* the same bit, in the pixel's high byte */
#define kOpaque             255

#define kWeightBits         14          /* a pixel's weights add up to 1 << kWeightBits */
#define kWeightOne          (1 << kWeightBits)
#define kRowBits            6           /* fraction bits kept between the two passes */
#define kKernelReach        2.0         /* the cubic's half width, in source pixels at full size */
#define kPixelHalf          0.5

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
        if (src->pixelSize == 16 && dst->pixelSize == 16) {
            const unsigned short *from = (const unsigned short *)in;
            unsigned short *to = (unsigned short *)out;
            for (x = 0; x < width; x++) {
                if (!alpha || (from[x] & kAlpha1555) != 0)
                    to[x] = from[x] & 0x7fff;
            }
        } else if (src->pixelSize == 24 && dst->pixelSize == 16) {
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

static int ClampInt(int v, int low, int high)
{
    return v < low ? low : v > high ? high : v;
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

/* The source pixels each destination pixel along one axis is made of: for destination pixel i, count[i]
   pixels from tap start[i] on, each a source pixel (clamped into the picture: the edge pixel stands in for
   what lies outside) and its weight. The weights of a pixel add up to kWeightOne. */
struct Axis {
    std::vector<int> start;
    std::vector<int> count;
    std::vector<int> pixel;
    std::vector<int> weight;
    int lowest;                         /* the source pixels all of them need */
    int highest;
};

/* `count' destination pixels made of the source pixels from `start' on, `length' of them, in a picture
   `limit' wide */
static void MakeAxis(Axis &axis, int count, int start, int length, int limit)
{
    double scale = (double)count / length;
    double stretch = scale < 1.0 ? 1.0 / scale : 1.0;
    int maxCount = (int)ceil(2.0 * (kKernelReach * stretch + kPixelHalf)) + 2;
    std::vector<int> w(maxCount);

    axis.start.resize(count);
    axis.count.resize(count);
    axis.pixel.clear();
    axis.weight.clear();
    axis.lowest = limit - 1;
    axis.highest = 0;
    for (int i = 0; i < count; i++) {
        /* where the destination pixel's centre lies in the source, source pixel k covering k - 1/2 .. k + 1/2 */
        double centre = start + (i + kPixelHalf) / scale - kPixelHalf;
        int first = (int)floor(centre - kKernelReach * stretch - kPixelHalf);
        int last = (int)ceil(centre + kKernelReach * stretch + kPixelHalf);
        if (last - first + 1 > maxCount)
            last = first + maxCount - 1;
        int sum = 0;
        int largest = 0;
        for (int k = first; k <= last; k++) {
            double weight = KernelArea((k + kPixelHalf - centre) / stretch) - KernelArea((k - kPixelHalf - centre) / stretch);
            w[k - first] = (int)floor(weight * kWeightOne + 0.5);
            sum += w[k - first];
            if (w[k - first] > w[largest])
                largest = k - first;
        }
        w[largest] += kWeightOne - sum;         /* rounding must not change the brightness */
        axis.start[i] = (int)axis.pixel.size();
        for (int k = first; k <= last; k++) {
            if (w[k - first] == 0)
                continue;
            int at = ClampInt(k, 0, limit - 1);
            axis.pixel.push_back(at);
            axis.weight.push_back(w[k - first]);
            if (at < axis.lowest)
                axis.lowest = at;
            if (at > axis.highest)
                axis.highest = at;
        }
        axis.count[i] = (int)axis.pixel.size() - axis.start[i];
    }
}

/* one source row (R G B A bytes from the axis' lowest pixel on) resampled across into destination columns
   left .. right, `Channels' values each, with kRowBits fraction bits */
template <int Channels>
static void ResampleRow(const Axis &axis, const unsigned char *line, int left, int right, short *row)
{
    for (int x = left; x < right; x++) {
        const int *pixel = &axis.pixel[axis.start[x]];
        const int *w = &axis.weight[axis.start[x]];
        int sum[Channels] = {};
        for (int k = 0; k < axis.count[x]; k++) {
            const unsigned char *c = &line[(pixel[k] - axis.lowest) * 4];
            for (int ch = 0; ch < Channels; ch++)
                sum[ch] += w[k] * c[ch];
        }
        for (int ch = 0; ch < Channels; ch++)
            *row++ = (short)((sum[ch] + (1 << (kWeightBits - kRowBits - 1))) >> (kWeightBits - kRowBits));
    }
}

/* `count' summed R G B triples (kWeightBits + kRowBits fraction bits) as 16-bit pixels */
static void Pack16(const int *sum, int count, unsigned short *out)
{
    constexpr int kShift = kWeightBits + kRowBits;
    constexpr int kHalf = 1 << (kShift - 1);

    for (int x = 0; x < count; x++, sum += 3) {
        int r = std::min(std::max((sum[0] + kHalf) >> kShift, 0), kOpaque);
        int g = std::min(std::max((sum[1] + kHalf) >> kShift, 0), kOpaque);
        int b = std::min(std::max((sum[2] + kHalf) >> kShift, 0), kOpaque);
        out[x] = (unsigned short)(((r >> 3) << 10) | ((g >> 3) << 5) | (b >> 3));
    }
}

/* srcRect of SRC resampled into dstRect of DST. With `alpha' a source pixel without bit 15 is transparent:
   the picture is resampled with its coverage and laid over what DST holds. Only the part of dstRect inside
   DST is worked out. */
static void CopyScaled(PixMapPtr src, PixMapPtr dst, const Rect *srcRect, const Rect *dstRect, int alpha)
{
    int dstWidth = dstRect->right - dstRect->left;
    int dstHeight = dstRect->bottom - dstRect->top;
    if (dstWidth <= 0 || dstHeight <= 0 || srcRect->right <= srcRect->left || srcRect->bottom <= srcRect->top
            || src->width <= 0 || src->height <= 0)
        return;
    /* the destination columns and rows inside DST */
    int left = dstRect->left < 0 ? -dstRect->left : 0;
    int right = dstRect->right > dst->width ? dst->width - dstRect->left : dstWidth;
    int top = dstRect->top < 0 ? -dstRect->top : 0;
    int bottom = dstRect->bottom > dst->height ? dst->height - dstRect->top : dstHeight;
    if (left >= right || top >= bottom)
        return;

    static Axis across;
    static Axis down;
    MakeAxis(across, dstWidth, srcRect->left, srcRect->right - srcRect->left, src->width);
    MakeAxis(down, dstHeight, srcRect->top, srcRect->bottom - srcRect->top, src->height);
    int channels = alpha ? 4 : 3;
    int span = across.highest - across.lowest + 1;
    int rowLength = (right - left) * channels;
    int firstRow = down.highest;
    int lastRow = down.lowest;
    for (int y = top; y < bottom; y++) {
        for (int k = 0; k < down.count[y]; k++) {
            int at = down.pixel[down.start[y] + k];
            if (at < firstRow)
                firstRow = at;
            if (at > lastRow)
                lastRow = at;
        }
    }
    static std::vector<unsigned char> line;     /* one source row as R G B A, coverage already applied */
    static std::vector<short> rows;             /* the source rows needed, each resampled across */
    static std::vector<int> sums;               /* one destination row being summed down */
    line.resize((size_t)span * 4);
    rows.resize((size_t)(lastRow - firstRow + 1) * rowLength);
    sums.resize(rowLength);

    /* across: every source row that is needed */
    for (int y = firstRow; y <= lastRow; y++) {
        const unsigned char *in = PixelAt(src, across.lowest, y);
        for (int x = 0; x < span; x++) {
            Color c;
            ReadPixel(in, src->pixelSize, alpha, &c);
            line[x * 4 + 0] = c.r;
            line[x * 4 + 1] = c.g;
            line[x * 4 + 2] = c.b;
            line[x * 4 + 3] = c.a;
            in += src->pixelSize / 8;
        }
        short *row = &rows[(size_t)(y - firstRow) * rowLength];
        if (alpha)
            ResampleRow<4>(across, line.data(), left, right, row);
        else
            ResampleRow<3>(across, line.data(), left, right, row);
    }

    /* down, a whole row at a time, into the destination */
    for (int y = top; y < bottom; y++) {
        const int *pixel = &down.pixel[down.start[y]];
        const int *w = &down.weight[down.start[y]];
        int *total = sums.data();
        std::fill(sums.begin(), sums.end(), 0);
        for (int k = 0; k < down.count[y]; k++) {
            const short *row = &rows[(size_t)(pixel[k] - firstRow) * rowLength];
            int weight = w[k];
            for (int i = 0; i < rowLength; i++)
                total[i] += weight * row[i];
        }
        const int *sum = sums.data();
        unsigned char *out = PixelAt(dst, dstRect->left + left, dstRect->top + y);
        if (!alpha && dst->pixelSize == 16) {
            Pack16(sum, right - left, (unsigned short *)out);
            continue;
        }
        for (int x = left; x < right; x++, sum += channels, out += dst->pixelSize / 8) {
            int c[4];
            for (int ch = 0; ch < channels; ch++)
                c[ch] = ClampInt((sum[ch] + (1 << (kWeightBits + kRowBits - 1))) >> (kWeightBits + kRowBits),
                                 0, kOpaque);
            if (alpha) {
                if (c[3] == 0)
                    continue;
                if (c[3] < kOpaque) {
                    Color old;
                    ReadPixel(out, dst->pixelSize, 0, &old);
                    c[0] = ClampInt(c[0] + old.r * (kOpaque - c[3]) / kOpaque, 0, kOpaque);
                    c[1] = ClampInt(c[1] + old.g * (kOpaque - c[3]) / kOpaque, 0, kOpaque);
                    c[2] = ClampInt(c[2] + old.b * (kOpaque - c[3]) / kOpaque, 0, kOpaque);
                }
            }
            WritePixel(out, dst->pixelSize, c[0], c[1], c[2]);
        }
    }
}

static void Blit(PixMapPtr src, PixMapPtr dst, const Rect *srcRect, const Rect *dstRect, int alpha)
{
    if (dstRect->right - dstRect->left == srcRect->right - srcRect->left
            && dstRect->bottom - dstRect->top == srcRect->bottom - srcRect->top)
        CopySameSize(src, dst, srcRect, dstRect, alpha);
    else
        CopyScaled(src, dst, srcRect, dstRect, alpha);
}

void BlitDirect16(PixMapPtr src, PixMapPtr dst, Rect *srcRect, Rect *dstRect)
{
    Blit(src, dst, srcRect, dstRect, 0);
}

void BlitDirect24(PixMapPtr src, PixMapPtr dst, Rect *srcRect, Rect *dstRect)
{
    Blit(src, dst, srcRect, dstRect, 0);
}

void BlitDirect32(PixMapPtr src, PixMapPtr dst, Rect *srcRect, Rect *dstRect)
{
    Blit(src, dst, srcRect, dstRect, 0);
}

/* A black mask pixel makes its source pixel opaque, written into the source itself; maskRect is not
   read. */
void BlitDirectMask16(PixMapPtr src, PixMapPtr mask, PixMapPtr dst, Rect *srcRect,
                   Rect *maskRect, Rect *dstRect)
{
    unsigned char *pixels;
    unsigned char *maskPixels;
    int32_t x;
    int32_t y;

    pixels = (unsigned char *)src->baseAddr;
    maskPixels = (unsigned char *)mask->baseAddr;
    for (y = 0; y < src->height; y++) {
        for (x = 0; x < src->width; x++) {
            if (maskPixels[y * mask->rowBytes + x * 2] == 0)
                pixels[y * src->rowBytes + x * 2 + 1] |= kAlpha1555High;
        }
    }
    Blit(src, dst, srcRect, dstRect, 1);
}
