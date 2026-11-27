/* comm.cpp -- the communication message strip (win TU region 0x0047e2d0-0x0047e463). Notes: Research/notes/comm.md */

#include "game.h"
#include "viewscale.h"
#include "missions.h"
#include "render.h"
#include "text.h"
#include "painter.h"

#define kCommMsgMax     0xff
#define kCommTextMax    0xfe

/* the style of the message shown: white in the comm style, or a caller's own (DisplayCommStyled) */
static UI::TextSpec gCommSpec = { UI::Style::Comm, UI::kWhite };
static bool gCommDrawn;                 /* CommOverlay's frame shows the message */
static SWFrameRec *gCommFrame;          /* CommOverlay's */

/* The comm message in its box, on black: added over the view, black adds nothing */
static void DrawCommText(UI::Painter &p, const Rect &box)
{
    unsigned char text[256];

    strncpy((char *)text, commMsg, kCommTextMax);
    text[kCommTextMax] = 0;
    PtoCstr(text);
    p.TextBox(box, text, strlen((char *)text), gCommSpec, UI::kBlack);
}

/* Grows commRect upwards to two lines of the comm style plus a 4-pixel gap, measured on the comm frame. */
static void FitCommRect(SWFrameRec *frame)
{
    short need;

    need = (short)View::ScaleInView(2 * UI::Painter(UI::Surface(frame)).LineHeight(UI::Style::Comm) + 4);
    if (commRect.bottom - commRect.top < need)
        commRect.top = commRect.bottom - need;
}

/* The comm strip along the bottom of the view, for the view's size and the UI scale; CommOverlay makes its
   frame anew. */
void ResetCommRect(void)
{
    SetRect(&commRect, View::ScaleInView(25), 0, View::width - View::ScaleInView(50),
            View::height - View::ScaleInView(5));
    commRect.top = commRect.bottom - View::ScaleInView(26 + 4);   /* CommOverlay grows it to the font's two lines */
    if (gCommFrame != NULL)
        SWDisposeFrame(&gCommFrame);
}

/* Shows `msg' in the comm strip for `count' frames; Screen::ComposeView lays it over the view (CommOverlay). */
// FUNCTION: EVNOVA 0x0047e2d0
// MAC: _DisplayComm i386 0x00090201 ppc 0x0009be7c
void DisplayComm(PStrChar *msg, short count)
{
    DisplayCommStyled(msg, count, { UI::Style::Comm, UI::kWhite });
}

void DisplayCommStyled(PStrChar *msg, short count, const UI::TextSpec &spec)
{
    gCommSpec = spec;
    commCount = count;
    strncpy(commMsg, (char *)msg, kCommMsgMax);
    gCommDrawn = false;
}

const UI::TextSpec &CommStyle(void)
{
    return gCommSpec;
}

void DisplayComm(const std::string &msg, short count)
{
    Str255 text;

    ToPascal(msg, text);
    DisplayComm(text, count);
}

void DisplayCommStyled(const std::string &msg, short count, const UI::TextSpec &spec)
{
    Str255 text;

    ToPascal(msg, text);
    DisplayCommStyled(text, count, spec);
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
        if (!View::MakeOverlayFrame(&frame, bounds))
            return NULL;
        gCommDrawn = false;
        FitCommRect(frame);
    }
    if (!gCommDrawn) {
        UI::Painter p{UI::Surface(frame)};
        DrawCommText(p, frame->frameRect);
        gCommDrawn = true;
    }
    return frame;
}
