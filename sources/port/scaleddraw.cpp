/* scaleddraw.cpp -- the drawing primitives on a scaled pixel map: logical coordinates mapped to device pixels
   (uiscale.h), then drawn as the original primitives draw. */
#include <math.h>
#include <stdlib.h>
#include "macport.h"
#include "uiscale.h"
#include "scaleddraw.h"

namespace Scaled {

namespace {

unsigned short Pixel555(const RGBColor &color)
{
    return (unsigned short)(gGammaRamp[color.red >> 11] << 10 | gGammaRamp[color.green >> 11] << 5
                            | gGammaRamp[color.blue >> 11]);
}

/* a device rect cut to the pixel map; false when nothing is left */
bool ClipToPixels(const PixMap &pix, Rect &device)
{
    if (device.left < 0)
        device.left = 0;
    if (device.top < 0)
        device.top = 0;
    if (device.right > pix.width)
        device.right = (short)pix.width;
    if (device.bottom > pix.height)
        device.bottom = (short)pix.height;
    return device.left < device.right && device.top < device.bottom;
}

unsigned short *Row(const PixMap &pix, int y)
{
    return (unsigned short *)((char *)pix.baseAddr + (size_t)y * pix.rowBytes);
}

void FillDevice(const PixMap &pix, Rect device, unsigned short pixel)
{
    if (pix.pixelSize != 16 || !ClipToPixels(pix, device))
        return;
    for (int y = device.top; y < device.bottom; y++) {
        unsigned short *row = Row(pix, y);
        for (int x = device.left; x < device.right; x++)
            row[x] = pixel;
    }
}

void PlotDevice(const PixMap &pix, int x, int y, unsigned short pixel)
{
    if (x >= 0 && y >= 0 && x < pix.width && y < pix.height)
        Row(pix, y)[x] = pixel;
}

/* the pen's size in device pixels, at least one */
int DevicePen(int logical, double scale)
{
    int device = (int)floor(logical * scale + 0.5);

    return device < 1 ? 1 : device;
}

} // namespace

void FillRect(const Rect &r, const RGBColor &color)
{
    const PixMap &pix = CurrentGrafPort->pix;

    FillDevice(pix, UI::Mapping(pix).DeviceRect(r), Pixel555(color));
}

void InvertRect(const Rect &r)
{
    const PixMap &pix = CurrentGrafPort->pix;
    Rect device = UI::Mapping(pix).DeviceRect(r);

    if (pix.pixelSize != 16 || !ClipToPixels(pix, device))
        return;
    for (int y = device.top; y < device.bottom; y++) {
        unsigned short *row = Row(pix, y);
        for (int x = device.left; x < device.right; x++)
            row[x] = (unsigned short)(~row[x] & 0x7fff);
    }
}

/* one logical pixel wide on each side, as the original (it ignores the pen size) */
void FrameRect(const Rect &r, const RGBColor &color)
{
    Rect side;

    if (r.right <= r.left || r.bottom <= r.top)
        return;
    SetRect(&side, r.left, r.top, r.right, r.top + 1);
    FillRect(side, color);
    SetRect(&side, r.left, r.bottom - 1, r.right, r.bottom);
    FillRect(side, color);
    SetRect(&side, r.left, r.top + 1, r.left + 1, r.bottom - 1);
    FillRect(side, color);
    SetRect(&side, r.right - 1, r.top + 1, r.right, r.bottom - 1);
    FillRect(side, color);
}

/* From the pen to (h, v), the end point left out, as the original. Straight lines are rects (no seams with
   rects drawn beside them); slanted ones are drawn in device pixels with the pen as a block. */
void LineTo(short h, short v)
{
    CGrafPort &port = *CurrentGrafPort;
    UI::Mapping map(port.pix);
    int x0 = port.pnLoc.h;
    int y0 = port.pnLoc.v;
    int x1 = h;
    int y1 = v;
    int penW = port.pnSize.h;
    int penH = port.pnSize.v;

    port.pnLoc.h = h;
    port.pnLoc.v = v;
    if (x0 > x1)
        x0--;
    else if (x0 != x1)
        x1--;
    if (y0 > y1)
        y0--;
    else if (y0 != y1)
        y1--;

    if (x0 == x1 || y0 == y1) {
        Rect r;
        SetRect(&r, x0 < x1 ? x0 : x1, y0 < y1 ? y0 : y1, (x0 > x1 ? x0 : x1) + penW, (y0 > y1 ? y0 : y1) + penH);
        FillRect(r, port.foreColor);
        return;
    }

    unsigned short pixel = Pixel555(port.foreColor);
    int blockW = DevicePen(penW, map.ScaleX());
    int blockH = DevicePen(penH, map.ScaleY());
    int dx0 = map.X(x0);
    int dy0 = map.Y(y0);
    int dx1 = map.X(x1);
    int dy1 = map.Y(y1);
    int steps = abs(dx1 - dx0) > abs(dy1 - dy0) ? abs(dx1 - dx0) : abs(dy1 - dy0);

    for (int i = 0; i <= steps; i++) {
        int x = steps == 0 ? dx0 : dx0 + (int)floor((double)(dx1 - dx0) * i / steps + 0.5);
        int y = steps == 0 ? dy0 : dy0 + (int)floor((double)(dy1 - dy0) * i / steps + 0.5);
        for (int by = 0; by < blockH; by++)
            for (int bx = 0; bx < blockW; bx++)
                PlotDevice(port.pix, x + bx, y + by, pixel);
    }
}

namespace {

/* the original's circle: centred in the rect, its radius half the rect's height */
struct Circle {
    double cx;
    double cy;
    double radius;
};

Circle DeviceCircle(const Rect &r)
{
    UI::Mapping map(CurrentGrafPort->pix);
    Rect d = map.DeviceRect(r);

    return { (d.left + d.right) / 2.0, (d.top + d.bottom) / 2.0, (d.bottom - d.top) / 2.0 };
}

/* the pixels whose centres lie within the circle and, for a ring, outside the inner one */
void DrawCircle(const Circle &c, double inner)
{
    const PixMap &pix = CurrentGrafPort->pix;
    unsigned short pixel = Pixel555(CurrentGrafPort->foreColor);

    for (int y = (int)floor(c.cy - c.radius); y <= (int)ceil(c.cy + c.radius); y++) {
        for (int x = (int)floor(c.cx - c.radius); x <= (int)ceil(c.cx + c.radius); x++) {
            double dx = x + 0.5 - c.cx;
            double dy = y + 0.5 - c.cy;
            double d2 = dx * dx + dy * dy;
            if (d2 <= c.radius * c.radius && (inner < 0 || d2 > inner * inner))
                PlotDevice(pix, x, y, pixel);
        }
    }
}

} // namespace

void FrameOval(const Rect &r)
{
    Circle c = DeviceCircle(r);
    int pen = DevicePen(CurrentGrafPort->pnSize.v, UI::Mapping(CurrentGrafPort->pix).ScaleY());

    DrawCircle(c, c.radius - pen);
}

void PaintOval(const Rect &r)
{
    DrawCircle(DeviceCircle(r), -1);
}

namespace {

constexpr int kModeCopy = 0;
constexpr int kModeOr = 1;

/* the pixel map as an unscaled one of its device pixels, for the original blitters */
PixMap DeviceView(const PixMap &pix)
{
    PixMap view = pix;

    SetRect(&view.bounds, 0, 0, pix.width, pix.height);
    return view;
}

int Width(const Rect &r) { return r.right - r.left; }
int Height(const Rect &r) { return r.bottom - r.top; }

/* srcOr from an 8-bit icon to 16-bit, stretched to the nearest pixel (sharp, as small icons should be);
   colour 0 is left out */
void StretchOr8(const PixMap &src, const PixMap &dst, const Rect &s, Rect d)
{
    Rect full = d;

    if (src.pixelSize != 8 || dst.pixelSize != 16 || !ClipToPixels(dst, d))
        return;
    for (int y = d.top; y < d.bottom; y++) {
        int sy = s.top + (y - full.top) * Height(s) / Height(full);
        const unsigned char *in = (const unsigned char *)src.baseAddr + (size_t)sy * src.rowBytes;
        unsigned short *out = Row(dst, y);
        for (int x = d.left; x < d.right; x++) {
            unsigned idx = in[s.left + (x - full.left) * Width(s) / Width(full)];
            if (idx == 0)
                continue;
            const unsigned char *rgb = src.ctab + idx * 3;
            out[x] = (unsigned short)((rgb[0] >> 3) << 10 | (rgb[1] >> 3) << 5 | rgb[2] >> 3);
        }
    }
}

/* two scales close enough to be one rounded differently */
bool SameScale(double a, double b)
{
    return fabs(a / b - 1.0) < 0.01;
}

/* The device rects of a copy. When both sides have the same scale and the copy the same logical size, it is
   only moved: the destination keeps its rect and the source is taken at the same device size, so nothing is
   resampled. */
void DeviceRects(const PixMap &src, const PixMap &dst, const Rect &srcRect, const Rect &dstRect, Rect &s, Rect &d)
{
    UI::Mapping from(src);
    UI::Mapping to(dst);

    s = from.DeviceRect(srcRect);
    d = to.DeviceRect(dstRect);
    if (Width(srcRect) != Width(dstRect) || Height(srcRect) != Height(dstRect)
        || !SameScale(from.ScaleX(), to.ScaleX()) || !SameScale(from.ScaleY(), to.ScaleY()))
        return;
    s.right = (short)(s.left + Width(d));
    s.bottom = (short)(s.top + Height(d));
    if (s.right > src.width)
        OffsetRect(&s, src.width - s.right, 0);
    if (s.bottom > src.height)
        OffsetRect(&s, 0, src.height - s.bottom);
}

} // namespace

void CopyBits(const PixMap &src, const PixMap &dst, const Rect &srcRect, const Rect &dstRect, int mode)
{
    PixMap from = DeviceView(src);
    PixMap to = DeviceView(dst);
    Rect s;
    Rect d;

    DeviceRects(src, dst, srcRect, dstRect, s, d);
    bool stretched = Width(s) != Width(d) || Height(s) != Height(d);
    if (!stretched || mode == kModeCopy) {
        ::CopyBits((BitMapPtr)&from, (BitMapPtr)&to, &s, &d, mode, NULL);
        return;
    }
    if (mode == kModeOr) {
        StretchOr8(from, to, s, d);
        return;
    }
    /* a blend mode: the source stretched into a scratch page first, then blended at the same size */
    CGrafPtr scratch;
    Rect area;
    SetRect(&area, 0, 0, Width(d), Height(d));
    if (NewGWorld(&scratch, 16, &area, NULL) != 0)
        return;
    ::CopyBits((BitMapPtr)&from, (BitMapPtr)&scratch->pix, &s, &area, kModeCopy, NULL);
    ::CopyBits((BitMapPtr)&scratch->pix, (BitMapPtr)&to, &area, &d, mode, NULL);
    DisposeGWorld(scratch);
}

void CopyMask(const PixMap &src, const PixMap &mask, const PixMap &dst, const Rect &srcRect, const Rect &maskRect,
              const Rect &dstRect)
{
    PixMap from = DeviceView(src);
    PixMap through = DeviceView(mask);
    PixMap to = DeviceView(dst);
    Rect s;
    Rect d;
    Rect m = UI::Mapping(mask).DeviceRect(maskRect);

    DeviceRects(src, dst, srcRect, dstRect, s, d);
    ::CopyMask((BitMapPtr)&from, (BitMapPtr)&through, (BitMapPtr)&to, &s, &m, &d);
}

} // namespace Scaled

namespace UI {

namespace {

/* the composition being put together: its port, the logical area, and two 1x pages over that area -- the art
   and its mask (0 = drawn, as CopyMask reads masks; anything else = left as it was) */
struct Composition {
    CGrafPtr target = NULL;
    Rect area;
    CGrafPtr art = NULL;
    CGrafPtr mask = NULL;
} gComposition;

void MarkDrawn(const Rect &local)
{
    CGrafPtr saved = CurrentGrafPort;
    RGBColor black = { 0, 0, 0 };

    CurrentGrafPort = gComposition.mask;
    ::FillRectWithColor((Rect *)&local, &black);
    CurrentGrafPort = saved;
}

/* a piece's mask added to the composition's: what either draws is drawn */
void AddToMask(const PixMap &mask, const Rect &maskRect, const Rect &local)
{
    CGrafPtr piece;
    Rect bounds;
    Rect clipped;

    SetRect(&bounds, 0, 0, local.right - local.left, local.bottom - local.top);
    if (bounds.right <= 0 || bounds.bottom <= 0 || NewGWorld(&piece, 16, &bounds, NULL) != 0)
        return;
    ::CopyBits((BitMapPtr)&mask, (BitMapPtr)&piece->pix, &maskRect, &bounds, Scaled::kModeCopy, NULL);
    const PixMap &all = gComposition.mask->pix;
    if (SectRect(&local, &all.bounds, &clipped)) {
        for (int y = clipped.top; y < clipped.bottom; y++) {
            const unsigned short *from = Scaled::Row(piece->pix, y - local.top);
            unsigned short *to = Scaled::Row(all, y);
            for (int x = clipped.left; x < clipped.right; x++)
                to[x] &= from[x - local.left];
        }
    }
    DisposeGWorld(piece);
}

Rect Local(const Rect &r)
{
    Rect local = r;

    OffsetRect(&local, -gComposition.area.left, -gComposition.area.top);
    return local;
}

} // namespace

ArtComposition::ArtComposition(CGrafPtr port, const Rect &area) : target(NULL)
{
    Rect bounds;

    if (gComposition.target != NULL || port == NULL || !Mapping(port->pix).Scaled())
        return;
    SetRect(&bounds, 0, 0, area.right - area.left, area.bottom - area.top);
    if (bounds.right <= 0 || bounds.bottom <= 0)
        return;
    if (NewGWorld(&gComposition.art, 16, &bounds, NULL) != 0)
        return;
    if (NewGWorld(&gComposition.mask, 16, &bounds, NULL) != 0) {
        DisposeGWorld(gComposition.art);
        gComposition.art = NULL;
        return;
    }
    CGrafPtr saved = CurrentGrafPort;
    RGBColor white = { 0xffff, 0xffff, 0xffff };
    CurrentGrafPort = gComposition.mask;
    ::FillRectWithColor(&bounds, &white);
    CurrentGrafPort = saved;
    gComposition.target = port;
    gComposition.area = area;
    target = port;
}

ArtComposition::~ArtComposition()
{
    if (target == NULL)
        return;
    Rect shown;
    gComposition.target = NULL;                 /* the copy below goes to the port itself */
    if (SectRect(&gComposition.area, &target->pix.bounds, &shown)) {
        Rect local = Local(shown);
        ::CopyMask((BitMapPtr)&gComposition.art->pix, (BitMapPtr)&gComposition.mask->pix,
                   (BitMapPtr)&target->pix, &local, &local, &shown);
    }
    DisposeGWorld(gComposition.art);
    DisposeGWorld(gComposition.mask);
    gComposition.art = gComposition.mask = NULL;
}

bool ComposeCopy(PixMap *src, PixMap *dst, const Rect *srcRect, const Rect *dstRect, int mode)
{
    if (gComposition.target == NULL || dst != &gComposition.target->pix || mode != Scaled::kModeCopy)
        return false;
    Rect local = Local(*dstRect);
    ::CopyBits((BitMapPtr)src, (BitMapPtr)&gComposition.art->pix, srcRect, &local, mode, NULL);
    MarkDrawn(local);
    return true;
}

bool ComposeMask(PixMap *src, PixMap *mask, PixMap *dst, const Rect *srcRect, const Rect *maskRect,
                 const Rect *dstRect)
{
    if (gComposition.target == NULL || dst != &gComposition.target->pix)
        return false;
    Rect local = Local(*dstRect);
    ::CopyMask((BitMapPtr)src, (BitMapPtr)mask, (BitMapPtr)&gComposition.art->pix, (Rect *)srcRect,
               (Rect *)maskRect, &local);
    AddToMask(*mask, *maskRect, local);
    return true;
}

} // namespace UI
