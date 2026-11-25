/* comm.cpp -- the communication message strip (win TU region 0x0047e2d0-0x0047e463). Notes: Research/notes/comm.md */

#include "game.h"
#include "viewscale.h"
#include "missions.h"
#include "render.h"
#include "text.h"

#define kCommMsgMax     0xff
#define kCommTextMax    0xfe

/* the style of the message shown: white in the comm size, or a caller's own (DisplayCommStyled) */
static struct {
    short font;
    short size;
    RGBColor color;
} gCommStyle = { 0, kCommTextSize, { 0xffff, 0xffff, 0xffff } };
static bool gCommDrawn;                 /* CommOverlay's frame shows the message */
static SWFrameRec *gCommFrame;          /* CommOverlay's */

/* The comm message in its box on the current port, on black: added over the view, black adds nothing */
static void DrawCommText(Rect *box)
{
    unsigned char text[256];

    strncpy((char *)text, commMsg, kCommTextMax);
    text[kCommTextMax] = 0;
    PtoCstr(text);
    RGBForeColor(&gCommStyle.color);
    TextFont(gCommStyle.font);
    TextSize(gCommStyle.size);
    TextFace(0);
    BackColor(blackColor);
    TETextBox(text, strlen((char *)text), box, 0);
    ForeColor(blackColor);
    BackColor(whiteColor);
}

/* Grows commRect upwards to two lines of the comm font plus a 4-pixel gap, measured on the comm frame. */
static void FitCommRect(SWFrameRec *frame)
{
    GrafPtr saved;
    short need;

    GetPort(&saved);
    SWSetPortToFrame(frame);
    TextFont(0);
    TextSize(kCommTextSize);
    TextFace(0);
    need = (short)View::ScaleInView(2 * TextLineHeight() + 4);
    SetPort(saved);
    if (commRect.bottom - commRect.top < need)
        commRect.top = commRect.bottom - need;
}

/* The comm strip along the bottom of the view, for the view's size and the UI scale; CommOverlay makes its
   frame anew. */
void ResetCommRect(void)
{
    SetRect(&commRect, 25, 0, View::width - 50, View::height - 5);
    commRect.top = commRect.bottom - View::ScaleInView(26 + 4);   /* CommOverlay grows it to the font's two lines */
    if (gCommFrame != NULL)
        SWDisposeFrame(&gCommFrame);
}

/* Shows `msg' in the comm strip for `count' frames; Screen::ComposeView lays it over the view (CommOverlay). */
// FUNCTION: EVNOVA 0x0047e2d0
// MAC: _DisplayComm i386 0x00090201 ppc 0x0009be7c
void DisplayComm(PStrChar *msg, short count)
{
    DisplayCommStyled(msg, count, 0, kCommTextSize, &rgbWhite);
}

void DisplayCommStyled(PStrChar *msg, short count, short font, short size, const RGBColor *color)
{
    gCommStyle.font = font;
    gCommStyle.size = size;
    gCommStyle.color = *color;
    commCount = count;
    strncpy(commMsg, (char *)msg, kCommMsgMax);
    gCommDrawn = false;
    SWSetPortToWindow(gSpriteWorld);                /* callers draw on the window next */
}

void DisplayComm(const std::string &msg, short count)
{
    Str255 text;

    ToPascal(msg, text);
    DisplayComm(text, count);
}

void DisplayCommStyled(const std::string &msg, short count, short font, short size, const RGBColor *color)
{
    Str255 text;

    ToPascal(msg, text);
    DisplayCommStyled(text, count, font, size, color);
}

/* The comm message drawn in its own frame, at the screen's resolution, for Screen::ComposeView to lay over
   the view; NULL when there is none. The frame is drawn again only when the message changes. */
SWFrameRec *CommOverlay(void)
{
    SWFrameRec *&frame = gCommFrame;
    static Rect bounds;
    if (commCount <= 0)
        return NULL;
    for (int pass = 0; pass < 2; pass++) {          /* the second pass follows a commRect FitCommRect grew */
        Rect local = commRect;
        OffsetRect(&local, -local.left, -local.top);
        Rect wanted = View::OverlayFrameRect(local);
        if (frame != NULL && wanted.right == bounds.right && wanted.bottom == bounds.bottom)
            break;
        if (frame != NULL)
            SWDisposeFrame(&frame);
        bounds = wanted;
        if (SWCreateFrame(GetGDevice(), &frame, &bounds, gSpriteWorld->pixelDepth, 1) != 0 || frame == NULL) {
            frame = NULL;
            return NULL;
        }
        View::ScaleOverlayFrame(frame);
        SWLockFrame(frame);
        gCommDrawn = false;
        FitCommRect(frame);
    }
    if (!gCommDrawn) {
        GrafPtr saved;
        GetPort(&saved);
        SWSetPortToFrame(frame);
        DrawCommText(&bounds);
        SetPort(saved);
        gCommDrawn = true;
    }
    return frame;
}
