/* screen.cpp -- the screen and the frame: the interface's layer (the game port's GPU target) handed to the
   platform's window, with the space view the GPU draws under it. */
#include "burger.h"
#include "canvas.h"
#include "dialogs.h"
#include "platform.h"
#include "profiler.h"
#include "uiscale.h"
#include "viewdraw.h"
#include "viewscale.h"
#include "screen.h"

namespace Screen {

namespace {

constexpr int kTargetDepth = 32;    /* a GPU target's pixels */

Page page;
bool viewFresh;                     /* composed since the last present */
bool viewCovered;                   /* drawn over the view's place since that was last made clear */
bool changed;                       /* something to show since the last frame end */
bool redrawing;                     /* inside Redraw */
unsigned shownWhite;                /* the white-out level of the frame last shown */
unsigned drawnEpoch;                /* PlatUIEpoch when the layer was made, or last drawn whole again */
Scene scene;                        /* SetScene */
Scene above;                        /* the innermost Above's */

/* 1.1.1's hyperspace white-out: 1.1.1 has CGDisplayFade blend the whole display to and from solid white over
   1.5 s while the game runs on; here the platform shows the frame blended toward white by the fade's position
   at that moment. What the game draws is untouched. */
unsigned whiteFrom;
unsigned whiteTo;
Word32 whiteStart;
uint32_t whiteTicks;

unsigned CurrentWhite()
{
    uint32_t elapsed = ReadTick() - whiteStart;

    if (elapsed >= whiteTicks)
        return whiteTo;
    return whiteFrom + ((int32_t)whiteTo - (int32_t)whiteFrom) * (int32_t)elapsed / (int32_t)whiteTicks;
}

/* the GPU lost what the targets held: each drawn again, the layer from its scene up */
void Redraw()
{
    UI::GameScale game;                 /* also from a frame end inside a MenuScale: each part takes its own */

    redrawing = true;
    drawnEpoch = PlatUIEpoch();
    UI::Canvas::RedrawTargets();
    PlatTargetClear(gGamePort.target);
    viewCovered = false;
    if (scene != NULL)
        scene();
    DrawShownDialogs();
    if (above != NULL)
        above();
    changed = true;
    redrawing = false;
}

/* the game port on a screen of width x height, drawn through a new layer; false = none made, nothing changed */
bool MakeLayer(int width, int height)
{
    PixMap &pix = gGamePort.pix;
    Page old = { pix.width, pix.height };

    pix.width = width;
    pix.height = height;
    if (!UI::Canvas::MakeTarget(&gGamePort)) {
        pix.width = old.width;
        pix.height = old.height;
        return false;
    }
    pix.pixelSize = kTargetDepth;
    pix.baseAddr = NULL;
    pix.rowBytes = 0;
    gGamePort.depth = kTargetDepth;
    page.width = width;
    page.height = height;
    drawnEpoch = PlatUIEpoch();
    viewCovered = false;
    viewFresh = false;
    changed = true;
    return true;
}

} // namespace

const Page &Get()
{
    return page;
}

bool Open(int width, int height)
{
    return MakeLayer(width, height);
}

bool Resize(int width, int height)
{
    if (!PlatVideoCanResize(width, height) || !MakeLayer(width, height))
        return false;
    PlatVideoResize(width, height);
    return true;
}

void Close()
{
    viewFresh = false;
    UI::Canvas::DropTarget(&gGamePort);
    gGamePort.pix.width = 0;
    gGamePort.pix.height = 0;
    page = Page();
}

void SetScene(Scene shown)
{
    scene = shown;
}

void DrawScene()
{
    if (scene != NULL)
        scene();
}

Above::Above(Scene draw) : under(above)
{
    above = draw;
}

Above::~Above()
{
    above = under;
}

void Drawn(int left, int top, int right, int bottom)
{
    Rect view = View::OnPage(page);

    if (left < view.right && right > view.left && top < view.bottom && bottom > view.top)
        viewCovered = true;
}

void ComposeView()
{
    if (gGamePort.target == NULL || View::width <= 0)
        return;
    Profiler::Begin(Profiler::Compose);
    if (viewCovered) {
        UI::Painter(UI::Surface::Screen()).Clear(View::OnScreen());
        viewCovered = false;
    }
    viewFresh = true;
    ViewDraw::BeginOverlays();
    DrawOverlays();
    changed = true;
    Profiler::End(Profiler::Compose);
}

void DropView()
{
    viewFresh = false;
    ViewDraw::Clear();
    PlatVideoDropView();
}

void Changed()
{
    changed = true;
}

void EndFrame()
{
    unsigned white = CurrentWhite();

    if (gGamePort.target == NULL)
        return;
    if (drawnEpoch != PlatUIEpoch() && !redrawing)
        Redraw();
    if (!changed && white == shownWhite)
        return;
    changed = false;
    shownWhite = white;
    UI::Canvas::FrameDone();
    PlatVideoSetWhite(white);
    PlatVideoView view = {};
    const PlatVideoView *given = NULL;
    if (viewFresh) {
        Rect r = View::OnPage(page);
        view.width = View::width;
        view.height = View::height;
        view.x = r.left;
        view.y = r.top;
        view.w = r.right - r.left;
        view.h = r.bottom - r.top;
        view.zoom = View::Zoom();
        ViewDraw::Take(view);
        viewFresh = false;
        given = &view;
    }
    PlatVideoPresentLayer(gGamePort.target, given);
}

void Idle()
{
    EndFrame();
    PlatWaitEvent(1);
}

void FadeWhite(unsigned from, unsigned to, uint32_t ticks)
{
    whiteFrom = from;
    whiteTo = to;
    whiteTicks = ticks;
    whiteStart = ReadTick();
}

} // namespace Screen
