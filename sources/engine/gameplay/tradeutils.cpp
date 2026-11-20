/* tradeutils.cpp -- buy/sell eligibility, port availability and demand, cargo and holds */

#include "game.h"
#include "player.h"
#include "ships.h"
#include "prototypes.h"
#include "dialogs.h"
#include "missions.h"

EXTERN_C_BEGIN

#define kModTypeMaxOwned   0x1b
#define kModTypeGunLimit   0x2d
#define kModTypeTurretLimit 0x2e

// FUNCTION: EVNOVA 0x004656a0
// MAC: _HasMaxOfItem i386 0x00004512 ppc 0x000058b4
Boolean HasMaxOfItem(short index, short *outBound, short *outCap)
{
    short sumFlag1;
    short sumFlag2;
    short j;
    short k;
    short sumOwned;
    short limit;
    short ammo;

    if (index < 0 || index >= kMaxOutfits) {
        if (outBound)
            *outBound = 0;
        if (outCap)
            *outCap = 0;
        return 1;
    }

    limit = outfitSystem[index].maxCount;
    if (outCap)
        *outCap = limit;

    if (outfitSystem[index].modType[0] == kOutfModAmmo) {
        if (weaponStats[outfitSystem[index].modVal[0]].maxAmmo > 0) {
            ammo = weaponStats[outfitSystem[index].modVal[0]].maxAmmo * shipData->weap[outfitSystem[index].modVal[0]].count;
            if (outCap && ammo < *outCap)
                *outCap = ammo;
            if (shipSystems[index] >= ammo) {
                if (outBound)
                    *outBound = ammo;
                return 1;
            }
        }
    }

    sumOwned = 0;
    for (j = 0; j < kMaxOutfits; j++) {
        if (shipSystems[j] > 0) {
            for (k = 0; k < kOutfMods; k++) {
                if (outfitSystem[j].modType[k] == kModTypeMaxOwned &&
                    outfitSystem[j].modVal[k] == index + 128)
                    sumOwned += shipSystems[j];
            }
        }
    }
    if (sumOwned < 1)
        sumOwned = 1;
    limit = sumOwned * outfitSystem[index].maxCount;
    if (outCap && (limit < *outCap || sumOwned > 1))
        *outCap = limit;
    if (shipSystems[index] >= limit) {
        if (outBound)
            *outBound = limit;
        return 1;
    }

    sumFlag1 = sumFlag2 = 0;
    for (j = 0; j < kMaxOutfits; j++) {
        if (shipSystems[j] > 0) {
            if (outfitSystem[j].flags & kOutfFixedGun)
                sumFlag1 += shipSystems[j];
            if (outfitSystem[j].flags & kOutfTurret)
                sumFlag2 += shipSystems[j];
        }
    }

    if (outfitSystem[index].flags & kOutfFixedGun) {
        limit = shipClass[shipData->type].maxGuns;
        for (j = 0; j < kMaxOutfits; j++) {
            if (shipSystems[j] > 0) {
                for (k = 0; k < kOutfMods; k++) {
                    if (outfitSystem[j].modType[k] == kModTypeGunLimit) {
                        limit += outfitSystem[j].modVal[k];
                        break;
                    }
                }
            }
        }
        if (outCap && limit < *outCap)
            *outCap = limit;
        if (limit <= 0) {
            if (outBound)
                *outBound = 0;
            return 1;
        }
        if (sumFlag1 >= limit) {
            if (outBound)
                *outBound = limit;
            return 1;
        }
    }

    if (outfitSystem[index].flags & kOutfTurret) {
        limit = shipClass[shipData->type].maxTurrets;
        for (j = 0; j < kMaxOutfits; j++) {
            if (shipSystems[j] > 0) {
                for (k = 0; k < kOutfMods; k++) {
                    if (outfitSystem[j].modType[k] == kModTypeTurretLimit) {
                        limit += outfitSystem[j].modVal[k];
                        break;
                    }
                }
            }
        }
        if (outCap && limit < *outCap)
            *outCap = limit;
        if (limit <= 0) {
            if (outBound)
                *outBound = 0;
            return 1;
        }
        if (sumFlag2 >= limit) {
            if (outBound)
                *outBound = limit;
            return 1;
        }
    }

    if (outBound)
        *outBound = shipSystems[index];
    return 0;
}


// FUNCTION: EVNOVA 0x00465e50
// MAC: _CanBuyGoods i386 0x0000ccec ppc 0x0000fe3c
Boolean CanBuyGoods(short item)
{
    if (shipData->credits < tradePrice[item])
        return 0;
    if (TotalCargo(shipData) >= TotalFleetHolds())
        return 0;
    return 1;
}

// FUNCTION: EVNOVA 0x00465ea0
// MAC: _CanSellGoods i386 0x00004a94 ppc 0x00005efc
Boolean CanSellGoods(short item)
{
    short junk;

    if (item <= 5) {
        if (shipData->cargo[item] > 0)
            return 1;
    } else {
        junk = junkType[item - kFirstJunkItem];
        if (junk != -1 && junkData[junk].held > 0)
            return 1;
    }
    return 0;
}

#define kMaxHiredEscorts 6

// FUNCTION: EVNOVA 0x00468920
// MAC: _CanHireEscorts i386 0x00005795 ppc 0x00006df0
Boolean CanHireEscorts(void)
{
    short i;
    short escorts;

    escorts = 0;
    for (i = 1; i < kMaxShips; i++) {
        if (shipData[i].active != 0 && shipData[i].parentShip == 0 &&
            shipData[i].aiType == kShipTypeEscort && shipData[i].specialMission == -1)
            escorts++;
    }
    if (escorts < kMaxHiredEscorts)
        return 1;
    return 0;
}

// FUNCTION: EVNOVA 0x00469100
// MAC: _PlayerShipTradeInPrice i386 0x0000b079 ppc 0x0000d9cc
int32_t PlayerShipTradeInPrice(void)
{
    short i;
    int32_t value;
    int32_t cost;

    value = (shipClass[shipData->type].cost * 0.25);
    for (i = 0; i < kMaxOutfits; i++) {
        if (shipSystems[i] > 0 && outfitSystem[i].persistent == 0) {
            cost = AdjustedItemCost(&outfitSystem[i], &shipClass[shipData->type]);
            value = value + cost * shipSystems[i] * 0.5;
        }
    }
    if (value < 0)
        value = 0;
    return value;
}

// FUNCTION: EVNOVA 0x00469230
// MAC: _PlayerCanUseAmmoType i386 0x00005896 ppc 0x00006f40
Boolean PlayerCanUseAmmoType(short weapIndex)
{
    short i;

    for (i = 0; i < kMaxWeapTypes; i++) {
        if (shipData->weap[i].count > 0) {
            int32_t w = weapIndex;

            if (weaponStats[i].guidance == kGuidanceBay) {
                if (weaponStats[i].ammoType == weaponStats[w].ammoType)
                    return 1;
                return 0;
            }
            if (weapIndex == weaponStats[i].ammoType)
                return 1;
        }
    }
    return 0;
}

// FUNCTION: EVNOVA 0x004694a0
// MAC: _CanBuyFighter i386 0x00005a82 ppc 0x00007190
Boolean CanBuyFighter(short shipType, short outfitIndex, short weapIndex)
{
    short limit;
    short count;
    short i;
    short m;

    if (outfitIndex == -1 && weapIndex == -1) {
        for (i = 0; i < kMaxWeapTypes; i++) {
            if (shipData->weap[i].count > 0 &&
                weaponStats[i].guidance == kGuidanceBay &&
                weaponStats[i].ammoType - 128 == shipType) {
                weapIndex = i;
                break;
            }
        }
        if (weapIndex == -1) {
            for (i = 0; i < kMaxWeapTypes; i++) {
                if (shipData->weap[i].count > 0 &&
                    weaponStats[i].guidance == kGuidanceBay &&
                    shipClass[weaponStats[i].ammoType - 128].spriteIndex ==
                        shipClass[shipType].spriteIndex) {
                    weapIndex = i;
                    break;
                }
            }
        }
        if (weapIndex == -1)
            return 0;
        for (i = 0; i < kMaxOutfits; i++) {
            for (m = 0; m < kOutfMods; m++) {
                if (outfitSystem[i].techLevel < kOutfTechAbsent &&
                    outfitSystem[i].modType[m] == kOutfModAmmo &&
                    weapIndex == outfitSystem[i].modVal[m]) {
                    outfitIndex = i;
                    break;
                }
            }
        }
        if (outfitIndex == -1)
            return 0;
        count = shipData->weap[weapIndex].ammo;
    } else {
        if (outfitIndex == -1 || weapIndex == -1)
            return 0;
        count = shipSystems[outfitIndex];
    }

    if (weaponStats[weapIndex].maxAmmo > 0)
        limit = weaponStats[weapIndex].maxAmmo * shipData->weap[weapIndex].count;
    else
        limit = outfitSystem[outfitIndex].maxCount;

    for (m = 1; m < kMaxShips; m++) {
        if (shipData[m].active != 0 &&
            shipType == shipData[m].type &&
            shipData[m].parentShip == 0 &&
            shipData[m].aiType == kShipTypeFighter &&
            shipData[m].specialMission == -1)
            count++;
    }
    if (count < limit)
        return 1;
    return 0;
}

#define kMaxHolds 32000

// FUNCTION: EVNOVA 0x00469760
// MAC: _TotalFleetHolds i386 0x0000c24d ppc 0x0000ef88
short TotalFleetHolds(void)
{
    short i;
    int32_t total;

    total = ShipTotalHolds();
    for (i = 1; i < kMaxShips; i++) {
        if (shipData[i].active != 0 && !IsDying(&shipData[i])
                && shipData[i].parentShip == 0 && shipData[i].aiType == kShipTypeEscort
                && shipData[i].specialMission == -1
                && shipClass[shipData[i].type].inherentAI <= 2)
            total += shipClass[shipData[i].type].cargo;
    }
    if (total > kMaxHolds)
        total = kMaxHolds;
    return total;
}

// FUNCTION: EVNOVA 0x00469810
// MAC: _DestroyPartialFleetCargo i386 0x0000cd32 ppc 0x0000feb0
void DestroyPartialFleetCargo(short shipIndex)
{
    double newShipHolds;
    double fleetHolds;
    double totalCargo;
    double ratio;
    int32_t rounded;
    short i;

    if (shipIndex == 0) {
        newShipHolds = ShipTotalHolds();
    } else {
        newShipHolds = shipClass[shipData[shipIndex].type].cargo;
    }
    fleetHolds = ShipTotalHolds();
    for (i = 1; i < kMaxShips; i++) {
        if (shipData[i].active != 0 && (!IsDying(&shipData[i]) || i == shipIndex)
                && shipData[i].curSystem == shipData->curSystem
                && shipData[i].parentShip == 0 && shipData[i].aiType == kShipTypeEscort
                && shipData[i].specialMission == -1
                && shipClass[shipData[i].type].inherentAI <= 2)
            fleetHolds = shipClass[shipData[i].type].cargo + fleetHolds;
    }
    totalCargo = 0.0;
    for (i = 0; i <= kLastCommodity; i++)
        totalCargo += shipData->cargo[i];
    for (i = 0; i < kMaxJunks; i++)
        totalCargo += junkData[i].held;
    ratio = newShipHolds / totalCargo;
    ratio = newShipHolds / fleetHolds;
    if (ratio > 1.0)
        ratio = 1.0;

    for (i = 0; i <= kLastCommodity; i++) {
        rounded = (shipData->cargo[i] * ratio);
        if (shipData->cargo[i] <= rounded) {
            shipData->cargo[i] = 0;
        } else {
            shipData->cargo[i] -= rounded;
        }
        shipData[shipIndex].cargo[i] = rounded;
        if (shipData->cargo[i] < 0)
            shipData->cargo[i] = 0;
    }
    for (i = 0; i < kMaxJunks; i++) {
        rounded = (junkData[i].held * ratio);
        if (junkData[i].held < rounded) {
            junkData[i].held = 0;
        } else {
            junkData[i].held -= rounded;
        }
        if (junkData[i].held < 0)
            junkData[i].held = 0;
    }
    statusRedraw[kRedrawCargo] = 1;
    ResetPlayerPrecalcedValues();
}

#define kDemandMedium 2

// FUNCTION: EVNOVA 0x00469d30
// MAC: _CalcPortDemand i386 0x000056b1 ppc 0x00006d04
short CalcPortDemand(short commodity, uint32_t flags)
{
    switch (commodity) {
    case 0:
        if ((flags & kSpobDemandLow0) != 0)
            return kDemandLow;
        if ((flags & kSpobDemandMedium0) != 0)
            return kDemandMedium;
        if ((flags & kSpobDemandHigh0) != 0)
            return kDemandHigh;
        break;
    case 1:
        if ((flags & kSpobDemandLow1) != 0)
            return kDemandLow;
        if ((flags & kSpobDemandMedium1) != 0)
            return kDemandMedium;
        if ((flags & kSpobDemandHigh1) != 0)
            return kDemandHigh;
        break;
    case 2:
        if ((flags & kSpobDemandLow2) != 0)
            return kDemandLow;
        if ((flags & kSpobDemandMedium2) != 0)
            return kDemandMedium;
        if ((flags & kSpobDemandHigh2) != 0)
            return kDemandHigh;
        break;
    case 3:
        if ((flags & kSpobDemandLow3) != 0)
            return kDemandLow;
        if ((flags & kSpobDemandMedium3) != 0)
            return kDemandMedium;
        if ((flags & kSpobDemandHigh3) != 0)
            return kDemandHigh;
        break;
    case 4:
        if ((flags & kSpobDemandLow4) != 0)
            return kDemandLow;
        if ((flags & kSpobDemandMedium4) != 0)
            return kDemandMedium;
        if ((flags & kSpobDemandHigh4) != 0)
            return kDemandHigh;
        break;
    case 5:
        if ((flags & kSpobDemandLow5) != 0)
            return kDemandLow;
        if ((flags & kSpobDemandMedium5) != 0)
            return kDemandMedium;
        if ((flags & kSpobDemandHigh5) != 0)
            return kDemandHigh;
        break;
    }
    return 0;
}

#define kPortSpecialTech    8

// FUNCTION: EVNOVA 0x00469e90
// MAC: _SetupPortAvailableShipTypes i386 0x0000bbe3 ppc 0x0000e880
void SetupPortAvailableShipTypes(SpobRecord *port)
{
    short i;
    short j;
    short k;
    short threshold;
    short best;
    Boolean avail[kMaxShipTypes];

    if (port == 0)
        return;

    for (i = 0; i < kMaxShipTypes; i++)
        dockedPortShipTypes[i] = -1;

    k = 0;
    for (i = 0; i < kMaxShipTypes; i++) {
        avail[i] = 0;
        if (shipClass[i].techLevel >= 0) {
            if (shipClass[i].techLevel <= port->techLevel)
                avail[i] = 1;
            else {
                for (j = 0; j < kPortSpecialTech; j++) {
                    if (port->specialTech[j] == shipClass[i].techLevel) {
                        avail[i] = 1;
                        break;
                    }
                }
            }
        }

        if (avail[i]) {
            if (shipBuyMode == 0) {
                if (shipClass[i].buyRandom == 0
                        || shipClass[i].dailySaleRoll > shipClass[i].buyRandom)
                    avail[i] = 0;
            } else {
                if (shipClass[i].hireRandom == 0
                        || shipClass[i].dailyHireRoll > shipClass[i].hireRandom)
                    avail[i] = 0;
            }
        }

        if (avail[i]) {
            if (shipClass[i].flags3 & kShipHideIfRequireUnmet) {
                if (!PlayerMeetsRequirements(shipClass[i].requires[0],
                        shipClass[i].requires[1]))
                    avail[i] = 0;
            }
        }

        if (avail[i]) {
            if (shipClass[i].flags3 & kShipHideIfUnavailable) {
                if (!EvalMissionBitTestString(shipClass[i].availability))
                    avail[i] = 0;
            }
        }

        if (avail[i]) {
            if (shipClass[i].dispWeight > threshold || k == 0)
                threshold = shipClass[i].dispWeight;
            k++;
        }
    }

    for (i = 0; i < kMaxShipTypes; i++) {
        if (avail[i] && (shipClass[i].flags3 & kShipSupersedes)) {
            for (k = i + 1; k < kMaxShipTypes; k++) {
                if (shipClass[k].dispWeight == shipClass[i].dispWeight)
                    avail[k] = 0;
            }
        }
    }

    k = 0;
    do {
        j = 0;
        best = 0;
        for (i = 0; i < kMaxShipTypes; i++) {
            if (avail[i]) {
                if (shipClass[i].dispWeight >= threshold) {
                    dockedPortShipTypes[k] = i;
                    k++;
                    avail[i] = 0;
                } else if (shipClass[i].dispWeight > best || j == 0) {
                    best = shipClass[i].dispWeight;
                }
                j++;
            }
        }
        threshold = best;
    } while (j > 0);
}


// FUNCTION: EVNOVA 0x0046a220
// MAC: _SetupPortAvailableItems i386 0x0000bedb ppc 0x0000ebd0
Boolean SetupPortAvailableItems(SpobRecord *port)
{
    short i;
    short j;
    short count;
    short threshold;
    short best;
    short saved[kMaxOutfits];
    Boolean avail[kMaxOutfits];
    short n;

    if (port == 0)
        return 1;

    count = 0;
    for (i = 0; i < kMaxOutfits; i++) {
        saved[i] = dockedPortItems[i];
        dockedPortItems[i] = -1;
        avail[i] = 0;
        if ((port->flags2 & kSpobBuysAnyOutfit) && shipSystems[i] > 0
                && !(outfitSystem[i].flags & kOutfCantSell))
            avail[i] = 1;
        if (outfitSystem[i].techLevel >= 0
                && outfitSystem[i].techLevel < kOutfTechAbsent) {
            if (outfitSystem[i].techLevel <= port->techLevel)
                avail[i] = 1;
            else {
                for (j = 0; j < kPortSpecialTech; j++) {
                    if (port->specialTech[j] == outfitSystem[i].techLevel) {
                        avail[i] = 1;
                        break;
                    }
                }
            }
        }

        if (avail[i]) {
            if ((outfitSystem[i].flags & kOutfTestAvailability)
                    && shipSystems[i] <= 0)
                avail[i] = EvalMissionBitTestString(outfitSystem[i].availability);
        }

        if (avail[i]) {
            if ((outfitSystem[i].flags & kOutfCheckRequire)
                    && shipSystems[i] <= 0)
                avail[i] = PlayerMeetsRequirements(outfitSystem[i].requires[0],
                        outfitSystem[i].requires[1]);
        }

        if (avail[i]) {
            if (shipSystems[i] <= 0) {
                if (outfitSystem[i].buyRandom <= 0
                        || outfitSystem[i].dailyRoll
                                > outfitSystem[i].buyRandom)
                    avail[i] = 0;
            } else
                outfitSystem[i].dailyRoll = 0;
        }

        if (outfitSystem[i].flags & kOutfSellAnywhere) {
            if (shipSystems[i] > 0)
                avail[i] = 1;
        }

        if (avail[i]) {
            if (outfitSystem[i].dispWeight > threshold || count == 0)
                threshold = outfitSystem[i].dispWeight;
            count++;
        }
    }

    for (i = 0; i < kMaxOutfits; i++) {
        if (avail[i] && (outfitSystem[i].flags & kOutfSupersedes)) {
            for (count = i + 1; count < kMaxOutfits; count++) {
                if (outfitSystem[count].dispWeight == outfitSystem[i].dispWeight)
                    avail[count] = 0;
            }
        }
    }

    count = 0;
    do {
        j = 0;
        best = 0;
        for (i = 0; i < kMaxOutfits; i++) {
            if (avail[i]) {
                if (outfitSystem[i].dispWeight >= threshold) {
                    dockedPortItems[count] = i;
                    count++;
                    avail[i] = 0;
                } else {
                    n = outfitSystem[i].dispWeight;
                    if (n > best || j == 0)
                        best = n;
                }
                j++;
            }
        }
        threshold = best;
    } while (j > 0);

    for (i = 0; i < kMaxOutfits; i++) {
        if (saved[i] != dockedPortItems[i])
            return 1;
    }
    return 0;
}

// FUNCTION: EVNOVA 0x0046a5d0
// MAC: _TotalCargo i386 0x00004a0b ppc 0x00005e3c
short TotalCargo(ShipObj *ship)
{
    short i;
    short total;

    total = 0;
    for (i = 0; i <= kLastCommodity; i++)
        total += ship->cargo[i];
    if (ship->index == 0) {
        for (i = 0; i < kMaxActiveMissions; i++) {
            if (currentMissionObjectives[i].active != 0 && currentMissions[i].hasCargo != 0
                    && currentMissions[i].cargoQty >= 0)
                total += currentMissions[i].cargoQty;
        }
        for (i = 0; i < kMaxJunks; i++) {
            if (junkData[i].held > 0)
                total += junkData[i].held;
        }
    }
    return total;
}

// FUNCTION: EVNOVA 0x0046a680
// MAC: _PlayerHasAnyCargo i386 0x00005d77 ppc 0x000074f0
Boolean PlayerHasAnyCargo(void)
{
    short i;

    for (i = 0; i <= kLastCommodity; i++) {
        if (shipData[0].cargo[i] > 0)
            return 1;
    }
    for (i = 0; i < kMaxActiveMissions; i++) {
        if (currentMissionObjectives[i].active != 0 && currentMissions[i].hasCargo != 0
                && currentMissions[i].cargoType != -1 && currentMissions[i].cargoQty >= 0)
            return 1;
    }
    for (i = 0; i < kMaxJunks; i++) {
        if (junkData[i].held > 0)
            return 1;
    }
    return 0;
}

/* the player's own hold capacity, outfit modifiers included */
// FUNCTION: EVNOVA 0x0046a730
// MAC: _ShipTotalHolds i386 0x00005cb3 ppc 0x0000740c
short ShipTotalHolds(void)
{
    short holds;
    short i;
    short j;

    holds = shipClass[shipData->type].cargo;
    for (i = 0; i < kMaxOutfits; i++) {
        if (shipSystems[i] > 0) {
            for (j = 0; j < kOutfMods; j++) {
                if (outfitSystem[i].modType[j] == kOutfModCargo)
                    holds += outfitSystem[i].modVal[j] * shipSystems[i];
            }
        }
    }
    return holds;
}

// FUNCTION: EVNOVA 0x0046a7c0
// MAC: _TotalMissionCargoSpace i386 0x0000cffb ppc 0x00010310
short TotalMissionCargoSpace(void)
{
    short holds;
    short fleetHolds;
    short cargo;
    short missionUnits;
    short i;
    short spare;
    short result;

    holds = ShipTotalHolds();
    fleetHolds = TotalFleetHolds();
    cargo = TotalCargo(shipData);
    missionUnits = 0;
    for (i = 0; i < kMaxActiveMissions; i++) {
        if (currentMissionObjectives[i].active != 0 && currentMissions[i].hasCargo != 0
                && currentMissions[i].cargoQty >= 0)
            missionUnits += currentMissions[i].cargoQty;
    }
    if (fleetHolds > holds) {
        spare = cargo - missionUnits - (fleetHolds - holds);
        if (spare < 0)
            spare = 0;
        spare += missionUnits;
        result = holds - spare;
    } else {
        result = holds - cargo;
    }
    return result;
}

// FUNCTION: EVNOVA 0x0046cca0
// MAC: _GetPlayerContributeBits i386 0x000076b2 ppc 0x00009224
void GetPlayerContributeBits(uint32_t *lo, uint32_t *hi)
{
    uint32_t bits[2];
    short i;

    bits[0] = shipClass[shipData->type].contributes[0];
    bits[1] = shipClass[shipData->type].contributes[1];
    for (i = 0; i < kMaxRanks; i++) {
        if (rankData[i].active != '\0' && rankData[i].loaded != '\0') {
            bits[0] |= rankData[i].contributes[0];
            bits[1] |= rankData[i].contributes[1];
        }
    }
    for (i = 0; i < kMaxCrons; i++) {
        if (cronEvent[i].active != '\0' && cronEvent[i].loaded != '\0'
                && cronEvent[i].holdoffLeft <= 0) {
            bits[0] |= cronEvent[i].contributes[0];
            bits[1] |= cronEvent[i].contributes[1];
        }
    }
    for (i = 0; i < kMaxOutfits; i++) {
        if (shipSystems[i] > 0) {
            bits[0] |= outfitSystem[i].contributes[0];
            bits[1] |= outfitSystem[i].contributes[1];
        }
    }
    *lo = bits[0];
    *hi = bits[1];
}

// FUNCTION: EVNOVA 0x0046cd80
// MAC: _PlayerMeetsRequirements i386 0x0000776f ppc 0x00009334
Boolean PlayerMeetsRequirements(uint32_t reqLo, uint32_t reqHi)
{
    uint32_t bits[2];
    uint32_t gotLo;
    uint32_t gotHi;

    GetPlayerContributeBits(&bits[0], &bits[1]);
    gotLo = bits[0] & reqLo;
    gotHi = bits[1] & reqHi;
    if (gotLo == reqLo)
        if (gotHi == reqHi)
            return 1;
    return 0;
}

// FUNCTION: EVNOVA 0x0046e910
// MAC: _AdjustedItemCost i386 0x00005869 ppc 0x00006f04
int32_t AdjustedItemCost(OutfRecord *outf, ShipRec *ship)
{
    int32_t cost;

    cost = outf->cost;
    if (cost <= 0)
        return 0;
    if (OutfCostScaled(outf)) {
        cost *= ship->mass;
        if (cost < outf->cost)
            cost = outf->cost;
    }
    return cost;
}

// FUNCTION: EVNOVA 0x0046e950
// MAC: _AdjustedItemMass i386 0x00002889 ppc 0x00003330
int32_t AdjustedItemMass(OutfRecord *outf, ShipRec *ship)
{
    short mass;
    int32_t adjusted;

    mass = outf->mass;
    adjusted = mass;
    if (adjusted <= 0)
        return adjusted;
    if (OutfMassScaled(outf)) {
        adjusted = adjusted * ship->mass;
        adjusted *= 0.01;
        if (adjusted < mass)
            adjusted = mass;
    }
    return adjusted;
}

// FUNCTION: EVNOVA 0x0046ea40
// MAC: _PlayerHasFightersOfOutfitType i386 0x00007ff7 ppc 0x00009e2c
short PlayerHasFightersOfOutfitType(short outfitIndex)
{
    short i;
    short k;
    short bayType = -1;
    short weapon;
    short count;

    for (i = 0; i < kOutfMods; i++) {
        if (outfitSystem[outfitIndex].modType[i] == kOutfModAmmo) {
            weapon = outfitSystem[outfitIndex].modVal[i];
            if (weapon >= 0 && weapon < kMaxWeapTypes) {
                if (weaponStats[weapon].guidance == kGuidanceBay)
                    bayType = weaponStats[weapon].ammoType - 128;
            }
        }
        if (bayType != -1)
            break;
    }
    if (bayType == -1)
        return 0;

    count = 0;
    for (k = 1; k < kMaxShips; k++) {
        if (shipData[k].active == 0)
            continue;
        if (shipData[k].parentShip != 0)
            continue;
        if (shipData[k].aiType != kShipTypeFighter)
            continue;
        if (bayType != shipData[k].type)
            continue;
        if (IsDisabled(&shipData[k]))
            continue;
        count++;
    }
    if (count > 0)
        return count;

    for (k = 0; k < kMaxWeapTypes; k++) {
        if (weaponStats[k].guidance != kGuidanceBay)
            continue;
        if (shipData[0].weap[k].count <= 0)
            continue;
        if (weaponStats[k].ammoType - 128 != bayType)
            continue;
        if (shipData[0].weap[k].ammo > 0)
            return shipData[0].weap[k].ammo;
    }
    return 0;
}

// FUNCTION: EVNOVA 0x0046f140
// MAC: _PlayerHasJettisonableCargo i386 0x0000825a ppc 0x0000a164
Boolean PlayerHasJettisonableCargo(void)
{
    short i;

    for (i = 0; i < kCargoTypes; i++) {
        if (shipData[0].cargo[i] > 0)
            return 1;
    }
    for (i = 0; i < kMaxJunks; i++) {
        if (junkData[i].held > 0)
            return 1;
    }
    for (i = 0; i < kMaxActiveMissions; i++) {
        if (currentMissionObjectives[i].active != 0 && currentMissions[i].hasCargo != 0
                && currentMissions[i].cargoType != -1 && currentMissions[i].cargoQty >= 0
                && currentMissions[i].canAbort != 0)
            return 1;
    }
    return 0;
}

EXTERN_C_END
