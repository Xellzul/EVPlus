/* briefingtext.cpp -- briefing text token substitution */

#include "game.h"
#include "gameutil.h"
#include "player.h"
#include "missions.h"
#include "stellar.h"
#include "prototypes.h"

#define kBriefingSize   0x2000
#define kNameLen        0x3f    /* strncpy budget for a 64-byte token buffer */
#define kNoRankIndex    341
#define kOwedBase       50000   /* pay below -50000 is a debt of the excess */

// FUNCTION: EVNOVA 0x004444f0
// MAC: _MungeBriefing i386 0x0009c11d ppc 0x000a81c0
void MungeBriefing(Boolean briefing, short which)
{
    char tokenStr[64];
    char destStelStr[64];
    char destSysStr[64];
    char retStelStr[64];
    char retSysStr[64];
    char cargoStr[64];
    char cargoQtyStr[64];
    char shipMsgStr[64];
    char deadlineStr[64];
    char playerNameStr[64];
    char nickStr[64];
    char playerShipStr[64];
    char personStr[64];
    char shipTypeStr[64];
    char prkStr[64];
    char srkStr[64];
    char payStr[64];
    char rank3Str[64];
    char rank4Str[64];
    char rrkStr[128];
    Str255 numStr;
    short i;
    short sys;
    short bestIdx;
    short bestVal;
    int32_t pay;
    int32_t pct;

    stringHandle = NULL;
    stringHandle = (Handle)AllocAHandle2(strlen(briefingTextPtr), 0);
    if (stringHandle == NULL)
        return;
    memmove(*stringHandle, briefingTextPtr, GetAHandleSize(stringHandle));
    for (i = 0; i < kBriefingSize; i++)
        briefingTextPtr[i] = '\0';

    strncpy(destStelStr, "[Error]", 10);
    strncpy(destSysStr, "[Error]", 10);
    strncpy(retStelStr, "[Error]", 10);
    strncpy(retSysStr, "[Error]", 10);
    strncpy(cargoStr, "[Error]", 10);
    strncpy(cargoQtyStr, "[Error]", 10);
    strncpy(shipMsgStr, "[Error]", 10);
    strncpy(deadlineStr, "[Error]", 10);
    strncpy(personStr, "[Error]", 10);
    strncpy(shipTypeStr, "[Error]", 10);
    strncpy(prkStr, "[Error]", 10);
    strncpy(srkStr, "[Error]", 10);
    strncpy(rank3Str, "[Error]", 10);
    strncpy(rank4Str, "[Error]", 10);
    strncpy(rrkStr, "[Error]", 10);
    strncpy(payStr, "[Error]", 10);

    if (briefing != 0) {
        if (which == -1)
            goto tokens;

        if (briefMissionInfo[which].destStellar >= 0 &&
            briefMissionInfo[which].destStellar < kMaxStellars) {
            strncpy(destStelStr,
                    (char *)stellarData[briefMissionInfo[which].destStellar].name, kNameLen);
            PtoCstr((unsigned char *)destStelStr);
        }
        if (briefMissionInfo[which].destSystem >= 0 &&
            briefMissionInfo[which].destSystem < kMaxSystems) {
            sys = FindActiveCoLocatedSystem(briefMissionInfo[which].destSystem);
            if (sys == -1)
                sys = FindFirstCoLocatedSystem(briefMissionInfo[which].destSystem);
            if (sys != -1) {
                strncpy(destSysStr, (char *)systemData[sys].name, kNameLen);
                PtoCstr((unsigned char *)destSysStr);
            }
        }
        if (briefMissionInfo[which].returnStellar >= 0 &&
            briefMissionInfo[which].returnStellar < kMaxStellars) {
            strncpy(retStelStr,
                    (char *)stellarData[briefMissionInfo[which].returnStellar].name, kNameLen);
            PtoCstr((unsigned char *)retStelStr);
        }
        if (briefMissionInfo[which].returnSystem >= 0 &&
            briefMissionInfo[which].returnSystem < kMaxSystems) {
            sys = FindActiveCoLocatedSystem(briefMissionInfo[which].returnSystem);
            if (sys == -1)
                sys = FindFirstCoLocatedSystem(briefMissionInfo[which].returnSystem);
            if (sys != -1) {
                strncpy(retSysStr, (char *)systemData[sys].name, kNameLen);
                PtoCstr((unsigned char *)retSysStr);
            }
        }

        if (strcmp(destStelStr, "[Error]") == 0) {
            if (strcmp(retStelStr, "[Error]") != 0)
                strcpy(destStelStr, retStelStr);
        }
        if (strcmp(destSysStr, "[Error]") == 0) {
            if (strcmp(retSysStr, "[Error]") != 0)
                strcpy(destSysStr, retSysStr);
        }

        if (briefMissionInfo[which].cargoType >= 0 &&
            briefMissionInfo[which].cargoType < kMaxCargo) {
            strncpy(cargoStr, (char *)lcCargoName[briefMissionInfo[which].cargoType], kNameLen);
            PtoCstr((unsigned char *)cargoStr);
            if (cargoStr[0] == '*') {
                sys = strlen(cargoStr);
                for (i = 0; i < sys; i++)
                    cargoStr[i] = cargoStr[i + 1];
            }
            NumToString(briefMissionInfo[which].cargoUnits, numStr);
            strncpy(cargoQtyStr, (char *)numStr, kNameLen);
            PtoCstr((unsigned char *)cargoQtyStr);
        }

        if (briefMissionInfo[which].dueDate.month != gameTime.month ||
            briefMissionInfo[which].dueDate.day != gameTime.day ||
            briefMissionInfo[which].dueDate.year != gameTime.year)
            DateToLongString(briefMissionInfo[which].dueDate, deadlineStr);

        pay = 0;
        if (briefMissionInfo[which].payment > 0) {
            pay = briefMissionInfo[which].payment;
        } else if (briefMissionInfo[which].payment <= -(kPctOfCashBase + 1) &&
                   briefMissionInfo[which].payment > -(kPctOfCashBase + 100)) {
            pct = abs(briefMissionInfo[which].payment);
            pct -= kPctOfCashBase;
            pay = (shipData[0].credits * (pct * 0.01));
        } else if (briefMissionInfo[which].payment < -kOwedBase) {
            pay = -kOwedBase - briefMissionInfo[which].payment;
        }
        if (pay >= 0) {
            payStr[0] = '\0';
            StrcatCommaNumber(pay, payStr);
        }
    } else {
        if (which == -1)
            goto tokens;
        if (currentMissionObjectives[which].active == 0)
            goto tokens;

        if (currentMissions[which].travelStellar >= 0 &&
            currentMissions[which].travelStellar < kMaxStellars) {
            strncpy(destStelStr,
                    (char *)stellarData[currentMissions[which].travelStellar].name, kNameLen);
            PtoCstr((unsigned char *)destStelStr);
            if (stellarData[currentMissions[which].travelStellar].system >= 0 &&
                stellarData[currentMissions[which].travelStellar].system < kMaxSystems) {
                sys = FindActiveCoLocatedSystem(
                          stellarData[currentMissions[which].travelStellar].system);
                if (sys == -1)
                    sys = FindFirstCoLocatedSystem(
                              stellarData[currentMissions[which].travelStellar].system);
            } else {
                sys = -1;
            }
            if (sys == -1)
                sys = FindSystemFromStellar(currentMissions[which].travelStellar);
            if (sys != -1) {
                strncpy(destSysStr, (char *)systemData[sys].name, kNameLen);
                PtoCstr((unsigned char *)destSysStr);
            }
        }
        if (currentMissions[which].returnStellar >= 0 &&
            currentMissions[which].returnStellar < kMaxStellars) {
            strncpy(retStelStr,
                    (char *)stellarData[currentMissions[which].returnStellar].name, kNameLen);
            PtoCstr((unsigned char *)retStelStr);
            if (stellarData[currentMissions[which].returnStellar].system >= 0 &&
                stellarData[currentMissions[which].returnStellar].system < kMaxSystems) {
                sys = FindActiveCoLocatedSystem(
                          stellarData[currentMissions[which].returnStellar].system);
                if (sys == -1)
                    sys = FindFirstCoLocatedSystem(
                              stellarData[currentMissions[which].returnStellar].system);
            } else {
                sys = -1;
            }
            if (sys == -1)
                sys = FindSystemFromStellar(currentMissions[which].returnStellar);
            if (sys != -1) {
                strncpy(retSysStr, (char *)systemData[sys].name, kNameLen);
                PtoCstr((unsigned char *)retSysStr);
            }
        }

        if (strcmp(destStelStr, "[Error]") == 0) {
            if (strcmp(retStelStr, "[Error]") != 0)
                strcpy(destStelStr, retStelStr);
        }
        if (strcmp(destSysStr, "[Error]") == 0) {
            if (strcmp(retSysStr, "[Error]") != 0)
                strcpy(destSysStr, retSysStr);
        }

        if (currentMissions[which].cargoType >= 0 &&
            currentMissions[which].cargoType < kMaxCargo) {
            strncpy(cargoStr, (char *)lcCargoName[currentMissions[which].cargoType], kNameLen);
            PtoCstr((unsigned char *)cargoStr);
            if (cargoStr[0] == '*') {
                sys = strlen(cargoStr);
                for (i = 0; i < sys; i++)
                    cargoStr[i] = cargoStr[i + 1];
            }
            NumToString(currentMissions[which].cargoQty, numStr);
            strncpy(cargoQtyStr, (char *)numStr, kNameLen);
            PtoCstr((unsigned char *)cargoQtyStr);
        }

        if (currentMissionObjectives[which].dueDate.month != gameTime.month ||
            currentMissionObjectives[which].dueDate.day != gameTime.day ||
            currentMissionObjectives[which].dueDate.year != gameTime.year)
            DateToLongString(currentMissionObjectives[which].dueDate, deadlineStr);

        strncpy(shipMsgStr, currentMissions[which].shipName, kNameLen);
        PtoCstr((unsigned char *)shipMsgStr);

        if (currentMissions[which].payment > 0) {
            pay = currentMissions[which].payment;
        } else if (currentMissions[which].payment <= -(kPctOfCashBase + 1) &&
                   currentMissions[which].payment > -(kPctOfCashBase + 100)) {
            pct = abs(currentMissions[which].payment);
            pct -= kPctOfCashBase;
            pay = (shipData[0].credits * (pct * 0.01));
        } else if (currentMissions[which].payment < -kOwedBase) {
            pay = -kOwedBase - currentMissions[which].payment;
        }
        if (pay >= 0) {
            payStr[0] = '\0';
            StrcatCommaNumber(pay, payStr);
        }
    }

tokens:
    if (offeringShipIndex >= 0 && offeringShipIndex < kMaxShips &&
        (shipData[offeringShipIndex].persType >= 0 &&
         shipData[offeringShipIndex].persType < kMaxPersons)) {
        strncpy(personStr,
                personData[shipData[offeringShipIndex].persType].name, kNameLen);
        PtoCstr((unsigned char *)personStr);
    }

    strncpy(playerNameStr, (char *)playerName, kNameLen);
    PtoCstr((unsigned char *)playerNameStr);
    if (playerNickname[0] != 0) {
        strncpy(nickStr, (char *)playerNickname, kNameLen);
        PtoCstr((unsigned char *)nickStr);
    } else {
        strncpy(nickStr, (char *)playerName, kNameLen);
    }
    strncpy(playerShipStr, (char *)shipName, kNameLen);
    PtoCstr((unsigned char *)playerShipStr);
    strncpy(shipTypeStr, (char *)shipClass[shipData[0].type].name, kNameLen);
    PtoCstr((unsigned char *)shipTypeStr);

    bestIdx = -1;
    for (i = 0; i < kMaxRanks; i++) {
        if (rankData[i].active != 0 && rankData[i].loaded != 0 &&
            strlen(rankData[i].convName) != 0 &&
            (rankData[i].priority > bestVal || bestIdx == -1)) {
            bestIdx = i;
            bestVal = rankData[i].priority;
        }
    }
    if (bestIdx != -1) {
        strcpy(prkStr, rankData[bestIdx].convName);
    } else {
        strcpy(prkStr, "");
        StrcatIndCString(prkStr, kGameStrList, kNoRankIndex);
    }

    bestIdx = -1;
    for (i = 0; i < kMaxRanks; i++) {
        if (rankData[i].active != 0 && rankData[i].loaded != 0 &&
            strlen(rankData[i].shortName) != 0 &&
            (rankData[i].priority > bestVal || bestIdx == -1)) {
            bestIdx = i;
            bestVal = rankData[i].priority;
        }
    }
    if (bestIdx != -1) {
        strcpy(srkStr, rankData[bestIdx].shortName);
    } else {
        strcpy(srkStr, "");
        StrcatIndCString(srkStr, kGameStrList, kNoRankIndex);
    }

    bestIdx = -1;
    if (descRankGovt[0] != -1) {
        for (i = 0; i < kMaxRanks; i++) {
            if (rankData[i].active != 0 && rankData[i].loaded != 0 &&
                descRankGovt[0] == rankData[i].govt &&
                strlen(rankData[i].shortName) != 0 &&
                (rankData[i].priority > bestVal || bestIdx == -1)) {
                bestIdx = i;
                bestVal = rankData[i].priority;
            }
        }
    }
    if (bestIdx != -1) {
        strcpy(rank3Str, rankData[bestIdx].shortName);
    } else {
        strcpy(rank3Str, "");
        StrcatIndCString(rank3Str, kGameStrList, kNoRankIndex);
    }

    bestIdx = -1;
    if (descRankGovt[1] != -1) {
        for (i = 0; i < kMaxRanks; i++) {
            if (rankData[i].active != 0 && rankData[i].loaded != 0 &&
                descRankGovt[1] == rankData[i].govt &&
                strlen(rankData[i].shortName) != 0 &&
                (rankData[i].priority > bestVal || bestIdx == -1)) {
                bestIdx = i;
                bestVal = rankData[i].priority;
            }
        }
    }
    if (bestIdx != -1) {
        strcpy(rank4Str, rankData[bestIdx].shortName);
    } else {
        strcpy(rank4Str, "");
        StrcatIndCString(rank4Str, kGameStrList, kNoRankIndex);
    }

    if (recentActivatedRank != -1) {
        strcpy(rrkStr, rankData[recentActivatedRank].name);
    } else {
        strcpy(rrkStr, "");
        StrcatIndCString(rrkStr, kGameStrList, kNoRankIndex);
    }

    MungeForSymbol("<DST>", destStelStr);
    MungeForSymbol("<DSY>", destSysStr);
    MungeForSymbol("<RST>", retStelStr);
    MungeForSymbol("<RSY>", retSysStr);
    MungeForSymbol("<CT>", cargoStr);
    MungeForSymbol("<CQ>", cargoQtyStr);
    MungeForSymbol("<SN>", shipMsgStr);
    MungeForSymbol("<DL>", deadlineStr);
    MungeForSymbol("<PN>", playerNameStr);
    MungeForSymbol("<PNN>", nickStr);
    MungeForSymbol("<PSN>", playerShipStr);
    MungeForSymbol("<PST>", shipTypeStr);
    MungeForSymbol("<OSN>", personStr);
    MungeForSymbol("<PRK>", prkStr);
    MungeForSymbol("<SRK>", srkStr);
    MungeForSymbol("<RRK>", rrkStr);
    MungeForSymbol("<PAY>", payStr);
    MungeForSymbol("<REG>", kRegisteredName);

    if (descRankGovt[0] != -1) {
        sprintf(tokenStr, "<PRK%i>", descRankGovt[0] + kFirstResourceID);
        MungeForSymbol(tokenStr, rank4Str);
    }
    if (descRankGovt[1] != -1) {
        sprintf(tokenStr, "<SRK%i>", descRankGovt[1] + kFirstResourceID);
        MungeForSymbol(tokenStr, rank3Str);
    }

    if (leetMode != 0) {
        MungeForSymbol("you", "j00");
        MungeForSymbol("You", "j00");
        MungeForSymbol("ck", "x0r");
        MungeForSymbol("i", "1");
        MungeForSymbol("I", "1");
        MungeForSymbol("e", "3");
        MungeForSymbol("E", "3");
        MungeForSymbol("l", "7");
        MungeForSymbol("L", "7");
    }

    if (GetAHandleSize(stringHandle) <= kBriefingSize) {
        memmove(briefingTextPtr, *stringHandle, GetAHandleSize(stringHandle));
    } else {
        memmove(briefingTextPtr, *stringHandle, kBriefingSize);
    }
    DeallocAHandle(stringHandle);
    stringHandle = NULL;
}

// FUNCTION: EVNOVA 0x00445d70
// MAC: _MungeForSymbol i386 0x000984a3 ppc 0x000a4230
void MungeForSymbol(char *needle, char *replacement)
{
    short needleLen;
    short replLen;
    short at;

    needleLen = strlen(needle);
    replLen = strlen(replacement);
    at = 0;
    do {
        at = ReplaceInHandle(stringHandle, at, needle, needleLen, replacement, replLen);
    } while (at >= 0);
}
