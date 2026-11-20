/* shipcloak.cpp -- cloaking, detection and targetability */

#include "burgermath.h"
#include "ai.h"
#include "game.h"
#include "player.h"
#include "render.h"
#include "ships.h"
#include "sound.h"

EXTERN_C_BEGIN

#define kDetectRange    200.0f

// FUNCTION: EVNOVA 0x00464a90
// MAC: _ShipVisibleToShip i386 0x0000971c ppc 0x0000ba14
Boolean ShipVisibleToShip(ShipObj *ship, ShipObj *viewer)
{
    if (AIShipIsEmergingFromHypergate(ship))
        return 0;
    if (ShipIsCloaked(ship)) {
        float d;

        if (viewer->parentShip == ship->index) {
            if (HasCloak(viewer))
                return 1;
        }
        if (viewer->seesCloaked[0] == 1) {
            d = FloatAbs(viewer->pos.x - ship->pos.x);
            if (d <= kDetectRange) {
                d = FloatAbs(viewer->pos.y - ship->pos.y);
                if (d <= kDetectRange)
                    return 1;
            }
        }
        return 0;
    }
    return 1;
}

#define kOutfModType13 0xd
#define kOutfModType14 0xe
#define kOutfModType30 0x1e
#define kOutfModVal30Bit2 4
#define kOutfModVal30Bit3 8
#define kOutfModNibblePair 0x11

/*
 * Every one of those four guards falls back to the default scan rather than to
 * a no, which is why they are written as a chain of `if (!x) goto the scan`
 * rather than a single `&&`.
 */

// FUNCTION: EVNOVA 0x00464b50
// MAC: _HasCloak i386 0x000095f2 ppc 0x0000b85c
Boolean HasCloak(ShipObj *ship)
{
    short i;
    short j;

    if (ship->index == 0) {
        for (i = 0; i < kMaxOutfits; i++) {
            if (shipSystems[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (outfitSystem[i].modType[j] == kOutfModType17)
                        return 1;
                }
            }
        }
    } else {
        if (ship->parentShip != -1 && IsShipInCloseForm(ship) &&
            ShipIsCloaked(&shipData[ship->parentShip]) &&
            HasAreaCloak(&shipData[ship->parentShip]))
            return 1;
        for (i = 0; i < kDefaultOutfitSlots; i++) {
            if (shipClass[ship->type].defOutfCount[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (outfitSystem[shipClass[ship->type].defOutfID[i]].modType[j] ==
                        kOutfModType17)
                        return 1;
                }
            }
        }
    }
    return 0;
}

/* the same question as HasCloak but only for values carrying bit 12 */
#define kOutfModVal17Bit12 0x1000

// FUNCTION: EVNOVA 0x00464c80
// MAC: _HasAreaCloak i386 0x00003d49 ppc 0x00004e84
Boolean HasAreaCloak(ShipObj *ship)
{
    short i;
    short j;

    if (ship->index == 0) {
        if (playerHasAreaCloak != -1) {
            if (playerHasAreaCloak == 1)
                return 1;
        } else {
            playerHasAreaCloak = 0;
            for (i = 0; i < kMaxOutfits; i++) {
                if (shipSystems[i] > 0) {
                    for (j = 0; j < kOutfMods; j++) {
                        if (outfitSystem[i].modType[j] == kOutfModType17 &&
                            (outfitSystem[i].modVal[j] & kOutfModVal17Bit12) != 0) {
                            playerHasAreaCloak = 1;
                            return 1;
                        }
                    }
                }
            }
        }
    } else {
        for (i = 0; i < kDefaultOutfitSlots; i++) {
            if (shipClass[ship->type].defOutfCount[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (outfitSystem[shipClass[ship->type].defOutfID[i]].modType[j] ==
                            kOutfModType17 &&
                        (outfitSystem[shipClass[ship->type].defOutfID[i]].modVal[j] &
                            kOutfModVal17Bit12) != 0)
                        return 1;
                }
            }
        }
    }
    return 0;
}

/*
 * The first owned mod-type-17 outfit's high nibble (win 0x00464db0). What
 * that mod type means is not established here — only that its modVal is read
 * as two nibbles and this returns the upper one, with 0 for anything but the
 * player's own ship.
 */
// FUNCTION: EVNOVA 0x00464db0
// MAC: _CloakFuelUsage i386 0x00003e25 ppc 0x00004fb8
short CloakFuelUsage(ShipObj *ship)
{
    short i;
    short j;

    if (ship->index == 0) {
        for (i = 0; i < kMaxOutfits; i++) {
            if (shipSystems[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (outfitSystem[i].modType[j] == kOutfModNibblePair)
                        return (outfitSystem[i].modVal[j] >> 4) & 0xf;
                }
            }
        }
    }
    return 0;
}

// FUNCTION: EVNOVA 0x00464e30
// MAC: _CloakZeroesShields i386 0x00003e9d ppc 0x00005040
Boolean CloakZeroesShields(ShipObj *ship)
{
    short i;
    short j;

    if (ship->index == 0) {
        for (i = 0; i < kMaxOutfits; i++) {
            if (shipSystems[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (outfitSystem[i].modType[j] == kOutfModType17) {
                        if ((outfitSystem[i].modVal[j] & kOutfModVal30Bit2) != 0)
                            return 1;
                        return 0;
                    }
                }
            }
        }
    } else {
        for (i = 0; i < kDefaultOutfitSlots; i++) {
            if (shipClass[ship->type].defOutfCount[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (outfitSystem[shipClass[ship->type].defOutfID[i]].modType[j] ==
                        kOutfModType17) {
                        if ((outfitSystem[shipClass[ship->type].defOutfID[i]].modVal[j] &
                             kOutfModVal30Bit2) != 0)
                            return 1;
                        return 0;
                    }
                }
            }
        }
    }
    return 0;
}

// FUNCTION: EVNOVA 0x00464f60
// MAC: _ShipDecloaksWhenHit i386 0x00003f68 ppc 0x00005180
Boolean ShipDecloaksWhenHit(ShipObj *ship)
{
    short i;
    short j;

    if (ship->index == 0) {
        for (i = 0; i < kMaxOutfits; i++) {
            if (shipSystems[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (outfitSystem[i].modType[j] == kOutfModType17) {
                        if ((outfitSystem[i].modVal[j] & kOutfModVal30Bit3) != 0)
                            return 1;
                        return 0;
                    }
                }
            }
        }
    } else {
        for (i = 0; i < kDefaultOutfitSlots; i++) {
            if (shipClass[ship->type].defOutfCount[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (outfitSystem[shipClass[ship->type].defOutfID[i]].modType[j] ==
                        kOutfModType17) {
                        if ((outfitSystem[shipClass[ship->type].defOutfID[i]].modVal[j] &
                             kOutfModVal30Bit3) != 0)
                            return 1;
                        return 0;
                    }
                }
            }
        }
    }
    return 0;
}

// FUNCTION: EVNOVA 0x00465090
// MAC: _CloakShieldUsage i386 0x00004033 ppc 0x000052c0
short CloakShieldUsage(ShipObj *ship)
{
    short i;
    short j;

    if (ship->index == 0) {
        for (i = 0; i < kMaxOutfits; i++) {
            if (shipSystems[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (outfitSystem[i].modType[j] == kOutfModType17)
                        return (outfitSystem[i].modVal[j] >> 8) & 0xf;
                }
            }
        }
    } else {
        for (i = 0; i < kDefaultOutfitSlots; i++) {
            if (shipClass[ship->type].defOutfCount[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (outfitSystem[shipClass[ship->type].defOutfID[i]].modType[j] ==
                        kOutfModType17)
                        return (outfitSystem[shipClass[ship->type].defOutfID[i]].modVal[j] >>
                                8) & 0xf;
                }
            }
        }
    }
    return 0;
}

/*
 * A non-matching bit does NOT stop the scan - the walk keeps going, so the
 * test is one `&&` condition rather than a nested pair with their own
 * epilogues.
 */
#define kOutfModVal17Bit1 2

// FUNCTION: EVNOVA 0x004651a0
// MAC: _ShipCloakVisibleOnRadar i386 0x000040f8 ppc 0x000053f8
Boolean ShipCloakVisibleOnRadar(ShipObj *ship)
{
    short i;
    short j;

    if (ship->index == 0) {
        for (i = 0; i < kMaxOutfits; i++) {
            if (shipSystems[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (outfitSystem[i].modType[j] == kOutfModType17 &&
                        (outfitSystem[i].modVal[j] & kOutfModVal17Bit1) != 0)
                        return 1;
                }
            }
        }
    } else {
        for (i = 0; i < kDefaultOutfitSlots; i++) {
            if (shipClass[ship->type].defOutfCount[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (outfitSystem[shipClass[ship->type].defOutfID[i]].modType[j] ==
                            kOutfModType17 &&
                        (outfitSystem[shipClass[ship->type].defOutfID[i]].modVal[j] &
                            kOutfModVal17Bit1) != 0)
                        return 1;
                }
            }
        }
    }
    return 0;
}

#define kPersTypeAlways 0x3ff
#define kOutfModVal30Bit0 1
#define kOutfModVal30Bit1 2

// FUNCTION: EVNOVA 0x004652a0
// MAC: _ShipCanSeeCloakedShips i386 0x000041a9 ppc 0x000054f4
Boolean ShipCanSeeCloakedShips(ShipObj *ship, short which)
{
    short i;
    short j;

    if (ship->persType == kPersTypeAlways)
        return 1;
    if (ship->index == 0) {
        for (i = 0; i < kMaxOutfits; i++) {
            if (shipSystems[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (outfitSystem[i].modType[j] == kOutfModType30) {
                        if ((outfitSystem[i].modVal[j] & kOutfModVal30Bit1) != 0 &&
                            which == 0)
                            return 1;
                        if ((outfitSystem[i].modVal[j] & kOutfModVal30Bit0) != 0 &&
                            which == 1)
                            return 1;
                    }
                }
            }
        }
    } else {
        for (i = 0; i < kDefaultOutfitSlots; i++) {
            if (shipClass[ship->type].defOutfCount[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (outfitSystem[shipClass[ship->type].defOutfID[i]].modType[j] ==
                        kOutfModType30) {
                        if (which == 0 &&
                            (outfitSystem[shipClass[ship->type].defOutfID[i]].modVal[j] &
                                kOutfModVal30Bit1) != 0)
                            return 1;
                        if (which == 1 &&
                            (outfitSystem[shipClass[ship->type].defOutfID[i]].modVal[j] &
                                kOutfModVal30Bit0) != 0)
                            return 1;
                    }
                }
            }
        }
    }
    return 0;
}

// FUNCTION: EVNOVA 0x00465410
// MAC: _HasDensityScanner i386 0x000042c0 ppc 0x00005634
Boolean HasDensityScanner(ShipObj *ship)
{
    short i;
    short j;

    if (ship->index == 0) {
        if (playerHasDensityScanner >= 0) {
            if (playerHasDensityScanner != 0)
                return 1;
        } else {
            playerHasDensityScanner = 0;
            for (i = 0; i < kMaxOutfits; i++) {
                if (shipSystems[i] > 0) {
                    for (j = 0; j < kOutfMods; j++) {
                        if (outfitSystem[i].modType[j] == kOutfModType13) {
                            playerHasDensityScanner = 1;
                            return 1;
                        }
                    }
                }
            }
        }
    }
    return 0;
}

/* the literal twin of the above, one modifier type along and its own cache */
// FUNCTION: EVNOVA 0x004654b0
// MAC: _HasIFF i386 0x00004348 ppc 0x000056d4
Boolean HasIFF(ShipObj *ship)
{
    short i;
    short j;

    if (ship->index == 0) {
        if (playerHasIFF >= 0) {
            if (playerHasIFF != 0)
                return 1;
        } else {
            playerHasIFF = 0;
            for (i = 0; i < kMaxOutfits; i++) {
                if (shipSystems[i] > 0) {
                    for (j = 0; j < kOutfMods; j++) {
                        if (outfitSystem[i].modType[j] == kOutfModType14) {
                            playerHasIFF = 1;
                            return 1;
                        }
                    }
                }
            }
        }
    }
    return 0;
}

/* ShipCanCloak (no mac symbol). */
// FUNCTION: EVNOVA 0x00467e80
// MAC: _ShipCanCloak i386 0x0000553c ppc 0x00006b28
Boolean ShipCanCloak(ShipObj *ship)
{
    short idx;
    short i;
    short j;
    Boolean found;
    Boolean nibbleLo;
    Boolean nibbleHi;

    if (IsDisabled(ship))
        return 0;

    found = 0;
    idx = ship->index;
    if (idx == 0) {
        for (i = 0; i < kMaxOutfits; i++) {
            if (shipSystems[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (outfitSystem[i].modType[j] == kOutfModType17) {
                        found = 1;
                        /* BUG: both branches read modType[j] for modVal[j], so the nibbles are always 1 and 0 */
                        nibbleLo = (outfitSystem[i].modType[j] >> 4) & 0xf;
                        nibbleHi = (outfitSystem[i].modType[j] >> 8) & 0xf;
                        break;
                    }
                }
            }
        }
    } else {
        for (i = 0; i < kDefaultOutfitSlots; i++) {
            if (shipClass[ship->type].defOutfCount[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (outfitSystem[shipClass[ship->type].defOutfID[i]].modType[j] ==
                            kOutfModType17) {
                        found = 1;
                        nibbleLo = (outfitSystem[shipClass[ship->type].defOutfID[i]].modType[j] >> 4) & 0xf;
                        nibbleHi = (outfitSystem[shipClass[ship->type].defOutfID[i]].modType[j] >> 8) & 0xf;
                        break;
                    }
                }
            }
        }
    }

    if (found) {
        if (nibbleLo != 0) {
            if (ship->fuel <= 0.0f)
                return 0;
        }
        if (nibbleHi != 0) {
            if (ship->shield <= 0.0f)
                return 0;
        }

        if (idx == 0 && ship->jumpTimer > 0.0f
            && (shipClass[ship->type].flags2 & kShipCloaksToJump) == 0)
            return 0;
        return 1;
    }
    return 0;
}

// FUNCTION: EVNOVA 0x004680d0
// MAC: _DoShipCloak i386 0x0000de73 ppc 0x00011490
void DoShipCloak(ShipObj *ship)
{
    if (!ShipIsCloaked(ship)) {
        if (ship->index == 0 && ship->cloakDir <= 0)
            PlaySound(cloakSnd[1], 8, gSoundVolume, gSoundVolume);
        if (ship->cloak <= 0.0f) {
            ship->cloakDir = 1;
        } else {
            if (ship->cloakDir < 0)
                ship->cloakDir = 0;
        }
        if (ship->shield > 0.0f && CloakZeroesShields(ship)) {
            ship->shield = 0;
            if (ship->index == 0)
                statusRedraw[kRedrawShield] = 1;
        }
    }
}

// FUNCTION: EVNOVA 0x00468190
// MAC: _DoShipUncloak i386 0x0000df55 ppc 0x000115f0
void DoShipUncloak(ShipObj *ship)
{
    if (ShipIsCloaked(ship)) {
        if (ship->index == 0 && ship->cloakDir >= 0)
            PlaySound(cloakSnd[0], 8, gSoundVolume, gSoundVolume);
        if (ship->cloak > 0.0f) {
            ship->cloakDir = -1;
        } else {
            if (ship->cloakDir > 0)
                ship->cloakDir = 0;
        }
    }
}

// FUNCTION: EVNOVA 0x0046c7a0
// MAC: _ShipIsCloaked i386 0x0000255f ppc 0x00002f20
Boolean ShipIsCloaked(ShipObj *ship)
{
    float cloak;

    cloak = ship->cloak;
    if (cloak > 24.0f && ship->cloakDir >= 0)
        return 1;
    if (cloak > 8.0f && ship->cloakDir < 0)
        return 1;
    if (cloak > 16.0f)
        return 1;
    return 0;
}

// FUNCTION: EVNOVA 0x0046c930
// MAC: _ShipCanTargetUntargetableShips i386 0x0000238f ppc 0x00002c90
Boolean ShipCanTargetUntargetableShips(ShipObj *ship)
{
    short i;
    short j;

    if (ship->index == 0) {
        if (playerCanTargetUntargetableShips >= 0) {
            if (playerCanTargetUntargetableShips != 0)
                return 1;
        } else {
            playerCanTargetUntargetableShips = 0;
            for (i = 0; i < kMaxOutfits; i++) {
                if (shipSystems[i] > 0) {
                    for (j = 0; j < kOutfMods; j++) {
                        if (outfitSystem[i].modType[j] == kOutfModType30 &&
                            (outfitSystem[i].modVal[j] & kOutfModVal30Bit2) != 0) {
                            playerCanTargetUntargetableShips = 1;
                            return 1;
                        }
                    }
                }
            }
        }
    } else {
        for (i = 0; i < kDefaultOutfitSlots; i++) {
            if (shipClass[ship->type].defOutfCount[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (outfitSystem[shipClass[ship->type].defOutfID[i]].modType[j] ==
                            kOutfModType30 &&
                        (outfitSystem[shipClass[ship->type].defOutfID[i]].modVal[j] &
                            kOutfModVal30Bit2) != 0)
                        return 1;
                }
            }
        }
    }
    return 0;
}

/* the twin of the above: bit 3 of the value rather than bit 2, its own cache */

// FUNCTION: EVNOVA 0x0046ca60
// MAC: _ShipCanTargetCloakedShips i386 0x00002466 ppc 0x00002dc0
Boolean ShipCanTargetCloakedShips(ShipObj *ship)
{
    short i;
    short j;

    if (ship->index == 0) {
        if (playerCanTargetCloakedShips >= 0) {
            if (playerCanTargetCloakedShips != 0)
                return 1;
        } else {
            playerCanTargetCloakedShips = 0;
            for (i = 0; i < kMaxOutfits; i++) {
                if (shipSystems[i] > 0) {
                    for (j = 0; j < kOutfMods; j++) {
                        if (outfitSystem[i].modType[j] == kOutfModType30 &&
                            (outfitSystem[i].modVal[j] & kOutfModVal30Bit3) != 0) {
                            playerCanTargetCloakedShips = 1;
                            return 1;
                        }
                    }
                }
            }
        }
    } else {
        for (i = 0; i < kDefaultOutfitSlots; i++) {
            if (shipClass[ship->type].defOutfCount[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (outfitSystem[shipClass[ship->type].defOutfID[i]].modType[j] ==
                            kOutfModType30 &&
                        (outfitSystem[shipClass[ship->type].defOutfID[i]].modVal[j] &
                            kOutfModVal30Bit3) != 0)
                        return 1;
                }
            }
        }
    }
    return 0;
}

EXTERN_C_END
