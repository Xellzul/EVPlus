/* viewdraw.cpp -- the space view drawn by the GPU (viewdraw.h). */
#include <stdlib.h>
#include <unordered_map>
#include <vector>
#include "burger.h"
#include "game.h"
#include "pixel.h"
#include "player.h"
#include "render.h"
#include "spriteworld.h"
#include "viewdraw.h"
#include <initializer_list>
#include "viewscale.h"
#include "uiscale.h"

namespace ViewDraw {

namespace {

std::vector<PlatDraw> draws;
std::vector<PlatVertex> vertices;
std::vector<PlatDraw> overlays;
std::vector<PlatDraw> interfaceDraws;   /* the interface layer's sprites, in page pixels over the overlays */
std::vector<PlatDraw> shownOverlays;    /* overlays + interfaceDraws, as Take hands them over */
std::vector<PlatDraw> *building = &draws;   /* where NewDraw adds */
std::unordered_map<const SWFrameRec *, PlatImage *> spriteImages;
struct OverlayImage {
    PlatImage *image;
    int width;
    int height;
};
std::unordered_map<const SWFrameRec *, OverlayImage> overlayImages;
std::vector<uint16_t> scratchPixels;
std::vector<uint8_t> scratchAlpha;
/* A flash covers every frame taken for kFlashMicros from the first: a single frame can be gone before the
   display shows it. Its fill is laid over the draws at Take and lifted again at the next Take. */
constexpr uint64_t kFlashMicros = 50000;
constexpr size_t kNoFlashDraw = (size_t)-1;
bool flashing;
bool flashTaken;
uint64_t flashStart;
uint16_t flashColor;
size_t flashDraw = kNoFlashDraw;

constexpr int kLevels = 32;         /* the game's alpha and weight steps */

/* a weight of 32 (or a product of two, of 32 * 32) as 0..255 */
unsigned char Weight(uint32_t of32)
{
    return (unsigned char)(of32 >= kLevels ? 255 : of32 * 255 / kLevels);
}

unsigned char Weight2(uint32_t of1024)
{
    return (unsigned char)(of1024 >= kLevels * kLevels ? 255 : of1024 * 255 / (kLevels * kLevels));
}

int Clamp32(int v)
{
    return v < 0 ? 0 : v > kLevels ? kLevels : v;
}

/* -- images ---------------------------------------------------------------------------------------- */

/* A sprite frame's RLE tokens as pixels and an alpha: a line start begins the next row, pixel runs are byte
   counts padded to 4, a single-colour run repeats a pixel pair. */
PlatImage *DecodeFrame(const SWFrameRec *frame)
{
    int width = frame->frameRect.right - frame->frameRect.left;
    int height = frame->frameRect.bottom - frame->frameRect.top;
    const unsigned char *tokens = (const unsigned char *)frame->rleTokenStart;

    if (width <= 0 || height <= 0 || tokens == NULL)
        return NULL;
    scratchPixels.assign((size_t)width * height, 0);
    scratchAlpha.assign((size_t)width * height, 0);
    int row = -1;
    int x = 0;
    for (;;) {
        uint32_t token = *(const uint32_t *)tokens;
        uint32_t opcode = token >> kTokenShift;
        int32_t bytes = (int32_t)(token & kCountMask);
        tokens += 4;
        if (opcode == kEndShapeToken)
            break;
        if (opcode == kLineStartToken) {
            row++;
            x = 0;
            if (row >= height)
                break;
            continue;
        }
        int count = bytes / Pixel::kBytes;
        if (opcode == kDrawPixelsToken || opcode == kSingleColorToken) {
            const uint16_t *run = (const uint16_t *)tokens;
            for (int i = 0; i < count; i++) {
                if (row < 0 || x + i < 0 || x + i >= width)
                    continue;
                size_t at = (size_t)row * width + x + i;
                scratchPixels[at] = opcode == kDrawPixelsToken ? run[i] : run[i & 1];
                scratchAlpha[at] = 1;
            }
            tokens += opcode == kDrawPixelsToken ? (bytes + 3) & ~3 : 4;
        } else if (opcode != kSkipPixelsToken) {
            break;
        }
        x += count;
    }
    return PlatImageCreate(width, height, scratchPixels.data(), scratchAlpha.data());
}

PlatImage *SpriteImage(const SWFrameRec *frame)
{
    auto found = spriteImages.find(frame);
    if (found != spriteImages.end())
        return found->second;
    PlatImage *image = DecodeFrame(frame);
    spriteImages[frame] = image;
    return image;
}

/* -- draws ----------------------------------------------------------------------------------------- */

PlatDraw &NewDraw(unsigned char kind, unsigned char blend, PlatImage *image = NULL)
{
    building->push_back(PlatDraw());
    PlatDraw &d = building->back();
    d.kind = kind;
    d.blend = blend;
    d.alpha = 255;
    d.mod[0] = d.mod[1] = d.mod[2] = 255;
    d.image = image;
    return d;
}

void Place(PlatDraw &d, const Rect &r)
{
    d.x = r.left;
    d.y = r.top;
    d.w = r.right - r.left;
    d.h = r.bottom - r.top;
}

void Fill(const Rect &r, uint16_t color)
{
    PlatDraw &d = NewDraw(kPlatDrawFill, kPlatBlendMix);
    Place(d, r);
    d.color = color;
}

/* the system's colour over the whole view, under everything */
void Background(const Rect &view)
{
    RGBColor color = systemData[shipData[0].curSystem < 0 ? 0 : shipData[0].curSystem].rgb;

    Fill(view, (uint16_t)Color2Index(&color));
}

/* the field of a pixel-pair channel value, 0..31 */
uint32_t Field(uint32_t pair, int shift)
{
    return pair >> shift & 0x1f;
}

/* A sprite as SpriteBlend::Of says, added: its pixels times each channel's weight -- after
   the tint (murk, ions, gate white) has taken its share -- over the view kept times the translucency; the
   tint and a weight over 32 come as the sprite's silhouette in their colour. */
void Sprite(const SpriteRec &sprite, const Rect &place)
{
    PlatImage *image = SpriteImage(sprite.curFrameP);
    SpriteBlend blend = SpriteBlend::Of(sprite);

    if (image == NULL || blend.kind == SpriteBlend::Kind::Hidden)
        return;
    PlatDraw &d = NewDraw(kPlatDrawImage, blend.kind == SpriteBlend::Kind::AddOver ? kPlatBlendAdd : kPlatBlendMix,
                          image);
    Place(d, place);
    if (blend.kind != SpriteBlend::Kind::Translucent)
        return;
    uint32_t keep = blend.alpha > kLevels ? kLevels : blend.alpha;
    uint32_t own = blend.overBias < kLevels ? blend.overBias : kLevels;     /* the sprite's share; the rest tint */
    uint32_t weights[3] = { blend.alphaR, blend.alphaG, blend.alphaB };
    /* a 5-bit pixel's share rounds down as in the 16-bit blend: at 1 of 32 it is nothing, and a sprite tinted
       that far (murk at its deepest) shows only the tint -- which is how distant sprites vanish into a murky system */
    for (int c = 0; c < 3; c++)
        d.mod[c] = own <= 1 ? 0 : Weight2(weights[c] * own);
    d.alpha = Weight(kLevels - keep);
    if (own < kLevels) {
        PlatDraw &tint = NewDraw(kPlatDrawSilhouette, kPlatBlendAdd, image);
        Place(tint, place);
        /* the tint colour scaled and rounded down per channel as in the 16-bit blend, so at full tint it
           matches the system's background exactly */
        tint.color = (uint16_t)(Pixel::ScalePair(blend.biasColor, kLevels - own) & 0x7fff);
        for (int c = 0; c < 3; c++)
            tint.mod[c] = Weight(weights[c]);
    }
    uint32_t over[3] = { Field(blend.overR, 10), Field(blend.overG, 5), Field(blend.overB, 0) };
    if (over[0] != 0 || over[1] != 0 || over[2] != 0) {
        PlatDraw &boost = NewDraw(kPlatDrawSilhouette, kPlatBlendAdd, image);
        Place(boost, place);
        boost.color = 0x7fff;
        for (int c = 0; c < 3; c++)
            boost.mod[c] = (unsigned char)(over[c] * 255 / 31);
    }
}

/* -- lines and dots -------------------------------------------------------------------------------- */

/* the vertices from `first' on drawn as triangles: joined to the draw before when that one ends there, so
   the particles and beam lines of a frame go as a few draws */
void Triangles(size_t first)
{
    if (!draws.empty() && draws.back().kind == kPlatDrawTriangles
            && (size_t)(draws.back().first + draws.back().count) == first) {
        draws.back().count = (int)(vertices.size() - draws.back().first);
        return;
    }
    PlatDraw &d = NewDraw(kPlatDrawTriangles, kPlatBlendMix);
    d.first = (int)first;
    d.count = (int)(vertices.size() - first);
}

void Vertex(float x, float y, uint16_t color, int level)
{
    PlatVertex v;
    v.x = x;
    v.y = y;
    v.color = color;
    v.alpha = Weight((uint32_t)Clamp32(level));
    vertices.push_back(v);
}

/* lines and dots are a game pixel wide, and never under a screen pixel where the view is reduced */
float lineWidth = 1.0f;

/* a line-wide strip from a to b, its level changing linearly between the two */
void Strip(float ax, float ay, int levelA, float bx, float by, int levelB, bool longerHoriz, uint16_t color)
{
    float nx = longerHoriz ? 0.0f : 0.5f * lineWidth;
    float ny = longerHoriz ? 0.5f * lineWidth : 0.0f;

    Vertex(ax - nx, ay - ny, color, levelA);
    Vertex(ax + nx, ay + ny, color, levelA);
    Vertex(bx + nx, by + ny, color, levelB);
    Vertex(ax - nx, ay - ny, color, levelA);
    Vertex(bx + nx, by + ny, color, levelB);
    Vertex(bx - nx, by - ny, color, levelB);
}

void DotAt(int x, int y, uint16_t color, int level)
{
    size_t first = vertices.size();
    float side = (lineWidth - 1.0f) / 2;

    Strip(x - side, y + 0.5f, level, x + 1.0f + side, y + 0.5f, level, true, color);
    Triangles(first);
}

/* A beam line as strips (the levels of SWBeams.c BlitTranslucentLine), pixel k of `steps' along the longer
   axis: its level starts at the first pixel's, climbs one step a pixel while within the start ramp, holds at
   the shade, and once within the end ramp falls one step a pixel, the line ending where it reaches 0 */
struct ViewLines : BeamLines {
    void Line(Point srcPoint, Point dstPoint, uint32_t color, short srcShade, short dstShade, short srcRamp,
              short dstRamp) override
    {
        int dx = dstPoint.h - srcPoint.h;
        int dy = dstPoint.v - srcPoint.v;
        bool longerHoriz = abs(dx) > abs(dy);
        int steps = longerHoriz ? abs(dx) : abs(dy);
        int shade = Clamp32(srcShade);
        int upRamp = Clamp32(srcRamp);
        int downRamp = Clamp32(dstRamp);
        int start = upRamp != 0 ? Clamp32(dstShade) : shade;
        uint16_t c = (uint16_t)(color & 0x7fff);

        if (steps == 0) {
            if (start != 0)
                DotAt(srcPoint.h, srcPoint.v, c, start);
            return;
        }
        /* the level at the pixel boundaries and centres where it changes: a step is two knots at one place */
        float knotK[8];
        int knotLevel[8];
        int knots = 0;
        auto knot = [&](float k, int level) {
            knotK[knots] = k;
            knotLevel[knots++] = level;
        };
        int upLast = upRamp > 0 ? (upRamp - 1 < steps ? upRamp - 1 : steps) : -1;   /* the start ramp's last pixel */
        int downFirst = downRamp > 0 ? steps - downRamp + 1 : steps + 1;            /* the end ramp's first pixel */
        if (downFirst < upLast + 1)
            downFirst = upLast + 1;
        int level = start;
        knot(-0.5f, start);
        if (upLast >= 0) {
            if (start < shade) {
                int climb = shade - start < upLast ? shade - start : upLast;
                level = start + climb;
                knot((float)climb, level);
            }
            knot(upLast + 0.5f, level);
        }
        if (downFirst - 1 > upLast) {
            level = shade;
            knot(upLast + 0.5f, level);
            knot(downFirst - 0.5f, level);
        }
        if (downFirst <= steps && level > 0) {
            int end = downFirst + level - 1 < steps ? downFirst + level - 1 : steps;
            knot(downFirst - 0.5f, level);
            knot(end + 0.5f, level - (end - downFirst + 1));
        }
        float ux = (float)dx / steps;
        float uy = (float)dy / steps;
        float ox = srcPoint.h + 0.5f;
        float oy = srcPoint.v + 0.5f;
        size_t first = vertices.size();
        for (int i = 0; i + 1 < knots; i++) {
            if (knotK[i + 1] <= knotK[i] || (knotLevel[i] == 0 && knotLevel[i + 1] == 0))
                continue;
            Strip(ox + ux * knotK[i], oy + uy * knotK[i], knotLevel[i], ox + ux * knotK[i + 1],
                  oy + uy * knotK[i + 1], knotLevel[i + 1], longerHoriz, c);
        }
        if (vertices.size() > first)
            Triangles(first);
    }
};

void Dot(void *, int x, int y, int32_t color, int level)
{
    DotAt(x, y, (uint16_t)(color & 0x7fff), level);
}

/* -- the world ------------------------------------------------------------------------------------- */

/* An interface sprite's rect (screen coordinates, as dialogs use) in the page's pixels. */
Rect OnScreen(const Rect &r)
{
    Rect device;

    SetRect(&device, UI::ScreenDeviceX(r.left), UI::ScreenDeviceY(r.top), UI::ScreenDeviceX(r.right),
            UI::ScreenDeviceY(r.bottom));
    return device;
}

/* The interface layer (cursor, alert, double time): over the view at the interface's scale, so the game size
   and the zoom leave it as it is. */
void InterfaceSprites(SpriteLayerRec *layer)
{
    building = &interfaceDraws;
    for (SpritePtr sprite = layer->headSpriteP; sprite != NULL; sprite = sprite->nextSpriteP) {
        if (sprite->isVisible && sprite->curFrameP != NULL && sprite->curFrameP->isFrameLocked)
            Sprite(*sprite, OnScreen(sprite->destFrameRect));
    }
    building = &draws;
}

/* The sprite world's frame, back to front in SWAnimateSpriteWorld's order: the
   system's colour, each layer's visible sprites, the beams (under the ships after the stellar layer, the rest
   after the target layer), the particles. */
void DrawWorld(SpriteWorldRec *world)
{
    Rect view = world->windRect;
    ViewLines lines;

    draws.clear();
    vertices.clear();
    interfaceDraws.clear();
    flashDraw = kNoFlashDraw;
    lineWidth = View::Scale() < 1.0 ? (float)(1.0 / View::Scale()) : 1.0f;
    Background(view);
    lines.bounds = view;
    for (SpriteLayerRec *layer = world->headSpriteLayerP; layer != NULL; layer = layer->nextSpriteLayerP) {
        if (layer == interfaceLayer) {
            InterfaceSprites(layer);
            continue;
        }
        for (SpritePtr sprite = layer->headSpriteP; sprite != NULL; sprite = sprite->nextSpriteP) {
            if (!sprite->isVisible || sprite->curFrameP == NULL || !sprite->curFrameP->isFrameLocked)
                continue;
            const Rect &r = sprite->destFrameRect;
            if (r.left > view.right || r.right < view.left || r.bottom < view.top || r.top > view.bottom)
                continue;
            Sprite(*sprite, r);
        }
        if (layer == stellarLayer)
            DrawBeams(lines, true);
        else if (layer == targetLayer)
            DrawBeams(lines, false);
    }
    ForEachDrawnParticle(view, Dot, NULL);
    FloaterDrawCallback(world);
}

/* a freed image taken out of the draws still kept, so none is drawn after it is gone */
void Forget(PlatImage *image)
{
    for (PlatDraw &d : draws)
        if (d.image == image)
            d.image = NULL;
    for (std::vector<PlatDraw> *list : { &overlays, &interfaceDraws, &shownOverlays })
        for (PlatDraw &d : *list)
            if (d.image == image)
                d.image = NULL;
    PlatImageDestroy(image);
}

} // namespace

void Install()
{
    gSWDraw = DrawWorld;
    gSWFrameGone = FrameGone;
}

void Clear()
{
    draws.clear();
    vertices.clear();
    interfaceDraws.clear();
    flashDraw = kNoFlashDraw;
    flashing = false;
    if (gSpriteWorld != NULL)
        Background(gSpriteWorld->windRect);
}

void BeginOverlays()
{
    overlays.clear();
}

void Overlay(SWFrameRec *frame, const Rect &place, short level)
{
    const PixMap &pix = frame->framePort->pix;
    int width = place.right - place.left;
    int height = place.bottom - place.top;

    if (width <= 0 || height <= 0 || width > pix.width || height > pix.height)
        return;
    scratchPixels.resize((size_t)width * height);
    scratchAlpha.assign((size_t)width * height, 1);
    for (int y = 0; y < height; y++)
        memcpy(&scratchPixels[(size_t)y * width], Pixel::Row(pix, y), (size_t)width * Pixel::kBytes);
    OverlayImage &kept = overlayImages[frame];
    if (kept.image != NULL && (kept.width != width || kept.height != height)) {
        PlatImageDestroy(kept.image);
        kept.image = NULL;
    }
    if (kept.image != NULL) {
        PlatImageUpdate(kept.image, scratchPixels.data(), scratchAlpha.data());
    } else {
        kept.image = PlatImageCreate(width, height, scratchPixels.data(), scratchAlpha.data());
        kept.width = width;
        kept.height = height;
    }
    if (kept.image == NULL)
        return;
    overlays.push_back(PlatDraw());
    PlatDraw &d = overlays.back();
    d.kind = kPlatDrawImage;
    d.blend = kPlatBlendAdd;
    d.image = kept.image;
    Place(d, place);
    d.mod[0] = d.mod[1] = d.mod[2] = Weight((uint32_t)(kLevels - level));
    d.alpha = 255;
}

void Flash(const RGBColor &color)
{
    RGBColor c = color;

    flashColor = (uint16_t)Color2Index(&c);
    flashing = true;
    flashTaken = false;
}

void Take(PlatVideoView &view)
{
    if (flashDraw < draws.size())
        draws.resize(flashDraw);
    flashDraw = kNoFlashDraw;
    if (flashing && flashTaken && PlatMicroseconds() - flashStart >= kFlashMicros)
        flashing = false;
    if (flashing && gSpriteWorld != NULL) {
        if (!flashTaken)
            flashStart = PlatMicroseconds();
        flashTaken = true;
        flashDraw = draws.size();
        Fill(gSpriteWorld->windRect, flashColor);
    }
    view.draws = draws.data();
    view.drawCount = (int)draws.size();
    view.vertices = vertices.data();
    view.vertexCount = (int)vertices.size();
    shownOverlays = overlays;
    shownOverlays.insert(shownOverlays.end(), interfaceDraws.begin(), interfaceDraws.end());
    view.overlays = shownOverlays.data();
    view.overlayCount = (int)shownOverlays.size();
}

void FrameGone(SWFrameRec *frame)
{
    auto sprite = spriteImages.find(frame);
    if (sprite != spriteImages.end()) {
        if (sprite->second != NULL)
            Forget(sprite->second);
        spriteImages.erase(sprite);
    }
    auto overlay = overlayImages.find(frame);
    if (overlay != overlayImages.end()) {
        if (overlay->second.image != NULL)
            Forget(overlay->second.image);
        overlayImages.erase(overlay);
    }
}

} // namespace ViewDraw
