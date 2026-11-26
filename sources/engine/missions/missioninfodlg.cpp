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
#include "text.h"
#include "missionlist.h"
#include "painter.h"

#define kDateCaptionIdx     0x168

#define kMissionInfoTextSize 0xfa

// FUNCTION: EVNOVA 0x00445dc0
// MAC: _SetupMissionInfoList i386 0x0009de7a ppc 0x000a9a9c
void SetupMissionInfoList(void)
{
    MissionList list(missionInfoList);
    char marker[256];
    char nameBuf[256];
    short i;
    short count;
    short itemType;
    void *itemHandle;
    Rect box;
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

    GetDialogItem(missionInfoDialog, 2, &itemType, (Handle *)&itemHandle, &box);

    bool scrollBar = !MissionList::Fits(box, count);
    if (scrollBar)
        box.right -= 15;
    list.Create(missionInfoDialog, box, count, scrollBar);

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

    list.SelectFirst();
}

/* --- DoMissionInfoDialog (0x00446150) --- */

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
    static Boolean Filter(DialogPtr unused, const DialogEvent &ev, short *itemHit);
    static short CountActive();
    static void ShowSelected(short old, bool pick);
    static void Step(short dir);
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
    MissionList(missionInfoList).Dispose();
    dialog.Close();
    backdrop.Release();
    RedrawDialogs(1);
    PlaySound(beepSnd[2], 1, gSoundVolume, gSoundVolume);
}

void MissionInfoDialog::Hit(short item)
{
    Point cell;
    short i;
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
            MissionInfoButtons().DrawOnDialog(-1);
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
                if (CountActive() > 0) {
                    MissionList(missionInfoList).Dispose();
                    SetupMissionInfoList();
                    selMissionInfoListItem = -1;
                    RedrawDialog(missionInfoDialog);
                    cell.h = cell.v = 0;
                    {
                        UI::Painter p(UI::Surface::Dialog(missionInfoDialog));
                        while (cell.v < (*missionInfoList)->dataBounds.bottom) {
                            p.ListSelect(missionInfoList, false, cell, UI::Style::Small);
                            cell.v++;
                        }
                    }
                    LAutoScroll(missionInfoList);
                    for (i = 0; i < kMaxActiveMissions; i++)
                        activeMissions[i] = -1;
                    /* BUG: no hidden-mission filter here, unlike Setup: rows and activeMissions can disagree */
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
            }
        }
    }
}

static constexpr short kMissionInfoSlotItems[] = { 1, 5 };

/* Active missions, hidden ones included. */
short MissionInfoDialog::CountActive()
{
    short count = 0;

    for (short i = 0; i < kMaxActiveMissions; i++) {
        if (currentMissionObjectives[i].active != 0)
            count++;
    }
    return count;
}

/* Loads the selected mission's briefing and abort state (pick also selects its row); redraws briefing and abort
   button if it changed. */
void MissionInfoDialog::ShowSelected(short old, bool pick)
{
    strncpy(briefingTextPtr, "", 3);
    if (selMissionInfoListItem != -1) {
        LoadDescription(briefingTextPtr,
                        currentMissions[activeMissions[selMissionInfoListItem]].quickBriefDesc);
        MungeBriefing(0, activeMissions[selMissionInfoListItem]);
        if (pick)
            MissionList(missionInfoList).Pick(selMissionInfoListItem);
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

/* The next (dir 1) or previous (dir -1) mission slot in use, wrapping around. */
void MissionInfoDialog::Step(short dir)
{
    MissionList(missionInfoList).Leave(selMissionInfoListItem, dir);
    short old = selMissionInfoListItem;
    if (CountActive() > 0) {
        do {
            selMissionInfoListItem += dir;
            if (selMissionInfoListItem >= kMaxActiveMissions)
                selMissionInfoListItem = 0;
            else if (selMissionInfoListItem < 0)
                selMissionInfoListItem = kMaxActiveMissions - 1;
        } while (activeMissions[selMissionInfoListItem] == -1);
    }
    ShowSelected(old, true);
}

// FUNCTION: EVNOVA 0x00446770
// MAC: _MissionInfoFilter i386 0x0009d6e5 ppc 0x000a9304
Boolean MissionInfoDialog::Filter(DialogPtr unused, const DialogEvent &ev, short *itemHit)
{
    Rect box;
    short itemType;
    Point where;
    void *item;
    EventChar c;
    short old;

    if (ev.what == DialogInput::Key) {
        if (QuickKeyHit(6, -1, -1, itemHit))
            return 1;
        c = ev.ch;
        if (IsReturnChar(c) || c == kEscapeChar || KeyCheck(keyPrefs.missions)) {
            *itemHit = 1;
            return 1;
        }
        if (c == kTabChar || c == kDownArrowChar)
            Step(1);
        if (c == kUpArrowChar)
            Step(-1);
    }
    if (ev.what == DialogInput::Click) {
        where = ev.where;
        GetDialogItem(missionInfoDialog, 2, &itemType, (Handle *)&item, &box);
        if (PtInRect(where.h, where.v, &box)) {
            LClick(where, ev.mods, missionInfoList);
            old = selMissionInfoListItem;
            selMissionInfoListItem = MissionList(missionInfoList).Selected();
            ShowSelected(old, false);
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
    std::string date;
    Rect box;

    UI::Painter p(UI::Surface::Load());

    p.Fill(*GetDialogPortRect(missionInfoDialog), UI::kBlack);
    if (missionInfoBackgroundPic != NULL)
        p.Picture(missionInfoBackgroundPic, *GetDialogPortRect(missionInfoDialog));
    GetDialogItem(missionInfoDialog, 3, &itemType, (Handle *)&item, &box);
    p.Text(box.left, box.bottom, IndStringRaw(kGameStrList, kDateCaptionIdx), { UI::Style::Small, rgbMedGrey });
    date = DateMedText(gameTime);
    GetDialogItem(missionInfoDialog, 7, &itemType, (Handle *)&item, &box);
    p.Text(box.left, box.right, box.top + smallTextSize, date, { UI::Style::Small, rgbDarkGrey }, UI::Align::Center);
    DrawMissionBriefing(p, missionInfoDialog, selMissionInfoListItem != -1);
    MissionInfoButtons().Draw(p, -1);
    MissionList(missionInfoList).Present(p, missionInfoDialog);
}
