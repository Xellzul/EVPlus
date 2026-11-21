/* alerts.cpp -- alerts, yes/no, text input and quantity dialogs */

#include "dialogs.h"
#include "render.h"
#include "ai.h"
#include "dialog.h"
#include "platform.h"

DialogPtr alertDialog;
DialogPtr qtyDialog;

EXTERN_C_BEGIN

/* the port's body takes no arguments and answers the single window; every
   caller still passes the dialog, so it is declared that way here */

// FUNCTION: EVNOVA 0x004976b0
// MAC: _EVNoteAlert i386 0x00055710 ppc 0x0005d994
void EVNoteAlert(const unsigned char *text)
{
    Boolean done;
    Rect box;
    short itemHit;
    short itemType;
    Handle item;

    done = 0;
    Dialog dialog;
    dialog.Open(3000, NULL, NULL, &alertDialog);
    if (!dialog.IsOpen()) {
        char message[256];

        memcpy(message, text + 1, text[0]);
        message[text[0]] = 0;
        PlatAlert("EV Nova", message);
        return;
    }
    SetDialogGWorld(alertDialog);
    SetDialogDefaultItem(alertDialog, 1);
    SetDialogGWorld(alertDialog);
    SetDialogGWorld(alertDialog);
    DrawDialog(alertDialog);
    GetDialogItem(alertDialog, 3, &itemType, &item, &box);
    SetDialogItemText((Handle)item,text);
    FlushEvents(everyEvent - diskMask, 0);
    ShowTheCursor();
    do {
        itemHit = dialog.Modal();
        if (itemHit == 1)
            done = 1;
    } while (done == 0);
    dialog.Close();
}

// FUNCTION: EVNOVA 0x004977d0
// MAC: _EVYesNoDialog i386 0x0005558c ppc 0x0005d84c
Boolean EVYesNoDialog(unsigned char *text)
{
    Boolean done;
    Boolean result;
    Rect box;
    short itemHit;
    short itemType;
    Handle item;

    done = 0;
    Dialog dialog;
    dialog.Open(3002, NULL, NULL, &alertDialog);
    if (!dialog.IsOpen())
        return 0;
    SetDialogDefaultItem(alertDialog, 1);
    SetDialogGWorld(alertDialog);
    GetDialogItem(alertDialog, 3, &itemType, &item, &box);
    SetDialogItemText((Handle)item,text);
    FlushEvents(everyEvent - diskMask, 0);
    ShowTheCursor();
    do {
        itemHit = dialog.Modal();
        if (itemHit == 1) {
            result = 1;
            done = 1;
        }
        if (itemHit == 5) {
            result = 0;
            done = 1;
        }
    } while (done == 0);
    SWSetPortToWindow(gSpriteWorld);
    dialog.Close();
    SWSetPortToWindow(gSpriteWorld);
    return result;
}

// FUNCTION: EVNOVA 0x00497900
// MAC: _EVTextInputDialog i386 0x000552a3 ppc 0x0005d610
Boolean EVTextInputDialog(unsigned char *prompt, unsigned char *initial, short maxLen)
{
    Boolean result;
    Boolean done;
    Str255 text;
    short itemHit;
    Rect box;
    Handle item;
    short itemType;

    done = 0;
    Dialog dialog;
    dialog.Open(3001, NULL, NULL, &alertDialog);
    if (!dialog.IsOpen())
        return 0;
    SetDialogDefaultItem(alertDialog, 1);
    SetDialogGWorld(alertDialog);
    GetDialogItem(alertDialog, 3, &itemType, &item, &box);
    SetDialogItemText((Handle)item,prompt);
    GetDialogItem(alertDialog, 5, &itemType, &item, &box);
    SetDialogItemText((Handle)item,initial);
    SelectDialogItemText(alertDialog, 5, 0, 254);
    FlushEvents(everyEvent - diskMask, 0);
    ShowTheCursor();
    do {
        itemHit = dialog.Modal();
        if (itemHit == 1) {
            GetDialogItem(alertDialog, 5, &itemType, &item, &box);
            GetDialogItemText((Handle)item,text);
            if (text[0] <= maxLen) {
                result = 1;
                done = 1;
            } else {
                SysBeep(0);
                SelectDialogItemText(alertDialog, 5, 0, maxLen - 1);
            }
        }
        if (itemHit == 6) {
            result = 0;
            done = 1;
        }
    } while (done == 0);
    GetDialogItem(alertDialog, 5, &itemType, &item, &box);
    GetDialogItemText((Handle)item,textInputString);
    SWSetPortToWindow(gSpriteWorld);
    dialog.Close();
    SWSetPortToWindow(gSpriteWorld);
    return result;
}

#define kQtyDlogID 0x3eb
#define kQtyPromptItem 1
#define kQtyMaxItem 2
#define kQtyTextItem 3

// FUNCTION: EVNOVA 0x0049e8e0
// MAC: _DoQuantityDialog i386 0x00056d5b ppc 0x0005f1e4
int32_t DoQuantityDialog(int32_t maxCount, short mode)
{
    Handle item;
    short itemHit;
    int32_t value;
    Boolean done;
    short i;
    Rect box;
    short itemType;
    Str255 text;
    Str255 digits;
    Boolean allDigits;
    short destIdx;
    int32_t result;
    Dialog dialog;

    dialog.Open(kQtyDlogID, NULL, NULL, &qtyDialog, DialogClose::NoFlash);
    if (!dialog.IsOpen())
        return 0;

    SetDialogGWorld(qtyDialog);
    SelectDialogItemText(qtyDialog, kQtyTextItem, 0, 0x7fff);

    GetDialogItem(qtyDialog, kQtyPromptItem, &itemType, &item, &box);
    if (mode == 1)
        SetControlTitle((ControlRef)item,buttonLabel[kSellButtonLabel]);
    else if (mode == 2)
        SetControlTitle((ControlRef)item,buttonLabel[kBetButtonLabel]);
    else
        SetControlTitle((ControlRef)item,buttonLabel[kBuyButtonLabel]);
    if (mode == 2)
        GetIndString(text, kGameStrList, 0x174);
    else
        GetIndString(text, kGameStrList, 0x173);

    GetDialogItem(qtyDialog, kQtyMaxItem, &itemType, &item, &box);
    SetDialogItemText((Handle)item,text);

    GetDialogItem(qtyDialog, kQtyTextItem, &itemType, &item, &box);
    NumToString(maxCount, text);
    SetDialogItemText((Handle)item,text);

    done = 0;
    result = 0;
    do {
        itemHit = dialog.Modal();
        if (itemHit == 4) {
            done = 1;
        } else if (itemHit == 1) {
            GetDialogItem(qtyDialog, kQtyTextItem, &itemType, &item, &box);
            GetDialogItemText((Handle)item,text);
            if (text[0] != 0) {
                allDigits = 1;
                digits[0] = 0;
                destIdx = 1;
                for (i = 0; i < text[0]; i++) {
                    if (text[i + 1] >= '0' && text[i + 1] <= '9') {
                        digits[0]++;
                        digits[destIdx] = text[i + 1];
                        destIdx++;
                    } else {
                        allDigits = 0;
                    }
                }
                if (allDigits) {
                    StringToNum(text, &value);
                    if (value >= 0 && value <= maxCount) {
                        result = value;
                        done = 1;
                    } else {
                        SysBeep(0);
                        if (value < 0)
                            NumToString(0, text);
                        else
                            NumToString(maxCount, text);
                        SetDialogItemText((Handle)item,text);
                        SelectDialogItemText(qtyDialog, kQtyTextItem, 0, 0x7fff);
                    }
                } else {
                    SysBeep(0);
                    SetDialogItemText((Handle)item,digits);
                    SelectDialogItemText(qtyDialog, kQtyTextItem, 0, 0x7fff);
                }
            } else {
                SysBeep(0);
                SetDialogItemText((Handle)item,(const unsigned char *)"\p");
                SelectDialogItemText(qtyDialog, kQtyTextItem, 0, 0x7fff);
            }
        }
    } while (!done);

    dialog.Close();
    return result;
}

EXTERN_C_END
