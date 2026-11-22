/* scrolltext.cpp -- ScrollDialogText and the ScrollingText widget built on it */
#include "dialogs.h"
#include "game.h"
#include "render.h"
#include "dialog.h"
#include "buttonbar.h"
#include "scrolltext.h"

Boolean canScrollTextUp;
Boolean canScrollTextDown;
uint32_t lastTextScrollTime;

#define kScrollTicksToLines 0.01667     /* ~1/60: ticks to scroll units */


// FUNCTION: EVNOVA 0x00499270
// MAC: _ScrollDialogText i386 0x00051839 ppc 0x0005a10c
void ScrollDialogText(DialogPtr dlg, short item, TEHandle hTE, short dv, Boolean force)
{
    Rect box;
    short type;
    void *handle;
    short savedTop;
    short delta;

    if (!force) {
        delta = dv * (float)(ReadTick() - lastTextScrollTime) * kScrollTicksToLines;
        dv = delta;
        if (dv == 0)
            return;
    }

    SetGWorldContainer(&loadPort);
    savedTop = (*hTE)->destRect.top;
    (*hTE)->inPort = (GrafPtr)loadPort.world;
    ForeColor(whiteColor);
    BackColor(blackColor);
    TEPinScroll(0, dv, hTE);
    ForeColor(blackColor);
    BackColor(whiteColor);
    lastTextScrollTime = ReadTick();
    SetDialogGWorld(dlg);

    if ((*hTE)->viewRect.top > (*hTE)->destRect.top)
        canScrollTextUp = 1;
    else
        canScrollTextUp = 0;

    if ((*hTE)->destRect.top == savedTop && dv < 0)
        canScrollTextDown = 0;
    else if ((*hTE)->viewRect.bottom > (*hTE)->destRect.top)
        canScrollTextDown = 1;

    GetDialogItem(dlg, item, &type, (Handle *)&handle, &box);
    CopyLoadPortToDialog(dlg, box);
}

/* TEPinScroll draws nothing here: the TE has no port until the first scroll */
void ScrollingText::ProbeRange() const
{
    short savedBottom;

    canScrollTextUp = 0;
    canScrollTextDown = 1;
    savedBottom = (*te)->destRect.bottom;
    TEPinScroll(0, -1, te);
    if ((*te)->destRect.bottom == savedBottom)
        canScrollTextDown = 0;
    TEPinScroll(0, 1, te);
}

void ScrollingText::Scroll(short dv) const
{
    ScrollDialogText(dlg, layout.textItem, te, dv, 1);
}

bool ScrollingText::Key(EventChar c, short *itemHit) const
{
    if (canScrollTextUp != 0) {
        if (c == kUpArrowChar) {
            *itemHit = layout.upItem;
            return true;
        }
        if (c == kHomeChar) {
            Scroll(kScrollToEnd);
            *itemHit = -1;
            canScrollTextUp = 0;
            return true;
        }
        if (c == kPageUpChar) {
            Scroll(layout.pageStep);
            *itemHit = -1;
            return true;
        }
    }
    if (canScrollTextDown != 0) {
        if (c == kDownArrowChar) {
            *itemHit = layout.downItem;
            return true;
        }
        if (c == kEndChar) {
            Scroll(-kScrollToEnd);
            *itemHit = -1;
            canScrollTextDown = 0;
            return true;
        }
        if (c == kPageDownChar) {
            Scroll(-layout.pageStep);
            *itemHit = -1;
            return true;
        }
    }
    return false;
}

void ScrollingText::Hold(short dv, short slot, ButtonBar &buttons) const
{
    lastTextScrollTime = ReadTick();
    while (StillDown()) {
        BusyIdle();
        Scroll(dv);
        SetDialogGWorld(dlg);
        buttons.Draw(slot);
        while (lastTextScrollTime == ReadTick() && StillDown())
            BusyIdle();
    }
    MouseFlushWheel();                  /* notches turned while held do not scroll on after the release */
    SetDialogGWorld(dlg);
    buttons.Draw(-1);
}

bool ScrollingText::Press(Point where, short *itemHit, ButtonBar &buttons) const
{
    Rect box;
    short itemType;
    void *item;

    if (canScrollTextUp != 0) {
        GetDialogItem(dlg, layout.upItem, &itemType, (Handle *)&item, &box);
        if (PtInRectPoint(where, &box) && canScrollTextUp != 0) {
            Hold(1, layout.upSlot, buttons);
            *itemHit = -1;
            return true;
        }
    }
    if (canScrollTextDown != 0) {
        GetDialogItem(dlg, layout.downItem, &itemType, (Handle *)&item, &box);
        if (PtInRectPoint(where, &box) && canScrollTextDown != 0) {
            Hold(-1, layout.downSlot, buttons);
            *itemHit = -1;
            return true;
        }
    }
    return false;
}

bool ScrollingText::Wheel(short *itemHit) const
{
    int wheel = MouseTakeWheel();

    if (wheel > 0 && canScrollTextUp != 0) {
        *itemHit = layout.upItem;
        return true;
    }
    if (wheel < 0 && canScrollTextDown != 0) {
        *itemHit = layout.downItem;
        return true;
    }
    return false;
}

void ScrollingText::Step(short itemHit) const
{
    if (itemHit == layout.upItem && canScrollTextUp != 0)
        Scroll(layout.lineStep);
    if (itemHit == layout.downItem && canScrollTextDown != 0)
        Scroll(-layout.lineStep);
}

void ScrollingText::InvalidateChanged(Boolean &savedUp, Boolean &savedDown) const
{
    if (canScrollTextUp != savedUp) {
        InvalDialogItem(dlg, layout.upItem);
        savedUp = canScrollTextUp;
    }
    if (canScrollTextDown != savedDown) {
        InvalDialogItem(dlg, layout.downItem);
        savedDown = canScrollTextDown;
    }
}
