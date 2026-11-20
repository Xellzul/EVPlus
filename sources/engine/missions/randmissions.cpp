/* randmissions.cpp -- generating the offered missions */

#include "game.h"
#include "gameutil.h"
#include "crashlog.h"
#include "missions.h"
#include "stellar.h"
#include "prototypes.h"
#include "gamedialog.h"

// FUNCTION: EVNOVA 0x0043cf00
// MAC: _RandomizeMissionData i386 0x000a1756 ppc 0x000ad45c
void RandomizeMissionData(void)
{
    short i;
    short m;
    short venue;
    short count;
    short saved;
    short best;
    short next;
    short n;
    short j;
    short v;
    Boolean hit;
    short p;
    short k;

    PropagateMissionBitEffects();
    saved = missionsOutside;
    for (venue = 0; venue <= kMissionVenues - 1; venue++) {
        missionsOutside = venue;
        for (i = 0; i < kMaxMissions; i++)
            dockedPortMissions[venue][i] = -1;
        count = 0;
        for (m = 0; m < kMaxMissions; m++) {
            hit = 0;
            for (j = 0; j < kMaxActiveMissions; j++) {
                if (currentMissionObjectives[j].active != 0) {
                    if (currentMissions[j].misnIndex == m) {
                        hit = 1;
                        break;
                    }
                }
            }
            if (!hit) {
                if (IsMissionAvailable(m, 0)) {
                    dockedPortMissions[venue][count] = m;
                    count++;
                }
            }
        }
        for (i = 0; i < kMaxMissions; i++) {
            if (dockedPortMissions[venue][i] != -1)
                RandomizeOneMission(dockedPortMissions[venue][i]);
        }
    }
    missionsOutside = saved;

    for (i = 0; i < kMaxMissions; i++)
        tempMissions[i] = -1;

    for (venue = 0; venue <= kMissionVenues - 1; venue++) {
        n = 0;
        for (i = 0; i < kMaxMissions; i++) {
            if (dockedPortMissions[venue][i] != -1) {
                tempMissions[n] = dockedPortMissions[venue][i];
                dockedPortMissions[venue][i] = -1;
                if (missionAvailData[tempMissions[n]].dispWeight > best || n == 0)
                    best = missionAvailData[tempMissions[n]].dispWeight;
                n++;
            }
        }
        k = 0;
        do {
            n = 0;
            for (i = 0; i < kMaxMissions; i++) {
                v = tempMissions[i];
                if (v != -1) {
                    p = missionAvailData[v].dispWeight;
                    if (p >= best) {
                        dockedPortMissions[venue][k] = v;
                        k++;
                        tempMissions[i] = -1;
                    } else if (p > next || n == 0) {
                        next = p;
                    }
                    n++;
                }
            }
            best = next;
        } while (n > 0);
    }

    n = 0;
    for (i = 0; i < kMaxMissions; i++) {
        if (dockedPortMissions[1][i] != -1)
            n++;
    }
    if (n > 0)
        nextPortMission = 0;
    else
        nextPortMission = -1;
}

/*
 * The source and destination halves are NOT symmetric: the source stellar's
 * system goes through FindActiveCoLocatedSystem while the destination's is
 * read straight out of stellarData. The out-of-range arm skips the fallback,
 * which is why it is written as an else rather than as a pre-set.
 */
// FUNCTION: EVNOVA 0x0043d240
// MAC: _RandomizeOneMission i386 0x000a14d1 ppc 0x000ad1ac
void RandomizeOneMission(short mission)
{
    short i;
    short loc;

    if (offeringFromShip != 0) {
        loc = 0;
        for (i = 0; i < kNavSlots; i++) {
            if (systemData[shipData->curSystem].navDefault[i] != -1) {
                loc = systemData[shipData->curSystem].navDefault[i];
                break;
            }
        }
    } else {
        loc = shipData->targetStellar;
    }

    if (missionAvailData[mission].sourceStellar == -1) {
        briefMissionInfo[mission].destStellar = -1;
        briefMissionInfo[mission].destSystem = -1;
    } else {
        briefMissionInfo[mission].destStellar =
            SelectMissionStellar(missionAvailData[mission].sourceStellar, loc, -1, mission);
        if (briefMissionInfo[mission].destStellar >= 0
                && briefMissionInfo[mission].destStellar < kMaxStellars) {
            briefMissionInfo[mission].destSystem = FindActiveCoLocatedSystem(
                stellarData[briefMissionInfo[mission].destStellar].system);
            if (briefMissionInfo[mission].destSystem == -1)
                briefMissionInfo[mission].destSystem =
                    FindSystemFromStellar(briefMissionInfo[mission].destStellar);
        } else {
            briefMissionInfo[mission].destSystem = -1;
        }
    }

    if (missionAvailData[mission].destStellar == -1) {
        briefMissionInfo[mission].returnStellar = briefMissionInfo[mission].destStellar;
    } else {
        briefMissionInfo[mission].returnStellar =
            SelectMissionStellar(missionAvailData[mission].destStellar, loc,
                         briefMissionInfo[mission].destStellar, mission);
    }
    if (briefMissionInfo[mission].returnStellar >= 0
            && briefMissionInfo[mission].returnStellar < kMaxStellars) {
        briefMissionInfo[mission].returnSystem =
            stellarData[briefMissionInfo[mission].returnStellar].system;
        if (briefMissionInfo[mission].returnSystem == -1)
            briefMissionInfo[mission].returnSystem =
                FindSystemFromStellar(briefMissionInfo[mission].returnStellar);
    } else {
        briefMissionInfo[mission].returnSystem = -1;
    }

    briefMissionInfo[mission].cargoType = SelectMissionCargoType(missionAvailData[mission].cargoType);
    briefMissionInfo[mission].cargoUnits = SelectMissionCargoQty(missionAvailData[mission].cargoQty);
    briefMissionInfo[mission].payment = missionAvailData[mission].payment;
    SetMissionDeadline(&briefMissionInfo[mission].dueDate,
                  missionAvailData[mission].timeLimit);
}

// FUNCTION: EVNOVA 0x0043d490
// MAC: _SelectMissionCargoType i386 0x000981d8 ppc 0x000a3e68
short SelectMissionCargoType(short choice)
{
    if (choice >= 0 && choice < 1000)
        return choice;
    if (choice == 1000)
        return Rand(kCargoTypes);
    return -1;
}

// FUNCTION: EVNOVA 0x0043d4c0
// MAC: _SelectMissionCargoQty i386 0x00098191 ppc 0x000a3e00
short SelectMissionCargoQty(short choice)
{
    short spread;

    if (choice >= 0)
        return choice;
    if (choice <= -2) {
        spread = abs(choice);
        return Rand(spread) + spread / 2;
    }
    return 0;
}

#define kStellarNone        -1
#define kStellarRandom      -2
#define kStellarRandomUninh -3
#define kStellarNoneAlt     -4

#define kStellarGovt        10000
#define kStellarAllied      15000
#define kStellarNotGovt     20000
#define kStellarEnemy       25000
#define kStellarClass       30000
#define kStellarNotClass    31000

// FUNCTION: EVNOVA 0x0043d510
// MAC: _SelectMissionStellar i386 0x0009a26a ppc 0x000a609c
short SelectMissionStellar(short sel, short loc, short avoid, short mission)
{
    short i;
    short stellar;
    short count;
    Boolean allowUninhabited;
    Boolean ok;

    stellar = loc;
    if (sel == kStellarNone || sel == kStellarNoneAlt) {
        stellar = loc;
    } else {
        if (sel == kStellarRandom) {
            count = 0;
            for (i = 0; i < kMaxStellars; i++) {
                if (i != avoid && i != loc
                        && stellarData[i].loaded
                        && StellarIsNormalLandable(&stellarData[i])
                        && (stellarData[i].flags & kSpobUninhabited) == 0
                        && StellarsAreDistant(i, loc))
                    count++;
                if (count >= 1)
                    break;
            }
            if (count <= 0) {
                crashlog_print("warning: mission ");
                crashlog_print_num(mission + kFirstResourceID);
                crashlog_print(" failed to find a valid nontransient random stellar");
                if (loc != kStellarNone) {
                    crashlog_print(", using ");
                    crashlog_print_pstr(stellarData[loc].name);
                    crashlog_print(" instead");
                }
                crashlog_print("\n");
                return loc;
            } else {
                /* BUG: the distance term tests the scan's leftover i, not the rolled stellar */
                do {
                    stellar = Rand(kMaxStellars);
                } while (stellar == avoid || stellar == loc
                        || !stellarData[stellar].loaded
                        || !SystemVisible(stellarData[stellar].system)
                        || !StellarIsNormalLandable(&stellarData[stellar])
                        || (stellarData[stellar].flags & kSpobUninhabited) != 0
                        || (stellarData[stellar].flags & kSpobIsStation) != 0
                        || !StellarsAreDistant(i, loc));
            }
        }
        if (sel == kStellarRandomUninh) {
            count = 0;
            for (i = 0; i < kMaxStellars; i++) {
                if (i != avoid && i != loc
                        && stellarData[i].loaded
                        && SystemVisible(stellarData[i].system)
                        && (stellarData[i].flags & kSpobUninhabited) != 0
                        && StellarIsNormalLandable(&stellarData[i])
                        && (stellarData[i].flags & kSpobIsStation) == 0
                        && StellarsAreDistant(i, loc))
                    count++;
                if (count >= 1)
                    break;
            }
            if (count <= 0) {
                crashlog_print("warning: mission ");
                crashlog_print_num(mission + kFirstResourceID);
                crashlog_print(" failed to find a valid nontransient random uninhabited stellar");
                if (loc != kStellarNone) {
                    crashlog_print(", using ");
                    crashlog_print_pstr(stellarData[loc].name);
                    crashlog_print(" instead");
                }
                crashlog_print("\n");
                return loc;
            } else {
                do {
                    stellar = Rand(kMaxStellars);
                } while (stellar == avoid || stellar == loc
                        || !stellarData[stellar].loaded
                        || !SystemVisible(stellarData[stellar].system)
                        || (stellarData[stellar].flags & kSpobUninhabited) == 0
                        || !StellarIsNormalLandable(&stellarData[stellar])
                        || (stellarData[stellar].flags & kSpobIsStation) != 0
                        || !StellarsAreDistant(stellar, loc));
            }
        }
        if (sel > 0 && sel < kFirstResourceID) {
            crashlog_print("warning: mission ");
            crashlog_print_num(mission + kFirstResourceID);
            crashlog_print(" failed to find stellar with ID of ");
            crashlog_print_num(sel);
            crashlog_print("\n");
            return loc;
        }
        if (sel >= kFirstResourceID && sel < kFirstResourceID + kMaxStellars) {
            stellar = sel;
            stellar -= kFirstResourceID;
        }
        /* BUG: off by one, 9999 enters the govt band as if it were one */
        if (sel >= kStellarGovt - 1 && sel < kStellarAllied) {
            allowUninhabited = 0;
            if (sel >= kStellarGovt
                    && (govtData[sel - kStellarGovt].flags & kGovtAlwaysDisabled) != 0)
                allowUninhabited = 1;
            count = 0;
            for (i = 0; i < kMaxStellars; i++) {
                if (i != avoid && i != loc
                        && stellarData[i].loaded
                        && SystemVisible(stellarData[i].system)
                        && stellarData[i].govt == sel - kStellarGovt
                        && ((stellarData[i].flags & kSpobUninhabited) == 0 || allowUninhabited)
                        && StellarIsNormalLandable(&stellarData[i])
                        && StellarsAreDistant(i, loc))
                    count++;
                if (count >= 1)
                    break;
            }
            if (count <= 0) {
                crashlog_print("warning: mission ");
                crashlog_print_num(mission + kFirstResourceID);
                crashlog_print(" failed to find a valid nontransient random stellar of govt ");
                crashlog_print_num(sel - kStellarGovt + kFirstResourceID);
                if (loc != kStellarNone) {
                    crashlog_print(", using ");
                    crashlog_print_pstr(stellarData[loc].name);
                    crashlog_print(" instead");
                }
                crashlog_print("\n");
                return loc;
            } else {
                do {
                    stellar = Rand(kMaxStellars);
                } while (stellar == avoid || stellar == loc
                        || !stellarData[stellar].loaded
                        || !SystemVisible(stellarData[stellar].system)
                        || stellarData[stellar].govt != sel - kStellarGovt
                        || ((stellarData[stellar].flags & kSpobUninhabited) != 0 && !allowUninhabited)
                        || !StellarIsNormalLandable(&stellarData[stellar])
                        || !StellarsAreDistant(stellar, loc));
            }
        }
        if (sel >= kStellarAllied && sel < kStellarNotGovt) {
            count = 0;
            for (i = 0; i < kMaxStellars; i++) {
                ok = 0;
                if (stellarData[i].govt != -1
                        && stellarData[i].govt != sel - kStellarAllied) {
                    if (systemData[i].govt == sel - kStellarAllied)
                        ok = 1;
                    if (GovtAllies(stellarData[i].govt, sel - kStellarAllied))
                        ok = 1;
                }
                if (i != avoid && i != loc
                        && stellarData[i].loaded
                        && SystemVisible(stellarData[i].system)
                        && ok
                        && (stellarData[i].flags & kSpobUninhabited) == 0
                        && StellarIsNormalLandable(&stellarData[i])
                        && StellarsAreDistant(i, loc))
                    count++;
                if (count >= 1)
                    break;
            }
            if (count <= 0) {
                crashlog_print("warning: mission ");
                crashlog_print_num(mission + kFirstResourceID);
                crashlog_print(" failed to find a valid nontransient random stellar allied with govt ");
                crashlog_print_num(sel - kStellarAllied + kFirstResourceID);
                if (loc != kStellarNone) {
                    crashlog_print(", using ");
                    crashlog_print_pstr(stellarData[loc].name);
                    crashlog_print(" instead");
                }
                crashlog_print("\n");
                return loc;
            } else {
                do {
                    stellar = Rand(kMaxStellars);
                    ok = 0;
                    if (stellarData[stellar].govt != -1
                            && stellarData[stellar].govt != sel - kStellarAllied) {
                        if (systemData[stellar].govt == sel - kStellarAllied)
                            ok = 1;
                        if (GovtAllies(stellarData[stellar].govt, sel - kStellarAllied))
                            ok = 1;
                    }
                } while (stellar == avoid || stellar == loc
                        || !stellarData[stellar].loaded
                        || !SystemVisible(stellarData[stellar].system)
                        || !ok
                        || (stellarData[stellar].flags & kSpobUninhabited) != 0
                        || !StellarIsNormalLandable(&stellarData[stellar])
                        || !StellarsAreDistant(stellar, loc));
            }
        }
        if (sel >= kStellarNotGovt && sel < kStellarEnemy) {
            count = 0;
            for (i = 0; i < kMaxStellars; i++) {
                if (i != avoid && i != loc
                        && stellarData[i].loaded
                        && SystemVisible(stellarData[i].system)
                        && stellarData[i].govt != sel - kStellarNotGovt
                        && (stellarData[i].flags & kSpobUninhabited) == 0
                        && StellarIsNormalLandable(&stellarData[i])
                        && StellarsAreDistant(i, loc))
                    count++;
                if (count >= 1)
                    break;
            }
            if (count <= 0) {
                crashlog_print("warning: mission ");
                crashlog_print_num(mission + kFirstResourceID);
                crashlog_print(" failed to find a valid nontransient random stellar not of govt ");
                crashlog_print_num(sel - kStellarNotGovt + kFirstResourceID);
                if (loc != kStellarNone) {
                    crashlog_print(", using ");
                    crashlog_print_pstr(stellarData[loc].name);
                    crashlog_print(" instead");
                }
                crashlog_print("\n");
                return loc;
            }
            do {
                stellar = Rand(kMaxStellars);
            } while (stellar == avoid
                    || !stellarData[stellar].loaded
                    || !SystemVisible(stellarData[stellar].system)
                    || stellarData[stellar].govt == sel - kStellarNotGovt
                    || (stellarData[stellar].flags & kSpobUninhabited) != 0
                    || !StellarIsNormalLandable(&stellarData[stellar])
                    || !StellarsAreDistant(stellar, loc));
        }
        if (sel >= kStellarEnemy && sel < kStellarClass) {
            count = 0;
            for (i = 0; i < kMaxStellars; i++) {
                ok = 0;
                if (stellarData[i].govt != -1
                        && stellarData[i].govt != sel - kStellarEnemy) {
                    if (GovtEnemies(stellarData[i].govt, sel - kStellarEnemy))
                        ok = 1;
                }
                if (i != avoid && i != loc
                        && stellarData[i].loaded
                        && SystemVisible(stellarData[i].system)
                        && ok
                        && (stellarData[i].flags & kSpobUninhabited) == 0
                        && StellarIsNormalLandable(&stellarData[i])
                        && StellarsAreDistant(i, loc))
                    count++;
                if (count >= 1)
                    break;
            }
            if (count <= 0) {
                crashlog_print("warning: mission ");
                crashlog_print_num(mission + kFirstResourceID);
                crashlog_print(" failed to find a valid nontransient random stellar that is enemy of govt ");
                crashlog_print_num(sel - kStellarEnemy + kFirstResourceID);
                if (loc != kStellarNone) {
                    crashlog_print(", using ");
                    crashlog_print_pstr(stellarData[loc].name);
                    crashlog_print(" instead");
                }
                crashlog_print("\n");
                return loc;
            } else {
                do {
                    stellar = Rand(kMaxStellars);
                    ok = 0;
                    if (stellarData[stellar].govt != -1
                            && stellarData[stellar].govt != sel - kStellarEnemy) {
                        if (GovtEnemies(stellarData[stellar].govt, sel - kStellarEnemy))
                            ok = 1;
                    }
                } while (stellar == avoid || stellar == loc
                        || !stellarData[stellar].loaded
                        || !SystemVisible(stellarData[stellar].system)
                        || !ok
                        || (stellarData[stellar].flags & kSpobUninhabited) != 0
                        || !StellarIsNormalLandable(&stellarData[stellar])
                        || !StellarsAreDistant(stellar, loc));
            }
        }
        if (sel >= kStellarClass && sel < kStellarNotClass) {
            /* BUG: an exact govt match is rejected, so the band never picks the govt it names */
            count = 0;
            for (i = 0; i < kMaxStellars; i++) {
                ok = 0;
                if (stellarData[i].govt != -1
                        && stellarData[i].govt != sel - kStellarClass) {
                    if (GovtSharedClass(stellarData[i].govt, sel - kStellarClass))
                        ok = 1;
                }
                if (i != avoid && i != loc
                        && stellarData[i].loaded
                        && SystemVisible(stellarData[i].system)
                        && ok
                        && (stellarData[i].flags & kSpobUninhabited) == 0
                        && StellarIsNormalLandable(&stellarData[i])
                        && StellarsAreDistant(i, loc))
                    count++;
                if (count >= 1)
                    break;
            }
            if (count <= 0) {
                crashlog_print("warning: mission ");
                crashlog_print_num(mission + kFirstResourceID);
                crashlog_print(" failed to find a valid nontransient random stellar of govt ");
                crashlog_print_num(sel - kStellarClass + kFirstResourceID);
                crashlog_print(" or a govt of same class");
                if (loc != kStellarNone) {
                    crashlog_print(", using ");
                    crashlog_print_pstr(stellarData[loc].name);
                    crashlog_print(" instead");
                }
                crashlog_print("\n");
                return loc;
            } else {
                do {
                    stellar = Rand(kMaxStellars);
                    ok = 0;
                    if (stellarData[stellar].govt != -1
                            && stellarData[stellar].govt != sel - kStellarClass) {
                        if (GovtSharedClass(stellarData[stellar].govt, sel - kStellarClass))
                            ok = 1;
                    }
                } while (stellar == avoid || stellar == loc
                        || !stellarData[stellar].loaded
                        || !SystemVisible(stellarData[stellar].system)
                        || !ok
                        || (stellarData[stellar].flags & kSpobUninhabited) != 0
                        || !StellarIsNormalLandable(&stellarData[stellar])
                        || !StellarsAreDistant(stellar, loc));
            }
        }
        if (sel >= kStellarNotClass && sel < 32000) {
            count = 0;
            for (i = 0; i < kMaxStellars; i++) {
                ok = 0;
                if (stellarData[i].govt != -1
                        && stellarData[i].govt != sel - kStellarNotClass) {
                    if (!GovtSharedClass(stellarData[i].govt, sel - kStellarNotClass))
                        ok = 1;
                }
                if (i != avoid && i != loc
                        && stellarData[i].loaded
                        && SystemVisible(stellarData[i].system)
                        && ok
                        && (stellarData[i].flags & kSpobUninhabited) == 0
                        && StellarIsNormalLandable(&stellarData[i])
                        && StellarsAreDistant(i, loc))
                    count++;
                if (count >= 1)
                    break;
            }
            if (count <= 0) {
                crashlog_print("warning: mission ");
                crashlog_print_num(mission + kFirstResourceID);
                crashlog_print(" failed to find a valid nontransient random stellar not of govt ");
                crashlog_print_num(sel - kStellarNotClass + kFirstResourceID);
                crashlog_print(" or a govt of same class");
                if (loc != kStellarNone) {
                    crashlog_print(", using ");
                    crashlog_print_pstr(stellarData[loc].name);
                    crashlog_print(" instead");
                }
                crashlog_print("\n");
                return loc;
            }
            do {
                stellar = Rand(kMaxStellars);
                ok = 0;
                if (stellarData[stellar].govt != -1
                        && stellarData[stellar].govt != sel - kStellarNotClass) {
                    if (!GovtSharedClass(stellarData[stellar].govt,
                            sel - kStellarNotClass))
                        ok = 1;
                }
            } while (stellar == avoid || stellar == loc
                    || !stellarData[stellar].loaded
                    || !SystemVisible(stellarData[stellar].system)
                    || !ok
                    || (stellarData[stellar].flags & kSpobUninhabited) != 0
                    || !StellarIsNormalLandable(&stellarData[stellar])
                    || !StellarsAreDistant(stellar, loc));
        }
    }
    return stellar;
}

// FUNCTION: EVNOVA 0x0043e6f0
// MAC: _SelectMissionSystem i386 0x0009b993 ppc 0x000a7998
short SelectMissionSystem(short sel, short loc)
{
    short sys;
    short i;
    short count;
    Boolean ok;
    short link;

    sys = loc;
    if (sel == kSystemRandom) {
        count = 0;
        for (sys = 0; sys < kMaxSystems; sys++) {
            if (systemData[sys].loaded
                    && SystemVisible(sys)
                    && FindFirstCoLocatedSystem(sys)
                        != FindFirstCoLocatedSystem(shipData->curSystem))
                count++;
            if (count >= 1)
                break;
        }
        if (count <= 0) {
            return -1;
        } else {
            do {
                sys = Rand(kMaxSystems);
            } while (!systemData[sys].loaded
                    || !SystemVisible(sys)
                    || FindFirstCoLocatedSystem(sys)
                        == FindFirstCoLocatedSystem(shipData->curSystem));
        }
    } else if (sel == kSystemAdjacent) {
        do {
            i = Rand(kMaxSystemLinks);
        } while (systemData[loc].links[i] == -1
                || (link = FindActiveCoLocatedSystem(systemData[loc].links[i])) == -1
                || !SystemVisible(link));
        sys = link;
    } else if (sel >= kFirstResourceID && sel < kFirstResourceID + kMaxSystems) {
        sys = sel;
        sys -= kFirstResourceID;
    } else if (sel >= kSystemGovt - 1 && sel < kSystemAllied) {
        /* BUG: off by one, 9999 enters the govt band as if it were one */
        count = 0;
        for (i = 0; i < kMaxSystems; i++) {
            if (systemData[i].loaded
                    && SystemVisible(i)
                    && systemData[i].govt == sel - kSystemGovt
                    && FindFirstCoLocatedSystem(i)
                        != FindFirstCoLocatedSystem(shipData->curSystem))
                count++;
            if (count >= 1)
                break;
        }
        if (count <= 0) {
            return -1;
        } else {
            do {
                sys = Rand(kMaxSystems);
            } while (!systemData[sys].loaded
                    || !SystemVisible(sys)
                    || systemData[sys].govt != sel - kSystemGovt
                    || FindFirstCoLocatedSystem(sys)
                        == FindFirstCoLocatedSystem(shipData->curSystem));
        }
    } else if (sel >= kSystemAllied && sel < kSystemNotGovt) {
        count = 0;
        for (i = 0; i < kMaxSystems; i++) {
            ok = 0;
            if (GovtAllies(systemData[i].govt, sel - kSystemAllied))
                ok = 1;
            if (systemData[i].loaded
                    && SystemVisible(i)
                    && ok
                    && FindFirstCoLocatedSystem(i)
                        != FindFirstCoLocatedSystem(shipData->curSystem))
                count++;
            if (count >= 1)
                break;
        }
        if (count <= 0) {
            return -1;
        } else {
            do {
                sys = Rand(kMaxSystems);
                ok = 0;
                if (systemData[sys].govt >= 0
                        && systemData[sys].govt != sel - kSystemAllied) {
                    if (GovtAllies(systemData[sys].govt, sel - kSystemAllied))
                        ok = 1;
                }
            } while (!systemData[sys].loaded
                    || !SystemVisible(sys)
                    || !ok
                    || FindFirstCoLocatedSystem(sys)
                        == FindFirstCoLocatedSystem(shipData->curSystem));
        }
    } else if (sel >= kSystemNotGovt && sel < kSystemEnemy) {
        count = 0;
        for (i = 0; i < kMaxSystems; i++) {
            if (systemData[i].loaded
                    && SystemVisible(i)
                    && systemData[i].govt != sel - kSystemNotGovt
                    && FindFirstCoLocatedSystem(i)
                        != FindFirstCoLocatedSystem(shipData->curSystem))
                count++;
            if (count >= 1)
                break;
        }
        if (count <= 0) {
            return -1;
        } else {
            do {
                sys = Rand(kMaxSystems);
            } while (!systemData[sys].loaded
                    || !SystemVisible(sys)
                    || systemData[sys].govt == sel - kSystemNotGovt
                    || FindFirstCoLocatedSystem(sys)
                        == FindFirstCoLocatedSystem(shipData->curSystem));
        }
    } else if (sel >= kSystemEnemy && sel < kSystemClass) {
        count = 0;
        for (i = 0; i < kMaxSystems; i++) {
            ok = 0;
            if (GovtEnemies(systemData[i].govt, sel - kSystemEnemy))
                ok = 1;
            if (systemData[i].loaded
                    && SystemVisible(i)
                    && ok
                    && FindFirstCoLocatedSystem(i)
                        != FindFirstCoLocatedSystem(shipData->curSystem))
                count++;
            if (count >= 1)
                break;
        }
        if (count <= 0) {
            return -1;
        } else {
            do {
                sys = Rand(kMaxSystems);
                ok = 0;
                if (GovtEnemies(systemData[sys].govt, sel - kSystemEnemy))
                    ok = 1;
            } while (!systemData[sys].loaded
                    || !SystemVisible(sys)
                    || !ok
                    || FindFirstCoLocatedSystem(sys)
                        == FindFirstCoLocatedSystem(shipData->curSystem));
        }
    } else if (sel >= kSystemClass && sel < kSystemNotClass) {
        count = 0;
        for (i = 0; i < kMaxSystems; i++) {
            ok = 0;
            if (GovtSharedClass(systemData[i].govt, sel - kSystemClass))
                ok = 1;
            if (systemData[i].loaded
                    && SystemVisible(i)
                    && ok
                    && FindFirstCoLocatedSystem(i)
                        != FindFirstCoLocatedSystem(shipData->curSystem))
                count++;
            if (count >= 1)
                break;
        }
        if (count <= 0) {
            return -1;
        } else {
            do {
                sys = Rand(kMaxSystems);
                ok = 0;
                if (GovtSharedClass(systemData[sys].govt, sel - kSystemClass))
                    ok = 1;
            } while (!systemData[sys].loaded
                    || !SystemVisible(sys)
                    || !ok
                    || FindFirstCoLocatedSystem(sys)
                        == FindFirstCoLocatedSystem(shipData->curSystem));
        }
    } else if (sel >= kSystemNotClass && sel < kSystemBandLimit) {
        count = 0;
        for (i = 0; i < kMaxSystems; i++) {
            ok = 0;
            if (!GovtSharedClass(systemData[i].govt, sel - kSystemNotClass))
                ok = 1;
            if (systemData[i].loaded
                    && SystemVisible(i)
                    && ok
                    && FindFirstCoLocatedSystem(i)
                        != FindFirstCoLocatedSystem(shipData->curSystem))
                count++;
            if (count >= 1)
                break;
        }
        if (count <= 0) {
            return -1;
        } else {
            do {
                sys = Rand(kMaxSystems);
                ok = 0;
                if (!GovtSharedClass(systemData[sys].govt, sel - kSystemNotClass))
                    ok = 1;
            } while (!systemData[sys].loaded
                    || !SystemVisible(sys)
                    || !ok
                    || FindFirstCoLocatedSystem(sys)
                        == FindFirstCoLocatedSystem(shipData->curSystem));
        }
    }
    return sys;
}

/* the date `days` days from today; nothing is written for a non-positive count */

// FUNCTION: EVNOVA 0x0043f080
// MAC: _SetMissionDeadline i386 0x00098226 ppc 0x000a3ec4
void SetMissionDeadline(DateTimeRec *out, short days)
{
    DateTimeRec d;
    short i;

    d.month = gameTime.month;
    d.day = gameTime.day;
    d.year = gameTime.year;
    if (days <= 0)
        return;
    for (i = 0; i < days; i++)
        IncrementDate(&d, 0);
    out->month = d.month;
    out->day = d.day;
    out->year = d.year;
}
