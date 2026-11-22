/* shipcloak.cpp -- cloaking, detection and targetability */

#include "burgermath.h"
#include "ai.h"
#include "game.h"
#include "outfitmods.h"
#include "player.h"
#include "render.h"
#include "ships.h"
#include "sound.h"

EXTERN_C_BEGIN

#define kDetectRange    200.0f

// FUNCTION: EVNOVA 0x00464a90
// MAC: _ShipVisibleToShip i386 0x0000971c ppc 0x0000ba14
Boolean ShipObj::VisibleTo(ShipObj *viewer)
{
    if (AIShipIsEmergingFromHypergate(this))
        return 0;
    if (IsCloaked()) {
        float d;

        if (viewer->parentShip == index) {
            if (viewer->HasCloak())
                return 1;
        }
        if (viewer->seesCloaked[0] == 1) {
            d = FloatAbs(viewer->pos.x - pos.x);
            if (d <= kDetectRange) {
                d = FloatAbs(viewer->pos.y - pos.y);
                if (d <= kDetectRange)
                    return 1;
            }
        }
        return 0;
    }
    return 1;
}

/* Cloak value bits */
#define kCloakRadarVisible 2
#define kCloakZeroesShields 4
#define kCloakDecloakOnHit 8
#define kCloakArea 0x1000
/* CloakScanner value bits */
#define kScanSeeOnRadar 1
#define kScanSeeOnScreen 2
#define kScanTargetUntargetable 4
#define kScanTargetCloaked 8

// FUNCTION: EVNOVA 0x00464b50
// MAC: _HasCloak i386 0x000095f2 ppc 0x0000b85c
Boolean ShipObj::HasCloak()
{
    if (IsPlayer()) {
        if (PlayerHasMod(OutfMod::Cloak))
            return 1;
    } else {
        if (parentShip != -1 && IsShipInCloseForm(this) &&
            shipData[parentShip].IsCloaked() &&
            shipData[parentShip].HasAreaCloak())
            return 1;
        if (DefaultHasMod(Class(), OutfMod::Cloak))
            return 1;
    }
    return 0;
}

/* the same question as HasCloak but only for area cloaks */
// FUNCTION: EVNOVA 0x00464c80
// MAC: _HasAreaCloak i386 0x00003d49 ppc 0x00004e84
Boolean ShipObj::HasAreaCloak()
{
    if (IsPlayer()) {
        if (playerStats.hasAreaCloak != -1) {
            if (playerStats.hasAreaCloak == 1)
                return 1;
        } else {
            playerStats.hasAreaCloak = 0;
            if (ForEachPlayerMod(OutfMod::Cloak,
                                 [](short val, short) { return (val & kCloakArea) != 0; })) {
                playerStats.hasAreaCloak = 1;
                return 1;
            }
        }
    } else {
        if (ForEachDefaultMod(Class(), OutfMod::Cloak,
                              [](short val, short) { return (val & kCloakArea) != 0; }))
            return 1;
    }
    return 0;
}

/* the first owned cloak outfit's fuel nibble (bits 4-7); 0 for anything but the player's ship */
// FUNCTION: EVNOVA 0x00464db0
// MAC: _CloakFuelUsage i386 0x00003e25 ppc 0x00004fb8
short ShipObj::CloakFuelUsage()
{
    short usage = 0;

    if (IsPlayer()) {
        ForEachPlayerMod(OutfMod::Cloak, [&](short val, short) {
            usage = (val >> 4) & 0xf;
            return true;
        });
    }
    return usage;
}

// FUNCTION: EVNOVA 0x00464e30
// MAC: _CloakZeroesShields i386 0x00003e9d ppc 0x00005040
Boolean ShipObj::CloakZeroesShields()
{
    Boolean result = 0;
    auto test = [&](short val, short) {
        result = (val & kCloakZeroesShields) != 0;
        return true;
    };

    if (IsPlayer())
        ForEachPlayerMod(OutfMod::Cloak, test);
    else
        ForEachDefaultMod(Class(), OutfMod::Cloak, test);
    return result;
}

// FUNCTION: EVNOVA 0x00464f60
// MAC: _ShipDecloaksWhenHit i386 0x00003f68 ppc 0x00005180
Boolean ShipObj::DecloaksWhenHit()
{
    Boolean result = 0;
    auto test = [&](short val, short) {
        result = (val & kCloakDecloakOnHit) != 0;
        return true;
    };

    if (IsPlayer())
        ForEachPlayerMod(OutfMod::Cloak, test);
    else
        ForEachDefaultMod(Class(), OutfMod::Cloak, test);
    return result;
}

// FUNCTION: EVNOVA 0x00465090
// MAC: _CloakShieldUsage i386 0x00004033 ppc 0x000052c0
short ShipObj::CloakShieldUsage()
{
    short usage = 0;
    auto take = [&](short val, short) {
        usage = (val >> 8) & 0xf;
        return true;
    };

    if (IsPlayer())
        ForEachPlayerMod(OutfMod::Cloak, take);
    else
        ForEachDefaultMod(Class(), OutfMod::Cloak, take);
    return usage;
}

// FUNCTION: EVNOVA 0x004651a0
// MAC: _ShipCloakVisibleOnRadar i386 0x000040f8 ppc 0x000053f8
Boolean ShipObj::CloakVisibleOnRadar()
{
    auto visible = [](short val, short) { return (val & kCloakRadarVisible) != 0; };

    if (IsPlayer())
        return ForEachPlayerMod(OutfMod::Cloak, visible);
    return ForEachDefaultMod(Class(), OutfMod::Cloak, visible);
}

#define kPersTypeAlways 0x3ff

// FUNCTION: EVNOVA 0x004652a0
// MAC: _ShipCanSeeCloakedShips i386 0x000041a9 ppc 0x000054f4
Boolean ShipObj::CanSeeCloakedShips(short which)
{
    if (persType == kPersTypeAlways)
        return 1;
    if (IsPlayer()) {
        if (ForEachPlayerMod(OutfMod::CloakScanner, [&](short val, short) {
                if ((val & kScanSeeOnScreen) != 0 &&
                    which == 0)
                    return true;
                if ((val & kScanSeeOnRadar) != 0 &&
                    which == 1)
                    return true;
                return false;
            }))
            return 1;
    } else {
        if (ForEachDefaultMod(Class(), OutfMod::CloakScanner, [&](short val, short) {
                if (which == 0 &&
                    (val & kScanSeeOnScreen) != 0)
                    return true;
                if (which == 1 &&
                    (val & kScanSeeOnRadar) != 0)
                    return true;
                return false;
            }))
            return 1;
    }
    return 0;
}

// FUNCTION: EVNOVA 0x00465410
// MAC: _HasDensityScanner i386 0x000042c0 ppc 0x00005634
Boolean ShipObj::HasDensityScanner()
{
    if (IsPlayer()) {
        if (playerStats.hasDensityScanner >= 0) {
            if (playerStats.hasDensityScanner != 0)
                return 1;
        } else {
            playerStats.hasDensityScanner = 0;
            if (PlayerHasMod(OutfMod::DensityScanner)) {
                playerStats.hasDensityScanner = 1;
                return 1;
            }
        }
    }
    return 0;
}

/* the literal twin of the above, one modifier type along and its own cache */
// FUNCTION: EVNOVA 0x004654b0
// MAC: _HasIFF i386 0x00004348 ppc 0x000056d4
Boolean ShipObj::HasIFF()
{
    if (IsPlayer()) {
        if (playerStats.hasIFF >= 0) {
            if (playerStats.hasIFF != 0)
                return 1;
        } else {
            playerStats.hasIFF = 0;
            if (PlayerHasMod(OutfMod::IFF)) {
                playerStats.hasIFF = 1;
                return 1;
            }
        }
    }
    return 0;
}

// FUNCTION: EVNOVA 0x00467e80
// MAC: _ShipCanCloak i386 0x0000553c ppc 0x00006b28
Boolean ShipObj::CanCloak()
{
    short idx;
    short i;
    short j;
    Boolean found;
    Boolean nibbleLo;
    Boolean nibbleHi;

    if (IsDisabled())
        return 0;

    found = 0;
    idx = index;
    if (idx == 0) {
        for (i = 0; i < kMaxOutfits; i++) {
            if (shipSystems[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (outfitSystem[i].modType[j] == OutfMod::Cloak) {
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
            if (Class().defOutfCount[i] > 0) {
                for (j = 0; j < kOutfMods; j++) {
                    if (outfitSystem[Class().defOutfID[i]].modType[j] ==
                            OutfMod::Cloak) {
                        found = 1;
                        nibbleLo = (outfitSystem[Class().defOutfID[i]].modType[j] >> 4) & 0xf;
                        nibbleHi = (outfitSystem[Class().defOutfID[i]].modType[j] >> 8) & 0xf;
                        break;
                    }
                }
            }
        }
    }

    if (found) {
        if (nibbleLo != 0) {
            if (fuel <= 0.0f)
                return 0;
        }
        if (nibbleHi != 0) {
            if (shield <= 0.0f)
                return 0;
        }

        if (idx == 0 && jumpTimer > 0.0f
            && (Class().flags2 & kShipCloaksToJump) == 0)
            return 0;
        return 1;
    }
    return 0;
}

// FUNCTION: EVNOVA 0x004680d0
// MAC: _DoShipCloak i386 0x0000de73 ppc 0x00011490
void ShipObj::DoCloak()
{
    if (!IsCloaked()) {
        if (IsPlayer() && cloakDir <= 0)
            PlaySound(cloakSnd[1], 8, gSoundVolume, gSoundVolume);
        if (cloak <= 0.0f) {
            cloakDir = 1;
        } else {
            if (cloakDir < 0)
                cloakDir = 0;
        }
        if (shield > 0.0f && CloakZeroesShields()) {
            shield = 0;
            if (IsPlayer())
                statusRedraw[kRedrawShield] = 1;
        }
    }
}

// FUNCTION: EVNOVA 0x00468190
// MAC: _DoShipUncloak i386 0x0000df55 ppc 0x000115f0
void ShipObj::DoUncloak()
{
    if (IsCloaked()) {
        if (IsPlayer() && cloakDir >= 0)
            PlaySound(cloakSnd[0], 8, gSoundVolume, gSoundVolume);
        if (cloak > 0.0f) {
            cloakDir = -1;
        } else {
            if (cloakDir > 0)
                cloakDir = 0;
        }
    }
}

// FUNCTION: EVNOVA 0x0046c7a0
// MAC: _ShipIsCloaked i386 0x0000255f ppc 0x00002f20
Boolean ShipObj::IsCloaked()
{
    float level;

    level = cloak;
    if (level > 24.0f && cloakDir >= 0)
        return 1;
    if (level > 8.0f && cloakDir < 0)
        return 1;
    if (level > 16.0f)
        return 1;
    return 0;
}

// FUNCTION: EVNOVA 0x0046c930
// MAC: _ShipCanTargetUntargetableShips i386 0x0000238f ppc 0x00002c90
Boolean ShipObj::CanTargetUntargetableShips()
{
    auto scanner = [](short val, short) { return (val & kScanTargetUntargetable) != 0; };

    if (IsPlayer()) {
        if (playerStats.canTargetUntargetableShips >= 0) {
            if (playerStats.canTargetUntargetableShips != 0)
                return 1;
        } else {
            playerStats.canTargetUntargetableShips = 0;
            if (ForEachPlayerMod(OutfMod::CloakScanner, scanner)) {
                playerStats.canTargetUntargetableShips = 1;
                return 1;
            }
        }
    } else {
        if (ForEachDefaultMod(Class(), OutfMod::CloakScanner, scanner))
            return 1;
    }
    return 0;
}

/* the twin of the above: bit 3 of the value rather than bit 2, its own cache */

// FUNCTION: EVNOVA 0x0046ca60
// MAC: _ShipCanTargetCloakedShips i386 0x00002466 ppc 0x00002dc0
Boolean ShipObj::CanTargetCloakedShips()
{
    auto scanner = [](short val, short) { return (val & kScanTargetCloaked) != 0; };

    if (IsPlayer()) {
        if (playerStats.canTargetCloakedShips >= 0) {
            if (playerStats.canTargetCloakedShips != 0)
                return 1;
        } else {
            playerStats.canTargetCloakedShips = 0;
            if (ForEachPlayerMod(OutfMod::CloakScanner, scanner)) {
                playerStats.canTargetCloakedShips = 1;
                return 1;
            }
        }
    } else {
        if (ForEachDefaultMod(Class(), OutfMod::CloakScanner, scanner))
            return 1;
    }
    return 0;
}

EXTERN_C_END
