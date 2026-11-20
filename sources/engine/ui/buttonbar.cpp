/* buttonbar.cpp -- ButtonBar: draws a dialog's picture buttons and runs the press loop they all share */
#include "buttonbar.h"
#include "gametypes.h"
#include "dialogs.h"
#include "game.h"

ButtonBar::Button &ButtonBar::Set(short slot, short item, unsigned char *label, ButtonLook look)
{
    Rect box;

    GetDialogItem(dialog, item, NULL, NULL, &box);
    return SetBox(slot, box, label, look);
}

ButtonBar::Button &ButtonBar::SetBox(short slot, const Rect &box, unsigned char *label, ButtonLook look)
{
    buttons[slot] = { box, label, look, true, true, false, NULL };
    return buttons[slot];
}

void ButtonBar::SetPress(short slot, bool onDown, bool held)
{
    buttons[slot].pressOnDown = onDown;
    buttons[slot].pressHeld = held;
}

void ButtonBar::Draw(short hilited)
{
    CGrafPtr port;

    ForeColor(blackColor);
    drawing = true;
    Update(hilited);
    drawing = false;
    port = GetDialogPort(dialog);
    for (short slot = 0; slot < count; slot++) {
        Button &b = buttons[slot];
        switch (b.look) {
        case ButtonLook::Normal:
            DrawButton(port, &b.box, b.label, 0, slot == hilited || b.latched, NULL);
            break;
        case ButtonLook::Dim:
            DrawButton(port, &b.box, b.label, kButtonDim, 0, NULL);
            break;
        case ButtonLook::Disabled:
            DrawButton(port, &b.box, b.label, kButtonDisabled, 0, NULL);
            break;
        case ButtonLook::Picture:
            DrawButton(port, &b.box, b.label, kButtonPicture, slot == hilited, b.pic);
            break;
        case ButtonLook::Hidden:
            break;
        }
    }
}

short ButtonBar::Hit(Point where, bool held) const
{
    short hit = -1;

    for (short n = 0; n < count; n++) {
        short slot = order != nullptr ? order[n] : n;
        Rect box = buttons[slot].box;
        if ((held ? buttons[slot].pressHeld : buttons[slot].pressOnDown) && PtInRectPoint(where, &box)) {
            hit = slot;
            if (rule == HitRule::FirstWins)
                break;
        }
    }
    return hit;
}

/* Hilites the button pressed while the mouse is over it; returns it if released there. */
short ButtonBar::Track(Point where)
{
    short drawn;
    short down;
    short over;
    Point mouse;

    ForeColor(blackColor);
    Update(-1);
    drawn = Hit(where, false);
    down = drawn;
    if (drawn != -1) {
        Draw(drawn);
        nv_UpdateDialogWindow(0);
        while (StillDown() != 0) {
            BusyIdle();
            GetMouse(&mouse);
            Update(-1);
            over = Hit(mouse, true);
            if (over != drawn && over != down)
                over = -1;
            if (over != drawn) {
                Draw(over);
                nv_UpdateDialogWindow(0);
            }
            drawn = over;
        }
    }
    return drawn;
}
