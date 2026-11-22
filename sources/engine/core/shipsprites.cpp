/* shipsprites.cpp -- loading the ship sprites */

#include "game.h"
#include "render.h"
#include "crashlog.h"
#include "gameutil.h"
#include "player.h"
#include "sound.h"

/* the shan resource type name, as printed in log lines */
#define kShanTypeName "sh\x8an"

// FUNCTION: EVNOVA 0x004aeda0
// MAC: _LoadShipSprites i386 0x00023e3e ppc 0x000281a4
void LoadShipSprites(void)
{
    short k;
    short shipType;
    short count;
    short spriteIndex;

    for (shipType = 0; shipType < kMaxShipTypes; shipType++) {
        for (k = 0; k < 6; k++)
            shipSpriteLoaded[shipType][k] = -1;
        masterShipSprite[shipType] = 0;
        masterShipGlowSprite[shipType] = 0;
        masterShipLightSprite[shipType] = 0;
        masterShipWeaponSprite[shipType] = 0;
        masterShipAltSprite[shipType] = 0;
        masterShipShieldSprite[shipType] = 0;
    }

    for (shipType = 0; shipType < kMaxShipTypes; shipType++) {
        ServiceIntroMusic(0);
        count = LoadExtendedShipSprites(shipType);
        if (count > 0) {
            DeltaIntroProgressBar(1.0);

            if (targetInfoPic[shipType]) {
                shipClass[shipType].targetPictID = shipType;
            } else {
                shipClass[shipType].targetPictID = spriteIndex = shipClass[shipType].spriteIndex;
                if (shipType != kMaxShipTypes - 1) {
                    if (crashlog_isactive()) {
                        if (targetInfoPic[shipClass[shipType].targetPictID] == 0) {
                            crashlog_print("warning: ");
                            crashlog_print_pstr(shipClass[shipType].name);
                            crashlog_print(" (");
                            crashlog_print_num(shipType + kFirstResourceID);
                            crashlog_print(") lacks target picture\n");
                        }
                    }
                }
            }

            if (ResourceExists(kPictType, shipType + 5000)) {
                shipClass[shipType].shipyardPictID = shipType + 5000;
            } else {
                shipClass[shipType].shipyardPictID = shipClass[shipType].spriteIndex + 5000;
                if (crashlog_isactive()) {
                    if (shipClass[shipType].buyRandom != 0 || shipClass[shipType].hireRandom != 0) {
                        if (!ResourceExists(kPictType, shipClass[shipType].spriteIndex + 5000)) {
                            crashlog_print("warning: ");
                            crashlog_print_pstr(shipClass[shipType].name);
                            crashlog_print(" (");
                            crashlog_print_num(shipType + kFirstResourceID);
                            crashlog_print(") lacks shipyard picture\n");
                        }
                    }
                }
            }
        }

        IterateInGameEventLoop(1);
    }
}


ShanRec shipSpriteInfo;
ShanRec **shipSpriteInfoHand;
short firstShipGlowIndex;
short firstShipLightIndex;
short firstShipAltIndex;
short firstShipWeapIndex;
short firstShipShieldIndex;
short shipSpriteLoaded[kMaxShipTypes][6];


/* loads one sprite layer of a ship: a clone of an earlier ship's layer with the same image, else the RLE
   resource, else the PICT converted to RLE */
static void LoadShipLayerSprite(SpritePtr *master, short shipType, short slot, short spinID, short maskID,
                                short xSize, short ySize, int rleFrames, int pictFrames, const char *rleText,
                                const char *pictText, const char *convertText)
{
    short i;
    short err;

    i = FindPriorLoadedShipSprite(spinID, shipType, slot);
    if (i != -1) {
        SWCloneSprite(master[i], &master[shipType], NULL);
        SWLockSprite(master[shipType]);
    } else if (ResourceExists(kRLEDResType, spinID)) {
        err = SWCreateSpriteFromRLEResource(&master[shipType], NULL, spinID, rleFrames, 0);
        if ((int32_t)err != 0)
            SpriteLoadFailed(rleText, shipType + kFirstResourceID, 2, shipType + kFirstResourceID, err);
        SWLockSprite(master[shipType]);
    } else {
        err = SWCreateSpriteFromSinglePictXY(gSpriteWorld, &master[shipType], NULL, spinID, maskID, xSize, ySize,
                                             0, 0, pictFrames, 1);
        if ((int32_t)err != 0)
            SpriteLoadFailed(pictText, shipType + kFirstResourceID, 3, shipType + kFirstResourceID, err);
        master[shipType]->numFrames = pictFrames;
        SWLockSprite(master[shipType]);
        err = ConvertSpriteToRLE(master[shipType]);
        if ((int32_t)err != 0)
            SpriteLoadFailed(convertText, shipType + kFirstResourceID, 4, shipType + kFirstResourceID, err);
    }
}

// FUNCTION: EVNOVA 0x004b4ee0
// MAC: _LoadExtendedShipSprites i386 0x000226af ppc 0x00026b28
short LoadExtendedShipSprites(short shipType)
{
    short total;
    short spinID;
    short lightID;
    short maskID;
    short xSize;
    short ySize;
    short setCount;
    short i;
    short err;
    short count;
    short j;

    total = 0;
    shipSpriteInfoHand = (ShanRec **)GetResource(kShanType, shipType + kFirstResourceID);
    if (shipSpriteInfoHand != NULL) {
        LockAHandle((Handle)shipSpriteInfoHand);

        spinID = (*shipSpriteInfoHand)->baseImageID;
        maskID = (*shipSpriteInfoHand)->baseMaskID;
        xSize = (*shipSpriteInfoHand)->baseXSize;
        ySize = (*shipSpriteInfoHand)->baseYSize;
        shipClass[shipType].framesPer = (*shipSpriteInfoHand)->framesPer;
        if (shipClass[shipType].framesPer == 0)
            shipClass[shipType].framesPer = 36;
        setCount = (*shipSpriteInfoHand)->baseSetCount;
        if (setCount <= 0)
            setCount = 1;
        if (settings.shipAnimations != 0) {
            count = setCount;
            count *= shipClass[shipType].framesPer;
        } else {
            count = shipClass[shipType].framesPer;
        }
        shipClass[shipType].weapDecay = (*shipSpriteInfoHand)->weapDecay * 0.333;

        i = FindPriorLoadedShipSprite(spinID, shipType, 0);
        if (i != -1) {
            SWCloneSprite(masterShipSprite[i], &masterShipSprite[shipType], NULL);
            SWLockSprite(masterShipSprite[shipType]);
            shipSpriteLoaded[shipType][0] = spinID;
            shipClass[shipType].spriteIndex = i;
        } else {
            if (ResourceExists(kRLEDResType, spinID)) {
                err = SWCreateSpriteFromRLEResource(&masterShipSprite[shipType], NULL,
                                                    spinID, count, 0);
                if ((int32_t)err != 0)
                    SpriteLoadFailed("error loading ship base sprite from RLE, " kShanTypeName " ID ",
                                     shipType + kFirstResourceID, 2, shipType + kFirstResourceID, err);
                SWLockSprite(masterShipSprite[shipType]);
            } else {
                err = SWCreateSpriteFromSinglePictXY(gSpriteWorld, &masterShipSprite[shipType],
                                                     NULL, spinID, maskID, xSize, ySize, 0, 0,
                                                     shipClass[shipType].framesPer * setCount, 1);
                if ((int32_t)err != 0)
                    SpriteLoadFailed("error loading ship base sprite from PICT, " kShanTypeName " ID ",
                                     shipType + kFirstResourceID, 3, shipType + kFirstResourceID, err);
                SWLockSprite(masterShipSprite[shipType]);
                err = ConvertSpriteToRLE(masterShipSprite[shipType]);
                if ((int32_t)err != 0)
                    SpriteLoadFailed("error converting ship base sprite to RLE, " kShanTypeName " ID ",
                                     shipType + kFirstResourceID, 4, shipType + kFirstResourceID, err);
                masterShipSprite[shipType]->numFrames =
                    shipClass[shipType].framesPer * setCount;
            }
        }
        shipSpriteLoaded[shipType][0] = spinID;
        total += masterShipSprite[shipType]->numFrames;

        shipClass[shipType].shanFlags = (*shipSpriteInfoHand)->flags;
        shipClass[shipType].animDelay = (*shipSpriteInfoHand)->animDelay;
        shipClass[shipType].transparency = (int)(*shipSpriteInfoHand)->baseTransparency;
        if (settings.shipAnimations != 0)
            shipClass[shipType].baseSets = setCount;
        else
            shipClass[shipType].baseSets = 1;

        if ((*shipSpriteInfoHand)->altImageID > 0 && (*shipSpriteInfoHand)->altSetCount > 0) {
            if (settings.shipAnimations != 0)
                shipClass[shipType].altSets = (*shipSpriteInfoHand)->altSetCount;
            else
                shipClass[shipType].altSets = 1;
            spinID = (*shipSpriteInfoHand)->altImageID;
            maskID = (*shipSpriteInfoHand)->altMaskID;
            xSize = (*shipSpriteInfoHand)->altXSize;
            ySize = (*shipSpriteInfoHand)->altYSize;

            LoadShipLayerSprite(masterShipAltSprite, shipType, 1, spinID, maskID, xSize, ySize,
                                shipClass[shipType].altSets * shipClass[shipType].framesPer,
                                shipClass[shipType].framesPer * shipClass[shipType].altSets,
                                "error loading ship alt sprite from RLE, " kShanTypeName " ID ",
                                "error loading ship alt sprite from PICT, " kShanTypeName " ID ",
                                "error converting ship alt sprite to RLE, " kShanTypeName " ID ");
            shipSpriteLoaded[shipType][1] = spinID;
            total += masterShipAltSprite[shipType]->numFrames;
        }

        if ((*shipSpriteInfoHand)->glowImageID > 0 && settings.engineGlows != 0) {
            spinID = (*shipSpriteInfoHand)->glowImageID;
            maskID = (*shipSpriteInfoHand)->glowMaskID;
            xSize = (*shipSpriteInfoHand)->glowXSize;
            ySize = (*shipSpriteInfoHand)->glowYSize;

            LoadShipLayerSprite(masterShipGlowSprite, shipType, 2, spinID, maskID, xSize, ySize, count,
                                shipClass[shipType].framesPer * setCount,
                                "error loading ship glow sprite from RLE, " kShanTypeName " ID ",
                                "error loading ship glow sprite from PICT, " kShanTypeName " ID ",
                                "error converting ship glow sprite to RLE, " kShanTypeName " ID ");
            shipSpriteLoaded[shipType][2] = spinID;
            total += masterShipGlowSprite[shipType]->numFrames;
        }

        if ((*shipSpriteInfoHand)->lightImageID > 0 && settings.runningLights != 0) {
            lightID = (*shipSpriteInfoHand)->lightImageID;
            maskID = (*shipSpriteInfoHand)->lightMaskID;
            xSize = (*shipSpriteInfoHand)->lightXSize;
            ySize = (*shipSpriteInfoHand)->lightYSize;

            LoadShipLayerSprite(masterShipLightSprite, shipType, 3, lightID, maskID, xSize, ySize, count,
                                shipClass[shipType].framesPer * setCount,
                                "error loading ship light sprite from RLE, " kShanTypeName " ID ",
                                "error loading ship light sprite from PICT, " kShanTypeName " ID ",
                                "error converting ship light sprite to RLE, " kShanTypeName " ID ");
            shipClass[shipType].blinkMode = (*shipSpriteInfoHand)->blinkMode;
            for (j = 0; j < 4; j++)
                shipClass[shipType].blinkValue[j] = (*shipSpriteInfoHand)->blinkValue[j];
            if (shipClass[shipType].blinkMode == 2 &&
                shipClass[shipType].blinkValue[2] > 31)
                shipClass[shipType].blinkValue[2] = 31;
            if (shipClass[shipType].blinkMode == 3 &&
                shipClass[shipType].blinkValue[1] > 31)
                shipClass[shipType].blinkValue[1] = 31;
            shipSpriteLoaded[shipType][3] = lightID;
            total += masterShipLightSprite[shipType]->numFrames;
        }

        if ((*shipSpriteInfoHand)->weapImageID > 0 && settings.weaponGlows != 0) {
            spinID = (*shipSpriteInfoHand)->weapImageID;
            maskID = (*shipSpriteInfoHand)->weapMaskID;
            xSize = (*shipSpriteInfoHand)->weapXSize;
            ySize = (*shipSpriteInfoHand)->weapYSize;

            LoadShipLayerSprite(masterShipWeaponSprite, shipType, 4, spinID, maskID, xSize, ySize, count,
                                shipClass[shipType].framesPer * setCount,
                                "error loading ship weapon sprite from RLE, " kShanTypeName " ID ",
                                "error loading ship weapon sprite from PICT, " kShanTypeName " ID ",
                                "error converting ship weapon sprite to RLE, " kShanTypeName " ID ");
            shipSpriteLoaded[shipType][4] = spinID;
            total += masterShipWeaponSprite[shipType]->numFrames;
        }

        if ((*shipSpriteInfoHand)->shieldImageID > 0) {
            spinID = (*shipSpriteInfoHand)->shieldImageID;
            maskID = (*shipSpriteInfoHand)->shieldMaskID;
            xSize = (*shipSpriteInfoHand)->shieldXSize;
            ySize = (*shipSpriteInfoHand)->shieldYSize;

            i = FindPriorLoadedShipSprite(spinID, shipType, 5);
            if (i != -1) {
                SWCloneSprite(masterShipShieldSprite[i],
                              &masterShipShieldSprite[shipType], NULL);
                SWLockSprite(masterShipShieldSprite[shipType]);
            } else {
                if (ResourceExists(kRLEDResType, spinID)) {
                    err = SWCreateSpriteFromRLEResource(&masterShipShieldSprite[shipType], NULL,
                                                        spinID, count, 0);
                    if ((int32_t)err != 0)
                        SpriteLoadFailed("error loading ship shield sprite from RLE, " kShanTypeName " ID ",
                                         shipType + kFirstResourceID, 2, shipType + kFirstResourceID, err);
                    SWLockSprite(masterShipShieldSprite[shipType]);
                } else {
                    err = SWCreateSpriteFromSinglePictXY(gSpriteWorld,
                                                         &masterShipShieldSprite[shipType], NULL,
                                                         spinID, maskID, xSize, ySize, 0, 0, 0, 1);
                    if ((int32_t)err != 0)
                        SpriteLoadFailed("error loading ship shield sprite from PICT, " kShanTypeName " ID ",
                                         shipType + kFirstResourceID, 3, shipType + kFirstResourceID, err);
                    SWLockSprite(masterShipShieldSprite[shipType]);
                    err = ConvertSpriteToRLE(masterShipShieldSprite[shipType]);
                    if ((int32_t)err != 0)
                        SpriteLoadFailed("error converting ship shield sprite to RLE, " kShanTypeName " ID ",
                                         shipType + kFirstResourceID, 4, shipType + kFirstResourceID, err);
                }
            }
            shipSpriteLoaded[shipType][5] = spinID;
            total += masterShipShieldSprite[shipType]->numFrames;
        }

        shipClass[shipType].weapPos[0][0].h = (*shipSpriteInfoHand)->mountPos[0].x[0];
        shipClass[shipType].weapPos[0][0].v = (*shipSpriteInfoHand)->mountPos[0].y[0];
        shipClass[shipType].weapPos[0][1].h = (*shipSpriteInfoHand)->mountPos[0].x[1];
        shipClass[shipType].weapPos[0][1].v = (*shipSpriteInfoHand)->mountPos[0].y[1];
        shipClass[shipType].weapPos[0][2].h = (*shipSpriteInfoHand)->mountPos[0].x[2];
        shipClass[shipType].weapPos[0][2].v = (*shipSpriteInfoHand)->mountPos[0].y[2];
        shipClass[shipType].weapPos[0][3].h = (*shipSpriteInfoHand)->mountPos[0].x[3];
        shipClass[shipType].weapPos[0][3].v = (*shipSpriteInfoHand)->mountPos[0].y[3];
        shipClass[shipType].weapPos[1][0].h = (*shipSpriteInfoHand)->mountPos[1].x[0];
        shipClass[shipType].weapPos[1][0].v = (*shipSpriteInfoHand)->mountPos[1].y[0];
        shipClass[shipType].weapPos[1][1].h = (*shipSpriteInfoHand)->mountPos[1].x[1];
        shipClass[shipType].weapPos[1][1].v = (*shipSpriteInfoHand)->mountPos[1].y[1];
        shipClass[shipType].weapPos[1][2].h = (*shipSpriteInfoHand)->mountPos[1].x[2];
        shipClass[shipType].weapPos[1][2].v = (*shipSpriteInfoHand)->mountPos[1].y[2];
        shipClass[shipType].weapPos[1][3].h = (*shipSpriteInfoHand)->mountPos[1].x[3];
        shipClass[shipType].weapPos[1][3].v = (*shipSpriteInfoHand)->mountPos[1].y[3];
        shipClass[shipType].weapPos[2][0].h = (*shipSpriteInfoHand)->mountPos[2].x[0];
        shipClass[shipType].weapPos[2][0].v = (*shipSpriteInfoHand)->mountPos[2].y[0];
        shipClass[shipType].weapPos[2][1].h = (*shipSpriteInfoHand)->mountPos[2].x[1];
        shipClass[shipType].weapPos[2][1].v = (*shipSpriteInfoHand)->mountPos[2].y[1];
        shipClass[shipType].weapPos[2][2].h = (*shipSpriteInfoHand)->mountPos[2].x[2];
        shipClass[shipType].weapPos[2][2].v = (*shipSpriteInfoHand)->mountPos[2].y[2];
        shipClass[shipType].weapPos[2][3].h = (*shipSpriteInfoHand)->mountPos[2].x[3];
        shipClass[shipType].weapPos[2][3].v = (*shipSpriteInfoHand)->mountPos[2].y[3];
        shipClass[shipType].weapPos[3][0].h = (*shipSpriteInfoHand)->mountPos[3].x[0];
        shipClass[shipType].weapPos[3][0].v = (*shipSpriteInfoHand)->mountPos[3].y[0];
        shipClass[shipType].weapPos[3][1].h = (*shipSpriteInfoHand)->mountPos[3].x[1];
        shipClass[shipType].weapPos[3][1].v = (*shipSpriteInfoHand)->mountPos[3].y[1];
        shipClass[shipType].weapPos[3][2].h = (*shipSpriteInfoHand)->mountPos[3].x[2];
        shipClass[shipType].weapPos[3][2].v = (*shipSpriteInfoHand)->mountPos[3].y[2];
        shipClass[shipType].weapPos[3][3].h = (*shipSpriteInfoHand)->mountPos[3].x[3];
        shipClass[shipType].weapPos[3][3].v = (*shipSpriteInfoHand)->mountPos[3].y[3];

        shipClass[shipType].weapPosZ[0][0] = (*shipSpriteInfoHand)->mountPosZ[0][0];
        shipClass[shipType].weapPosZ[0][1] = (*shipSpriteInfoHand)->mountPosZ[0][1];
        shipClass[shipType].weapPosZ[0][2] = (*shipSpriteInfoHand)->mountPosZ[0][2];
        shipClass[shipType].weapPosZ[0][3] = (*shipSpriteInfoHand)->mountPosZ[0][3];
        shipClass[shipType].weapPosZ[1][0] = (*shipSpriteInfoHand)->mountPosZ[1][0];
        shipClass[shipType].weapPosZ[1][1] = (*shipSpriteInfoHand)->mountPosZ[1][1];
        shipClass[shipType].weapPosZ[1][2] = (*shipSpriteInfoHand)->mountPosZ[1][2];
        shipClass[shipType].weapPosZ[1][3] = (*shipSpriteInfoHand)->mountPosZ[1][3];
        shipClass[shipType].weapPosZ[2][0] = (*shipSpriteInfoHand)->mountPosZ[2][0];
        shipClass[shipType].weapPosZ[2][1] = (*shipSpriteInfoHand)->mountPosZ[2][1];
        shipClass[shipType].weapPosZ[2][2] = (*shipSpriteInfoHand)->mountPosZ[2][2];
        shipClass[shipType].weapPosZ[2][3] = (*shipSpriteInfoHand)->mountPosZ[2][3];
        shipClass[shipType].weapPosZ[3][0] = (*shipSpriteInfoHand)->mountPosZ[3][0];
        shipClass[shipType].weapPosZ[3][1] = (*shipSpriteInfoHand)->mountPosZ[3][1];
        shipClass[shipType].weapPosZ[3][2] = (*shipSpriteInfoHand)->mountPosZ[3][2];
        shipClass[shipType].weapPosZ[3][3] = (*shipSpriteInfoHand)->mountPosZ[3][3];

        shipClass[shipType].compress[0].x = (*shipSpriteInfoHand)->upCompressX * 0.01;
        shipClass[shipType].compress[0].y = (*shipSpriteInfoHand)->upCompressY * 0.01;
        shipClass[shipType].compress[1].x = (*shipSpriteInfoHand)->downCompressX * 0.01;
        shipClass[shipType].compress[1].y = (*shipSpriteInfoHand)->downCompressY * 0.01;

        for (i = 0; i <= 1; i++) {
            if (shipClass[shipType].compress[i].x <= 0.0f)
                shipClass[shipType].compress[i].x = 1.0f;
            if (shipClass[shipType].compress[i].y <= 0.0f)
                shipClass[shipType].compress[i].y = 1.0f;
        }

        AdjustHandleSize((Handle)shipSpriteInfoHand, 0xc0);
        ReleaseResource((Handle)shipSpriteInfoHand);
    }
    return total;
}

// FUNCTION: EVNOVA 0x004b67c0
// MAC: _FindPriorLoadedShipSprite i386 0x0001e5e4 ppc 0x00022f34
short FindPriorLoadedShipSprite(short value, short count, short field)
{
    short i;

    for (i = 0; i < count; i++) {
        if (value == shipSpriteLoaded[i][field])
            return i;
    }
    return -1;
}
