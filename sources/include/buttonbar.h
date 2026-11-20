/* buttonbar.h -- a dialog's row of picture buttons (DrawButton), drawn and pressed as one; filter helpers */
#pragma once
#include <array>
#include "macport.h"

enum class ButtonLook : unsigned char { Normal, Dim, Disabled, Hidden, Picture };

enum class HitRule : unsigned char { FirstWins, LastWins };

class ButtonBar {
public:
    void Draw(short hilited);
    short Track(Point where);       /* the slot pressed and released, or -1 */

protected:
    struct Button {
        Rect box;
        unsigned char *label;
        ButtonLook look;
        bool pressOnDown;           /* can be the button a mouse-down lands on */
        bool pressHeld;             /* can be the button under the mouse while it is held */
        bool latched;               /* drawn hilited whatever is hilited */
        PicHandle pic;              /* ButtonLook::Picture */
    };

    static constexpr short kMaxButtons = 7;

    static constexpr ButtonLook Lit(bool on) { return on ? ButtonLook::Normal : ButtonLook::Dim; }

    ButtonBar(DialogPtr dialog, short count, HitRule rule, const short *order = nullptr)
        : dialog(dialog), count(count), rule(rule), order(order) {}
    ~ButtonBar() = default;

    virtual void Update(short hilited) = 0;    /* fills every slot from the game state */
    bool Drawing() const { return drawing; }   /* Update runs for Draw, not for a hit test */

    /* a slot from its DITL item (or a box), pressable on down and while held */
    Button &Set(short slot, short item, unsigned char *label, ButtonLook look = ButtonLook::Normal);
    Button &SetBox(short slot, const Rect &box, unsigned char *label, ButtonLook look);
    void SetPress(short slot, bool onDown, bool held);

    std::array<Button, kMaxButtons> buttons;

private:
    short Hit(Point where, bool held) const;

    DialogPtr dialog;
    short count;
    HitRule rule;
    const short *order;             /* hit-test order, default 0..count-1 */
    bool drawing = false;
};

/* A filter's mouse-down: *itemHit = slot + 1 for the slot Track returned, else -1. Returns 1, the filter's
   "handled" result, so a filter can end with `return ButtonItemHit(...)`. */
inline Boolean ButtonItemHit(short button, short *itemHit)
{
    *itemHit = button >= 0 ? button + 1 : -1;
    return 1;
}

/* the same with each slot's item from a table (-1 for a slot that reports nothing) */
template <short N>
Boolean ButtonItemHit(short button, const short (&slotItems)[N], short *itemHit)
{
    *itemHit = button >= 0 && button < N ? slotItems[button] : -1;
    return 1;
}

/* a key that stands for a dialog item */
struct KeyItem {
    EventChar key;
    short item;
};

/* *itemHit = the item of the first entry whose key is c; false if none is */
template <short N>
bool KeyItemHit(EventChar c, const KeyItem (&keys)[N], short *itemHit)
{
    for (const KeyItem &k : keys) {
        if (c == k.key) {
            *itemHit = k.item;
            return true;
        }
    }
    return false;
}
