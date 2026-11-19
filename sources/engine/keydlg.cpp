/* keydlg.cpp -- the key-binding dialog's update proc (win 0x0048b860). Notes: Research/notes/keydlg.md */
#include <stddef.h>

#include "macport.h"
#include "dialogs.h"
#include "game.h"
#include "player.h"
#include "render.h"
#include "gametypes.h"



/* both take an argument in this TU that their own bodies ignore */

#define kBindingCount 0x22


// FUNCTION: EVNOVA 0x0048b280
void DoKeyDialog(void)
{
    Boolean done;
    short itemHit;
    short itemType;
    Handle item;
    Rect box;
    short i;
    short j;
    Boolean matched;
    ScanCode localBuf[kBindingCount];
    ScanCode defaultBuf[kBindingCount];

    keyDialog = NULL;
    keyDialog = GetNewDialog(0xfa2, KeyDialogUpdate, KeyFilter);
    if (keyDialog == NULL)
        return;

    KeyPrefsToSlotTable();
    if (gWind != NULL)
        CenterWindow(FrontWindow(keyDialog), gWind);
    ShowMacWindow(FrontWindow(keyDialog));
    FrontWindow(keyDialog);
    SetDialogGWorld(keyDialog);
    RedrawDialog(keyDialog);
    keyPic = GetPicture(0x8b);

    done = 0;
    KeyDialogUpdate();
    do {
        if (gQuit)
            done = 1;
        MyModalDialog(keyDialog, KeyFilter, &itemHit, 0);
        if (itemHit == 1) {
            matched = 0;
            for (j = 0; j < kBindingCount; j++) {
                if (DupKeyCheck(newKeyPrefs[j], j)) {
                    keyDialogSel = j;
                    RedrawDialog(keyDialog);
                    SysBeep(0);
                    matched = 1;
                    break;
                }
            }
            if (!matched) {
                SlotTableToKeyPrefs();
                done = 1;
                SavePrefs();
            }
        }
        if (itemHit == 2)
            done = 1;
        if (itemHit == 3) {
            KeyPrefsToSlotTable();
            for (i = 0; i < kBindingCount; i++)
                localBuf[i] = newKeyPrefs[i];
            SetDefaultKeys();
            KeyPrefsToSlotTable();
            for (i = 0; i < kBindingCount; i++) {
                defaultBuf[i] = newKeyPrefs[i];
                newKeyPrefs[i] = localBuf[i];
            }
            SlotTableToKeyPrefs();
            for (i = 0; i < kBindingCount; i++)
                newKeyPrefs[i] = defaultBuf[i];
            keyDialogSel = 0;
            RedrawDialog(keyDialog);
        }
        if (itemHit >= 5 && itemHit <= 0x27) {
            keyDialogSel = itemHit - 5;
            RedrawDialog(keyDialog);
        }
    } while (!done);

    if (gWind != NULL)
        SWSetPortToWindow(gSpriteWorld);
    if (keyPic != NULL)
        QDReleaseResource(keyPic);
    DisposeDialog(keyDialog);
    keyDialog = NULL;
}

#define kFirstBindingItem   5
#define kNoNameStrIndex  0x119
#define kRowTextInset       5
#define kRowBaseline      0xf

/*
 * v and h must be signed `short`, and v must be declared first. PtInRect widens
 * them, and mwcc defers a short local's widening to the use site, so the field
 * read stays a movzx and the widening is a separate movsx hoisted out of the
 * loop.
 */
// FUNCTION: EVNOVA 0x0048b6d0
// MAC: _KeyFilter i386 0x00017482 ppc 0x0001b030
Boolean KeyFilter(DialogPtr unused, EventRecord *ev, short *itemHit)
{
    Rect box;
    short itemType;
    void *item;
    short v;
    short h;
    int i;
    ScanCode code;
    short sel;

    SetDialogGWorld(keyDialog);
    if ((int)ev->what == kMouseDown) {
        SetDialogGWorld(keyDialog);
        v = ev->where.v;
        h = ev->where.h;
        *itemHit = -1;
        i = 0;
        do {
            GetDialogItem(keyDialog, (short)i + kFirstBindingItem, &itemType,
                          (Handle *)&item, &box);
            if (PtInRect(h, v, &box) != 0) {
                *itemHit = (short)i + kFirstBindingItem;
                return 1;
            }
            i = i + 1;
        } while ((short)i < kBindingCount);
        return 0;
    }
    code = FirstKeyDown();
    if (code != kScanNone && keyDialogSel != -1) {
        newKeyPrefs[keyDialogSel] = code;
        sel = keyDialogSel;
        keyDialogSel = (keyDialogSel + 1) % kBindingCount;
        InvalDialogItem(keyDialog, sel + kFirstBindingItem);
        InvalDialogItem(keyDialog, keyDialogSel + kFirstBindingItem);
        while (KeyCheck(code))
            BusyIdle();
        return 1;
    }
    if ((int)ev->what != kUpdateEvt)
        return 0;
    KeyDialogUpdate();
    return 0;
}

// FUNCTION: EVNOVA 0x0048b860
// MAC: _KeyDialogUpdate i386 0x00017199 ppc 0x0001adf8
void KeyDialogUpdate(void)
{
    short itemType;
    Handle item;
    Rect itemBox;
    Rect fill;
    Str255 name;
    Rect picBox;
    short i;

    SetDialogGWorld(keyDialog);
    GetDialogItem(keyDialog, 4, &itemType, &item, &picBox);
    SWSetPortToWorkArea(gSpriteWorld);
    ForeColor(blackColor);
    TextFont(0);
    TextSize(12);
    DrawPicHandle(keyPic, &picBox);
    for (i = 0; i < kBindingCount; i++) {
        GetDialogItem(keyDialog, i + kFirstBindingItem, &itemType, &item, &itemBox);
        if (RectInRgn(&itemBox, GetWindowVisRgn(FrontWindow(keyDialog)))) {
            if (keyDialogSel == i) {
                fill = itemBox;
                InsetRect(&fill, 1, 1);
                ForeColor(whiteColor);
                FrameRect(&itemBox);
                InsetRect(&fill, 1, 1);
                ForeColor(blackColor);
                PaintRect(&fill);
                ForeColor(blackColor);
                FrameRect(&itemBox);
                ForeColor(whiteColor);
                MoveTo(itemBox.left + kRowTextInset, itemBox.top + kRowBaseline);
            } else {
                fill = itemBox;
                InsetRect(&fill, 1, 1);
                ForeColor(whiteColor);
                PaintRect(&fill);
                ForeColor(blackColor);
                FrameRect(&itemBox);
                ForeColor(blackColor);
                MoveTo(itemBox.left + kRowTextInset, itemBox.top + kRowBaseline);
            }
            KeyboardScanCodeToString((char *)name, sizeof(name), newKeyPrefs[i]);
            CtoPstr((char *)name);
            if (name[0] == 0)
                DrawIndString(kGameStrList, kNoNameStrIndex);
            else
                DrawString(name);
        }
    }
    ForeColor(blackColor);
    if (gWind != NULL)
        SWSetPortToWindow(gSpriteWorld);
    SetDialogGWorld(keyDialog);
    ForeColor(blackColor);
    CopyBits(GetPortPixMapBits(gSpriteWorld->workFrameP->framePort),
             GetPortBitMapForCopyBits(GetDialogPort(keyDialog)),
             &picBox, &picBox, 0, NULL);
}
