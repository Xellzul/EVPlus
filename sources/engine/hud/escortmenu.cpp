/* escortmenu.cpp -- the escort menu */

#include "game.h"
#include "stellar.h"
#include "gameutil.h"
#include "player.h"
#include "render.h"

// FUNCTION: EVNOVA 0x0049e430
// MAC: _DrawEscortMenuInFrame i386 0x00056268 ppc 0x0005e590
void DrawEscortMenuInFrame()
{
    Rect bar;
    Rect box;
    Rect *frame;
    Boolean flags[4];
    Str255 str;
    short i;
    short category;

    SWSetPortToFrame(floatingEscortMenuFrame);
    frame = &floatingEscortMenuFrame->frameRect;
    box = *frame;

    ForeColor(blackColor);
    PaintRect(&box);

    for (i = 0; i < 4; i++)
        flags[i] = 0;

    RGBForeColor(&floatingBorderColor);
    FrameRect(&box);

    ForeColor(whiteColor);
    TextFont(smallTextFont);
    TextSize(smallTextSize);

    GetIndString(str, kGameStrList, 0x85);
    MoveTo((box.left + box.right) / 2 - StringWidth(str) / 2, (box.top + 0xf));
    DrawString(str);

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
            RGBForeColor(&escortMenuColor);
            PaintRect(&bar);
        }

        if (i == -1)
            ForeColor(whiteColor);
        else if (flags[i] != 0)
            ForeColor(whiteColor);
        else
            RGBForeColor(&rgbDarkGrey);

        MoveTo(box.left + 7, (box.top + 0x28 + (i + 1) * 18));
        NumToString(i + 2, str);
        DrawString(str);
        DrawCString(") ");

        if (i == -1)
            DrawIndString(kGameStrList, 0x90);
        else if (i == 0)
            DrawIndString(kGameStrList, 0x8c);
        else if (i == 1)
            DrawIndString(kGameStrList, 0x8d);
        else if (i == 2)
            DrawIndString(kGameStrList, 0x8e);
        else if (i == 3)
            DrawIndString(kGameStrList, 0x8f);

        if (i != -1 && flags[i] != 0) {
            strcpy((char *)str, "");
            if ((int32_t)playerEscortOrders[i] == 1)
                GetIndString(str, kGameStrList, 0x91);
            else if ((int32_t)playerEscortOrders[i] == 2)
                GetIndString(str, kGameStrList, 0x92);
            else if ((int32_t)playerEscortOrders[i] == 4)
                GetIndString(str, kGameStrList, 0x93);
            else if ((int32_t)playerEscortOrders[i] == 3)
                GetIndString(str, kGameStrList, 0x94);
            else
                GetIndString(str, kGameStrList, 0x95);
            ForeColor(whiteColor);
            MoveTo(box.right - StringWidth(str) - 7, (box.top + 0x28 + (i + 1) * 18));
            DrawString(str);
        }
    }

    ForeColor(blackColor);
    SWSetPortToWindow(gSpriteWorld);
}

#define kDefaultMapZoom 0x20

// FUNCTION: EVNOVA 0x0049e8d0
// MAC: _ShowEscortMenu i386 0x0005664a ppc 0x0005e980
void ShowEscortMenu(void)
{
    escortMenuOnscreen = kDefaultMapZoom;
    DrawEscortMenuInFrame();
}
