/* shipstats.cpp -- ship capabilities from outfits, and the player's precalculated values */

#include "ai.h"
#include "game.h"
#include "outfitmods.h"
#include "player.h"
#include "ships.h"
#include "gameutil.h"
#include "stellar.h"

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
        if (outfitSystem[i].modType[0] == OutfMod::Weapon
                && outfitSystem[i].modVal[0] >= 0 && outfitSystem[i].modVal[0] < kMaxWeapTypes) {
            slot = outfitSystem[i].modVal[0];
            if (shipSystems[i] >= countBuf[slot]) {
                shipSystems[i] = countBuf[slot];
                countBuf[slot] = 0;
            } else {
                countBuf[slot] -= shipSystems[i];
            }
        } else if (outfitSystem[i].modType[0] == OutfMod::Ammo
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
                if (outfitSystem[i].techLevel < 0x8000 && outfitSystem[i].modType[0] == OutfMod::Weapon
                        && outfitSystem[i].modVal[0] == k) {
                    shipSystems[i] += countBuf[k];
                    countBuf[k] = 0;
                    break;
                }
            }
        }
        if (ammoBuf[k] > 0) {
            for (i = 0; i < kMaxOutfits; i++) {
                if (outfitSystem[i].techLevel < 0x8000 && outfitSystem[i].modType[0] == OutfMod::Ammo
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
        if (outfitSystem[i].modType[0] == OutfMod::Weapon)
            shipData->weap[outfitSystem[i].modVal[0]].count += shipSystems[i];
        if (outfitSystem[i].modType[0] == OutfMod::Ammo)
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

/* the player's values are cached in playerStats until ResetPlayerPrecalcedValues */

// FUNCTION: EVNOVA 0x00463550
// MAC: _ShipShieldCapacity i386 0x00002995 ppc 0x000034e8
float ShipObj::ShieldCapacity()
{
    float cap;

    cap = Class().shield;
    if (IsPlayer()) {
        if (playerStats.shieldCapacity >= 0.0f) {
            cap = playerStats.shieldCapacity;
        } else {
            ForEachPlayerMod(OutfMod::ShieldBoost, [&](short val, short owned) {
                cap += val * owned;
                return false;
            });
            if (cap < 0.0f)
                cap = 0.0f;
            playerStats.shieldCapacity = cap;
        }
    } else {
        if (persType != -1) {
            if (personData[persType].shieldMod > 0.0f)
                cap = cap * personData[persType].shieldMod;
        }
    }
    if (!IsPlayer() && aiType == kAIFighter)
        cap *= 1.333;
    return cap;
}

// FUNCTION: EVNOVA 0x00463680
// MAC: _ShipShieldRechargeRate i386 0x00002abb ppc 0x0000367c
float ShipObj::ShieldRechargeRate()
{
    float cap;
    int tmp;

    cap = shipClass[tmp = type].shieldRech;
    if (IsPlayer()) {
        if (playerStats.shieldRechargeRate >= 0.0f) {
            cap = playerStats.shieldRechargeRate;
        } else {
            ForEachPlayerMod(OutfMod::ShieldRech, [&](short val, short owned) {
                cap += (float)val * 0.001 * owned;
                return false;
            });
            if (cap < 0.0f)
                cap = 0.0f;
            playerStats.shieldRechargeRate = cap;
        }
    }
    if (cap < 0.0f)
        cap = 0.0f;
    if (!IsPlayer() && aiType == kAIFighter)
        cap *= 1.333;
    return cap;
}

/* persType is range-checked here; ShieldCapacity only tests != -1 */

// FUNCTION: EVNOVA 0x004637a0
// MAC: _ShipArmorCapacity i386 0x00002bc0 ppc 0x000037e4
float ShipObj::ArmorCapacity()
{
    float cap;
    short pers;

    cap = Class().armor;
    if (IsPlayer()) {
        if (playerStats.armorCapacity >= 0.0f) {
            cap = playerStats.armorCapacity;
        } else {
            ForEachPlayerMod(OutfMod::Armor, [&](short val, short owned) {
                cap += val * owned;
                return false;
            });
            if (cap < 0.0f)
                cap = 0.0f;
            playerStats.armorCapacity = cap;
        }
    } else {
        pers = persType;
        if (pers >= 0 && pers < kMaxPersons) {
            if (personData[pers].shieldMod > 0.0f)
                cap = cap * personData[pers].shieldMod;
        }
    }
    if (!IsPlayer() && aiType == kAIFighter)
        cap *= 1.333;
    return cap;
}

// FUNCTION: EVNOVA 0x004638e0
// MAC: _ShipArmorRechargeRate i386 0x00002e0e ppc 0x00003acc
float ShipObj::ArmorRechargeRate()
{
    float cap;

    if (IsDisabled())
        return 0.0f;
    cap = Class().armorRech;
    if (IsPlayer()) {
        if (playerStats.armorRechargeRate >= 0.0f) {
            cap = playerStats.armorRechargeRate;
        } else {
            ForEachPlayerMod(OutfMod::ArmorRech, [&](short val, short owned) {
                cap += (float)val * 0.001 * owned;
                return false;
            });
            if (cap < 0.0f)
                cap = 0.0f;
            playerStats.armorRechargeRate = cap;
        }
    }
    if (cap < 0.0f)
        cap = 0.0f;
    if (IsPlayer() && cheatsActive != 0)
        cap *= 50.0f;
    return cap;
}

#define kMaxFuel        32000

// FUNCTION: EVNOVA 0x00463a20
// MAC: _ShipFuelCapacity i386 0x00002f21 ppc 0x00003c74
short ShipObj::FuelCapacity()
{
    int32_t total;

    total = Class().fuel;
    if (IsPlayer()) {
        if (playerStats.fuelCapacity >= 0.0f) {
            total = playerStats.fuelCapacity;
        } else {
            ForEachPlayerMod(OutfMod::FuelCapacity, [&](short val, short owned) {
                total += val * owned;
                return false;
            });
            if (total < 0)
                total = 0;
            if (total > kMaxFuel)
                total = kMaxFuel;
            playerStats.fuelCapacity = total;
        }
    }
    return total;
}

// FUNCTION: EVNOVA 0x00463b30
// MAC: _ShipFuelGenRate i386 0x00002ffb ppc 0x00003d88
Boolean ShipObj::FuelGenRate(float *rate)
{
    short i;
    short j;

    if (fuelRegenState < 0) {
        fuelRegenState = 0;
        fuelRegenRate = 0;
        if (Class().fuelRegen > 0 &&
            ((Class().flags & kShipFuelRegen) != 0 ||
             !IsPlayer())) {
            fuelRegenState = 1;
            fuelRegenRate = 1.0 / (float)Class().fuelRegen;
        }
        if (IsPlayer()) {
            ForEachPlayerMod(OutfMod::FuelRegen, [&](short val, short owned) {
                fuelRegenState = 1;
                fuelRegenRate += (float)owned * (1.0 / (float)val);
                return false;
            });
        } else {
            for (i = 0; i < kDefaultOutfitSlots; i++) {
                if (Class().defOutfCount[i] > 0) {
                    short id = Class().defOutfID[i];

                    if (id >= 0 && id < kMaxOutfits) {
                        for (j = 0; j < kOutfMods; j++) {
                            if (outfitSystem[id].modType[j] == OutfMod::FuelRegen) {
                                fuelRegenState = 1;
                                fuelRegenRate += (float)Class().defOutfCount[i]
                                    * (1.0 / (float)outfitSystem[id].modVal[j]);
                            }
                        }
                    }
                }
            }
        }
    }
    if (fuelRegenState == 0) {
        *rate = 0;
        return 0;
    }
    *rate = fuelRegenRate;
    return 1;
}

// FUNCTION: EVNOVA 0x00463e70
// MAC: _ShipTurnRate i386 0x000032f7 ppc 0x000041b8
float ShipObj::TurnRate()
{
    float v;
    float ratio;

    v = Class().turnRate;
    if (IsPlayer()) {
        if (playerStats.turnRate >= 0.0f) {
            v = playerStats.turnRate;
        } else {
            ForEachPlayerMod(OutfMod::Turn, [&](short val, short owned) {
                v += val * owned * 0.01;
                return false;
            });
            if (v < 1.0f && Class().turnRate > 0.0f)
                v = 1.0f;
            playerStats.turnRate = v;
        }
    } else {
        if (tractorShip != -1 && tractorShip != index)
            v *= 0.333;
    }
    if (Class().turnRate >= 1.0f && v < 1.0f)
        v = 1.0f;
    if (ionization > 0.0f && jumpTimer <= 0.0f) {
        ratio = IonizationFactor();
        if (ratio > 0.7)
            ratio = 0.7f;
        v *= (1.0 - ratio);
    }
    if (v < 0.0f)
        v = 0.0f;
    return v;
}

// FUNCTION: EVNOVA 0x004640a0
// MAC: _ShipAccelRate i386 0x0000353b ppc 0x00004490
float ShipObj::AccelRate()
{
    float v;
    float ratio;

    v = Class().accel;
    if (IsPlayer()) {
        if (playerStats.accelRate >= 0.0f) {
            v = playerStats.accelRate;
        } else {
            ForEachPlayerMod(OutfMod::Accel, [&](short val, short owned) {
                v += (float)((float)(val / 10000.0) * owned);
                return false;
            });
            if (v < 0.0f)
                v = 0.0f;
            playerStats.accelRate = v;
        }
        v *= 2.0;
    } else {
        if ((Class().flags & kShipPlanetType) != 0)
            return 0.0f;
        v *= skill;
        if (govt != -1)
            v *= govtData[govt].skillMult;
        if (tractorShip != -1 && tractorShip != index)
            v *= 0.333;
        else if (parentShip == 0)
            v *= 2.0;
        else
            v *= 2.0;
    }
    if (ionization > 0.0f) {
        ratio = IonizationFactor();
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
float ShipObj::MaxSpeed()
{
    float v;

    v = Class().maxSpeed;
    if (IsPlayer()) {
        if (playerStats.maxSpeed >= 0.0f) {
            v = playerStats.maxSpeed;
        } else {
            ForEachPlayerMod(OutfMod::Speed, [&](short val, short owned) {
                v += (float)((float)(val / 100.0) * owned);
                return false;
            });
            if (v < 0.0f)
                v = 0.0f;
            playerStats.maxSpeed = v;
        }
    } else {
        if ((Class().flags & kShipPlanetType) != 0)
            return 0.0f;
        v *= skill;
        if (govt != -1)
            v *= govtData[govt].skillMult;
        if (tractorShip != -1 && tractorShip != index)
            v *= 0.333;
    }
    if (IsPlayer() || parentShip == 0) {
        if (strictPlay == 0)
            v *= 1.5;
    }
    if (v < 0.0f)
        v = 0.0f;
    return v;
}

/* Guidance 99 is the carried-ship type. */

// FUNCTION: EVNOVA 0x004644a0
// MAC: _HasEscapePod i386 0x0000b83d ppc 0x0000e3e0
Boolean ShipObj::HasEscapePod()
{
    if (IsPlayer()) {
        if (PlayerHasMod(OutfMod::EscapePod))
            return 1;
    }
    if (HasEscapeShip())
        return 1;
    return 0;
}

// FUNCTION: EVNOVA 0x00464520
// MAC: _HasEscapeShip i386 0x0000386e ppc 0x000048c4
Boolean ShipObj::HasEscapeShip()
{
    short i;
    short escType;

    for (i = 0; i < kMaxWeapTypes; i++) {
        if (weaponStats[i].guidance == kGuidanceBay &&
            weap[i].ammo > 0) {
            escType = weaponStats[i].ammoType - 128;
            if ((shipClass[escType].flags & kShipEscapeShip) != 0)
                return 1;
        }
    }
    return 0;
}

// FUNCTION: EVNOVA 0x00464590
// MAC: _EscapeShipType i386 0x000038d0 ppc 0x00004938
short ShipObj::EscapeShipType()
{
    short i;
    short escType;

    for (i = 0; i < kMaxWeapTypes; i++) {
        if (weaponStats[i].guidance == kGuidanceBay &&
            weap[i].ammo > 0) {
            escType = weaponStats[i].ammoType - 128;
            if ((shipClass[escType].flags & kShipEscapeShip) != 0)
                return escType;
        }
    }
    return -1;
}

// FUNCTION: EVNOVA 0x00464600
// MAC: _EscapeShipWeapType i386 0x00003936 ppc 0x000049ac
short ShipObj::EscapeShipWeapType()
{
    short i;
    short escType;

    for (i = 0; i < kMaxWeapTypes; i++) {
        if (weaponStats[i].guidance == kGuidanceBay && weap[i].ammo > 0) {
            escType = weaponStats[i].ammoType - kFirstResourceID;
            if ((shipClass[escType].flags & kShipEscapeShip) != 0)
                return i;
        }
    }
    return -1;
}

// FUNCTION: EVNOVA 0x00464670
// MAC: _KeyCarriedShipTypeOnboard i386 0x0000399c ppc 0x00004a28
Boolean ShipObj::KeyCarriedShipTypeOnboard()
{
    short i;
    short escort;
    short bay;

    escort = Class().keyCarried;
    if (escort == -1)
        return 0;
    for (i = 0; i < kMaxWeapTypes; i++) {
        if (weaponStats[i].guidance == kGuidanceBay &&
            weap[i].count > 0 && weap[i].ammo > 0) {
            bay = (weaponStats[i].ammoType - 128);
            if (bay == escort)
                return 1;
        }
    }
    return 0;
}

// FUNCTION: EVNOVA 0x00464700
// MAC: _HasAutoEject i386 0x00003a14 ppc 0x00004ab4
Boolean ShipObj::HasAutoEject()
{
    if (IsPlayer()) {
        if (PlayerHasMod(OutfMod::AutoEject))
            return 1;
    }
    return 0;
}

// FUNCTION: EVNOVA 0x00464760
// MAC: _HasAfterburner i386 0x00003a6d ppc 0x00004b24
Boolean ShipObj::HasAfterburner()
{
    if (cheatsActive != 0)
        return 1;
    if (IsPlayer()) {
        if (playerStats.hasAfterburner >= 0) {
            if (playerStats.hasAfterburner != 0)
                return 1;
        } else {
            playerStats.hasAfterburner = 0;
            if (PlayerHasMod(OutfMod::Afterburner)) {
                playerStats.hasAfterburner = 1;
                return 1;
            }
        }
    }
    return 0;
}

// FUNCTION: EVNOVA 0x00464810
// MAC: _ShipECM i386 0x00003aff ppc 0x00004bdc
short ShipObj::ECM(short kind)
{
    if (kind < 0 || kind >= 4)
        return 0;
    if (IsDisabled())
        return 0;
    if (jamming[kind] >= 0)
        return jamming[kind];

    if (Class().inherentGovtB >= 0 &&
        Class().inherentGovtB < kMaxGovts)
        jamming[kind] =
            govtData[Class().inherentGovtB].inhJam[kind];
    else
        jamming[kind] = 0;

    if (IsPlayer()) {
        ForEachPlayerMod(OutfMod::Jam1 + kind, [&](short val, short) {
            jamming[kind] += val;
            return false;
        });
    } else {
        ForEachDefaultMod(Class(), OutfMod::Jam1 + kind, [&](short val, short) {
            jamming[kind] += val;
            return false;
        });
        if (govt >= 0 && govt < kMaxGovts &&
            (govtData[govt].flags & kGovtFreighterHalfJam) != 0)
            jamming[kind] = jamming[kind] / 2;
    }

    if (jamming[kind] < 0)
        jamming[kind] = 0;
    if (jamming[kind] > 100)
        jamming[kind] = 100;
    return jamming[kind];
}

// FUNCTION: EVNOVA 0x00465550
// MAC: _ShipHyperTransitTime i386 0x000043d0 ppc 0x00005774
short ShipObj::HyperTransitTime()
{
    short count;

    count = 1;
    if (Class().mass >= 100)
        count++;
    if (Class().mass >= 200)
        count++;
    if (IsPlayer()) {
        ForEachPlayerMod(OutfMod::DaysPerJump, [&](short val, short owned) {
            count += val * owned;
            return false;
        });
    }
    if (count < 1)
        count = 1;
    return count;
}

/*
 * A squared range: floored at zero and then squared, so the caller can compare
 * against a squared distance without a square root.
 */

// FUNCTION: EVNOVA 0x00465610
// MAC: _ShipHyperSafeDist i386 0x00004485 ppc 0x00005830
int32_t ShipObj::HyperSafeDist()
{
    int32_t range;

    range = 1000;
    if (IsPlayer()) {
        ForEachPlayerMod(OutfMod::HyperSafeDist, [&](short val, short owned) {
            range += val * owned;
            return false;
        });
    }
    if (range < 0)
        range = 0;
    return range * range;
}

/*
 * Cached in a short that is below -100 until computed and clamped to -100..100
 * once it is.
 */

// FUNCTION: EVNOVA 0x0046abb0
// MAC: _ShipAntiInterference i386 0x00005f2c ppc 0x0000772c
short ShipAntiInterference(void)
{
    short total;

    if (playerStats.antiInterference < -100) {
        total = 0;
        ForEachPlayerMod(OutfMod::Interference, [&](short val, short owned) {
            total += val * owned;
            return false;
        });
        if (total > 100)
            total = 100;
        if (total < -100)
            total = -100;
        playerStats.antiInterference = total;
        return total;
    }
    return playerStats.antiInterference;
}

// FUNCTION: EVNOVA 0x0046c080
// MAC: _ShipDeionizationRate i386 0x00006ec3 ppc 0x00008908
float ShipObj::DeionizationRate()
{
    float value;

    value = Class().deionize;
    if (IsPlayer()) {
        if (playerStats.deionizeRate >= 0.0f) {
            value = playerStats.deionizeRate;
        } else {
            ForEachPlayerMod(OutfMod::Deionize, [&](short val, short owned) {
                value = value + val * 0.01 * owned;
                return false;
            });
            playerStats.deionizeRate = value;
        }
    }
    return value;
}

// FUNCTION: EVNOVA 0x0046c160
// MAC: _ShipIonizationFactor i386 0x00003225 ppc 0x00004094
float ShipObj::IonizationFactor()
{
    float total;

    total = Class().ionizeMax;
    if (IsPlayer()) {
        if (playerStats.ionizeMax >= 0.0f) {
            total = playerStats.ionizeMax;
        } else {
            ForEachPlayerMod(OutfMod::IonCapacity, [&](short val, short owned) {
                total = total + val * owned;
                return false;
            });
            playerStats.ionizeMax = total;
        }
    }
    if (total > 0.0f)
        return ionization / total;
    return 0.0f;
}

// FUNCTION: EVNOVA 0x0046c250
// MAC: _PlayerEffectiveMurk i386 0x00006f8f ppc 0x00008a18
short PlayerEffectiveMurk(void)
{
    short level;

    level = systemData[shipData[0].curSystem].murk;
    if (level < 0)
        level = 0;
    ForEachPlayerMod(OutfMod::Murk, [&](short val, short owned) {
        level += val * owned;
        return false;
    });
    if (level < 0)
        level = 0;
    if (level > 100)
        level = 100;
    return level;
}

// FUNCTION: EVNOVA 0x0046cb90
// MAC: _ShipCanScoop i386 0x000075c7 ppc 0x000090dc
Boolean ShipObj::CanScoop()
{
    if (IsPlayer()) {
        if (PlayerHasMod(OutfMod::Scoop))
            return 1;
    } else {
        if (DefaultHasMod(Class(), OutfMod::Scoop))
            return 1;
    }
    return 0;
}

// FUNCTION: EVNOVA 0x0046cdd0
// MAC: _ShipJumpsPerJump i386 0x000077b4 ppc 0x00009394
short ShipObj::JumpsPerJump()
{
    short total;

    total = 1;
    if (IsPlayer()) {
        ForEachPlayerMod(OutfMod::MultiJump, [&](short val, short) {
            total += val;
            return false;
        });
    } else {
        ForEachDefaultMod(Class(), OutfMod::MultiJump, [&](short val, short) {
            total += val;
            return false;
        });
    }
    if (total < 1)
        total = 1;
    return total;
}

// FUNCTION: EVNOVA 0x0046cec0
// MAC: _ShipMaxWeaponRange i386 0x0000bafd ppc 0x0000e734
short ShipObj::MaxWeaponRange()
{
    int32_t best;
    int32_t range;
    short i;

    best = 0;
    for (i = 0; i < kMaxWeapTypes; i++) {
        if (weap[i].count > 0 && WeaponHasAmmo(this, i) != 0) {
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

// FUNCTION: EVNOVA 0x0046d080
// MAC: _ShipCanExpiditeJumps i386 0x00007872 ppc 0x00009498
Boolean ShipObj::CanExpediteJumps()
{
    if ((Class().flags2 & kShipJumpsAtSpeed) != 0)
        return 1;
    if (IsPlayer()) {
        if (PlayerHasMod(OutfMod::FastJump))
            return 1;
    } else {
        if (DefaultHasMod(Class(), OutfMod::FastJump))
            return 1;
    }
    return 0;
}

void PlayerStats::Reset()
{
    shieldCapacity = -1.0f;
    shieldRechargeRate = -1.0f;
    armorCapacity = -1.0f;
    armorRechargeRate = -1.0f;
    turnRate = -1.0f;
    accelRate = -1.0f;
    maxSpeed = -1.0f;
    fuelCapacity = -1.0f;
    ionizeMax = -1.0f;
    deionizeRate = -1.0f;
    hasAfterburner = -1;
    hasDensityScanner = -1;
    hasIFF = -1;
    hasInertialDamper = -1;
    antiInterference = 0x8300;
    effectiveMurk = -1;
    canTargetUntargetableShips = -1;
    canTargetCloakedShips = -1;
    abFuelUsage = -1.0f;
    resistsGravity = -1;
    resistsDeadlyStellars = -1;
    hasAreaCloak = -1;
}

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

    playerStats.Reset();
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
                if (outfitSystem[i].modType[j] == OutfMod::BombDestroys) {
                    playerSabotaged = 1;
                    sabotageCounter = 0.0f;
                }
                if (outfitSystem[i].modType[j] == OutfMod::BombDisables) {
                    if (playerSabotaged < 1)
                        playerSabotaged = 2;
                    sabotageCounter = 0.0f;
                }
                if (outfitSystem[i].modType[j] == OutfMod::BlockReinforce) {
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
                if (outfitSystem[i].modType[j] == OutfMod::IFFScrambler) {
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
                if (outfitSystem[i].modType[j] == OutfMod::Scoop)
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
    playerStats.effectiveMurk = PlayerEffectiveMurk();
}

// FUNCTION: EVNOVA 0x0046df70
// MAC: _ShipIsInertialess i386 0x000079c2 ppc 0x00009640
Boolean ShipObj::IsInertialess()
{
    if (!IsPlayer() && parentShip != -1 && IsShipInCloseForm(this))
        return 0;
    if ((Class().flags2 & kShipInertialess) != 0)
        return 1;
    if (IsPlayer()) {
        if (playerStats.hasInertialDamper >= 0) {
            if (playerStats.hasInertialDamper != 0)
                return 1;
        } else {
            playerStats.hasInertialDamper = 0;
            if (PlayerHasMod(OutfMod::Inertialess)) {
                playerStats.hasInertialDamper = 1;
                return 1;
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
float ShipObj::AfterburnerFuelUsage()
{
    short i;
    short j;

    if (IsPlayer()) {
        if (playerStats.abFuelUsage < 0.0) {
            playerStats.abFuelUsage = 0.0f;
            for (i = 0; i < kMaxOutfits; i++) {
                if (shipSystems[i] > 0) {
                    for (j = 0; j < kOutfMods; j++) {
                        if (outfitSystem[i].modType[j] == OutfMod::Afterburner) {
                            playerStats.abFuelUsage = outfitSystem[i].modVal[j] * 0.0333;
                            break;
                        }
                    }
                }
            }
        }
        return playerStats.abFuelUsage;
    }
    return 0.0f;
}

// FUNCTION: EVNOVA 0x0046e120
// MAC: _ShipResistsGravity i386 0x00007b10 ppc 0x00009850
short ShipObj::ResistsGravity()
{
    if ((Class().flags3 & kShipIgnoresDeadly) != 0)
        return 1;
    if (IsInertialess())
        return 1;
    if (IsPlayer()) {
        if (playerStats.resistsGravity != -1) {
            if (playerStats.resistsGravity != 0)
                return 1;
        } else {
            playerStats.resistsGravity = 0;
            if (PlayerHasMod(OutfMod::ResistGravity))
                playerStats.resistsGravity = 1;
        }
        return playerStats.resistsGravity;
    }
    return 0;
}

// FUNCTION: EVNOVA 0x0046e210
// MAC: _ShipResistsDeadlyStellars i386 0x00007bbe ppc 0x00009950
short ShipObj::ResistsDeadlyStellars()
{
    if ((Class().flags3 & kShipIgnoresDeadly) != 0)
        return 1;
    if (IsPlayer()) {
        if (playerStats.resistsDeadlyStellars != -1) {
            if (playerStats.resistsDeadlyStellars != 0)
                return 1;
        } else {
            playerStats.resistsDeadlyStellars = 0;
            if (PlayerHasMod(OutfMod::ResistDeadly))
                playerStats.resistsDeadlyStellars = 1;
        }
        return playerStats.resistsDeadlyStellars;
    }
    return 0;
}

// FUNCTION: EVNOVA 0x0046e540
// MAC: _ShipCanSelfRepair i386 0x00009134 ppc 0x0000b258
Boolean ShipObj::CanSelfRepair()
{
    if (gSpeedMult > 0.0) {
        if (Rand((500.0 / gSpeedMult)) != 0)
            return 0;
    } else {
        if (Rand(500) != 0)
            return 0;
    }
    if (IsDying())
        return 0;

    if (IsPlayer()) {
        if (PlayerHasMod(OutfMod::RepairDroids))
            return 1;
    } else {
        if (DefaultHasMod(Class(), OutfMod::RepairDroids))
            return 1;
    }
    return 0;
}
