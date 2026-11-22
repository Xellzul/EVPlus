/* spritetest.cpp -- test pages for the flight drawing, run after the scale test (kScaleTest, mainscreen.cpp):
   scaletest_sprites.raw: game sprites through each draw proc and blend (plain, translucent, tinted, add-over,
   murk), whole and clipped at the frame's edges; beams, lightning (fixed seed), translucent lines, the rect
   copies, BlitPixieFloatingRect and particles, over a fixed background.
   scaletest_window.raw: the second frame the lightning and the particle update draw into.
   scaletest_uisprite.raw: UI::DrawSprite on a frame at UI scale 1.875.
   scaletest_flush.raw: View::Enlarge of a fixed view with each scale filter, then unscaled.
   Which sprites were used is logged to spritetest.txt. Each file must stay the same before and after a
   change; build/devtools/scaletest.sh runs it. */

#include <vector>
#include "burger.h"
#include "game.h"
#include "platform.h"
#include "player.h"
#include "render.h"
#include "screen.h"
#include "space.h"
#include "uiscale.h"
#include "uisprite.h"
#include "viewscale.h"

namespace {

constexpr short kPageWidth = 640;
constexpr short kPageHeight = 480;
constexpr int32_t kLightningSeed = 12345;

/* a sprite's look as the translucent draw proc reads it from the sprite being drawn */
struct Blend {
    short translucency;
    short tintRed;
    short tintGreen;
    short tintBlue;
    short tintMode;
    uint32_t tintColor;
};

constexpr Blend kBlends[] = {
    { 0, 32, 32, 32, 0, 0 },                /* plain */
    { 32, 32, 32, 32, 0, 0 },               /* add-over */
    { 16, 32, 32, 32, 0, 0 },               /* half translucent */
    { 0, 16, 32, 48, 0, 0 },                /* tinted, blue over */
    { 0, 32, 32, 32, 16, 0x03e0 },          /* murk toward green */
    { 40, 40, 24, 32, 8, 0x7c00 },
    { 8, 64, 8, 32, 0, 0 },                 /* red over */
    { 64, 32, 32, 32, 0, 0 },               /* fully translucent: not drawn */
};
constexpr int kBlendCount = sizeof(kBlends) / sizeof(kBlends[0]);

SpriteRec gTestSprite;

SWFrameRec *NewFrame(short width, short height)
{
    SWFrameRec *frame = NULL;
    Rect bounds;

    SetRect(&bounds, 0, 0, width, height);
    if (SWCreateFrame(GetGDevice(), &frame, &bounds, 16, 1) != 0)
        return NULL;
    SWLockFrame(frame);
    return frame;
}

/* red across, green down, a blue checkerboard of 16 pixels; on the device pixels */
void FillBackground(SWFrameRec *frame)
{
    const PixMap &pix = frame->framePort->pix;

    for (int y = 0; y < pix.height; y++) {
        unsigned short *row = (unsigned short *)((char *)pix.baseAddr + (size_t)y * pix.rowBytes);
        for (int x = 0; x < pix.width; x++) {
            unsigned r = (unsigned)(x * 31 / (pix.width - 1));
            unsigned g = (unsigned)(y * 31 / (pix.height - 1));
            unsigned b = (((x >> 4) + (y >> 4)) & 1) ? 24 : 6;
            row[x] = (unsigned short)(r << 10 | g << 5 | b);
        }
    }
}

void FillStripes(SWFrameRec *frame)
{
    const PixMap &pix = frame->framePort->pix;

    for (int y = 0; y < pix.height; y++) {
        unsigned short *row = (unsigned short *)((char *)pix.baseAddr + (size_t)y * pix.rowBytes);
        for (int x = 0; x < pix.width; x++)
            row[x] = ((x + y) / 4 & 1) ? 0x7fe0 : 0x001f;
    }
}

void WriteRaw(const char *name, int width, int height, const unsigned short *pixels, size_t rowPixels)
{
    int32_t size[2] = { width, height };
    FILE *file = PlatFileOpen(name, "wb");

    if (file == NULL)
        return;
    fwrite(size, sizeof(size), 1, file);
    for (int y = 0; y < height; y++)
        fwrite(pixels + (size_t)y * rowPixels, 2, width, file);
    fclose(file);
}

/* the frame's device pixels as raw2png.py reads them */
void WriteFrame(const char *name, SWFrameRec *frame)
{
    const PixMap &pix = frame->framePort->pix;

    WriteRaw(name, pix.width, pix.height, (const unsigned short *)pix.baseAddr, pix.rowBytes / 2);
}

/* frame `index' of a sprite, locked as the game locks a master when it clones it; NULL if it has none */
SWFrameRec *SpriteFrame(SpritePtr sprite, int index, const char *what, FILE *log)
{
    if (sprite == NULL || sprite->frameArray == NULL || sprite->numFrames <= 0) {
        if (log != NULL)
            fprintf(log, "%s: none\n", what);
        return NULL;
    }
    if (index >= sprite->numFrames)
        index = sprite->numFrames - 1;
    SWFrameRec *frame = sprite->frameArray[index];
    if (frame != NULL && !frame->isFrameLocked)
        SWLockSprite(sprite);
    if (frame == NULL || frame->rleTokenStart == NULL) {
        if (log != NULL)
            fprintf(log, "%s: frame %d has no RLE data\n", what, index);
        return NULL;
    }
    if (log != NULL)
        fprintf(log, "%s: frame %d of %d, %d x %d\n", what, index, (int)sprite->numFrames,
                frame->frameRect.right - frame->frameRect.left, frame->frameRect.bottom - frame->frameRect.top);
    return frame;
}

SpritePtr FirstSprite(SpriteRec **sprites, int count)
{
    for (int i = 0; i < count; i++) {
        if (sprites[i] != NULL)
            return sprites[i];
    }
    return NULL;
}

/* a sprite frame centred on cx, cy, drawn as SpriteWorld draws a sprite: with the sprite being drawn set */
void DrawCentred(DrawProcPtr proc, SWFrameRec *sprite, SWFrameRec *dst, int cx, int cy, const Blend &blend,
                 bool throughUI = false)
{
    if (sprite == NULL)
        return;
    Rect src = sprite->frameRect;
    int width = src.right - src.left;
    int height = src.bottom - src.top;
    Rect where;

    SetRect(&where, (short)(cx - width / 2), (short)(cy - height / 2), (short)(cx - width / 2 + width),
            (short)(cy - height / 2 + height));
    gTestSprite.translucency = blend.translucency;
    gTestSprite.tintRed = blend.tintRed;
    gTestSprite.tintGreen = blend.tintGreen;
    gTestSprite.tintBlue = blend.tintBlue;
    gTestSprite.tintMode = blend.tintMode;
    gTestSprite.tintColor = blend.tintColor;
    SpriteRec *saved = gSWCurrentSpriteBeingDrawn;
    gSWCurrentSpriteBeingDrawn = &gTestSprite;
    if (throughUI)
        UI::DrawSprite(proc, sprite, dst, &src, &where);
    else
        proc(sprite, dst, &src, &where);
    gSWCurrentSpriteBeingDrawn = saved;
}

Point At(short h, short v)
{
    Point p;

    p.h = h;
    p.v = v;
    return p;
}

/* each sprite through the plain RLE proc and through the translucent proc with each blend, in rows; then the
   same clipped at the four edges and in the corners */
void DrawSprites(SWFrameRec *page, SWFrameRec *const *sprites, int spriteCount)
{
    constexpr int kColumns = kBlendCount + 1;
    constexpr int kPitch = kPageWidth / (kColumns + 1);

    for (int s = 0; s < spriteCount; s++) {
        int cy = 64 + s * 64;
        for (int c = 0; c < kColumns; c++) {
            int cx = kPitch + c * kPitch;
            if (c == 0)
                DrawCentred(BlitPixieRLEDrawProc, sprites[s], page, cx, cy, kBlends[0]);
            else
                DrawCentred(BlitPixieRLETranslucentDrawProc, sprites[s], page, cx, cy, kBlends[c - 1]);
        }
    }
    for (int c = 0; c < kColumns; c++) {
        int cx = kPitch + c * kPitch;
        SWFrameRec *sprite = sprites[c % spriteCount];
        DrawProcPtr proc = c == 0 ? BlitPixieRLEDrawProc : BlitPixieRLETranslucentDrawProc;
        const Blend &blend = kBlends[c == 0 ? 0 : c - 1];
        DrawCentred(proc, sprite, page, cx, 0, blend);
        DrawCentred(proc, sprite, page, cx, kPageHeight, blend);
    }
    for (int c = 0; c < kColumns; c++) {
        int cy = 48 + c * 44;
        SWFrameRec *sprite = sprites[(c + 1) % spriteCount];
        DrawProcPtr proc = c == 0 ? BlitPixieRLEDrawProc : BlitPixieRLETranslucentDrawProc;
        const Blend &blend = kBlends[c == 0 ? 0 : c - 1];
        DrawCentred(proc, sprite, page, 0, cy, blend);
        DrawCentred(proc, sprite, page, kPageWidth, cy, blend);
    }
    DrawCentred(BlitPixieRLEDrawProc, sprites[0], page, 4, 4, kBlends[0]);
    DrawCentred(BlitPixieRLETranslucentDrawProc, sprites[0], page, kPageWidth - 4, 4, kBlends[2]);
    DrawCentred(BlitPixieRLETranslucentDrawProc, sprites[0], page, 4, kPageHeight - 4, kBlends[1]);
    DrawCentred(BlitPixieRLETranslucentDrawProc, sprites[0], page, kPageWidth - 4, kPageHeight - 4, kBlends[3]);
}

/* the beams as the beam callbacks draw and erase them, lightning from a fixed seed, translucent lines */
void DrawBeams(SWFrameRec *page, SWFrameRec *back, SWFrameRec *window)
{
    int32_t seed = GetQDGlobalsRandomSeed();

    DrawBeam(page, At(20, 330), At(380, 352), 0x7c00, 2, 0x3c0f, 4);
    DrawBeam(page, At(60, 316), At(84, 444), 0x03ff, 3, 0x0010, 2);
    DrawBeam(page, At(-50, 400), At(150, 380), 0x03e0, 1, 0x7fe0, 8);
    DrawBeam(page, At(200, 440), At(390, 330), 0x7fff, 2, 0x7c1f, 0);
    DrawBeam(page, At(300, 300), At(300, 300), 0x7fff, 2, 0x7c1f, 4);
    DrawBeam(page, At(600, 420), At(700, 470), 0x7e00, 2, 0x4000, 4);

    DrawBeam(page, At(120, 420), At(380, 440), 0x7fe0, 2, 0x3c00, 4);
    EraseBeam(back, page, At(120, 420), At(380, 440), 2, 4);
    DrawBeam(page, At(130, 300), At(390, 310), 0x03e0, 1, 0x0200, 6);
    EraseBeam(back, page, At(130, 300), At(390, 310), 0x20, 0x10);

    SetQDGlobalsRandomSeed(kLightningSeed);
    DrawLightningBeam(page, NULL, At(100, 360), At(390, 420), 0x7fff, 2, 10, 6, 32);
    DrawLightningBeam(page, window, At(10, 440), At(250, 320), 0x5eff, 1, 20, 4, 20);
    DrawLightningBeam(page, NULL, At(160, 470), At(380, 476), 0x7f00, 2, 15, 8, 32);
    SetQDGlobalsRandomSeed(kLightningSeed);
    DrawLightningBeam(page, NULL, At(260, 380), At(390, 400), 0x7fff, 2, 10, 6, 32);
    EraseLightningBeam(back, page, At(260, 380), At(390, 400), 2, 10, 6);
    SetQDGlobalsRandomSeed(seed);

    DrawTranslucentLine(page, At(10, 470), At(390, 460), 0x7fff, 32, 4, 0, 32);
    DrawTranslucentLine(page, At(395, 300), At(395, 470), 0x7c00, 8, 32, 20, 10);
    DrawTranslucentLine(page, At(-20, 300), At(40, 470), 0x001f, 32, 32, 32, 32);
}

/* rect copies from a striped frame: whole, clipped at the right edge, translucent at levels 0..32, and the
   bare translucent copy the overlays use */
void DrawRects(SWFrameRec *page, SWFrameRec *stripes)
{
    Rect src;
    Rect dst;

    SetRect(&src, 0, 0, 100, 40);
    SetRect(&dst, 410, 300, 510, 340);
    BlitPixieRectDrawProc(stripes, page, &src, &dst);
    SetRect(&src, 10, 0, 110, 40);
    SetRect(&dst, 590, 300, 690, 340);
    BlitPixieRectDrawProc(stripes, page, &src, &dst);
    for (short level = 0; level <= 32; level += 8) {
        short left = (short)(410 + level / 8 * 46);
        SetRect(&src, 0, 0, 40, 30);
        SetRect(&dst, left, 350, left + 40, 380);
        BlitPixieFloatingRectDrawProc(stripes, page, &src, &dst, level, 0);
    }
    SetRect(&src, 0, 0, 40, 30);
    SetRect(&dst, 620, 390, 660, 420);
    BlitPixieFloatingRectDrawProc(stripes, page, &src, &dst, 16, 0);
    for (int k = 0; k < 3; k++) {
        unsigned short level = (unsigned short)(k * 12 + 4);
        unsigned char *from = (unsigned char *)stripes->frameBaseAddr;
        unsigned char *to = (unsigned char *)page->frameBaseAddr + 390 * page->frameRowBytes + (410 + k * 64) * 2;
        BlitPixieFloatingRect(from, to, stripes->frameRowBytes, page->frameRowBytes, 56 * 2, 30, level);
    }
}

/* particles drawn, moved into the window frame, erased and drawn again where they moved to */
void DrawParticles(SWFrameRec *page, SWFrameRec *back, SWFrameRec *window)
{
    SpriteWorldRec world;

    memset(&world, 0, sizeof(world));
    world.workFrameP = page;
    world.backFrameP = back;
    world.windowFrameP = window;
    world.pixelDepth = 16;
    world.fastBlit = 1;

    ClearParticles(0);
    SetParticleScrollOffset(0, 0);
    for (int i = 0; i < 64; i++) {
        int32_t color = (int32_t)((i * 0x0843 + 0x1234) & 0x7fff);
        int32_t h = (500 + (i % 8) * 16) << 8;
        int32_t v = (440 + (i / 8) * 4) << 8;
        NewParticle(color, h, v, (i % 5 - 2) * 160, (i % 3 - 1) * 200 - 300, (short)(i % 4 == 0 ? 32 : 16),
                    (short)(2 + i * 2));
    }
    DrawParticlesOffscreen(&world);
    UpdateParticlesInWindow(&world);
    EraseParticlesOffscreen(&world);
    for (int k = 0; k < 6; k++)
        MoveParticles();
    DrawParticlesOffscreen(&world);
    UpdateParticlesInWindow(&world);
    ClearParticles(0);
}

/* the scaled UI sprite path: the sprites whole, clipped at the edges, and with a rollover-like blend */
void DrawUISprites(SWFrameRec *const *sprites, int spriteCount)
{
    constexpr Blend kRollover = { 32, 20, 20, 20, 0, 0 };
    SWFrameRec *frame = NULL;
    Rect bounds;

    SetRect(&bounds, 0, 0, 200, 120);
    if (SWCreateFrame(GetGDevice(), &frame, &bounds, 16, 1) != 0 || frame == NULL)
        return;
    UI::ScalePortPixels(frame->framePort, 1.875);
    SWInitializeFrame(frame);
    SWLockFrame(frame);
    FillBackground(frame);
    for (int s = 0; s < spriteCount; s++) {
        int cx = 30 + s * 50;
        DrawCentred(BlitPixieRLEDrawProc, sprites[s], frame, cx, 30, kBlends[0], true);
        DrawCentred(BlitPixieRLETranslucentDrawProc, sprites[s], frame, cx, 85, s % 2 ? kRollover : kBlends[2],
                    true);
    }
    DrawCentred(BlitPixieRLEDrawProc, sprites[0], frame, 0, 60, kBlends[0], true);
    DrawCentred(BlitPixieRLETranslucentDrawProc, sprites[0], frame, 200, 60, kRollover, true);
    DrawCentred(BlitPixieRLEDrawProc, sprites[0], frame, 100, 0, kBlends[0], true);
    DrawCentred(BlitPixieRLEDrawProc, sprites[0], frame, 100, 120, kBlends[0], true);
    WriteFrame("scaletest_uisprite.raw", frame);
    SWDisposeFrame(&frame);
}

/* View::Enlarge of a fixed view into a page of half the screen's height: each filter at 15/8 of the view's
   size, then the view at the screen's own size. The view's pixels are put back. */
void FlushPages(FILE *log)
{
    SWFrameRec *view = gSpriteWorld != NULL ? gSpriteWorld->windowFrameP : NULL;
    const Screen::Page &real = Screen::Get();

    if (view == NULL || view->framePort == NULL || real.pixels == NULL || real.height < 4)
        return;
    PixMap &viewPix = view->framePort->pix;
    int capacity = View::width * View::height;
    int dstWidth = UI::Mapping(gGamePort.pix).X(UI::ScreenWidth() - kStatusBarWidth);
    int screenWidth = real.pitch / 2;
    int pageHeight = real.height / 2;
    if (viewPix.width != View::width || viewPix.height != View::height || capacity <= 0 || dstWidth <= 0
            || dstWidth > screenWidth) {
        if (log != NULL)
            fprintf(log, "flush: no view to test\n");
        return;
    }
    if (log != NULL)
        fprintf(log, "flush: screen %d x %u, view area %d\n", screenWidth, (unsigned)real.height, dstWidth);

    struct Page {
        int width;
        int height;
        int screenHeight;
        short filter;
    };
    int sameHeight = capacity / dstWidth < pageHeight ? capacity / dstWidth : pageHeight;
    const Page pages[] = {
        { dstWidth * 8 / 15, pageHeight * 8 / 15, pageHeight, kScaleFilterNearest },
        { dstWidth * 8 / 15, pageHeight * 8 / 15, pageHeight, kScaleFilterLinear },
        { dstWidth * 8 / 15, pageHeight * 8 / 15, pageHeight, kScaleFilterPixelArt },
        { dstWidth, sameHeight, sameHeight, kScaleFilterLinear },
    };

    unsigned short *viewPixels = (unsigned short *)viewPix.baseAddr;
    std::vector<unsigned short> savedView(viewPixels, viewPixels + capacity);
    short savedWidth = View::width;
    short savedHeight = View::height;
    short savedFilter = settings.scaleFilter;
    std::vector<unsigned short> out;
    std::vector<unsigned short> screen;

    for (const Page &page : pages) {
        if (page.width * page.height > capacity || page.width <= 0 || page.height <= 0)
            continue;
        for (int y = 0; y < page.height; y++) {
            for (int x = 0; x < page.width; x++) {
                unsigned r = (unsigned)(x * 31 / (page.width > 1 ? page.width - 1 : 1));
                unsigned g = (unsigned)(y * 31 / (page.height > 1 ? page.height - 1 : 1));
                unsigned b = ((x / 3 + y / 3) & 1) ? 31 : 0;
                unsigned short p = (unsigned short)(r << 10 | g << 5 | b);
                if (x % 17 == 0 && y % 13 == 0)
                    p = 0x7fff;
                viewPixels[(size_t)y * page.width + x] = p;
            }
        }
        screen.assign((size_t)screenWidth * page.screenHeight, 0x4210);
        View::width = (short)page.width;
        View::height = (short)page.height;
        settings.scaleFilter = page.filter;
        Screen::Page to = real;
        to.pixels = (char *)screen.data();
        to.height = page.screenHeight;
        View::Enlarge(to);
        out.insert(out.end(), screen.begin(), screen.end());
    }

    View::width = savedWidth;
    View::height = savedHeight;
    settings.scaleFilter = savedFilter;
    memcpy(viewPixels, savedView.data(), savedView.size() * 2);
    WriteRaw("scaletest_flush.raw", screenWidth, (int)(out.size() / screenWidth), out.data(), screenWidth);
}

} // namespace

void DebugSpriteTest(void)
{
    FILE *log = PlatFileOpen("spritetest.txt", "wb");
    SWFrameRec *page = NewFrame(kPageWidth, kPageHeight);
    SWFrameRec *back = NewFrame(kPageWidth, kPageHeight);
    SWFrameRec *window = NewFrame(kPageWidth, kPageHeight);
    SWFrameRec *stripes = NewFrame(120, 40);

    if (page != NULL && back != NULL && window != NULL && stripes != NULL) {
        SWFrameRec *found[] = {
            SpriteFrame(masterShipSprite[0], 9, "ship", log),
            SpriteFrame(FirstSprite(masterAsteroidSprite, 16), 5, "asteroid", log),
            SpriteFrame(FirstSprite(masterExplodSprite, 64), 4, "explosion", log),
            SpriteFrame(FirstSprite(masterShotSprite, 256), 3, "shot", log),
            SpriteFrame(masterStarSprite, 0, "star", log),
        };
        SWFrameRec *sprites[5];
        int count = 0;
        for (SWFrameRec *frame : found) {
            if (frame != NULL)
                sprites[count++] = frame;
        }

        FillBackground(page);
        FillBackground(back);
        FillBackground(window);
        FillStripes(stripes);
        if (count > 0)
            DrawSprites(page, sprites, count);
        DrawBeams(page, back, window);
        DrawRects(page, stripes);
        DrawParticles(page, back, window);
        WriteFrame("scaletest_sprites.raw", page);
        WriteFrame("scaletest_window.raw", window);
        if (count > 0)
            DrawUISprites(sprites, count);
    }
    for (SWFrameRec *frame : { page, back, window, stripes }) {
        if (frame != NULL)
            SWDisposeFrame(&frame);
    }
    FlushPages(log);
    if (log != NULL)
        fclose(log);
}
