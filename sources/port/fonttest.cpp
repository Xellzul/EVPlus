/* fonttest.cpp -- a debug page for the text code: every font the game has asked for by then, one a page, with
   the name asked for and the name of the font really drawn, a sample line in each size and style with its
   measured and drawn widths, and a word-wrapped paragraph; the names also go into the debug log. Shown
   before the main screen when kDebugFonts (mainscreen.cpp) is 1. */

#include <stdio.h>
#include <string.h>
#include "macport.h"
#include "burger.h"
#include "spriteworld.h"
#include "game.h"
#include "render.h"
#include "player.h"
#include "crashlog.h"

#define kLabelLeft      20
#define kSampleLeft     250
#define kHeaderBase     30
#define kFirstLineTop   50
#define kLineGap        10              /* between two sample lines, on top of the size */
#define kLabelSize      10
#define kHeaderSize     12
#define kRealNameSize   12              /* the size the real font name is asked at */
#define kWrapWidth      320
#define kWrapHeight     110
#define kWrapSize       12
#define kWrapGap        24
#define kTickHeight     4
#define kFooterRaise    16
#define kGrey           0x9000
#define kTickRed        0xffff
#define kKeyEscape      0x1b
#define kNameMax        64

typedef struct SampleLine {
    short size;
    short style;
    const char *styleName;
} SampleLine;

static const SampleLine kLines[] = {
    { 9, 0, "plain" },  { 10, 0, "plain" }, { 11, 0, "plain" }, { 12, 0, "plain" },
    { 13, 0, "plain" }, { 14, 0, "plain" }, { 18, 0, "plain" }, { 24, 0, "plain" },
    { 12, kFontBold, "bold" }, { 12, kFontItalic, "italic" },
    { 12, kFontBold | kFontItalic, "bold italic" }, { 18, kFontBold, "bold" }
};

/* MacRoman, as the game's own text: e acute, u umlaut, ellipsis, trade mark at the end */
static const char kSample[] = "The quick brown fox jumps over the lazy dog 0123456789 \x8e\x9f\xc9\xaa";
static const char kParagraph[] =
    "Word wrap: the Federation patrol hails you as you leave the system, asking whether you have seen "
    "any pirate activity near the asteroid belt. You tell them what you know, which is not much, and "
    "they thank you and move on.";

static void SetColor(unsigned short red, unsigned short green, unsigned short blue)
{
    RGBColor color;

    color.red = red;
    color.green = green;
    color.blue = blue;
    RGBForeColor(&color);
}

static void Label(short h, short v, const char *text)
{
    TextFont(smallTextFont);
    TextFace(0);
    TextSize(kLabelSize);
    SetColor(kGrey, kGrey, kGrey);
    MoveTo(h, v);
    DrawCString(text);
}

static void DrawFontPage(short family, short count)
{
    char text[256];
    char real[kNameMax];
    Rect box;
    Rect tick;
    unsigned i;
    short y;
    short measured;
    short measuredMac;
    short drawn;

    FontRealName(family, kRealNameSize, 0, real, sizeof(real));
    snprintf(text, sizeof(text), "Font %d of %d   asked for: \"%s\"   drawn with: \"%s\"", family + 1, count,
            FontFamilyName(family), real);
    TextFont(smallTextFont);
    TextFace(0);
    TextSize(kHeaderSize);
    SetColor(0xffff, 0xffff, 0xffff);
    MoveTo(kLabelLeft, kHeaderBase);
    DrawCString(text);

    y = kFirstLineTop;
    for (i = 0; i < sizeof(kLines) / sizeof(kLines[0]); i++) {
        y += kLines[i].size + kLineGap;
        TextFont(family);
        TextFace(kLines[i].style);
        TextSize(kLines[i].size);
        measured = MeasureCString(kSample);
        measuredMac = TextWidthMacRoman(kSample, strlen(kSample));
        SetColor(0xffff, 0xffff, 0xffff);
        MoveTo(kSampleLeft, y);
        DrawCString(kSample);
        drawn = CurrentGrafPort->pnLoc.h - CurrentGrafPort->pix.bounds.left - kSampleLeft;
        /* a red tick under the line's end as TextWidthMacRoman measures it */
        SetColor(kTickRed, 0, 0);
        SetRect(&tick, kSampleLeft + measuredMac, y + 1, kSampleLeft + measuredMac + 1, y + 1 + kTickHeight);
        PaintRect(&tick);
        snprintf(text, sizeof(text), "%d %s  width %d / %d / %d", kLines[i].size, kLines[i].styleName, measured,
                measuredMac, drawn);
        Label(kLabelLeft, y, text);
    }

    y += kWrapGap;
    SetRect(&box, kSampleLeft, y, kSampleLeft + kWrapWidth, y + kWrapHeight);
    TextFont(family);
    TextFace(0);
    TextSize(kWrapSize);
    snprintf(text, sizeof(text), "%d plain, %d wide: height %d", kWrapSize, kWrapWidth,
            MeasureWrappedText(kParagraph, strlen(kParagraph), kWrapWidth));
    SetColor(0xffff, 0xffff, 0xffff);
    DrawTextRun(kParagraph, strlen(kParagraph), &box, 1, 0);
    InsetRect(&box, -1, -1);
    SetColor(kGrey, kGrey, kGrey);
    FrameRect(&box);
    Label(kLabelLeft, y + kWrapSize, text);

    Label(kLabelLeft, CurrentGrafPort->pix.bounds.bottom - kFooterRaise,
          "widths: measured as ANSI (StringWidth) / as MacRoman / pen advance when drawn; red tick = MacRoman "
          "width.   Any key or click: next font.   Esc: leave.");
}

/* true = Esc */
static Boolean WaitForInput(void)
{
    int key;

    for (;;) {
        LastTick = ReadTick();
        WaitTicks(1);
        if (Button() == 0)
            break;
    }
    for (;;) {
        LastTick = ReadTick();
        WaitTicks(1);
        key = KeyboardGet();
        if (key == kKeyEscape)
            return 1;
        if (key != 0 || Button() != 0)
            return 0;
    }
}

void DebugFontScreen(void)
{
    GWorldContainer page;
    char text[256];
    char real[kNameMax];
    Rect screen;
    short family;
    short count;

    screen = *GetCWindowPortRect((WindowRef)gWind);
    page.world = 0;
    CreateGWorldContainer(&page, GetMainDevice(), 0, &screen);
    if (page.world == 0)
        return;
    count = FontFamilyCount();
    for (family = 0; family < count; family++) {
        FontRealName(family, kRealNameSize, 0, real, sizeof(real));
        snprintf(text, sizeof(text), "font %d: asked for \"%s\", drawn with \"%s\"", family + 1, FontFamilyName(family), real);
        crashlog(text);
    }
    for (family = 0; family < count; family++) {
        SetGWorldContainer(&page);
        ForeColor(blackColor);
        PaintRect(&page.bounds);
        DrawFontPage(family, count);
        SWSetPortToWindow(gSpriteWorld);
        ForeColor(blackColor);
        CopyBits(GWorldContainerBits(&page), GetPortBitMapForCopyBits(GetCWindowGrafPort((WindowRef)gWind)),
                 &page.bounds, &page.bounds, 0, 0);
        QDFlushPortBuffer(GetWindowPort(gWind), 0);
        if (WaitForInput())
            break;
    }
    DisposeGWorldContainer(&page);
    SWSetPortToWindow(gSpriteWorld);
}
