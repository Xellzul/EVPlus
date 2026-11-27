/* gputest.cpp -- EVPLUS_GPUTEST: Painter drawings, a dialog and the overlays on GPU targets vs. a reference */

#include <math.h>
#include <algorithm>
#include <string>
#include <vector>
#include "canvas.h"
#include "dialogs.h"
#include "game.h"
#include "missions.h"
#include "painter.h"
#include "pixel.h"
#include "platform.h"
#include "render.h"
#include "spriteworld.h"
#include "stellar.h"
#include "text.h"
#include "uiscale.h"

namespace {

constexpr short kPageWidth = 640;
constexpr short kPageHeight = 670;
constexpr double kTestBrightness = 0.8;     /* the exponent of the page's second drawing at scale 1 */
constexpr double kTestScales[] = { 1.0, 1.25, 1.875, 2.0 };

constexpr short kPictBig = 133;
constexpr short kPictArrow = 134;
constexpr short kIconMapArrow = 15000;
constexpr short kPatternStatic = 128;
constexpr short kFirstPict = 128;           /* where a direct-colour picture is looked for */
constexpr short kLastPict = 20000;
constexpr int kDeepPictMax = 300;

/* what is expected of an item, by its letter in gputest.txt */
constexpr char kSame = 'G';             /* the same pixels at 5 bits a channel, at every scale */
constexpr char kSameAtOne = 'S';        /* the same at scale 1; stretched or put together at a greater depth elsewhere */
constexpr char kDeeper = 'A';           /* blended at a greater depth: a level off */
constexpr char kText = 'T';             /* text: the levels of its edges differ */
constexpr char kFiltered = 'F';         /* stretched by the GPU's filter where the port stretches bicubic */
constexpr char kLook = 'L';             /* a look changed by design */

struct Item {
    const char *name;
    char kind;
    Rect r;
};

std::vector<Item> items;
bool recording;
char refFolder[kPlatPathMax];           /* EVPLUS_GPUTEST_REF: 16-bit drawings, a renderer's GPU ones in its folder */

void Mark(const char *name, char kind, short left, short top, short right, short bottom)
{
    Item item = { name, kind, { top, left, bottom, right } };

    if (recording)
        items.push_back(item);
}

RGBColor Colour(unsigned short red, unsigned short green, unsigned short blue)
{
    RGBColor colour = { red, green, blue };

    return colour;
}

/* a port for a page or a source: pixels at the scale, with a GPU target of that size when asked */
struct TestPort {
    SWFrameRec *frame = NULL;

    bool Make(short width, short height, double scale, bool gpu)
    {
        Rect bounds;

        SetRect(&bounds, 0, 0, width, height);
        if (SWCreateFrame(GetGDevice(), &frame, &bounds, 16, 1) != 0 || frame == NULL)
            return false;
        UI::ScalePortPixels(frame->framePort, scale);
        SWInitializeFrame(frame);
        SWLockFrame(frame);
        return !gpu || UI::Canvas::MakeTarget(frame->framePort);
    }

    void Drop()
    {
        if (frame == NULL)
            return;
        UI::Canvas::DropTarget(frame->framePort);
        SWDisposeFrame(&frame);
    }

    /* the target's device pixels as premultiplied ARGB, once every canvas is sent */
    bool Read(uint32_t *out) const
    {
        UI::Canvas::FlushAll();
        return PlatTargetRead(frame->framePort->target, NULL, out) != 0;
    }

    UI::Surface Surface() const { return UI::Surface(frame); }
};

/* what the page copies from: gradient, status and button at its scale (GPU targets there), the rest at theirs */
struct Sources {
    TestPort gradient;
    TestPort piece;
    TestPort mask;
    TestPort status;
    TestPort button;
    TestPort icons;
};

void DrawSources(const Sources &sources)
{
    Rect band;
    Rect all;

    {
        UI::Painter p(sources.gradient.Surface());
        for (short x = 0; x < 120; x += 10) {
            SetRect(&band, x, 0, x + 10, 60);
            p.Fill(band, Colour(x * 500, 0x4000, 0xffff - x * 500));
        }
        SetRect(&band, 20, 10, 100, 50);
        p.Oval(band, UI::kWhite, 2);
        p.Line(0, 59, 119, 0, UI::kBlack);
    }
    SetRect(&all, 0, 0, 120, 60);
    {
        UI::Painter p(sources.piece.Surface());
        for (short y = 0; y < 60; y += 6) {
            SetRect(&band, 0, y, 120, y + 6);
            p.Fill(band, Colour(0xffff - y * 1000, y * 1000, 0x8000));
        }
    }
    {
        /* a mask as a decoded picture's pixels: its oval painted by QuickDraw */
        UI::Painter p(sources.mask.Surface());
        p.Fill(all, UI::kWhite);
        SetRect(&band, 10, 10, 110, 50);
        RGBForeColor(&UI::kBlack);
        PaintOval(&band);
        SetRect(&band, 0, 0, 30, 30);
        p.Fill(band, UI::kBlack);
    }
    {
        UI::Painter p(sources.status.Surface());
        p.Fill(all, Colour(0x1000, 0x1800, 0x3000));
        for (short x = 0; x < 120; x += 15) {
            SetRect(&band, x, 0, x + 7, 60);
            p.Fill(band, Colour(0x2000 + x * 300, 0x3000, 0x5000));
        }
        SetRect(&band, 4, 4, 116, 56);
        p.Frame(band, Colour(0x8000, 0x8000, 0xc000));
        SetRect(&band, 40, 14, 80, 46);
        p.Oval(band, Colour(0, 0xc000, 0), 1);
    }
}

void DrawShapes(UI::Painter &p)
{
    Rect r;

    Mark("fills", kSame, 8, 4, 160, 44);
    SetRect(&r, 8, 4, 58, 44);
    p.Fill(r, Colour(0xffff, 0, 0));
    SetRect(&r, 58, 4, 108, 44);
    p.Fill(r, Colour(0, 0xffff, 0));
    SetRect(&r, 108, 4, 160, 44);
    p.Fill(r, Colour(0x1234, 0x5678, 0xffff));
    SetRect(&r, 20, 10, 150, 38);
    p.Frame(r, Colour(0xffff, 0xffff, 0));

    Mark("frames", kSame, 166, 4, 230, 44);
    SetRect(&r, 166, 4, 230, 44);
    p.Frame(r, UI::kWhite);
    SetRect(&r, 171, 9, 225, 39);
    p.Frame(r, Colour(0x8000, 0x8000, 0x8000));
    SetRect(&r, 180, 15, 181, 16);
    p.Frame(r, Colour(0xffff, 0, 0));

    Mark("lines", kSame, 236, 4, 430, 44);
    p.Line(236, 4, 316, 44, Colour(0xffff, 0xffff, 0));
    p.Line(236, 44, 316, 4, Colour(0xffff, 0xffff, 0));
    p.Line(320, 4, 320, 44, Colour(0, 0xffff, 0xffff), 3);
    p.Line(328, 24, 380, 24, Colour(0, 0xffff, 0xffff), 3);
    p.Line(330, 6, 378, 18, Colour(0xffff, 0x8000, 0), 3);
    p.Line(386, 42, 396, 6, Colour(0xffff, 0, 0xffff));
    p.Line(428, 6, 400, 40, Colour(0x8000, 0xffff, 0x8000), 2);
    p.Line(340, 30, 341, 31, UI::kWhite);

    Mark("ovals", kSame, 436, 4, 636, 44);
    SetRect(&r, 436, 4, 476, 44);
    p.Oval(r, Colour(0, 0xffff, 0xffff));
    SetRect(&r, 482, 4, 522, 44);
    p.Oval(r, Colour(0xffff, 0xffff, 0), 3);
    SetRect(&r, 528, 4, 568, 44);
    p.FillOval(r, Colour(0xffff, 0x4000, 0x4000));
    SetRect(&r, 574, 8, 606, 31);
    p.Oval(r, UI::kWhite, 2);
    SetRect(&r, 612, 12, 630, 30);
    p.FillOval(r, Colour(0x4000, 0x4000, 0xffff));
}

void DrawStyles(UI::Painter &p)
{
    static const struct {
        UI::Style style;
        const char *name;
        const char *item;
    } kStyles[] = {
        { UI::Style::Small, "Small", "text Small" },       { UI::Style::Status, "Status", "text Status" },
        { UI::Style::StatusLarge, "StatusLarge", "text StatusLarge" }, { UI::Style::Menu, "Menu", "text Menu" },
        { UI::Style::Button, "Button", "text Button" },    { UI::Style::Title, "Title", "text Title" },
        { UI::Style::System, "System", "text System" },    { UI::Style::Comm, "Comm", "text Comm" },
        { UI::Style::Version, "Version", "text Version" },
    };
    static const RGBColor kColours[] = { { 0xffff, 0xffff, 0xffff }, { 0xffff, 0xc000, 0 },
                                         { 0x4000, 0xffff, 0x4000 }, { 0x8000, 0xc000, 0xffff },
                                         { 0xffff, 0x4000, 0x4000 }, { 0xc000, 0xc000, 0xc000 } };
    short y = 48;
    int i = 0;

    for (const auto &each : kStyles) {
        std::string line = std::string(each.name) + ": The quick brown fox, 0123456789";
        Mark(each.item, kText, 8, y, 336, y + 20);
        short end = p.Text(8, y + 15, line, { each.style, kColours[i % 6] });
        p.Text(" bold", { each.style, kColours[(i + 1) % 6], true });
        (void)end;
        y += 20;
        i++;
    }
}

void DrawTexts(UI::Painter &p)
{
    static const char kParagraph[] =
        "Word wrap: the Federation patrol hails you as you leave the system, asking whether you have seen any "
        "pirate activity near the asteroid belt.\rA second paragraph follows the first.";
    static const char kSpanned[] =
        "Spans: mission text with a highlighted name and a second highlight further on, scrolled up by a few "
        "rows so that its first line is cut by the top of the box.";
    static const TextSpan kSpans[] = { { 7, 19, { 0xffff, 0xc000, 0 } }, { 46, 62, { 0x4000, 0xffff, 0xffff } } };
    Rect box;

    Mark("text pen", kText, 344, 48, 636, 68);
    p.Text(344, 62, "Credits: ", { UI::Style::System, UI::kWhite });
    p.Text("25,000", { UI::Style::System, Colour(0xffff, 0xc000, 0) });
    p.Text("  hostile", { UI::Style::System, Colour(0xffff, 0, 0), true });

    Mark("text aligned", kText, 344, 70, 636, 104);
    p.Text(344, 636, 84, "Centred between the edges", { UI::Style::Small, UI::kWhite }, UI::Align::Center);
    p.Text(344, 636, 100, "Ending at the right edge", { UI::Style::Small, Colour(0x8000, 0xffff, 0x8000) },
           UI::Align::Right);
    p.Line(344, 102, 636, 102, Colour(0x6000, 0x6000, 0x6000));

    Mark("textbox", kText, 344, 108, 636, 168);
    SetRect(&box, 344, 108, 636, 168);
    p.TextBox(box, kParagraph, (int32_t)strlen(kParagraph), { UI::Style::Small, UI::kWhite },
              Colour(0x2000, 0x2000, 0x4000));

    Mark("spans", kText, 344, 172, 636, 226);
    SetRect(&box, 344, 172, 636, 226);
    p.Fill(box, Colour(0x1000, 0x3000, 0x1000));
    p.TextSpans(box, kSpanned, (uint32_t)strlen(kSpanned), { UI::Style::Small, Colour(0xc000, 0xc000, 0xc000) }, 5,
                kSpans, 2);
}

SpriteRec gTestSprite;

SWFrameRec *SpriteFrame(SpritePtr sprite, int index)
{
    if (sprite == NULL || sprite->frameArray == NULL || sprite->numFrames <= 0)
        return NULL;
    if (index >= sprite->numFrames)
        index = sprite->numFrames - 1;
    SWFrameRec *frame = sprite->frameArray[index];
    if (frame != NULL && !frame->isFrameLocked)
        SWLockSprite(sprite);
    return frame != NULL && frame->rleTokenStart != NULL ? frame : NULL;
}

/* a sprite's look as the translucent draw proc reads it from the sprite being drawn */
struct Look {
    short translucency;
    short tint[3];
};

constexpr Look kPlain = { 0, { 32, 32, 32 } };
constexpr Look kRollover = { 32, { 20, 20, 20 } };      /* the main menu's glow: added at 20 of 32 */
constexpr Look kBoosted = { 16, { 20, 28, 32 } };       /* channels over 32: the port ORs the excess in */

/* a sprite frame with its top left at left, top, through the translucent draw proc unless plain */
void DrawSprite(UI::Painter &p, SWFrameRec *sprite, short left, short top, const Look &look)
{
    if (sprite == NULL)
        return;
    Rect src = sprite->frameRect;
    Rect where;
    bool plain = look.translucency == 0 && look.tint[0] == 32;

    SetRect(&where, left, top, left + (src.right - src.left), top + (src.bottom - src.top));
    gTestSprite.translucency = look.translucency;
    gTestSprite.tintRed = look.tint[0];
    gTestSprite.tintGreen = look.tint[1];
    gTestSprite.tintBlue = look.tint[2];
    gTestSprite.tintMode = 0;
    gTestSprite.tintColor = 0;
    SpriteRec *saved = gSWCurrentSpriteBeingDrawn;
    gSWCurrentSpriteBeingDrawn = &gTestSprite;
    p.Sprite(plain ? BlitPixieRLEDrawProc : BlitPixieRLETranslucentDrawProc, sprite, src, where);
    gSWCurrentSpriteBeingDrawn = saved;
}

/* the pictures and patterns the page draws */
struct TestArt {
    PicHandle big;                      /* palette pictures: stretched to the nearest pixel */
    PicHandle arrow;
    PicHandle deep;                     /* a direct-colour picture: stretched with the bicubic kernel */
    Image_t *icon;
    Image_t *pattern;
    SWFrameRec *ship;
    SWFrameRec *rock;
};

void DrawArt(UI::Painter &p, const Sources &sources, const TestArt &art)
{
    PicHandle big = art.big;
    PicHandle arrow = art.arrow;
    Image_t *icon = art.icon;
    Image_t *pattern = art.pattern;
    SWFrameRec *ship = art.ship;
    SWFrameRec *rock = art.rock;
    Rect r;
    Rect piece;

    Mark("picture", kSame, 8, 232, 168, 332);
    SetRect(&r, 8, 232, 168, 332);
    if (big != NULL)
        p.Picture(big, r);

    Mark("picture own size", kSame, 176, 232, 196, 248);
    if (arrow != NULL) {
        QDGetPictureBounds(arrow, &r);
        OffsetRect(&r, 176, 232);
        p.Picture(arrow, r);
    }
    Mark("picture stretched", kSame, 200, 232, 240, 262);
    SetRect(&r, 200, 232, 233, 259);
    if (arrow != NULL)
        p.Picture(arrow, r);

    Mark("icon", kSame, 244, 232, 284, 272);
    if (icon != NULL) {
        QDGetCIconBounds(icon, &r);
        OffsetRect(&r, 244, 232);
        p.Icon(icon, r);
    }

    Mark("pattern", kSameAtOne, 290, 232, 440, 282);
    SetRect(&r, 290, 232, 440, 282);
    if (pattern != NULL)
        p.Pattern(r, pattern);

    /* pieces put together: a picture, a masked piece beside it, a stretched picture, over a fill */
    Mark("composition", kSameAtOne, 176, 266, 286, 332);
    SetRect(&r, 176, 266, 286, 332);
    p.Fill(r, Colour(0x3000, 0x3000, 0x6000));
    {
        UI::Painter::Composition parts(p, r);
        SetRect(&piece, 0, 0, 60, 60);
        SetRect(&r, 180, 268, 240, 328);
        p.CopyMasked(sources.piece.Surface(), sources.mask.Surface(), piece, piece, r);
        SetRect(&piece, 60, 0, 100, 60);
        SetRect(&r, 240, 268, 280, 328);
        p.CopyMasked(sources.piece.Surface(), sources.mask.Surface(), piece, piece, r);
        if (arrow != NULL) {
            QDGetPictureBounds(arrow, &r);
            OffsetRect(&r, 200, 290);
            p.Picture(arrow, r);
        }
    }

    Mark("picture direct colour", kSame, 290, 286, 440, 332);
    SetRect(&r, 290, 286, 440, 332);
    if (art.deep != NULL)
        p.Picture(art.deep, r);

    Mark("sprite", kSameAtOne, 446, 232, 540, 282);
    SetRect(&r, 446, 232, 636, 332);
    p.Fill(r, Colour(0x2000, 0x4000, 0x6000));
    DrawSprite(p, ship, 450, 236, kPlain);
    DrawSprite(p, rock, 500, 240, kPlain);
    Mark("sprite glow", kDeeper, 540, 232, 636, 282);
    DrawSprite(p, ship, 544, 236, kRollover);
    DrawSprite(p, rock, 594, 240, kRollover);
    Mark("sprite boosted", kLook, 446, 282, 636, 332);
    DrawSprite(p, ship, 450, 286, kBoosted);
    DrawSprite(p, rock, 544, 286, kBoosted);
}

void DrawCopies(UI::Painter &p, const UI::Surface &self, const Sources &sources)
{
    Rect area;
    Rect half;
    Rect r;

    SetRect(&area, 0, 0, 120, 60);
    Mark("copy", kSame, 8, 336, 128, 396);
    SetRect(&r, 8, 336, 128, 396);
    p.Copy(sources.gradient.Surface(), area, r);

    Mark("add over black", kSame, 136, 336, 256, 396);
    SetRect(&r, 136, 336, 256, 396);
    p.Copy(sources.gradient.Surface(), area, r, UI::Blit::AddOver);
    Mark("max over black", kSame, 264, 336, 384, 396);
    SetRect(&r, 264, 336, 384, 396);
    p.Copy(sources.gradient.Surface(), area, r, UI::Blit::AdMax);

    Mark("add over grey", kLook, 392, 336, 512, 396);
    SetRect(&r, 392, 336, 512, 396);
    p.Fill(r, Colour(0x3000, 0x3000, 0x3000));
    p.Copy(sources.gradient.Surface(), area, r, UI::Blit::AddOver);
    Mark("max over grey", kSame, 520, 336, 636, 396);
    SetRect(&r, 520, 336, 636, 396);
    p.Fill(r, Colour(0x3000, 0x3000, 0x3000));
    SetRect(&half, 0, 0, 116, 60);
    p.Copy(sources.gradient.Surface(), half, r, UI::Blit::AdMax);

    Mark("copy of pixels", kSame, 8, 402, 128, 462);
    SetRect(&r, 8, 402, 128, 462);
    p.Copy(sources.piece.Surface(), area, r);

    Mark("masked", kSameAtOne, 136, 402, 256, 462);
    SetRect(&r, 136, 402, 256, 462);
    p.Fill(r, Colour(0x2000, 0x2000, 0x2000));
    p.CopyMasked(sources.piece.Surface(), sources.mask.Surface(), area, area, r);

    Mark("copy of itself", kSame, 264, 402, 416, 442);
    SetRect(&area, 8, 4, 160, 44);
    SetRect(&r, 264, 402, 416, 442);
    p.Copy(self, area, r);

    Mark("text over art", kText, 424, 402, 636, 462);
    SetRect(&area, 0, 0, 120, 60);
    SetRect(&r, 424, 402, 544, 462);
    p.Copy(sources.gradient.Surface(), area, r);
    p.Text(430, 426, "Text over a copy", { UI::Style::Title, UI::kWhite });
    p.Text(430, 452, "and beside it, small", { UI::Style::Small, Colour(0xffff, 0xffff, 0x8000) });
}

/* edit records drawn, and scrolled in a box shorter than the text; the scroll updates the current dialog's
   window: one that is not shown */
void DrawEdits(UI::Painter &p)
{
    static const char kLong[] =
        "An edit record: its text wrapped into the box in the small style, long enough to fill more rows than "
        "the box holds, so that scrolling it up by a few rows shows other lines than the first ones. Then a "
        "third sentence, and a fourth one after it, to have something to scroll to.";
    const UI::TextSpec spec = { UI::Style::Small, UI::kWhite };
    DialogRecord hidden = {};
    DialogPtr current = gCurDialog;
    Rect box;

    Mark("edit draw", kText, 8, 468, 316, 528);
    SetRect(&box, 8, 468, 316, 528);
    TEHandle te = TENew(&box, &box);
    p.EditSetText(te, kLong, (int32_t)strlen(kLong), UI::Style::Small);
    p.EditDraw(te, box, spec, Colour(0x2000, 0x2000, 0x4000));
    TEDispose(te);

    Mark("edit scroll", kText, 324, 468, 636, 528);
    SetRect(&box, 324, 468, 636, 504);
    te = TENew(&box, &box);
    p.EditSetText(te, kLong, (int32_t)strlen(kLong), UI::Style::Small);
    gCurDialog = &hidden;
    p.EditScroll(te, -18, spec, Colour(0x1000, 0x3000, 0x1000));
    gCurDialog = current;
    TEDispose(te);
}

/* a button's face as the game puts it together, a stretched copy between targets, Painters nested */
void DrawPieces(UI::Painter &p, const UI::Surface &self, const Sources &sources)
{
    Rect area;
    Rect band;
    Rect piece;
    Rect r;

    /* masked ends at their own size, the middle stretched between them, stretched onto the port as one */
    Mark("button face", kSameAtOne, 8, 532, 248, 592);
    SetRect(&r, 8, 532, 248, 592);
    p.Fill(r, Colour(0x3000, 0x3000, 0x6000));
    {
        UI::Painter::Composition face(p, r);
        UI::Painter::Composition again(p, r);       /* one inside another: the outer one's still */
        SetRect(&piece, 0, 0, 30, 60);
        SetRect(&r, 8, 532, 38, 592);
        p.CopyMasked(sources.piece.Surface(), sources.mask.Surface(), piece, piece, r);
        SetRect(&piece, 90, 0, 120, 60);
        SetRect(&r, 218, 532, 248, 592);
        p.CopyMasked(sources.piece.Surface(), sources.mask.Surface(), piece, piece, r);
        SetRect(&piece, 30, 0, 60, 60);
        SetRect(&r, 38, 532, 218, 592);
        p.Copy(sources.piece.Surface(), piece, r);
    }

    Mark("copy stretched", kFiltered, 256, 532, 376, 592);
    SetRect(&area, 40, 10, 80, 50);
    SetRect(&r, 256, 532, 376, 592);
    p.Copy(sources.gradient.Surface(), area, r);

    Mark("nested painters", kSame, 384, 532, 636, 592);
    SetRect(&area, 0, 0, 120, 60);
    {
        UI::Painter inner(sources.gradient.Surface());      /* the source drawn into while the page's draws wait */
        SetRect(&band, 0, 0, 60, 30);
        inner.Fill(band, Colour(0, 0xffff, 0));
    }
    SetRect(&r, 384, 532, 504, 592);
    p.Copy(sources.gradient.Surface(), area, r);
    {
        UI::Painter inner(sources.gradient.Surface());      /* and after the copy: that shows it as it was */
        SetRect(&band, 60, 30, 120, 60);
        inner.Fill(band, Colour(0xffff, 0, 0xffff));
    }
    SetRect(&r, 510, 532, 630, 592);
    p.Copy(sources.gradient.Surface(), area, r);
    {
        UI::Painter same(self);                             /* a second Painter on the page: one order */
        SetRect(&band, 390, 540, 450, 560);
        same.Fill(band, UI::kWhite);
    }
    SetRect(&band, 400, 545, 440, 555);
    p.Fill(band, Colour(0xffff, 0, 0));
}

/* the game's drawing on its GPU-target ports: status bar parts, an icon cell, a button, the map's nebulae */
void DrawTargets(UI::Painter &p, const UI::Surface &self, const Sources &sources, const TestArt &art)
{
    Rect area;
    Rect piece;
    Rect box;
    Rect r;

    Mark("status part", kSame, 8, 600, 128, 660);
    SetRect(&area, 0, 0, 120, 60);
    SetRect(&r, 8, 600, 128, 660);
    p.Copy(sources.status.Surface(), area, r);

    Mark("status text", kText, 136, 600, 256, 660);
    SetRect(&area, 30, 0, 90, 60);
    SetRect(&r, 136, 600, 196, 660);
    p.Copy(sources.status.Surface(), area, r);
    SetRect(&r, 196, 600, 256, 660);
    p.Fill(r, UI::kBlack);
    SetRect(&r, 140, 640, 230, 646);
    p.Fill(r, Colour(0, 0x8000, 0xffff));
    p.Frame(r, Colour(0x8000, 0xc000, 0xffff));
    p.Text(140, 614, "Shields", { UI::Style::Status, Colour(0x8000, 0xffff, 0x8000) });
    p.Text(140, 632, "Nav 88%", { UI::Style::StatusLarge, UI::kWhite });

    Mark("icon cell", kSameAtOne, 264, 600, 328, 664);
    SetRect(&piece, 32, 0, 64, 32);
    {
        UI::Painter icons(sources.icons.Surface());
        icons.Fill(piece, UI::kBlack);
        if (art.big != NULL)
            icons.Picture(art.big, piece);
    }
    SetRect(&r, 264, 600, 296, 632);
    p.Copy(sources.icons.Surface(), piece, r);
    SetRect(&r, 296, 632, 328, 664);
    p.Copy(sources.icons.Surface(), piece, r);

    Mark("button on its port", kSameAtOne, 336, 600, 456, 640);
    SetRect(&r, 336, 600, 456, 640);
    p.Fill(r, Colour(0x3000, 0x2000, 0x1000));
    SetRect(&box, 0, 0, 120, 40);
    {
        UI::Painter face(sources.button.Surface());
        face.Copy(self, r, box);
        {
            UI::Painter::Composition parts(face, box);
            SetRect(&piece, 0, 10, 30, 50);
            SetRect(&area, 0, 0, 30, 40);
            face.CopyMasked(sources.piece.Surface(), sources.mask.Surface(), piece, piece, area);
            SetRect(&piece, 90, 10, 120, 50);
            SetRect(&area, 90, 0, 120, 40);
            face.CopyMasked(sources.piece.Surface(), sources.mask.Surface(), piece, piece, area);
            SetRect(&piece, 30, 10, 60, 50);
            SetRect(&area, 30, 0, 90, 40);
            face.Copy(sources.piece.Surface(), piece, area);
        }
        face.Line(50, 20, 70, 20, UI::kWhite, 2);
        face.Line(60, 10, 60, 30, UI::kWhite, 2);
    }
    p.Copy(sources.button.Surface(), box, r);

    Mark("max over borders", kSame, 464, 600, 584, 660);
    const RGBColor borders[] = { Colour(0x8000, 0, 0), Colour(0, 0x6000, 0x1000), Colour(0x2000, 0x2000, 0xa000),
                                 Colour(0x7000, 0x7000, 0) };
    for (short i = 0; i < 4; i++) {
        SetRect(&r, 464 + i * 30, 600, 494 + i * 30, 630);
        p.Fill(r, borders[i]);
        SetRect(&r, 464 + i * 30, 630, 494 + i * 30, 660);
        p.Fill(r, borders[3 - i]);
    }
    SetRect(&area, 0, 0, 120, 60);
    SetRect(&r, 464, 600, 584, 660);
    p.Copy(sources.gradient.Surface(), area, r, UI::Blit::AdMax);
}

void DrawPage(const TestPort &page, const Sources &sources, const TestArt &art)
{
    Rect all;

    DrawSources(sources);
    UI::Painter p(page.Surface());
    SetRect(&all, 0, 0, kPageWidth, kPageHeight);
    p.Fill(all, UI::kBlack);
    DrawShapes(p);
    DrawStyles(p);
    DrawTexts(p);
    DrawArt(p, sources, art);
    DrawCopies(p, page.Surface(), sources);
    DrawEdits(p);
    DrawPieces(p, page.Surface(), sources);
    DrawTargets(p, page.Surface(), sources, art);
}

int Expand5(int v)
{
    return v << 3 | v >> 2;
}

/* an item's device pixels on both pages: how many differ at 5 bits, and how far in 8-bit levels */
void Compare(FILE *log, double scale, const char *pass, const Item &item, const PixMap &cpu,
             const std::vector<uint32_t> &gpu, int *bad)
{
    Rect d = UI::Mapping(cpu).DeviceRect(item.r);
    int count = 0;
    int differ = 0;
    int largest = 0;
    int over8 = 0;
    int over16 = 0;

    for (int y = d.top; y < d.bottom && y < cpu.height; y++) {
        const unsigned short *row = (const unsigned short *)((const char *)cpu.baseAddr + (size_t)y * cpu.rowBytes);
        for (int x = d.left; x < d.right && x < cpu.width; x++) {
            uint32_t g = gpu[(size_t)y * cpu.width + x];
            bool same = true;
            int worst = 0;
            for (int shift = 0; shift < 3; shift++) {
                int c5 = row[x] >> (10 - 5 * shift) & 0x1f;
                int c8 = (int)(g >> (16 - 8 * shift) & 0xff);
                int delta = abs(c8 - Expand5(c5));
                same = same && (c8 >> 3) == c5;
                if (delta > worst)
                    worst = delta;
            }
            count++;
            differ += same ? 0 : 1;
            over8 += worst > 8 ? 1 : 0;
            over16 += worst > 16 ? 1 : 0;
            if (worst > largest)
                largest = worst;
        }
    }
    bool must = item.kind == kSame || (item.kind == kSameAtOne && scale == 1.0);
    bool off = must && differ != 0;
    if (off)
        (*bad)++;
    if (log != NULL)
        fprintf(log,
                "scale %-5g%s %c %-29s %6d px: %5d differ at 5 bits, largest %3d of 255, %5d over 8, %5d over 16%s\n",
                scale, pass, item.kind, item.name, count, differ, largest, over8, over16, off ? "  MISMATCH" : "");
}

void WriteRaw(const char *kind, double scale, int width, int height, const void *pixels, size_t pitch, size_t bytes)
{
    char name[64];
    int32_t size[2] = { width, height };

    snprintf(name, sizeof(name), "gputest_%g_%s.raw", scale, kind);
    FILE *file = PlatFileOpen(name, "wb");
    if (file == NULL)
        return;
    fwrite(size, sizeof(size), 1, file);
    for (int y = 0; y < height; y++)
        fwrite((const char *)pixels + (size_t)y * pitch, bytes, width, file);
    fclose(file);
}

/* a raw file of the reference, width x height pixels of `bytes'; false = none of that size */
bool ReadRaw(const char *kind, double scale, bool ofRenderer, int width, int height, size_t bytes,
             std::vector<unsigned char> &out)
{
    char name[kPlatPathMax];
    int32_t size[2] = { 0, 0 };

    if (ofRenderer)
        snprintf(name, sizeof(name), "%s/%s/gputest_%g_%s.raw", refFolder, PlatRendererName(), scale, kind);
    else
        snprintf(name, sizeof(name), "%s/gputest_%g_%s.raw", refFolder, scale, kind);
    FILE *file = PlatFileOpen(name, "rb");
    if (file == NULL)
        return false;
    out.resize((size_t)width * height * bytes);
    bool got = fread(size, sizeof(size), 1, file) == 1 && size[0] == width && size[1] == height
               && fread(out.data(), 1, out.size(), file) == out.size();
    fclose(file);
    return got;
}

/* the reference's CPU drawing of a port's size, as that port's pixel map over it */
bool ReadCpu(const char *kind, double scale, const PixMap &like, PixMap &cpu, std::vector<unsigned char> &pixels)
{
    if (!ReadRaw(kind, scale, false, like.width, like.height, 2, pixels))
        return false;
    cpu = like;
    cpu.pixelSize = 16;
    cpu.baseAddr = pixels.data();
    cpu.rowBytes = like.width * 2;
    return true;
}

/* an item's device pixels against the reference's GPU drawing: equal to the byte */
void CompareGpu(FILE *log, double scale, const char *pass, const Item &item, const PixMap &like,
                const std::vector<uint32_t> &gpu, const std::vector<unsigned char> &ref, int *bad)
{
    Rect d = UI::Mapping(like).DeviceRect(item.r);
    const uint32_t *was = (const uint32_t *)ref.data();
    int differ = 0;

    for (int y = std::max((int)d.top, 0); y < d.bottom && y < like.height; y++)
        for (int x = std::max((int)d.left, 0); x < d.right && x < like.width; x++)
            differ += gpu[(size_t)y * like.width + x] != was[(size_t)y * like.width + x] ? 1 : 0;
    if (differ != 0)
        (*bad)++;
    if (log != NULL && differ != 0)
        fprintf(log, "scale %-5g%s %-31s %6d px differ from the reference GPU drawing  MISMATCH\n", scale, pass,
                item.name, differ);
}

/* the GPU's drawing of `kind' against both of the reference's; written to the game folder, as a new one's */
void CompareAll(FILE *log, double scale, const char *pass, const char *kind, const Item *list, size_t count,
                const PixMap &like, const std::vector<uint32_t> &gpu, int *bad)
{
    std::vector<unsigned char> cpuPixels;
    std::vector<unsigned char> gpuRef;
    std::string gpuKind = std::string(kind) + "gpu";
    std::string cpuKind = std::string(kind) + "cpu";
    PixMap cpu;

    if (!ReadCpu(cpuKind.c_str(), scale, like, cpu, cpuPixels)) {
        (*bad)++;
        if (log != NULL)
            fprintf(log, "scale %g%s: no reference CPU drawing %s  MISMATCH\n", scale, pass, cpuKind.c_str());
    } else {
        for (size_t i = 0; i < count; i++)
            Compare(log, scale, pass, list[i], cpu, gpu, bad);
    }
    if (!ReadRaw(gpuKind.c_str(), scale, true, like.width, like.height, 4, gpuRef)) {
        (*bad)++;
        if (log != NULL)
            fprintf(log, "scale %g%s: no reference GPU drawing %s for %s  MISMATCH\n", scale, pass, gpuKind.c_str(),
                    PlatRendererName());
    } else {
        for (size_t i = 0; i < count; i++)
            CompareGpu(log, scale, pass, list[i], like, gpu, gpuRef, bad);
    }
    WriteRaw(gpuKind.c_str(), scale, like.width, like.height, gpu.data(), (size_t)like.width * 4, 4);
}

bool MakeSources(Sources &sources, double scale, bool gpu)
{
    return sources.gradient.Make(120, 60, scale, gpu) && sources.piece.Make(120, 60, 1.0, false)
           && sources.mask.Make(120, 60, 1.0, false) && sources.status.Make(120, 60, scale, gpu)
           && sources.button.Make(120, 40, scale, gpu) && sources.icons.Make(128, 32, 1.0, false);
}

void DropSources(Sources &sources)
{
    for (TestPort *each : { &sources.gradient, &sources.piece, &sources.mask, &sources.status, &sources.button,
                            &sources.icons })
        each->Drop();
}

/* one scale's page on a GPU target vs. the reference; under a brightness too, which targets do not take in */
void RunScale(FILE *log, double scale, double brightness, const TestArt &art, int *bad)
{
    TestPort gpu;
    Sources sources;
    bool bright = brightness != 1.0;

    if (bright)
        PlatVideoSetBrightness(brightness);
    if (gpu.Make(kPageWidth, kPageHeight, scale, true) && MakeSources(sources, scale, true)) {
        const PixMap &pix = gpu.frame->framePort->pix;
        std::vector<uint32_t> read((size_t)pix.width * pix.height);
        bool got;

        recording = items.empty();
        DrawPage(gpu, sources, art);
        recording = false;
        got = gpu.Read(read.data());
        if (got)
            CompareAll(log, scale, bright ? " bright" : "", "", items.data(), items.size(), pix, read, bad);
        else if (log != NULL)
            fprintf(log, "scale %g: the target could not be read\n", scale);
    } else if (log != NULL) {
        fprintf(log, "scale %g: no pages\n", scale);
    }
    DropSources(sources);
    gpu.Drop();
    if (bright)
        ApplyBrightness();
}

/* Stretched copies between targets, and from a target onto itself: nothing from beside the part shows, a part
   of one pixel is that pixel, and an axis that is not stretched stays as sharp as it is. */
void CheckCopies(FILE *log, int *bad)
{
    TestPort from;
    TestPort to;
    const RGBColor red = Colour(0xffff, 0, 0);
    const RGBColor blue = Colour(0, 0, 0xffff);
    const uint32_t kRed = 0xffff0000u;
    const uint32_t kBlue = 0xff0000ffu;
    int failed = 0;
    int checks = 0;
    Rect r;
    Rect part;

    if (!from.Make(16, 16, 1.0, true) || !to.Make(64, 64, 1.0, true)) {
        if (log != NULL)
            fprintf(log, "copies: no ports\n");
        (*bad)++;
        from.Drop();
        to.Drop();
        return;
    }
    {
        UI::Painter p(from.Surface());
        SetRect(&r, 0, 0, 16, 16);
        p.Fill(r, Colour(0, 0xffff, 0));
        SetRect(&r, 4, 4, 8, 12);
        p.Fill(r, red);
        SetRect(&r, 8, 4, 12, 12);
        p.Fill(r, blue);
    }
    {
        UI::Painter p(to.Surface());
        SetRect(&r, 0, 0, 64, 64);
        p.Fill(r, UI::kBlack);
        SetRect(&part, 4, 4, 12, 12);
        SetRect(&r, 0, 0, 40, 40);
        p.Copy(from.Surface(), part, r);                /* stretched both ways */
        SetRect(&r, 0, 44, 8, 60);
        p.Copy(from.Surface(), part, r);                /* stretched down only */
        SetRect(&part, 5, 5, 6, 6);
        SetRect(&r, 44, 0, 60, 16);
        p.Copy(from.Surface(), part, r);                /* one pixel */
        SetRect(&part, 38, 0, 40, 2);
        SetRect(&r, 44, 44, 60, 60);
        p.Copy(to.Surface(), part, r);                  /* a part of itself, black beside it */
    }
    std::vector<uint32_t> read(64 * 64);
    auto expect = [&](const char *what, int x, int y, uint32_t want) {
        uint32_t got = read[(size_t)y * 64 + x];
        checks++;
        if (got == want)
            return;
        failed++;
        if (log != NULL)
            fprintf(log, "copies: %s at %d,%d is %08x, not %08x\n", what, x, y, got, want);
    };
    if (to.Read(read.data())) {
        for (int i = 0; i < 40; i += 13) {
            expect("a stretched part's left edge", 0, i, kRed);
            expect("a stretched part's right edge", 39, i, kBlue);
            expect("a stretched part's top edge, left", 5 + i / 4, 0, kRed);
            expect("a stretched part's bottom edge, right", 39 - i / 4, 39, kBlue);
        }
        for (int y = 44; y < 60; y += 5) {
            expect("a part stretched down only, left half", 3, y, kRed);
            expect("a part stretched down only, right half", 4, y, kBlue);
        }
        for (int i = 0; i < 16; i += 5) {
            expect("a part of one pixel", 44 + i, i, kRed);
            expect("a part of one pixel", 59 - i, i, kRed);
            expect("a stretched part of the target itself", 44 + i, 44 + i, kBlue);
            expect("a stretched part of the target itself", 59 - i, 44 + i, kBlue);
        }
    } else {
        failed++;
    }
    if (log != NULL)
        fprintf(log, "copies: %d checks, %d failed\n", checks, failed);
    *bad += failed;
    from.Drop();
    to.Drop();
}

/* -- a dialog: its controls from its pixels under what its update proc draws, edit records and a list -- */

constexpr short kDialogWidth = 420;
constexpr short kDialogHeight = 260;
constexpr short kEditItem = 5;
constexpr short kListRows = 8;
constexpr short kListRowHeight = 18;

const DialogItemSpec kDialogSpecs[] = {
    { kDitlButton, 8, 10, 30, 130, "Button", 0 },
    { kDitlCheckBox, 40, 10, 58, 130, "Checked", 0 },
    { kDitlCheckBox, 62, 10, 80, 130, "Not checked", 0 },
    { kDitlStaticText, 86, 10, 130, 130, "Static text of the dialog, wrapped into its box.", 0 },
    { kDitlEditText, 138, 10, 158, 130, "Edit field", 0 },
    { kDitlPicture, 166, 10, 182, 30, NULL, kPictArrow },
};

/* the update proc's fill lies over the button's right part; the controls' other pixels show as they are */
const Item kDialogItems[] = {
    { "dialog controls", kSame, { 30, 0, 222, 138 } },
    { "dialog button, its part shown", kSame, { 0, 0, 30, 70 } },
    { "dialog fill over a control", kSame, { 8, 70, 30, 140 } },
    { "dialog update text", kText, { 8, 140, 30, 412 } },
    { "dialog edit", kText, { 40, 140, 100, 412 } },
    { "dialog edit scrolled", kText, { 104, 140, 140, 412 } },
    { "dialog list", kText, { 146, 140, 218, 397 } },
    { "dialog list scroll bar", kSame, { 146, 397, 218, 412 } },
    { "dialog frame", kSame, { 222, 0, 260, 140 } },
    { "dialog from the load page", kText, { 222, 140, 258, 412 } },
    { "dialog frame, below", kSame, { 258, 140, 260, 412 } },
    { "dialog frame, right", kSame, { 222, 412, 260, 420 } },
};

struct TestDialog {
    DialogPtr dlg = NULL;
    TestPort load;                      /* its load page: drawn on, then copied onto the dialog */
    TEHandle edit = NULL;
    TEHandle scrolled = NULL;
    ScrollRec **list = NULL;
};

TestDialog *gDrawnDialog;               /* the one whose update proc runs */

void TestListRow(short message, Boolean selected, Rect *box, Point *cell, short offset, short length,
                 ScrollRec **list)
{
    UI::Painter p(UI::Painter::ListSurface());

    p.Fill(*box, selected ? Colour(0x8000, 0x2000, 0x2000) : Colour(0x1000, 0x1000, 0x3000));
    p.Text(box->left + 4, box->top + 13, std::string(*(*list)->cells + offset, length),
           { UI::Style::Small, UI::kWhite });
}

void TestDialogUpdate(void)
{
    static const char kLong[] =
        "An edit record on a dialog: its text wrapped into the box in the small style, with enough words to "
        "fill more rows than the box holds, so that the scrolled one starts at another line than this one.";
    const UI::TextSpec white = { UI::Style::Small, UI::kWhite };
    const TestDialog &test = *gDrawnDialog;
    UI::Painter p(UI::Surface::Dialog(test.dlg));
    Point cell;
    Rect r;

    SetRect(&r, 70, 8, 412, 30);
    p.Fill(r, Colour(0x2000, 0x2000, 0x6000));
    p.Text(150, 24, "Drawn by the update proc", { UI::Style::System, UI::kWhite });
    p.EditSetText(test.edit, kLong, (int32_t)strlen(kLong), UI::Style::Small);
    p.EditDraw(test.edit, (*test.edit)->viewRect, white, Colour(0x2000, 0x2000, 0x4000));
    p.EditSetText(test.scrolled, kLong, (int32_t)strlen(kLong), UI::Style::Small);
    (*test.scrolled)->destRect = (*test.scrolled)->viewRect;
    p.EditScroll(test.scrolled, -14, white, Colour(0x1000, 0x3000, 0x1000));
    p.List(test.list, UI::Style::Small);
    cell.h = 0;
    cell.v = 2;
    p.ListSelect(test.list, true, cell, UI::Style::Small);

    SetRect(&r, 140, 222, 412, 258);
    {
        UI::Painter load(test.load.Surface());
        Rect band;
        load.Fill(r, Colour(0x1000, 0x1000, 0x2000));
        for (short x = 140; x < 412; x += 34) {
            SetRect(&band, x, 222, x + 17, 228);
            load.Fill(band, Colour(0x4000, 0x2000 + x * 60, 0x6000));
        }
        load.Line(140, 256, 411, 230, Colour(0xffff, 0xc000, 0));
        load.Text(146, 248, "Drawn on the load page", { UI::Style::Small, UI::kWhite });
    }
    p.Copy(test.load.Surface(), r, r);
}

/* closed without the redraw of what lies under it: the menu is not up yet */
void DropTestDialog(TestDialog &test)
{
    Boolean paused = gPause;
    SpriteWorldPtr world = gSpriteWorld;

    if (test.dlg != NULL) {
        gPause = 0;
        gSpriteWorld = NULL;
        DisposeDialog(test.dlg);
        gPause = paused;
        gSpriteWorld = world;
    }
    if (test.edit != NULL)
        TEDispose(test.edit);
    if (test.scrolled != NULL)
        TEDispose(test.scrolled);
    if (test.list != NULL)
        LDispose(test.list);
    test.load.Drop();
    test = TestDialog();
}

/* the dialog with its port at the scale, drawn through its GPU canvas */
bool MakeTestDialog(TestDialog &test, double scale)
{
    Handle item;
    Rect box;
    Rect rows;
    Point cellSize;
    Point cell;

    test.dlg = NewDialogFromItems(kDialogWidth, kDialogHeight, kDialogSpecs,
                                  (short)(sizeof(kDialogSpecs) / sizeof(kDialogSpecs[0])), TestDialogUpdate, NULL);
    if (test.dlg == NULL || !test.load.Make(kDialogWidth, kDialogHeight, scale, true)) {
        DropTestDialog(test);
        return false;
    }
    CGrafPtr port = GetDialogPort(test.dlg);
    UI::ScalePortPixels(port, scale);
    if (!UI::Canvas::MakeTarget(port)) {
        DropTestDialog(test);
        return false;
    }
    GetDialogItem(test.dlg, 2, NULL, &item, NULL);
    SetControlValue((ControlPtr)item, 1);
    SelectDialogItemText(test.dlg, kEditItem, 0, 4);
    SetRect(&box, 140, 40, 412, 100);
    test.edit = TENew(&box, &box);
    SetRect(&box, 140, 104, 412, 140);
    test.scrolled = TENew(&box, &box);
    SetRect(&box, 140, 146, 397, 146 + 4 * kListRowHeight);
    SetRect(&rows, 0, 0, 1, kListRows);
    cellSize.h = box.right - box.left;
    cellSize.v = kListRowHeight;
    test.list = nv_NewDialog(&box, &rows, cellSize, TestListRow, (CGrafPtr)test.dlg, 1);
    cell.h = 0;
    for (cell.v = 0; cell.v < kListRows; cell.v++) {
        std::string row = "List row " + std::to_string(cell.v + 1) + ", drawn by its draw proc";
        LSetCell((void *)row.data(), (int32_t)row.size(), cell, test.list);
    }
    LSetDrawingMode(1, test.list);
    LScroll(0, 1, test.list);
    return true;
}

void DrawTestDialog(TestDialog &test)
{
    gDrawnDialog = &test;
    DrawDialog(test.dlg);
    gDrawnDialog = NULL;
}

/* one scale's dialog canvas vs. the reference: opaque, and at the screen's scale the same as the screen */
void RunDialog(FILE *log, double scale, int *bad)
{
    TestDialog test;

    if (!MakeTestDialog(test, scale)) {
        (*bad)++;
        if (log != NULL)
            fprintf(log, "scale %g dialog: no dialog on a canvas\n", scale);
        return;
    }
    CGrafPtr port = GetDialogPort(test.dlg);
    const PixMap pix = port->pix;
    std::vector<uint32_t> read((size_t)pix.width * pix.height);
    std::vector<uint32_t> shown;
    Rect place = UI::Mapping(gGamePort.pix).DeviceRect(test.dlg->bounds);
    PlatRect onScreen = { place.left, place.top, place.right - place.left, place.bottom - place.top };
    bool sameScale = onScreen.w == pix.width && onScreen.h == pix.height;
    int clear = 0;
    int moved = 0;

    DrawTestDialog(test);
    UI::Canvas::FlushAll();
    bool got = PlatTargetRead(port->target, NULL, read.data()) != 0;
    if (got && sameScale) {
        shown.resize(read.size());
        got = PlatTargetRead(gGamePort.target, &onScreen, shown.data()) != 0;
    }
    DropTestDialog(test);
    if (!got) {
        (*bad)++;
        if (log != NULL)
            fprintf(log, "scale %g dialog: the canvas could not be read\n", scale);
        return;
    }
    /* a dialog's pixels at a scale depend on the screen's: its size names it */
    std::string kind = "dialog_" + std::to_string(pix.width) + "x" + std::to_string(pix.height) + "_";
    CompareAll(log, scale, " dialog", kind.c_str(), kDialogItems, sizeof(kDialogItems) / sizeof(kDialogItems[0]),
               pix, read, bad);
    for (size_t i = 0; i < read.size(); i++) {
        clear += read[i] >> 24 != 0xff ? 1 : 0;
        moved += sameScale && shown[i] != read[i] ? 1 : 0;
    }
    *bad += (clear != 0 ? 1 : 0) + (moved != 0 ? 1 : 0);
    if (log != NULL) {
        fprintf(log, "scale %-5g dialog: %d of %d x %d pixels of its canvas are not opaque%s\n", scale, clear,
                pix.width, pix.height, clear != 0 ? "  MISMATCH" : "");
        if (sameScale)
            fprintf(log, "scale %-5g dialog: %d pixels differ between its canvas and the screen%s\n", scale, moved,
                    moved != 0 ? "  MISMATCH" : "");
    }
}

/* an overlay frame as the game draws it (draw; NULL: already drawn) vs. the reference; black again after, as
   it is made */
void RunOverlay(FILE *log, const char *name, SWFrameRec *frame, void (*draw)(void), int *bad)
{
    if (frame == NULL || frame->framePort->target == NULL) {
        (*bad)++;
        if (log != NULL)
            fprintf(log, "overlay %s: no frame with a target\n", name);
        return;
    }
    CGrafPtr port = frame->framePort;
    const PixMap &pix = port->pix;
    std::vector<uint32_t> read((size_t)pix.width * pix.height);
    Rect border = frame->frameRect;

    if (draw != NULL)
        draw();
    UI::Canvas::FlushAll();
    bool got = PlatTargetRead(port->target, NULL, read.data()) != 0;
    UI::Painter(UI::Surface(frame)).Fill(frame->frameRect, UI::kBlack);
    if (!got) {
        (*bad)++;
        if (log != NULL)
            fprintf(log, "overlay %s: the target could not be read\n", name);
        return;
    }
    border.bottom = border.top + 1;
    const Item checks[] = { { name, kText, frame->frameRect }, { "  its top row", kSame, border } };
    std::string kind = std::string("overlay_") + name + "_";
    CompareAll(log, UI::Mapping(pix).ScaleX(), " overlay", kind.c_str(), checks, 2, pix, read, bad);
}

/* the comm line, escort menu and floating map, each on its target; the game's state put back */
void RunOverlays(FILE *log, int *bad)
{
    short count = commCount;
    UI::TextSpec spec = CommStyle();
    Str255 msg;

    memcpy(msg, commMsg, sizeof msg);
    DisplayComm(std::string("The comm line of the GPU test: a message long enough to be wrapped onto the second of "
                            "its two lines in the comm style, as a long hail or a mission's message would be, and "
                            "then some more words to be sure of it."), 30);
    RunOverlay(log, "comm", CommOverlay(), NULL, bad);
    DisplayCommStyled(msg, count, spec);
    ResetCommRect();
    RunOverlay(log, "escorts", floatingEscortMenuFrame, DrawEscortMenuInFrame, bad);
    RunOverlay(log, "map", floatingMapFrame, [] {
        MapScroll scroll = mapScroll;
        DrawMapInFloatingFrame();
        mapScroll = scroll;
    }, bad);
}

} // namespace

void DebugGpuTest(void)
{
    char asked[8];

    if (PlatEnv("EVPLUS_GPUTEST", asked, sizeof asked) == 0)
        return;
    if (PlatEnv("EVPLUS_GPUTEST_REF", refFolder, sizeof refFolder) == 0)
        snprintf(refFolder, sizeof refFolder, "gputest-ref");
    FILE *log = PlatFileOpen("gputest.txt", "wb");
    TestArt art = { GetPicture(kPictBig), GetPicture(kPictArrow), NULL, GetCIcon(kIconMapArrow),
                    GetPixPat(kPatternStatic), SpriteFrame(masterShipSprite[0], 9), SpriteFrame(masterStarSprite, 0) };
    short deepID = 0;
    int bad = 0;

    /* the first direct-colour picture that is not large */
    for (short id = kFirstPict; id < kLastPict && art.deep == NULL; id++) {
        PicHandle pic = GetPicture(id);
        if (pic != NULL && pic->DataType > 8 && pic->Width <= kDeepPictMax && pic->Height <= kDeepPictMax) {
            art.deep = pic;
            deepID = id;
        } else if (pic != NULL) {
            QDReleaseResource(pic);
        }
    }
    if (log != NULL)
        fprintf(log, "picture %d: %d bits; picture %d: %d bits; picture %d: %d bits, %d x %d; ship %s, star %s\n",
                kPictBig, art.big != NULL ? art.big->DataType : 0, kPictArrow,
                art.arrow != NULL ? art.arrow->DataType : 0, deepID, art.deep != NULL ? art.deep->DataType : 0,
                art.deep != NULL ? (int)art.deep->Width : 0, art.deep != NULL ? (int)art.deep->Height : 0,
                art.ship != NULL ? "found" : "none", art.rock != NULL ? "found" : "none");
    for (double scale : kTestScales)
        RunScale(log, scale, 1.0, art, &bad);
    RunScale(log, 1.0, kTestBrightness, art, &bad);
    CheckCopies(log, &bad);
    for (double scale : kTestScales)
        RunDialog(log, scale, &bad);
    RunOverlays(log, &bad);
    if (log != NULL) {
        fprintf(log, "items that must match and do not: %d\n", bad);
        fclose(log);
    }
    for (PicHandle pic : { art.big, art.arrow, art.deep })
        if (pic != NULL)
            QDReleaseResource(pic);
    if (art.icon != NULL)
        DisposeCIcon(art.icon);
    if (art.pattern != NULL)
        DisposePixPat(art.pattern);
}
