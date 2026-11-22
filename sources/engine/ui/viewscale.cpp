/* viewscale.cpp -- the space view drawn at the game size and enlarged onto the screen. */
#include <math.h>
#include <vector>
#include "burger.h"
#include "pixel.h"
#include "player.h"
#include "spriteworld.h"
#include "uiscale.h"
#include "viewscale.h"
#include "screen.h"

namespace View {

short gameWidth;
short width;
short height;

namespace {

short gameHeight;
double scale = 1.0;                 /* screen pixels per game pixel; 1 = drawn as is */
double overlayScale = 1.0;          /* view pixels per logical UI pixel: the UI scale over the view's enlargement */
SWFrameRec *uiWork;
SWFrameRec *uiBack;
SWFrameRec *menuWork;               /* the menu's, when its scale differs from the UI's */
SWFrameRec *menuBack;
CGrafPort viewPort;
std::vector<unsigned short> pixels;

/* one destination pixel's two source pixels along an axis and the weight of the second (0..32) */
struct Tap {
    int first;
    int second;
    unsigned weight;
};

constexpr unsigned kWeightOne = 32;

/* the taps along one axis for the filter; Pixel art blends only within a screen pixel of a texel edge */
std::vector<Tap> Taps(int src, int dst, int filter)
{
    std::vector<Tap> taps(dst);
    double step = (double)src / dst;

    for (int i = 0; i < dst; i++) {
        double center = (i + 0.5) * step - 0.5;
        int first = (int)floor(center);
        double frac = center - first;
        if (filter == kScaleFilterNearest)
            frac = frac < 0.5 ? 0.0 : 1.0;
        else if (filter == kScaleFilterPixelArt)
            frac = fmin(fmax((frac - 0.5) / step + 0.5, 0.0), 1.0);
        if (first < 0) {
            first = 0;
            frac = 0;
        }
        int second = first + 1 < src ? first + 1 : first;
        taps[i] = { first, second, (unsigned)(frac * kWeightOne + 0.5) };
    }
    return taps;
}

/* where the status bar begins on the screen, in device pixels */
int BarDeviceLeft()
{
    return UI::Mapping(gGamePort.pix).X(UI::ScreenWidth() - kStatusBarWidth);
}

/* the size on a page the whole view is enlarged to, from its top left corner */
struct Enlargement {
    int width;
    int height;
};

Enlargement EnlargedOnto(const Screen::Page &page)
{
    return { BarDeviceLeft(), page.height };
}

} // namespace

void SetSizes(short gameW, short gameH, short drawW, short drawH, double ui)
{
    gameWidth = gameW;
    gameHeight = gameH;
    scale = gameH > 0 ? (double)drawH / gameH : 1.0;
    if (fabs(scale - 1.0) < 1e-9)
        scale = 1.0;
    overlayScale = ui / scale;
    width = (short)(UI::ScreenDeviceX(UI::ScreenWidth() - kStatusBarWidth) / scale + 0.5);
    height = gameH;
}

CWindowPtr Open()
{
    pixels.assign((size_t)width * height, 0);
    memset(&viewPort, 0, sizeof(viewPort));
    viewPort.pnSize.h = viewPort.pnSize.v = 1;
    viewPort.pix.pixelSize = 16;
    viewPort.pix.bounds.right = width;
    viewPort.pix.bounds.bottom = height;
    viewPort.pix.baseAddr = pixels.data();
    viewPort.pix.width = width;
    viewPort.pix.height = height;
    viewPort.pix.rowBytes = width * 2;
    viewPort.depth = 15;
    return &viewPort;
}

bool Enlarge(const Screen::Page &page)
{
    static std::vector<Tap> across;
    static std::vector<Tap> down;
    static std::vector<uint32_t> rows;      /* each view row stretched across, spread */
    static int cachedFilter = -1;
    static int cachedWidth;
    static int cachedHeight;
    static int cachedSrcWidth;
    static int cachedSrcHeight;
    Enlargement to = EnlargedOnto(page);
    int dstWidth = to.width;
    int dstHeight = to.height;
    int filter = settings.scaleFilter;

    if (pixels.empty() || dstWidth <= 0 || page.pixels == NULL)
        return false;
    if (dstWidth == width && dstHeight == height) {
        for (int y = 0; y < height; y++)
            memcpy(page.pixels + (size_t)y * page.pitch, &pixels[(size_t)y * width], (size_t)width * 2);
        return true;
    }
    if (filter != cachedFilter || dstWidth != cachedWidth || dstHeight != cachedHeight || width != cachedSrcWidth
            || height != cachedSrcHeight) {
        across = Taps(width, dstWidth, filter);
        down = Taps(height, dstHeight, filter);
        rows.assign((size_t)height * dstWidth, 0);
        cachedFilter = filter;
        cachedWidth = dstWidth;
        cachedHeight = dstHeight;
        cachedSrcWidth = width;
        cachedSrcHeight = height;
    }
    for (int y = 0; y < height; y++) {
        const unsigned short *src = &pixels[(size_t)y * width];
        uint32_t *row = &rows[(size_t)y * dstWidth];
        for (int x = 0; x < dstWidth; x++) {
            const Tap &t = across[x];
            uint32_t a = Pixel::Spread(src[t.first]);
            row[x] = t.weight == 0 ? a : Pixel::Mix(a, Pixel::Spread(src[t.second]), t.weight);
        }
    }
    for (int y = 0; y < dstHeight; y++) {
        const Tap &t = down[y];
        const uint32_t *row0 = &rows[(size_t)t.first * dstWidth];
        const uint32_t *row1 = &rows[(size_t)t.second * dstWidth];
        unsigned short *out = (unsigned short *)(page.pixels + (size_t)y * page.pitch);
        if (t.weight == 0) {
            for (int x = 0; x < dstWidth; x++)
                out[x] = Pixel::Unspread(row0[x]);
        } else {
            for (int x = 0; x < dstWidth; x++)
                out[x] = Pixel::Unspread(Pixel::Mix(row0[x], row1[x], t.weight));
        }
    }
    return true;
}

Rect OverlayFrameRect(const Rect &viewRect)
{
    Rect r = viewRect;

    r.right = (short)(r.left + (r.right - r.left) / overlayScale + 0.5);
    r.bottom = (short)(r.top + (r.bottom - r.top) / overlayScale + 0.5);
    return r;
}

Rect OverlayLocalRect(const Rect &viewRect)
{
    Rect r = viewRect;

    OffsetRect(&r, -r.left, -r.top);
    return OverlayFrameRect(r);
}

Point ToOverlay(Point pt, const Rect &viewRect)
{
    Point local;

    local.h = (short)floor((pt.h - viewRect.left) / overlayScale);
    local.v = (short)floor((pt.v - viewRect.top) / overlayScale);
    return local;
}

void ScaleOverlayFrame(SWFrameRec *frame)
{
    if (!UI::Enabled())
        return;
    UI::ScaleUIPort(frame->framePort);
    SWInitializeFrame(frame);
}

Rect ScreenRectOfView(const Rect &viewRect)
{
    Enlargement to = EnlargedOnto(Screen::Get());
    double across = (double)to.width / width;
    double down = (double)to.height / height;
    Rect r;

    r.left = (short)floor(viewRect.left * across + 0.5);
    r.right = (short)floor(viewRect.right * across + 0.5);
    r.top = (short)floor(viewRect.top * down + 0.5);
    r.bottom = (short)floor(viewRect.bottom * down + 0.5);
    return r;
}

void SetPortToView()
{
    SWSetPortToFrame(gSpriteWorld->windowFrameP);
}

void ToView(Point *pt)
{
    UI::Mapping screen(gGamePort.pix);
    Enlargement to = EnlargedOnto(Screen::Get());

    if (to.width <= 0)
        return;
    pt->h = (short)floor(screen.X(pt->h) * (double)width / to.width);
    pt->v = (short)floor(screen.Y(pt->v) * (double)height / to.height);
}

static void CreateFramePair(Rect bounds, SWFrameRec **work, SWFrameRec **back)
{
    SWCreateFrame(GetGDevice(), work, &bounds, 16, 1);
    SWCreateFrame(GetGDevice(), back, &bounds, 16, 1);
    for (SWFrameRec *frame : { *work, *back }) {
        UI::ScaleUIPort(frame->framePort);
        SWInitializeFrame(frame);
    }
    SWLockFrame(*work);
    SWLockFrame(*back);
}

void CreateUIFrames(Rect bounds)
{
    for (SWFrameRec **frame : { &uiWork, &uiBack, &menuWork, &menuBack }) {
        if (*frame != NULL)
            SWDisposeFrame(frame);
    }
    CreateFramePair(bounds, &uiWork, &uiBack);
    UI::MenuScale menu;
    if (UI::InMenuScale()) {
        SetRect(&bounds, 0, 0, UI::ScreenWidth() - kStatusBarWidth, UI::ScreenHeight());
        CreateFramePair(bounds, &menuWork, &menuBack);
    }
}

SWFrameRec *UIWork()
{
    return UI::InMenuScale() ? menuWork : uiWork;
}

SWFrameRec *UIBack()
{
    return UI::InMenuScale() ? menuBack : uiBack;
}

void SetPortToUIWork()
{
    SWSetPortToFrame(UIWork());
}

void SetPortToUIBack()
{
    SWSetPortToFrame(UIBack());
}

int ScaleInView(int value)
{
    return (int)floor(value * overlayScale + 0.5);
}

} // namespace View
