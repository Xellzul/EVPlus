/* missioninfodlg.cpp -- the mission info dialog */

#include "game.h"
#include "gameutil.h"
#include "player.h"
#include "missions.h"
#include "render.h"
#include "sound.h"
#include "stellar.h"
#include "prototypes.h"
#include "gamedialog.h"
#include "buttons.h"

#define kDateCaptionIdx     0x168

#define kMissionInfoTextSize 0xfa

// FUNCTION: EVNOVA 0x00445dc0
// MAC: _SetupMissionInfoList i386 0x0009de7a ppc 0x000a9a9c
void SetupMissionInfoList(void)
{
    char marker[256];
    char nameBuf[256];
    short i;
    short count;
    short itemType;
    void *itemHandle;
    Rect cellBounds;
    Rect box;
    unsigned char hasScrollbar;
    Point cellSize;
    Point rowPt;
    int32_t len;

    for (i = 0; i < kMaxActiveMissions; i++)
        activeMissions[i] = -1;

    count = 0;
    for (i = 0; i < kMaxActiveMissions; i++) {
        if (currentMissionObjectives[i].active != 0
                && (currentMissionObjectives[i].flags & kMissionHidden) == 0) {
            activeMissions[count] = i;
            count++;
        }
    }

    SetDialogGWorld(missionInfoDialog);
    GetDialogItem(missionInfoDialog, 2, &itemType, (Handle *)&itemHandle, &box);
    SetRect(&cellBounds, 0, 0, 1, count);

    TextFont(smallTextFont);
    TextSize(smallTextSize);

    cellSize.v = (kListItemBaseHeight * 1.5);

    if (cellSize.v * count <= box.bottom - box.top) {
        hasScrollbar = 0;
    } else {
        box.right -= 15;
        hasScrollbar = 1;
    }
    cellSize.h = box.right - box.left;
    missionInfoList = nv_NewDialog(&box, &cellBounds, cellSize,
        (ListDrawProcPtr)MissionListDefinition, (CGrafPtr)missionInfoDialog, hasScrollbar);
    LockAHandle((Handle)missionInfoList);

    rowPt.h = rowPt.v = 0;
    for (i = 0; i < kMaxActiveMissions; i++) {
        if (currentMissionObjectives[i].active == 0
                || (currentMissionObjectives[i].flags & kMissionHidden) != 0)
            continue;

        strncpy(marker, "", 3);
        if (currentMissionObjectives[i].failed != 0)
            strncpy(marker, "\xa5 ", 4);

        strncpy(nameBuf, currentMissions[i].name, kMissionInfoTextSize);
        PtoCstr((unsigned char *)nameBuf);
        strncpy(briefingTextPtr, nameBuf, kMissionInfoTextSize);
        MungeBriefing(0, i);
        strncpy(nameBuf, briefingTextPtr, kMissionInfoTextSize);
        nameBuf[kMissionInfoTextSize] = 0;
        strcat(marker, nameBuf);
        len = strlen(marker);
        LSetCell(marker, len, rowPt, missionInfoList);
        rowPt.v++;
    }

    rowPt.h = rowPt.v = 0;
    LSetSelect(1, rowPt, missionInfoList);
    LSetDrawingMode(1, missionInfoList);
    LAutoScroll(missionInfoList);
}

/* --- DoMissionInfoDialog (0x00446150) --- */

/* the stellar record, stride 0x498; only the field the map jump reads */

/* the system record, stride 0x1fc; only the id this scan matches on */

/* the player ship, only the two screen-position fields the map disturbs */

#define kMissionInfoDlogID   0x3f4
#define kMissionInfoPictID   0x2145

namespace {

// FUNCTION: EVNOVA 0x00446150
// MAC: _DoMissionInfoDialog i386 0x0009e19b ppc 0x000a9dd4
/* The player's active missions: their briefings, the map to one, and aborting one. */
class MissionInfoDialog final : public GameDialog {
public:
    void Run();

private:
    void Hit(short item) override;
    static Boolean Filter(DialogPtr unused, EventRecord *ev, short *itemHit);
};

void MissionInfoDialog::Run()
{
    if (screenFaded != 0)
        return;
    PlaySound(beepSnd[1], 1, gSoundVolume, gSoundVolume);
    dialog.Open(kMissionInfoDlogID, MissionInfoDialogUpdate, Filter, &missionInfoDialog,
                DialogClose::NoFlash);
    if (!dialog.IsOpen())
        return;
    Picture backdrop(missionInfoBackgroundPic);
    backdrop.Load(kMissionInfoPictID);
    SetDialogGWorld(missionInfoDialog);
    FlushEvents(everyEvent - diskMask, 0);
    RedrawDialog(missionInfoDialog);
    SetupMissionInfoList();
    selMissionInfoListItem = 0;
    LoadDescription(briefingTextPtr,
                    currentMissions[activeMissions[selMissionInfoListItem]].quickBriefDesc);
    MungeBriefing(0, activeMissions[selMissionInfoListItem]);
    showAbortMissionButton =
        currentMissions[activeMissions[selMissionInfoListItem]].canAbort;
    MissionInfoDialogUpdate();
    Loop();
    if (missionInfoList != NULL) {
        LDispose(missionInfoList);
        missionInfoList = NULL;
    }
    dialog.Close();
    backdrop.Release();
    RedrawDialogs(1);
    PlaySound(beepSnd[2], 1, gSoundVolume, gSoundVolume);
}

void MissionInfoDialog::Hit(short item)
{
    Point cell;
    short i;
    short count;
    short n;
    short stellar;

    if (item == 1)
        done = 1;
    if (item == 6 && missionDialog == NULL) {
        greenMissionSyst = -1;
        multiArrowsOnSingleSystem = 0;
        if (selMissionInfoListItem != -1) {
            if ((currentMissions[activeMissions[selMissionInfoListItem]].flags & kMissionBriefingArrow) != 0) {
                if (currentMissions[activeMissions[selMissionInfoListItem]].travelStellar != -1) {
                    stellar = currentMissions[activeMissions[selMissionInfoListItem]].travelStellar;
                    greenMissionSyst = stellarData[stellar].system;
                } else if (currentMissions[activeMissions[selMissionInfoListItem]].returnStellar != -1) {
                    stellar = currentMissions[activeMissions[selMissionInfoListItem]].returnStellar;
                    greenMissionSyst = stellarData[stellar].system;
                }
            }
        }
        hypergateMapMode = 0;
        DoSystemMap();
        greenMissionSyst = -1;
        multiArrowsOnSingleSystem = 1;
        RedrawAfterSubDialog(missionInfoDialog);
    }
    if (item == 5) {
        if (selMissionInfoListItem == -1) {
            PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);
            MissionInfoButtons().Draw(-1);
        } else {
            if (currentMissions[activeMissions[selMissionInfoListItem]].canAbort != 0) {
                if ((currentMissions[activeMissions[selMissionInfoListItem]].flags & kMissionAbortPenalty) != 0
                    && currentMissions[activeMissions[selMissionInfoListItem]].compGovt != -1) {
                    for (i = 0; i < kMaxSystems; i++) {
                        if (systemData[i].govt == currentMissions[activeMissions[selMissionInfoListItem]].compGovt)
                            playerRecord[i] -=
                                currentMissions[activeMissions[selMissionInfoListItem]].compReward * 5;
                    }
                }
                AbortMission(activeMissions[selMissionInfoListItem], 1);
                count = 0;
                for (i = 0; i < kMaxActiveMissions; i++) {
                    if (currentMissionObjectives[i].active != 0)
                        count++;
                }
                if (count > 0) {
                    if (missionInfoList != NULL)
                        LDispose(missionInfoList);
                    SetupMissionInfoList();
                    selMissionInfoListItem = -1;
                    RedrawDialog(missionInfoDialog);
                    cell.h = cell.v = 0;
                    while (cell.v < (*missionInfoList)->dataBounds.bottom) {
                        LSetSelect(0, cell, missionInfoList);
                        cell.v++;
                    }
                    LAutoScroll(missionInfoList);
                    for (i = 0; i < kMaxActiveMissions; i++)
                        activeMissions[i] = -1;
                    n = 0;
                    for (i = 0; i < kMaxActiveMissions; i++) {
                        if (currentMissionObjectives[i].active != 0) {
                            activeMissions[n] = i;
                            n++;
                        }
                    }
                } else {
                    done = 1;
                }
                DrawStatusCargo();
                SetDialogGWorld(missionInfoDialog);
            }
        }
    }
}

static constexpr short kMissionInfoSlotItems[] = { 1, 5 };

// FUNCTION: EVNOVA 0x00446770
// MAC: _MissionInfoFilter i386 0x0009d6e5 ppc 0x000a9304
Boolean MissionInfoDialog::Filter(DialogPtr unused, EventRecord *ev, short *itemHit)
{
    Rect box;
    short itemType;
    Point where;
    Point cell;
    void *item;
    EventKeyByte c;
    short old;
    short i;
    short count;

    if ((int)ev->what == kKeyDown) {
        if (QuickKeyHit(6, -1, -1, itemHit))
            return 1;
        c = EventCharOf(ev);
        if (IsReturnChar(c) || c == kEscapeChar || KeyCheck(keyPrefs.missions)) {
            *itemHit = 1;
            return 1;
        }
        if (c == kTabChar || c == kDownArrowChar) {
            cell.h = 0;
            cell.v = selMissionInfoListItem;
            if (selMissionInfoListItem != -1) {
                LSetSelect(0, cell, missionInfoList);
                GetListCellRect(&box, cell, missionInfoList);
                if (box.bottom >= (*missionInfoList)->rView.bottom)
                    LScroll(0, 1, missionInfoList);
            }
            old = selMissionInfoListItem;
            count = 0;
            for (i = 0; i < kMaxActiveMissions; i++) {
                if (currentMissionObjectives[i].active != 0)
                    count++;
            }
            if (count > 0) {
                do {
                    selMissionInfoListItem++;
                    if (selMissionInfoListItem >= kMaxActiveMissions)
                        selMissionInfoListItem = 0;
                } while (activeMissions[selMissionInfoListItem] == -1);
            }
            strncpy(briefingTextPtr, "", 3);
            if (selMissionInfoListItem != -1) {
                LoadDescription(briefingTextPtr,
                                currentMissions[activeMissions[selMissionInfoListItem]].quickBriefDesc);
                MungeBriefing(0, activeMissions[selMissionInfoListItem]);
                cell.v = selMissionInfoListItem;
                LSetSelect(1, cell, missionInfoList);
                LAutoScroll(missionInfoList);
                showAbortMissionButton =
                    currentMissions[activeMissions[selMissionInfoListItem]].canAbort;
            } else {
                showAbortMissionButton = 0;
            }
            if (selMissionInfoListItem != old) {
                InvalDialogItem(missionInfoDialog, 4);
                InvalDialogItem(missionInfoDialog, 5);
            }
        }
        if (c == kUpArrowChar) {
            cell.h = 0;
            cell.v = selMissionInfoListItem;
            if (selMissionInfoListItem != -1) {
                LSetSelect(0, cell, missionInfoList);
                GetListCellRect(&box, cell, missionInfoList);
                if (box.top <= (*missionInfoList)->rView.top)
                    LScroll(0, -1, missionInfoList);
            }
            old = selMissionInfoListItem;
            count = 0;
            for (i = 0; i < kMaxActiveMissions; i++) {
                if (currentMissionObjectives[i].active != 0)
                    count++;
            }
            if (count > 0) {
                do {
                    selMissionInfoListItem--;
                    if (selMissionInfoListItem < 0)
                        selMissionInfoListItem = kMaxActiveMissions - 1;
                } while (activeMissions[selMissionInfoListItem] == -1);
            }
            strncpy(briefingTextPtr, "", 3);
            if (selMissionInfoListItem != -1) {
                LoadDescription(briefingTextPtr,
                                currentMissions[activeMissions[selMissionInfoListItem]].quickBriefDesc);
                MungeBriefing(0, activeMissions[selMissionInfoListItem]);
                cell.v = selMissionInfoListItem;
                LSetSelect(1, cell, missionInfoList);
                LAutoScroll(missionInfoList);
                showAbortMissionButton =
                    currentMissions[activeMissions[selMissionInfoListItem]].canAbort;
            } else {
                showAbortMissionButton = 0;
            }
            if (selMissionInfoListItem != old) {
                InvalDialogItem(missionInfoDialog, 4);
                InvalDialogItem(missionInfoDialog, 5);
            }
        }
    }
    if ((int)ev->what == kMouseDown) {
        where = ev->where;
        GetDialogItem(missionInfoDialog, 2, &itemType, (Handle *)&item, &box);
        if (PtInRect(where.h, where.v, &box)) {
            LClick(where, ev->modifiers, missionInfoList);
            old = selMissionInfoListItem;
            selMissionInfoListItem = -1;
            cell.h = 0;
            cell.v = 0;
            for (; cell.v < (*missionInfoList)->dataBounds.bottom; cell.v++) {
                if (LGetSelect(0, &cell, missionInfoList)) {
                    selMissionInfoListItem = cell.v;
                    break;
                }
            }
            strncpy(briefingTextPtr, "", 3);
            if (selMissionInfoListItem != -1) {
                LoadDescription(briefingTextPtr,
                                currentMissions[activeMissions[selMissionInfoListItem]].quickBriefDesc);
                MungeBriefing(0, activeMissions[selMissionInfoListItem]);
                showAbortMissionButton =
                    currentMissions[activeMissions[selMissionInfoListItem]].canAbort;
            } else {
                showAbortMissionButton = 0;
            }
            if (selMissionInfoListItem != old) {
                InvalDialogItem(missionInfoDialog, 4);
                InvalDialogItem(missionInfoDialog, 5);
            }
        }
        MissionInfoDialogUpdate();
        return ButtonItemHit(MissionInfoButtons().Track(where), kMissionInfoSlotItems, itemHit);
    }
    return 0;
}

} // namespace

void DoMissionInfoDialog(void)
{
    MissionInfoDialog().Run();
}

// FUNCTION: EVNOVA 0x00446e00
// MAC: _MissionInfoDialogUpdate i386 0x00098508 ppc 0x000a42a0
void MissionInfoDialogUpdate(void)
{
    short itemType;
    void *item;
    char text[256];
    Rect box;

    SetGWorldContainer(&loadPort);
    TextFont(smallTextFont);
    TextSize(smallTextSize);
    ForeColor(blackColor);
    PaintRect(GetDialogPortRect(missionInfoDialog));
    if (missionInfoBackgroundPic != NULL)
        DrawPicHandle(missionInfoBackgroundPic, GetDialogPortRect(missionInfoDialog));
    ForeColor(blackColor);
    GetDialogItem(missionInfoDialog, 3, &itemType, (Handle *)&item, &box);
    RGBForeColor(&rgbMedGrey);
    MoveTo(box.left, box.bottom);
    DrawIndString(kGameStrList, kDateCaptionIdx);
    ForeColor(blackColor);
    RGBForeColor(&rgbDarkGrey);
    DateToMedString(gameTime, text);
    CtoPstr(text);
    GetDialogItem(missionInfoDialog, 7, &itemType, (Handle *)&item, &box);
    MoveTo((box.right + box.left) / 2 - StringWidth((unsigned char *)text) / 2,
              box.top + smallTextSize);
    DrawString((unsigned char *)text);
    ForeColor(blackColor);
    GetDialogItem(missionInfoDialog, 4, &itemType, (Handle *)&item, &box);
    if (RectInRgn(&box, GetWindowVisRgn(FrontWindow(missionInfoDialog)))) {
        if (selMissionInfoListItem != -1) {
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
    MissionInfoButtons().Draw(-1);
    if (missionInfoList != NULL)
        ListUpdate(NULL, missionInfoList);
    CopyLoadPortToDialog(missionInfoDialog);
    TextFont(smallTextFont);
    TextSize(smallTextSize);
    if (missionInfoList != NULL) {
        SetDialogGWorld(missionInfoDialog);
        ListUpdate(GetWindowVisRgn(FrontWindow(missionInfoDialog)), missionInfoList);
    }
}
