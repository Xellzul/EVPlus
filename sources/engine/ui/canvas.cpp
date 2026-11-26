/* canvas.cpp -- the Painter's GPU backend (canvas.h): shapes in the device pixels scaleddraw.cpp fills, art and
   text as images, one PlatTargetDraw for all a target's Painters gathered. */
#include <math.h>
#include <algorithm>
#include "canvas.h"
#include "glyphatlas.h"
#include "pixel.h"
#include "scratch.h"
#include "screen.h"
#include "spriteworld.h"
#include "uiart.h"
#include "uiscale.h"
#include "viewdraw.h"

namespace UI {

namespace {

std::vector<Canvas *> open;             /* the canvases a Painter draws on */
std::vector<Canvas *> spare;            /* closed ones, kept for their vectors */
std::vector<uint32_t> work;
std::vector<uint32_t> whole;
std::vector<PlatDraw> clipped;          /* draws sent on their own, kept to a clip */

/* the 16-bit pixels one port's canvas copies from a pixel map, kept as an image: changed rows are taken over */
struct PortImage {
    const PixMap *pix;
    CGrafPtr to;                        /* the port copied onto: each keeps what it last took */
    int width;                          /* the pixel map's size */
    int height;
    Rect region;                        /* the map's device pixels the image holds: every part copied so far */
    PlatImage *image;
    unsigned used;                      /* `frames' when it was last copied from */
};

/* an image or a target made for one draw, kept for a later one that fits into it */
struct Scratch {
    PlatImage *image;                   /* one of the two */
    PlatTarget *target;
    int width;
    int height;

    const void *Made() const { return image != NULL ? (const void *)image : target; }
};

std::vector<PortImage> mirrors;
std::vector<Scratch> scratch;
unsigned frames;                        /* FrameDone's count: the mirrors' clock */

constexpr int kScratchStep = 64;                /* scratch sizes are multiples of this, to fit more draws */
constexpr size_t kScratchTexels = 1 << 20;      /* scratch of each kind kept between draws, in pixels */
constexpr unsigned kMirrorIdle = 512;           /* frames after which a port's image not copied from is freed */
constexpr unsigned kMirrorSweep = 64;           /* frames between two looks for such images */

int Width(const Rect &r) { return r.right - r.left; }
int Height(const Rect &r) { return r.bottom - r.top; }

void Ink(PlatDraw &d, const RGBColor &colour)
{
    d.deep = 1;
    d.rgb[0] = (unsigned char)(colour.red >> 8);
    d.rgb[1] = (unsigned char)(colour.green >> 8);
    d.rgb[2] = (unsigned char)(colour.blue >> 8);
}

/* a copy's source part cut to the source's width x height, the place (and the mask's part) cut with it */
bool ClipPart(Rect &s, Rect &d, int width, int height, Rect *m)
{
    int sw = Width(s);
    int sh = Height(s);
    int dw = Width(d);
    int dh = Height(d);

    if (sw <= 0 || sh <= 0 || dw <= 0 || dh <= 0)
        return false;
    if (s.left < 0) {
        d.left = (short)(d.left + dw * -s.left / sw);
        if (m != NULL)
            m->left = (short)(m->left - s.left);
        s.left = 0;
    }
    if (s.top < 0) {
        d.top = (short)(d.top + dh * -s.top / sh);
        if (m != NULL)
            m->top = (short)(m->top - s.top);
        s.top = 0;
    }
    if (s.right > width) {
        d.right = (short)(d.right - dw * (s.right - width) / sw);
        s.right = (short)width;
    }
    if (s.bottom > height) {
        d.bottom = (short)(d.bottom - dh * (s.bottom - height) / sh);
        s.bottom = (short)height;
    }
    return s.left < s.right && s.top < s.bottom && d.left < d.right && d.top < d.bottom;
}

/* 16-bit pixels of `s' as opaque ARGB, clear outside the pixel map and where the mask's pixel at `m' is not black */
void ReadPixels(const PixMap &from, const PixMap *mask, const Rect &s, const Rect &m, std::vector<uint32_t> &out)
{
    int width = Width(s);
    int height = Height(s);

    out.assign((size_t)width * height, 0);
    for (int y = 0; y < height; y++) {
        int row = s.top + y;
        int maskRow = m.top + y;
        if (row < 0 || row >= from.height || (mask != NULL && (maskRow < 0 || maskRow >= mask->height)))
            continue;
        const uint16_t *pixels = Pixel::Row(from, row);
        const uint16_t *through = mask != NULL ? Pixel::Row(*mask, maskRow) : NULL;
        for (int x = 0; x < width; x++) {
            int column = s.left + x;
            int maskColumn = m.left + x;
            if (column < 0 || column >= from.width)
                continue;
            if (through != NULL && (maskColumn < 0 || maskColumn >= mask->width || through[maskColumn] != 0))
                continue;
            out[(size_t)y * width + x] = Pixel::ToArgb(pixels[column]);
        }
    }
}

/* the part `s' of 16-bit pixels (through the mask's at `m') stretched bicubic, read with what the kernel reaches */
void StretchPixels(const PixMap &from, const PixMap *mask, const Rect &s, const Rect &m, int width, int height,
                   std::vector<uint32_t> &out)
{
    double across = (double)Width(s) / width;
    double down = (double)Height(s) / height;
    int reachX = (int)ceil(2.0 * std::max(across, 1.0)) + 2;
    int reachY = (int)ceil(2.0 * std::max(down, 1.0)) + 2;
    Rect read;
    Rect readMask;
    Rect part;

    SetRect(&read, std::max(s.left - reachX, 0), std::max(s.top - reachY, 0),
            std::min(s.right + reachX, (int)from.width), std::min(s.bottom + reachY, (int)from.height));
    readMask = read;
    OffsetRect(&readMask, m.left - s.left, m.top - s.top);
    part = s;
    OffsetRect(&part, -read.left, -read.top);
    ReadPixels(from, mask, read, readMask, whole);
    out.resize((size_t)width * height);
    Art::Stretch(whole.data(), Width(read), Height(read), part, out.data(), width, height);
}

bool Drawable(const PixMap &pix)
{
    return pix.pixelSize == 16 && pix.baseAddr != NULL;
}

/* a premultiplied pixel over another */
uint32_t Over(uint32_t top, uint32_t under)
{
    uint32_t keep = 255 - (top >> 24);
    uint32_t out = 0;

    for (int shift = 0; shift < 32; shift += 8) {
        uint32_t c = (top >> shift & 0xff) + ((under >> shift & 0xff) * keep + 127) / 255;
        out |= std::min(c, 255u) << shift;
    }
    return out;
}

int ScratchSize(int size)
{
    return (size + kScratchStep - 1) / kScratchStep * kScratchStep;
}

/* scratch of one kind, at least width x height, that no waiting draw uses; made when there is none (NULL: not) */
const Scratch *TakeScratch(bool asTarget, int width, int height, bool (*waiting)(const void *))
{
    for (const Scratch &each : scratch)
        if ((each.target != NULL) == asTarget && each.width >= width && each.height >= height
                && !waiting(each.Made()))
            return &each;
    Scratch made = { NULL, NULL, ScratchSize(width), ScratchSize(height) };
    if (asTarget)
        made.target = PlatTargetCreate(made.width, made.height);
    else
        made.image = PlatImageCreate32(made.width, made.height, NULL, 0);
    if (made.Made() == NULL)
        return NULL;
    scratch.push_back(made);
    return &scratch.back();
}

void FreeScratch(const Scratch &each)
{
    PlatImageDestroy(each.image);
    PlatTargetDestroy(each.target);
}

/* scratch no draw waits for is freed, beyond what is kept of each kind for the next draw */
void TrimScratch(bool (*waiting)(const void *))
{
    size_t kept[2] = { 0, 0 };

    for (size_t i = scratch.size(); i-- > 0;) {
        const Scratch &each = scratch[i];
        if (waiting(each.Made()))
            continue;
        size_t &texels = kept[each.target != NULL];
        texels += (size_t)each.width * each.height;
        if (texels > kScratchTexels) {
            FreeScratch(each);
            scratch.erase(scratch.begin() + i);
        }
    }
}

/* the layout's glyphs as parts of the atlas in the text's colour */
class GlyphSink : public TextLayout::Sink {
public:
    GlyphSink(Canvas &canvas, const RGBColor &ink) : canvas(canvas), ink(ink) {}

    void Glyph(const RasterGlyph &glyph, int x, int baseline, const RGBColor *colour,
               const TextLayout::Clip &clip) override
    {
        GlyphAtlas::Place place;
        int left = x + glyph.left;
        int top = baseline - glyph.top;
        int l = std::max(left, clip.left);
        int t = std::max(top, clip.top);
        int r = std::min(left + glyph.width, clip.right);
        int b = std::min(top + glyph.height, clip.bottom);

        if (l >= r || t >= b || !GlyphAtlas::Find(glyph, &place))
            return;
        PlatDraw &d = canvas.Add(kPlatDrawSilhouette, kPlatBlendMix);
        Ink(d, colour != NULL ? *colour : ink);
        d.image = place.page;
        d.x = (float)l;
        d.y = (float)t;
        d.w = (float)(r - l);
        d.h = (float)(b - t);
        d.sx = (float)(place.x + l - left);
        d.sy = (float)(place.y + t - top);
        d.sw = d.w;
        d.sh = d.h;
    }

private:
    Canvas &canvas;
    RGBColor ink;
};

} // namespace

/* -- the canvases ------------------------------------------------------------------------------------------- */

Canvas *Canvas::Open(CGrafPtr port)
{
    Canvas *canvas;

    if (port == NULL || port->target == NULL)
        return NULL;
    for (Canvas *each : open) {
        if (each->port == port) {
            each->users++;
            return each;
        }
    }
    if (spare.empty()) {
        canvas = new Canvas;
    } else {
        canvas = spare.back();
        spare.pop_back();
    }
    canvas->port = port;
    canvas->target = port->target;
    canvas->users = 1;
    canvas->composing = false;
    open.push_back(canvas);
    return canvas;
}

void Canvas::Close()
{
    if (--users > 0)
        return;
    EndComposition();
    Flush();
    open.erase(std::find(open.begin(), open.end(), this));
    spare.push_back(this);
}

/* draws into the target now, kept to clip; the screen is told what they cover */
void Canvas::Send(const PlatDraw *list, int count, const PlatRect *clip)
{
    if (count <= 0 || target == NULL)
        return;
    PlatTargetDraw(target, list, count, NULL, 0, clip);
    if (port != &gGamePort)
        return;
    float left = list[0].x;
    float top = list[0].y;
    float right = left + list[0].w;
    float bottom = top + list[0].h;
    for (int i = 1; i < count; i++) {
        left = std::min(left, list[i].x);
        top = std::min(top, list[i].y);
        right = std::max(right, (float)(list[i].x + list[i].w));
        bottom = std::max(bottom, (float)(list[i].y + list[i].h));
    }
    Screen::Drawn((int)floorf(left), (int)floorf(top), (int)ceilf(right), (int)ceilf(bottom));
}

void Canvas::Flush()
{
    Send(draws.data(), (int)draws.size(), NULL);
    draws.clear();
    used.clear();
    TrimScratch(Waiting);
    SpareScratch(work);
    SpareScratch(whole);
}

void Canvas::FlushAll()
{
    for (Canvas *canvas : open)
        canvas->Flush();
}

void Canvas::FrameDone()
{
    FlushAll();
    frames++;
    if (frames % kMirrorSweep != 0)
        return;
    for (size_t i = mirrors.size(); i-- > 0;) {
        if (frames - mirrors[i].used > kMirrorIdle) {
            PlatImageDestroy(mirrors[i].image);
            mirrors.erase(mirrors.begin() + i);
        }
    }
}

bool Canvas::MakeTarget(CGrafPtr port)
{
    PlatTarget *made = port != NULL ? PlatTargetCreate(port->pix.width, port->pix.height) : NULL;

    if (made == NULL)
        return false;
    DropTarget(port);
    gPortGone = PortGone;
    port->target = made;
    for (Canvas *each : open)
        if (each->port == port)
            each->target = made;
    return true;
}

void Canvas::DropTarget(CGrafPtr port)
{
    if (port == NULL || port->target == NULL)
        return;
    FlushAll();
    for (Canvas *each : open)
        if (each->port == port)
            each->target = NULL;
    PlatTargetDestroy(port->target);
    port->target = NULL;
}

void Canvas::Release()
{
    FlushAll();
    for (const PortImage &each : mirrors)
        PlatImageDestroy(each.image);
    mirrors.clear();
    for (const Scratch &each : scratch)
        FreeScratch(each);
    scratch.clear();
}

/* before a draw into this target: a canvas whose waiting draws read it sends them, to read it as it is now */
void Canvas::Touch()
{
    for (Canvas *other : open) {
        if (other != this && std::find(other->used.begin(), other->used.end(), target) != other->used.end())
            other->Flush();
    }
}

PlatDraw &Canvas::Add(unsigned char kind, unsigned char blend)
{
    Touch();
    draws.push_back(PlatDrawOf(kind, blend));
    return draws.back();
}

/* -- what is kept between draws ----------------------------------------------------------------------------- */

/* a waiting draw uses that image or target */
bool Canvas::Waiting(const void *used)
{
    for (const Canvas *each : open)
        if (std::find(each->used.begin(), each->used.end(), used) != each->used.end())
            return true;
    return false;
}

/* before an image changes or goes: the draws waiting to show it are sent, to show it as it is */
void Canvas::SendUsing(const void *used)
{
    for (Canvas *each : open)
        if (std::find(each->used.begin(), each->used.end(), used) != each->used.end())
            each->Flush();
}

/* a port is disposed: its target and the images of its pixels and of what it copied go */
void Canvas::PortGone(CGrafPtr port)
{
    DropTarget(port);
    for (size_t i = mirrors.size(); i-- > 0;) {
        if (mirrors[i].pix != &port->pix && mirrors[i].to != port)
            continue;
        SendUsing(mirrors[i].image);
        PlatImageDestroy(mirrors[i].image);
        mirrors.erase(mirrors.begin() + i);
    }
}

/* the image of what this port copies from a pixel map, up to date in `part': the map's pixels in, the image's out */
PlatImage *Canvas::Mirror(const PixMap &pix, Rect &part)
{
    Rect region = part;
    size_t at = 0;

    while (at < mirrors.size() && (mirrors[at].pix != &pix || mirrors[at].to != port))
        at++;
    if (at < mirrors.size()) {
        const PortImage &kept = mirrors[at];
        bool sized = kept.width == pix.width && kept.height == pix.height;
        bool held = part.left >= kept.region.left && part.top >= kept.region.top && part.right <= kept.region.right
                    && part.bottom <= kept.region.bottom;
        if (!sized || !held) {
            /* made again over the parts copied so far and this one */
            if (sized)
                SetRect(&region, std::min(part.left, kept.region.left), std::min(part.top, kept.region.top),
                        std::max(part.right, kept.region.right), std::max(part.bottom, kept.region.bottom));
            SendUsing(kept.image);
            PlatImageDestroy(kept.image);
            mirrors.erase(mirrors.begin() + at);
            at = mirrors.size();
        }
    }
    if (at == mirrors.size()) {
        PlatImage *image = PlatImageCreate(Width(region), Height(region), NULL, NULL);
        if (image == NULL)
            return NULL;
        gPortGone = PortGone;
        mirrors.push_back({ &pix, port, pix.width, pix.height, region, image, frames });
    }
    PortImage &mirror = mirrors[at];
    const uint16_t *pixels = Pixel::Row(pix, part.top) + part.left;
    OffsetRect(&part, -mirror.region.left, -mirror.region.top);
    PlatRect rows = { part.left, part.top, Width(part), Height(part) };
    SendUsing(mirror.image);
    PlatImageUpdatePart(mirror.image, &rows, pixels, pix.rowBytes);
    mirror.used = frames;
    return mirror.image;
}

/* an image of at least width x height that no waiting draw shows */
PlatImage *Canvas::ScratchImage(int width, int height)
{
    const Scratch *found = TakeScratch(false, width, height, Waiting);

    return found != NULL ? found->image : NULL;
}

/* a target of at least width x height that no waiting draw reads */
PlatTarget *Canvas::ScratchTarget(int width, int height)
{
    const Scratch *found = TakeScratch(true, width, height, Waiting);

    return found != NULL ? found->target : NULL;
}

/* a target's part as it stands, in a scratch target `pad' pixels in, its edge pixels repeated over the pad */
PlatTarget *Canvas::CopyOfPart(PlatTarget *from, const Rect &part, int pad)
{
    int width = Width(part);
    int height = Height(part);
    PlatTarget *copy = ScratchTarget(width + 2 * pad, height + 2 * pad);
    PlatDraw pieces[9];
    int count = 0;

    if (copy == NULL)
        return NULL;
    auto piece = [&](int sx, int sy, int sw, int sh, int x, int y) {
        PlatDraw &d = pieces[count++];
        d = PlatDrawOf(kPlatDrawImage, kPlatBlendCopy);
        d.source = from;
        d.sx = (float)sx;
        d.sy = (float)sy;
        d.sw = d.w = (float)sw;
        d.sh = d.h = (float)sh;
        d.x = (float)x;
        d.y = (float)y;
    };
    piece(part.left, part.top, width, height, pad, pad);
    if (pad > 0) {
        int right = part.right - 1;
        int bottom = part.bottom - 1;
        piece(part.left, part.top, 1, height, 0, pad);
        piece(right, part.top, 1, height, pad + width, pad);
        piece(part.left, part.top, width, 1, pad, 0);
        piece(part.left, bottom, width, 1, pad, pad + height);
        piece(part.left, part.top, 1, 1, 0, 0);
        piece(right, part.top, 1, 1, pad + width, 0);
        piece(part.left, bottom, 1, 1, 0, pad + height);
        piece(right, bottom, 1, 1, pad + width, pad + height);
    }
    PlatTargetDraw(copy, pieces, count, NULL, 0, NULL);
    return copy;
}

/* -- shapes -------------------------------------------------------------------------------------------------- */

/* a device rect cut to the target, as the port cuts a fill to its pixels */
void Canvas::AddFill(int left, int top, int right, int bottom, const RGBColor &colour, unsigned char alpha)
{
    left = std::max(left, 0);
    top = std::max(top, 0);
    right = std::min(right, (int)port->pix.width);
    bottom = std::min(bottom, (int)port->pix.height);
    if (left >= right || top >= bottom)
        return;
    PlatDraw &d = Add(kPlatDrawFill, kPlatBlendCopy);
    Ink(d, colour);
    d.alpha = alpha;
    d.x = (float)left;
    d.y = (float)top;
    d.w = (float)(right - left);
    d.h = (float)(bottom - top);
}

void Canvas::Fill(const Rect &r, const RGBColor &colour)
{
    Rect d = Mapping(port->pix).DeviceRect(r);

    AddFill(d.left, d.top, d.right, d.bottom, colour, 255);
}

void Canvas::Clear(const Rect &r)
{
    Rect d = Mapping(port->pix).DeviceRect(r);

    AddFill(d.left, d.top, d.right, d.bottom, kBlack, 0);
}

/* one logical pixel wide on each side */
void Canvas::Frame(const Rect &r, const RGBColor &colour)
{
    Rect side;

    if (r.right <= r.left || r.bottom <= r.top)
        return;
    SetRect(&side, r.left, r.top, r.right, r.top + 1);
    Fill(side, colour);
    SetRect(&side, r.left, r.bottom - 1, r.right, r.bottom);
    Fill(side, colour);
    SetRect(&side, r.left, r.top + 1, r.left + 1, r.bottom - 1);
    Fill(side, colour);
    SetRect(&side, r.right - 1, r.top + 1, r.right, r.bottom - 1);
    Fill(side, colour);
}

/* the end point left out: a straight line a rect, a slanted one the pen's block at each device step */
void Canvas::Line(short h0, short v0, short h1, short v1, const RGBColor &colour, short width)
{
    Mapping map(port->pix);
    int x0 = h0;
    int y0 = v0;
    int x1 = h1;
    int y1 = v1;

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
        SetRect(&r, std::min(x0, x1), std::min(y0, y1), std::max(x0, x1) + width, std::max(y0, y1) + width);
        Fill(r, colour);
        return;
    }

    int blockW = Scaled::DevicePen(width, map.ScaleX());
    int blockH = Scaled::DevicePen(width, map.ScaleY());
    int dx0 = map.X(x0);
    int dy0 = map.Y(y0);
    int dx1 = map.X(x1);
    int dy1 = map.Y(y1);
    int steps = std::max(abs(dx1 - dx0), abs(dy1 - dy0));
    int left = 0;
    int top = 0;
    int right = 0;
    int bottom = 0;
    int lastX = 0;
    int lastY = 0;

    for (int i = 0; i <= steps; i++) {
        int x = steps == 0 ? dx0 : dx0 + (int)floor((double)(dx1 - dx0) * i / steps + 0.5);
        int y = steps == 0 ? dy0 : dy0 + (int)floor((double)(dy1 - dy0) * i / steps + 0.5);
        if (i > 0 && y == lastY && abs(x - lastX) == 1 && top == bottom) {
            left = std::min(left, x);
            right = std::max(right, x);
        } else if (i > 0 && x == lastX && abs(y - lastY) == 1 && left == right) {
            top = std::min(top, y);
            bottom = std::max(bottom, y);
        } else if (i == 0 || x != lastX || y != lastY) {
            if (i > 0)
                AddFill(left, top, right + blockW, bottom + blockH, colour, 255);
            left = right = x;
            top = bottom = y;
        }
        lastX = x;
        lastY = y;
    }
    AddFill(left, top, right + blockW, bottom + blockH, colour, 255);
}

/* a circle centred in the rect, its radius half the rect's height: the pixels whose centres lie in it (or its ring) */
void Canvas::Oval(const Rect &r, const RGBColor &colour, short width)
{
    Mapping map(port->pix);
    Rect d = map.DeviceRect(r);
    double cx = (d.left + d.right) / 2.0;
    double cy = (d.top + d.bottom) / 2.0;
    double radius = (d.bottom - d.top) / 2.0;
    double inner = width < 0 ? -1 : radius - Scaled::DevicePen(width, map.ScaleY());

    for (int y = (int)floor(cy - radius); y <= (int)ceil(cy + radius); y++) {
        int last = (int)ceil(cx + radius);
        int start = 0;
        bool inside = false;
        for (int x = (int)floor(cx - radius); x <= last; x++) {
            double dx = x + 0.5 - cx;
            double dy = y + 0.5 - cy;
            double d2 = dx * dx + dy * dy;
            bool on = d2 <= radius * radius && (inner < 0 || d2 > inner * inner);
            if (on && !inside)
                start = x;
            else if (!on && inside)
                AddFill(start, y, x, y + 1, colour, 255);
            inside = on;
        }
        if (inside)
            AddFill(start, y, last + 1, y + 1, colour, 255);
    }
}

/* -- art ----------------------------------------------------------------------------------------------------- */

/* an image or a target's part at a device place, whole pixels on both sides; smooth: stretched by the filter */
void Canvas::AddImage(PlatImage *image, PlatTarget *source, const Rect *part, const Rect &place, unsigned char blend,
                      bool smooth)
{
    if ((image == NULL && source == NULL) || Width(place) <= 0 || Height(place) <= 0)
        return;
    PlatDraw &d = Add(kPlatDrawImage, blend);
    d.image = image;
    d.source = source;
    d.x = (float)place.left;
    d.y = (float)place.top;
    d.w = (float)Width(place);
    d.h = (float)Height(place);
    if (part != NULL) {
        d.sx = (float)part->left;
        d.sy = (float)part->top;
        d.sw = (float)Width(*part);
        d.sh = (float)Height(*part);
    }
    if (smooth)
        d.filter = kPlatFilterLinear;
}

/* pixels of the place's size shown there, through a scratch image */
void Canvas::AddPixels(const uint32_t *pixels, const Rect &place, unsigned char blend)
{
    PlatRect rows = { 0, 0, Width(place), Height(place) };
    PlatImage *image = rows.w > 0 && rows.h > 0 ? ScratchImage(rows.w, rows.h) : NULL;
    Rect part;

    if (image == NULL)
        return;
    PlatImageUpdate32(image, &rows, pixels, rows.w * (int)sizeof(uint32_t));
    SetRect(&part, 0, 0, rows.w, rows.h);
    used.push_back(image);
    AddImage(image, NULL, &part, place, blend, false);
}

bool Canvas::BeginComposition(const Rect &area)
{
    if (composing || !Mapping(port->pix).Scaled() || Width(area) <= 0 || Height(area) <= 0)
        return false;
    composing = true;
    composed = area;
    composition.assign((size_t)Width(area) * Height(area), 0);
    return true;
}

/* the pieces put together, stretched onto the port in one go and laid over what is there */
void Canvas::EndComposition()
{
    Rect shown;
    Rect local;

    if (!composing)
        return;
    composing = false;
    if (SectRect(&composed, &port->pix.bounds, &shown)) {
        local = shown;
        OffsetRect(&local, -composed.left, -composed.top);
        Rect d = Mapping(port->pix).DeviceRect(shown);
        if (Width(d) > 0 && Height(d) > 0) {
            work.resize((size_t)Width(d) * Height(d));
            Art::Stretch(composition.data(), Width(composed), Height(composed), local, work.data(), Width(d),
                         Height(d));
            AddPixels(work.data(), d, kPlatBlendMix);
        }
    }
    SpareScratch(composition);
}

/* a piece's premultiplied pixels, `stride' a row, laid over the composition at its logical place */
void Canvas::Compose(const uint32_t *pixels, int stride, const Rect &place)
{
    int width = Width(composed);
    int height = Height(composed);

    for (int y = 0; y < Height(place); y++) {
        int row = place.top - composed.top + y;
        if (row < 0 || row >= height)
            continue;
        for (int x = 0; x < Width(place); x++) {
            int column = place.left - composed.left + x;
            uint32_t pixel = pixels[(size_t)y * stride + x];
            if (column < 0 || column >= width || pixel >> 24 == 0)
                continue;
            uint32_t &under = composition[(size_t)row * width + column];
            under = pixel >> 24 == 255 ? pixel : Over(pixel, under);
        }
    }
}

/* tiles from the rect's corner, cut at its far edges; on a scaled port put together and stretched as one */
void Canvas::Pattern(const Rect &r, const Image_t *pattern)
{
    Mapping map(port->pix);

    if (pattern == NULL || pattern->Width == 0 || pattern->Height == 0 || Width(r) <= 0 || Height(r) <= 0)
        return;
    if (!map.Scaled()) {
        PlatImage *image = Art::Pattern(pattern);
        Rect d = map.DeviceRect(r);
        if (image == NULL)
            return;
        PlatDraw &tiles = Add(kPlatDrawTiled, kPlatBlendCopy);
        tiles.image = image;
        tiles.x = d.left;
        tiles.y = d.top;
        tiles.w = (float)Width(d);
        tiles.h = (float)Height(d);
        tiles.tileX = d.left;
        tiles.tileY = d.top;
        return;
    }
    if (!Art::Pixels(pattern, false, whole))
        return;
    bool own = BeginComposition(r);
    if (!composing)
        return;
    for (int y = r.top; y < r.bottom; y += pattern->Height) {
        for (int x = r.left; x < r.right; x += pattern->Width) {
            Rect tile;
            SetRect(&tile, x, y, std::min(x + (int)pattern->Width, (int)r.right),
                    std::min(y + (int)pattern->Height, (int)r.bottom));
            Compose(whole.data(), pattern->Width, tile);
        }
    }
    if (own)
        EndComposition();
}

void Canvas::Picture(const Image_t *pic, const Rect &dst)
{
    if (pic == NULL)
        return;
    if (composing) {
        if (Art::PicturePixels(pic, Width(dst), Height(dst), work))
            Compose(work.data(), Width(dst), dst);
        return;
    }
    Rect d = Mapping(port->pix).DeviceRect(dst);
    AddImage(Art::Picture(pic, Width(d), Height(d)), NULL, NULL, d, kPlatBlendCopy, false);
}

/* an unscaled port stretches no icon: one of another size is not drawn */
void Canvas::Icon(const Image_t *icon, const Rect &dst)
{
    Mapping map(port->pix);
    Rect d = map.DeviceRect(dst);

    if (icon == NULL)
        return;
    if (!map.Scaled() && (Width(dst) != (int)icon->Width || Height(dst) != (int)icon->Height))
        return;
    AddImage(Art::Icon(icon, Width(d), Height(d)), NULL, NULL, d, kPlatBlendMix, false);
}

/* -- copies -------------------------------------------------------------------------------------------------- */

void Canvas::Copy(CGrafPtr from, const Rect &src, const Rect &dst, Blit mode)
{
    unsigned char blend = mode == Blit::Copy ? kPlatBlendCopy : kPlatBlendAdd;

    if (from == NULL)
        return;
    if (from->target != NULL)
        CopyTarget(from, src, dst, blend);
    else
        CopyPixels(from->pix, NULL, src, src, dst, blend);
}

void Canvas::CopyMasked(CGrafPtr from, CGrafPtr mask, const Rect &src, const Rect &maskRect, const Rect &dst)
{
    if (from == NULL || mask == NULL)
        return;
    if (from->target != NULL)
        CopyTarget(from, src, dst, kPlatBlendMix);
    else
        CopyPixels(from->pix, &mask->pix, src, maskRect, dst, kPlatBlendMix);
}

/* from a port's target; a stretched copy, and one from this target, goes through a padded copy of the part */
void Canvas::CopyTarget(CGrafPtr from, const Rect &src, const Rect &dst, unsigned char blend)
{
    PlatTarget *source = from->target;
    Rect s;
    Rect d;

    if (composing && blend != kPlatBlendAdd) {
        ComposeTarget(from, src, dst);
        return;
    }
    Scaled::DeviceRects(from->pix, port->pix, src, dst, s, d);
    if (!ClipPart(s, d, from->pix.width, from->pix.height, NULL))
        return;
    bool stretched = Width(s) != Width(d) || Height(s) != Height(d);
    /* what waits to be drawn into the source comes first */
    for (Canvas *other : open)
        if (other->port == from)
            other->Flush();
    if (from == port || stretched) {
        int pad = stretched ? 1 : 0;
        source = CopyOfPart(from->target, s, pad);
        if (source == NULL)
            return;
        OffsetRect(&s, pad - s.left, pad - s.top);
    }
    used.push_back(source);
    AddImage(NULL, source, &s, d, blend, stretched);
}

/* a target's part read back, brought to the logical size and laid into the composition */
void Canvas::ComposeTarget(CGrafPtr from, const Rect &src, const Rect &dst)
{
    Rect s = Mapping(from->pix).DeviceRect(src);
    Rect d = dst;
    Rect all;

    if (!ClipPart(s, d, from->pix.width, from->pix.height, NULL))
        return;
    PlatRect rows = { s.left, s.top, Width(s), Height(s) };
    FlushAll();
    whole.resize((size_t)rows.w * rows.h);
    if (PlatTargetReadDrawn(from->target, &rows, whole.data()) == 0)
        return;
    if (rows.w == Width(d) && rows.h == Height(d)) {
        Compose(whole.data(), rows.w, d);
        return;
    }
    SetRect(&all, 0, 0, rows.w, rows.h);
    work.resize((size_t)Width(d) * Height(d));
    Art::Stretch(whole.data(), rows.w, rows.h, all, work.data(), Width(d), Height(d));
    Compose(work.data(), Width(d), d);
}

bool Canvas::ReadBack(CGrafPtr port, PixMap &into, std::vector<unsigned short> &pixels)
{
    const PixMap &pix = port->pix;
    PlatRect all = { 0, 0, pix.width, pix.height };

    FlushAll();
    whole.resize((size_t)pix.width * pix.height);
    if (whole.empty() || PlatTargetReadDrawn(port->target, &all, whole.data()) == 0)
        return false;
    pixels.resize(whole.size());
    for (size_t i = 0; i < whole.size(); i++)
        pixels[i] = Pixel::FromRGB24(whole[i]);
    into = pix;
    into.pixelSize = 16;
    into.baseAddr = pixels.data();
    into.rowBytes = pix.width * Pixel::kBytes;
    return true;
}

void Canvas::OwnPixels()
{
    CopyPixels(port->pix, NULL, port->pix.bounds, port->pix.bounds, port->pix.bounds, kPlatBlendCopy);
}

/* a port's 16-bit pixels brought to the logical size and laid into the composition */
void Canvas::ComposePixels(const PixMap &from, const PixMap *mask, const Rect &src, const Rect &maskRect,
                           const Rect &dst)
{
    Rect s = Mapping(from).DeviceRect(src);
    Rect m = mask != NULL ? Mapping(*mask).DeviceRect(maskRect) : s;
    Rect d = dst;

    if (!ClipPart(s, d, from.width, from.height, &m))
        return;
    if (Width(s) == Width(d) && Height(s) == Height(d))
        ReadPixels(from, mask, s, m, work);
    else
        StretchPixels(from, mask, s, m, Width(d), Height(d), work);
    Compose(work.data(), Width(d), d);
}

/* a port's 16-bit pixels: from the port's kept image, or masked or stretched (bicubic) through a scratch image */
void Canvas::CopyPixels(const PixMap &from, const PixMap *mask, const Rect &src, const Rect &maskRect,
                        const Rect &dst, unsigned char blend)
{
    static int said;
    Rect s;
    Rect d;
    Rect m;

    if (!Drawable(from) || (mask != NULL && !Drawable(*mask))) {
        PlatDebugLogOnce(&said, "canvas: a copy from pixels that are not 16-bit is not drawn");
        return;
    }
    if (composing && blend != kPlatBlendAdd) {
        ComposePixels(from, mask, src, maskRect, dst);
        return;
    }
    Scaled::DeviceRects(from, port->pix, src, dst, s, d);
    m = mask != NULL ? Mapping(*mask).DeviceRect(maskRect) : s;
    if (!ClipPart(s, d, from.width, from.height, &m))
        return;
    if (Width(s) != Width(d) || Height(s) != Height(d)) {
        StretchPixels(from, mask, s, m, Width(d), Height(d), work);
        AddPixels(work.data(), d, blend);
        return;
    }
    if (mask != NULL) {
        ReadPixels(from, mask, s, m, work);
        AddPixels(work.data(), d, blend);
        return;
    }
    PlatImage *image = Mirror(from, s);
    if (image == NULL)
        return;
    used.push_back(image);
    AddImage(image, NULL, &s, d, blend, false);
}

/* a sprite frame blended as its draw proc blends it: the part src and dst both hold, at dst's corner */
void Canvas::Sprite(DrawProcPtr draw, const SWFrameRec *sprite, const Rect &src, const Rect &dst)
{
    PlatImage *image = Art::Sprite(sprite);
    SpriteBlend blend = {};
    Mapping map(port->pix);
    Rect place;
    Rect frame;

    if (image == NULL)
        return;
    int width = std::min({ Width(src), Width(dst), Width(sprite->frameRect) });
    int height = std::min({ Height(src), Height(dst), Height(sprite->frameRect) });
    if (width <= 0 || height <= 0)
        return;
    SetRect(&place, dst.left, dst.top, dst.left + width, dst.top + height);
    SetRect(&frame, dst.left, dst.top, dst.left + Width(sprite->frameRect), dst.top + Height(sprite->frameRect));
    Rect d = map.DeviceRect(place);
    bool part = width != Width(sprite->frameRect) || height != Height(sprite->frameRect);
    bool stretched = Width(d) != width || Height(d) != height;
    blend.kind = SpriteBlend::Kind::Plain;
    if (draw == BlitPixieRLETranslucentDrawProc && gSWCurrentSpriteBeingDrawn != NULL)
        blend = SpriteBlend::Of(*gSWCurrentSpriteBeingDrawn);
    Touch();
    /* a part of a stretched frame: the whole frame stretched by the GPU's filter, kept to the part's place */
    if (part && stretched) {
        PlatRect clip = { d.left, d.top, Width(d), Height(d) };
        Flush();
        clipped.clear();
        ViewDraw::BlendDraws(image, blend, map.DeviceRect(frame), clipped);
        for (PlatDraw &each : clipped)
            each.filter = kPlatFilterLinear;
        Send(clipped.data(), (int)clipped.size(), &clip);
        return;
    }
    size_t first = draws.size();
    ViewDraw::BlendDraws(image, blend, d, draws);
    for (size_t i = first; i < draws.size(); i++) {
        PlatDraw &each = draws[i];
        if (part) {
            each.sw = (float)width;
            each.sh = (float)height;
        }
        if (stretched)
            each.filter = kPlatFilterLinear;
    }
}

/* -- text ---------------------------------------------------------------------------------------------------- */

void Canvas::Text(const TextLayout::Setup &setup, TextLayout::Pen &pen, const void *text, uint32_t length,
                  const RGBColor &colour)
{
    GlyphSink sink(*this, colour);

    TextLayout::Draw(&sink, Mapping(port->pix), setup, text, length, NULL, false, 0, NULL, 0, pen);
}

int Canvas::TextWrapped(const TextLayout::Setup &setup, const Rect &box, const void *text, uint32_t length,
                        const RGBColor &colour, int scroll, const TextSpan *spans, int spanCount)
{
    GlyphSink sink(*this, colour);
    TextLayout::Pen pen = {};

    return TextLayout::Draw(&sink, Mapping(port->pix), setup, text, length, &box, true, scroll, spans, spanCount,
                            pen);
}

} // namespace UI
