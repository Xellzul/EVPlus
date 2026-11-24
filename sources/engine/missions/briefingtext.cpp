/* briefingtext.cpp -- briefing text token substitution */

#include <algorithm>
#include "game.h"
#include "gameutil.h"
#include "player.h"
#include "missions.h"
#include "stellar.h"
#include "prototypes.h"
#include "text.h"

#define kNameLen        0x3f    /* name tokens: PascalToCBuffer size (62 chars), <PNN> cut */
#define kTokenMax       63      /* other tokens from 64-byte fields */
#define kRecentRankMax  127
#define kNoRankIndex    341
#define kOwedBase       50000   /* pay below -50000 is a debt of the excess */

static void MungeForSymbol(std::string &text, const char *needle, const std::string &replacement);

/* a Pascal name as a token: at most 62 bytes, up to a NUL */
static std::string NameToken(const unsigned char *name)
{
    char buf[kNameLen];

    PascalToCBuffer(buf, sizeof buf, name);
    return buf;
}

static std::string NameToken(const std::string &name)
{
    return UpToNul(name, kNameLen - 1);
}

/* a C string field of srcSize bytes as a token: up to its NUL, at most max bytes */
static std::string FieldToken(const char *src, size_t srcSize, size_t max)
{
    return std::string(src, strnlen(src, std::min(srcSize, max)));
}

// FUNCTION: EVNOVA 0x004444f0
// MAC: _MungeBriefing i386 0x0009c11d ppc 0x000a81c0
void MungeBriefing(Boolean briefing, short which)
{
    std::string text;
    std::string tokenStr;
    std::string destStelStr = "[Error]";
    std::string destSysStr = "[Error]";
    std::string retStelStr = "[Error]";
    std::string retSysStr = "[Error]";
    std::string cargoStr = "[Error]";
    std::string cargoQtyStr = "[Error]";
    std::string shipMsgStr = "[Error]";
    std::string deadlineStr = "[Error]";
    std::string personStr = "[Error]";
    std::string shipTypeStr = "[Error]";
    std::string prkStr = "[Error]";
    std::string srkStr = "[Error]";
    std::string rank3Str = "[Error]";
    std::string rank4Str = "[Error]";
    std::string rrkStr = "[Error]";
    std::string payStr = "[Error]";
    std::string playerNameStr;
    std::string nickStr;
    std::string playerShipStr;
    std::string noRank;
    std::string dateText;
    size_t len;
    short i;
    short sys;
    short bestIdx;
    short bestVal = 0;
    int32_t pay;
    int32_t pct;

    /* an empty text leaves the buffer as it is */
    len = strnlen(briefingTextPtr, kLongTextSize);
    if (len == 0)
        return;
    text.assign(briefingTextPtr, len);
    memset(briefingTextPtr, 0, kLongTextSize);

    if (briefing != 0) {
        if (which == -1)
            goto tokens;

        if (briefMissionInfo[which].destStellar >= 0 &&
            briefMissionInfo[which].destStellar < kMaxStellars) {
            destStelStr = NameToken(stellarData[briefMissionInfo[which].destStellar].name);
        }
        if (briefMissionInfo[which].destSystem >= 0 &&
            briefMissionInfo[which].destSystem < kMaxSystems) {
            sys = FindActiveCoLocatedSystem(briefMissionInfo[which].destSystem);
            if (sys == -1)
                sys = FindFirstCoLocatedSystem(briefMissionInfo[which].destSystem);
            if (sys != -1) {
                destSysStr = NameToken(systemData[sys].name);
            }
        }
        if (briefMissionInfo[which].returnStellar >= 0 &&
            briefMissionInfo[which].returnStellar < kMaxStellars) {
            retStelStr = NameToken(stellarData[briefMissionInfo[which].returnStellar].name);
        }
        if (briefMissionInfo[which].returnSystem >= 0 &&
            briefMissionInfo[which].returnSystem < kMaxSystems) {
            sys = FindActiveCoLocatedSystem(briefMissionInfo[which].returnSystem);
            if (sys == -1)
                sys = FindFirstCoLocatedSystem(briefMissionInfo[which].returnSystem);
            if (sys != -1) {
                retSysStr = NameToken(systemData[sys].name);
            }
        }

        if (destStelStr == "[Error]") {
            if (retStelStr != "[Error]")
                destStelStr = retStelStr;
        }
        if (destSysStr == "[Error]") {
            if (retSysStr != "[Error]")
                destSysStr = retSysStr;
        }

        if (briefMissionInfo[which].cargoType >= 0 &&
            briefMissionInfo[which].cargoType < kMaxCargo) {
            cargoStr = NameToken(lcCargoName[briefMissionInfo[which].cargoType]);
            if (!cargoStr.empty() && cargoStr[0] == '*')
                cargoStr.erase(0, 1);
            cargoQtyStr = NumText(briefMissionInfo[which].cargoUnits);
        }

        if (briefMissionInfo[which].dueDate.month != gameTime.month ||
            briefMissionInfo[which].dueDate.day != gameTime.day ||
            briefMissionInfo[which].dueDate.year != gameTime.year) {
            dateText = DateLongText(briefMissionInfo[which].dueDate);
            deadlineStr = FieldToken(dateText.c_str(), dateText.size(), kTokenMax);
        }

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
        if (pay >= 0)
            payStr = CommaNumber(pay);
    } else {
        if (which == -1)
            goto tokens;
        if (currentMissionObjectives[which].active == 0)
            goto tokens;

        if (currentMissions[which].travelStellar >= 0 &&
            currentMissions[which].travelStellar < kMaxStellars) {
            destStelStr = NameToken(stellarData[currentMissions[which].travelStellar].name);
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
                destSysStr = NameToken(systemData[sys].name);
            }
        }
        if (currentMissions[which].returnStellar >= 0 &&
            currentMissions[which].returnStellar < kMaxStellars) {
            retStelStr = NameToken(stellarData[currentMissions[which].returnStellar].name);
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
                retSysStr = NameToken(systemData[sys].name);
            }
        }

        if (destStelStr == "[Error]") {
            if (retStelStr != "[Error]")
                destStelStr = retStelStr;
        }
        if (destSysStr == "[Error]") {
            if (retSysStr != "[Error]")
                destSysStr = retSysStr;
        }

        if (currentMissions[which].cargoType >= 0 &&
            currentMissions[which].cargoType < kMaxCargo) {
            cargoStr = NameToken(lcCargoName[currentMissions[which].cargoType]);
            if (!cargoStr.empty() && cargoStr[0] == '*')
                cargoStr.erase(0, 1);
            cargoQtyStr = NumText(currentMissions[which].cargoQty);
        }

        if (currentMissionObjectives[which].dueDate.month != gameTime.month ||
            currentMissionObjectives[which].dueDate.day != gameTime.day ||
            currentMissionObjectives[which].dueDate.year != gameTime.year) {
            dateText = DateLongText(currentMissionObjectives[which].dueDate);
            deadlineStr = FieldToken(dateText.c_str(), dateText.size(), kTokenMax);
        }

        shipMsgStr = NameToken((const unsigned char *)currentMissions[which].shipName);

        pay = 0;
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
        if (pay >= 0)
            payStr = CommaNumber(pay);
    }

tokens:
    if (offeringShipIndex >= 0 && offeringShipIndex < kMaxShips &&
        (shipData[offeringShipIndex].persType >= 0 &&
         shipData[offeringShipIndex].persType < kMaxPersons)) {
        personStr = NameToken((const unsigned char *)personData[shipData[offeringShipIndex].persType].name);
    }

    playerNameStr = NameToken(playerName);
    if (playerNickname[0] != 0)
        nickStr = NameToken(playerNickname);
    else
        nickStr = FieldToken((const char *)playerName, sizeof playerName, kNameLen);
    playerShipStr = NameToken(shipName);
    shipTypeStr = NameToken(shipClass[shipData[0].type].name);

    noRank = IndString(kGameStrList, kNoRankIndex);
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
        prkStr = FieldToken(rankData[bestIdx].convName, sizeof rankData[bestIdx].convName, kTokenMax);
    } else {
        prkStr = FieldToken(noRank.c_str(), noRank.size(), kTokenMax);
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
        srkStr = FieldToken(rankData[bestIdx].shortName, sizeof rankData[bestIdx].shortName, kTokenMax);
    } else {
        srkStr = FieldToken(noRank.c_str(), noRank.size(), kTokenMax);
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
        rank3Str = FieldToken(rankData[bestIdx].shortName, sizeof rankData[bestIdx].shortName, kTokenMax);
    } else {
        rank3Str = FieldToken(noRank.c_str(), noRank.size(), kTokenMax);
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
        rank4Str = FieldToken(rankData[bestIdx].shortName, sizeof rankData[bestIdx].shortName, kTokenMax);
    } else {
        rank4Str = FieldToken(noRank.c_str(), noRank.size(), kTokenMax);
    }

    if (recentActivatedRank != -1) {
        rrkStr = FieldToken(rankData[recentActivatedRank].name, sizeof rankData[recentActivatedRank].name,
                            kRecentRankMax);
    } else {
        rrkStr = FieldToken(noRank.c_str(), noRank.size(), kRecentRankMax);
    }

    MungeForSymbol(text, "<DST>", destStelStr);
    MungeForSymbol(text, "<DSY>", destSysStr);
    MungeForSymbol(text, "<RST>", retStelStr);
    MungeForSymbol(text, "<RSY>", retSysStr);
    MungeForSymbol(text, "<CT>", cargoStr);
    MungeForSymbol(text, "<CQ>", cargoQtyStr);
    MungeForSymbol(text, "<SN>", shipMsgStr);
    MungeForSymbol(text, "<DL>", deadlineStr);
    MungeForSymbol(text, "<PN>", playerNameStr);
    MungeForSymbol(text, "<PNN>", nickStr);
    MungeForSymbol(text, "<PSN>", playerShipStr);
    MungeForSymbol(text, "<PST>", shipTypeStr);
    MungeForSymbol(text, "<OSN>", personStr);
    MungeForSymbol(text, "<PRK>", prkStr);
    MungeForSymbol(text, "<SRK>", srkStr);
    MungeForSymbol(text, "<RRK>", rrkStr);
    MungeForSymbol(text, "<PAY>", payStr);
    MungeForSymbol(text, "<REG>", kRegisteredName);

    if (descRankGovt[0] != -1) {
        tokenStr = "<PRK" + std::to_string(descRankGovt[0] + kFirstResourceID) + ">";
        MungeForSymbol(text, tokenStr.c_str(), rank4Str);
    }
    if (descRankGovt[1] != -1) {
        tokenStr = "<SRK" + std::to_string(descRankGovt[1] + kFirstResourceID) + ">";
        MungeForSymbol(text, tokenStr.c_str(), rank3Str);
    }

    if (leetMode != 0) {
        MungeForSymbol(text, "you", "j00");
        MungeForSymbol(text, "You", "j00");
        MungeForSymbol(text, "ck", "x0r");
        MungeForSymbol(text, "i", "1");
        MungeForSymbol(text, "I", "1");
        MungeForSymbol(text, "e", "3");
        MungeForSymbol(text, "E", "3");
        MungeForSymbol(text, "l", "7");
        MungeForSymbol(text, "L", "7");
    }

    /* the text stays NUL-ended: at most kLongTextSize - 1 bytes */
    memcpy(briefingTextPtr, text.data(), std::min(text.size(), (size_t)kLongTextSize - 1));
}

/* the first `find' at or after offset replaced by `repl'; returns the offset after the replacement, -1 when
   there is none */
// FUNCTION: EVNOVA 0x004bc100
static int32_t ReplaceInText(std::string &text, int32_t offset, const char *find, int32_t findLen,
                             const char *repl, int32_t replLen)
{
    size_t at;

    if ((int32_t)text.size() - offset - findLen + 1 <= 0)
        return -1;
    at = text.find(find, (size_t)offset, (size_t)findLen);
    if (at == std::string::npos)
        return -1;
    text.replace(at, (size_t)findLen, repl, (size_t)replLen);
    return (int32_t)at + replLen;
}

/* every needle in the text replaced; the resume offset is a short: past 32767 it wraps */
// FUNCTION: EVNOVA 0x00445d70
// MAC: _MungeForSymbol i386 0x000984a3 ppc 0x000a4230
static void MungeForSymbol(std::string &text, const char *needle, const std::string &replacement)
{
    short needleLen;
    short replLen;
    short at;

    needleLen = strlen(needle);
    replLen = replacement.size();
    at = 0;
    do {
        at = ReplaceInText(text, at, needle, needleLen, replacement.data(), replLen);
    } while (at >= 0);
}
