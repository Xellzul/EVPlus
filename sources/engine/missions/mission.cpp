/* mission.cpp -- mission load, accept, success, failure and abort, availability, objectives */

#include "ai.h"
#include "game.h"
#include "gameutil.h"
#include "player.h"
#include "crashlog.h"
#include "missions.h"
#include "render.h"
#include "ships.h"
#include "sound.h"
#include "stellar.h"
#include "prototypes.h"
#include "endianswap.h"
#include "gamedialog.h"

#define kDudeResName "d\x9f" "de"
#define kSpobResName "sp\x9a" "b"

/* the resource through the handle's own cast: 1.0.10 keeps one deref, win and 1.1.1 reread it */
#define MisnFileRec ((MisnRes *)*missionFileHand)

// FUNCTION: EVNOVA 0x0043bbb0
// MAC: _InitMissions i386 0x00097b97 ppc 0x000a37ac
void InitMissions(void)
{
    MisnRes *rec;
    short i;
    short j;
    short resID;
    uint32_t resType;
    short numLoaded;
    short numMisns;
    unsigned char name[256];
    int32_t k;

    lastMissionStellar = -1;
    offeringFromShip = 0;
    offeringShipIndex = -1;
    missionDialog = NULL;
    greenMissionSyst = -1;
    multiArrowsOnSingleSystem = 1;
    forceRandom = EVBugResourceFieldTest(15);
    currentMissionIndexForSetParser = -1;
    if (EVBugResourceFieldTest(1))
        return;

    if (!EVBugResourceFieldTest(9)) {
        for (j = 0; j < kMaxActiveMissions; j++) {
            currentMissionObjectives[j].active = 0;
            currentMissionObjectives[j].failed = 0;
        }
        for (i = 0; i < kMaxMissionBits; i++)
            missionBit[i] = 0;
        for (i = 0; i < kMaxMissions; i++) {
            missionAvailData[i].availStel = -32000;
            missionAvailData[i].sourceStellar = -1;
            missionAvailData[i].destStellar = -1;
            missionAvailData[i].availRandom = 0;
            missionAvailData[i].availLoc = -1;
            missionAvailData[i].availShipType = -1;
            missionAvailData[i].cargoType = -1;
            missionAvailData[i].cargoQty = -1;
            missionAvailData[i].requires[0] = 0;
            missionAvailData[i].requires[1] = 0;
            missionAvailData[i].payment = 0;
            missionAvailData[i].dispWeight = 0;
            strcpy(missionAvailData[i].availBits, "");
        }
    }

    numLoaded = 0;
    numMisns = CountResources(kMisnType);
    for (i = 0; i < kMaxMissions; i++) {
        missionFileHand = NULL;
        missionFileHand = GetResource(kMisnType, i + kFirstResourceID);
        if (missionFileHand != NULL) {
            numLoaded++;
            rec = (MisnRes *)*missionFileHand;
            missionAvailData[i].availStel = ((MisnRes *)*missionFileHand)->availStel;
            missionAvailData[i].availLoc = MisnFileRec->availLoc;
            missionAvailData[i].availRecord = MisnFileRec->availRecord;
            missionAvailData[i].availRating = MisnFileRec->availRating;
            missionAvailData[i].availRandom = MisnFileRec->availRandom;
            missionAvailData[i].cargoQty = MisnFileRec->cargoQty;
            missionAvailData[i].cargoType = MisnFileRec->cargoType;
            missionAvailData[i].sourceStellar = MisnFileRec->travelStel;
            missionAvailData[i].destStellar = MisnFileRec->returnStel;
            missionAvailData[i].timeLimit = MisnFileRec->timeLimit;
            missionAvailData[i].flags = MisnFileRec->flags;
            missionAvailData[i].flags2 = MisnFileRec->flags2;
            missionAvailData[i].availShipType = MisnFileRec->availShipType;
            missionAvailData[i].requires[0] = MisnFileRec->require1;
            missionAvailData[i].requires[1] = MisnFileRec->require2;
            missionAvailData[i].payment = MisnFileRec->payVal;
            missionAvailData[i].dispWeight = MisnFileRec->dispWeight;
            if (missionAvailData[i].availStel < -1) {
                crashlog_print("warning: mission ");
                crashlog_print_num(i + kFirstResourceID);
                crashlog_print(" has invalid AvailStel value\n");
                missionAvailData[i].availStel = -1;
            }
            if (((MisnRes *)*missionFileHand)->shipDude >= kFirstResourceID
                    && ((MisnRes *)*missionFileHand)->shipDude < kFirstResourceID + kMaxDudes) {
                if (dudeData[((MisnRes *)*missionFileHand)->shipDude - kFirstResourceID].loaded == 0) {
                    crashlog_print("warning: mission ");
                    crashlog_print_num(i + kFirstResourceID);
                    crashlog_print(" makes reference to nonexistent " kDudeResName " resource ");
                    crashlog_print_num(((MisnRes *)*missionFileHand)->shipDude);
                    crashlog_print("\n");
                }
            } else if (((MisnRes *)*missionFileHand)->shipCount > 0) {
                crashlog_print("warning: mission ");
                crashlog_print_num(i + kFirstResourceID);
                crashlog_print(" makes reference to nonexistent " kDudeResName " resource ");
                crashlog_print_num(((MisnRes *)*missionFileHand)->shipDude);
                crashlog_print("\n");
            }
            if (((MisnRes *)*missionFileHand)->auxShipDude >= kFirstResourceID
                    && ((MisnRes *)*missionFileHand)->auxShipDude < kFirstResourceID + kMaxDudes) {
                if (dudeData[((MisnRes *)*missionFileHand)->auxShipDude - kFirstResourceID].loaded == 0) {
                    crashlog_print("warning: mission ");
                    crashlog_print_num(i + kFirstResourceID);
                    crashlog_print(" makes reference to nonexistent " kDudeResName " resource ");
                    crashlog_print_num(((MisnRes *)*missionFileHand)->auxShipDude);
                    crashlog_print("\n");
                }
            } else if (*(short *)(*missionFileHand + 0x48) > 0) {
                crashlog_print("warning: mission ");
                crashlog_print_num(i + kFirstResourceID);
                crashlog_print(" makes reference to nonexistent " kDudeResName " resource ");
                crashlog_print_num(((MisnRes *)*missionFileHand)->auxShipDude);
                crashlog_print("\n");
            }
            if (((MisnRes *)*missionFileHand)->travelStel >= kFirstResourceID
                    && ((MisnRes *)*missionFileHand)->travelStel < kFirstResourceID + kMaxStellars) {
                if (stellarData[((MisnRes *)*missionFileHand)->travelStel - kFirstResourceID].prepared == 0) {
                    crashlog_print("warning: mission ");
                    crashlog_print_num(i + kFirstResourceID);
                    crashlog_print(" makes reference to nonexistent " kSpobResName " resource ");
                    crashlog_print_num(((MisnRes *)*missionFileHand)->travelStel);
                    crashlog_print("\n");
                }
            }
            if (((MisnRes *)*missionFileHand)->returnStel >= kFirstResourceID
                    && ((MisnRes *)*missionFileHand)->returnStel < kFirstResourceID + kMaxStellars) {
                if (stellarData[((MisnRes *)*missionFileHand)->returnStel - kFirstResourceID].prepared == 0) {
                    crashlog_print("warning: mission ");
                    crashlog_print_num(i + kFirstResourceID);
                    crashlog_print(" makes reference to nonexistent " kSpobResName " resource ");
                    crashlog_print_num(((MisnRes *)*missionFileHand)->returnStel);
                    crashlog_print("\n");
                }
            }
            strcpy(missionAvailData[i].availBits, ((MisnRes *)*missionFileHand)->availBits);
            AdjustHandleSize(missionFileHand, kMisnResSize);
            if (missionAvailData[i].availLoc < 0)
                missionAvailData[i].availLoc = 0;
            GetResInfo(missionFileHand, &resID, &resType, name);
            TrimResourceName(name);
            strncpy((char *)missionName[i], (char *)name, 250);
            ReleaseResource(missionFileHand);
            if (i % 5 == 0)
                IterateInGameEventLoop(10);
        }
        if (numLoaded >= numMisns)
            break;
    }

    for (i = 0; i < kMaxMissions; i++)
        suppressMissionOffer[i] = 0;
    lastPortMissionLoc = -1;
}

#define kRandomStellarA  -2
#define kRandomStellarB  -3

// FUNCTION: EVNOVA 0x0043c3e0
// MAC: _MissionPostLoadValidCheck i386 0x0009b8f2 ppc 0x000a78ec
void MissionPostLoadValidCheck(void)
{
    short i;

    if (crashlog_isactive() != 0) {
        for (i = 0; i < kMaxMissions; i++) {
            if (missionAvailData[i].sourceStellar == kRandomStellarA
                    || missionAvailData[i].sourceStellar == kRandomStellarB
                    || missionAvailData[i].sourceStellar > 0)
                SelectMissionStellar(missionAvailData[i].sourceStellar, -1, -1, i);
            if (missionAvailData[i].destStellar == kRandomStellarA
                    || missionAvailData[i].destStellar == kRandomStellarB
                    || missionAvailData[i].destStellar > 0)
                SelectMissionStellar(missionAvailData[i].destStellar, -1, -1, i);
        }
    }
}

#define kStrNoHoldsAtAll       355
#define kStrHoldsAreFull       356
#define kMissionNameMax       0x7f

/* mission Pay values below this encode a COST of (kMissionCostFloor - pay) */
#define kMissionCostFloor   (-50000L)

// FUNCTION: EVNOVA 0x0043f100
// MAC: _DoMissionAccept i386 0x000a1b11 ppc 0x000ad8bc
Boolean DoMissionAccept(short mission)
{
    short i;
    short n;
    short savedVenue;
    short slot;
    short list[kMaxMissions];

    slot = -1;
    for (i = 0; i < kMaxActiveMissions; i++) {
        if (currentMissionObjectives[i].active == 0) {
            slot = i;
            break;
        }
    }
    if (slot == -1)
        return 0;

    if (briefMissionInfo[mission].cargoUnits > 0) {
        if (ShipTotalHolds() < briefMissionInfo[mission].cargoUnits) {
            strcpy(textShowString, "");
            StrcatIndCString(textShowString, kGameStrList, kStrNoHoldsAtAll);
            ShowTextDialog(0, 0);
            return 0;
        }
        if (TotalMissionCargoSpace() < briefMissionInfo[mission].cargoUnits) {
            strcpy(textShowString, "");
            StrcatIndCString(textShowString, kGameStrList, kStrHoldsAreFull);
            ShowTextDialog(0, 0);
            return 0;
        }
    }

    currentMissionObjectives[slot].active = 1;
    currentMissionObjectives[slot].failed = 0;
    currentMissionObjectives[slot].returnDone = 0;
    currentMissionObjectives[slot].objectivesDone = 0;
    currentMissions[slot].specialShipsKilled = 0;
    currentMissions[slot].specialShipsJumpedOut = 0;
    currentMissions[slot].specialShipsJumpedIn = 0;
    currentMissions[slot].specialShipsBoarded = 0;
    currentMissions[slot].specialShipsDisabled = 0;
    strncpy(currentMissions[slot].name, (char *)missionName[mission], kMissionNameMax);
    LoadCurrentMissionData(mission, slot);
    currentMissions[slot].misnIndex = mission;
    if (currentMissions[slot].timeLimit > 0)
        SetMissionDeadline(&currentMissionObjectives[slot].dueDate, currentMissions[slot].timeLimit);
    currentMissionObjectives[slot].flags = currentMissions[slot].flags;
    crashlog_mission_bit_set("beginning mission", currentMissions[slot].name, 1,
                             currentMissions[slot].misnIndex + kFirstResourceID,
                             currentMissions[slot].beginSetString);
    EvalCurrentMissionBitSetString(currentMissions[slot].beginSetString, slot);
    if (currentMissions[slot].payment < kMissionCostFloor) {
        shipData->credits -= kMissionCostFloor - currentMissions[slot].payment;
        if (shipData->credits < 0)
            shipData->credits = 0;
        statusRedraw[kRedrawCargo] = 1;
    }
    if (missionDialog != NULL)
        SetDialogVisible(missionDialog, 0);
    if (currentMissions[slot].briefDesc != -1) {
        strncpy(briefingTextPtr, "", 3);
        LoadDescription(briefingTextPtr, currentMissions[slot].briefDesc);
        MungeBriefing(0, slot);
        strncpy(textShowString, briefingTextPtr, kBriefingTextMax);
        if (missionDialog != NULL)
            SetDialogVisible(missionDialog, 0);
        greenMissionSyst = -1;
        multiArrowsOnSingleSystem = 1;
        if (currentMissions[slot].flags & kMissionBriefingArrow) {
            if (currentMissions[slot].travelStellar != -1)
                greenMissionSyst = stellarData[currentMissions[slot].travelStellar].system;
            else if (currentMissions[slot].returnStellar != -1)
                greenMissionSyst = stellarData[currentMissions[slot].returnStellar].system;
        }
        if (portDialog != NULL && !offeringFromShip)
            PortDialogUpdate();
        ShowTextDialog(1, 1);
        greenMissionSyst = -1;
    }
    if (currentMissions[slot].pickupMode == 0) {
        currentMissions[slot].hasCargo = 1;
        if (currentMissions[slot].loadCargoDesc != -1) {
            strncpy(briefingTextPtr, "", 3);
            LoadDescription(briefingTextPtr, currentMissions[slot].loadCargoDesc);
            MungeBriefing(0, slot);
            strncpy(textShowString, briefingTextPtr, kBriefingTextMax);
            ShowTextDialog(0, 1);
            RefreshSAT();
        }
    }
    if ((currentMissions[slot].travelStellar == shipData->targetStellar && missionDialog != NULL)
        || currentMissions[slot].travelStellar == -1)
        currentMissionObjectives[slot].returnDone = 1;
    if (!offeringFromShip && missionDialog != NULL) {
        savedVenue = missionsOutside;
        for (missionsOutside = 0; missionsOutside <= 1; missionsOutside++) {
            n = 0;
            for (i = 0; i < kMaxMissions; i++)
                list[i] = -1;
            for (i = 0; i < kMaxMissions; i++) {
                if (mission != dockedPortMissions[missionsOutside][i]
                    && dockedPortMissions[missionsOutside][i] != -1) {
                    if (IsMissionAvailable(dockedPortMissions[missionsOutside][i], 1)) {
                        list[n] = dockedPortMissions[missionsOutside][i];
                        n++;
                    } else {
                        crashlog_print("suppressing previously available mission ");
                        crashlog_print_num(dockedPortMissions[missionsOutside][i] + kFirstResourceID);
                        crashlog_print("\n");
                    }
                }
            }
            for (i = 0; i < kMaxMissions; i++)
                dockedPortMissions[missionsOutside][i] = list[i];
        }
        missionsOutside = savedVenue;
    }
    if (playerIsLanded && !offeringFromShip
        && (currentMissions[slot].flags & kMissionAutoAbort)
        && currentMissionObjectives[slot].returnDone
        && currentMissions[slot].shipCount == 0
        && currentMissions[slot].returnStellar == -1)
        AutoAbortMission(slot);
    missionRandom[mission] = 0;
    return 1;
}

// FUNCTION: EVNOVA 0x0043f8c0
// MAC: _LoadCurrentMissionData i386 0x000a0a38 ppc 0x000ac638
void LoadCurrentMissionData(short mission, short slot)
{
    Str255 buf;
    Handle h;
    short strCount;
    short n;

    currentMissions[slot].travelStellar = briefMissionInfo[mission].destStellar;
    currentMissions[slot].returnStellar = briefMissionInfo[mission].returnStellar;
    currentMissions[slot].cargoType = briefMissionInfo[mission].cargoType;
    currentMissions[slot].cargoQty = briefMissionInfo[mission].cargoUnits;

    missionFileHand = GetResource(kMisnType, mission + kFirstResourceID);
    if (missionFileHand != NULL) {
        LockAHandle(missionFileHand);
        currentMissions[slot].flags = ((MisnRes *)*missionFileHand)->flags;
        currentMissions[slot].flags2 = ((MisnRes *)*missionFileHand)->flags2;
        currentMissions[slot].pickupMode = ((MisnRes *)*missionFileHand)->pickupMode;
        currentMissions[slot].dropoffMode = ((MisnRes *)*missionFileHand)->dropOffMode;
        currentMissions[slot].scanMask = ((MisnRes *)*missionFileHand)->scanMask;
        currentMissions[slot].payment = ((MisnRes *)*missionFileHand)->payVal;
        currentMissions[slot].shipCount = ((MisnRes *)*missionFileHand)->shipCount;
        currentMissions[slot].initialShipCount = currentMissions[slot].shipCount;
        currentMissions[slot].shipDude = ((MisnRes *)*missionFileHand)->shipDude;
        if (currentMissions[slot].shipDude > 0)
            currentMissions[slot].shipDude -= kFirstResourceID;
        currentMissions[slot].shipGoal = ((MisnRes *)*missionFileHand)->shipGoal;
        currentMissions[slot].shipBehav = ((MisnRes *)*missionFileHand)->shipBehav;
        currentMissions[slot].shipStart = ((MisnRes *)*missionFileHand)->shipStart;
        if ((currentMissions[slot].flags & kMissionFixedShipType)
                && currentMissions[slot].shipDude >= 0) {
            currentMissions[slot].shipTypeIndex = SelectShipFieldFromDude(&dudeData[currentMissions[slot].shipDude], 0);
            if (currentMissions[slot].shipTypeIndex == -1)
                currentMissions[slot].shipTypeIndex = SelectShipFieldFromDude(&dudeData[currentMissions[slot].shipDude], 1);
        } else {
            currentMissions[slot].shipTypeIndex = -1;
        }
        currentMissions[slot].compGovt = ((MisnRes *)*missionFileHand)->compGovt;
        currentMissions[slot].compReward = ((MisnRes *)*missionFileHand)->compReward;
        if (currentMissions[slot].compGovt >= kFirstResourceID && currentMissions[slot].compGovt < kFirstResourceID + kMaxGovts)
            currentMissions[slot].compGovt -= kFirstResourceID;
        else
            currentMissions[slot].compGovt = -1;
        if (currentMissions[slot].compGovt < 0 || currentMissions[slot].compGovt >= kMaxGovts)
            currentMissions[slot].compReward = 0;
        currentMissions[slot].briefDesc = ((MisnRes *)*missionFileHand)->briefText;
        currentMissions[slot].quickBriefDesc = ((MisnRes *)*missionFileHand)->quickBrief;
        currentMissions[slot].loadCargoDesc = ((MisnRes *)*missionFileHand)->loadCargText;
        currentMissions[slot].dropCargoDesc = ((MisnRes *)*missionFileHand)->dumpCargoText;
        currentMissions[slot].compDesc = ((MisnRes *)*missionFileHand)->compText;
        currentMissions[slot].failDesc = ((MisnRes *)*missionFileHand)->failText;
        currentMissions[slot].shipDoneDesc = ((MisnRes *)*missionFileHand)->shipDoneText;
        currentMissions[slot].refuseDesc = ((MisnRes *)*missionFileHand)->refuseText;
        currentMissions[slot].timeLimit = ((MisnRes *)*missionFileHand)->timeLimit;
        currentMissions[slot].datePostIncrement = ((MisnRes *)*missionFileHand)->datePostInc;
        if (currentMissions[slot].briefDesc <= 0)
            currentMissions[slot].briefDesc = -1;
        if (currentMissions[slot].quickBriefDesc <= 0)
            currentMissions[slot].quickBriefDesc = -1;
        if (currentMissions[slot].loadCargoDesc <= 0)
            currentMissions[slot].loadCargoDesc = -1;
        if (currentMissions[slot].dropCargoDesc <= 0)
            currentMissions[slot].dropCargoDesc = -1;
        if (currentMissions[slot].compDesc <= 0)
            currentMissions[slot].compDesc = -1;
        if (currentMissions[slot].failDesc <= 0)
            currentMissions[slot].failDesc = -1;
        if (currentMissions[slot].shipDoneDesc <= 0)
            currentMissions[slot].shipDoneDesc = -1;
        if (currentMissions[slot].refuseDesc <= 0)
            currentMissions[slot].refuseDesc = -1;
        currentMissions[slot].auxShipDude = ((MisnRes *)*missionFileHand)->auxShipDude;
        currentMissions[slot].auxShipCount = ((MisnRes *)*missionFileHand)->auxShipCount;
        currentMissions[slot].auxShipSyst = ((MisnRes *)*missionFileHand)->auxShipSyst;
        currentMissions[slot].auxShipDelay = Rand(70) + 70;
        currentMissions[slot].auxShipsLeft = currentMissions[slot].auxShipCount;
        if (currentMissions[slot].auxShipDude >= kFirstResourceID && currentMissions[slot].auxShipDude < kFirstResourceID + kMaxDudes) {
            currentMissions[slot].auxShipDude -= kFirstResourceID;
        } else {
            currentMissions[slot].auxShipDude = -1;
            currentMissions[slot].auxShipCount = 0;
        }
        if (currentMissions[slot].auxShipCount <= 0) {
            currentMissions[slot].auxShipCount = -1;
            currentMissions[slot].auxShipsLeft = -1;
        }
        if (currentMissions[slot].shipStart > 0) {
            if (currentMissions[slot].shipBehav == kMissionModeEscort && currentMissions[slot].shipGoal == kMissionGoalEscort)
                currentMissions[slot].shipDelay = 30;
            else
                currentMissions[slot].shipDelay = Rand(100) + 100;
        } else {
            currentMissions[slot].shipDelay = -1;
        }
        currentMissions[slot].destStrList = -1;
        currentMissions[slot].destStrIndex = -1;
        currentMissions[slot].shipStrList = -1;
        currentMissions[slot].shipStrIndex = -1;
        if (currentMissions[slot].timeLimit <= 0)
            currentMissions[slot].timeLimit = -32000;
        if (((MisnRes *)*missionFileHand)->canAbort != 0)
            currentMissions[slot].canAbort = 1;
        else
            currentMissions[slot].canAbort = 0;
        n = ((MisnRes *)*missionFileHand)->shipSyst;
        if (n == -1) {
            currentMissions[slot].shipSyst = shipData->curSystem;
        } else if (n == kSystemRandom) {
            currentMissions[slot].shipSyst = SelectMissionSystem(n, shipData->curSystem);
        } else if (n == -3) {
            if (briefMissionInfo[mission].destStellar >= 0
                    && briefMissionInfo[mission].destStellar < kMaxStellars) {
                currentMissions[slot].shipSyst =
                    stellarData[briefMissionInfo[mission].destStellar].system;
            } else if (briefMissionInfo[mission].returnStellar >= 0
                    && briefMissionInfo[mission].returnStellar < kMaxStellars) {
                currentMissions[slot].shipSyst =
                    stellarData[briefMissionInfo[mission].returnStellar].system;
            } else {
                currentMissions[slot].shipSyst = -1;
                crashlog_print("warning: mission ");
                crashlog_print_num(mission + kFirstResourceID);
                crashlog_print(" failed to find a suitable system for special ships (TravelStel doesn't exist!)\n");
            }
        } else if (n == -4) {
            if (briefMissionInfo[mission].returnStellar >= 0
                    && briefMissionInfo[mission].returnStellar < kMaxStellars) {
                currentMissions[slot].shipSyst =
                    stellarData[briefMissionInfo[mission].returnStellar].system;
            } else {
                currentMissions[slot].shipSyst = -1;
                crashlog_print("warning: mission ");
                crashlog_print_num(mission + kFirstResourceID);
                crashlog_print(" failed to find a suitable system for special ships (ReturnStel doesn't exist!)\n");
            }
        } else if (n == -6) {
            currentMissions[slot].shipSyst = -6;
        } else if (n == kSystemAdjacent || n >= kSystemGovt - 1) {
            currentMissions[slot].shipSyst = SelectMissionSystem(n, shipData->curSystem);
        } else if (n >= kFirstResourceID) {
            currentMissions[slot].shipSyst = n - kFirstResourceID;
        } else {
            if (currentMissions[slot].shipCount > 0) {
                crashlog_print("warning: mission ");
                crashlog_print_num(mission + kFirstResourceID);
                crashlog_print(" contained invalid special ship system reference\n");
            }
            currentMissions[slot].shipSyst = -1;
        }
        strncpy(currentMissions[slot].shipName, (char *)"\p", 3);
        strncpy(currentMissions[slot].shipSubtitle, (char *)"\p", 3);
        if (((MisnRes *)*missionFileHand)->shipNameID != -1) {
            currentMissions[slot].destStrList = ((MisnRes *)*missionFileHand)->shipNameID;
            h = GetResource(kStringListType, currentMissions[slot].destStrList);
            if (h != NULL) {
                LockAHandle(h);
                strCount = SwapEndianShort((const unsigned short *)*h);
                ReleaseResource(h);
                n = Rand(strCount) + 1;
                GetIndString(buf, ((MisnRes *)*missionFileHand)->shipNameID, n);
                strncpy(currentMissions[slot].shipName, (char *)buf, 0x3f);
                currentMissions[slot].destStrIndex = n;
            }
        }
        if (((MisnRes *)*missionFileHand)->shipSubtitle != -1) {
            currentMissions[slot].shipStrList = ((MisnRes *)*missionFileHand)->shipSubtitle;
            h = GetResource(kStringListType, currentMissions[slot].shipStrList);
            if (h != NULL) {
                LockAHandle(h);
                strCount = SwapEndianShort((const unsigned short *)*h);
                ReleaseResource(h);
                n = Rand(strCount) + 1;
                GetIndString(buf, ((MisnRes *)*missionFileHand)->shipSubtitle, n);
                strncpy(currentMissions[slot].shipSubtitle, (char *)buf, 0x3f);
                currentMissions[slot].shipStrIndex = n;
            }
        }
        strcpy(currentMissions[slot].beginSetString, ((MisnRes *)*missionFileHand)->onAccept);
        strcpy(currentMissions[slot].setString2, ((MisnRes *)*missionFileHand)->onRefuse);
        strcpy(currentMissions[slot].succSetString, ((MisnRes *)*missionFileHand)->onSuccess);
        strcpy(currentMissions[slot].failSetString, ((MisnRes *)*missionFileHand)->onFailure);
        strcpy(currentMissions[slot].abortSetString, ((MisnRes *)*missionFileHand)->onAbort);
        strcpy(currentMissions[slot].shipDoneSetString, ((MisnRes *)*missionFileHand)->onShipDone);
        currentMissions[slot].hasCargo = 0;
        currentMissions[slot].specialShipsKilled = 0;
        currentMissions[slot].specialShipsBoarded = 0;
        currentMissions[slot].specialShipsDisabled = 0;
        currentMissions[slot].specialShipsJumpedOut = 0;
        if (currentMissions[slot].shipStart == 1)
            currentMissions[slot].specialShipsJumpedIn = 0;
        else
            currentMissions[slot].specialShipsJumpedIn = currentMissions[slot].initialShipCount;
        currentMissions[slot].auxShipsSpawned = 0;
        ReleaseResource(missionFileHand);
    }
    currentMissions[slot].misnIndex = mission;
}

/* the capacity gate a mission offer has to clear before it can be accepted.
   Both callers pass the mission's +0x12 and +0x14 counts, but only the second
   is ever read -- both free-space figures are checked against it, and each
   shortfall has its own STR# 2002 line. */

// FUNCTION: EVNOVA 0x00440370
// MAC: _DoMissionCargoPickup i386 0x00098292 ppc 0x000a3f50
Boolean DoMissionCargoPickup(short unused, short needed)
{
    if (needed > 0) {
        if (ShipTotalHolds() < needed) {
            strcpy(textShowString, "");
            StrcatIndCString(textShowString, kGameStrList, 357);
            ShowTextDialog(0, 0);
            return 0;
        }
        if (TotalMissionCargoSpace() < needed) {
            strcpy(textShowString, "");
            StrcatIndCString(textShowString, kGameStrList, 358);
            ShowTextDialog(0, 0);
            return 0;
        }
    }
    statusRedraw[kRedrawCargo] = 1;
    ResetPlayerPrecalcedValues();
    return 1;
}

// FUNCTION: EVNOVA 0x00440410
// MAC: _DoMissionSuccess i386 0x000a03fc ppc 0x000abff8
void DoMissionSuccess(short mission)
{
    short i;

    if (currentMissions[mission].compDesc != -1) {
        ShowTheCursor();
        strncpy(briefingTextPtr, "", 3);
        LoadDescription(briefingTextPtr, currentMissions[mission].compDesc);
        MungeBriefing(0, mission);
        strncpy(textShowString, briefingTextPtr, kBriefingTextMax);
        ShowTextDialog(0, 1);
    }
    currentMissionObjectives[mission].active = 0;
    crashlog_mission_bit_set("mission success", currentMissions[mission].name, 1,
                             currentMissions[mission].misnIndex + kFirstResourceID,
                             currentMissions[mission].succSetString);
    EvalCurrentMissionBitSetString(currentMissions[mission].succSetString, mission);
    if (currentMissions[mission].datePostIncrement > 0) {
        crashlog_print_mdy(&gameTime);
        crashlog_print(": incrementing date ");
        crashlog_print_num(currentMissions[mission].datePostIncrement);
        crashlog_print(" days for mission ");
        crashlog_print_num(currentMissions[mission].misnIndex + kFirstResourceID);
        crashlog_print(" completion post increment\n");
        for (i = 0; i < currentMissions[mission].datePostIncrement; i++)
            IncrementGameTime();
    }
    if (currentMissions[mission].compGovt >= 0 && currentMissions[mission].compGovt < kMaxGovts) {
        for (i = 0; i < kMaxSystems; i++) {
            if (systemData[i].govt == currentMissions[mission].compGovt)
                playerRecord[i] += currentMissions[mission].compReward;
            else if (systemData[i].govt != -1) {
                if (GovtEnemies(systemData[i].govt, currentMissions[mission].compGovt))
                    playerRecord[i] -= currentMissions[mission].compReward * 0.5;
                else if (GovtAllies(systemData[i].govt, currentMissions[mission].compGovt))
                    playerRecord[i] += currentMissions[mission].compReward * 0.5;
            }
        }
    }
    ApplyMissionPay(currentMissions[mission].payment);
    lastMissionStellar = -1;
}

/* The mission payment field doubles as a command channel. Positive values are
   cash; four negative bands instead clear the player's criminal record -- with
   one government, with its allies, with the third relation -- or write a
   percentage off their money. Government ids arrive biased by the resource
   base, which is where the 10128/20128/30128 constants come from. */
// FUNCTION: EVNOVA 0x00440750
// MAC: _ApplyMissionPay i386 0x00098326 ppc 0x000a3ffc
void ApplyMissionPay(int32_t amount)
{
    int32_t i;
    int32_t govt;

    if (amount > 0) {
        shipData[0].credits += amount;
        statusRedraw[kRedrawCargo] = 1;
    } else if (amount <= -kPayClearGovt && amount > -kPayClearAllies) {
        govt = abs(amount) - (kPayClearGovt + kFirstResourceID);
        for (i = 0; i < kMaxSystems; i++) {
            if (govt == systemData[i].govt && playerRecord[i] < 0)
                playerRecord[i] = 0;
        }
    } else if (amount <= -kPayClearAllies && amount > -kPayClearClass) {
        govt = abs(amount) - (kPayClearAllies + kFirstResourceID);
        for (i = 0; i < kMaxSystems; i++) {
            if (GovtAllies(systemData[i].govt, govt) && playerRecord[i] < 0)
                playerRecord[i] = 0;
        }
    } else if (amount <= -kPayClearClass && amount > -kPctOfCashBase) {
        govt = abs(amount) - (kPayClearClass + kFirstResourceID);
        for (i = 0; i < kMaxSystems; i++) {
            if (GovtSharedClass(systemData[i].govt, govt) && playerRecord[i] < 0)
                playerRecord[i] = 0;
        }
    } else if (amount <= -(kPctOfCashBase + 1) && amount > -(kPctOfCashBase + 100)) {
        i = abs(amount) - kPctOfCashBase;
        shipData[0].credits -= shipData[0].credits * ((float)i * 0.01);
    }
}

/* a failed mission costs legal standing in every system of the offended
   government, then shows its failure desc and aborts itself */
// FUNCTION: EVNOVA 0x00440930
// MAC: _DoMissionFailure i386 0x000a0285 ppc 0x000abe54
void DoMissionFailure(short mission)
{
    short i;

    crashlog_mission_bit_set("mission failure", currentMissions[mission].name, 1,
                             currentMissions[mission].misnIndex + kFirstResourceID,
                             currentMissions[mission].failSetString);
    EvalCurrentMissionBitSetString(currentMissions[mission].failSetString, mission);
    if (currentMissions[mission].compGovt != -1) {
        for (i = 0; i < kMaxSystems; i++) {
            if (systemData[i].govt == currentMissions[mission].compGovt)
                playerRecord[i] -= currentMissions[mission].compReward / 2;
        }
    }
    currentMissionObjectives[mission].active = 0;
    if (currentMissions[mission].failDesc != -1) {
        ShowTheCursor();
        strncpy(briefingTextPtr, "", 3);
        LoadDescription(briefingTextPtr, currentMissions[mission].failDesc);
        MungeBriefing(0, mission);
        strncpy(textShowString, briefingTextPtr, kBriefingTextMax);
        ShowTextDialog(0, 1);
    }
    AbortMission(mission, 0);
    lastMissionStellar = -1;
}

/* the ships an aborted mission owned are detached and sent away; `evalBits`
   is what a deliberate abort passes to run the mission's abort expression,
   and a failure path passes 0 so that only the failure expression runs */
// FUNCTION: EVNOVA 0x00440aa0
// MAC: _AbortMission i386 0x00099a92 ppc 0x000a57c4
void AbortMission(short mission, Boolean evalBits)
{
    short i;

    for (i = 1; i < kMaxShips; i++) {
        if (shipData[i].active != 0 && shipData[i].specialMission == mission) {
            shipData[i].specialMission = -1;
            if (shipData[i].parentShip != -1) {
                shipData[i].aiType = shipClass[shipData[i].type].inherentAI;
                shipData[i].parentShip = -1;
                AIMakeShipLeave(&shipData[i]);
            }
            if (portDialog != NULL)
                shipData[i].active = 0;
        }
    }
    if (evalBits != 0) {
        crashlog_mission_bit_set("aborting mission", currentMissions[mission].name, 1,
                                 currentMissions[mission].misnIndex + kFirstResourceID,
                                 currentMissions[mission].abortSetString);
        EvalCurrentMissionBitSetString(currentMissions[mission].abortSetString, mission);
    }
    currentMissions[mission].hasCargo = 0;
    currentMissionObjectives[mission].active = 0;
    lastMissionStellar = -1;
}

// FUNCTION: EVNOVA 0x00440bf0
// MAC: _QuickMissionFailure i386 0x00099d1b ppc 0x000a5ac8
void QuickMissionFailure(short mission)
{
    crashlog_mission_bit_set("quick mission failure", currentMissions[mission].name, 1,
                             currentMissions[mission].misnIndex + kFirstResourceID,
                             currentMissions[mission].failSetString);
    EvalCurrentMissionBitSetString(currentMissions[mission].failSetString, mission);
    currentMissionObjectives[mission].failed = 1;
    if (currentMissions[mission].canAbort != 0)
        AbortMission(mission, 0);
    lastMissionStellar = -1;
}

#define kMissionConds        10

#define kAvailStelSentinel  -32000  /* availStel at or below this is "never" */
#define kAvailLocShip            2  /* availLoc: offered by a ship, not a place */

#define kAvailLinkBase        5000
#define kAvailOwnerBase      10000  /* 10000 + govt; 9999 is the independent govt, -1 */
#define kAvailAlliedBase     15000
#define kAvailGovtBase       20000
#define kAvailEnemyBase      25000
#define kAvailClassBase      30000
#define kAvailNotClassBase   31000

#define kMinFuelForMission 100.0f
#define kLargeShipClass          3

/* mïsn ship-type requirement classes (ResClassBase) */
#define kReqNotShipType     1
#define kReqShipGovt        2
#define kReqNotShipGovt     3

#define kCostFloor          -50000

// FUNCTION: EVNOVA 0x00441b40
// MAC: _IsMissionAvailable i386 0x0009b152 ppc 0x000a7000
Boolean IsMissionAvailable(short mission, Boolean offer)
{
    Boolean cond[kMissionConds];
    short n;
    short syst;
    short loc;

    if (missionAvailData[mission].availStel <= kAvailStelSentinel)
        return 0;
    if (missionAvailData[mission].availRandom <= 0)
        return 0;
    if (missionAvailData[mission].availLoc == -1)
        return 0;
    if (missionAvailData[mission].availLoc != kAvailLocShip) {
        if (offeringFromShip != 0)
            return 0;
        if (missionsOutside != 0 && missionAvailData[mission].availLoc == 0)
            return 0;
        if (missionsOutside == 0 && missionAvailData[mission].availLoc != 0)
            return 0;
    } else {
        if (offeringFromShip == 0)
            return 0;
    }

    if (missionAvailData[mission].availStel < kFirstResourceID
        || missionAvailData[mission].availStel >= kFirstResourceID + kMaxStellars) {
        if (missionAvailData[mission].sourceStellar >= kFirstResourceID
            && missionAvailData[mission].sourceStellar < kFirstResourceID + kMaxStellars) {
            syst = stellarData[missionAvailData[mission].sourceStellar - kFirstResourceID].system;
            if (syst < 0 || syst >= kMaxSystems)
                syst = FindSystemFromStellar(
                    missionAvailData[mission].sourceStellar - kFirstResourceID);
            if (syst >= 0 && syst < kMaxSystems) {
                if (FindFirstCoLocatedSystem(syst)
                    == FindFirstCoLocatedSystem(shipData->curSystem))
                    return 0;
            }
        }
        if (missionAvailData[mission].destStellar >= kFirstResourceID
            && missionAvailData[mission].destStellar < kFirstResourceID + kMaxStellars) {
            syst = stellarData[missionAvailData[mission].destStellar - kFirstResourceID].system;
            if (syst < 0 || syst >= kMaxSystems)
                syst = FindSystemFromStellar(
                    missionAvailData[mission].destStellar - kFirstResourceID);
            if (syst >= 0 && syst < kMaxSystems) {
                if (FindFirstCoLocatedSystem(syst)
                    == FindFirstCoLocatedSystem(shipData->curSystem))
                    return 0;
            }
        }
    }

    if ((missionAvailData[mission].flags & kMissionTakesFuel) != 0
        && shipData->fuel < kMinFuelForMission)
        return 0;

    for (n = 0; n < kMissionConds; n++)
        cond[n] = 0;

    loc = missionAvailData[mission].availStel;
    if (loc == -1 || offeringFromShip != 0) {
        cond[0] = 1;
    } else if (loc >= kFirstResourceID && loc < kAvailLinkBase) {
        if (missionAvailData[mission].availStel - kFirstResourceID == shipData->targetStellar)
            cond[0] = 1;
    } else if (loc >= kAvailLinkBase && loc < kAvailOwnerBase - 1) {
        for (n = 0; n < kNavSlots; n++) {
            if (systemData[shipData->curSystem].links[n]
                == missionAvailData[mission].availStel - kAvailLinkBase) {
                cond[0] = 1;
                break;
            }
        }
    } else if (missionAvailData[mission].availStel >= kAvailOwnerBase - 1
               && missionAvailData[mission].availStel < kAvailAlliedBase) {
        if (missionAvailData[mission].availStel - kAvailOwnerBase == dockedPort->govt)
            cond[0] = 1;
    } else if (loc >= kAvailAlliedBase && loc < kAvailGovtBase) {
        if (dockedPort->govt != -1
            && GovtAllies(loc - kAvailAlliedBase, dockedPort->govt) != 0)
            cond[0] = 1;
    } else if (missionAvailData[mission].availStel >= kAvailGovtBase
               && missionAvailData[mission].availStel < kAvailEnemyBase) {
        if (missionAvailData[mission].availStel - kAvailGovtBase != dockedPort->govt)
            cond[0] = 1;
    } else if (loc >= kAvailEnemyBase && loc < kAvailClassBase) {
        if (dockedPort->govt != -1
            && GovtEnemies(loc - kAvailEnemyBase, dockedPort->govt) != 0)
            cond[0] = 1;
    } else if (loc >= kAvailClassBase && loc < kAvailNotClassBase) {
        if (GovtSharedClass(loc - kAvailClassBase, dockedPort->govt) != 0)
            cond[0] = 1;
    } else if (loc >= kAvailNotClassBase && loc < 32000) {
        if (dockedPort->govt != -1
            && GovtSharedClass(loc - kAvailClassBase, dockedPort->govt) == 0)
            cond[0] = 1;
    }

    if (offer != 0)
        missionAvailData[mission].avail =
            EvalMissionBitTestString(missionAvailData[mission].availBits);
    if (missionAvailData[mission].avail != 0)
        cond[1] = 1;

    cond[6] = PlayerMeetsRequirements(missionAvailData[mission].requires[0],
                                      missionAvailData[mission].requires[1]);

    if (missionAvailData[mission].availRecord == 0) {
        cond[2] = 1;
    } else if (missionAvailData[mission].availRecord <= kAvailStelSentinel
               && offeringFromShip == 0) {
        if (missionAvailData[mission].availRecord == kAvailStelSentinel
            && stellarData[shipData->targetStellar].dominated != 0) {
            cond[2] = 1;
        } else if (missionAvailData[mission].availRecord == kAvailStelSentinel - 1) {
            for (n = 0; n < kMaxSystems; n++) {
                if (stellarData[n].loaded != 0 && stellarData[n].dominated != 0) {
                    cond[2] = 1;
                    break;
                }
            }
        }
    } else if (missionAvailData[mission].availRecord > 0) {
        if (playerRecord[shipData->curSystem]
            >= missionAvailData[mission].availRecord)
            cond[2] = 1;
    } else if (playerRecord[shipData->curSystem]
               <= missionAvailData[mission].availRecord) {
        cond[2] = 1;
    }

    if (missionAvailData[mission].availRating <= 0)
        cond[3] = 1;
    else if (playerKills >= missionAvailData[mission].availRating)
        cond[3] = 1;

    if (missionAvailData[mission].availRandom >= kDailyRollRange)
        cond[4] = 1;
    else if (missionRandom[mission] <= missionAvailData[mission].availRandom)
        cond[4] = 1;

    if (offeringFromShip != 0) {
        if (missionAvailData[mission].cargoQty <= 0)
            cond[5] = 1;
        else if (TotalMissionCargoSpace() < missionAvailData[mission].cargoQty)
            cond[5] = 0;
        else
            cond[5] = 1;
    } else if ((missionAvailData[mission].flags2 & kMissionNeedsCargoSpace) != 0) {
        if (TotalMissionCargoSpace() < missionAvailData[mission].cargoQty)
            cond[5] = 0;
        else
            cond[5] = 1;
    } else {
        cond[5] = 1;
    }

    if (forceRandom != 0)
        cond[4] = 1;

    if (missionAvailData[mission].availShipType >= kFirstResourceID
        && missionAvailData[mission].availShipType <= kFirstResourceID + kMaxShipTypes) {
        if (shipData->type == missionAvailData[mission].availShipType - kFirstResourceID)
            cond[7] = 1;
    } else if (missionAvailData[mission].availShipType >= ResClassBase(kReqNotShipType)
               && missionAvailData[mission].availShipType
                  <= ResClassBase(kReqNotShipType) + kMaxShipTypes) {
        if (shipData->type != missionAvailData[mission].availShipType - ResClassBase(kReqNotShipType))
            cond[7] = 1;
    } else if (missionAvailData[mission].availShipType >= ResClassBase(kReqShipGovt)
               && missionAvailData[mission].availShipType <= ResClassBase(kReqShipGovt) + kMaxGovts) {
        if (shipClass[shipData->type].inherentGovt
                == missionAvailData[mission].availShipType - ResClassBase(kReqShipGovt)
            || shipClass[shipData->type].inherentGovtB
                == missionAvailData[mission].availShipType - ResClassBase(kReqShipGovt))
            cond[7] = 1;
    } else if (missionAvailData[mission].availShipType >= ResClassBase(kReqNotShipGovt)
               && missionAvailData[mission].availShipType <= ResClassBase(kReqNotShipGovt) + kMaxGovts) {
        if (shipClass[shipData->type].inherentGovt
                != missionAvailData[mission].availShipType - ResClassBase(kReqNotShipGovt)
            && shipClass[shipData->type].inherentGovtB
                != missionAvailData[mission].availShipType - ResClassBase(kReqNotShipGovt))
            cond[7] = 1;
    } else {
        cond[7] = 1;
    }

    cond[8] = 1;
    if ((missionAvailData[mission].flags & kMissionNotForFreighters) != 0
        && shipClass[shipData->type].inherentAI <= kLargeShipClass - 1)
        cond[8] = 0;
    if ((missionAvailData[mission].flags & kMissionNotForWarships) != 0
        && shipClass[shipData->type].inherentAI >= kLargeShipClass)
        cond[8] = 0;

    if (missionAvailData[mission].payment < kCostFloor) {
        if (shipData->credits >= kCostFloor - missionAvailData[mission].payment)
            cond[9] = 1;
        else
            cond[9] = 0;
    } else {
        cond[9] = 1;
    }

    for (n = 0; n < kMissionConds; n++) {
        if (cond[n] == 0)
            return 0;
    }

    if ((missionAvailData[mission].sourceStellar == -2
         || missionAvailData[mission].sourceStellar == -3
         || missionAvailData[mission].sourceStellar > 0)
        && SelectMissionStellar(missionAvailData[mission].sourceStellar,
                                -1, -1, mission) == -1)
        return 0;
    if ((missionAvailData[mission].destStellar == -2
         || missionAvailData[mission].destStellar == -3
         || missionAvailData[mission].destStellar > 0)
        && SelectMissionStellar(missionAvailData[mission].destStellar,
                                -1, -1, mission) == -1)
        return 0;

    for (n = 0; n < kMaxActiveMissions; n++) {
        if (currentMissionObjectives[n].active != 0
            && currentMissions[n].misnIndex == mission)
            return 0;
    }
    return 1;
}

// FUNCTION: EVNOVA 0x00443760
// MAC: _MissionHandler i386 0x0009eda8 ppc 0x000aab58
void MissionHandler(void)
{
    short i;

    for (i = 0; i < kMaxActiveMissions; i++)
        MissionObjectivesCheck(i);
}

// FUNCTION: EVNOVA 0x00443780
// MAC: _MissionLandCheck i386 0x000a19cc ppc 0x000ad724
void MissionLandCheck(void)
{
    short i;
    short stel;
    Boolean anySuccess;

    anySuccess = 0;
    lastPortMissionLoc = -1;
    stel = shipData[0].targetStellar;
    for (i = 0; i < kMaxActiveMissions; i++) {
        if (currentMissionObjectives[i].active != 0) {
            MissionObjectivesCheck(i);
            MissionLandCargoCheck(i, stel);
            if (StellarsAreIdentical(currentMissions[i].returnStellar, stel)) {
                if (currentMissions[i].returnStellar == currentMissions[i].travelStellar)
                    currentMissionObjectives[i].returnDone = 1;
                if (currentMissions[i].travelStellar == -1)
                    currentMissionObjectives[i].returnDone = 1;
                if (currentMissionObjectives[i].failed != 0) {
                    DoMissionFailure(i);
                } else {
                    if (currentMissions[i].shipGoal == kMissionGoalEscort && currentMissions[i].specialShipsKilled <= 0)
                        currentMissionObjectives[i].objectivesDone = 1;
                    if (currentMissionObjectives[i].returnDone != 0
                            && currentMissionObjectives[i].objectivesDone != 0) {
                        DoMissionSuccess(i);
                        anySuccess = 1;
                    }
                }
            }
            MissionObjectivesCheck(i);
        }
    }
    PropagateMissionBitEffects();
    if (anySuccess)
        RandomizeMissionData();
}

// FUNCTION: EVNOVA 0x004438d0
// MAC: _MissionLandCargoCheck i386 0x0009edc5 ppc 0x000aab94
void MissionLandCargoCheck(short slot, short stel)
{
    if (StellarsAreIdentical(currentMissions[slot].travelStellar, stel)) {
        if (currentMissions[slot].pickupMode == 1 && currentMissions[slot].hasCargo == 0) {
            if (DoMissionCargoPickup(currentMissions[slot].cargoType, currentMissions[slot].cargoQty)) {
                currentMissions[slot].hasCargo = 1;
                currentMissionObjectives[slot].returnDone = 1;
                crashlog_mission_bit_set("doing mission cargo pickup",
                                         currentMissions[slot].name, 1,
                                         currentMissions[slot].misnIndex + kFirstResourceID, "");
                if (currentMissions[slot].loadCargoDesc != -1) {
                    ShowTheCursor();
                    strncpy(briefingTextPtr, "", 3);
                    LoadDescription(briefingTextPtr, currentMissions[slot].loadCargoDesc);
                    MungeBriefing(0, slot);
                    strncpy(textShowString, briefingTextPtr, kBriefingTextMax);
                    ShowTextDialog(0, 1);
                }
            }
        } else {
            currentMissionObjectives[slot].returnDone = 1;
        }
        if (currentMissions[slot].dropoffMode == 0 && currentMissions[slot].hasCargo != 0) {
            currentMissions[slot].hasCargo = 0;
            crashlog_mission_bit_set("doing mission cargo dropoff",
                                     currentMissions[slot].name, 1,
                                     currentMissions[slot].misnIndex + kFirstResourceID, "");
            if (currentMissions[slot].dropCargoDesc != -1) {
                ShowTheCursor();
                strncpy(briefingTextPtr, "", 3);
                LoadDescription(briefingTextPtr, currentMissions[slot].dropCargoDesc);
                MungeBriefing(0, slot);
                strncpy(textShowString, briefingTextPtr, kBriefingTextMax);
                ShowTextDialog(0, 1);
                ResetPlayerPrecalcedValues();
            }
            statusRedraw[kRedrawCargo] = 1;
        }
    }
    if (StellarsAreIdentical(currentMissions[slot].returnStellar, stel)) {
        if (currentMissions[slot].dropoffMode == 1 && currentMissions[slot].hasCargo != 0) {
            if (currentMissionObjectives[slot].objectivesDone != 0 || currentMissions[slot].shipGoal == kMissionGoalNone
                    || (currentMissions[slot].shipGoal == kMissionGoalEscort && currentMissions[slot].specialShipsKilled <= 0)) {
                currentMissions[slot].hasCargo = 0;
                crashlog_mission_bit_set("doing mission cargo dropoff",
                                         currentMissions[slot].name, 1,
                                         currentMissions[slot].misnIndex + kFirstResourceID, "");
                if (currentMissions[slot].dropCargoDesc != -1) {
                    ShowTheCursor();
                    strncpy(briefingTextPtr, "", 3);
                    LoadDescription(briefingTextPtr, currentMissions[slot].dropCargoDesc);
                    MungeBriefing(0, slot);
                    strncpy(textShowString, briefingTextPtr, kBriefingTextMax);
                    ShowTextDialog(0, 1);
                }
                statusRedraw[kRedrawCargo] = 1;
            }
        }
    }
}

// FUNCTION: EVNOVA 0x00443c60
// MAC: _MissionObjectivesCheck i386 0x0009e79e ppc 0x000aa434
void MissionObjectivesCheck(short slot)
{
    Str255 buf;
    Rect vis;
    Rect shipRect;
    Boolean prevDone;
    short i;
    short count;
    short syst;

    if (currentMissionObjectives[slot].active == 0)
        return;
    if (currentMissions[slot].shipGoal == kMissionGoalNone && currentMissions[slot].shipCount > 0
            && currentMissions[slot].specialShipsJumpedIn <= 0
            && (currentMissions[slot].flags & kMissionAutoAbort) != 0)
        currentMissionObjectives[slot].objectivesDone = 0;
    prevDone = currentMissionObjectives[slot].objectivesDone;
    if (currentMissions[slot].shipGoal == kMissionGoalNone && currentMissions[slot].initialShipCount <= 0) {
        currentMissionObjectives[slot].objectivesDone = 1;
    } else if (currentMissions[slot].initialShipCount > 0) {
        if (currentMissions[slot].shipGoal == kMissionGoalNone) {
            if ((currentMissions[slot].flags & kMissionAutoAbort) != 0) {
                if (currentMissions[slot].specialShipsJumpedIn > 0
                        || currentMissions[slot].shipStart != 1)
                    currentMissionObjectives[slot].objectivesDone = 1;
            } else {
                currentMissionObjectives[slot].objectivesDone = 1;
            }
        } else if (currentMissions[slot].specialShipsJumpedIn >= currentMissions[slot].initialShipCount
                || currentMissions[slot].shipCount == 0) {
            if (currentMissions[slot].shipGoal == kMissionGoalDestroy
                    && currentMissions[slot].specialShipsKilled >= currentMissions[slot].initialShipCount)
                currentMissionObjectives[slot].objectivesDone = 1;
            if (currentMissions[slot].shipGoal == kMissionGoalDisable) {
                if (currentMissions[slot].specialShipsKilled > 0)
                    currentMissionObjectives[slot].failed = 1;
                else if (currentMissions[slot].specialShipsDisabled >= currentMissions[slot].initialShipCount)
                    currentMissionObjectives[slot].objectivesDone = 1;
            }
            if ((currentMissions[slot].shipGoal == kMissionGoalBoard || currentMissions[slot].shipGoal == kMissionGoalRescue)
                    && currentMissions[slot].specialShipsBoarded >= currentMissions[slot].initialShipCount)
                currentMissionObjectives[slot].objectivesDone = 1;
            if (currentMissions[slot].shipGoal == kMissionGoalEscort) {
                if (currentMissions[slot].specialShipsKilled > 0
                        || currentMissions[slot].specialShipsDisabled > 0) {
                    currentMissionObjectives[slot].failed = 1;
                } else {
                    count = 0;
                    for (i = 1; i < kMaxShips; i++) {
                        if (slot == shipData[i].specialMission
                                && shipData[i].active != 0)
                            count++;
                    }
                    if (count == 0)
                        currentMissionObjectives[slot].objectivesDone = 0;
                    else
                        currentMissionObjectives[slot].objectivesDone = 1;
                }
            }
            if (currentMissions[slot].shipGoal == kMissionGoalObserve
                    && currentMissionObjectives[slot].objectivesDone == 0
                    && gPause == 0) {
                syst = currentMissions[slot].shipSyst;
                if (syst >= 0 && syst < kMaxSystems)
                    syst = FindActiveCoLocatedSystem(syst);
                else
                    syst = -1;
                if ((syst == shipData[0].curSystem
                        || currentMissions[slot].shipSyst == -6)
                        && currentMissions[slot].specialShipsJumpedIn >= currentMissions[slot].shipCount) {
                    for (i = 1; i < kMaxShips; i++) {
                        if (shipData[i].active != 0
                                && slot == shipData[i].specialMission
                                && shipData[i].sprite != 0
                                && ShipVisibleToShip(&shipData[i], shipData) != 0) {
                            if (ShipCanCloak(&shipData[i]) != 0) {
                                vis = gSpriteWorld->windRect;
                                shipRect = shipData[i].sprite->destFrameRect;
                                InsetRect(&vis,
                                          ((shipRect.right - shipRect.left) * 0.75),
                                          ((shipRect.bottom - shipRect.top) * 0.75));
                                if (SectRect(&vis, &shipRect, &shipRect) != 0) {
                                    currentMissionObjectives[slot].objectivesDone = 1;
                                    break;
                                }
                            } else {
                                currentMissionObjectives[slot].objectivesDone = 1;
                                break;
                            }
                        }
                    }
                }
            }
            if (currentMissions[slot].shipGoal == kMissionGoalChaseOff) {
                if (currentMissions[slot].specialShipsKilled + currentMissions[slot].specialShipsJumpedOut
                        >= currentMissions[slot].initialShipCount)
                    currentMissionObjectives[slot].objectivesDone = 1;
                else
                    currentMissionObjectives[slot].objectivesDone = 0;
            }
        } else {
            currentMissionObjectives[slot].objectivesDone = 0;
        }
    } else {
        currentMissionObjectives[slot].objectivesDone = 1;
    }
    if (currentMissionObjectives[slot].failed == 0 && currentMissions[slot].timeLimit <= 0
            && currentMissions[slot].timeLimit > -32000 && portDialog == 0) {
        currentMissionObjectives[slot].failed = 1;
        if ((currentMissionObjectives[slot].flags & kMissionHidden) == 0) {
            PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);
            GetIndString(buf, kGameStrList, 285);
            DisplayComm((PStrChar *)buf, kCommFrames);
        }
        QuickMissionFailure(slot);
    }
    if (currentMissionObjectives[slot].failed == 0
            && currentMissionObjectives[slot].objectivesDone != 0
            && prevDone != currentMissionObjectives[slot].objectivesDone) {
        if (currentMissions[slot].shipDoneDesc != -1) {
            ShowTheCursor();
            strncpy(briefingTextPtr, "", 3);
            LoadDescription(briefingTextPtr, currentMissions[slot].shipDoneDesc);
            MungeBriefing(0, slot);
            strncpy(textShowString, briefingTextPtr, kBriefingTextMax);
            ShowTextDialog(0, 1);
        }
        if (currentMissions[slot].shipGoal != kMissionGoalNone) {
            crashlog_mission_bit_set("mission ship goal complete",
                                     currentMissions[slot].name, 1,
                                     currentMissions[slot].misnIndex + kFirstResourceID,
                                     currentMissions[slot].shipDoneSetString);
            EvalCurrentMissionBitSetString(currentMissions[slot].shipDoneSetString, slot);
        }
        if ((currentMissions[slot].flags & kMissionAutoAbort) != 0)
            AutoAbortMission(slot);
    }
}

// FUNCTION: EVNOVA 0x00447a30
// MAC: _ValidAuxShipSystem i386 0x000992b5 ppc 0x000a4e60
Boolean ValidAuxShipSystem(short sys, short mission)
{
    short spec;
    short i;

    spec = currentMissions[mission].auxShipSyst;
    if (spec == -1 || spec == -6) {
        if (sys == shipData->curSystem)
            return 1;
    } else if (spec == -2) {
        if (currentMissions[mission].travelStellar != -1) {
            if (sys == stellarData[currentMissions[mission].travelStellar].system)
                return 1;
        }
    } else if (spec == -3) {
        if (currentMissions[mission].returnStellar != -1) {
            if (sys == stellarData[currentMissions[mission].returnStellar].system)
                return 1;
        }
    } else if (spec >= kFirstResourceID && spec < kFirstResourceID + kMaxSystems) {
        if (sys == spec - kFirstResourceID)
            return 1;
        if (sys == FindActiveCoLocatedSystem(spec - kFirstResourceID))
            return 1;
    } else if (spec >= kAvailLinkBase && spec < kSystemGovt - 1) {
        if (sys == spec - kAvailLinkBase)
            return 1;
        for (i = 0; i < kNavSlots; i++) {
            if (sys == systemData[spec - kAvailLinkBase].links[i])
                return 1;
        }
    } else if (spec >= kSystemGovt - 1 && spec < kSystemAllied) {
        if (systemData[sys].govt == spec - kSystemGovt)
            return 1;
    } else if (spec >= kSystemAllied && spec < kSystemNotGovt) {
        if (systemData[sys].govt >= 0) {
            if (systemData[sys].govt == spec - kSystemAllied)
                return 1;
            if (GovtAllies(systemData[sys].govt, spec - kSystemAllied))
                return 1;
        }
    } else if (spec >= kSystemNotGovt && spec < kSystemEnemy) {
        if (systemData[sys].govt != spec - kSystemNotGovt)
            return 1;
    } else if (spec >= kSystemEnemy && spec < kSystemClass) {
        if (systemData[sys].govt >= 0) {
            if ((govtData[spec - kSystemEnemy].flags & kGovtXenophobic) > 0U && systemData[sys].govt != spec - kSystemEnemy &&
                !GovtAllies(systemData[sys].govt, spec - kSystemEnemy))
                return 1;
            if (GovtEnemies(systemData[sys].govt, spec - kSystemEnemy))
                return 1;
        }
    } else if (spec >= kSystemClass && spec < kSystemNotClass) {
        if (systemData[sys].govt >= 0) {
            if (GovtSharedClass(systemData[sys].govt, spec - kSystemClass))
                return 1;
        }
    } else if (spec >= kSystemNotClass && spec < kSystemBandLimit) {
        if (systemData[sys].govt >= 0) {
            if (!GovtSharedClass(systemData[sys].govt, spec - kSystemNotClass))
                return 1;
        }
    }
    return 0;
}

// FUNCTION: EVNOVA 0x00447d90
// MAC: _AutoAbortMission i386 0x00099bdc ppc 0x000a5954
void AutoAbortMission(short mission)
{
    short i;

    crashlog_mission_bit_set("auto-aborting mission", currentMissions[mission].name, 1,
                             currentMissions[mission].misnIndex + kFirstResourceID,
                             currentMissions[mission].abortSetString);
    EvalCurrentMissionBitSetString(currentMissions[mission].abortSetString, mission);
    if (currentMissions[mission].datePostIncrement > 0) {
        crashlog_print_mdy(&gameTime);
        crashlog_print(": incrementing date ");
        crashlog_print_num(currentMissions[mission].datePostIncrement);
        crashlog_print(" days for mission ");
        crashlog_print_num(currentMissions[mission].misnIndex + kFirstResourceID);
        crashlog_print(" auto-abort post increment\n");
        for (i = 0; i < currentMissions[mission].datePostIncrement; i++)
            IncrementGameTime();
    }
    if (currentMissions[mission].flags & kMissionTakesFuel) {
        shipData[0].fuel -= 100.0f;
        statusRedraw[kRedrawFuel] = 1;
    }
    if (currentMissions[mission].flags2 & kMissionPaysOnAbort)
        ApplyMissionPay(currentMissions[mission].payment);
    AbortMission(mission, 0);
}

// FUNCTION: EVNOVA 0x00447f00
// MAC: _SystemVisible i386 0x00098207 ppc 0x000a3ea8
Boolean SystemVisible(short sys)
{
    return systemData[sys].loaded;
}

// FUNCTION: EVNOVA 0x00447f20
// MAC: _EvalMissionBitTestString i386 0x0009959e ppc 0x000a5160
Boolean EvalMissionBitTestString(char *expr)
{
    short i;
    short out;
    short len;
    char prev;
    short err;

    if (strlen(expr) == 0)
        return 1;
    if (expr[0] != 'b' && expr[0] != 'B' && expr[0] != '(' && expr[0] != '!'
            && expr[0] != 'P' && expr[0] != 'p' && expr[0] != 'G' && expr[0] != 'g'
            && expr[0] != 'O' && expr[0] != 'o' && expr[0] != 'E' && expr[0] != 'e')
        return 0;
    len = strlen(expr);
    prev = ' ';
    out = 0;
    for (i = 0; i < len; i++) {
        if (expr[i] == '(' && prev == '(') {
            parseBuffer[out] = ' ';
            out++;
        }
        parseBuffer[out] = expr[i];
        prev = expr[i];
        out++;
    }
    parseBuffer[out] = 0;
    err = 0;
    return (EvalTestExp(&err) == 1) ? true : false;
}
