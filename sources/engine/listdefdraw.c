// COMPILE: -lang c++ -bool off
/* listdefdraw.c -- the mission-list TU: per-arrival mission housekeeping and the mission list's LDEF (win 0x00448910..0x00448b85). Notes: Research/notes/listdefdraw.md */
#include "macport.h"
#include "gametypes.h"
#include "shipobj.h"
#include "render.h"
#include "gameutil.h"
#include "prototypes.h"
#include "stellar.h"
#include "gameconst.h"
#include "dialogs.h"
#include "game.h"
#include "missions.h"
#include "cefix.h"

/* the mission object's globals; this file ends that object (the .CRT hook) */
unsigned char missionName[1000][256];
Boolean forceRandom;
Boolean canRefuseMission;
Boolean offeringFromShip;
Boolean showAbortMissionButton;
Boolean reenterMissionDialog;
Boolean suppressMissionOffer[1000];
PicHandle missionBackgroundPic[3];
PicHandle missionInfoBackgroundPic;
short activeMissions[16];
short missionsOutside;
short selMissionListItem;
short selMissionInfoListItem;
short offeringShipIndex;
short tempMissions[kMaxMissions];
short nextPortMission;
short lastPortMissionLoc;
DialogPtr missionDialog = NULL;
DialogPtr missionInfoDialog;
ScrollRec **missionList;
ScrollRec **missionInfoList;
char briefingTextPtr[0x2000];
uint32_t missionOfferTime;
Handle stringHandle;
TEHandle offerTextHandle;
short currentMissionIndexForSetParser;
BriefMissionRec briefMissionInfo[1000];
Handle missionFileHand;
MissionAvailRec missionAvailData[kMaxMissions];

/*
 * Per-arrival mission housekeeping (win 0x00448910). A mission wanting the
 * system the player just reached -- or the wildcard -6 -- has its shipDelay countdown
 * restarted, short for the one kind that asks for it and random otherwise, and
 * every active mission gets a fresh auxShipDelay.
 */

// FUNCTION: EVNOVA 0x00448910
// MAC: _MissionHandlePlayerEnteredNewSystem i386 0x00099f11 ppc 0x000a5d14
void MissionHandlePlayerEnteredNewSystem(void)
{
    short i;
    short sys;

    for (i = 0; i < 16; i++) {
        if (currentMissionObjectives[i].active != 0) {
            if (currentMissions[i].shipSyst >= 0 && currentMissions[i].shipSyst < 2048)
                sys = FindActiveCoLocatedSystem(currentMissions[i].shipSyst);
            else
                sys = -1;
            if (currentMissions[i].shipCount > 0) {
                if (sys == shipData[0].curSystem ||
                    currentMissions[i].shipSyst == -6) {
                    if (currentMissions[i].shipStart == 1) {
                        if (currentMissions[i].shipBehav == kMissionModeEscort && currentMissions[i].shipGoal == kMissionGoalEscort)
                            currentMissions[i].shipDelay = 30;
                        else
                            currentMissions[i].shipDelay = Rand(100) + 100;
                        currentMissions[i].specialShipsJumpedIn = 0;
                    }
                }
            }
            if ((currentMissions[i].flags & kMissionInfiniteAuxShips) != 0)
                currentMissions[i].auxShipsLeft = currentMissions[i].auxShipCount;
            currentMissions[i].auxShipDelay = Rand(70) + 70;
            currentMissions[i].auxShipsSpawned = 0;
        }
    }
}

// FUNCTION: EVNOVA 0x00448a30
// MAC: _MissionListDefinition i386 0x0009a018 ppc 0x000a5e68
void MissionListDefinition(short lMessage, Boolean lSelect, Rect *lRect, int32_t lCell,
                   short lDataOffset, short lDataLen, ScrollRec **lHandle)
{
    void *text;
    short v;

    text = *(*lHandle)->cells;
    switch (lMessage) {
    case 1:
        if (lSelect)
            RGBForeColor(&ldefColors[2]);
        else
            RGBForeColor(&ldefColors[1]);
        PaintRect(lRect);
        ForeColor(blackColor);
        if (lDataLen <= 0)
            break;
        v = lRect->top + (*lHandle)->indent.v;
        CE_SetPen(lRect->left + (*lHandle)->indent.h + CE_Scale(4),
                  v + smallTextSize - 9 + CE_Scale(9));
        TextFace(0);
        RGBForeColor(&ldefColors[0]);
        DrawText(text, lDataOffset, lDataLen);
        ForeColor(blackColor);
        break;
    case 2:
        if (lSelect)
            RGBForeColor(&ldefColors[2]);
        else
            RGBForeColor(&ldefColors[1]);
        PaintRect(lRect);
        ForeColor(blackColor);
        if (lDataLen <= 0)
            break;
        v = lRect->top + (*lHandle)->indent.v;
        CE_SetPen(lRect->left + (*lHandle)->indent.h + CE_Scale(4),
                  v + smallTextSize - 9 + CE_Scale(9));
        TextFace(0);
        RGBForeColor(&ldefColors[0]);
        DrawText(text, lDataOffset, lDataLen);
        ForeColor(blackColor);
        break;
    }
}
