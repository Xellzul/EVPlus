/* missiondlg.cpp -- the mission computer dialog */

#include "game.h"
#include "gameutil.h"
#include "missions.h"
#include "render.h"
#include "ships.h"
#include "prototypes.h"
#include "gamedialog.h"
#include "buttons.h"

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
    static Boolean Filter(DialogPtr unused, EventRecord *ev, short *itemHit);

    Boolean accepted = 0;   /* the last accept took the mission */
};

void MissionDialog::Run()
{
    Str255 msg;
    Str255 numBuf;
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
            strcpy((char *)msg, "");
            StrcatIndCString((char *)msg, kGameStrList, kStrTooManyA);
            strcat((char *)msg, " ");
            NumToString(kMaxActiveMissions, numBuf);
            PtoCstr(numBuf);
            strcat((char *)msg, (char *)numBuf);
            strcat((char *)msg, " ");
            StrcatIndCString((char *)msg, kGameStrList, kStrTooManyB);
            strncpy(textShowString, (char *)msg, kShowStringMax);
            ShowTextDialog(0, 0);
            SetDialogGWorld(portDialog);
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
            strcpy(textShowString, "");
            StrcatIndCString(textShowString, kGameStrList, kStrNoMissions);
            ShowTextDialog(0, 0);
        }
        SetDialogGWorld(portDialog);
        return;
    }
    dialog.Open(kMissionDlogID, MissionDialogUpdate, Filter, &missionDialog, DialogClose::NoFlash);
    if (!dialog.IsOpen())
        return;
    Picture backdrop(missionBackgroundPic[0]);
    backdrop.Load(kMissionPictID);
    SetDialogGWorld(missionDialog);
    FlushEvents(everyEvent - diskMask, 0);
    reenterMissionDialog = 0;
    ForeColor(blackColor);
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
    backdrop.Release();
    if (missionList != NULL) {
        LDispose(missionList);
        missionList = NULL;
    }
    dialog.Close();
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

// FUNCTION: EVNOVA 0x00440c90
// MAC: _MissionFilter i386 0x0009f7a9 ppc 0x000ab3fc
Boolean MissionDialog::Filter(DialogPtr unused, EventRecord *ev, short *itemHit)
{
    EventKeyByte key;
    Rect box;
    short itemType;
    Point where;
    Point cell;
    void *item;
    short old;

    if (LeaveRequested(kItemDone, itemHit))
        return 1;
    if ((int)ev->what == kKeyDown) {
        if (QuickKeyHit(kItemMap, kItemPlayerInfo, kItemMissionInfo, itemHit))
            return 1;
        key = EventCharOf(ev);
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
            cell.h = 0;
            cell.v = selMissionListItem;
            if (selMissionListItem != -1) {
                LSetSelect(0, cell, missionList);
                GetListCellRect(&box, cell, missionList);
                if (box.bottom >= (*missionList)->rView.bottom)
                    LScroll(0, 1, missionList);
            }
            old = selMissionListItem;
            selMissionListItem++;
            if (selMissionListItem >= kMaxMissions
                    || dockedPortMissions[missionsOutside][selMissionListItem] == -1)
                selMissionListItem = 0;
            strncpy(briefingTextPtr, "", 3);
            if (selMissionListItem != -1) {
                LoadDescription(briefingTextPtr,
                                dockedPortMissions[missionsOutside][selMissionListItem]
                                    + kMissionDescBase);
                MungeBriefing(1,
                              dockedPortMissions[missionsOutside][selMissionListItem]);
                cell.v = selMissionListItem;
                LSetSelect(1, cell, missionList);
                LAutoScroll(missionList);
            }
            if (selMissionListItem != old) {
                InvalDialogItem(missionDialog, 4);
                InvalDialogItem(missionDialog, 5);
            }
            cell.v = selMissionListItem;
            LSetSelect(1, cell, missionList);
            LAutoScroll(missionList);
        }
        if (key == kDownArrowChar) {
            if (selMissionListItem + 1 < kMaxMissions) {
                cell.h = 0;
                cell.v = selMissionListItem;
                if (selMissionListItem != -1) {
                    LSetSelect(0, cell, missionList);
                    GetListCellRect(&box, cell, missionList);
                    if (box.bottom >= (*missionList)->rView.bottom)
                        LScroll(0, 1, missionList);
                }
                old = selMissionListItem;
                selMissionListItem++;
                if (dockedPortMissions[missionsOutside][selMissionListItem] == -1)
                    selMissionListItem = 0;
                strncpy(briefingTextPtr, "", 3);
                if (selMissionListItem != -1) {
                    LoadDescription(briefingTextPtr,
                                    dockedPortMissions[missionsOutside]
                                                      [selMissionListItem]
                                        + kMissionDescBase);
                    MungeBriefing(1,
                                  dockedPortMissions[missionsOutside]
                                                    [selMissionListItem]);
                    cell.v = selMissionListItem;
                    LSetSelect(1, cell, missionList);
                    LAutoScroll(missionList);
                }
                if (selMissionListItem != old) {
                    InvalDialogItem(missionDialog, 4);
                    InvalDialogItem(missionDialog, 5);
                }
            }
        }
        if (key == kUpArrowChar) {
            if (selMissionListItem == -1) {
                selMissionListItem = kMaxMissions - 1;
                while (dockedPortMissions[missionsOutside]
                                         [selMissionListItem - 1] == -1
                       && selMissionListItem > 1)
                    selMissionListItem--;
            }
            if (selMissionListItem != -1) {
                cell.h = 0;
                cell.v = selMissionListItem;
                if (selMissionListItem != -1) {
                    LSetSelect(0, cell, missionList);
                    GetListCellRect(&box, cell, missionList);
                    if (box.top <= (*missionList)->rView.top)
                        LScroll(0, -1, missionList);
                }
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
                strncpy(briefingTextPtr, "", 3);
                if (selMissionListItem != -1) {
                    LoadDescription(briefingTextPtr,
                                    dockedPortMissions[missionsOutside]
                                                      [selMissionListItem]
                                        + kMissionDescBase);
                    MungeBriefing(1,
                                  dockedPortMissions[missionsOutside]
                                                    [selMissionListItem]);
                    cell.v = selMissionListItem;
                    LSetSelect(1, cell, missionList);
                    LAutoScroll(missionList);
                }
                if (selMissionListItem != old) {
                    InvalDialogItem(missionDialog, 4);
                    InvalDialogItem(missionDialog, 5);
                }
            }
        }
    }
    if ((int)ev->what == kMouseDown) {
        where = ev->where;
        GetDialogItem(missionDialog, 2, &itemType, (Handle *)&item, &box);
        if (PtInRect(where.h, where.v, &box)) {
            LClick(where, ev->modifiers, missionList);
            old = selMissionListItem;
            selMissionListItem = -1;
            cell.h = 0;
            for (cell.v = 0; cell.v < (*missionList)->dataBounds.bottom; cell.v++) {
                if (LGetSelect(0, &cell, missionList)) {
                    selMissionListItem = cell.v;
                    break;
                }
            }
            strncpy(briefingTextPtr, "", 3);
            if (selMissionListItem != -1) {
                LoadDescription(briefingTextPtr,
                                dockedPortMissions[missionsOutside][selMissionListItem]
                                    + kMissionDescBase);
                MungeBriefing(1,
                              dockedPortMissions[missionsOutside][selMissionListItem]);
            }
            if (selMissionListItem != old) {
                InvalDialogItem(missionDialog, 4);
                InvalDialogItem(missionDialog, 5);
            }
        }
        GetDialogItem(missionDialog, 3, &itemType, (Handle *)&item, &box);
        if (PtInRect(where.h, where.v, &box))
            LClick(where, ev->modifiers, missionList);
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
    short i;
    short count;
    int32_t len;
    Rect box3;
    Rect box;
    Rect cellBounds;
    short itemType;
    Handle itemHandle;
    Str255 buf;
    unsigned char visible;
    Point cellSize;
    Point rowPt;

    selMissionListItem = 0;

    count = 0;
    for (i = 0; i < kMaxMissions; i++) {
        if (dockedPortMissions[0][i] != -1)
            count++;
    }

    SetDialogGWorld(missionDialog);
    GetDialogItem(missionDialog, 2, &itemType, &itemHandle, &box);
    SetRect(&cellBounds, 0, 0, 1, count);

    TextFont(smallTextFont);
    TextSize(smallTextSize);

    cellSize.v = (kListItemBaseHeight * 1.5);
    if (cellSize.v * count <= box.bottom - box.top) {
        visible = 0;
        GetDialogItem(missionDialog, 3, &itemType, &itemHandle, &box3);
        box.right = box3.right;
    } else {
        visible = 1;
    }
    cellSize.h = box.right - box.left;

    missionList = nv_NewDialog(&box, &cellBounds, cellSize,
        (ListDrawProcPtr)MissionListDefinition, (CGrafPtr)missionDialog, visible);
    LockAHandle((Handle)missionList);
    LSetDrawingMode(0, missionList);

    rowPt.h = 0;
    for (rowPt.v = 0; rowPt.v < count; rowPt.v++) {
        if (dockedPortMissions[0][rowPt.v] != -1) {
            strncpy((char *)buf, (char *)missionName[dockedPortMissions[0][rowPt.v]], 0xfa);
            PtoCstr(buf);
            strncpy(briefingTextPtr, (char *)buf, 0xfa);
            MungeBriefing(1, dockedPortMissions[0][rowPt.v]);
            strncpy((char *)buf, briefingTextPtr, 0xfa);
        } else {
            strncpy((char *)buf, "", 3);
        }
        len = strlen((char *)buf);
        LSetCell(buf, len, rowPt, missionList);
    }

    rowPt.h = rowPt.v = 0;
    LSetSelect(1, rowPt, missionList);
    LSetDrawingMode(1, missionList);
    LAutoScroll(missionList);
}

#define kListCaptionIdx  0x167

/* The mac's TruncString(width, title, smTruncEnd) for the mission title: cut the end and add an
   ellipsis until it fits. Done after MungeBriefing, so expanded tags fit too (the mac truncated
   before expanding them). */
static void TruncateTextToWidth(char *text, short width)
{
    short len;

    len = strlen(text);
    if (TextWidthMacRoman(text, len) <= width)
        return;
    while (len > 0 && TextWidthMacRoman(text, len) + TextWidthMacRoman("\xc9", 1) > width)
        len--;
    text[len] = '\xc9';
    text[len + 1] = 0;
}

// FUNCTION: EVNOVA 0x00441620
// MAC: _MissionDialogUpdate i386 0x0009f156 ppc 0x000aaf3c
void MissionDialogUpdate(void)
{
    char savedBriefing[0x2000];
    void *item;
    short itemType;
    char text[256];
    Rect box;

    SetGWorldContainer(&loadPort);
    ForeColor(blackColor);
    PaintRect(GetDialogPortRect(missionDialog));
    if (missionBackgroundPic[0] != NULL)
        DrawPicHandle(missionBackgroundPic[0], GetDialogPortRect(missionDialog));
    GetDialogItem(missionDialog, 8, &itemType, (Handle *)&item, &box);
    if (RectInRgn(&box, GetWindowVisRgn(FrontWindow(missionDialog)))) {
        TextFont(smallTextFont);
        TextSize(smallTextSize);
        RGBForeColor(&rgbLightGrey);
        MoveTo(box.left, box.top + 12);
        DrawIndString(kGameStrList, kListCaptionIdx);
        ForeColor(blackColor);
    }
    GetDialogItem(missionDialog, 5, &itemType, (Handle *)&item, &box);
    if (RectInRgn(&box, GetWindowVisRgn(FrontWindow(missionDialog)))) {
        if (selMissionListItem != -1) {
            ForeColor(whiteColor);
            TextFont(gTimesFont);
            TextSize(18);
            strncpy((char *)text, (char *)missionName[dockedPortMissions[missionsOutside]
                                                               [selMissionListItem]], 0xfa);
            PtoCstr((unsigned char *)text);
            strncpy(savedBriefing, briefingTextPtr, kBriefingTextMax);
            strncpy(briefingTextPtr, (char *)text, 0xfa);
            MungeBriefing(1, dockedPortMissions[missionsOutside][selMissionListItem]);
            strncpy((char *)text, briefingTextPtr, 0xfa);
            strncpy(briefingTextPtr, savedBriefing, kBriefingTextMax);
            TruncateTextToWidth(text, box.right - box.left - 1);
            MoveTo(box.left, box.top + CurrentGrafPort->txSize);
            DrawTextRun(text, strlen(text), NULL, 0, 0);
        } else {
            ForeColor(blackColor);
            PaintRect(&box);
        }
    }
    GetDialogItem(missionDialog, 4, &itemType, (Handle *)&item, &box);
    if (RectInRgn(&box, GetWindowVisRgn(FrontWindow(missionDialog)))) {
        if (selMissionListItem != -1) {
            ForeColor(blackColor);
            TextFont(smallTextFont);
            TextSize(smallTextSize);
            TETextBox(briefingTextPtr, strlen(briefingTextPtr), &box, 0);
            InvertRect(&box);
        } else {
            ForeColor(blackColor);
            PaintRect(&box);
        }
    }
    GetDialogItem(missionDialog, 11, &itemType, (Handle *)&item, &box);
    RGBForeColor(&rgbDarkGrey);
    DateToMedString(gameTime, (char *)text);
    CtoPstr((char *)text);
    MoveTo(box.left, box.bottom);
    DrawString((unsigned char *)text);
    ForeColor(blackColor);
    MissionButtons().Draw(-1);
    if (missionList != NULL)
        ListUpdate(NULL, missionList);
    CopyLoadPortToDialog(missionDialog);
    TextFont(smallTextFont);
    TextSize(smallTextSize);
    if (missionList != NULL) {
        SetDialogGWorld(missionDialog);
        ListUpdate(GetWindowVisRgn(FrontWindow(missionDialog)), missionList);
    }
}
