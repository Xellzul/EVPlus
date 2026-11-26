/* missiondlg.cpp -- the mission computer dialog */

#include "game.h"
#include "gameutil.h"
#include "missions.h"
#include "render.h"
#include "ships.h"
#include "prototypes.h"
#include "gamedialog.h"
#include "buttons.h"
#include "missionlist.h"
#include "text.h"
#include "painter.h"

#define kStrTooManyA            351
#define kStrTooManyB            352
#define kStrNoMissions          353
#define kMissionDlogID         1006
#define kMissionPictID       0x2139
#define kShowStringMax        0x0fa

#define kItemDone                 7
#define kItemMap                  6
#define kItemMissionInfo         10
#define kItemPlayerInfo           9

namespace {

// FUNCTION: EVNOVA 0x0043c470
// MAC: _DoMissionDialog i386 0x000a37cf ppc 0x000af2b8
/* The mission computer: the port's missions on offer, with the map, mission info and player info keys. */
class MissionDialog final : public GameDialog {
public:
    void Run();

private:
    void Hit(short item) override;
    static Boolean Filter(DialogPtr unused, const DialogEvent &ev, short *itemHit);
    static void ShowSelected(short old, bool pick);
    static void StepDown();

    Boolean accepted = 0;   /* the last accept took the mission */
};

void MissionDialog::Run()
{
    std::string msg;
    short i;
    short j;
    short count;

    missionsOutside = 0;
    count = 0;
    for (i = 0; i < kMaxActiveMissions; i++) {
        if (currentMissionObjectives[i].active == 0)
            count++;
    }
    if (count <= 0) {
        if (reenterMissionDialog != 0) {
            reenterMissionDialog = 0;
        } else {
            msg = TextF("%s %d %s", IndString(kGameStrList, kStrTooManyA).c_str(), (int)kMaxActiveMissions,
                        IndString(kGameStrList, kStrTooManyB).c_str());
            strncpy(textShowString, msg.c_str(), kShowStringMax);
            if (msg.size() >= kShowStringMax)
                textShowString[kShowStringMax] = 0;
            ShowTextDialog(0, 0);
        }
        return;
    }
    greenMissionSyst = -1;
    multiArrowsOnSingleSystem = 1;
    count = 0;
    for (j = 0; j < kMaxMissions; j++) {
        if (dockedPortMissions[0][j] != -1) {
            if (IsMissionAvailable(dockedPortMissions[0][j], 1))
                count++;
        }
    }
    if (count <= 0) {
        if (reenterMissionDialog != 0) {
            reenterMissionDialog = 0;
        } else {
            SetShowText(IndString(kGameStrList, kStrNoMissions));
            ShowTextDialog(0, 0);
        }
        return;
    }
    dialog.Open(kMissionDlogID, MissionDialogUpdate, Filter, &missionDialog, DialogClose::NoFlash);
    if (!dialog.IsOpen())
        return;
    Picture backdrop(missionBackgroundPic[0]);
    backdrop.Load(kMissionPictID);
    FlushEvents(everyEvent - diskMask, 0);
    reenterMissionDialog = 0;
    RedrawDialog(missionDialog);
    SetupMissionList();
    ShowTheCursor();
    strncpy(briefingTextPtr, "", 3);
    if (selMissionListItem != -1) {
        LoadDescription(briefingTextPtr,
                        (dockedPortMissions[0][selMissionListItem]
                                + kMissionDescBase));
        MungeBriefing(1, dockedPortMissions[0][selMissionListItem]);
    }
    MissionDialogUpdate();
    Loop();
    dialog.Close();
    backdrop.Release();
    MissionList(missionList).Dispose();
    return;
}

void MissionDialog::Hit(short item)
{
    if (item == kItemAccept) {
        FlushEvents(3, 0); /* BUG: event kinds passed as masks: the Mac flushes no key events */
        FlushEvents(5, 0);
        FlushEvents(1, 0);
        FlushEvents(2, 0);
        if (selMissionListItem != -1)
            accepted = DoMissionAccept(
                dockedPortMissions[0][selMissionListItem]);
        if (selMissionListItem != -1)
            LoadDescription(briefingTextPtr,
                            (dockedPortMissions[0][selMissionListItem]
                                    + kMissionDescBase));
        if (accepted != 0) {
            reenterMissionDialog = 1;
            done = 1;
        } else if (selMissionListItem != -1) {
            LoadDescription(briefingTextPtr,
                            (dockedPortMissions[0][selMissionListItem]
                                    + kMissionDescBase));
            MungeBriefing(1,
                          dockedPortMissions[0][selMissionListItem]);
        }
        MissionDialogUpdate();
    }
    if (item == kItemDone) {
        reenterMissionDialog = 0;
        done = 1;
    }
    if (item == kItemMap) {
        FlushEvents(3, 0); /* BUG: event kinds passed as masks: the Mac flushes no key events */
        FlushEvents(5, 0);
        FlushEvents(1, 0);
        FlushEvents(2, 0);
        greenMissionSyst = -1;
        multiArrowsOnSingleSystem = 1;
        if (selMissionListItem != -1) {
            if ((missionAvailData[dockedPortMissions[0][selMissionListItem]].flags
                 & kMissionBriefingArrow) != 0) {
                if (briefMissionInfo[dockedPortMissions[0][selMissionListItem]].destStellar
                    != -1) {
                    greenMissionSyst = stellarData[
                        briefMissionInfo[dockedPortMissions[0][selMissionListItem]].destStellar].system;
                } else if (briefMissionInfo[dockedPortMissions[0][selMissionListItem]].returnStellar
                           != -1) {
                    greenMissionSyst = stellarData[
                        briefMissionInfo[dockedPortMissions[0][selMissionListItem]].returnStellar].system;
                }
            }
        }
        OpenMapKeepingNav();
        greenMissionSyst = -1;
        statusRedraw[kRedrawNav] = 1;
        DrawStatus();
        RedrawAfterSubDialog(missionDialog);
    }
    if (item == kItemMissionInfo) {
        FlushEvents(3, 0); /* BUG: event kinds passed as masks: the Mac flushes no key events */
        FlushEvents(5, 0);
        FlushEvents(1, 0);
        FlushEvents(2, 0);
        if (AnyMissionShown())
            DoMissionInfoDialog();
        if (selMissionListItem != -1) {
            LoadDescription(briefingTextPtr,
                            (dockedPortMissions[0][selMissionListItem]
                                    + kMissionDescBase));
            MungeBriefing(1,
                          dockedPortMissions[0][selMissionListItem]);
        }
        RedrawAfterSubDialog(missionDialog);
    }
    if (item == kItemPlayerInfo) {
        FlushEvents(3, 0); /* BUG: event kinds passed as masks: the Mac flushes no key events */
        FlushEvents(5, 0);
        FlushEvents(1, 0);
        FlushEvents(2, 0);
        PlayerInfoFromDialog(missionDialog);
    }
}

static constexpr short kMissionSlotItems[] = { kItemAccept, kItemDone };

/* Loads the selected mission's briefing (pick also selects its row); redraws briefing and title if it changed. */
void MissionDialog::ShowSelected(short old, bool pick)
{
    strncpy(briefingTextPtr, "", 3);
    if (selMissionListItem != -1) {
        LoadDescription(briefingTextPtr,
                        dockedPortMissions[missionsOutside][selMissionListItem]
                            + kMissionDescBase);
        MungeBriefing(1,
                      dockedPortMissions[missionsOutside][selMissionListItem]);
        if (pick)
            MissionList(missionList).Pick(selMissionListItem);
    }
    if (selMissionListItem != old) {
        InvalDialogItem(missionDialog, 4);
        InvalDialogItem(missionDialog, 5);
    }
}

/* The next mission, back to the first after the last. */
void MissionDialog::StepDown()
{
    MissionList(missionList).Leave(selMissionListItem, 1);
    short old = selMissionListItem;
    selMissionListItem++;
    if (selMissionListItem >= kMaxMissions
            || dockedPortMissions[missionsOutside][selMissionListItem] == -1)
        selMissionListItem = 0;
    ShowSelected(old, true);
}

// FUNCTION: EVNOVA 0x00440c90
// MAC: _MissionFilter i386 0x0009f7a9 ppc 0x000ab3fc
Boolean MissionDialog::Filter(DialogPtr unused, const DialogEvent &ev, short *itemHit)
{
    MissionList list(missionList);
    EventChar key;
    Rect box;
    short itemType;
    Point where;
    void *item;
    short old;

    if (LeaveRequested(kItemDone, itemHit))
        return 1;
    if (ev.what == DialogInput::Key) {
        if (QuickKeyHit(kItemMap, kItemPlayerInfo, kItemMissionInfo, itemHit))
            return 1;
        key = ev.ch;
        key = tolower(key);
        if (IsReturnChar(key) || key == kEscapeChar) {
            *itemHit = kItemDone;
            return 1;
        }
        if (key == 'l') {
            *itemHit = kItemDone;
            return 1;
        }
        if (key == 'a' && selMissionListItem != -1) {
            *itemHit = kItemAccept;
            return 1;
        }
        if (key == kTabChar) {
            StepDown();
            list.Pick(selMissionListItem);
        }
        if (key == kDownArrowChar && selMissionListItem + 1 < kMaxMissions)
            StepDown();
        if (key == kUpArrowChar) {
            if (selMissionListItem == -1) {
                /* BUG: with every slot filled this stops one short, so Up selects the second-to-last mission */
                selMissionListItem = kMaxMissions - 1;
                while (dockedPortMissions[missionsOutside]
                                         [selMissionListItem - 1] == -1
                       && selMissionListItem > 1)
                    selMissionListItem--;
            }
            if (selMissionListItem != -1) {
                list.Leave(selMissionListItem, -1);
                old = selMissionListItem;
                selMissionListItem--;
                if (selMissionListItem < 0) {
                    selMissionListItem = kMaxMissions - 1;
                    while (dockedPortMissions[missionsOutside]
                                             [selMissionListItem] == -1
                           && selMissionListItem > 0)
                        selMissionListItem--;
                } else if (dockedPortMissions[missionsOutside]
                                             [selMissionListItem] == -1) {
                    selMissionListItem = -1;
                }
                ShowSelected(old, true);
            }
        }
    }
    if (ev.what == DialogInput::Click) {
        where = ev.where;
        GetDialogItem(missionDialog, 2, &itemType, (Handle *)&item, &box);
        if (PtInRect(where.h, where.v, &box)) {
            LClick(where, ev.mods, missionList);
            old = selMissionListItem;
            selMissionListItem = list.Selected();
            ShowSelected(old, false);
        }
        GetDialogItem(missionDialog, 3, &itemType, (Handle *)&item, &box);
        /* BUG: when the list fits it reaches into item 3; a click there moves the hilite but not the selection */
        if (PtInRect(where.h, where.v, &box))
            LClick(where, ev.mods, missionList);
        MissionDialogUpdate();
        return ButtonItemHit(MissionButtons().Track(where), kMissionSlotItems, itemHit);
    }
    return 0;
}

} // namespace

void DoMissionDialog(void)
{
    MissionDialog().Run();
}

// FUNCTION: EVNOVA 0x0043cc20
// MAC: _SetupMissionList i386 0x000a069e ppc 0x000ac360
void SetupMissionList(void)
{
    MissionList list(missionList);
    short i;
    short count;
    int32_t len;
    Rect box3;
    Rect box;
    short itemType;
    Handle itemHandle;
    Str255 buf;
    Point rowPt;

    selMissionListItem = 0;

    count = 0;
    for (i = 0; i < kMaxMissions; i++) {
        if (dockedPortMissions[0][i] != -1)
            count++;
    }

    GetDialogItem(missionDialog, 2, &itemType, &itemHandle, &box);

    bool scrollBar = !MissionList::Fits(box, count);
    if (!scrollBar) {
        GetDialogItem(missionDialog, 3, &itemType, &itemHandle, &box3);
        box.right = box3.right;
    }
    list.Create(missionDialog, box, count, scrollBar);
    LSetDrawingMode(0, missionList);

    /* BUG: rows come from slots 0..count-1, right only while the slots are packed (they are filled that way);
       a click on a row of an empty slot would read missionName[-1] */
    rowPt.h = 0;
    for (rowPt.v = 0; rowPt.v < count; rowPt.v++) {
        if (dockedPortMissions[0][rowPt.v] != -1) {
            strncpy((char *)buf, (char *)missionName[dockedPortMissions[0][rowPt.v]], 0xfa);
            PtoCstr(buf);
            strncpy(briefingTextPtr, (char *)buf, 0xfa);
            MungeBriefing(1, dockedPortMissions[0][rowPt.v]);
            strncpy((char *)buf, briefingTextPtr, 0xfa);
            buf[0xfa] = 0;
        } else {
            strncpy((char *)buf, "", 3);
        }
        len = strlen((char *)buf);
        LSetCell(buf, len, rowPt, missionList);
    }

    list.SelectFirst();
}

#define kListCaptionIdx  0x167
#define kTitleSize       18      /* the title style's size: its baseline below the box top */

/* The mac's TruncString(width, title, smTruncEnd) for the mission title: cut the end and add an
   ellipsis until it fits in the style. Done after MungeBriefing, so expanded tags fit too (the mac truncated
   before expanding them). */
static void TruncateTextToWidth(UI::Painter &p, char *text, short width, UI::Style style)
{
    short len;

    len = strlen(text);
    if (p.DrawnWidth(text, len, style) <= width)
        return;
    while (len > 0 && p.DrawnWidth(text, len, style) + p.DrawnWidth("\xc9", 1, style) > width)
        len--;
    text[len] = '\xc9';
    text[len + 1] = 0;
}

// FUNCTION: EVNOVA 0x00441620
// MAC: _MissionDialogUpdate i386 0x0009f156 ppc 0x000aaf3c
void MissionDialogUpdate(void)
{
    char savedBriefing[kLongTextSize];
    void *item;
    short itemType;
    char text[256];
    Rect box;

    UI::Painter p(UI::Surface::Load());

    p.Fill(*GetDialogPortRect(missionDialog), UI::kBlack);
    if (missionBackgroundPic[0] != NULL)
        p.Picture(missionBackgroundPic[0], *GetDialogPortRect(missionDialog));
    GetDialogItem(missionDialog, 8, &itemType, (Handle *)&item, &box);
    p.Text(box.left, box.top + 12, IndStringRaw(kGameStrList, kListCaptionIdx), { UI::Style::Small, rgbLightGrey });
    GetDialogItem(missionDialog, 5, &itemType, (Handle *)&item, &box);
    if (selMissionListItem != -1) {
        strncpy((char *)text, (char *)missionName[dockedPortMissions[missionsOutside]
                                                           [selMissionListItem]], 0xfa);
        PtoCstr((unsigned char *)text);
        CopyBriefingText(savedBriefing, briefingTextPtr);
        strncpy(briefingTextPtr, (char *)text, 0xfa);
        MungeBriefing(1, dockedPortMissions[missionsOutside][selMissionListItem]);
        strncpy((char *)text, briefingTextPtr, 0xfa);
        text[0xfa] = 0;
        CopyBriefingText(briefingTextPtr, savedBriefing);
        TruncateTextToWidth(p, text, box.right - box.left - 1, UI::Style::Title);
        p.Text(box.left, box.top + kTitleSize, text, { UI::Style::Title, UI::kWhite });
    } else {
        p.Fill(box, UI::kBlack);
    }
    DrawMissionBriefing(p, missionDialog, selMissionListItem != -1);
    GetDialogItem(missionDialog, 11, &itemType, (Handle *)&item, &box);
    p.Text(box.left, box.bottom, DateMedText(gameTime), { UI::Style::Small, rgbDarkGrey });
    MissionButtons().Draw(p, -1);
    MissionList(missionList).Present(p, missionDialog);
}
