/* viewscale.cpp -- the space view: its sizes, its place on the screen, where space lies in it; the overlay and UI
   frames. */
#include <math.h>
#include <initializer_list>
#include "burger.h"
#include "player.h"
#include "render.h"
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
int zoomLevel;                      /* Zoom() = 2^(zoomLevel / kZoomStepsPerDouble) */
constexpr int kZoomStepsPerDouble = 3;
constexpr int kMaxZoomLevel = 6;    /* 4 times */
double scale = 1.0;                 /* screen pixels per game pixel; 1 = drawn as is */
double overlayScale = 1.0;          /* view pixels per logical UI pixel: the UI scale over the view's scale */
SWFrameRec *uiWork;
SWFrameRec *uiBack;
SWFrameRec *menuWork;               /* the menu's, when its scale differs from the UI's */
SWFrameRec *menuBack;

/* where the status bar begins on the screen, in device pixels */
int BarDeviceLeft()
{
    return UI::Mapping(gGamePort.pix).X(UI::ScreenWidth() - kStatusBarWidth);
}

/* the size the view is shown at on the screen (enlarged or reduced), from its top left corner */
struct Enlargement {
    int width;
    int height;
};

Enlargement EnlargedOnto(const Screen::Page &page)
{
    return { BarDeviceLeft(), page.height };
}

} // namespace

double Scale()
{
    return scale;
}

void SetSizes(short gameW, short gameH, short drawH, double ui)
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

Rect OnPage(const Screen::Page &page)
{
    Enlargement to = EnlargedOnto(page);
    Rect r;

    SetRect(&r, 0, 0, to.width, to.height);
    return r;
}

Rect OnScreen()
{
    Rect r;

    SetRect(&r, 0, 0, UI::ScreenWidth() - kStatusBarWidth, UI::ScreenHeight());
    return r;
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

int X(float spaceX)
{
    return (short)(spaceX - shipData[0].pos.x) + screenCenter.h;
}

int Y(float spaceY)
{
    return (short)(spaceY - shipData[0].pos.y) + screenCenter.v;
}

float ExactX(float spaceX)
{
    return spaceX - shipData[0].pos.x + screenCenter.h;
}

float ExactY(float spaceY)
{
    return spaceY - shipData[0].pos.y + screenCenter.v;
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

float Zoom()
{
    return (float)pow(2.0, (double)zoomLevel / kZoomStepsPerDouble);
}

void ZoomBy(int notches)
{
    zoomLevel += notches;
    if (zoomLevel < 0)
        zoomLevel = 0;
    if (zoomLevel > kMaxZoomLevel)
        zoomLevel = kMaxZoomLevel;
}

void ResetZoom()
{
    zoomLevel = 0;
}

void Unzoom(Point *pt)
{
    float zoom = Zoom();

    pt->h = (short)floor(screenCenter.h + (pt->h - screenCenter.h) / zoom);
    pt->v = (short)floor(screenCenter.v + (pt->v - screenCenter.v) / zoom);
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

int ScaleInView(int value)
{
    return (int)floor(value * overlayScale + 0.5);
}

} // namespace View
