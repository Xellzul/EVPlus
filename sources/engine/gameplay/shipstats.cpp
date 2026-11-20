/* shipstats.cpp -- ship capabilities from outfits, and the player's precalculated values */

#include "ai.h"
#include "game.h"
#include "player.h"
#include "ships.h"
#include "gameutil.h"
#include "stellar.h"

EXTERN_C_BEGIN

// FUNCTION: EVNOVA 0x00462ec0
// MAC: _ShipStatsToSystemInfo i386 0x0000caee ppc 0x0000fc04
void ShipStatsToSystemInfo(void)
{
    short countBuf[kMaxWeapTypes];
    short ammoBuf[kMaxWeapTypes];
    short slot;
    short i;
    short k;

    for (i = 0; i < kMaxWeapTypes; i++) {
        countBuf[i] = shipData->weap[i].count;
        ammoBuf[i] = shipData->weap[i].ammo;
    }

    for (i = 0; i < kMaxOutfits; i++) {
        if (shipSystems[i] <= 0)
            continue;
        if (outfitSystem[i].techLevel >= 0x8000)
            continue;
        if (outfitSystem[i].modType[0] == kOutfModWeapon
                && outfitSystem[i].modVal[0] >= 0 && outfitSystem[i].modVal[0] < kMaxWeapTypes) {
            slot = outfitSystem[i].modVal[0];
            if (shipSystems[i] >= countBuf[slot]) {
                shipSystems[i] = countBuf[slot];
                countBuf[slot] = 0;
            } else {
                countBuf[slot] -= shipSystems[i];
            }
        } else if (outfitSystem[i].modType[0] == kOutfModAmmo
                && outfitSystem[i].modVal[0] >= 0 && outfitSystem[i].modVal[0] < kMaxWeapTypes) {
            slot = outfitSystem[i].modVal[0];
            if (shipSystems[i] >= ammoBuf[slot]) {
                shipSystems[i] = ammoBuf[slot];
                ammoBuf[slot] = 0;
            } else {
                ammoBuf[slot] -= shipSystems[i];
            }
        }
    }

    for (i = 0; i < kMaxOutfits; i++) {
        if (shipSystems[i] < 0)
            shipSystems[i] = 0;
    }

    for (k = 0; k < kMaxWeapTypes; k++) {
        if (countBuf[k] > 0) {
            for (i = 0; i < kMaxOutfits; i++) {
                if (outfitSystem[i].techLevel < 0x8000 && outfitSystem[i].modType[0] == kOutfModWeapon
                        && outfitSystem[i].modVal[0] == k) {
                    shipSystems[i] += countBuf[k];
                    countBuf[k] = 0;
                    break;
                }
            }
        }
        if (ammoBuf[k] > 0) {
            for (i = 0; i < kMaxOutfits; i++) {
                if (outfitSystem[i].techLevel < 0x8000 && outfitSystem[i].modType[0] == kOutfModAmmo
                        && outfitSystem[i].modVal[0] == k) {
                    shipSystems[i] += ammoBuf[k];
                    ammoBuf[k] = 0;
                    break;
                }
            }
        }
    }

    ResetPlayerPrecalcedValues();
}

/*
 * modType[0] == 1 IS a weapon (modVal[0] names the slot - add the owned count
 * to weap[].count), modType[0] == 3 IS ammo for a weapon slot (add to
 * weap[].ammo). Same modType convention loaddata.cpp's def-outfit grant scan and
 * the shipyard's persistent-outfit check already use.
 */

// FUNCTION: EVNOVA 0x00463260
// MAC: _SystemInfoToShipStats i386 0x0000ca33 ppc 0x0000fb0c
void SystemInfoToShipStats(void)
{
    short i;

    for (i = 0; i < kMaxWeapTypes; i++) {
        shipData->weap[i].count = 0;
        shipData->weap[i].ammo = 0;
    }

    for (i = 0; i < kMaxOutfits; i++) {
        if (shipSystems[i] <= 0)
            continue;
        if (outfitSystem[i].techLevel >= kOutfTechAbsent)
            continue;
        if (outfitSystem[i].modType[0] == kOutfModWeapon)
            shipData->weap[outfitSystem[i].modVal[0]].count += shipSystems[i];
        if (outfitSystem[i].modType[0] == kOutfModAmmo)
            shipData->weap[outfitSystem[i].modVal[0]].ammo += shipSystems[i];
    }

    ResetPlayerPrecalcedValues();
}

// FUNCTION: EVNOVA 0x00463470
// MAC: _ShipFreeMass i386 0x0000b462 ppc 0x0000dee4
int32_t ShipFreeMass(void)
{
    short i;
    short type;
    int32_t free;
    int32_t mass;

    type = shipData[0].type;
    if (type < 0 || type >= kMaxShipTypes)
        return 0;
    free = shipClass[type].freeMass;
    for (i = 0; i < kMaxOutfits; i++) {
        if (shipSystems[i] > 0) {
            mass = AdjustedItemMass(&outfitSystem[i], &shipClass[shipData[0].type]);
            free -= mass * shipSystems[i];
        }
    }
    if (free < 0)
        free = 0;
    return free;
}

/*
 * Both walk the 512 outfit slots and check each owned outfit's four mod slots
 * (modType 4 = shield boost, 15 = afterburner); both results are cached in
 * player-scoped globals that other code invalidates.
 */

#define kOutfModShieldBoost 4

// FUNCTION: EVNOVA 0x00463550
// MAC: _ShipShieldCapacity i386 0x00002995 ppc 0x000034e8
float ShipShieldCapacity(ShipObj *ship)
{
    int32_t i;
    float cap;
    short j;

    cap = shipClass[ship->type].shield;
    if (ship->index == 0) {
        if (playerShieldCapacity >= 0.0f) {
            cap = playerShieldCapacity;
        } else {
            for (i = 0; i < kMaxOutfits; i++) {
                if (shipSystems[i] > 0) {
                    for (j = 0; j < kOutfMods; j++) {
                        if (outfitSystem[i].modType[j] == kOutfModShieldBoost)
                            cap += outfitSystem[i].modVal[j] * shipSystems[i];
                    }
                }
            }
            if (cap < 0.0f)
                cap = 0.0f;
            playerShieldCapacity = cap;
        }
    } else {
        if (ship->persType != -1) {
            if (personData[ship->persType].shieldMod > 0.0f)
                cap = cap * personData[ship->persType].shieldMod;
        }
    }
    if (ship->index != 0 && ship->aiType == kShipTypeFighter)
        cap *= 1.333;
    return cap;
}

#define kOutfModShieldRech 5

// FUNCTION: EVNOVA 0x00463680
// MAC: _ShipShieldRechargeRate i386 0x00002abb ppc 0x0000367c
float ShipShieldRechargeRate(ShipObj *ship)
{
    short i;
    float cap;
    short j;
    int tmp;

    cap = shipClass[tmp = ship->type].shieldRech;
    if (ship->index == 0) {
        if (playerShieldRechargeRate >= 0.0f) {
            cap = playerShieldRechargeRate;
        } else {
            for (i = 0; i < kMaxOutfits; i++) {
                if (shipSystems[i] > 0) {
                    for (j = 0; j < kOutfMods; j++) {
                        if (outfitSystem[i].modType[j] == kOutfModShieldRech)
                            cap += (float)outfitSystem[i].modVal[j] * 0.001
                                   * shipSystems[i];
                    }
                }
            }
            if (cap < 0.0f)
                cap = 0.0f;
            playerShieldRechargeRate = cap;
        }
    }
    if (cap < 0.0f)
        cap = 0.0f;
    if (ship->index != 0 && ship->aiType == kShipTypeFighter)
        cap *= 1.333;
    return cap;
}

/*
 * The persType guard is a RANGE here (0 <= pers < 1024, the array's own size)
 * where the shield twin tests `pers != -1`, and the base is `fild`ed from a
 * long (`armor` at ShipRec+0x60) where shieldRech's was a float.
 */

#define kOutfModArmor 6

// FUNCTION: EVNOVA 0x004637a0
// MAC: _ShipArmorCapacity i386 0x00002bc0 ppc 0x000037e4
float ShipArmorCapacity(ShipObj *ship)
{
    short i;
    float cap;
    short j;
    short pers;

    cap = shipClass[ship->type].armor;
    if (ship->index == 0) {
        if (playerArmorCapacity >= 0.0f) {
            cap = playerArmorCapacity;
        } else {
            for (i = 0; i < kMaxOutfits; i++) {
                if (shipSystems[i] > 0) {
                    for (j = 0; j < kOutfMods; j++) {
                        if (outfitSystem[i].modType[j] == kOutfModArmor)
                            cap += outfitSystem[i].modVal[j] * shipSystems[i];
                    }
                }
            }
            if (cap < 0.0f)
                cap = 0.0f;
            playerArmorCapacity = cap;
        }
    } else {
        pers = ship->persType;
        if (pers >= 0 && pers < kMaxPersons) {
            if (personData[pers].shieldMod > 0.0f)
                cap = cap * personData[pers].shieldMod;
        }
    }
    if (ship->index != 0 && ship->aiType == kShipTypeFighter)
        cap *= 1.333;
    return cap;
}

#define kOutfModArmorRech 0x1d

// FUNCTION: EVNOVA 0x004638e0
// MAC: _ShipArmorRechargeRate i386 0x00002e0e ppc 0x00003acc
float ShipArmorRechargeRate(ShipObj *ship)
{
    short i;
    float cap;
    short j;

    if (IsDisabled(ship))
        return 0.0f;
    cap = shipClass[ship->type].armorRech;
    if (ship->index == 0) {
        if (playerArmorRechargeRate >= 0.0f) {
            cap = playerArmorRechargeRate;
        } else {
            for (i = 0; i < kMaxOutfits; i++) {
                if (shipSystems[i] > 0) {
                    for (j = 0; j < kOutfMods; j++) {
                        if (outfitSystem[i].modType[j] == kOutfModArmorRech)
                            cap += (float)outfitSystem[i].modVal[j] * 0.001
                                   * shipSystems[i];
                    }
                }
            }
            if (cap < 0.0f)
                cap = 0.0f;
            playerArmorRechargeRate = cap;
        }
    }
    if (cap < 0.0f)
        cap = 0.0f;
    if (ship->index == 0 && cheatsActive != 0)
        cap *= 50.0f;
    return cap;
}

#define kOutfModType12  0xc
#define kMaxFuel        32000

// FUNCTION: EVNOVA 0x00463a20
// MAC: _ShipFuelCapacity i386 0x00002f21 ppc 0x00003c74
short ShipFuelCapacity(ShipObj *ship)
{
    short i;
    short j;
    int32_t total;

    total = shipClass[ship->type].fuel;
    if (ship->index == 0) {
        if (playerFuelCapacity >= 0.0f) {
            total = playerFuelCapacity;
        } else {
            for (i = 0; i < kMaxOutfits; i++) {
                if (shipSystems[i] > 0) {
                    for (j = 0; j < kOutfMods; j++) {
                        if (outfitSystem[i].modType[j] == kOutfModType12)
                            total += outfitSystem[i].modVal[j]
                                     * shipSystems[i];
                    }
                }
            }
            if (total < 0)
                total = 0;
            if (total > kMaxFuel)
                total = kMaxFuel;
            playerFuelCapacity = total;
        }
    }
    return total;
}

#define kOutfModType18  0x12

// FUNCTION: EVNOVA 0x00463b30
// MAC: _ShipFuelGenRate i386 0x00002ffb ppc 0x00003d88
Boolean ShipFuelGenRate(ShipObj *ship, float *rate)
{
    short i;
    short j;

    if (ship->fuelRegenState < 0) {
        ship->fuelRegenState = 0;
        ship->fuelRegenRate = 0;
        if (shipClass[ship->type].fuelRegen > 0 &&
            ((shipClass[ship->type].flags & kShipFuelRegen) != 0 ||
             ship->index != 0)) {
            ship->fuelRegenState = 1;
            ship->fuelRegenRate = 1.0 / (float)shipClass[ship->type].fuelRegen;
        }
        if (ship->index == 0) {
            for (i = 0; i < kMaxOutfits; i++) {
                if (shipSystems[i] > 0) {
                    for (j = 0; j < kOutfMods; j++) {
                        if (outfitSystem[i].modType[j] == kOutfModType18) {
                            ship->fuelRegenState = 1;
                            ship->fuelRegenRate += (float)shipSystems[i] * (1.0 / (float)outfitSystem[i].modVal[j]);
                        }
                    }
                }
            }
        } else {
            for (i = 0; i < kDefaultOutfitSlots; i++) {
                if (shipClass[ship->type].defOutfCount[i] > 0) {
                    short id = shipClass[ship->type].defOutfID[i];

                    if (id >= 0 && id < kMaxOutfits) {
                        for (j = 0; j < kOutfMods; j++) {
                            if (outfitSystem[id].modType[j] == kOutfModType18) {
                                ship->fuelRegenState = 1;
                                ship->fuelRegenRate += (float)shipClass[ship->type].defOutfCount[i]
                                    * (1.0 / (float)outfitSystem[id].modVal[j]);
                            }
                        }
                    }
                }
            }
        }
    }
    if (ship->fuelRegenState == 0) {
        *rate = 0;
        return 0;
    }
    *rate = ship->fuelRegenRate;
    return 1;
}

#define kOutfModSpeed    8

#define kOutfModTurn    9

// FUNCTION: EVNOVA 0x00463e70
// MAC: _ShipTurnRate i386 0x000032f7 ppc 0x000041b8
float ShipTurnRate(ShipObj *ship)
{
    short i;
    float v;
    short j;
    float ratio;

    v = shipClass[ship->type].turnRate;
    if (ship->index == 0) {
        if (playerTurnRate >= 0.0f) {
            v = playerTurnRate;
        } else {
            for (i = 0; i < kMaxOutfits; i++) {
                if (shipSystems[i] > 0) {
                    for (j = 0; j < kOutfMods; j++) {
                        if (outfitSystem[i].modType[j] == kOutfModTurn)
                            v += outfitSystem[i].modVal[j]
                                 * shipSystems[i] * 0.01;
                    }
                }
            }
            if (v < 1.0f && shipClass[ship->type].turnRate > 0.0f)
                v = 1.0f;
            playerTurnRate = v;
        }
    } else {
        if (ship->tractorShip != -1 && ship->tractorShip != ship->index)
            v *= 0.333;
    }
    if (shipClass[ship->type].turnRate >= 1.0f && v < 1.0f)
        v = 1.0f;
    if (ship->ionization > 0.0f && ship->jumpTimer <= 0.0f) {
        ratio = ShipIonizationFactor(ship);
        if (ratio > 0.7)
            ratio = 0.7f;
        v *= (1.0 - ratio);
    }
    if (v < 0.0f)
        v = 0.0f;
    return v;
}

#define kOutfModAccel   7

// FUNCTION: EVNOVA 0x004640a0
// MAC: _ShipAccelRate i386 0x0000353b ppc 0x00004490
float ShipAccelRate(ShipObj *ship)
{
    short i;
    float v;
    short j;
    float ratio;

    v = shipClass[ship->type].accel;
    if (ship->index == 0) {
        if (playerAccelRate >= 0.0f) {
            v = playerAccelRate;
        } else {
            for (i = 0; i < kMaxOutfits; i++) {
                if (shipSystems[i] > 0) {
                    for (j = 0; j < kOutfMods; j++) {
                        if (outfitSystem[i].modType[j] == kOutfModAccel)
                            v += (float)((float)(outfitSystem[i].modVal[j] / 10000.0)
                                 * shipSystems[i]);
                    }
                }
            }
            if (v < 0.0f)
                v = 0.0f;
            playerAccelRate = v;
        }
        v *= 2.0;
    } else {
        if ((shipClass[ship->type].flags & kShipPlanetType) != 0)
            return 0.0f;
        v *= ship->skill;
        if (ship->govt != -1)
            v *= govtData[ship->govt].skillMult;
        if (ship->tractorShip != -1 && ship->tractorShip != ship->index)
            v *= 0.333;
        else if (ship->parentShip == 0)
            v *= 2.0;
        else
            v *= 2.0;
    }
    if (ship->ionization > 0.0f) {
        ratio = ShipIonizationFactor(ship);
        if (ratio > 0.7)
            ratio = 0.7f;
        v *= (1.0 - ratio);
    }
    if (v < 0.0f)
        v = 0.0f;
    return v;
}

// FUNCTION: EVNOVA 0x004642e0
// MAC: _ShipMaxSpeed i386 0x000036f2 ppc 0x000046c4
float ShipMaxSpeed(ShipObj *ship)
{
    short i;
    float v;
    short j;

    v = shipClass[ship->type].maxSpeed;
    if (ship->index == 0) {
        if (playerMaxSpeed >= 0.0f) {
            v = playerMaxSpeed;
        } else {
            for (i = 0; i < kMaxOutfits; i++) {
                if (shipSystems[i] > 0) {
                    for (j = 0; j < kOutfMods; j++) {
                        if (outfitSystem[i].modType[j] == kOutfModSpeed)
                            v += (float)((float)(outfitSystem[i].modVal[j] / 100.0)
                                 * shipSystems[i]);
                    }
                }
            }
            if (v < 0.0f)
                v = 0.0f;
            playerMaxSpeed = v;
        }
    } else {
        if ((shipClass[ship->type].flags & kShipPlanetType) != 0)
            return 0.0f;
        v *= ship->skill;
        if (ship->govt != -1)
            v *= govtData[ship->govt].skillMult;
        if (ship->tractorShip != -1 && ship->tractorShip != ship->index)
            v *= 0.333;
    }
    if (ship->index == 0 || ship->parentShip == 0) {
        if (strictPlay == 0)
            v *= 1.5;
    }
    if (v < 0.0f)
        v = 0.0f;
    return v;
}

/* Guidance 99 is the carried-ship type. */

#define kOutfModBay      11

// FUNCTION: EVNOVA 0x004644a0
// MAC: _HasEscapePod i386 0x0000b83d ppc 0x0000e3e0
Boolean HasEscapePod(ShipObj *ship)
{
    short i;
    short j;

    if (ship->index == 0) {
        for (i = 0; i < kMaxOutfits; i++) {
            if (shipSystems[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (outfitSystem[i].modType[j] == kOutfModBay)
                        return 1;
                }
            }
        }
    }
    if (HasEscapeShip(ship))
        return 1;
    return 0;
}

// FUNCTION: EVNOVA 0x00464520
// MAC: _HasEscapeShip i386 0x0000386e ppc 0x000048c4
Boolean HasEscapeShip(ShipObj *ship)
{
    short i;
    short type;

    for (i = 0; i < kMaxWeapTypes; i++) {
        if (weaponStats[i].guidance == kGuidanceBay &&
            ship->weap[i].ammo > 0) {
            type = weaponStats[i].ammoType - 128;
            if ((shipClass[type].flags & kShipEscapeShip) != 0)
                return 1;
        }
    }
    return 0;
}

// FUNCTION: EVNOVA 0x00464590
// MAC: _EscapeShipType i386 0x000038d0 ppc 0x00004938
short EscapeShipType(ShipObj *ship)
{
    short i;
    short type;

    for (i = 0; i < kMaxWeapTypes; i++) {
        if (weaponStats[i].guidance == kGuidanceBay &&
            ship->weap[i].ammo > 0) {
            type = weaponStats[i].ammoType - 128;
            if ((shipClass[type].flags & kShipEscapeShip) != 0)
                return type;
        }
    }
    return -1;
}

// FUNCTION: EVNOVA 0x00464600
// MAC: _EscapeShipWeapType i386 0x00003936 ppc 0x000049ac
short EscapeShipWeapType(ShipObj *ship)
{
    short i;
    short type;

    for (i = 0; i < kMaxWeapTypes; i++) {
        if (weaponStats[i].guidance == kGuidanceBay && ship->weap[i].ammo > 0) {
            type = weaponStats[i].ammoType - kFirstResourceID;
            if ((shipClass[type].flags & kShipEscapeShip) != 0)
                return i;
        }
    }
    return -1;
}

// FUNCTION: EVNOVA 0x00464670
// MAC: _KeyCarriedShipTypeOnboard i386 0x0000399c ppc 0x00004a28
Boolean KeyCarriedShipTypeOnboard(ShipObj *ship)
{
    short i;
    short escort;
    short bay;

    escort = shipClass[ship->type].keyCarried;
    if (escort == -1)
        return 0;
    for (i = 0; i < kMaxWeapTypes; i++) {
        if (weaponStats[i].guidance == kGuidanceBay &&
            ship->weap[i].count > 0 && ship->weap[i].ammo > 0) {
            bay = (weaponStats[i].ammoType - 128);
            if (bay == escort)
                return 1;
        }
    }
    return 0;
}

#define kOutfModType20 0x14
#define kOutfModType15 0xf

/* does the player own any outfit carrying modifier type 0x14 */

// FUNCTION: EVNOVA 0x00464700
// MAC: _HasAutoEject i386 0x00003a14 ppc 0x00004ab4
Boolean HasAutoEject(ShipObj *ship)
{
    short i;
    short j;

    if (ship->index == 0) {
        for (i = 0; i < kMaxOutfits; i++) {
            if (shipSystems[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (outfitSystem[i].modType[j] == kOutfModType20)
                        return 1;
                }
            }
        }
    }
    return 0;
}

// FUNCTION: EVNOVA 0x00464760
// MAC: _HasAfterburner i386 0x00003a6d ppc 0x00004b24
Boolean HasAfterburner(ShipObj *ship)
{
    short i;
    short j;

    if (cheatsActive != 0)
        return 1;
    if (ship->index == 0) {
        if (playerHasAfterburner >= 0) {
            if (playerHasAfterburner != 0)
                return 1;
        } else {
            playerHasAfterburner = 0;
            for (i = 0; i < kMaxOutfits; i++) {
                if (shipSystems[i] > 0) {
                    for (j = 0; j < kOutfMods; j++) {
                        if (outfitSystem[i].modType[j] == kOutfModType15) {
                            playerHasAfterburner = 1;
                            return 1;
                        }
                    }
                }
            }
        }
    }
    return 0;
}

#define kOutfModType33  0x21

// FUNCTION: EVNOVA 0x00464810
// MAC: _ShipECM i386 0x00003aff ppc 0x00004bdc
short ShipECM(ShipObj *ship, short kind)
{
    short i;
    short j;

    if (ship == NULL)
        return 0;
    if (kind < 0 || kind >= 4)
        return 0;
    if (IsDisabled(ship))
        return 0;
    if (ship->jamming[kind] >= 0)
        return ship->jamming[kind];

    if (shipClass[ship->type].inherentGovtB >= 0 &&
        shipClass[ship->type].inherentGovtB < kMaxGovts)
        ship->jamming[kind] =
            govtData[shipClass[ship->type].inherentGovtB].inhJam[kind];
    else
        ship->jamming[kind] = 0;

    if (ship->index == 0) {
        for (i = 0; i < kMaxOutfits; i++) {
            if (shipSystems[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (outfitSystem[i].modType[j] == kOutfModType33 + kind)
                        ship->jamming[kind] += outfitSystem[i].modVal[j];
                }
            }
        }
    } else {
        for (i = 0; i < kDefaultOutfitSlots; i++) {
            if (shipClass[ship->type].defOutfCount[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (outfitSystem[shipClass[ship->type].defOutfID[i]].modType[j] ==
                        kOutfModType33 + kind)
                        ship->jamming[kind] +=
                            outfitSystem[shipClass[ship->type].defOutfID[i]].modVal[j];
                }
            }
        }
        if (ship->govt >= 0 && ship->govt < kMaxGovts &&
            (govtData[ship->govt].flags & kGovtFreighterHalfJam) != 0)
            ship->jamming[kind] = ship->jamming[kind] / 2;
    }

    if (ship->jamming[kind] < 0)
        ship->jamming[kind] = 0;
    if (ship->jamming[kind] > 100)
        ship->jamming[kind] = 100;
    return ship->jamming[kind];
}

#define kOutfModType22 0x16

// FUNCTION: EVNOVA 0x00465550
// MAC: _ShipHyperTransitTime i386 0x000043d0 ppc 0x00005774
short ShipHyperTransitTime(ShipObj *ship)
{
    short i;
    short j;
    short count;

    count = 1;
    if (shipClass[ship->type].mass >= 100)
        count++;
    if (shipClass[ship->type].mass >= 200)
        count++;
    if (ship->index == 0) {
        for (i = 0; i < kMaxOutfits; i++) {
            if (shipSystems[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (outfitSystem[i].modType[j] == kOutfModType22)
                        count += outfitSystem[i].modVal[j] * shipSystems[i];
                }
            }
        }
    }
    if (count < 1)
        count = 1;
    return count;
}

/*
 * A squared range: floored at zero and then squared, so the caller can compare
 * against a squared distance without a square root.
 */
#define kOutfModType23 0x17

// FUNCTION: EVNOVA 0x00465610
// MAC: _ShipHyperSafeDist i386 0x00004485 ppc 0x00005830
int32_t ShipHyperSafeDist(ShipObj *ship)
{
    short i;
    short j;
    int32_t range;

    range = 1000;
    if (ship->index == 0) {
        for (i = 0; i < kMaxOutfits; i++) {
            if (shipSystems[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (outfitSystem[i].modType[j] == kOutfModType23)
                        range += outfitSystem[i].modVal[j] * shipSystems[i];
                }
            }
        }
    }
    if (range < 0)
        range = 0;
    return range * range;
}

/*
 * Cached in a short that is below -100 until computed and clamped to -100..100
 * once it is.
 */
#define kOutfModType24 0x18

// FUNCTION: EVNOVA 0x0046abb0
// MAC: _ShipAntiInterference i386 0x00005f2c ppc 0x0000772c
short ShipAntiInterference(void)
{
    short i;
    short j;
    short total;

    if (playerAntiInterference < -100) {
        total = 0;
        for (i = 0; i < kMaxOutfits; i++) {
            if (shipSystems[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (outfitSystem[i].modType[j] == kOutfModType24)
                        total += outfitSystem[i].modVal[j] * shipSystems[i];
                }
            }
        }
        if (total > 100)
            total = 100;
        if (total < -100)
            total = -100;
        playerAntiInterference = total;
        return total;
    }
    return playerAntiInterference;
}

#define kOutfModType39 0x27

// FUNCTION: EVNOVA 0x0046c080
// MAC: _ShipDeionizationRate i386 0x00006ec3 ppc 0x00008908
float ShipDeionizationRate(ShipObj *ship)
{
    short i;
    short j;
    float value;

    value = shipClass[ship->type].deionize;
    if (ship->index == 0) {
        if (playerDeionizeRate >= 0.0f) {
            value = playerDeionizeRate;
        } else {
            for (i = 0; i < kMaxOutfits; i++) {
                if (shipSystems[i] > 0) {
                    for (j = 0; j < kOutfMods; j++) {
                        if (outfitSystem[i].modType[j] == kOutfModType39)
                            value = value + outfitSystem[i].modVal[j] * 0.01 *
                                    shipSystems[i];
                    }
                }
            }
            playerDeionizeRate = value;
        }
    }
    return value;
}

#define kOutfModType40 0x28

// FUNCTION: EVNOVA 0x0046c160
// MAC: _ShipIonizationFactor i386 0x00003225 ppc 0x00004094
float ShipIonizationFactor(ShipObj *ship)
{
    short i;
    short j;
    float total;

    total = shipClass[ship->type].ionizeMax;
    if (ship->index == 0) {
        if (playerIonizeMax >= 0.0f) {
            total = playerIonizeMax;
        } else {
            for (i = 0; i < kMaxOutfits; i++) {
                if (shipSystems[i] > 0) {
                    for (j = 0; j < kOutfMods; j++) {
                        if (outfitSystem[i].modType[j] == kOutfModType40)
                            total = total + outfitSystem[i].modVal[j] *
                                    shipSystems[i];
                    }
                }
            }
            playerIonizeMax = total;
        }
    }
    if (total > 0.0f)
        return ship->ionization / total;
    return 0.0f;
}

#define kOutfModType28 0x1c

// FUNCTION: EVNOVA 0x0046c250
// MAC: _PlayerEffectiveMurk i386 0x00006f8f ppc 0x00008a18
short PlayerEffectiveMurk(void)
{
    short i;
    short j;
    short level;

    level = systemData[shipData[0].curSystem].murk;
    if (level < 0)
        level = 0;
    for (i = 0; i < kMaxOutfits; i++) {
        if (shipSystems[i] > 0) {
            for (j = 0; j < kOutfMods; j++) {
                if (outfitSystem[i].modType[j] == kOutfModType28)
                    level += outfitSystem[i].modVal[j] * shipSystems[i];
            }
        }
    }
    if (level < 0)
        level = 0;
    if (level > 100)
        level = 100;
    return level;
}

#define kOutfModType31 0x1f

// FUNCTION: EVNOVA 0x0046cb90
// MAC: _ShipCanScoop i386 0x000075c7 ppc 0x000090dc
Boolean ShipCanScoop(ShipObj *ship)
{
    short i;
    short j;

    if (ship->index == 0) {
        for (i = 0; i < kMaxOutfits; i++) {
            if (shipSystems[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (outfitSystem[i].modType[j] == kOutfModType31)
                        return 1;
                }
            }
        }
    } else {
        for (i = 0; i < kDefaultOutfitSlots; i++) {
            if (shipClass[ship->type].defOutfCount[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (outfitSystem[shipClass[ship->type].defOutfID[i]].modType[j] ==
                        kOutfModType31)
                        return 1;
                }
            }
        }
    }
    return 0;
}

#define kOutfModType32 0x20

// FUNCTION: EVNOVA 0x0046cdd0
// MAC: _ShipJumpsPerJump i386 0x000077b4 ppc 0x00009394
short ShipJumpsPerJump(ShipObj *ship)
{
    short i;
    short j;
    short total;

    total = 1;
    if (ship->index == 0) {
        for (i = 0; i < kMaxOutfits; i++) {
            if (shipSystems[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (outfitSystem[i].modType[j] == kOutfModType32)
                        total += outfitSystem[i].modVal[j];
                }
            }
        }
    } else {
        for (i = 0; i < kDefaultOutfitSlots; i++) {
            if (shipClass[ship->type].defOutfCount[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (outfitSystem[shipClass[ship->type].defOutfID[i]].modType[j] ==
                        kOutfModType32)
                        total += outfitSystem[shipClass[ship->type].defOutfID[i]].modVal[j];
                }
            }
        }
    }
    if (total < 1)
        total = 1;
    return total;
}

// FUNCTION: EVNOVA 0x0046cec0
// MAC: _ShipMaxWeaponRange i386 0x0000bafd ppc 0x0000e734
short ShipMaxWeaponRange(ShipObj *ship)
{
    int32_t best;
    int32_t range;
    short i;

    best = 0;
    for (i = 0; i < kMaxWeapTypes; i++) {
        if (ship->weap[i].count > 0 && WeaponHasAmmo(ship, i) != 0) {
            if (weaponStats[i].guidance == kGuidanceBeam
                    || weaponStats[i].guidance == kGuidanceTurretBeam) {
                range = weaponStats[i].beamLength;
            } else if (weaponStats[i].guidance == kGuidanceUnguided
                       || weaponStats[i].guidance == kGuidanceTurret
                       || weaponStats[i].guidance == kGuidanceFrontQuad
                       || weaponStats[i].guidance == kGuidanceFrontQuad) {
                range = weaponStats[i].totalRange;
            } else if (weaponStats[i].guidance == kGuidanceHoming) {
                range = (weaponStats[i].totalRange * 0.85);
            } else if (weaponStats[i].guidance == kGuidanceRocket) {
                range = (weaponStats[i].totalRange * 0.5);
            } else {
                range = 0;
            }
            if (range > best)
                best = range;
        }
    }
    return (best >= 0x7fff) ? 0x7fff : best;
}

#define kOutfModType37 0x25

// FUNCTION: EVNOVA 0x0046d080
// MAC: _ShipCanExpiditeJumps i386 0x00007872 ppc 0x00009498
Boolean ShipCanExpiditeJumps(ShipObj *ship)
{
    short i;
    short j;

    if ((shipClass[ship->type].flags2 & kShipJumpsAtSpeed) != 0)
        return 1;
    if (ship->index == 0) {
        for (i = 0; i < kMaxOutfits; i++) {
            if (shipSystems[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (outfitSystem[i].modType[j] == kOutfModType37)
                        return 1;
                }
            }
        }
    } else {
        for (i = 0; i < kDefaultOutfitSlots; i++) {
            if (shipClass[ship->type].defOutfCount[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (outfitSystem[shipClass[ship->type].defOutfID[i]].modType[j] ==
                        kOutfModType37)
                        return 1;
                }
            }
        }
    }
    return 0;
}

/* the modType values this routine reacts to */
#define kModCloak  0x1f
#define kModJam2C  0x2c
#define kModJam30  0x30
#define kModScan2F 0x2f
#define kModScan32 0x32

// FUNCTION: EVNOVA 0x0046d4b0
// MAC: _ResetPlayerPrecalcedValues i386 0x0000c357 ppc 0x0000f0c8
void ResetPlayerPrecalcedValues(void)
{
    short holds;
    short cargo;
    short i;
    short j;
    short m;
    short k;

    playerShieldCapacity = -1.0f;
    playerShieldRechargeRate = -1.0f;
    playerArmorCapacity = -1.0f;
    playerArmorRechargeRate = -1.0f;
    playerTurnRate = -1.0f;
    playerAccelRate = -1.0f;
    playerMaxSpeed = -1.0f;
    playerFuelCapacity = -1.0f;
    playerIonizeMax = -1.0f;
    playerDeionizeRate = -1.0f;
    playerHasAfterburner = -1;
    playerHasDensityScanner = -1;
    playerHasIFF = -1;
    playerHasInertialDamper = -1;
    playerAntiInterference = 0x8300;
    playerEffectiveMurk = -1;
    playerCanTargetUntargetableShips = -1;
    playerCanTargetCloakedShips = -1;
    playerABFuelUsage = -1.0f;
    playerResistsGravity = -1;
    playerResistsDeadlyStellars = -1;
    playerHasAreaCloak = -1;
    playerHasIllegalJunk = 0;
    playerHasIllegalOutfits = 0;
    playerSabotaged = 0;
    playerHasTribbles = 0;
    playerHasPerishableJunk = 0;
    shipData->fuelRegenState = -1;
    shipData->canScoop = 0;

    for (i = 0; i < kMaxJunks; i++) {
        if (junkData[i].held > 0) {
            if (junkData[i].scanMask != 0)
                playerHasIllegalJunk = 1;
            if ((junkData[i].flags & kJunkTribbles) != 0)
                playerHasTribbles = 1;
            else if ((junkData[i].flags & kJunkPerishable) != 0)
                playerHasPerishableJunk = 1;
        }
    }

    for (i = 0; i < kMaxOutfits; i++) {
        if (shipSystems[i] > 0 && outfitSystem[i].scanMask != 0) {
            playerHasIllegalOutfits = 1;
            break;
        }
    }

    for (i = 0; i < kMaxOutfits; i++) {
        if (shipSystems[i] > 0) {
            for (j = 0; j < kOutfMods; j++) {
                if (outfitSystem[i].modType[j] == kModScan2F) {
                    playerSabotaged = 1;
                    sabotageCounter = 0.0f;
                }
                if (outfitSystem[i].modType[j] == kModScan32) {
                    if (playerSabotaged < 1)
                        playerSabotaged = 2;
                    sabotageCounter = 0.0f;
                }
                if (outfitSystem[i].modType[j] == kModJam2C) {
                    if (outfitSystem[i].modVal[j] == -1) {
                        allReinforcementsInhibited = 1;
                    } else {
                        for (k = 0; k < kMaxGovts; k++) {
                            for (m = 0; m < kGovtClasses; m++) {
                                if (govtData[k].classes[m] ==
                                        outfitSystem[i].modVal[j] &&
                                    govtData[k].classes[m] != -1) {
                                    govtData[k].reinfInhibited = 1;
                                    break;
                                }
                            }
                        }
                    }
                }
                if (outfitSystem[i].modType[j] == kModJam30) {
                    for (k = 0; k < kMaxGovts; k++) {
                        for (m = 0; m < kGovtClasses; m++) {
                            if (govtData[k].classes[m] ==
                                    outfitSystem[i].modVal[j] &&
                                govtData[k].classes[m] != -1) {
                                govtData[k].iffScrambled = 1;
                                break;
                            }
                        }
                    }
                }
                if (outfitSystem[i].modType[j] == kModCloak)
                    shipData->canScoop = 1;
            }
        }
    }

    if (playerSabotaged != 0 && sabotageCounter <= 0.0f)
        sabotageCounter = Rand(100);
    if (shipData->canScoop != 0) {
        if (TotalFleetHolds() - TotalCargo(shipData) <= 0)
            shipData->canScoop = 0;
    }
    for (i = 0; i < 4; i++)
        shipData->jamming[i] = -1;
    holds = TotalFleetHolds();
    cargo = TotalCargo(shipData);
    if (cargo > holds) {
        for (i = 0; i <= kLastCommodity; i++)
            shipData->cargo[i] *= holds / (float)cargo;
        statusRedraw[kRedrawCargo] = 1;
    }
    for (i = 0; i <= kLastCommodity; i++) {
        if (shipData->cargo[i] < 0) {
            shipData->cargo[i] = 0;
            statusRedraw[kRedrawCargo] = 1;
        }
    }
    for (i = 0; i < kMaxJunks; i++) {
        if (junkData[i].held < 0) {
            junkData[i].held = 0;
            statusRedraw[kRedrawCargo] = 1;
        }
    }
    shipData->seesCloaked[0] = -1;
    shipData->seesCloaked[1] = -1;
    shipData->decloaksWhenHit = -1;

    for (j = 0; j < kMaxGovts; j++) {
        govtData[j].blessesPlayer[0] = 0;
        govtData[j].blessesPlayer[1] = 0;
    }
    for (i = 0; i < kMaxRanks; i++) {
        if (rankData[i].active != 0 && rankData[i].loaded != 0 &&
            (rankData[i].flags & (kRankNotAttacked | kRankCanAlwaysLand)) != 0 &&
            rankData[i].govt != -1) {
            for (j = 0; j < kMaxGovts; j++) {
                if (GovtAllies(j, rankData[i].govt) != 0) {
                    if ((rankData[i].flags & kRankNotAttacked) != 0)
                        govtData[j].blessesPlayer[0] = 1;
                    if ((rankData[i].flags & kRankCanAlwaysLand) != 0)
                        govtData[j].blessesPlayer[1] = 1;
                }
            }
        }
    }
    playerRepairInhibit = -1.0f;
    playerEffectiveMurk = PlayerEffectiveMurk();
}

#define kOutfModType38 0x26

// FUNCTION: EVNOVA 0x0046df70
// MAC: _ShipIsInertialess i386 0x000079c2 ppc 0x00009640
Boolean ShipIsInertialess(ShipObj *ship)
{
    short i;
    short j;

    if (ship->index != 0 && ship->parentShip != -1 && IsShipInCloseForm(ship))
        return 0;
    if ((shipClass[ship->type].flags2 & kShipInertialess) != 0)
        return 1;
    if (ship->index == 0) {
        if (playerHasInertialDamper >= 0) {
            if (playerHasInertialDamper != 0)
                return 1;
        } else {
            playerHasInertialDamper = 0;
            for (i = 0; i < kMaxOutfits; i++) {
                if (shipSystems[i] > 0) {
                    for (j = 0; j < kOutfMods; j++) {
                        if (outfitSystem[i].modType[j] == kOutfModType38) {
                            playerHasInertialDamper = 1;
                            return 1;
                        }
                    }
                }
            }
        }
    }
    return 0;
}

/*
 * Memoised in a float that is negative until computed, and the modifier's
 * value is scaled by 0.0333, which is one thirtieth to four figures.
 */

// FUNCTION: EVNOVA 0x0046e060
// MAC: _ShipAfterburnerFuelUsage i386 0x00007a82 ppc 0x00009758
float ShipAfterburnerFuelUsage(ShipObj *ship)
{
    short i;
    short j;

    if (ship->index == 0) {
        if (playerABFuelUsage < 0.0) {
            playerABFuelUsage = 0.0f;
            for (i = 0; i < kMaxOutfits; i++) {
                if (shipSystems[i] > 0) {
                    for (j = 0; j < kOutfMods; j++) {
                        if (outfitSystem[i].modType[j] == kOutfModType15) {
                            playerABFuelUsage = outfitSystem[i].modVal[j] * 0.0333;
                            break;
                        }
                    }
                }
            }
        }
        return playerABFuelUsage;
    }
    return 0.0f;
}

#define kOutfModType41 0x29

// FUNCTION: EVNOVA 0x0046e120
// MAC: _ShipResistsGravity i386 0x00007b10 ppc 0x00009850
short ShipResistsGravity(ShipObj *ship)
{
    short i;
    short j;

    if ((shipClass[ship->type].flags3 & kShipIgnoresDeadly) != 0)
        return 1;
    if (ShipIsInertialess(ship))
        return 1;
    if (ship->index == 0) {
        if (playerResistsGravity != -1) {
            if (playerResistsGravity != 0)
                return 1;
        } else {
            playerResistsGravity = 0;
            for (i = 0; i < kMaxOutfits; i++) {
                if (shipSystems[i] > 0) {
                    for (j = 0; j < kOutfMods; j++) {
                        if (outfitSystem[i].modType[j] == kOutfModType41) {
                            playerResistsGravity = 1;
                            break;
                        }
                    }
                }
            }
        }
        return playerResistsGravity;
    }
    return 0;
}

#define kOutfModType42 0x2a

// FUNCTION: EVNOVA 0x0046e210
// MAC: _ShipResistsDeadlyStellars i386 0x00007bbe ppc 0x00009950
short ShipResistsDeadlyStellars(ShipObj *ship)
{
    short i;
    short j;

    if ((shipClass[ship->type].flags3 & kShipIgnoresDeadly) != 0)
        return 1;
    if (ship->index == 0) {
        if (playerResistsDeadlyStellars != -1) {
            if (playerResistsDeadlyStellars != 0)
                return 1;
        } else {
            playerResistsDeadlyStellars = 0;
            for (i = 0; i < kMaxOutfits; i++) {
                if (shipSystems[i] > 0) {
                    for (j = 0; j < kOutfMods; j++) {
                        if (outfitSystem[i].modType[j] == kOutfModType42) {
                            playerResistsDeadlyStellars = 1;
                            break;
                        }
                    }
                }
            }
        }
        return playerResistsDeadlyStellars;
    }
    return 0;
}

#define kOutfModType49 0x31

#define kShipDefOutfits 8

// FUNCTION: EVNOVA 0x0046e540
// MAC: _ShipCanSelfRepair i386 0x00009134 ppc 0x0000b258
Boolean ShipCanSelfRepair(ShipObj *ship)
{
    short i;
    short j;

    if (gSpeedMult > 0.0) {
        if (Rand((500.0 / gSpeedMult)) != 0)
            return 0;
    } else {
        if (Rand(500) != 0)
            return 0;
    }
    if (IsDying(ship))
        return 0;

    if (ship->index == 0) {
        for (i = 0; i < kMaxOutfits; i++) {
            if (shipSystems[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (outfitSystem[i].modType[j] == kOutfModType49)
                        return 1;
                }
            }
        }
    } else {
        for (i = 0; i < kShipDefOutfits; i++) {
            if (shipClass[ship->type].defOutfCount[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (outfitSystem[shipClass[ship->type].defOutfID[i]].modType[j]
                            == kOutfModType49)
                        return 1;
                }
            }
        }
    }
    return 0;
}

EXTERN_C_END
