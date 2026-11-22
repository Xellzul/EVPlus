/* keydlg.cpp -- the key-binding dialog (win 0x0048b860), built in code: DITL 4002's buttons and rows plus a Speed
   Up row; the labels, headings and dotted group frames (PICT 139 there) drawn by the update proc.
   Notes: Research/notes/keydlg.md */

#include "game.h"
#include "player.h"
#include "viewscale.h"
#include "render.h"
#include "gamedialog.h"

namespace {

constexpr short kWidth = 594;
constexpr short kHeight = 353;
constexpr short kOKItem = 1;
constexpr short kCancelItem = 2;
constexpr short kDefaultItem = 3;
constexpr short kPanelItem = 4;
constexpr short kFirstBindingItem = 5;
constexpr short kSpeedUpRow = 34;
constexpr short kRowWidth = 81;
constexpr short kRowHeight = 22;
constexpr short kRowPitch = 24;
constexpr short kNoNameStrIndex = 0x119;
constexpr short kRowTextInset = 5;
constexpr short kRowBaseline = 15;
constexpr short kLabelBaseline = 14;        /* below the row's top; a two-line label 8 above and 4 below it */
constexpr short kHeadingInset = 11;         /* from the frame's left; the frame's dots leave 4 before, 7 after it */
constexpr short kAltGap = 3;                /* "Alt" ends this far left of the row */
constexpr short kLabelGap = 4;              /* a label too long for its place moves left to end this far from the row */

/* a binding row's label, two lines when `more` is set; alt: "Alt" before the key */
struct Row {
    const char *label;
    const char *more;
    bool alt;
};

/* in binding order (newKeyPrefs) */
constexpr Row kRows[kBindingCount] = {
    { "Accelerate:", nullptr, false },
    { "Reverse Course:", nullptr, false },
    { "Rotate Right:", nullptr, false },
    { "Rotate Left:", nullptr, false },
    { "Afterburner:", nullptr, false },
    { "Autopilot:", nullptr, false },
    { "Hyperspace Mode:", nullptr, false },
    { "Hyper Select:", nullptr, false },
    { "Hyper Jump:", nullptr, false },
    { "Nav System Off:", nullptr, false },
    { "Communicate:", nullptr, false },
    { "Land/Dock:", nullptr, false },
    { "Fire Primary:", nullptr, false },
    { "Fire", " Secondary:", false },
    { "Select", " Secondary:", false },
    { "Weap. Safety:", nullptr, false },
    { "Target Select:", nullptr, false },
    { "Closest Targ:", nullptr, false },
    { "Escort Menu:", nullptr, false },
    { "Attack Target:", nullptr, false },
    { "Defend Me:", nullptr, false },
    { "Hold Position:", nullptr, false },
    { "Recall:", "(alt = dock)", false },
    { "Pause:", nullptr, false },
    { "Acknowledge:", nullptr, false },
    { "Board:", nullptr, false },
    { "Jettison Cargo:", nullptr, true },
    { "Eject:", nullptr, true },
    { "Self-Destruct:", nullptr, true },
    { "Engage Cloak:", nullptr, false },
    { "Galaxy Map:", nullptr, false },
    { "Player Info:", nullptr, false },
    { "Mission Info:", nullptr, false },
    { "Show Framerate:", nullptr, false },
    { "Speed Up:", nullptr, false },
};

/* a group of rows: its dotted frame and heading, where its labels and rows start */
struct Group {
    const char *heading;
    Rect frame;
    short labelLeft;
    short rowLeft;
    short rowTop;
    short firstRow;
    short rowCount;
};

constexpr Group kGroups[] = {
    { "Navigation Controls:", { 9, 6, 312, 190 }, 11, 102, 19, 0, 12 },
    { "Battle Controls:", { 9, 205, 167, 389 }, 210, 301, 19, 12, 6 },
    { "Escort Controls:", { 178, 205, 312, 389 }, 210, 301, 188, 18, 5 },
    { "Miscellaneous Controls:", { 9, 404, 312, 588 }, 408, 500, 19, 23, 12 },
};

Rect RowBox(const Group &group, short row)
{
    Rect box;
    short top = group.rowTop + (row - group.firstRow) * kRowPitch;

    SetRect(&box, group.rowLeft, top, group.rowLeft + kRowWidth, top + kRowHeight);
    return box;
}

/* a 50% dotted frame, its top line left out from gapFrom to gapTo */
void DrawDots(const Rect &frame, short gapFrom, short gapTo)
{
    auto dot = [](short h, short v) {
        Rect r;

        if (((h + v) & 1) != 0) {
            SetRect(&r, h, v, h + 1, v + 1);
            PaintRect(&r);
        }
    };

    for (short h = frame.left; h < frame.right; h++) {
        if (h < gapFrom || h >= gapTo)
            dot(h, frame.top);
        dot(h, frame.bottom - 1);
    }
    for (short v = frame.top + 1; v < frame.bottom - 1; v++) {
        dot(frame.left, v);
        dot(frame.right - 1, v);
    }
}

void DrawGroups()
{
    TextFont(smallTextFont);
    TextSize(smallTextSize);
    ForeColor(blackColor);
    for (const Group &group : kGroups) {
        short headingLeft = group.frame.left + kHeadingInset;

        DrawDots(group.frame, headingLeft - 4, headingLeft + MeasureCString(group.heading) + 7);
        MoveTo(headingLeft, group.frame.top + 3);
        DrawCString(group.heading);
        for (short i = group.firstRow; i < group.firstRow + group.rowCount; i++) {
            const Row &row = kRows[i];
            Rect box = RowBox(group, i);
            short altLeft = box.left - kAltGap - MeasureCString("Alt");
            short right = (row.alt ? altLeft : box.left) - kLabelGap;
            short left = right - MeasureCString(row.label);

            if (left > group.labelLeft)
                left = group.labelLeft;
            if (left < group.frame.left + 2)
                left = group.frame.left + 2;
            if (row.more == nullptr) {
                MoveTo(left, box.top + kLabelBaseline);
                DrawCString(row.label);
            } else {
                MoveTo(group.labelLeft, box.top + kLabelBaseline - 8);
                DrawCString(row.label);
                MoveTo(group.labelLeft, box.top + kLabelBaseline + 4);
                DrawCString(row.more);
            }
            if (row.alt) {
                MoveTo(altLeft, box.top + kLabelBaseline);
                DrawCString("Alt");
            }
        }
    }
}

// FUNCTION: EVNOVA 0x0048b280
/* The key bindings: a key pressed binds the selected row; OK saves them unless a key is bound twice. */
class KeyDialog final : public GameDialog {
public:
    void Run();

private:
    void Hit(short item) override;
    static Boolean Filter(DialogPtr unused, const DialogEvent &ev, short *itemHit);
};

void KeyDialog::Run()
{
    ItemList items;

    items.add(kDitlButton, 323, 507, 343, 577, "OK");
    items.add(kDitlButton, 323, 425, 343, 495, "Cancel");
    items.add(kDitlButton, 323, 77, 343, 199, "Set Default");
    items.add(kDitlUserItem, 5, 6, 312, 588);
    for (const Group &group : kGroups) {
        for (short i = group.firstRow; i < group.firstRow + group.rowCount; i++) {
            Rect box = RowBox(group, i);
            items.add(kDitlUserItem, box.top, box.left, box.bottom, box.right);
        }
    }
    dialog.Open(kWidth, kHeight, items.data(), items.size(), KeyDialogUpdate, Filter, &keyDialog);
    if (!dialog.IsOpen())
        return;

    KeyPrefsToSlotTable();
    FrontWindow(keyDialog);
    SetDialogGWorld(keyDialog);
    RedrawDialog(keyDialog);

    KeyDialogUpdate();
    Loop();

    if (gWind != NULL)
        SWSetPortToWindow(gSpriteWorld);
    dialog.Close();
}

void KeyDialog::Hit(short item)
{
    short i;
    short j;
    Boolean matched;
    ScanCode localBuf[kBindingCount];
    ScanCode defaultBuf[kBindingCount];

    if (item == kOKItem) {
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
    if (item == kCancelItem)
        done = 1;
    if (item == kDefaultItem) {
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
    if (item >= kFirstBindingItem && item < kFirstBindingItem + kBindingCount) {
        keyDialogSel = item - kFirstBindingItem;
        RedrawDialog(keyDialog);
    }
}

// FUNCTION: EVNOVA 0x0048b6d0
// MAC: _KeyFilter i386 0x00017482 ppc 0x0001b030
Boolean KeyDialog::Filter(DialogPtr unused, const DialogEvent &ev, short *itemHit)
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
    if (ev.what == DialogInput::Click) {
        SetDialogGWorld(keyDialog);
        v = ev.where.v;
        h = ev.where.h;
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
    code = FirstKeyDown(keyDialogSel == kSpeedUpRow);
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
    GetDialogItem(keyDialog, kPanelItem, &itemType, &item, &picBox);
    View::SetPortToUIWork();
    ForeColor(whiteColor);
    PaintRect(&picBox);
    DrawGroups();
    ForeColor(blackColor);
    TextFont(0);
    TextSize(12);
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
