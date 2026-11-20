/* galaxyutils.cpp -- calendar, systems, stellars, governments, exploration, legal penalties */

#include "game.h"
#include "player.h"
#include "gameutil.h"
#include "prototypes.h"
#include "stellar.h"
#include "missions.h"
#include "crashlog.h"

EXTERN_C_BEGIN

// FUNCTION: EVNOVA 0x00466030
// MAC: _ColorCodeStellar i386 0x0000aa26 ppc 0x0000d1ac
void ColorCodeStellar(SpobRecord *spob, RGBColor *color)
{
    Boolean uninhabited;

    uninhabited = SpobUninhabited(spob);
    if (!StellarIsLandable(spob)) {
        *color = rgbMedGrey;
    } else if (uninhabited) {
        *color = rgbMedGrey;
    } else if (spob->flags2 & kSpobHypergate) {
        if (spob->govt != -1 && PlayerBlessedByGovt(spob->govt, 1)) {
            color->red = 0xffff;
            color->green = 0xffff;
            color->blue = 0;
        } else {
            if (spob->minStatus == 0x7fff
                    || (playerRecord[spob->system] < spob->minStatus
                        && spob->minStatus > -0x7fff)) {
                if (playerRecord[spob->system] >= 0) {
                    color->red = 0xffff;
                    color->green = 0x6666;
                    color->blue = 0;
                } else {
                    color->red = 0xffff;
                    color->green = 0;
                    color->blue = 0;
                }
            } else {
                color->red = 0xffff;
                color->green = 0xffff;
                color->blue = 0;
            }
        }
    } else if (spob->flags2 & kSpobWormhole) {
        color->red = 0;
        color->green = 0x55f0;
        color->blue = 0x55f0;
    } else if (spob->dominated == 0) {
        if (spob->govt != -1 && PlayerBlessedByGovt(spob->govt, 1)) {
            color->red = 0xffff;
            color->green = 0xffff;
            color->blue = 0;
        } else {
            if (spob->minStatus == 0x7fff
                    || (playerRecord[spob->system] < spob->minStatus
                        && spob->minStatus > -0x7fff)) {
                if (playerRecord[spob->system] >= 0) {
                    color->red = 0xffff;
                    color->green = 0x6666;
                    color->blue = 0;
                } else {
                    color->red = 0xffff;
                    color->green = 0;
                    color->blue = 0;
                }
            } else {
                color->red = 0xffff;
                color->green = 0xffff;
                color->blue = 0;
            }
        }
    } else {
        color->red = 0;
        color->green = 0xffff;
        color->blue = 0;
    }
}

// FUNCTION: EVNOVA 0x00466260
// MAC: _ColorCodeSystem i386 0x0000d672 ppc 0x00010b04
void ColorCodeSystem(short sys, RGBColor *color)
{
    short i;
    short special;
    short friendly;
    short warned;
    short hostile;
    short spob;
    short first;

    first = FindFirstCoLocatedSystem(sys);
    sys = FindActiveCoLocatedSystem(sys);
    *color = rgbDarkGrey;
    if (sys < 0 || sys >= kMaxSystems || first < 0 || first >= kMaxSystems)
        return;
    if (systemData[first].exploreLevel > 0) {
        if (SystemIsInhabited(sys)) {
            special = 0;
            friendly = 0;
            warned = 0;
            hostile = 0;
            for (i = 0; i < kNavSlots; i++) {
                spob = systemData[sys].navDefault[i];
                if (spob != -1 && stellarData[spob].loaded != 0 &&
                    !SpobIsHyperOrWorm(&stellarData[spob]) &&
                    !SpobUninhabited(&stellarData[spob]) &&
                    StellarIsLandable(&stellarData[spob])) {
                    if (stellarData[spob].dominated != 0) {
                        special++;
                    } else if (stellarData[spob].govt != -1 &&
                               PlayerBlessedByGovt(stellarData[spob].govt, 1)) {
                        friendly++;
                    } else if (stellarData[spob].minStatus == 0x7fff ||
                               (playerRecord[first] < stellarData[spob].minStatus &&
                                stellarData[spob].minStatus > -0x7fff)) {
                        if (playerRecord[first] >= 0)
                            warned++;
                        else
                            hostile++;
                    } else {
                        friendly++;
                    }
                }
            }
            if (special > 0) {
                *color = brightGreen;
            } else if (friendly > 0) {
                color->red = 0;
                color->green = 0;
                color->blue = 0xffff;
            } else if (warned > 0) {
                color->red = 0xffff;
                color->green = 0x6666;
                color->blue = 0;
            } else if (hostile > 0) {
                color->red = 0xffff;
                color->green = 0;
                color->blue = 0;
            } else {
                *color = rgbLightGrey;
            }
        } else {
            *color = rgbLightGrey;
        }
    } else {
        *color = rgbDarkGrey;
    }
}

// FUNCTION: EVNOVA 0x00466c40
// MAC: _IncrementDate i386 0x00004da6 ppc 0x000062cc
void IncrementDate(DateTimeRec *date, int32_t daysToAdd)
{
    short days;

    switch (date->month) {
    case 4:
    case 6:
    case 9:
    case 11:
        days = 30;
        break;
    case 2:
        if (date->year % 4 == 0)
            days = 29;
        else
            days = 28;
        break;
    default:
        days = 31;
        break;
    }
    date->day++;
    if (date->day > days) {
        date->day = 1;
        date->month++;
        if (date->month > 12) {
            date->month = 1;
            date->year++;
        }
    }
}

#define kDefWaveEncoded   0x3e8
#define kDefGrowOdds      0x1c2

// FUNCTION: EVNOVA 0x00466cb0
// MAC: _IncrementGameTime i386 0x0000b516 ppc 0x0000dfdc
void IncrementGameTime(void)
{
    short i;
    short j;
    short wave;
    IncrementDate(&gameTime, 0);
    CronEventHandler();

    for (i = 0; i < kMaxActiveMissions; i++) {
        if (currentMissions[i].timeLimit > 0)
            currentMissions[i].timeLimit--;
    }

    DoStellarTimePassage();
    DisasterHandler();

    for (j = 0; j < kMaxStellars; j++) {
        if (stellarData[j].loaded != 0) {
            stellarData[j].reloadTimer = 0;
            stellarData[j].burstShots = 0;
            if (stellarData[j].dominated != 0) {
                if (stellarData[j].defCount > kDefWaveEncoded)
                    wave = stellarData[j].defCount % kDefWaveEncoded;
                else
                    wave = stellarData[j].defCount;
                if (stellarData[j].defWaveSize < wave && Rand(kDefGrowOdds) == 0)
                    stellarData[j].defWaveSize++;
            }
            if (StellarIsDestroyed(&stellarData[j])) {
                if (stellarData[j].deadDays >= 0) {
                    stellarData[j].deadDaysLeft--;
                    if (stellarData[j].deadDaysLeft <= 0) {
                        stellarData[j].deadDaysLeft = -1;
                        stellarData[j].deadCounter = stellarData[j].strength;
                        crashlog_mission_bit_set("regenerating stellar",
                                                 (char *)stellarData[j].name, 1,
                                                 j + 128, stellarData[j].onRegen);
                        EvalMissionBitSetString(stellarData[j].onRegen);
                    }
                } else {
                    stellarData[j].deadDaysLeft = 1;
                }
            } else {
                stellarData[j].deadCounter = stellarData[j].strength;
                stellarData[j].deadDaysLeft = -1;
            }
        }
    }

    for (j = 0; j < kMaxStellars; j++) {
        if (systemData[j].loaded != 0 && systemData[j].reinforceDaysLeft > 0)
            systemData[j].reinforceDaysLeft--;
    }

    for (j = 0; j < kMaxRanks; j++) {
        if (rankData[j].active != 0 && rankData[j].loaded != 0 && rankData[j].salary != 0) {
            if (shipData->credits < rankData[j].salaryCap || rankData[j].salaryCap <= 0)
                shipData->credits += rankData[j].salary;
            if (shipData->credits < 0)
                shipData->credits = 0;
        }
    }

    for (j = 0; j < kMaxShipTypes; j++) {
        shipClass[j].dailyHireRoll = Rand(kDailyRollRange) + 1;
        shipClass[j].dailySaleRoll = Rand(kDailyRollRange) + 1;
    }

    for (j = 0; j < kMaxOutfits; j++)
        outfitSystem[j].dailyRoll = Rand(kDailyRollRange) + 1;
}

// FUNCTION: EVNOVA 0x00466fc0
// MAC: _SlapWithPenalty i386 0x00009bbc ppc 0x0000c108
void SlapWithPenalty(short sys, short govt, short type, short mode)
{
    short i;

    if (govt != -1) {
        if ((govtData[govt].flags & kGovtAlwaysDisabled) != 0)
            return;
    }
    for (i = 0; i < kMaxSystems; i++)
        systemFlag[i] = 0;

    if (mode == -1) {
        RecursivePenaltySlap(sys, govt, type, 1.0);

        for (i = 0; i < kMaxRanks; i++) {
            if (rankData[i].active != 0 && rankData[i].loaded != 0
                    && (rankData[i].flags & kRankNeverDropped) == 0
                    && GovtAllies(govt, rankData[i].govt) != 0) {
                if ((rankData[i].flags & kRankLostOnCrime) != 0)
                    DeactivateRank((char *)&rankData[i]);
                else if ((rankData[i].flags & kRankLostOnAttack) != 0
                        && (type == 1 || type == 3))
                    DeactivateRank((char *)&rankData[i]);
            }
        }
    }
}

// FUNCTION: EVNOVA 0x00467140
// MAC: _RecursivePenaltySlap i386 0x0000980a ppc 0x0000bb40
void RecursivePenaltySlap(short sys, short govt, short type, double amount)
{
    short startSys, i;
    short govtFlag1;
    short sysFlag1, sysFlag2;
    Boolean dirty;
    float penalty;

    if (sys < 0 || sys >= kMaxSystems)
        return;
    if (systemFlag[sys])
        return;
    systemFlag[sys] = 1;

    if (govt != -1)
        govtFlag1 = govtData[govt].flags & kGovtXenophobic;
    else
        govtFlag1 = 0;

    startSys = sys;
    dirty = 0;
    if (systemData[sys].colocGroup != -1)
        sys = systemData[sys].colocGroup;
    while (sys != -1) {
        systemFlag[sys] = 1;
        if (systemData[sys].loaded) {
            penalty = 0;
            sysFlag1 = 0;
            sysFlag2 = 0;
            if (systemData[sys].govt != -1) {
                sysFlag1 = govtData[systemData[sys].govt].flags & kGovtXenophobic;
                sysFlag2 = govtData[systemData[sys].govt].flags & kGovtEnforcesAbroad;
            }

            if (systemData[sys].govt == -1) {
                if (govt == -1) {
                    penalty = govtData[0].penalties[type] * 0.5;
                } else if (govtFlag1) {
                    penalty = -govtData[govt].penalties[type] * 0.5;
                } else {
                    penalty = govtData[govt].penalties[type] * 0.25;
                }
            } else if (govt == systemData[sys].govt) {
                penalty = govtData[govt].penalties[type];
            } else if (govt == -1) {
                if (sysFlag1)
                    penalty = -govtData[systemData[sys].govt].penalties[type] * 0.5;
                else if (sysFlag2)
                    penalty = govtData[systemData[sys].govt].penalties[type] * 0.5;
            } else if (!GovtAllies(govt, systemData[sys].govt)) {
                penalty = -govtData[systemData[sys].govt].penalties[type] * 0.5;
            } else if (!GovtEnemies(govt, systemData[sys].govt)) {
                if (GovtAllies(govt, systemData[sys].govt))
                    penalty = govtData[govt].penalties[type];
                else
                    penalty = govtData[govt].penalties[type] * 0.5;
            } else {
                penalty = -govtData[systemData[sys].govt].penalties[type] * 0.5;
            }

            penalty *= amount;
            if (penalty >= 1.0 || penalty <= -1.0) {
                dirty = 1;
                if (penalty > 0.0) {
                    if (playerRecord[sys] - penalty < -32000.0f)
                        playerRecord[sys] = -32000;
                    else
                        playerRecord[sys] -= penalty;
                } else {
                    if (playerRecord[sys] - penalty > 32000.0f)
                        playerRecord[sys] = 32000;
                    else
                        playerRecord[sys] -= penalty;
                }
                if (playerRecord[sys] > 32000)
                    playerRecord[sys] = 32000;
                if (playerRecord[sys] < -32000)
                    playerRecord[sys] = -32000;
            }
        }
        sys = systemData[sys].colocNext;
    }

    if (dirty) {
        amount *= 0.65;
        for (i = 0; i < kMaxSystemLinks; i++) {
            if (systemData[startSys].links[i] != -1)
                RecursivePenaltySlap(FindFirstCoLocatedSystem(systemData[startSys].links[i]),
                                     govt, type, amount);
        }
    }
}

EXTERN_C_END

/* scratch recursion marks, one per system (name invented) */
Boolean systemFlag[kMaxSystems];
short maxAutoExploreJumps;

EXTERN_C_BEGIN

// FUNCTION: EVNOVA 0x00467710
// MAC: _EscapeStellarSearch i386 0x0000d28f ppc 0x00010648
short EscapeStellarSearch(short sys)
{
    short i;

    for (i = 0; i < kMaxSystems; i++)
        systemFlag[i] = 0;
    systemFlag[sys] = 1;
    return RecursiveEscapeStellarSearch(sys);
}

// FUNCTION: EVNOVA 0x004677a0
// MAC: _RecursiveEscapeStellarSearch i386 0x0000d0fe ppc 0x00010464
short RecursiveEscapeStellarSearch(short sys)
{
    short i;
    short j;
    short link;
    short link2;
    short stellar;

    systemFlag[sys] = 1;

    for (i = 0; i < kNavSlots; i++) {
        link = systemData[sys].links[i];
        if (link == -1)
            continue;
        if (systemData[link].loaded == 0)
            continue;
        if (systemData[link].known == 0)
            continue;
        if (systemData[link].exploreLevel <= 0)
            continue;
        if (systemFlag[link] != 0)
            continue;
        for (j = 0; j < kNavSlots; j++) {
            stellar = systemData[link].navDefault[j];
            if (stellar == -1)
                continue;
            if (stellarData[stellar].loaded == 0)
                continue;
            if (!StellarIsNormalLandable(&stellarData[stellar]))
                continue;
            if ((stellarData[stellar].flags & kSpobHasShipyard) == 0)
                continue;
            if (stellarData[stellar].minStatus != 0x7fff &&
                    (playerRecord[link] >= stellarData[stellar].minStatus
                     || stellarData[stellar].minStatus == -0x7fff))
                return stellar;
        }
    }

    for (i = 0; i < kNavSlots; i++) {
        link2 = systemData[sys].links[i];
        if (link2 == -1)
            continue;
        if (systemData[link2].loaded == 0)
            continue;
        if (systemData[link2].known == 0)
            continue;
        if (systemData[link2].exploreLevel <= 0)
            continue;
        if (systemFlag[link2] != 0)
            continue;
        stellar = RecursiveEscapeStellarSearch(link2);
        if (stellar != -1)
            return stellar;
    }
    return -1;
}

// FUNCTION: EVNOVA 0x00467970
// MAC: _AutoSetExploration i386 0x0000daba ppc 0x00010ff4
void AutoSetExploration(short sys, short maxDepth, short level)
{
    short i;
    short j;
    short link;

    for (i = 0; i < kMaxSystems; i++)
        systemFlag[i] = 0;

    maxAutoExploreJumps = maxDepth;
    RecursiveAutoExplore(sys, 0, level);

    for (i = 0; i < kMaxSystems; i++) {
        if (systemData[i].loaded != 0 && systemData[i].prepared != 0 &&
            systemData[i].exploreLevel > 0) {
            systemData[i].known = 1;
            for (j = 0; j < kNavSlots; j++) {
                link = systemData[i].links[j];
                if (link != -1) {
                    link = FindActiveCoLocatedSystem(link);
                    if (link != -1)
                        systemData[link].known = 1;
                }
            }
        }
    }
}

// FUNCTION: EVNOVA 0x00467ab0
// MAC: _RecursiveAutoExplore i386 0x0000d942 ppc 0x00010e4c
void RecursiveAutoExplore(short sys, short depth, short level)
{
    short i;
    short coloc;
    short link;

    if (depth > maxAutoExploreJumps)
        return;
    if (systemFlag[sys] != 0)
        return;
    systemFlag[sys] = 1;

    ExploreNebulaeFromSystem(sys);

    if (systemData[sys].exploreLevel < level)
        systemData[sys].exploreLevel = level;

    coloc = FindFirstCoLocatedSystem(sys);
    if (coloc != sys && coloc >= 0 && coloc < kMaxSystems) {
        if (systemData[coloc].exploreLevel < level)
            systemData[coloc].exploreLevel = level;
    }

    for (i = 0; i < kNavSlots; i++) {
        link = systemData[sys].links[i];
        if (link != -1) {
            link = FindActiveCoLocatedSystem(link);
            if (link != -1)
                RecursiveAutoExplore(link, depth + 1, level);
        }
    }
}

/* Any nebula whose (inset) map rectangle contains this system counts as visited. */
// FUNCTION: EVNOVA 0x00467bd0
// MAC: _ExploreNebulaeFromSystem i386 0x0000508f ppc 0x00006600
void ExploreNebulaeFromSystem(short sys)
{
    Rect box;
    Point pt;
    short top;
    short i;

    for (i = 0; i < kMaxNebulae; i++) {
        if (nebulaData[i].loaded != 0 && nebulaData[i].explored == 0) {
            top = nebulaData[i].yPos;
            SetRect(&box, nebulaData[i].xPos, top,
                    nebulaData[i].xPos + nebulaData[i].width,
                    top + nebulaData[i].height);
            InsetRect(&box, 8, 8);
            pt.h = systemData[sys].pos.h;
            pt.v = systemData[sys].pos.v;
            if (PtInRectPoint(pt, &box) != 0) {
                nebulaData[i].explored = 1;
                crashlog_mission_bit_set("exploring nebula", 0, i + 128, -1,
                                         nebulaData[i].onExplore);
                EvalMissionBitSetString(nebulaData[i].onExplore);
            }
        }
    }
}

// FUNCTION: EVNOVA 0x00468210
// MAC: _SystemTradesGoods i386 0x0000d2c4 ppc 0x0001069c
Boolean SystemTradesGoods(short sysID, short commodity)
{
    short i;

    for (i = 0; i < kNavSlots; i++) {
        if (systemData[sysID].navDefault[i] != -1
                && stellarData[systemData[sysID].navDefault[i]].loaded != 0
                && (stellarData[systemData[sysID].navDefault[i]].flags
                    & kSpobUninhabited) == 0
                && StellarIsNormalLandable(
                       &stellarData[systemData[sysID].navDefault[i]])
                && (stellarData[systemData[sysID].navDefault[i]].flags
                    & kSpobTradeCenter) != 0
                && CalcPortDemand(commodity,
                       stellarData[systemData[sysID].navDefault[i]].flags) != 0)
            return 1;
    }
    return 0;
}

/* the same scan asking for one of three services by index; a `kind` outside
   0..2 leaves the mask unset, which the original does not guard */
// FUNCTION: EVNOVA 0x004682d0
// MAC: _SystemHasService i386 0x0000d373 ppc 0x0001077c
Boolean SystemHasService(short sysID, short kind)
{
    short i;
    int32_t mask;

    for (i = 0; i < kNavSlots; i++) {
        if (systemData[sysID].navDefault[i] != -1
                && stellarData[systemData[sysID].navDefault[i]].loaded != 0
                && (stellarData[systemData[sysID].navDefault[i]].flags
                    & kSpobUninhabited) == 0
                && StellarIsNormalLandable(
                       &stellarData[systemData[sysID].navDefault[i]])) {
            if (kind == 0)
                mask = kSpobTradeCenter;
            if (kind == 1)
                mask = kSpobHasOutfitter;
            if (kind == 2)
                mask = kSpobHasShipyard;
            if ((stellarData[systemData[sysID].navDefault[i]].flags & mask) != 0)
                return 1;
        }
    }
    return 0;
}

/* inhabited = at least one stellar that is populated and not a hypergate or wormhole */

// FUNCTION: EVNOVA 0x00468af0
// MAC: _SystemIsInhabited i386 0x00004c4a ppc 0x00006124
Boolean SystemIsInhabited(short sys)
{
    short i;
    short spob;

    for (i = 0; i <= 3; i++) {
        spob = systemData[sys].navDefault[i];
        if (spob != -1) {
            if ((stellarData[spob].flags & kSpobUninhabited) == 0) {
                if (!SpobIsHyperOrWorm(&stellarData[spob]))
                    return 1;
            }
        }
    }
    return 0;
}

// FUNCTION: EVNOVA 0x00468b50
// MAC: _StellarsAreDistant i386 0x00009cb9 ppc 0x0000c23c
Boolean StellarsAreDistant(short a, short b)
{
    short i;
    short found;
    short count;
    SpobRecord *pb;
    short sysA;
    short sysB;
    SpobRecord *pa;

    if (a < 0 || a >= kMaxStellars)
        return 1;
    pa = &stellarData[a];
    if (!pa->prepared)
        return 0;

    if (b >= 0 && b < kMaxStellars) {
        pb = &stellarData[b];
        if (pa->system < 0 || pa->system >= kMaxSystems ||
                pb->system < 0 || pb->system >= kMaxSystems)
            return 0;
        if (pa->system == pb->system)
            return 0;

        sysA = FindFirstCoLocatedSystem(pa->system);
        sysB = FindFirstCoLocatedSystem(pb->system);
        if (sysA == sysB)
            return 0;

        for (i = 0; i < kNavSlots; i++) {
            if (systemData[pa->system].links[i] != -1 &&
                    FindFirstCoLocatedSystem(systemData[pa->system].links[i]) == sysB)
                return 0;
            if (systemData[pb->system].links[i] != -1 &&
                    FindFirstCoLocatedSystem(systemData[pb->system].links[i]) == sysA)
                return 0;
        }
    } else {
        if (pa->system < 0 || pa->system >= kMaxSystems)
            return 0;
        sysA = FindFirstCoLocatedSystem(pa->system);
    }

    count = 0;
    found = 0;
    sysA = FindFirstCoLocatedSystem(sysA);
    do {
        count++;
        for (i = 0; i < kNavSlots; i++) {
            if (systemData[sysA].navDefault[i] == pa->index) {
                found++;
                break;
            }
        }
        sysA = systemData[sysA].colocNext;
    } while (sysA != -1);

    if (found != count)
        return 0;
    return 1;
}

/* the first system of the group that has actually been loaded */
// FUNCTION: EVNOVA 0x0046b920
// MAC: _FindActiveCoLocatedSystem i386 0x00004be0 ppc 0x000060b8
short FindActiveCoLocatedSystem(short sys)
{
    short next;

    if (sys < 0 || sys >= kMaxSystems)
        return -1;
    if (systemData[sys].colocGroup == -1) {
        if (systemData[sys].loaded != 0)
            return sys;
        return -1;
    }
    next = systemData[sys].colocGroup;
    do {
        if (systemData[next].loaded != 0)
            return next;
        next = systemData[next].colocNext;
    } while (next != -1);
    return -1;
}

// FUNCTION: EVNOVA 0x0046b9b0
// MAC: _FindFirstCoLocatedSystem i386 0x00004baa ppc 0x0000607c
short FindFirstCoLocatedSystem(short sys)
{
    if (sys == -1)
        return -1;
    if (systemData[sys].colocGroup == -1)
        return sys;
    return systemData[sys].colocGroup;
}

// FUNCTION: EVNOVA 0x0046bc90
// MAC: _GovtAllies i386 0x00004e3d ppc 0x00006390
Boolean GovtAllies(short a, short b)
{
    short i;
    short j;

    if (a == b)
        return 1;
    if (a >= 0 && a < kMaxGovts && b >= 0 && b < kMaxGovts) {
        if ((govtData[a].flags & kGovtAlwaysDisabled) != 0 ||
            (govtData[b].flags & kGovtAlwaysDisabled) != 0)
            return 0;

        for (i = 0; i < kGovtClasses; i++) {
            if (govtData[a].classes[i] >= 0) {
                for (j = 0; j < kGovtClasses; j++) {
                    if (govtData[b].allies[j] >= 0
                            && govtData[b].allies[j] == govtData[a].classes[i])
                        return 1;
                }
            }
            if (govtData[b].classes[i] >= 0) {
                for (j = 0; j < kGovtClasses; j++) {
                    if (govtData[a].allies[j] >= 0
                            && govtData[a].allies[j] == govtData[b].classes[i])
                        return 1;
                }
            }
        }
    }
    return 0;
}

// FUNCTION: EVNOVA 0x0046bdf0
// MAC: _GovtEnemies i386 0x00004f22 ppc 0x0000647c
Boolean GovtEnemies(short a, short b)
{
    short i;
    short j;

    if (a == b)
        return 0;
    if (a >= 0 && b >= 0 && a < kMaxGovts && b < kMaxGovts) {
        if ((govtData[a].flags & kGovtAlwaysDisabled) != 0 ||
            (govtData[b].flags & kGovtAlwaysDisabled) != 0)
            return 0;

        for (i = 0; i < kGovtClasses; i++) {
            if (govtData[a].classes[i] >= 0) {
                for (j = 0; j < kGovtClasses; j++) {
                    if (govtData[b].enemies[j] >= 0
                            && govtData[b].enemies[j] == govtData[a].classes[i])
                        return 1;
                }
            }
            if (govtData[b].classes[i] >= 0) {
                for (j = 0; j < kGovtClasses; j++) {
                    if (govtData[a].enemies[j] >= 0
                            && govtData[a].enemies[j] == govtData[b].classes[i])
                        return 1;
                }
            }
        }
    }

    if (!GovtAllies(a, b)) {
        if (a >= 0 && a < kMaxGovts && (govtData[a].flags & kGovtXenophobic) != 0)
            return 1;
        if (b >= 0 && b < kMaxGovts && (govtData[b].flags & kGovtXenophobic) != 0)
            return 1;
    }
    return 0;
}

// FUNCTION: EVNOVA 0x0046bff0
// MAC: _GovtSharedClass i386 0x00006e48 ppc 0x0000888c
Boolean GovtSharedClass(short a, short b)
{
    short i;

    if (a == b)
        return 1;
    if (a >= 0 && a < kMaxGovts && b >= 0 && b < kMaxGovts) {
        for (i = 0; i < kGovtClasses; i++) {
            if (govtData[a].classes[i] >= 0 &&
                govtData[a].classes[i] == govtData[b].classes[i])
                return 1;
        }
    }
    return 0;
}

// FUNCTION: EVNOVA 0x0046c800
// MAC: _CronDateInRange i386 0x000074d3 ppc 0x00008fa0
Boolean CronDateInRange(CronRecord *cron)
{
    if (cron->firstYear > 0 && gameTime.year < cron->firstYear)
        return 0;
    if (cron->firstMonth == 0) {
        if (cron->firstDay <= 0)
            goto firstShared;
        if (gameTime.day < cron->firstDay)
            return 0;
    } else {
firstShared:
        if (cron->firstMonth > 0 && cron->firstDay == 0) {
            if (gameTime.month < cron->firstMonth)
                return 0;
        } else if (cron->firstMonth > 0) {
            if (cron->firstDay > 0) {
                if (gameTime.month * 32 + gameTime.day < cron->firstMonth * 32 + cron->firstDay)
                    return 0;
            }
        }
    }
    if (cron->lastYear > 0 && gameTime.year > cron->lastYear)
        return 0;
    if (cron->lastMonth == 0) {
        if (cron->lastDay <= 0)
            goto lastShared;
        if (gameTime.day > cron->lastDay)
            return 0;
    } else {
lastShared:
        if (cron->lastMonth > 0 && cron->lastDay == 0) {
            if (gameTime.month > cron->lastMonth)
                return 0;
        } else if (cron->lastMonth > 0) {
            if (cron->lastDay > 0) {
                if (gameTime.month * 32 + gameTime.day > cron->lastMonth * 32 + cron->lastDay)
                    return 0;
            }
        }
    }
    return 1;
}

// FUNCTION: EVNOVA 0x0046e3c0
// MAC: _StellarIsDestroyed i386 0x00004e14 ppc 0x0000635c
Boolean StellarIsDestroyed(SpobRecord *spob)
{
    if (spob->strength > 0 && (spob->deadCounter < 0 || spob->deadDaysLeft > 0))
        return 1;
    return 0;
}

// FUNCTION: EVNOVA 0x0046e3f0
// MAC: _StellarIsLandable i386 0x0000a989 ppc 0x0000d0d0
Boolean StellarIsLandable(SpobRecord *spob)
{
    if (SpobCanLand(spob) &&
        ((StellarIsDestroyed(spob) && SpobLandOnlyWhenDestroyed(spob)) ||
         (!StellarIsDestroyed(spob) && !SpobLandOnlyWhenDestroyed(spob))))
        return 1;
    return 0;
}

/* landable and an actual place, i.e. not a hypergate or a wormhole */
// FUNCTION: EVNOVA 0x0046e440
// MAC: _StellarIsNormalLandable i386 0x0000a9f8 ppc 0x0000d164
Boolean StellarIsNormalLandable(SpobRecord *spob)
{
    if (StellarIsLandable(spob) && !SpobIsHyperOrWorm(spob))
        return 1;
    return 0;
}

// FUNCTION: EVNOVA 0x0046e790
// MAC: _FindSystemFromStellar i386 0x00007ec1 ppc 0x00009cd4
short FindSystemFromStellar(short stellarID)
{
    short i;
    short j;

    for (i = 0; i < kMaxSystems; i++) {
        if (systemData[i].loaded != 0) {
            for (j = 0; j < kNavSlots; j++) {
                if (systemData[i].navDefault[j] != -1
                        && stellarID == systemData[i].navDefault[j])
                    return i;
            }
        }
    }
    for (i = 0; i < kMaxSystems; i++) {
        for (j = 0; j < kNavSlots; j++) {
            if (systemData[i].navDefault[j] != -1
                    && stellarID == systemData[i].navDefault[j])
                return i;
        }
    }
    return -1;
}

// FUNCTION: EVNOVA 0x0046e860
// MAC: _PlayerBlessedByGovt i386 0x00002732 ppc 0x00003188
unsigned char PlayerBlessedByGovt(short govt, short which)
{
    if (govt < 0 || govt >= kMaxGovts)
        return 0;
    if (which < 0 || which > 1)
        return 0;
    return govtData[govt].blessesPlayer[which];
}

// FUNCTION: EVNOVA 0x0046ebb0
// MAC: _StellarNumHyperLinks i386 0x0000812a ppc 0x00009fc8
short StellarNumHyperLinks(SpobRecord *spob)
{
    short i;
    short count;

    if (spob == NULL)
        return 0;
    count = 0;
    for (i = 0; i < kSpobHyperLinks; i++) {
        if (spob->hyperLinks[i] >= 0 && spob->hyperLinks[i] < kMaxStellars)
            count = count + 1;
    }
    return count;
}

// FUNCTION: EVNOVA 0x0046efd0
// MAC: _StellarsAreIdentical i386 0x0000ab35 ppc 0x0000d334
Boolean StellarsAreIdentical(short a, short b)
{
    Str255 nameA;
    Str255 nameB;

    if (a >= 0 && a < kMaxStellars && b >= 0 && b < kMaxStellars) {
        if (a == b)
            return 1;
        if (stellarData[a].pos.h == stellarData[b].pos.h &&
            stellarData[a].pos.v == stellarData[b].pos.v &&
            (char)stellarData[a].name[0] == (char)stellarData[b].name[0]) {
            pstrcpy(nameA, stellarData[a].name);
            pstrcpy(nameB, stellarData[b].name);
            PtoCstr(nameA);
            PtoCstr(nameB);
            if (strcmp((char *)nameA, (char *)nameB) == 0)
                return 1;
        }
    }
    return 0;
}

EXTERN_C_END
