/* spritetest.cpp -- test pages for the CPU sprite blitters the UI draws with, run after the scale test
   (kScaleTest, mainscreen.cpp):
   scaletest_sprites.raw: game sprites through each draw proc and blend (plain, translucent, tinted, add-over,
   murk), whole and clipped at the frame's edges, and the rect copies, over a fixed background.
   Which sprites were used is logged to spritetest.txt. Each file must stay the same before and after a
   change; build/devtools/scaletest.sh runs it. */

#include "burger.h"
#include "game.h"
#include "platform.h"
#include "player.h"
#include "render.h"

namespace {

constexpr short kPageWidth = 640;
constexpr short kPageHeight = 480;

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
void DrawCentred(DrawProcPtr proc, SWFrameRec *sprite, SWFrameRec *dst, int cx, int cy, const Blend &blend)
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
    proc(sprite, dst, &src, &where);
    gSWCurrentSpriteBeingDrawn = saved;
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

/* rect copies from a striped frame: whole, and clipped at the right edge */
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
}

} // namespace

void DebugSpriteTest(void)
{
    FILE *log = PlatFileOpen("spritetest.txt", "wb");
    SWFrameRec *page = NewFrame(kPageWidth, kPageHeight);
    SWFrameRec *stripes = NewFrame(120, 40);

    if (page != NULL && stripes != NULL) {
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
        FillStripes(stripes);
        if (count > 0)
            DrawSprites(page, sprites, count);
        DrawRects(page, stripes);
        WriteFrame("scaletest_sprites.raw", page);
    }
    for (SWFrameRec *frame : { page, stripes }) {
        if (frame != NULL)
            SWDisposeFrame(&frame);
    }
    if (log != NULL)
        fclose(log);
}
