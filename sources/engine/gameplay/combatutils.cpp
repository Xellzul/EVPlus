/* combatutils.cpp -- targeting, weapon, AI and spawn helpers, ship colour codes */

#include "burgermath.h"
#include "ai.h"
#include "game.h"
#include "player.h"
#include "ships.h"
#include "gameutil.h"
#include "space.h"
#include "prototypes.h"
#include "stellar.h"

#define kRoidYieldBase   0x32
#define kRoidYieldSpread 0x65

EXTERN_C_BEGIN

// FUNCTION: EVNOVA 0x00462550
// MAC: _DestroyAsteroid i386 0x0000adc9 ppc 0x0000d688
void DestroyAsteroid(AsteroidObj *roid)
{

    short expl[2];
    short i;
    short n;
    short spread;

    spread = asteroidType[roid->type].fragCount;
    expl[0] = asteroidType[roid->type].fragType[0];
    expl[1] = asteroidType[roid->type].fragType[1];

    if (asteroidType[roid->type].yieldQty > 0) {
        n = (asteroidType[roid->type].yieldQty * (Rand(kRoidYieldSpread) + kRoidYieldBase) * 0.01);
        if (n > 0) {
            for (i = 0; i < n; i++)
                SpawnScoopableBox(&roid->pos,
                                  asteroidType[roid->type].yieldType,
                                  (roid->type >> 2) + 1);
        }
    }
    if (asteroidType[roid->type].partCount > 0) {
        SpawnParticles(roid->pos.x, roid->pos.y, 0.2f, 0x28, 0xf0,
                     0x1e0, asteroidType[roid->type].color, 0x20,
                     asteroidType[roid->type].partCount, GetSpriteXSize(roid->sprite) / 3);
    }
    if (asteroidType[roid->type].explodType != -1) {
        CreateExplosion(roid->pos,
                        asteroidType[roid->type].explodType, 0, 1);
    }
    if (spread > 0) {
        n = Rand(spread) + spread / 2;
        if (n > 0) {
            for (i = 0; i < n; i++) {
                if (expl[0] != -1 && expl[1] != -1)
                    SpawnSubAsteroid(&roid->pos, expl[Rand(2)]);
                else if (expl[0] != -1)
                    SpawnSubAsteroid(&roid->pos, expl[0]);
                else if (expl[1] != -1)
                    SpawnSubAsteroid(&roid->pos, expl[1]);
            }
        }
    }
    roid->active = 0;
    SWSetSpriteVisible(roid->sprite, 0);
}

// FUNCTION: EVNOVA 0x00462850
// MAC: _FindNearestShipToPlayer i386 0x0000923f ppc 0x0000b3e8
short FindNearestShipToPlayer(void)
{
    short i;
    short best;
    Boolean hasDetect2;
    float bestDist;
    float dx;
    float dy;
    float dist;
    Boolean hasDetect1;

    hasDetect1 = ShipCanTargetUntargetableShips(shipData);
    hasDetect2 = ShipCanTargetCloakedShips(shipData);
    best = -1;
    bestDist = -1.0f;
    for (i = 0; i < kMaxShips; i++) {
        if (i == 0)
            continue;
        if (!shipData[i].active)
            continue;
        if (shipData[i].parentShip == 0)
            continue;
        if (IsDying(&shipData[i]))
            continue;
        if (ShipIsCloaked(&shipData[i]) && !hasDetect2)
            continue;
        if (shipData[i].curSystem != shipData[0].curSystem)
            continue;
        if (AIShipIsEmergingFromHypergate(&shipData[i]))
            continue;
        if (!hasDetect1 && (shipClass[shipData[i].type].flags2 & kShipUntargetable) != 0)
            continue;

        dx = FloatAbs(shipData[0].pos.x - shipData[i].pos.x);
        dy = FloatAbs(shipData[0].pos.y - shipData[i].pos.y);
        dist = dx * dx + dy * dy;
        if (dist < bestDist || bestDist < 0.0f) {
            bestDist = dist;
            best = i;
        }
    }
    return best;
}

// FUNCTION: EVNOVA 0x004629e0
// MAC: _IsThreatToStellar i386 0x00009ef7 ppc 0x0000c488
Boolean IsThreatToStellar(ShipObj *ship, SpobRecord *spob)
{
    short target;
    Boolean found;

    if (spob->dominated != 0)
        return 0;

    if (ship->index != 0) {
        if (AIIsShipJumpingIn(ship) != 0)
            return 0;
    }

    if (spob->flags2 & kSpobFiresWhenProvoked) {
        if (spob->defenseActive != 0) {
            if (ship->index == 0 || ship->parentShip == 0)
                return 1;
        }
        goto fail;
    }

    if (ship->index == 0 || ship->parentShip == 0) {
        target = -1;
        if (spob->govt != -1) {
            target = spob->govt;
        } else {
            if (systemData[shipData->curSystem].govt != -1)
                target = systemData[shipData->curSystem].govt;
        }

        found = 0;
        if (target != -1) {
            if (playerRecord[shipData->curSystem] < -govtData[target].crimeTol)
                found = 1;
            if (!found) {
                if (govtData[target].flags & kGovtXenophobic) {
                    if (playerRecord[shipData->curSystem] < 0)
                        found = 1;
                } else {
                    if (GovtEnemies(shipClass[shipData->type].inherentGovt, spob->govt))
                        found = 1;
                }
            }
            if (found) {
                if (PlayerBlessedByGovt(spob->govt, 0))
                    found = 0;
                if (govtData[target].iffScrambled != 0)
                    found = 0;
            }
        }

        if (dockTarget == spob->index)
            found = 0;

        if (found)
            return 1;
        goto fail;
    }
    if (GovtEnemies(spob->govt, ship->govt))
        return 1;

fail:
    return 0;
}

#define kGateRange2 250000.0

// FUNCTION: EVNOVA 0x00462bd0
// MAC: _FindNearestThreatToPlayer i386 0x000093f9 ppc 0x0000b5f8
short FindNearestThreatToPlayer(void)
{
    short i;
    short best;
    short target;
    Boolean hasDetect1;
    Boolean hasDetect2;
    Boolean valid;
    float bestDist;
    float dx;
    float dy;
    float dist;

    hasDetect1 = ShipCanTargetUntargetableShips(shipData);
    hasDetect2 = ShipCanTargetCloakedShips(shipData);
    best = -1;
    bestDist = -1.0f;
    for (i = 0; i < kMaxShips; i++) {
        if (i == 0)
            continue;
        if (!shipData[i].active)
            continue;
        if (IsDying(&shipData[i]))
            continue;
        if (IsDisabled(&shipData[i]))
            continue;
        if (ShipIsCloaked(&shipData[i]) && !hasDetect2)
            continue;
        if (shipData[i].parentShip == 0)
            continue;
        if (shipData[i].curSystem != shipData[0].curSystem)
            continue;
        if (!hasDetect1 && (shipClass[shipData[i].type].flags2 & kShipUntargetable) != 0)
            continue;

        valid = 0;
        if (IsThreatToPlayer(&shipData[i])) {
            valid = 1;
        } else {
            target = shipData[i].targetShip;
            if (target != -1 && shipData[target].parentShip == 0 && shipData[target].active
                    && IsShipFightingMe(&shipData[i], &shipData[target])) {
                valid = 1;
            }
        }
        if (valid) {
            dx = FloatAbs(shipData[0].pos.x - shipData[i].pos.x);
            dy = FloatAbs(shipData[0].pos.y - shipData[i].pos.y);
            dist = dx * dx + dy * dy;
            if (dist < bestDist || bestDist < 0.0f) {
                bestDist = dist;
                best = i;
            }
        }
    }
    return best;
}

// FUNCTION: EVNOVA 0x00462db0
// MAC: _FindNearestStellar i386 0x0000276c ppc 0x000031c8
short FindNearestStellar(ShipObj *ship)
{
    short i;
    short stellar;
    short best;
    Boolean inRange;
    float bestDist;
    float dx;
    float dy;
    float dist;

    best = -1;
    bestDist = -1.0f;
    for (i = 0; i < kNavSlots; i++) {
        stellar = systemData[ship->curSystem].navDefault[i];
        if (stellar != -1 && stellarData[stellar].loaded != 0
            && SpobCanLand(&stellarData[stellar])) {
            dx = FloatAbs(ship->pos.x - stellarData[stellar].pos.h);
            dy = FloatAbs(ship->pos.y - stellarData[stellar].pos.v);
            dist = dx * dx + dy * dy;
            if (SpobIsHyperOrWorm(&stellarData[stellar])) {
                if (dist < kGateRange2)
                    inRange = 1;
                else
                    inRange = 0;
            } else
                inRange = 1;
            if (inRange && (dist < bestDist || bestDist < 0.0f)) {
                bestDist = dist;
                best = stellar;
            }
        }
    }
    return best;
}

// FUNCTION: EVNOVA 0x00463520
// MAC: _XYShortToFloat i386 0x000028c3 ppc 0x000033ac
FloatPoint XYShortToFloat(ShortPoint pt)
{
    FloatPoint f;

    f.x = pt.h;
    f.y = pt.v;
    return f;
}

// FUNCTION: EVNOVA 0x00463dc0
// MAC: _SuitableMissileType i386 0x000034e1 ppc 0x00004400
Boolean SuitableMissileType(short weap, ShipObj *ship)
{
    short turns;

    turns = ShipTurnRate(ship);
    if ((weaponStats[weap].flags & kWeapNoFastTargets) != 0)
        if (turns > 3)
            return 0;
    if (weaponStats[weap].guidedTurn <= 2.0f)
        if (turns > 3)
            return 0;
    return 1;
}

// FUNCTION: EVNOVA 0x00465f00
// MAC: _ColorCodeShip i386 0x00004ae8 ppc 0x00005f6c
void ColorCodeShip(ShipObj *ship, RGBColor *color)
{
    short parent;

    if (ship->index == 0) {
        color->red = 0;
        color->green = 0xffff;
        color->blue = 0xffff;
        return;
    }
    if (IsDisabled(ship)) {
        *color = rgbDarkGrey;
    } else if (IsThreatToPlayer(ship)) {
        color->red = 0xffff;
        color->green = 0;
        color->blue = 0;
    } else {
        parent = ship->parentShip;
        if (parent == 0 && ship->defenseStellar == -1) {
            *color = brightGreen;
        } else if (parent != -1) {
            if (shipData[parent].parentShip == 0) {
                *color = brightGreen;
            } else {
                color->red = 0;
                color->green = 0;
                color->blue = 0xffff;
            }
        } else {
            color->red = 0;
            color->green = 0;
            color->blue = 0xffff;
        }
    }
}

/*
 * Disabled is three unrelated things at once: the government marks all its
 * ships that way, a rescue mission holds this ship disabled, or the armour
 * has fallen below the threshold - a tenth for ship types carrying flag 0x10,
 * otherwise a third.
 */

// FUNCTION: EVNOVA 0x004687b0
// MAC: _IsDisabled i386 0x00002ce6 ppc 0x0000397c
Boolean IsDisabled(ShipObj *ship)
{
    short govt;
    short mission;

    govt = ship->govt;
    if (govt != -1 && (govtData[govt].flags & kGovtAlwaysDisabled) != 0)
        return 1;
    if (ship->index > 0) {
        mission = ship->specialMission;
        if (mission != -1 && currentMissionObjectives[mission].active != '\0'
                && currentMissions[mission].shipGoal == kMissionGoalRescue
                && currentMissionObjectives[mission].objectivesDone == '\0'
                && ship->boarded == '\0')
            return 1;
    }
    if (ship->index > 0 && ship->defenseStellar != -1)
        return 0;
    if ((shipClass[ship->type].flags & kShipArmorDisableTenth) != 0) {
        if (ship->armor * 100.0f < ShipArmorCapacity(ship) * 10.0)
            return 1;
    } else {
        if (ship->armor * 100.0f < ShipArmorCapacity(ship) * 33.333)
            return 1;
    }
    return 0;
}

// FUNCTION: EVNOVA 0x004688e0
// MAC: _IsDying i386 0x0000253d ppc 0x00002ef0
unsigned char IsDying(ShipObj *ship)
{
    if (ship->deathTimer > 0.0f)
        return 1;
    if (ship->armor <= 0.0f)
        return 1;
    return 0;
}

// FUNCTION: EVNOVA 0x00468990
// MAC: _WeaponHasAmmo i386 0x0000b95a ppc 0x0000e53c
Boolean WeaponHasAmmo(ShipObj *ship, short weaponIndex)
{
    short ammoType;

    if (ship->index != 0
            && (weaponStats[weaponIndex].flags2 & kWeapFlagAIRestricted) != 0)
        return 0;
    if (ShipIsCloaked(ship) && (weaponStats[weaponIndex].flags2 & kWeapFireWhileCloaked) == 0)
        return 0;
    if ((weaponStats[weaponIndex].flags2 & kWeapNeedsKeyCarried) != 0 && !KeyCarriedShipTypeOnboard(ship))
        return 0;
    if (weaponStats[weaponIndex].guidance == kGuidanceBay) {
        if (ship->weap[weaponIndex].ammo <= 0)
            return 0;
    } else {
        ammoType = weaponStats[weaponIndex].ammoType;
        if (ammoType >= 0 && ammoType < kMaxWeapTypes) {
            if (ship->index == 0) {
                if (ship->weap[ammoType].ammo <= 0)
                    return 0;
            } else if (ship->weap[weaponIndex].ammo <= 0)
                return 0;
        } else if (ammoType <= -kWeapAmmoFuelBase
                   && ship->fuel < (abs(ammoType) - kWeapAmmoFuelBase) * 0.1)
            return 0;
    }
    return 1;
}

/* the shorter way round a 360-unit circle */

// FUNCTION: EVNOVA 0x0046b210
// MAC: _AngularDifference i386 0x0000649e ppc 0x00007d40
short AngularDifference(short a, short b)
{
    short diff;

    diff = abs(a - b);
    if ((a >= 0xb4) != (b >= 0xb4))
        diff = 0x168 - diff;
    if (diff > 0xb4)
        diff = 0x168 - diff;
    return diff;
}

// FUNCTION: EVNOVA 0x0046b260
// MAC: _AIHasAfterburner i386 0x000064f5 ppc 0x00007d80
Boolean AIHasAfterburner(ShipObj *ship)
{
    short i;
    short flags;

    if (ship->index != 0) {
        for (i = 1; i < kMaxShips; i++) {
            if (i != ship->index && shipData[i].active != 0
                    && ship->index == shipData[i].swarmLeader)
                return 0;
        }
    }
    flags = shipClass[ship->type].flags;
    if ((flags & kShipPlanetType) != 0)
        return 0;
    if ((flags & kShipAlwaysAfterburner) != 0)
        return 1;
    if ((flags & kShipAfterburnerIfRated) != 0
            && playerKills / shipClass[0].strength
               >= Rand(0x540) + 0x100)
        return 1;
    return 0;
}

/* true for all but the top few hundred of the 0x540 range */
// FUNCTION: EVNOVA 0x0046b330
// MAC: _AIPerformsComplexManeuvers i386 0x0000659a ppc 0x00007e78
Boolean AIPerformsComplexManeuvers(ShipObj *ship)
{
    if (playerKills >= Rand(0x540) + 0x100)
        return 1;
    return 0;
}

// FUNCTION: EVNOVA 0x0046b360
// MAC: _TurretBlindSpot i386 0x0000b325 ppc 0x0000ddb8
unsigned char TurretBlindSpot(ShipObj *ship, short heading, short weap)
{
    short facing;
    short diff;
    unsigned char blocked;

    blocked = 0;
    facing = ship->heading;
    diff = AngularDifference(facing, heading);
    if (diff <= 0x2d) {
        if (weaponStats[weap].flags & kWeapTurretBlindFront)
            blocked = 1;
        if (shipClass[ship->type].flags & kShipTurretBlindFront)
            blocked = 1;
    } else if (diff <= 0x87) {
        if (weaponStats[weap].flags & kWeapTurretBlindSide)
            blocked = 1;
        if (shipClass[ship->type].flags & kShipTurretBlindSide)
            blocked = 1;
    } else {
        if (weaponStats[weap].flags & kWeapTurretBlindRear)
            blocked = 1;
        if (shipClass[ship->type].flags & kShipTurretBlindRear)
            blocked = 1;
    }
    return blocked;
}

#define kDudeSlots 16

// FUNCTION: EVNOVA 0x0046b4b0
// MAC: _SelectShipFieldFromDude i386 0x000065c2 ppc 0x00007ebc
short SelectShipFieldFromDude(DudeRecord *dude, unsigned char allowAll)
{
    short cum[kDudeSlots];
    short pick;
    short i;
    short count;
    short total;
    short roll;
    short j;

    total = 0;
    pick = -1;
    count = 0;
    for (i = 0; i < kDudeSlots; i++) {
        cum[i] = 0;
        if (dude->shipTypes[i] != -1
            && (shipClass[dude->shipTypes[i]].canAppear != 0 || allowAll != 0)) {
            total += dude->shipProbs[i];
            for (j = 0; j <= i; j++) {
                if (dude->shipTypes[j] != -1
                    && (shipClass[dude->shipTypes[j]].canAppear != 0
                        || allowAll != 0)) {
                    cum[i] += dude->shipProbs[j];
                    count++;
                }
            }
        }
    }
    if (total <= 0 || count == 0)
        return -1;
    while (pick == -1) {
        roll = Rand(total) + 1;
        for (i = kDudeSlots - 1; i >= 0; i--) {
            if (roll <= cum[i] && dude->shipTypes[i] != -1
                && (shipClass[dude->shipTypes[i]].canAppear != 0
                    || allowAll != 0))
                pick = i;
        }
    }
    return pick;
}

#define kSystDudeSlots 8

// FUNCTION: EVNOVA 0x0046b600
// MAC: _SelectDudeFieldFromSystem i386 0x0000674d ppc 0x00008070
short SelectDudeFieldFromSystem(SystRecord *sys)
{
    short cum[kSystDudeSlots];
    short pick;
    short i;
    short roll;
    short j;
    short total;

    total = 0;
    pick = -1;
    for (i = 0; i < kSystDudeSlots; i++) {
        cum[i] = 0;
        if (sys->dudeTypes[i] >= 0 && sys->dudeTypes[i] < kMaxDudes) {
            total += sys->dudeProbs[i];
            for (j = 0; j <= i; j++) {
                if (sys->dudeTypes[j] >= 0 && sys->dudeTypes[j] < kMaxDudes)
                    cum[i] += sys->dudeProbs[j];
            }
        }
    }
    if (total <= 0)
        return -1;
    while (pick == -1) {
        roll = Rand(total) + 1;
        for (i = kSystDudeSlots - 1; i >= 0; i--) {
            if (roll <= cum[i] && sys->dudeTypes[i] >= 0
                && sys->dudeTypes[i] < kMaxDudes)
                pick = i;
        }
    }
    return pick;
}

// FUNCTION: EVNOVA 0x0046b6d0
// MAC: _SelectFleetFromSystem i386 0x0000682c ppc 0x00008198
short SelectFleetFromSystem(SystRecord *syst)
{
    short i;
    short picks[8];
    short probs[8];
    short g;
    short total;
    short k;

    if (syst->fleetCount <= 0)
        return -1;

    g = 0;
    total = 0;
    for (i = 0; i < syst->fleetCount; i++) {
        if (fleetData[syst->fleets[i]].leadShipType != -1 && fleetData[syst->fleets[i]].usable != 0) {
            picks[g] = syst->fleets[i];
            probs[g] = syst->fleetProbs[i];
            total += syst->fleetProbs[i];
            for (k = 0; k < g; k++)
                probs[g] += probs[k];
            g++;
        }
    }

    if (g == 0 || total == 0)
        return -1;

    k = Rand(total) + 1;
    for (i = 0; i < g; i++) {
        if (k <= probs[i])
            return picks[i];
    }
    return -1;
}

/* the shipyard's asking-price jitter, as a fraction around 1.00 */

// FUNCTION: EVNOVA 0x0046b870
// MAC: _RandomSkillLevel i386 0x00006922 ppc 0x000082e0
float RandomSkillLevel(short type)
{
    return (Rand(shipClass[type].skillVar * 2 + 1)
            + (100 - shipClass[type].skillVar)) * 0.01;
}

/* the player, or something escorting the player directly or one level down */
// FUNCTION: EVNOVA 0x0046b8d0
// MAC: _AlliedWithPlayer i386 0x00006985 ppc 0x00008378
Boolean AlliedWithPlayer(ShipObj *ship)
{
    short parent;

    if (ship->index == 0)
        return 1;
    parent = ship->parentShip;
    if (parent == 0)
        return 1;
    if (parent > 0 && parent < kMaxShips && shipData[parent].parentShip == 0)
        return 1;
    return 0;
}

// FUNCTION: EVNOVA 0x0046ba30
// MAC: _FindNearestValidShotTarget i386 0x00006a3a ppc 0x00008444
short FindNearestValidShotTarget(ShotObj *shot)
{
    short i;
    short best;
    short bestDist;
    short dx;
    short dy;
    short dist;

    best = -1;
    for (i = 0; i < kMaxShips; i++) {
        if (shipData[i].active != 0 &&
            shipData[i].curSystem == shipData->curSystem &&
            ShotCanHitShip(shot, &shipData[i])) {
            dx = abs((int32_t)shipData[i].pos.x - (int32_t)shot->pos.x);
            dy = abs((int32_t)shipData[i].pos.y - (int32_t)shot->pos.y);
            dist = dx * dx + dy * dy;
            if (dist < bestDist || best == -1) {
                best = i;
                bestDist = dist;
            }
        }
    }
    return best;
}

// FUNCTION: EVNOVA 0x0046c2f0
// MAC: _WeaponDisableOnly i386 0x0000704c ppc 0x00008ad0
Boolean WeaponDisableOnly(short weap)
{
    if ((weaponStats[weap].flags2 & kWeapNotDestroying) != 0)
        return 1;
    return 0;
}

// FUNCTION: EVNOVA 0x0046c320
// MAC: _ModifyShotStartPosition i386 0x00007348 ppc 0x00008df0
short ModifyShotStartPosition(ShipObj *ship, FloatPoint *base, short weapType, FloatPoint *target)
{
    short group;
    short divisor;
    short angle;
    short result;

    if (ship == 0 || base == 0)
        return -1;

    result = -1;
    group = weaponStats[weapType].exitType;
    if (group >= 0 && group <= 3) {
        angle = ship->sprite->curFrameIndex % shipClass[ship->type].framesPer;
        angle *= 360.0 / (float)shipClass[ship->type].framesPer;

        if ((weaponStats[weapType].flags3 & kWeapClosestExit) && target != 0) {
            ship->weapGroupLane[group] = SelectClosestShotStartPosition(
                ship, base, weapType, angle, group, target);
        }

        if (ship->weapGroupLane[group] < 0 || ship->weapGroupLane[group] > 3)
            ship->weapGroupLane[group] = Rand(4);

        ModifyShotStartPosition2(ship, base, angle, group, ship->weapGroupLane[group]);

        result = ship->weapGroupLane[group];
        ship->weapGroupLane[group] = (ship->weapGroupLane[group] + 1) % 4;

        if (ship->weapGroupLane[group] < 0 || ship->weapGroupLane[group] > 3)
            ship->weapGroupLane[group] = Rand(4);
    }

    return result;
}

// FUNCTION: EVNOVA 0x0046c4e0
// MAC: _SelectClosestShotStartPosition i386 0x00007254 ppc 0x00008d10
short SelectClosestShotStartPosition(ShipObj *ship, FloatPoint *base, short unused,
                                    short weapType, short mount,
                                    FloatPoint *target)
{
    short best;
    short i;
    FloatPoint pos;
    FloatPoint start;
    float d;
    float bestDist;

    if (ship == 0 || base == 0 || target == 0 || mount < 0 || mount > 3)
        return 0;
    start.x = base->x;
    start.y = base->y;
    best = -1;
    for (i = 0; i <= 3; i++) {
        pos.x = start.x;
        pos.y = start.y;
        ModifyShotStartPosition2(ship, &pos, weapType, mount, i);
        d = XYFloatSquaredDist(&pos, target);
        if (d < bestDist || best == -1) {
            best = i;
            bestDist = d;
        }
    }
    return best;
}

/*
 * Added to the caller's point rather than replacing it, which is what lets the
 * caller run all four lanes from one base.
 */
// FUNCTION: EVNOVA 0x0046c5c0
// MAC: _ModifyShotStartPosition2 i386 0x0000706e ppc 0x00008af0
void ModifyShotStartPosition2(ShipObj *ship, FloatPoint *pos, short angle,
                              short mount, short lane)
{
    FloatPoint d;

    if (ship == 0 || pos == 0 || mount < 0 || mount > 3 || lane < 0 || lane > 3)
        return;

    d.x = d.y = 0;
    Accel(angle, shipClass[ship->type].weapPos[mount][lane].v, &d);
    Accel(((angle + 90) % 360),
          shipClass[ship->type].weapPos[mount][lane].h, &d);

    if (d.y < 0.0) {
        d.x = d.x * shipClass[ship->type].compress[0].x;
        d.y = d.y * shipClass[ship->type].compress[0].y;
    } else {
        d.x = d.x * shipClass[ship->type].compress[1].x;
        d.y = d.y * shipClass[ship->type].compress[1].y;
    }
    d.y = d.y - shipClass[ship->type].weapPosZ[mount][lane];

    pos->x = pos->x + d.x;
    pos->y = pos->y + d.y;
}

// FUNCTION: EVNOVA 0x0046d190
// MAC: _ShipsShareParent i386 0x00007926 ppc 0x00009588
Boolean ShipsShareParent(ShipObj *a, ShipObj *b)
{
    short rootA;
    short rootB;
    short next;

    if (a == 0 || b == 0)
        return 0;
    rootA = a->index;
    while (1) {
        if (shipData[rootA].active == '\0')
            break;
        next = shipData[rootA].parentShip;
        if (next < 0)
            break;
        if (next >= kMaxShips)
            break;
        if (rootA == next)
            break;
        rootA = next;
    }
    rootB = b->index;
    while (1) {
        if (shipData[rootB].active == '\0')
            break;
        next = shipData[rootB].parentShip;
        if (next < 0)
            break;
        if (next >= kMaxShips)
            break;
        if (rootB == next)
            break;
        rootB = next;
    }
    if (rootA == rootB)
        return 1;
    return 0;
}

/*
 * The distances are taken in hundredths and folded to positive by multiplying
 * by -1 rather than negating, and the squared distance is floored at 30 so a
 * near-coincident pair cannot produce an unbounded push.
 */

// FUNCTION: EVNOVA 0x0046e2f0
// MAC: _ApplyGravity i386 0x00007c5a ppc 0x00009a24
void ApplyGravity(FloatPoint *a, FloatPoint *b, float force, FloatPoint *out)
{
    float dx;
    float dy;
    float dist;
    dx = (a->x - b->x) * 0.01;
    dy = (a->y - b->y) * 0.01;
    if (dx < 0)
        dx *= -1.0;
    if (dy < 0)
        dy *= -1.0;
    dist = dx * dx + dy * dy;
    if (dist < 30)
        dist = 30;
    Accel(CalcInterceptAngle(a, b), force / dist, out);
}

#define kNeutralComponent 0x20

// FUNCTION: EVNOVA 0x0046e470
// MAC: _GetShipPaintColor i386 0x00007d0c ppc 0x00009adc
void GetShipPaintColor(ShipObj *ship, unsigned short *red, unsigned short *green,
                  unsigned short *blue)
{
    RGBColor c;

    c.red = c.green = c.blue = kNeutralComponent;
    if (ship->index == 0) {
        c = playerPaint;
    } else if (ship->persType != -1) {
        c = personData[ship->persType].shipColor;
    } else if (ship->govt != -1) {
        c = govtData[ship->govt].rgb2;
    }
    if (c.red == 0 && c.green == 0 && c.blue == 0)
        c.red = c.green = c.blue = kNeutralComponent;
    *red = c.red;
    *green = c.green;
    *blue = c.blue;
}

/* mac _PickEmergeStellar. */

// FUNCTION: EVNOVA 0x0046e9e0
// MAC: _PickEmergeStellar i386 0x00007f9a ppc 0x00009db8
short PickEmergeStellar(ShipObj *ship)
{
    short spob;

    spob = SelectRandomStellarDest(ship, Rand(3) == 0 ? true : false, 0);
    if (spob >= 0 && spob < kMaxStellars && SpobIsHyperOrWorm(&stellarData[spob]))
        return spob;
    return -1;
}

/* the ships of some governments always read as disabled */

// FUNCTION: EVNOVA 0x0046f100
// MAC: _ShipIsDerelict i386 0x00008225 ppc 0x0000a128
Boolean ShipIsDerelict(ShipObj *ship)
{
    short govt;

    govt = ship->govt;
    if (govt >= 0 && govt < kMaxGovts && (govtData[govt].flags & kGovtAlwaysDisabled) != 0)
        return 1;
    return 0;
}

// FUNCTION: EVNOVA 0x0046f1e0
// MAC: _AddToPlayerKills i386 0x000082dc ppc 0x0000a22c
void AddToPlayerKills(int amount)
{
    if (playerKills < 10000000) {
        if (amount < 5)
            playerKills += 1;
        else
            playerKills += amount * 0.2;
    } else {
        playerKills = 10000000;
    }
}

// FUNCTION: EVNOVA 0x0046f270
// MAC: _WeaponBurstInterval i386 0x0000831c ppc 0x0000a2bc
short WeaponBurstInterval(short weap, short count)
{
    if ((weaponStats[weap].flags & kWeapFireSimultaneously) != 0)
        return weaponStats[weap].burstCount;
    return count * weaponStats[weap].burstCount;
}

// FUNCTION: EVNOVA 0x0046f2c0
// MAC: _WeaponMaxSimultShots i386 0x00008350 ppc 0x0000a2f4
short WeaponMaxSimultShots(ShipObj *ship, short w)
{
    short count;
    short ammo;
    short type;
    float fuel;

    if ((weaponStats[w].flags & kWeapFireSimultaneously) != 0) {
        count = ship->weap[w].count;
        if ((weaponStats[w].flags & kWeapFireSimultaneously) != 0
            && (weaponStats[w].flags3 & kWeapAmmoAtBurstEnd) != 0) {
            if (weaponStats[w].guidance == kGuidanceBay) {
                ammo = ship->weap[w].ammo;
                if (ammo < count)
                    count = ammo;
            } else {
                type = weaponStats[w].ammoType;
                if (type >= 0 && type < kMaxWeapTypes) {
                    ammo = ship->weap[type].ammo;
                    if (ammo < count)
                        count = ammo;
                } else if (type < -kWeapAmmoFuelBase) {
                    fuel = ship->fuel;
                    if (fuel > 0.0f) {
                        ammo = (fuel
                               / ((abs(type) - kWeapAmmoFuelBase) * 0.1));
                        if (ammo < count)
                            count = ammo;
                    } else {
                        count = 0;
                    }
                }
            }
        }
    } else {
        count = 1;
    }
    return count;
}

// FUNCTION: EVNOVA 0x0046f3f0
// MAC: _IonizeShip i386 0x00008409 ppc 0x0000a408
void IonizeShip(ShipObj *ship, WeapRecord *weap, FloatPoint *pos)
{
    short falloff;
    float dist2;
    float rangeSq;

    falloff = weap->ionization;
    if (pos != 0) {
        dist2 = XYFloatSquaredDist(&ship->pos, pos);
        rangeSq = (weap->blastRadius * weap->blastRadius);
        if (dist2 > rangeSq) {
            falloff = 0;
        } else if (dist2 > 0.0) {
            falloff = (falloff * (1.0 - dist2 / rangeSq));
        }
    }
    if (falloff > 0) {
        ship->ionization += falloff;
        ship->ionColor |= weap->ionizeColor;
    }
}

EXTERN_C_END
