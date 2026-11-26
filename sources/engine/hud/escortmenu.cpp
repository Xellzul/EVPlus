/* escortmenu.cpp -- the escort menu */

#include "game.h"
#include "stellar.h"
#include "gameutil.h"
#include "player.h"
#include "render.h"
#include "painter.h"
#include "text.h"

// FUNCTION: EVNOVA 0x0049e430
// MAC: _DrawEscortMenuInFrame i386 0x00056268 ppc 0x0005e590
void DrawEscortMenuInFrame()
{
    static constexpr short kGroupNames[5] = { 0x90, 0x8c, 0x8d, 0x8e, 0x8f };
    const UI::TextSpec white = { UI::Style::Small, UI::kWhite };
    const UI::TextSpec grey = { UI::Style::Small, rgbDarkGrey };
    Rect bar;
    Rect box;
    Rect *frame;
    Boolean flags[4];
    std::string str;
    short i;
    short category;

    {
        UI::Painter p{UI::Surface(floatingEscortMenuFrame)};

        frame = &floatingEscortMenuFrame->frameRect;
        box = *frame;

        p.Fill(box, UI::kBlack);

        for (i = 0; i < 4; i++)
            flags[i] = 0;

        p.Frame(box, floatingBorderColor);

        p.Text(box.left, box.right, box.top + 0xf, IndStringRaw(kGameStrList, 0x85), white, UI::Align::Center);

        for (i = 1; i < kMaxShips; i++) {
            if (!shipData[i].active)
                continue;
            if (shipData[i].parentShip != 0)
                continue;
            category = shipClass[shipData[i].type].escortType;
            if (category < 0 || category > 3)
                continue;
            flags[category] = 1;
        }

        for (i = -1; i < 4; i++) {
            if (selEscortGroup == i) {
                bar.left = box.left + 4;
                bar.right = box.right - 4;
                bar.top = box.top + 0x1e + (i + 1) * 18;
                bar.bottom = box.top + 0x2b + (i + 1) * 18;
                p.Fill(bar, escortMenuColor);
            }

            const UI::TextSpec &spec = (i == -1 || flags[i] != 0) ? white : grey;

            p.Text(box.left + 7, box.top + 0x28 + (i + 1) * 18, NumText(i + 2), spec);
            p.Text(") ", spec);
            p.Text(IndStringRaw(kGameStrList, kGroupNames[i + 1]), spec);

            if (i != -1 && flags[i] != 0) {
                if ((int32_t)playerEscortOrders[i] == 1)
                    str = IndStringRaw(kGameStrList, 0x91);
                else if ((int32_t)playerEscortOrders[i] == 2)
                    str = IndStringRaw(kGameStrList, 0x92);
                else if ((int32_t)playerEscortOrders[i] == 4)
                    str = IndStringRaw(kGameStrList, 0x93);
                else if ((int32_t)playerEscortOrders[i] == 3)
                    str = IndStringRaw(kGameStrList, 0x94);
                else
                    str = IndStringRaw(kGameStrList, 0x95);
                p.Text(box.left, box.right - 7, box.top + 0x28 + (i + 1) * 18, str, white, UI::Align::Right);
            }
        }
    }
}

#define kDefaultMapZoom 0x20

// FUNCTION: EVNOVA 0x0049e8d0
// MAC: _ShowEscortMenu i386 0x0005664a ppc 0x0005e980
void ShowEscortMenu(void)
{
    escortMenuOnscreen = kDefaultMapZoom;
    DrawEscortMenuInFrame();
}
