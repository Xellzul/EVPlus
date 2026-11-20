/* keydlg.cpp -- the key-binding dialog's update proc (win 0x0048b860). Notes: Research/notes/keydlg.md */
#include <stddef.h>

#include "macport.h"
#include "dialogs.h"
#include "game.h"
#include "player.h"
#include "viewscale.h"
#include "render.h"
#include "gametypes.h"
#include "dialog.h"
#include "gamedialog.h"

#define kBindingCount 0x22


namespace {

// FUNCTION: EVNOVA 0x0048b280
/* The key bindings: a key pressed binds the selected row; OK saves them unless a key is bound twice. */
class KeyDialog final : public GameDialog {
public:
    void Run();

private:
    void Hit(short item) override;
    static Boolean Filter(DialogPtr unused, EventRecord *ev, short *itemHit);
};

void KeyDialog::Run()
{
    dialog.Open(0xfa2, KeyDialogUpdate, Filter, &keyDialog);
    if (!dialog.IsOpen())
        return;

    KeyPrefsToSlotTable();
    FrontWindow(keyDialog);
    SetDialogGWorld(keyDialog);
    RedrawDialog(keyDialog);
    Picture pic(keyPic);
    pic.Load(0x8b);

    KeyDialogUpdate();
    Loop();

    if (gWind != NULL)
        SWSetPortToWindow(gSpriteWorld);
    pic.Release();
    dialog.Close();
}

void KeyDialog::Hit(short item)
{
    short i;
    short j;
    Boolean matched;
    ScanCode localBuf[kBindingCount];
    ScanCode defaultBuf[kBindingCount];

    if (item == 1) {
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
    if (item == 2)
        done = 1;
    if (item == 3) {
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
    if (item >= 5 && item <= 0x27) {
        keyDialogSel = item - 5;
        RedrawDialog(keyDialog);
    }
}

#define kFirstBindingItem   5
#define kNoNameStrIndex  0x119
#define kRowTextInset       5
#define kRowBaseline      0xf

// FUNCTION: EVNOVA 0x0048b6d0
// MAC: _KeyFilter i386 0x00017482 ppc 0x0001b030
Boolean KeyDialog::Filter(DialogPtr unused, EventRecord *ev, short *itemHit)
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
    return 0;
}

} // namespace

void DoKeyDialog(void)
{
    KeyDialog().Run();
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
    View::SetPortToUIWork();
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
    CopyBits(GetPortPixMapBits(View::UIWork()->framePort),
             GetPortBitMapForCopyBits(GetDialogPort(keyDialog)),
             &picBox, &picBox, 0, NULL);
}
