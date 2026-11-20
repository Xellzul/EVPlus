/* buttonbar.h -- a dialog's row of picture buttons (DrawButton), drawn and pressed as one */
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
