/* screen.cpp -- the screen page and the frame: the page handed to the platform's window, with the space view the
   GPU draws under it. */
#include <algorithm>
#include "burger.h"
#include "platform.h"
#include "profiler.h"
#include "viewdraw.h"
#include "viewscale.h"
#include "screen.h"

namespace Screen {

namespace {

Page page;
bool viewFresh;                     /* composed since the last present */
bool changed;                       /* something to show since the last frame end */
unsigned shownWhite;                /* the white-out level of the frame last shown */

/* 1.1.1's hyperspace white-out: 1.1.1 has CGDisplayFade blend the whole display to and from solid white over
   1.5 s while the game runs on; here the platform shows the frame blended toward white by the fade's position
   at that moment. The page the game draws into is untouched. */
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

/* The view goes to the platform at its own size, under the page; its place on the page is marked see-through. */
void MarkView()
{
    Rect r = View::OnPage(page);

    for (int y = r.top; y < r.bottom; y++) {
        unsigned short *row = (unsigned short *)(page.pixels + (size_t)y * page.pitch);
        std::fill(row + r.left, row + r.right, (unsigned short)kPlatSeeThrough);
    }
    viewFresh = true;
}

/* a cleared page of width x height; NULL pixels when it can't be had */
Page NewPage(int width, int height, int depth)
{
    Page made;

    made.pixels = (char *)AllocAPointerClear((Word32)(2 * width * height));
    made.pitch = 2 * width;
    made.width = width;
    made.height = height;
    made.depth = depth;
    return made;
}

} // namespace

const Page &Get()
{
    return page;
}

bool Open(int width, int height, int depth)
{
    page = NewPage(width, height, depth);
    return page.pixels != NULL;
}

bool Resize(int width, int height)
{
    Page made = NewPage(width, height, page.depth);

    if (made.pixels == NULL)
        return false;
    if (PlatVideoResize(width, height) != 0) {
        DeallocAPointer(made.pixels);
        return false;
    }
    DeallocAPointer(page.pixels);
    page = made;
    viewFresh = false;
    changed = true;
    return true;
}

void Close()
{
    viewFresh = false;
    if (page.pixels != NULL)
        DeallocAPointer(page.pixels);
    page = Page();
}

void ComposeView()
{
    if (page.pixels == NULL || View::width <= 0)
        return;
    Profiler::Begin(Profiler::Compose);
    MarkView();
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

    if ((!changed && white == shownWhite) || page.pixels == NULL)
        return;
    changed = false;
    shownWhite = white;
    PlatVideoSetWhite(white);
    if (viewFresh) {
        Rect r = View::OnPage(page);
        PlatVideoView view = {};
        view.width = View::width;
        view.height = View::height;
        view.x = r.left;
        view.y = r.top;
        view.w = r.right - r.left;
        view.h = r.bottom - r.top;
        view.zoom = View::Zoom();
        ViewDraw::Take(view);
        viewFresh = false;
        PlatVideoPresentView(page.pixels, page.pitch, &view);
    } else {
        PlatVideoPresent(page.pixels, page.pitch);
    }
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
