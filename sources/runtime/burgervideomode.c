// COMPILE: -lang c++ -bool off
/* burgervideomode.c -- BurgerLib GrWin95Code on the platform layer (platform.h): the display mode, the page lock and flip, the release, the filled rectangle. The game draws a 5-5-5 offscreen page and the flip hands it to the platform's frame; DirectDraw's surfaces, palette and gamma ramp are gone. Notes: Research/notes/burgervideomode.md */

#include "burgertypes.h"
#include "burger.h"
#include "platform.h"

#ifdef __cplusplus
extern "C" {
#endif

Word ScreenClipRight;                            /* 0x0086ffe4 */
Word ScreenClipBottom;                           /* 0x008701c0 */
Word ScreenClipLeft;                             /* 0x00870d8c */
Word ScreenClipTop;                              /* 0x0087091c */
char *VideoPointer;                              /* 0x00870114 */
char *VideoOffscreen;                            /* 0x00870dbc the page the game draws */
static unsigned short *WhitePage;                /* the page blended toward white, while a white-out runs */
int32_t VideoHardWidth;                             /* 0x00870de8 its bytes per row */
Word VideoColorDepth;

#define kFullscreen      1                      /* SETDISPLAYFULLSCREEN */
#define kDoubleOk        4                      /* SETDISPLAYDOUBLEOK */
#define kDepth555        15
#define kBytesPerPixel   2

/* LockVideoPage (BurgerLib GrWin95Code): the page is the offscreen, always there. */
// FUNCTION: EVNOVA 0x004ec480
void LockVideoPage(void)
{
    VideoPointer = VideoOffscreen;
    VideoWidth = VideoHardWidth;
}

/* Port addition (not in the exe): 1.1.1's hyperspace white-out. 1.1.1 has CGDisplayFade blend the whole
   display to and from solid white over 1.5 s while the game runs on; here a copy of the offscreen blended
   toward white is shown in its place, by the fade's position at that moment. The offscreen the game draws
   into is untouched. */
static Word whiteFrom;
static Word whiteTo;
static Word32 whiteStart;
static uint32_t whiteTicks;

void VideoFadeWhite(Word from, Word to, uint32_t ticks)
{
    whiteFrom = from;
    whiteTo = to;
    whiteTicks = ticks;
    whiteStart = ReadTick();
}

static Word CurrentWhite(void)
{
    uint32_t elapsed;

    elapsed = ReadTick() - whiteStart;
    if (elapsed >= whiteTicks)
        return whiteTo;
    return whiteFrom + ((int32_t)whiteTo - (int32_t)whiteFrom) * (int32_t)elapsed / (int32_t)whiteTicks;
}

/* the offscreen into WhitePage, blended toward white by a of 256, linear in each channel as CGDisplayFade's
   kCGDisplayBlendSolidColor mix */
static void BlendPageToWhite(Word a)
{
    unsigned short lut5[32];
    const unsigned short *src;
    unsigned short *dst;
    unsigned short p;
    ULong x;
    ULong y;
    Word i;

    for (i = 0; i < 32; i++)
        lut5[i] = i + (((31 - i) * a + 128) >> 8);
    for (y = 0; y < ScreenHeight; y++) {
        src = (const unsigned short *)(VideoOffscreen + y * VideoHardWidth);
        dst = (unsigned short *)((char *)WhitePage + y * VideoHardWidth);
        for (x = 0; x < ScreenWidth; x++) {
            p = src[x];
            dst[x] = lut5[(p >> 10) & 31] << 10 | lut5[(p >> 5) & 31] << 5 | lut5[p & 31];
        }
    }
}

/* UpdateAndPageFlip (BurgerLib GrWin95Code): show the offscreen. */
// FUNCTION: EVNOVA 0x004ec960
void UpdateAndPageFlip(void)
{
    Word white;

    if (VideoOffscreen == 0)
        return;
    white = CurrentWhite();
    if (white != kWhiteNone && WhitePage == 0)
        WhitePage = (unsigned short *)AllocAPointer(VideoHardWidth * ScreenHeight);
    if (white != kWhiteNone && WhitePage != 0) {
        BlendPageToWhite(white);
        PlatVideoPresent(WhitePage, VideoHardWidth);
    } else {
        PlatVideoPresent(VideoOffscreen, VideoHardWidth);
    }
}

/* ReleaseVideo (BurgerLib GrWin95Code). */
// FUNCTION: EVNOVA 0x004ec990
void ReleaseVideo(void)
{
    PlatVideoClose();
    if (WhitePage != 0) {
        DeallocAPointer(WhitePage);
        WhitePage = 0;
    }
    if (VideoOffscreen != 0) {
        DeallocAPointer(VideoOffscreen);
        VideoOffscreen = 0;
    }
    ScreenWidth = 0;
    ScreenHeight = 0;
}

/* SetDisplayToSize (BurgerLib SetDisplayToSize). The one mode there is: a 5-5-5 offscreen of the size asked
   for (depth 15), shown full screen or in a window; the refresh rate in depth's high word is not used. */
// FUNCTION: EVNOVA 0x004ecbd0
int32_t SetDisplayToSize(ULong width, ULong height, ULong depth, ULong flags)
{
    depth = (unsigned short)depth;
    ReleaseVideo();
    flags &= ~kDoubleOk;
    if (depth != kDepth555
        || PlatVideoOpen(width, height, (flags & kFullscreen) != 0) != 0) {
        NonFatal("Couldn't set display mode %dx%dx%d\n", width, height, depth);
        return 1;
    }
    ScreenWidth = width;
    ScreenHeight = height;
    ScreenFlags = flags;
    ScreenClipTop = 0;
    ScreenClipLeft = 0;
    ScreenClipRight = width;
    ScreenClipBottom = height;
    VideoColorDepth = depth;
    VideoHardWidth = kBytesPerPixel * width;
    VideoOffscreen = (char *)AllocAPointer(VideoHardWidth * height);
    LockVideoPage();
    return 0;
}

/* Fill with a 16-bit value; an odd last byte takes its low half. */
// FUNCTION: EVNOVA 0x005273c0
static void FastMemSet16(void *dst, ULong value, ULong bytes)
{
    while (bytes >= 2) {
        *(unsigned short *)dst = value;
        dst = (unsigned short *)dst + 1;
        bytes -= 2;
    }
    if (bytes != 0)
        *(unsigned char *)dst = value;
}

/* DrawARect16 (BurgerLib DrawARect16). */
// FUNCTION: EVNOVA 0x004ee300
void DrawARect16(int x, int y, Word Width, Word Height, Word Color)
{
    char *Screenad;

    if (x >= (int)ScreenClipRight || y >= (int)ScreenClipBottom)
        goto Exit;
    Width += x;
    Height += y;
    if ((int)Width <= (int)ScreenClipLeft || (int)Height <= (int)ScreenClipTop)
        goto Exit;
    if (x < (int)ScreenClipLeft)
        goto ClipLeft;
    if (y < (int)ScreenClipTop)
        goto ClipTop;
ClipRightTest:
    if (Width > ScreenClipRight)
        goto ClipRight;
ClipBottomTest:
    if (Height > ScreenClipBottom)
        goto ClipBottom;
Clipped:
    Width -= x;
    Height -= y;
    x *= kBytesPerPixel;
    Width *= kBytesPerPixel;
    Screenad = VideoPointer + (VideoWidth * y + x);
    x = VideoWidth;
    if (Width == x)
        goto OneFill;
    do {
        FastMemSet16(Screenad, Color, Width);
        Screenad += x;
    } while (--Height);
Exit:
    return;
OneFill:
    FastMemSet16(Screenad, Color, Width * Height);
    return;

ClipLeft:
    x = ScreenClipLeft;
    if (y >= (int)ScreenClipTop)
        goto ClipRightTest;
ClipTop:
    y = ScreenClipTop;
    if (Width <= ScreenClipRight)
        goto ClipBottomTest;
ClipRight:
    Width = ScreenClipRight;
    if (Height <= ScreenClipBottom)
        goto Clipped;
ClipBottom:
    Height = ScreenClipBottom;
    goto Clipped;
}

#ifdef __cplusplus
}
#endif
