/* viewscale.cpp -- the space view drawn at the game size and enlarged onto the screen. */
#include <math.h>
#include <string.h>
#include <vector>
#include "macport.h"
#include "gameconst.h"
#include "gametypes.h"
#include "burger.h"
#include "player.h"
#include "cefix.h"
#include "spriteworld.h"
#include "uiscale.h"
#include "viewscale.h"

namespace View {

short gameWidth;
short width;
short height;

namespace {

short gameHeight;
double scale = 1.0;                 /* screen pixels per game pixel; 1 = drawn as is */
double uiScale = 1.0;               /* the preferences' UI scale, used inside the view */
SWFrameRec *uiWork;
SWFrameRec *uiBack;
CGrafPort viewPort;
std::vector<unsigned short> pixels;

/* one destination pixel's two source pixels along an axis and the weight of the second (0..32) */
struct Tap {
    int first;
    int second;
    unsigned weight;
};

constexpr unsigned kWeightOne = 32;
constexpr unsigned kSpread = 0x03e07c1f;    /* 5-5-5 with green moved up: room to multiply each channel by 32 */

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

inline uint32_t Spread(unsigned short p)
{
    return (p | (uint32_t)p << 16) & kSpread;
}

inline unsigned short Pack(uint32_t e)
{
    return (unsigned short)((e | e >> 16) & 0x7fff);
}

/* two spread pixels mixed, w/32 of the second */
inline uint32_t Mix(uint32_t a, uint32_t b, unsigned w)
{
    return ((a * (kWeightOne - w) + b * w) >> 5) & kSpread;
}

} // namespace

/* where the status bar begins on the screen, in device pixels */
static int BarDeviceLeft()
{
    return UI::Mapping(gGamePort.pix).X(UI::ScreenWidth() - kStatusBarWidth);
}

void SetSizes(short gameW, short gameH, short drawW, short drawH, double ui)
{
    gameWidth = gameW;
    gameHeight = gameH;
    uiScale = ui;
    scale = gameH > 0 ? (double)drawH / gameH : 1.0;
    if (fabs(scale - 1.0) < 1e-9)
        scale = 1.0;
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

void Flush()
{
    static std::vector<Tap> across;
    static std::vector<Tap> down;
    static std::vector<uint32_t> rows;      /* each view row stretched across, spread */
    static int cachedFilter = -1;
    static int cachedWidth;
    static int cachedHeight;
    int dstWidth = BarDeviceLeft();
    int dstHeight = gVideoHeight;
    int filter = settings.scaleFilter;

    if (pixels.empty() || dstWidth <= 0 || VideoOffscreen == NULL)
        return;
    if (dstWidth == width && dstHeight == height) {
        for (int y = 0; y < height; y++)
            memcpy(VideoOffscreen + (size_t)y * VideoWidth, &pixels[(size_t)y * width], (size_t)width * 2);
        DrawOverlays();
        return;
    }
    if (filter != cachedFilter || dstWidth != cachedWidth || dstHeight != cachedHeight) {
        across = Taps(width, dstWidth, filter);
        down = Taps(height, dstHeight, filter);
        rows.assign((size_t)height * dstWidth, 0);
        cachedFilter = filter;
        cachedWidth = dstWidth;
        cachedHeight = dstHeight;
    }
    for (int y = 0; y < height; y++) {
        const unsigned short *src = &pixels[(size_t)y * width];
        uint32_t *row = &rows[(size_t)y * dstWidth];
        for (int x = 0; x < dstWidth; x++) {
            const Tap &t = across[x];
            uint32_t a = Spread(src[t.first]);
            row[x] = t.weight == 0 ? a : Mix(a, Spread(src[t.second]), t.weight);
        }
    }
    for (int y = 0; y < dstHeight; y++) {
        const Tap &t = down[y];
        const uint32_t *row0 = &rows[(size_t)t.first * dstWidth];
        const uint32_t *row1 = &rows[(size_t)t.second * dstWidth];
        unsigned short *out = (unsigned short *)(VideoOffscreen + (size_t)y * VideoWidth);
        if (t.weight == 0) {
            for (int x = 0; x < dstWidth; x++)
                out[x] = Pack(row0[x]);
        } else {
            for (int x = 0; x < dstWidth; x++)
                out[x] = Pack(Mix(row0[x], row1[x], t.weight));
        }
    }
    DrawOverlays();
}

Rect OverlayFrameRect(const Rect &viewRect)
{
    Rect r = viewRect;

    r.right = (short)(r.left + (r.right - r.left) / uiScale + 0.5);
    r.bottom = (short)(r.top + (r.bottom - r.top) / uiScale + 0.5);
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

    local.h = (short)floor((pt.h - viewRect.left) / uiScale);
    local.v = (short)floor((pt.v - viewRect.top) / uiScale);
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
    double across = (double)BarDeviceLeft() / width;
    double down = (double)gVideoHeight / height;
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
    int dstWidth = BarDeviceLeft();

    if (dstWidth <= 0)
        return;
    pt->h = (short)floor(screen.X(pt->h) * (double)width / dstWidth);
    pt->v = (short)floor(screen.Y(pt->v) * (double)height / gVideoHeight);
}

void CreateUIFrames(Rect bounds)
{
    SWCreateFrame(GetGDevice(), &uiWork, &bounds, 16, 1);
    SWCreateFrame(GetGDevice(), &uiBack, &bounds, 16, 1);
    for (SWFrameRec *frame : { uiWork, uiBack }) {
        UI::ScaleUIPort(frame->framePort);
        SWInitializeFrame(frame);
    }
    SWLockFrame(uiWork);
    SWLockFrame(uiBack);
}

SWFrameRec *UIWork()
{
    return uiWork;
}

SWFrameRec *UIBack()
{
    return uiBack;
}

void SetPortToUIWork()
{
    SWSetPortToFrame(uiWork);
}

void SetPortToUIBack()
{
    SWSetPortToFrame(uiBack);
}

int ScaleInView(int value)
{
    return (int)floor(value * uiScale + 0.5);
}

} // namespace View
