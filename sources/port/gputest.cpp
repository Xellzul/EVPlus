/* gputest.cpp -- EVPLUS_GPUTEST: every Painter drawing, and a dialog, on pixels and on a GPU target, compared
   (gputest.txt) */

#include <math.h>
#include <string>
#include <vector>
#include "canvas.h"
#include "dialogs.h"
#include "game.h"
#include "painter.h"
#include "pixel.h"
#include "platform.h"
#include "render.h"
#include "spriteworld.h"
#include "uiscale.h"

namespace {

constexpr short kPageWidth = 640;
constexpr short kPageHeight = 600;
constexpr double kTestBrightness = 0.8;     /* the exponent of the page's second drawing at scale 1 */
constexpr int kBrightSlack = 12;            /* levels of 255 an item that must match may be off under it */
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

/* what the page copies from: a gradient at the page's scale, and a piece with its mask at their own size */
struct Sources {
    TestPort gradient;
    TestPort piece;
    TestPort mask;
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
        UI::Painter p(sources.mask.Surface());
        p.Fill(all, UI::kWhite);
        SetRect(&band, 10, 10, 110, 50);
        p.FillOval(band, UI::kBlack);
        SetRect(&band, 0, 0, 30, 30);
        p.Fill(band, UI::kBlack);
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
    Mark("max over grey", kLook, 520, 336, 636, 396);
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
}

int Expand5(int v)
{
    return v << 3 | v >> 2;
}

/* an item's device pixels on both pages: how many differ at 5 bits, and how far in 8-bit levels; with a slack
   an item that must match may be that many levels off (a page read back from under a brightness) */
void Compare(FILE *log, double scale, const char *pass, int slack, const Item &item, const PixMap &cpu,
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
    bool off = must && (slack > 0 ? largest > slack : differ != 0);
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

bool MakeSources(Sources &sources, double scale, bool gpu)
{
    return sources.gradient.Make(120, 60, scale, gpu) && sources.piece.Make(120, 60, 1.0, false)
           && sources.mask.Make(120, 60, 1.0, false);
}

void DropSources(Sources &sources)
{
    sources.gradient.Drop();
    sources.piece.Drop();
    sources.mask.Drop();
}

/* One scale's pages drawn and compared. With a brightness the GPU page is drawn under it: one of its pixels is
   checked as shown, and the page compared as read back with the brightness taken out again. */
void RunScale(FILE *log, double scale, double brightness, const TestArt &art, int *bad)
{
    TestPort cpu;
    TestPort gpu;
    Sources cpuSources;
    Sources gpuSources;
    bool bright = brightness != 1.0;

    if (bright)
        PlatVideoSetBrightness(brightness);
    if (cpu.Make(kPageWidth, kPageHeight, scale, false) && gpu.Make(kPageWidth, kPageHeight, scale, true)
            && MakeSources(cpuSources, scale, false) && MakeSources(gpuSources, scale, true)) {
        const PixMap &pix = cpu.frame->framePort->pix;
        std::vector<uint32_t> read((size_t)pix.width * pix.height);
        bool got;

        recording = items.empty();
        DrawPage(cpu, cpuSources, art);
        recording = false;
        DrawPage(gpu, gpuSources, art);
        got = gpu.Read(read.data());
        if (got && bright) {
            /* the third of the fills, 0x12 0x56 0xff, as it is shown */
            Rect at = UI::Mapping(pix).DeviceRect({ 20, 120, 21, 121 });
            uint32_t shown = read[(size_t)at.top * pix.width + at.left];
            int want = (int)(pow(0x56 / 255.0, brightness) * 255.0 + 0.5);
            int green = (int)(shown >> 8 & 0xff);
            if (abs(green - want) > 1)
                (*bad)++;
            if (log != NULL)
                fprintf(log, "brightness %g: a fill's green 0x56 is shown as %d, expected %d%s\n", brightness, green,
                        want, abs(green - want) > 1 ? "  MISMATCH" : "");
            got = PlatTargetReadDrawn(gpu.frame->framePort->target, NULL, read.data()) != 0;
        }
        if (got) {
            for (const Item &item : items)
                Compare(log, scale, bright ? " bright" : "", bright ? kBrightSlack : 0, item, pix, read, bad);
            if (!bright) {
                WriteRaw("cpu", scale, pix.width, pix.height, pix.baseAddr, pix.rowBytes, 2);
                WriteRaw("gpu", scale, pix.width, pix.height, read.data(), (size_t)pix.width * 4, 4);
            }
        } else if (log != NULL) {
            fprintf(log, "scale %g: the target could not be read\n", scale);
        }
    } else if (log != NULL) {
        fprintf(log, "scale %g: no pages\n", scale);
    }
    DropSources(cpuSources);
    DropSources(gpuSources);
    cpu.Drop();
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

/* Copies from GPU targets onto a port's pixels: each target read back as drawn, at its own scale, and a mask's
   black half copied through; at the default brightness and another. Looked at away from where a stretch blends. */
void CheckReadBack(FILE *log, int *bad)
{
    TestPort from;
    TestPort flat;
    TestPort mask;
    TestPort to;
    const unsigned short kRed = Pixel::Pack(31, 0, 0);
    const unsigned short kBlue = Pixel::Pack(0, 0, 31);
    int failed = 0;
    int checks = 0;
    Rect r;
    Rect part;

    if (!from.Make(16, 16, 2.0, true) || !flat.Make(16, 16, 1.0, true) || !mask.Make(16, 16, 1.0, true)
            || !to.Make(96, 32, 1.0, false)) {
        if (log != NULL)
            fprintf(log, "read back: no ports\n");
        (*bad)++;
        from.Drop();
        flat.Drop();
        mask.Drop();
        to.Drop();
        return;
    }
    for (const TestPort *each : { &from, &flat }) {
        UI::Painter p(each->Surface());
        SetRect(&r, 0, 0, 8, 16);
        p.Fill(r, Colour(0xffff, 0, 0));
        SetRect(&r, 8, 0, 16, 16);
        p.Fill(r, Colour(0, 0, 0xffff));
    }
    {
        UI::Painter p(mask.Surface());
        SetRect(&r, 0, 0, 16, 16);
        p.Fill(r, UI::kWhite);
        SetRect(&r, 0, 0, 16, 8);
        p.Fill(r, UI::kBlack);
    }
    const PixMap &pix = to.frame->framePort->pix;
    auto expect = [&](const char *what, double brightness, int x, int y, unsigned short want) {
        unsigned short got = Pixel::Row(pix, y)[x];
        checks++;
        if (got == want)
            return;
        failed++;
        if (log != NULL)
            fprintf(log, "read back, brightness %g: %s at %d,%d is %04x, not %04x\n", brightness, what, x, y, got,
                    want);
    };
    for (double brightness : { 1.0, kTestBrightness }) {
        if (brightness != 1.0)
            PlatVideoSetBrightness(brightness);
        {
            UI::Painter p(to.Surface());
            SetRect(&r, 0, 0, 96, 32);
            p.Fill(r, UI::kBlack);
            SetRect(&part, 0, 0, 16, 16);
            SetRect(&r, 0, 0, 16, 16);
            p.Copy(from.Surface(), part, r);                        /* a target at twice the scale */
            SetRect(&r, 20, 0, 52, 32);
            p.Copy(flat.Surface(), part, r);                        /* stretched */
            SetRect(&r, 60, 0, 76, 16);
            p.CopyMasked(flat.Surface(), mask.Surface(), part, part, r);
        }
        if (brightness != 1.0)
            ApplyBrightness();
        for (int y = 2; y < 14; y += 5) {
            expect("a target at twice the scale, left", brightness, 2, y, kRed);
            expect("a target at twice the scale, right", brightness, 13, y, kBlue);
        }
        for (int y = 4; y < 28; y += 7) {
            expect("a stretched target, left", brightness, 24, y, kRed);
            expect("a stretched target, right", brightness, 47, y, kBlue);
        }
        expect("through a mask, left", brightness, 62, 2, kRed);
        expect("through a mask, right", brightness, 73, 5, kBlue);
        expect("outside a mask", brightness, 62, 10, 0);
        expect("outside a mask", brightness, 73, 13, 0);
        expect("beside the copies", brightness, 18, 8, 0);
    }
    if (log != NULL)
        fprintf(log, "read back: %d checks, %d failed\n", checks, failed);
    *bad += failed;
    from.Drop();
    flat.Drop();
    mask.Drop();
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
    { "dialog frame", kSame, { 222, 0, 260, 420 } },
};

struct TestDialog {
    DialogPtr dlg = NULL;
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
}

/* the dialog with its port at the scale, drawn through a GPU canvas or, without one, on its pixels */
bool MakeTestDialog(TestDialog &test, double scale, bool gpu)
{
    Handle item;
    Rect box;
    Rect rows;
    Point cellSize;
    Point cell;

    test.dlg = NewDialogFromItems(kDialogWidth, kDialogHeight, kDialogSpecs,
                                  (short)(sizeof(kDialogSpecs) / sizeof(kDialogSpecs[0])), TestDialogUpdate, NULL);
    if (test.dlg == NULL)
        return false;
    CGrafPtr port = GetDialogPort(test.dlg);
    UI::ScalePortPixels(port, scale);
    if (!gpu)
        UI::Canvas::DropTarget(port);
    else if (!UI::Canvas::MakeTarget(port))
        return false;
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
    test = TestDialog();
}

void DrawTestDialog(TestDialog &test)
{
    gDrawnDialog = &test;
    DrawDialog(test.dlg);
    gDrawnDialog = NULL;
}

/* One scale's dialog drawn on a canvas and on pixels and compared. The canvas is opaque all over, and at the
   screen's own scale the screen holds the dialog as its canvas does. */
void RunDialog(FILE *log, double scale, int *bad)
{
    TestDialog gpu;
    TestDialog cpu;

    if (MakeTestDialog(gpu, scale, true)) {
        CGrafPtr port = GetDialogPort(gpu.dlg);
        int width = port->pix.width;
        int height = port->pix.height;
        std::vector<uint32_t> read((size_t)width * height);
        std::vector<uint32_t> shown;
        Rect place = UI::Mapping(gGamePort.pix).DeviceRect(gpu.dlg->bounds);
        PlatRect onScreen = { place.left, place.top, place.right - place.left, place.bottom - place.top };
        bool sameScale = onScreen.w == width && onScreen.h == height;
        int clear = 0;
        int moved = 0;

        DrawTestDialog(gpu);
        UI::Canvas::FlushAll();
        bool got = PlatTargetRead(port->target, NULL, read.data()) != 0;
        if (got && sameScale) {
            shown.resize(read.size());
            got = PlatTargetRead(gGamePort.target, &onScreen, shown.data()) != 0;
        }
        DropTestDialog(gpu);
        if (got && MakeTestDialog(cpu, scale, false)) {
            DrawTestDialog(cpu);
            const PixMap &pix = GetDialogPort(cpu.dlg)->pix;
            for (const Item &item : kDialogItems)
                Compare(log, scale, " dialog", 0, item, pix, read, bad);
            WriteRaw("dialog_cpu", scale, width, height, pix.baseAddr, pix.rowBytes, 2);
            WriteRaw("dialog_gpu", scale, width, height, read.data(), (size_t)width * 4, 4);
            for (size_t i = 0; i < read.size(); i++) {
                clear += read[i] >> 24 != 0xff ? 1 : 0;
                moved += sameScale && shown[i] != read[i] ? 1 : 0;
            }
            *bad += (clear != 0 ? 1 : 0) + (moved != 0 ? 1 : 0);
            if (log != NULL) {
                fprintf(log, "scale %-5g dialog: %d of %d x %d pixels of its canvas are not opaque%s\n", scale, clear,
                        width, height, clear != 0 ? "  MISMATCH" : "");
                if (sameScale)
                    fprintf(log, "scale %-5g dialog: %d pixels differ between its canvas and the screen%s\n", scale,
                            moved, moved != 0 ? "  MISMATCH" : "");
            }
        } else {
            (*bad)++;
            if (log != NULL)
                fprintf(log, "scale %g dialog: %s\n", scale,
                        got ? "no dialog on pixels" : "the canvas could not be read");
        }
    } else {
        (*bad)++;
        if (log != NULL)
            fprintf(log, "scale %g dialog: no dialog on a canvas\n", scale);
    }
    DropTestDialog(gpu);
    DropTestDialog(cpu);
}

} // namespace

void DebugGpuTest(void)
{
    char asked[8];

    if (PlatEnv("EVPLUS_GPUTEST", asked, sizeof asked) == 0)
        return;
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
    CheckReadBack(log, &bad);
    for (double scale : kTestScales)
        RunDialog(log, scale, &bad);
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
