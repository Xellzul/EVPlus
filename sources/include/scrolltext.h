/* scrolltext.h -- a dialog's scrolling TextEdit item and its up/down buttons */
#pragma once
#include "macport.h"

class ButtonBar;

/* Reads and sets canScrollTextUp/Down. Build one where it is used: the dialog and TE come from globals. */
class ScrollingText {
public:
    struct Layout {
        short textItem, upItem, downItem;
        short upSlot, downSlot;         /* ButtonBar slots drawn hilited while a button is held */
        short lineStep, pageStep;
    };

    ScrollingText(DialogPtr dlg, TEHandle te, const Layout &layout) : dlg(dlg), te(te), layout(layout) {}

    void ProbeRange() const;                            /* can it scroll down at all; it starts at the top */
    void Scroll(short dv) const;
    bool Key(EventChar c, short *itemHit) const;        /* arrows, Home/End, PgUp/PgDn; true when used */
    bool Press(Point where, short *itemHit, ButtonBar &buttons) const;   /* a held up/down button */
    bool Wheel(short *itemHit) const;                   /* one wheel notch as an up/down item hit */
    void Step(short itemHit) const;                     /* an up/down item hit scrolls one line */
    void InvalidateChanged(Boolean &savedUp, Boolean &savedDown) const;  /* redraw buttons whose state changed */

private:
    void Hold(short dv, short slot, ButtonBar &buttons) const;

    DialogPtr dlg;
    TEHandle te;
    Layout layout;
};
