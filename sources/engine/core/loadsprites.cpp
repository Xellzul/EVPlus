/* loadsprites.cpp -- loading the master sprites, and sprite helpers */

#include "game.h"
#include "render.h"
#include "crashlog.h"
#include "space.h"
#include "prototypes.h"
#include "gameutil.h"
#include "uiscale.h"
#include "sound.h"

/* the spin resource type name, as printed in log lines */
#define kSpinTypeName "sp\x95n"

/* loads a spin's sprite from its RLE resource, or else from its PICT converted to RLE */
static void LoadSpinSprite(SpritePtr *dst, short spinID, short spriteID, short maskID, short xSize, short ySize,
                           int frames, const char *rleText, const char *pictText)
{
    short err;

    if (ResourceExists(kRLEDResType, spriteID)) {
        err = SWCreateSpriteFromRLEResource(dst, NULL, spriteID, frames, 0);
        if ((int32_t)err != 0)
            SpriteLoadFailed(rleText, spinID, 2, spinID, err);
    } else {
        err = SWCreateSpriteFromSinglePictXY(gSpriteWorld, dst, NULL, spriteID, maskID, xSize, ySize, 0, 0, 0, 1);
        if ((int32_t)err != 0)
            SpriteLoadFailed(pictText, spinID, 3, spinID, err);
        SWLockSprite(*dst);
        err = ConvertSpriteToRLE(*dst);
        if ((int32_t)err != 0)
            SpriteLoadFailed("error converting sprite to RLE, " kSpinTypeName " ID ", spinID, 4, spinID, err);
        (*dst)->numFrames = frames;
    }
}

// FUNCTION: EVNOVA 0x004ad960
// MAC: _LoadSprites i386 0x00020541 ppc 0x00024ca0
void LoadSprites(void)
{
    short spriteID, maskID;
    short xSize, ySize;
    short xTiles, yTiles;
    short err;
    short i;
    int32_t resID;
    SpritePtr *q;

    for (i = 0; i < kMaxWeapTypes; i++) {
        if (GetSpriteInfo(i + 3000, &spriteID, &maskID, &xSize, &ySize, &xTiles, &yTiles)) {
            LoadSpinSprite(&masterShotSprite[i], i + 3000, spriteID, maskID, xSize, ySize, xTiles * yTiles,
                           "error loading sprite from RLE resource, " kSpinTypeName " ID ",
                           "error loading sprite from PICT resource, " kSpinTypeName " ID ");
            IterateInGameEventLoop(1);
        }
    }

    for (i = 0; i < kMaxExplodTypes; i++) {
        if (GetSpriteInfo(i + 400, &spriteID, &maskID, &xSize, &ySize, &xTiles, &yTiles)) {
            LoadSpinSprite(&masterExplodSprite[i], i + 400, spriteID, maskID, xSize, ySize, xTiles * yTiles,
                           "error loading sprite from RLE, " kSpinTypeName " ID ",
                           "error loading sprite from PICT, " kSpinTypeName " ID ");
            IterateInGameEventLoop(1);
        }
    }

    for (i = 0; i < 5; i++) {
        if (GetSpriteInfo(i + 500, &spriteID, &maskID, &xSize, &ySize, &xTiles, &yTiles)) {
            masterBoxSprite[i] = 0;
            LoadSpinSprite(&masterBoxSprite[i], i + 500, spriteID, maskID, xSize, ySize, xTiles * yTiles,
                           "error loading sprite from RLE, " kSpinTypeName " ID ",
                           "error loading sprite from PICT, " kSpinTypeName " ID ");
            IterateInGameEventLoop(1);
        } else
            SpriteLoadFailed("resource not found, " kSpinTypeName " ID ", i + 500, 1, i + 500, resNotFound);
    }

    for (i = 0; i < kMaxRoidTypes; i++) {
        masterAsteroidSprite[i] = 0;
        if (GetSpriteInfo(i + 800, &spriteID, &maskID, &xSize, &ySize, &xTiles, &yTiles)) {
            LoadSpinSprite(&masterAsteroidSprite[i], i + 800, spriteID, maskID, xSize, ySize, xTiles * yTiles,
                           "error loading sprite from RLE, " kSpinTypeName " ID ",
                           "error loading sprite from PICT, " kSpinTypeName " ID ");
            IterateInGameEventLoop(1);
        } else {
            masterAsteroidSprite[i] = 0;
            if (i == 0)
                SpriteLoadFailed("resource not found, " kSpinTypeName " ID ", i + 800, 1, i + 800, resNotFound);
        }
    }

    for (i = 0; i < kMainButtons; i++) {
        menuButtonSprite[i] = 0;
        if (GetSpriteInfo(i + 600, &spriteID, &maskID, &xSize, &ySize, &xTiles, &yTiles)) {
            if (ResourceExists(kRLEDResType, spriteID)) {
                err = SWCreateSpriteFromRLEResource(&menuButtonSprite[i], NULL, spriteID, xTiles * yTiles, 0);
                if ((int32_t)err != 0)
                    SpriteLoadFailed("error loading sprite from RLE, " kSpinTypeName " ID ", i + 600, 2, i + 600, err);
            } else {
                err = SWCreateSpriteFromSinglePictXY(gSpriteWorld, &menuButtonSprite[i], NULL, spriteID, maskID, xSize,
                                   ySize, 0, 0, 0, 1);
                if ((int32_t)err != 0)
                    SpriteLoadFailed("error loading sprite from PICT, " kSpinTypeName " ID ", i + 600, 3, i + 600, err);
                err = ConvertSpriteToRLE(menuButtonSprite[i]);
                if ((int32_t)err != 0)
                    SpriteLoadFailed("error converting sprite to RLE, " kSpinTypeName " ID ", i + 600, 4, i + 600, err);
                menuButtonSprite[i]->numFrames = xTiles * yTiles;
            }
            SWLockSprite(menuButtonSprite[i]);
            SWSetCurrentFrameIndex(menuButtonSprite[i], 0);
            IterateInGameEventLoop(1);
        } else
            menuButtonSprite[i] = 0;
    }

    if (GetSpriteInfo(700, &spriteID, &maskID, &xSize, &ySize, &xTiles, &yTiles)) {
        if (ResourceExists(kRLEDResType, spriteID)) {
            err = SWCreateSpriteFromRLEResource(&masterStarSprite, NULL, spriteID, xTiles * yTiles, 0);
            if ((int32_t)err != 0)
                SpriteLoadFailed("error loading sprite from RLE, " kSpinTypeName " ID ", 700, 2, 700, err);
        } else {
            err = SWCreateSpriteFromSinglePictXY(gSpriteWorld, &masterStarSprite, NULL, spriteID,
                               maskID, xSize, ySize, 0, 0, 0, 1);
            if ((int32_t)err != 0)
                SpriteLoadFailed("error loading sprite from PICT, " kSpinTypeName " ID ", 700, 3, 700, err);
            err = ConvertSpriteToRLE(masterStarSprite);
            if ((int32_t)err != 0)
                SpriteLoadFailed("error converting sprite to RLE, " kSpinTypeName " ID ", 700, 4, 700, err);
            masterStarSprite->numFrames = xTiles * yTiles;
        }
        SWLockSprite(masterStarSprite);
        SWSetCurrentFrameIndex(masterStarSprite, 0);
        IterateInGameEventLoop(1);
    } else {
        masterStarSprite = NULL;
        // BUG: i is kMainButtons here, so this never runs
        if (i == 0)
            SpriteLoadFailed("resource not found, " kSpinTypeName " ID ", 700, 1, 700, resNotFound);
    }

    if (GetSpriteInfo(607, &spriteID, &maskID, &xSize, &ySize, &xTiles, &yTiles)) {
        if (ResourceExists(kRLEDResType, spriteID)) {
            err = SWCreateSpriteFromRLEResource(&menuRolloverSprite, NULL, spriteID, xTiles * yTiles, 0);
            if ((int32_t)err != 0) {
                menuRolloverSprite = NULL;
                crashlog_print("error loading sprite from RLE, " kSpinTypeName " ID ");
                crashlog_print_num(607);
                crashlog_print("\n");
            }
        } else {
            err = SWCreateSpriteFromSinglePictXY(gSpriteWorld, &menuRolloverSprite, NULL, spriteID,
                               maskID, xSize, ySize, 0, 0, 0, 1);
            if ((int32_t)err != 0) {
                menuRolloverSprite = NULL;
                crashlog_print("error loading sprite from PICT, " kSpinTypeName " ID ");
                crashlog_print_num(607);
                crashlog_print("\n");
            }
            // BUG: a failed PICT load leaves the sprite NULL, which this and numFrames below dereference
            err = ConvertSpriteToRLE(menuRolloverSprite);
            if ((int32_t)err != 0) {
                crashlog_print("error converting sprite to RLE, " kSpinTypeName " ID ");
                crashlog_print_num(607);
                crashlog_print("\n");
            }
            menuRolloverSprite->numFrames = xTiles * yTiles;
        }
        if (menuRolloverSprite != NULL) {
            SWLockSprite(menuRolloverSprite);
            SWSetCurrentFrameIndex(menuRolloverSprite, 0);
        }
        IterateInGameEventLoop(1);
    } else {
        menuRolloverSprite = NULL;
        // BUG: i is kMainButtons here, so this never runs
        if (i == 0) {
            crashlog_print("resource not found, " kSpinTypeName " ID ");
            crashlog_print_num(607);
            crashlog_print("\n");
        }
    }

    if (GetSpriteInfo(650, &spriteID, &maskID, &xSize, &ySize, &xTiles, &yTiles)) {
        if (ResourceExists(kRLEDResType, spriteID)) {
            err = SWCreateSpriteFromRLEResource(&cursorSprite, NULL, spriteID, xTiles * yTiles, 0);
            if ((int32_t)err != 0) {
                cursorSprite = NULL;
                crashlog_print("error loading sprite from RLE, " kSpinTypeName " ID ");
                crashlog_print_num(650);
                crashlog_print("\n");
            }
        } else {
            err = SWCreateSpriteFromSinglePictXY(gSpriteWorld, &cursorSprite, NULL, spriteID,
                               maskID, xSize, ySize, 0, 0, 0, 1);
            if ((int32_t)err != 0) {
                crashlog_print("error loading sprite from PICT, " kSpinTypeName " ID ");
                crashlog_print_num(650);
                crashlog_print("\n");
            }
            // BUG: a failed PICT load leaves the sprite NULL, which this and numFrames below dereference
            err = ConvertSpriteToRLE(cursorSprite);
            if ((int32_t)err != 0) {
                crashlog_print("error converting sprite to RLE, " kSpinTypeName " ID ");
                crashlog_print_num(650);
                crashlog_print("\n");
            }
            cursorSprite->numFrames = xTiles * yTiles;
        }
        if (cursorSprite != NULL) {
            SWLockSprite(cursorSprite);
            SWSetCurrentFrameIndex(cursorSprite, 0);
        }
        IterateInGameEventLoop(1);
    } else {
        cursorSprite = NULL;
        // BUG: i is kMainButtons here, so this never runs
        if (i == 0) {
            crashlog_print("resource not found, " kSpinTypeName " ID ");
            crashlog_print_num(650);
            crashlog_print("\n");
        }
    }

    err = SWCreateSpriteFromCicnResource(gSpriteWorld, &masterNavTargetSprite, NULL, 10000, 8, 1);
    if ((int32_t)err != 0)
        SpriteLoadFailed("error loading sprite from cicn, ID ", 10000, 5, 10000, err);
    SWLockSprite(masterNavTargetSprite);
    err = SWCreateRLEData(masterNavTargetSprite, 0);
    if ((int32_t)err != 0)
        SpriteLoadFailed("error converting cicn sprite to RLE, ID ", 10000, 4, 10000, err);

    err = SWCreateSpriteFromCicnResource(gSpriteWorld, &masterWeapTargetSprite, NULL, 10008, 16, 1);
    if ((int32_t)err != 0)
        SpriteLoadFailed("error loading sprite from cicn, ID ", 10008, 5, 10008, err);
    SWLockSprite(masterWeapTargetSprite);
    err = SWCreateRLEData(masterWeapTargetSprite, 0);
    if ((int32_t)err != 0)
        SpriteLoadFailed("error converting cicn sprite to RLE, ID ", 10008, 4, 10008, err);
    IterateInGameEventLoop(1);

    for (i = 0; i < kMaxShipTypes; i++) {
        targetInfoPic[i] = GetPicture(i + 3000);
    }

    for (i = 0; i < 10; i++)
        staticPat[i] = GetPixPat(i + 128);
    IterateInGameEventLoop(1);

    for (i = 0, resID = 1000, q = masterSmokeSprite; i < 2; i++, resID += 8, q++) {
        masterSmokeSprite[i] = 0;
        err = SWCreateSpriteFromCicnResource(gSpriteWorld, q, NULL, resID, 8, 1);
        if ((int32_t)err == 0 && masterSmokeSprite[i] != NULL) {
            SWLockSprite(masterSmokeSprite[i]);
            err = SWCreateRLEData(masterSmokeSprite[i], 0);
            if ((int32_t)err != 0)
                SpriteLoadFailed("error converting sprite to RLE, " kSpinTypeName " ID ", resID, 4, resID, err);
        } else
            masterSmokeSprite[i] = 0;
    }
    IterateInGameEventLoop(1);

    err = SWCreateSpriteFromCicnResource(gSpriteWorld, &alertSprite, NULL, 18000, 2, 1);
    if ((int32_t)err != 0)
        SpriteLoadFailed("error loading sprite from cicn, ID ", 18000, 5, 18000, err);
    SWLockSprite(alertSprite);
    err = SWCreateRLEData(alertSprite, 0);
    if ((int32_t)err != 0)
        SpriteLoadFailed("error converting cicn sprite to RLE, ID ", 18000, 4, 18000, err);

    err = SWCreateSpriteFromCicnResource(gSpriteWorld, &doubleTimeSprite, NULL, 20000, 1, 1);
    if ((int32_t)err != 0)
        SpriteLoadFailed("error loading sprite from cicn, ID ", 20000, 5, 20000, err);
    SWLockSprite(doubleTimeSprite);
    err = SWCreateRLEData(doubleTimeSprite, 0);
    if ((int32_t)err != 0)
        SpriteLoadFailed("error converting cicn sprite to RLE, ID ", 20000, 4, 20000, err);
    IterateInGameEventLoop(1);

    redMapArrow = (Image_t *)GetCIcon(15000);
    greenMapArrow = (Image_t *)GetCIcon(15001);
}

// FUNCTION: EVNOVA 0x004af020
// MAC: _InitSprites i386 0x0001eda2 ppc 0x0002365c
void InitSprites(void)
{
    FloatPoint vel;
    FloatPoint origin;
    float step;
    float angle;
    short i;
    short j;
    short err;

    SWUnlockSpriteLayer(starLayer);
    SWUnlockSpriteLayer(shipLayer);
    SWUnlockSpriteLayer(escortLayer);
    SWUnlockSpriteLayer(stellarLayer);
    SWUnlockSpriteLayer(asteroidLayer);
    SWUnlockSpriteLayer(boxLayer);
    SWUnlockSpriteLayer(smokeLayer);
    SWUnlockSpriteLayer(shotLayer);
    SWUnlockSpriteLayer(explodLayer);
    SWUnlockSpriteLayer(targetLayer);

    firstShipGlowIndex = firstShipLightIndex = firstShipAltIndex =
        firstShipWeapIndex = firstShipShieldIndex = -1;

    origin.x = origin.y = 0.0f;

    for (i = 0; i < kMaxShipTypes; i++) {
        if (masterShipGlowSprite[i] != NULL && firstShipGlowIndex == -1)
            firstShipGlowIndex = i;
        if (masterShipLightSprite[i] != NULL && firstShipLightIndex == -1)
            firstShipLightIndex = i;
        if (masterShipWeaponSprite[i] != NULL && firstShipWeapIndex == -1)
            firstShipWeapIndex = i;
        if (masterShipAltSprite[i] != NULL && firstShipAltIndex == -1)
            firstShipAltIndex = i;
        if (masterShipShieldSprite[i] != NULL && firstShipShieldIndex == -1)
            firstShipShieldIndex = i;

        if ((shipClass[i].shanFlags & kShanSkewCorrect)
                && shipClass[i].framesPer > 0) {
            shipClass[i].frameAngles =
                (short *)AllocAPointer(shipClass[i].framesPer * 2);
            if (shipClass[i].frameAngles == NULL)
                EV_ASSERT_FAIL("Setup.c", 1740, 2035);
            step = 360.0 / (float)shipClass[i].framesPer;
            for (j = 0; j < shipClass[i].framesPer; j++) {
                angle = step * j;
                vel.x = vel.y = 0.0f;
                Accel(angle, 100.0, &vel);
                if (angle <= 90.0 || angle > 270.0) {
                    vel.x *= shipClass[i].compress[0].x;
                    vel.y *= shipClass[i].compress[0].y;
                } else {
                    vel.x *= shipClass[i].compress[1].x;
                    vel.y *= shipClass[i].compress[1].y;
                }
                shipClass[i].frameAngles[j] = CalcInterceptAngle(&origin, &vel);
            }
        }
    }

    IterateInGameEventLoop(1);

    for (i = kMaxShips - 1; i >= 0; i--) {
        ServiceIntroMusic(0);
        err = SWCloneSprite(masterShipSprite[0], &shipData[i].sprite, NULL);
        if ((int32_t)err != 0)
            SpriteLoadFailed("error cloning ship base sprite, #", i, 1, 0, err);
        SWCloneFramesFromMaster(shipData[i].sprite, masterShipSprite[0]);
        SWAddSprite(shipLayer, shipData[i].sprite);
        SWSetSpriteVisible(shipData[i].sprite, 0);
        SWSetCurrentFrameIndex(shipData[i].sprite, 0);
        SWSetSpriteCollideProc(shipData[i].sprite, ShipCollisionHandler);
        shipData[i].sprite->userData = i;
        SWLockSprite(shipData[i].sprite);

        if (firstShipAltIndex != -1) {
            err = SWCloneSprite(masterShipAltSprite[firstShipAltIndex],
                                &shipData[i].altSprite, NULL);
            if ((int32_t)err != 0)
                SpriteLoadFailed("error cloning ship alt sprite, #", i, 1, 0, err);
            SWAddSprite(shipLayer, shipData[i].altSprite);
            SWSetSpriteVisible(shipData[i].altSprite, 0);
            SWSetCurrentFrameIndex(shipData[i].altSprite, 0);
            shipData[i].altSprite->userData = i;
            SWSetSpriteCollideProc(shipData[i].altSprite, ShipCollisionHandler);
            SWLockSprite(shipData[i].altSprite);
            shipData[i].altSprite->translucency = 0;
            shipData[i].altSprite->tintMode = 0;
        }
        if (firstShipGlowIndex != -1) {
            err = SWCloneSprite(masterShipGlowSprite[firstShipGlowIndex],
                                &shipData[i].glowSprite, NULL);
            if ((int32_t)err != 0)
                SpriteLoadFailed("error cloning ship glow sprite, #", i, 1, 0, err);
            SWAddSprite(shipLayer, shipData[i].glowSprite);
            SWSetSpriteVisible(shipData[i].glowSprite, 0);
            SWSetCurrentFrameIndex(shipData[i].glowSprite, 0);
            SWLockSprite(shipData[i].glowSprite);
            shipData[i].glowSprite->translucency = 0x20;
            shipData[i].glowSprite->tintRed = 0x20;
            shipData[i].glowSprite->tintGreen = 0x20;
            shipData[i].glowSprite->tintBlue = 0x20;
            shipData[i].glowSprite->tintMode = 0;
            shipData[i].glowSprite->collisionsDisabled = 1;
        }
        if (firstShipLightIndex != -1) {
            err = SWCloneSprite(masterShipLightSprite[firstShipLightIndex],
                                &shipData[i].lightSprite, NULL);
            if ((int32_t)err != 0)
                SpriteLoadFailed("error cloning ship light sprite, #", i, 1, 0, err);
            SWAddSprite(shipLayer, shipData[i].lightSprite);
            SWSetSpriteVisible(shipData[i].lightSprite, 0);
            SWSetCurrentFrameIndex(shipData[i].lightSprite, 0);
            SWLockSprite(shipData[i].lightSprite);
            shipData[i].lightSprite->translucency = 0x20;
            shipData[i].lightSprite->tintRed = 0x20;
            shipData[i].lightSprite->tintGreen = 0x20;
            shipData[i].lightSprite->tintBlue = 0x20;
            shipData[i].lightSprite->tintMode = 0;
            shipData[i].lightSprite->collisionsDisabled = 1;
        }
        if (firstShipWeapIndex != -1) {
            err = SWCloneSprite(masterShipWeaponSprite[firstShipWeapIndex],
                                &shipData[i].weapSprite, NULL);
            if ((int32_t)err != 0)
                SpriteLoadFailed("error cloning ship weapon sprite, #", i, 1, 0, err);
            SWAddSprite(shipLayer, shipData[i].weapSprite);
            SWSetSpriteVisible(shipData[i].weapSprite, 0);
            SWSetCurrentFrameIndex(shipData[i].weapSprite, 0);
            SWLockSprite(shipData[i].weapSprite);
            shipData[i].weapSprite->translucency = 0x20;
            shipData[i].weapSprite->tintRed = 0x20;
            shipData[i].weapSprite->tintGreen = 0x20;
            shipData[i].weapSprite->tintBlue = 0x20;
            shipData[i].weapSprite->tintMode = 0;
            shipData[i].weapSprite->collisionsDisabled = 1;
        }
        if (firstShipShieldIndex != -1) {
            err = SWCloneSprite(masterShipShieldSprite[firstShipShieldIndex],
                                &shipData[i].shieldSprite, NULL);
            if ((int32_t)err != 0)
                SpriteLoadFailed("error cloning ship shield sprite, #", i, 1, 0, err);
            SWAddSprite(shipLayer, shipData[i].shieldSprite);
            SWSetSpriteVisible(shipData[i].shieldSprite, 0);
            SWSetCurrentFrameIndex(shipData[i].shieldSprite, 0);
            SWLockSprite(shipData[i].shieldSprite);
            shipData[i].shieldSprite->translucency = 0x20;
            shipData[i].shieldSprite->tintRed = 0x20;
            shipData[i].shieldSprite->tintGreen = 0x20;
            shipData[i].shieldSprite->tintBlue = 0x20;
            shipData[i].shieldSprite->tintMode = 0;
        }
    }

    for (i = 0; i < kMasterStellarSprites; i++)
        masterStellarSprite[i] = NULL;

    ServiceIntroMusic(0);
    LoadMasterStellarSprite(0);

    for (i = 0; i < kNavSlots; i++) {
        err = SWCreateEmptySprite(&stellarSprite[i], NULL);
        if ((int32_t)err != 0)
            ReportSpriteLoadError(1, 0, err);
        SWCloneFramesFromMaster(stellarSprite[i], masterStellarSprite[0]);
        SWAddSprite(stellarLayer, stellarSprite[i]);
        SWSetCurrentFrameIndex(stellarSprite[i], 0);
        SWSetSpriteVisible(stellarSprite[i], 0);
        SWLockSprite(stellarSprite[i]);
    }

    ServiceIntroMusic(0);

    for (i = 0; i < kMaxExplods; i++) {
        err = SWCloneSprite(masterExplodSprite[0], &explodData[i].sprite, NULL);
        if ((int32_t)err != 0)
            SpriteLoadFailed("error cloning explosion sprite, #", i, 1, 0, err);
        SWAddSprite(explodLayer, explodData[i].sprite);
        SWSetCurrentFrameIndex(explodData[i].sprite, 0);
        SWSetSpriteVisible(explodData[i].sprite, 0);
        explodData[i].sprite->translucency = 0x20;
        explodData[i].sprite->tintRed = 0x20;
        explodData[i].sprite->tintGreen = 0x20;
        explodData[i].sprite->tintBlue = 0x20;
        explodData[i].sprite->tintMode = 0;
        SWLockSprite(explodData[i].sprite);
    }

    ServiceIntroMusic(0);

    for (i = 0; i < kMaxShots; i++) {
        err = SWCloneSprite(masterShotSprite[0], &shotData[i].sprite, NULL);
        if ((int32_t)err != 0)
            SpriteLoadFailed("error cloning shot sprite, #", i, 1, 0, err);
        SWAddSprite(shotLayer, shotData[i].sprite);
        SWSetCurrentFrameIndex(shotData[i].sprite, 0);
        SWSetSpriteVisible(shotData[i].sprite, 0);
        shotData[i].sprite->userData = i;
        SWLockSprite(shotData[i].sprite);
    }

    ServiceIntroMusic(0);

    for (i = 0; i < kMaxBoxes; i++) {
        err = SWCloneSprite(masterBoxSprite[0], &boxData[i].sprite, NULL);
        if ((int32_t)err != 0)
            SpriteLoadFailed("error cloning box sprite, #", i, 1, 0, err);
        SWAddSprite(boxLayer, boxData[i].sprite);
        SWSetCurrentFrameIndex(boxData[i].sprite, 0);
        SWSetSpriteVisible(boxData[i].sprite, 0);
        SWLockSprite(boxData[i].sprite);
    }

    for (i = 0; i < kMaxPods; i++) {
        err = SWCloneSprite(masterShipSprite[kMaxShipTypes - 1],
                            &podData[i].sprite, NULL);
        if ((int32_t)err != 0)
            SpriteLoadFailed("error cloning pod sprite, #", i, 1, 0, err);
        SWAddSprite(boxLayer, podData[i].sprite);
        SWSetCurrentFrameIndex(podData[i].sprite, 0);
        SWSetSpriteVisible(podData[i].sprite, 0);
        SWLockSprite(podData[i].sprite);
    }

    for (i = 0; i < kMaxStars; i++) {
        err = SWCloneSprite(masterStarSprite, &starData[i].sprite, NULL);
        if ((int32_t)err != 0)
            SpriteLoadFailed("error cloning star sprite, #", i, 1, 0, err);
        starData[i].sprite->tintRed = 0;
        starData[i].sprite->tintGreen = 0;
        starData[i].sprite->tintBlue = 0;
        SWAddSprite(starLayer, starData[i].sprite);
        SWSetCurrentFrameIndex(starData[i].sprite, 0);
        SWSetSpriteVisible(starData[i].sprite, 0);
        SWLockSprite(starData[i].sprite);
    }

    if (masterSmokeSprite[0] != NULL) {
        for (i = 0; i < kMaxDebris; i++) {
            err = SWCloneSprite(masterSmokeSprite[0], &smokeData[i].sprite, NULL);
            if ((int32_t)err != 0)
                SpriteLoadFailed("error cloning smoke sprite, #", i, 1, 0, err);
            SWAddSprite(smokeLayer, smokeData[i].sprite);
            SWSetCurrentFrameIndex(smokeData[i].sprite, 0);
            SWSetSpriteVisible(smokeData[i].sprite, 0);
            SWLockSprite(smokeData[i].sprite);
        }
    } else {
        for (i = 0; i < kMaxDebris; i++)
            smokeData[i].sprite = NULL;
    }

    ServiceIntroMusic(0);

    for (i = 0; i < kMaxAsteroids; i++) {
        err = SWCloneSprite(masterAsteroidSprite[0], &asteroidData[i].sprite, NULL);
        if ((int32_t)err != 0)
            SpriteLoadFailed("error cloning asteroid sprite, #", i, 1, 0, err);
        SWAddSprite(asteroidLayer, asteroidData[i].sprite);
        SWSetCurrentFrameIndex(asteroidData[i].sprite, 0);
        SWSetSpriteVisible(asteroidData[i].sprite, 0);
        SWSetSpriteCollideProc(asteroidData[i].sprite, AsteroidCollisionHandler);
        SWLockSprite(asteroidData[i].sprite);
        asteroidData[i].sprite->userData = i;
    }

    for (i = 0; i < kMaxNavTargets; i++) {
        err = SWCloneSprite(masterNavTargetSprite, &navTargetSprite[i], NULL);
        if ((int32_t)err != 0)
            SpriteLoadFailed("error cloning nav target sprite, #", i, 1, 0, err);
        SWAddSprite(targetLayer, navTargetSprite[i]);
        SWSetSpriteVisible(navTargetSprite[i], 0);
        navTargetSprite[i]->tintMode = 0;
        navTargetSprite[i]->translucency = 0x20;
        navTargetSprite[i]->tintRed = 0x20;
        navTargetSprite[i]->tintGreen = 0x20;
        navTargetSprite[i]->tintBlue = 0x20;
        SWLockSprite(navTargetSprite[i]);
    }

    for (i = 0; i < kMaxNavTargets; i++) {
        err = SWCloneSprite(masterWeapTargetSprite, &weapTargetSprite[i], NULL);
        if ((int32_t)err != 0)
            SpriteLoadFailed("error cloning weap target sprite, #", i, 1, 0, err);
        SWAddSprite(targetLayer, weapTargetSprite[i]);
        SWSetSpriteVisible(weapTargetSprite[i], 0);
        weapTargetSprite[i]->tintMode = 0;
        weapTargetSprite[i]->translucency = 0x20;
        weapTargetSprite[i]->tintRed = 0x20;
        weapTargetSprite[i]->tintGreen = 0x20;
        weapTargetSprite[i]->tintBlue = 0x20;
        SWLockSprite(weapTargetSprite[i]);
    }

    if (cursorSprite != NULL) {
        SWAddSprite(interfaceLayer, cursorSprite);
        SWSetSpriteVisible(cursorSprite, 0);
        cursorSprite->tintMode = 0;
        cursorSprite->translucency = 0x20;
        cursorSprite->tintRed = 0x20;
        cursorSprite->tintGreen = 0x20;
        cursorSprite->tintBlue = 0x20;
        SWLockSprite(cursorSprite);
    }

    ServiceIntroMusic(0);

    SWAddSprite(interfaceLayer, alertSprite);
    SWSetCurrentFrameIndex(alertSprite, 0);
    SWSetSpriteVisible(alertSprite, 0);
    SWLockSprite(alertSprite);
    PlaceAlertSprite();

    SWAddSprite(interfaceLayer, doubleTimeSprite);
    SWSetCurrentFrameIndex(doubleTimeSprite, 0);
    SWSetSpriteVisible(doubleTimeSprite, 0);
    SWLockSprite(doubleTimeSprite);
    SWSetSpriteLocation(doubleTimeSprite, 0, 0);

    SWLockSpriteLayer(starLayer);
    SWLockSpriteLayer(shipLayer);
    SWLockSpriteLayer(escortLayer);
    SWLockSpriteLayer(shotLayer);
    SWLockSpriteLayer(stellarLayer);
    SWLockSpriteLayer(asteroidLayer);
    SWLockSpriteLayer(boxLayer);
    SWLockSpriteLayer(smokeLayer);
    SWLockSpriteLayer(explodLayer);
    SWLockSpriteLayer(targetLayer);
    SWLockSpriteLayer(interfaceLayer);

    ServiceIntroMusic(0);

    ResetCommRect();
}

/* the alert sprite, in the view's top right corner (screen coordinates: it is in the interface layer) */
void PlaceAlertSprite(void)
{
    SWSetSpriteLocation(alertSprite, UI::ScreenWidth() - kStatusBarWidth - GetSpriteXSize(alertSprite), 0);
}

/* 'r' 'l' <MacRoman e-diaeresis, 0x91> '8'/'D' as big-endian ResTypes */

#define kStellarSpinResBase 1000

// FUNCTION: EVNOVA 0x004b0a30
// MAC: _LoadMasterStellarSprite i386 0x0001eb72 ppc 0x00023474
void LoadMasterStellarSprite(short type)
{
    short spriteID, maskID, xSize, ySize, xTiles, yTiles;
    short err;
    SpriteRec *entry;

    if (GetSpriteInfo(type + kStellarSpinResBase, &spriteID, &maskID, &xSize, &ySize, &xTiles, &yTiles)) {
        if (ResourceExists(kRLEDResType, spriteID)) {
            err = SWCreateSpriteFromRLEResource(&masterStellarSprite[type], NULL, spriteID,
                                xTiles * yTiles, 0);
            if ((int)err != 0)
                SpriteLoadFailed("error loading stellar sprite from RLE, type ",
                                 type, 2, type + kStellarSpinResBase, err);
            SWLockSprite(masterStellarSprite[type]);
        } else {
            err = SWCreateSpriteFromSinglePictXY(gSpriteWorld, &masterStellarSprite[type], NULL,
                                spriteID, maskID, xSize, ySize, 0, 0, 0, 1);
            if ((int)err != 0)
                SpriteLoadFailed("error loading stellar sprite from PICT, type ",
                                 type, 3, type + kStellarSpinResBase, err);
            SWLockSprite(masterStellarSprite[type]);
            err = ConvertSpriteToRLE(masterStellarSprite[type]);
            if ((int)err != 0)
                SpriteLoadFailed("error converting stellar sprite to RLE, type ",
                                 type, 4, type + kStellarSpinResBase, err);
        }
        entry = masterStellarSprite[type];
        if (entry != NULL)
            entry->numFrames = xTiles * yTiles;
    }
}

// FUNCTION: EVNOVA 0x004b4cf0
// MAC: _ConvertSpriteToRLE i386 0x0001e489 ppc 0x00022d98
short ConvertSpriteToRLE(SpriteRec *set)
{
    int i;
    short result;
    Rect *r;
    SWFrameRec *first;
    int err;

    first = set->frameArray[0];
    if (first->framePort != NULL) {
        r = GetCWindowPortRect(first->framePort);
        first = set->frameArray[0];
        if ((r->right - r->left) * (r->bottom - r->top) !=
            (first->frameRect.right - first->frameRect.left) * set->numFrames *
            (first->frameRect.bottom - first->frameRect.top)) {
            crashlog("error - bad sprite image size");
            return -1;
        }
    }
    result = SWCreateRLEData(set, 0);
    err = result;
    if (err == 0) {
        if (set->frameArray[0]->framePort != NULL)
            DisposeGWorld(set->frameArray[0]->framePort);
        if (set->frameArray[0]->maskPort != NULL)
            DisposeGWorld(set->frameArray[0]->maskPort);
        i = 0;
        if (set->maxFrames > 0) {
            do {
                set->frameArray[(short)i]->framePort = NULL;
                set->frameArray[(short)i]->maskPort = NULL;
                i = i + 1;
            } while ((short)i < set->maxFrames);
        }
    } else {
        return result;
    }
    if (err == 0)
        goto clearCaches;
finish:
    SWLockSprite(set);
    return result;
clearCaches:
    set->sharedPictGWorld = NULL;
    set->sharedMaskGWorld = NULL;
    goto finish;
}

/* Every out-parameter is optional, so a caller that wants only the tile counts
   passes NULL for the rest. */

/* GetSpriteInfo's globals */
SpinHandle spriteInfoHand;

// FUNCTION: EVNOVA 0x004b4e10
// MAC: _GetSpriteInfo i386 0x0001bf80 ppc 0x000200a0
Boolean GetSpriteInfo(short id, short *spriteID, short *maskID, short *xSize,
                  short *ySize, short *xTiles, short *yTiles)
{
    spriteInfoHand = (SpinHandle)GetResource(kSpinType, id);
    if (spriteInfoHand != NULL) {
        if (spriteID != NULL)
            *spriteID = (*spriteInfoHand)->spriteID;
        if (maskID != NULL)
            *maskID = (*spriteInfoHand)->maskID;
        if (xSize != NULL)
            *xSize = (*spriteInfoHand)->xSize;
        if (ySize != NULL)
            *ySize = (*spriteInfoHand)->ySize;
        if (xTiles != NULL)
            *xTiles = (*spriteInfoHand)->xTiles;
        if (yTiles != NULL)
            *yTiles = (*spriteInfoHand)->yTiles;
        ReleaseResource((Handle)spriteInfoHand);
        return 1;
    }
    return 0;
}

// FUNCTION: EVNOVA 0x004b6620
// MAC: _ReportSpriteLoadError i386 0x0001ea6c ppc 0x0002337c
void ReportSpriteLoadError(short label, short value, short special)
{
    unsigned char labelText[256];
    unsigned char valueText[256];
    unsigned char specialText[256];
    extern void NumToString(int value, unsigned char *str);  /* 0x004bbce0 */

    if ((int32_t)special == 0)
        return;
    GetIndString(labelText, kAlertStrList, label);
    NumToString(value, valueText);
    if (special == -0xc0)
        GetIndString(specialText, kSpecialValueStrList, 1);
    else if (special == -0x27)
        GetIndString(specialText, kSpecialValueStrList, 2);
    else if ((int32_t)special == 0x6b)
        GetIndString(specialText, kSpecialValueStrList, 3);
    else if ((int32_t)special == -0x6c)
        GetIndString(specialText, kSpecialValueStrList, 4);
    else
        NumToString(special, specialText);
    ParamText(labelText, valueText, specialText, NULL);
    StopAlert(0x83, 0);
}

/* logs prefix and logNum, then shows the sprite load error alert */
void SpriteLoadFailed(const char *prefix, int logNum, short label, short value, short err)
{
    crashlog_print(prefix);
    crashlog_print_num(logNum);
    crashlog_print("\n");
    ReportSpriteLoadError(label, value, err);
}

// FUNCTION: EVNOVA 0x004b6810
// MAC: _DisposeClonedSprite i386 0x0001b3d0 ppc 0x0001f450
void DisposeClonedSprite(SpritePtr *p)
{
    SpritePtr q;

    if (p == NULL)
        return;
    q = *p;
    if (q == NULL)
        return;
    if (q->isEmptySprite != 0) {
        q->frameArray = NULL;
        q->numFrames = 0;
    }
    SWDisposeSprite(p);
}
