/* screen.cpp -- the screen page and the frame: the view composed onto the page, the page (or its white-out)
   handed to the platform's window. */
#include "burger.h"
#include "platform.h"
#include "profiler.h"
#include "viewscale.h"
#include "screen.h"

namespace Screen {

namespace {

Page page;
unsigned short *whitePage;          /* the page blended toward white, while a white-out runs */

/* 1.1.1's hyperspace white-out: 1.1.1 has CGDisplayFade blend the whole display to and from solid white over
   1.5 s while the game runs on; here a copy of the page blended toward white is shown in its place, by the
   fade's position at that moment. The page the game draws into is untouched. */
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

/* the page into whitePage, blended toward white by a of 256, linear in each channel as CGDisplayFade's
   kCGDisplayBlendSolidColor mix */
void BlendPageToWhite(unsigned a)
{
    unsigned short lut5[32];

    for (unsigned i = 0; i < 32; i++)
        lut5[i] = i + (((31 - i) * a + 128) >> 8);
    for (int y = 0; y < page.height; y++) {
        const unsigned short *src = (const unsigned short *)(page.pixels + (size_t)y * page.pitch);
        unsigned short *dst = (unsigned short *)((char *)whitePage + (size_t)y * page.pitch);
        for (int x = 0; x < page.width; x++) {
            unsigned short p = src[x];
            dst[x] = lut5[(p >> 10) & 31] << 10 | lut5[(p >> 5) & 31] << 5 | lut5[p & 31];
        }
    }
}

} // namespace

const Page &Get()
{
    return page;
}

bool Open(int width, int height, int depth)
{
    page.pixels = (char *)AllocAPointerClear((Word32)(2 * width * height));
    if (page.pixels == NULL)
        return false;
    page.pitch = 2 * width;
    page.width = width;
    page.height = height;
    page.depth = depth;
    return true;
}

void Close()
{
    if (whitePage != NULL) {
        DeallocAPointer(whitePage);
        whitePage = NULL;
    }
    if (page.pixels != NULL)
        DeallocAPointer(page.pixels);
    page = Page();
}

void ComposeView()
{
    Profiler::Begin(Profiler::Compose);
    if (View::Enlarge(page))
        DrawOverlays();
    Profiler::End(Profiler::Compose);
}

void Present()
{
    if (page.pixels == NULL)
        return;
    unsigned white = CurrentWhite();
    if (white != kWhiteNone && whitePage == NULL)
        whitePage = (unsigned short *)AllocAPointer((Word32)(page.pitch * page.height));
    if (white != kWhiteNone && whitePage != NULL) {
        BlendPageToWhite(white);
        PlatVideoPresent(whitePage, page.pitch);
    } else {
        PlatVideoPresent(page.pixels, page.pitch);
    }
}

void FadeWhite(unsigned from, unsigned to, uint32_t ticks)
{
    whiteFrom = from;
    whiteTo = to;
    whiteTicks = ticks;
    whiteStart = ReadTick();
}

} // namespace Screen
