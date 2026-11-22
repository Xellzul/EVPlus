/* handleplayer.cpp -- the player ship: controls, communication, weapons fire, wrapping, escort commands */

#include "burgermath.h"
#include "ai.h"
#include "dialogs.h"
#include "game.h"
#include "sound.h"
#include "space.h"
#include "stellar.h"
#include "prototypes.h"
#include "gameutil.h"
#include "missions.h"
#include "player.h"
#include "render.h"
#include "ships.h"
#include "crashlog.h"
#include "viewscale.h"
#include "screen.h"
#include "platform.h"

/* the cheat keys HandlePlayer reads by code */
#define kScanExploreCheat   kScanLang1
#define kScanArmorCheat     kScanF8
#define kScanDamageCheat    kScanF9
#define kScanInfoCheat      kScanF11 /* the mac's F10 is CE's explore cheat here */
#define AltKeyQuick()       (QuickKeyCheck(kScanLAlt) || QuickKeyCheck(kScanRAlt))
#define CmdKeyQuick()       (QuickKeyCheck(kScanLAlt) || QuickKeyCheck(kScanRAlt))
#define ShiftKeyQuick()     (QuickKeyCheck(kScanLShift) || QuickKeyCheck(kScanRShift))
#define CtrlKeyQuick()      (QuickKeyCheck(kScanLCtrl) || QuickKeyCheck(kScanRCtrl))

Boolean warpSoundDone;
Boolean playerTurnedThisFrame;
Boolean dontMovePlayerAfterLanding;
Boolean abOn;
short newbieFlag;
float sabotageCounter;
Boolean dockLand;
Boolean dockTryFlag;
Boolean boardTryFlag;
FloatPoint wrapDelta;
Boolean lastJumpDistOk;
Boolean ackCommFlag;
Boolean navModeSelectFlag;
Boolean navTargetSelectFlag;
Boolean navDefaultSelectFlag;
Boolean helpKeyFlag;
Boolean cloakFlag;
Boolean fighterCommandFlag;
Boolean weapModeSelectFlag;
Boolean weapTargetSelectFlag;
Boolean weapDefaultSelectFlag;
Boolean playerInfoFlag;
Boolean mapTryFlag;
Boolean jumpTryFlag;
Boolean weapSelectFlag;
Boolean spawnShipFlag;
Boolean weapCloseTargFlag;
Boolean missionInfoFlag;
Boolean commTryFlag;
Boolean mapZoomFlag;
Boolean framerateFlag;
Boolean escortMenuFlag;
Boolean escortGroupFlag[5];
Boolean escortCommandFlag[4];
Boolean pauseKeyFlag;
static Boolean speedUpFlag;
static Boolean speedUpToggled;
short selEscortGroup;
short lastEscortCount;

EXTERN_C_BEGIN

/* the sprite record the engine hangs off ShipObj.unk0 and stellarSprite[];
   gameloop.cpp's CursorSpriteRec is the same object -- only the three fields
   this TU hit-tests are transcribed */

/* SpriteWorldRec +0x1c is the third SWFrameRec in the arrival blit, so the
   macro casts it. */
#define SWWindowFrame(sw)   ((sw)->windowFrameP)
#define SWWindowRect(sw)    (&(sw)->windRect)

                                                            /* 0x004b8ca0 */
                                                            /* 0x0046aad0 */
                                                            /* 0x004192d0 */
                                                            /* 0x0046eeb0 */
                                                            /* 0x0041ad50 */
                                                            /* 0x0046ef00 */
                                                            /* 0x00465d90 */
                                                            /* 0x00413990 */
                                                            /* 0x004254b0 */
                                                            /* 0x00475f70 */
                                                            /* 0x00475830 */
                                                            /* 0x00470220 */

// FUNCTION: EVNOVA 0x0044aa70
// MAC: _HandlePlayer i386 0x00068390 ppc 0x00070b28
void HandlePlayer(ShipObj *player)
{
    Boolean jumping;
    short i;
    short j;
    short mapLink;
    short n;
    short count;
    short days;
    short fuelCap;
    short turnStep;
    float turnLim;
    float dist;
    float heading;
    float angle;
    Point pt;
    float dmg;
    Boolean ok;
    Rect box;
    Rect hit;
    Str255 msg;
    Str255 name;
    FloatPoint pos;
    FloatPoint dest;
    FloatPoint origin;
    short groupCount[4];
    float step;
    Boolean linked[kNavSlots];
    Boolean canJump;
    short lost;
    short dir;
    short k;
    Boolean found;
    Boolean escortHit;
    Boolean turned;
    Boolean targetUncloaked;
    Boolean targetCloaked;
    Boolean disabled;
    Boolean dying;
    Boolean routeDone;
    Boolean flag;
    Boolean destructing;
    short sys;
    short waiting;
    short rank;
    double span;

    quickKeyCheckResult = 0;
    if (settings.windowed == 0 && settings.noHyperGamma == 0 && ignoreKeys == 0) {
        if (player->jumpTimer > 0.0f) {
            if (hyperGamma < 0.0)
                hyperGamma = 0;
            if (hyperGamma > 100.0)
                hyperGamma = 100.0f;
            /* 1.1.1 */
            if (!screenFaded && hyperGamma > 0.0)
                FadeWhiteIn();
        } else if (hyperGamma >= 0.0) {
            hyperGamma = -1.0f;
            FadeWhiteOut();
        }
    }
    if (cheatsActive == 0)
        gameFrozen = 0;

    if (player->tractorShip != -1) {
        if (player->tractorShip != player->index) {
            if (!shipData[player->tractorShip].active
                    || shipData[player->tractorShip].IsDisabled()
                    || shipData[player->tractorShip].IsDying()) {
                player->tractorShip = -1;
                player->tractorTick = 0;
            }
        }
        step = shipData->AccelRate() * 0.25;
        if (player->tractorShip == player->index) {
            pos.x = 0.0f;
            pos.y = 0.0f;
        } else {
            pos.x = shipData[player->tractorShip].vel.x;
            pos.y = shipData[player->tractorShip].vel.y;
        }
        if (player->vel.x > pos.x + step)
            player->vel.x -= step;
        if (player->vel.x < pos.x - step)
            player->vel.x += step;
        if (player->vel.y > pos.y + step)
            player->vel.y -= step;
        if (player->vel.y < pos.y - step)
            player->vel.y += step;
        if (FloatAbs(player->vel.x - pos.x) < step)
            player->vel.x = pos.x;
        if (FloatAbs(player->vel.y - pos.y) < step)
            player->vel.y = pos.y;
        if (ReadTick() >= player->tractorTick + 0x1e)
            player->tractorShip = -1;
    }

    heading = player->heading;
    playerTurnedThisFrame = 0;
    if (player->jumpTimer <= 0.0f)
        WrapPlayerPosition();
    if (player->unk6e == 1 && player->targetShip != -1) {
        if (!shipData[player->targetShip].active
                || shipData[player->targetShip].IsDying()
                || AIShipIsEmergingFromHypergate(&shipData[player->targetShip])
                || (shipData[player->targetShip].IsCloaked()
                    && shipData[player->targetShip].parentShip != 0
                    && !player->CanTargetCloakedShips())) {
            player->targetShip = -1;
            statusRedraw[kRedrawTarg] = 1;
        }
    }

    if (!player->active) {
        player->targetShip = -1;
        player->targetStellar = -1;
        if (player->deathTimer > -240.0f) {
            player->deathTimer = player->deathTimer - 1.0f;
        } else {
            if (playerSabotaged != 0) {
                for (i = 0; i < kMaxOutfits; i++) {
                    if (shipSystems[i] > 0) {
                        for (j = 0; j < kOutfMods; j++) {
                            if (outfitSystem[i].modType[j] == OutfMod::BombDestroys
                                    && outfitSystem[i].modVal[j] > 0) {
                                HideCursorSprite();
                                RefreshSAT();
                                ShowTheCursor();
                                LoadDescription(textShowString,
                                                outfitSystem[i].modVal[j]);
                                ShowTextDialog(0, 1);
                                SetAllKeyCmdFlags();
                                break;
                            }
                        }
                    }
                }
            }
            gPause = 1;
        }
        if (QuickKeyCheck(keyPrefs.cheatRevive)) {
            ResetPlayer(1);
            player->targetShip = -1;
            player->selectedWeapon = -1;
            RedrawStatus(1);
        }
        return;
    }

    disabled = player->IsDisabled();
    if (disabled && player->jumpTimer > 0.0f) {
        span = (ReadTick() - player->jumpTick)
               * shipClass[player->type].mobilityFactor / (WarpSoundDuration() * 0.01);
        step = span;
        if (shipClass[player->type].mobilityFactor != 0)
            step = step - 35.0 / shipClass[player->type].mobilityFactor;
        if (step > 0.0) {
            if (step > player->MaxSpeed())
                step = player->MaxSpeed();
            player->vel.x = 0;
            player->vel.y = 0;
            Accel(player->heading, step, &player->vel);
        }
        player->jumpTimer = -1.0f;
        GetIndString(msg, kGameStrList, 0x23);
        DisplayComm((PStrChar *)msg, 0xfa);
        if (ST_IsSoundPlaying(warpUpSnd[0])) {
            ST_HaltSound(warpUpSnd[0]);
            warpSoundDone = 0;
        }
        if (ST_IsSoundPlaying(warpUpSnd[1])) {
            ST_HaltSound(warpUpSnd[1]);
            warpSoundDone = 0;
        }
        PlaySound(beepSnd[1], 0x32, gSoundVolume, gSoundVolume);
    }

    dying = player->IsDying();
    if (disabled) {
        player->vel.x *= 0.995;
        player->vel.y *= 0.995;
    } else {
        player->boarded = 0;
    }
    if (playerRepairInhibit >= 0.0)
        playerRepairInhibit = playerRepairInhibit - gSpeedMult;
    if (disabled && playerRepairInhibit < 0.0 && player->CanSelfRepair()) {
        if (shipClass[player->type].flags & kShipArmorDisableTenth)
            player->armor = player->ArmorCapacity() * 0.1 + 1.0;
        else
            player->armor = player->ArmorCapacity() * 0.3333 + 1.0;
        GetIndString(msg, kGameStrList, 0x25);
        DisplayComm((PStrChar *)msg, 0xfa);
        PlaySound(beepSnd[4], 1, gSoundVolume, gSoundVolume);
    }
    if (!disabled)
        HandleShipPointDefense(player);

    turned = 0;
    if (frameCounter % 60 == 0) {
        oldPlayerThreatened = playerThreatened;
        playerThreatened = IsPlayerThreatened();
        if (playerThreatened && !oldPlayerThreatened
                && player->escapePods <= 0) {
            PlaySound(redAlertSnd, 5, gSoundVolume, gSoundVolume);
            if (settings.volume <= 1)
                alertSpritePending = 0x1e;
        }
    }

    if (player->escapePods > 0) {
        AdjustedAccel(player->heading,
                      player->AccelRate() * gSpeedMult,
                      &player->vel, player->MaxSpeed());
        player->pos.x += player->vel.x * gSpeedMult;
        player->pos.y += player->vel.y * gSpeedMult;
        player->escapePods -= 1;
        if (player->escapePods == 0) {
            FadeScreenOut();
            ClearStarfield();
            ClearParticles(0);
            SWHideAllSprites(gSpriteWorld);
            SWSetPortToWindow(gSpriteWorld);
            ForeColor(blackColor);
            PaintRect(GetCWindowPortRect((WindowRef)gWind));
            FadeScreenIn();
            ShowTheCursor();
            SWSetPortToWindow(gSpriteWorld);
            ForeColor(blackColor);
            PaintRect(GetCWindowPortRect((WindowRef)gWind));
            ShowTheCursor();
            LoadDescription(textShowString, 0x36af);
            ShowTextDialog(0, 1);
            SWSetPortToWindow(gSpriteWorld);
            ForeColor(blackColor);
            PaintRect(GetCWindowPortRect((WindowRef)gWind));
            FadeScreenOut();
            for (i = 0; i < kMaxActiveMissions; i++) {
                if (currentMissionObjectives[i].active) {
                    AbortMission(i, 1);
                    currentMissionObjectives[i].active = 0;
                }
            }
            n = player->curSystem;
            ResetPlayer(0);
            EvalMissionBitSetString(shipClass[shipData->type].onPurchase);
            DeleteAllComputerShips(1, 1);
            j = EscapeStellarSearch(n);
            if (j == -1)
                player->curSystem = 0;
            else
                player->curSystem = stellarData[j].system;
            mapScroll.h = systemData[player->curSystem].pos.h;
            mapScroll.v = systemData[player->curSystem].pos.v;
            for (i = 0; i < kMaxWeapTypes; i++) {
                player->weap[0].count = 0;  /* BUG: clears weap[0] every pass, not weap[i] */
                player->weap[0].ammo = 0;
            }
            for (i = 0; i < kMaxWeapTypes; i++) {
                if (shipClass[player->type].weapAmmo[i] > 0) {
                    if (weaponStats[i].ammoType >= 0
                            && weaponStats[i].ammoType < kMaxWeapTypes
                            && weaponStats[i].guidance != kGuidanceBay)
                        player->weap[weaponStats[i].ammoType].ammo =
                            shipClass[player->type].weapAmmo[i];
                    else
                        player->weap[i].ammo =
                            shipClass[player->type].weapAmmo[i];
                }
                player->weap[i].count = shipClass[player->type].weapCount[i];
                player->weap[i].reloadTimer = 0;
                player->weap[i].burstCount = 0;
            }
            AutoSetExploration(player->curSystem, 0, 1);
            RandomizeStars();
            player->fuel = player->FuelCapacity();
            player->shield = player->ShieldCapacity();
            // BUG: armour is refilled to the SHIELD capacity after the escape pod, so the new ship starts
            // with too much or too little armour; CE fixed the same slip in the debug restore (F21.04) but
            // not here
            player->armor = player->ShieldCapacity();
            SetupShipsInSystem(player->curSystem, 1);
            n = Rand(0x1e) + 15;
            for (i = 0; i < n; i++)
                IncrementGameTime();
            HandleStellarSystemVisibility();
            pstrcpy(shipName, shipClass->name);
            PtoCstr(shipName);
            strcat((char *)shipName, " ");
            for (i = 0; i <= 3; i++) {
                NumToString(Rand(9) + 1, msg);
                PtoCstr(msg);
                strcat((char *)shipName, (char *)msg);
            }
            CtoPstr((char *)shipName);
            ForeColor(blackColor);
            PaintRect(GetCWindowPortRect((WindowRef)gWind));
            SetSystemBackgroundColor();
            PaintRect(&SWWindowFrame(gSpriteWorld)->frameRect);
            ForeColor(blackColor);
            RefreshSAT();
            RedrawStatus(1);
            statusRedraw[kRedrawTarg] = statusRedraw[kRedrawNav] = statusRedraw[kRedrawWeap] =
                statusRedraw[kRedrawFuel] = statusRedraw[kRedrawShield] = 1;
            for (i = 0; i < kMaxSystems; i++) {
                if (systemData[i].govt >= 0)
                    playerRecord[i] =
                        govtData[systemData[i].govt].initialRec;
                else
                    playerRecord[i] = 0;
            }
            if (strictPlay) {
                ok = 0;
                for (i = 0; i < kNavSlots; i++) {
                    if (systemData[player->curSystem].navDefault[i] != -1) {
                        SavePilotFile(
                            systemData[player->curSystem].navDefault[i]);
                        ok = 1;
                        break;
                    }
                }
                if (!ok)
                    SavePilotFile(0);
            }
            FadeScreenIn();
        }
        return;
    }

    if (playerSabotaged != 0) {
        if (sabotageCounter < 0.0f) {
            sabotageCounter = Rand(100);
        } else if (!dying) {
            sabotageCounter = sabotageCounter + gSpeedMult;
            if (sabotageCounter > 300.0f) {
                PlaySound(explodSnd[0], 0x64, gSoundVolume, gSoundVolume);
                if (playerSabotaged == 1) {
                    player->armor = -1.0f;
                    player->shield = -1.0f;
                    player->jumpTimer = -1.0f;
                    msg[0] = 0;
                    for (i = 0; i < kMaxOutfits; i++) {
                        if (shipSystems[i] > 0) {
                            for (j = 0; j < kOutfMods; j++) {
                                if (outfitSystem[i].modType[j] == OutfMod::BombDestroys) {
                                    if (shipSystems[i] > 1
                                            && lcPluralOutfitName[i][0]) {
                                        strcpy((char *)msg, "");
                                        StrcatIndCString((char *)msg, kGameStrList, 0x27);
                                        strcat((char *)msg, " ");
                                        pstrcpy(name, lcPluralOutfitName[i]);
                                        PtoCstr(name);
                                        strcat((char *)msg, (char *)name);
                                        strcat((char *)msg, " ");
                                        StrcatIndCString((char *)msg, kGameStrList, 0x29);
                                        CtoPstr((char *)msg);
                                    } else if (shipSystems[i] == 1
                                            && lcOutfitName[i][0]) {
                                        strcpy((char *)msg, "");
                                        StrcatIndCString((char *)msg, kGameStrList, 0x27);
                                        strcat((char *)msg, " ");
                                        pstrcpy(name, lcOutfitName[i]);
                                        PtoCstr(name);
                                        strcat((char *)msg, (char *)name);
                                        strcat((char *)msg, " ");
                                        StrcatIndCString((char *)msg, kGameStrList, 0x28);
                                        CtoPstr((char *)msg);
                                    }
                                    break;
                                }
                            }
                        }
                        if (msg[0])
                            break;
                    }
                    if (ST_IsSoundPlaying(warpUpSnd[0])) {
                        ST_HaltSound(warpUpSnd[0]);
                        warpSoundDone = 0;
                    }
                    if (ST_IsSoundPlaying(warpUpSnd[1])) {
                        ST_HaltSound(warpUpSnd[1]);
                        warpSoundDone = 0;
                    }
                    if (!msg[0]) {
                        if (leetMode)
                            pstrcpy(msg, "\pSomeone set up us the bomb!");
                        else
                            GetIndString(msg, kGameStrList, 0x26);
                    }
                    DisplayComm((PStrChar *)msg, 0x168);
                } else {
                    for (i = 0; i < kMaxOutfits; i++) {
                        count = shipSystems[i];
                        if (count > 0) {
                            for (j = 0; j < kOutfMods; j++) {
                                if (outfitSystem[i].modType[j] == OutfMod::BombDisables) {
                                    if (count > 1) {
                                        strcpy((char *)msg, "");
                                        StrcatIndCString((char *)msg, kGameStrList, 0x27);
                                        strcat((char *)msg, " ");
                                        pstrcpy(name, lcPluralOutfitName[i]);
                                        PtoCstr(name);
                                        strcat((char *)msg, (char *)name);
                                        strcat((char *)msg, " ");
                                        StrcatIndCString((char *)msg, kGameStrList, 0x29);
                                        CtoPstr((char *)msg);
                                    } else {
                                        strcpy((char *)msg, "");
                                        StrcatIndCString((char *)msg, kGameStrList, 0x27);
                                        strcat((char *)msg, " ");
                                        pstrcpy(name, lcOutfitName[i]);
                                        PtoCstr(name);
                                        strcat((char *)msg, (char *)name);
                                        strcat((char *)msg, " ");
                                        StrcatIndCString((char *)msg, kGameStrList, 0x28);
                                        CtoPstr((char *)msg);
                                    }
                                    DisplayComm((PStrChar *)msg, 0x190);
                                    if (outfitSystem[i].modVal[j] >= 0x80 && outfitSystem[i].modVal[j] < 0xc0)
                                        CreateExplosion(
                                            player->pos,
                                            outfitSystem[i].modVal[j] - 0x80, 0, 1);
                                    shipSystems[i] = 0;
                                    DamageShip(player, &player->pos, 0,
                                               Rand((player->ArmorCapacity() * 0.5 + 1.0))
                                               + player->ArmorCapacity(),
                                               0, -1, 0, 0, 1, 1, 1, 0);
                                    break;
                                }
                            }
                        }
                    }
                    statusRedraw[kRedrawShield] = 1;
                    ResetPlayerPrecalcedValues();
                }
            }
        }
    } else {
        sabotageCounter = 0;
    }

    if (QuickKeyCheck(keyPrefs.clearTarget) && (KeyCheck(kScanLAlt) || KeyCheck(kScanRAlt))) {
        keyPressedThisFrame = 1;
        if (!navModeSelectFlag) {
            navModeSelectFlag = 1;
            if (player->targetShip != -1) {
                PlaySound(beepSnd[0], 1, gSoundVolume, gSoundVolume);
                player->targetShip = -1;
                statusRedraw[kRedrawTarg] = 1;
            }
        }
    }
    if (((QuickKeyCheck(keyPrefs.clearTarget) && !KeyCheck(kScanLAlt) && !KeyCheck(kScanRAlt))
            || QuickKeyCheck(keyPrefs.hyperspace))
        && player->jumpTimer <= 0.0f) {
        keyPressedThisFrame = 1;
        if (!navModeSelectFlag) {
            navModeSelectFlag = 1;
            PlaySound(beepSnd[0], 1, gSoundVolume, gSoundVolume);
            statusRedraw[kRedrawNav] = 1;
            dockTarget = -1;
            dockPermission = -1;
            if (QuickKeyCheck(keyPrefs.clearTarget)) {
                player->navTargetKind = -1;
                player->targetStellar = -1;
                statusRedraw[kRedrawNav] = 1;
            }
            if (QuickKeyCheck(keyPrefs.hyperspace)
                    && player->navTargetKind != kNavTargetLink) {
                player->navTargetKind = kNavTargetLink;
                player->targetStellar = -1;
                CueNextHyperRouteDest();
            }
        }
        if (QuickKeyCheck(keyPrefs.hyperspace))
            ShowFloatingMap();
    } else {
        navModeSelectFlag = 0;
    }

    if (QuickKeyCheck(keyPrefs.nextSystem)) {
        keyPressedThisFrame = 1;
        if (!navTargetSelectFlag && player->jumpTimer <= 0.0f) {
            navTargetSelectFlag = 1;
            statusRedraw[kRedrawNav] = 1;
            n = 0;
            for (i = 0; i < kNavSlots; i++) {
                linked[i] = 0;
                if (systemData[player->curSystem].links[i] != -1
                        && FindActiveCoLocatedSystem(
                               systemData[player->curSystem].links[i]) != -1) {
                    linked[i] = 1;
                    n++;
                }
            }
            if (n > 0) {
                PlaySound(beepSnd[2], 1, gSoundVolume, gSoundVolume);
                if (player->navTargetKind != kNavTargetLink)
                    player->targetStellar = -1;
                player->navTargetKind = kNavTargetLink;
                do {
                    player->targetStellar += 1;
                    if (player->targetStellar >= kNavSlots)
                        player->targetStellar = 0;
                    if (player->targetStellar < 0)
                        player->targetStellar = kNavSlots - 1;
                } while (!linked[player->targetStellar]);
            } else {
                player->targetStellar = -1;
            }
        }
        if (player->navTargetKind == kNavTargetLink)
            ShowFloatingMap();
    } else {
        navTargetSelectFlag = 0;
    }


    if (player->jumpTimer <= 0.0f) {
        j = -1;
        if (QuickKeyCheck(kScan1))
            j = 0;
        if (QuickKeyCheck(kScan2))
            j = 1;
        if (QuickKeyCheck(kScan3))
            j = 2;
        if (QuickKeyCheck(kScan4))
            j = 3;
        for (i = 0; i <= 3; i++) {
            if (QuickKeyCheck(keyPrefs.navPreset[i]))
                j = i;
        }
        if (QuickKeyCheck(kScanF5)
                || (CmdKeyQuick() && QuickKeyCheck(kScan5)))
            j = 0x10;
        if (j != -1) {
            keyPressedThisFrame = 1;
            if (!navDefaultSelectFlag) {
                navDefaultSelectFlag = 1;
                if (j != 0x10)
                    n = systemData[player->curSystem].navDefault[j];
                else
                    n = FindNearestStellar(shipData);
                if (n != -1
                        && stellarData[n].system == player->curSystem) {
                    PlaySound(beepSnd[0], 1, gSoundVolume, gSoundVolume);
                    if (player->navTargetKind != kNavTargetStellar || player->targetStellar != n) {
                        navTargetExpand = 256.0f;
                        player->targetStellar = n;
                        statusRedraw[kRedrawNav] = 1;
                        player->navTargetKind = kNavTargetStellar;
                        dockTarget = -1;
                        dockPermission = -1;
                    }
                }
            }
        } else {
            navDefaultSelectFlag = 0;
        }
    }

    if (QuickKeyCheck(keyPrefs.targetNext)) {
        keyPressedThisFrame = 1;
        if (!weapTargetSelectFlag) {
            weapTargetSelectFlag = 1;
            statusRedraw[kRedrawTarg] = 1;
            if (player->unk6e == 1) {
                PlaySound(beepSnd[2], 1, gSoundVolume, gSoundVolume);
                if (ShiftKeyQuick())
                    n = FindPrevShipInSystem(player->targetShip, player->curSystem);
                else
                    n = FindNextShipInSystem(player->targetShip, player->curSystem);
                if (n != player->targetShip && n != player->index) {
                    player->targetShip = n;
                    if (SWIsSpriteInRect(shipData[n].sprite,
                                         SWWindowRect(gSpriteWorld)))
                        weapTargetExpand = 256.0f;
                    else
                        weapTargetExpand = 0;
                } else {
                    player->targetShip = -1;
                }
            }
        }
    } else if (Button()) {
        if (!weapTargetSelectFlag) {
            weapTargetSelectFlag = 1;
            n = player->targetShip;
            SWSetPortToWindow(gSpriteWorld);
            GetGlobalMouse(&pt);
            View::ToView(&pt);
            SetRect(&box, 0, 0, View::width, View::height);
            if (escortMenuOnscreen > 0
                    && PtInRectPoint(pt, &escortMenuRect)) {
                Rect menu = View::OverlayLocalRect(escortMenuRect);
                Point at = View::ToOverlay(pt, escortMenuRect);
                for (i = 0; i < 5; i++) {
                    hit.left = menu.left + 4;
                    hit.right = menu.right - 4;
                    hit.top = menu.top + 0x1e + i * 0x12;
                    hit.bottom = menu.top + 0x2b + i * 0x12;
                    if (PtInRectPoint(at, &hit)) {
                        if (selEscortGroup != i - 1) {
                            selEscortGroup = i - 1;
                            ShowEscortMenu();
                            lastEscortMenuTime = ReadTick();
                        }
                        break;
                    }
                }
            } else if (showFloatingMap > 0 && player->jumpTimer <= 0.0f
                    && PtInRectPoint(pt, &floatingMapRect)) {
                Rect map = View::OverlayLocalRect(floatingMapRect);
                Point at = View::ToOverlay(pt, floatingMapRect);
                hit.left = hit.right =
                    (map.left + map.right) / 2;
                hit.top = hit.bottom =
                    (map.top + map.bottom) / 2;
                InsetRect(&hit, -8, -8);
                if (PtInRectPoint(at, &hit)) {
                    player->navTargetKind = kNavTargetLink;
                    player->targetStellar = -1;
                    statusRedraw[kRedrawNav] = 1;
                    ShowFloatingMap();
                    PlaySound(beepSnd[2], 1, gSoundVolume,
                              gSoundVolume);
                } else {
                    for (mapLink = 0; mapLink < kNavSlots; mapLink++) {
                        if (systemData[player->curSystem].links[mapLink] != -1
                                && FindActiveCoLocatedSystem(
                                       systemData[player->curSystem].links[mapLink])
                                   != -1) {
                            hit.left = hit.right =
                                (map.left + map.right) / 2
                                + (systemData[systemData[player->curSystem].links[mapLink]].pos.h
                                   - systemData[player->curSystem].pos.h)
                                  / floatingMapZoom;
                            hit.top = hit.bottom =
                                (map.top + map.bottom) / 2
                                + (systemData[systemData[player->curSystem].links[mapLink]].pos.v
                                   - systemData[player->curSystem].pos.v)
                                  / floatingMapZoom;
                            if (floatingMapZoom <= 0.5)
                                InsetRect(&hit, -12, -12);
                            else
                                InsetRect(&hit, -8, -8);
                            if (PtInRectPoint(at, &hit)) {
                                player->targetStellar = mapLink;
                                player->navTargetKind = kNavTargetLink;
                                statusRedraw[kRedrawNav] = 1;
                                ShowFloatingMap();
                                PlaySound(beepSnd[2], 1, gSoundVolume,
                                          gSoundVolume);
                                break;
                            }
                        }
                    }
                }
            } else if (PtInRectPoint(pt, &box)) {
                targetUncloaked = shipData->CanTargetUntargetableShips();
                targetCloaked = shipData->CanTargetCloakedShips();
                if (player->active && player->sprite != NULL) {
                    box = ((SpriteRec *)player->sprite)->destFrameRect;
                    if (box.right - box.left < 0x30)
                        InsetRect(&box, -0x10, -0x10);
                    if (PtInRectPoint(pt, &box))
                        player->targetShip = -1;
                }
                for (i = 1; i < kMaxShips; i++) {
                    if (shipData[i].active
                            && !shipData[i].IsDying()
                            && (!shipData[i].IsCloaked() || targetCloaked
                                || shipData[i].parentShip == 0)
                            && shipData[i].curSystem == player->curSystem
                            && (targetUncloaked
                                || !(shipClass[shipData[i].type].flags2
                                     & kShipUntargetable))
                            && shipData[i].sprite != NULL) {
                        box = ((SpriteRec *)shipData[i].sprite)->destFrameRect;
                        if (box.right - box.left < 0x30)
                            InsetRect(&box, -0x10, -0x10);
                        if (PtInRectPoint(pt, &box)) {
                            player->targetShip = i;
                            break;
                        }
                    }
                }
                if (player->targetShip != n) {
                    if (player->unk6e == 1) {
                        statusRedraw[kRedrawTarg] = 1;
                        if (player->targetShip != -1)
                            PlaySound(beepSnd[2], 1, gSoundVolume, gSoundVolume);
                        else
                            PlaySound(beepSnd[0], 1, gSoundVolume, gSoundVolume);
                        if (player->targetShip != -1) {
                            if (SWIsSpriteInRect(shipData[player->targetShip].sprite,
                                    SWWindowRect(gSpriteWorld)))
                                weapTargetExpand = 256.0f;
                            else
                                weapTargetExpand = 0;
                        }
                    }
                } else if (player->jumpTimer <= 0.0f) {
                    for (i = 0; i < kNavSlots; i++) {
                        if (stellarSprite[i] != NULL && stellarSprite[i]->isVisible) {
                            j = stellarSprite[i]->userData;
                            if (stellarData[j].loaded) {
                                box = stellarSprite[i]->destFrameRect;
                                if (box.right - box.left < 0x30)
                                    InsetRect(&box, -0x10, -0x10);
                                if (PtInRectPoint(pt, &box)) {
                                    if (player->targetStellar != j
                                            || player->navTargetKind != kNavTargetStellar) {
                                        PlaySound(beepSnd[0], 1, gSoundVolume,
                                                  gSoundVolume);
                                        player->targetStellar = j;
                                        navTargetExpand = 256.0f;
                                        statusRedraw[kRedrawNav] = 1;
                                        player->navTargetKind = kNavTargetStellar;
                                        dockTarget = -1;
                                        dockPermission = -1;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    } else {
        weapTargetSelectFlag = 0;
    }

    if (QuickKeyCheck(keyPrefs.map)) {
        keyPressedThisFrame = 1;
        if (!mapTryFlag && player->jumpTimer <= 0.0f && shipData->deathTimer <= 0.0f
                && fadePending[0] == 0 && fadePending[1] == 0
                && frameCounter >= 0) {
            PlaySound(beepSnd[2], 1, gSoundVolume, gSoundVolume);
            HideCursorSprite();
            RefreshSAT();
            ShowTheCursor();
            hypergateMapMode = 0;
            DoSystemMap();
            PlaySound(beepSnd[0], 1, gSoundVolume, gSoundVolume);
            SWSetPortToWindow(gSpriteWorld);
            RedrawStatus(1);
            SetAllKeyCmdFlags();
        } else {
            mapTryFlag = 1;
        }
    } else {
        mapTryFlag = 0;
    }

    if (QuickKeyCheck(keyPrefs.land) && player->deathTimer <= 0.0f
            && player->escapePods <= 0 && player->jumpTimer <= 0.0f) {
        keyPressedThisFrame = 1;
        if (player->navTargetKind != kNavTargetStellar || player->targetStellar == -1) {
            n = FindNearestStellar(player);
            if (n != -1) {
                statusRedraw[kRedrawNav] = 1;
                player->navTargetKind = kNavTargetStellar;
                player->targetStellar = n;
                navTargetExpand = 256.0f;
            }
        }
        if (!dockTryFlag && player->navTargetKind == kNavTargetStellar
                && player->targetStellar != -1) {
            dockTryFlag = 1;
            HandlePlayerDockRequest();
        }
    } else {
        dockTryFlag = 0;
    }
    if (dockTarget != -1 && !dockTryFlag)
        HandlePlayerDockApproach();

    if (QuickKeyCheck(keyPrefs.board)) {
        keyPressedThisFrame = 1;
        HandlePlayerBoardAttempt();
    } else {
        boardTryFlag = 0;
    }

    if (QuickKeyCheck(keyPrefs.targetNearest)) {
        keyPressedThisFrame = 1;
        if (!weapCloseTargFlag) {
            if (AltKeyQuick())
                n = FindNearestShipToPlayer();
            else
                n = FindNearestThreatToPlayer();
            weapCloseTargFlag = 1;
            if (n != -1 && player->unk6e == 1
                    && n != player->targetShip) {
                PlaySound(beepSnd[2], 1, gSoundVolume, gSoundVolume);
                statusRedraw[kRedrawTarg] = 1;
                player->unk6e = 1;
                player->targetShip = n;
                if (SWIsSpriteInRect(shipData[n].sprite,
                                     SWWindowRect(gSpriteWorld)))
                    weapTargetExpand = 256.0f;
                else
                    weapTargetExpand = 0;
            } else if (n == -1) {
                PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);
            }
        }
    } else {
        weapCloseTargFlag = 0;
    }

    if (player->jumpTimer <= 0.0f && QuickKeyCheck(keyPrefs.firePrimary)
            && player->aiTimer <= 0.0f && !disabled) {
        keyPressedThisFrame = 1;
        for (i = 0; i < kMaxWeapTypes; i++) {
            if (player->weap[i].count > 0
                    && !(weaponStats[i].flags & kWeapSecondary))
                FirePlayerWeapon(i);
        }
    }
    if (player->jumpTimer <= 0.0f && QuickKeyCheck(keyPrefs.fireSecondary)
            && player->selectedWeapon != -1 && player->aiTimer <= 0.0f && !disabled) {
        keyPressedThisFrame = 1;
        FirePlayerWeapon(player->selectedWeapon);
    }
    if (player->selectedWeapon != -1
            && (weaponStats[player->selectedWeapon].flags & kWeapPersistentSmoke) /* BUG: Flags, not Flags2 */
            && !WeaponHasAmmo(player, player->selectedWeapon)) {
        player->selectedWeapon = -1;
        statusRedraw[kRedrawWeap] = 1;
    }

    if (QuickKeyCheck(keyPrefs.selectSecondary)) {
        keyPressedThisFrame = 1;
        if (!weapSelectFlag) {
            weapSelectFlag = 1;
            n = 0;
            for (i = 0; i < kMaxWeapTypes; i++) {
                if (player->weap[i].count > 0
                        && (weaponStats[i].flags & kWeapSecondary)
                        && (weaponStats[i].guidance < kGuidancePointDef
                            || weaponStats[i].guidance == kGuidanceBay)) {
                    if (weaponStats[i].flags2 & kWeapHideIfNoAmmo) {
                        if (WeaponHasAmmo(player, i))
                            n++;
                    } else {
                        n++;
                    }
                }
            }
            if (n > 0) {
                statusRedraw[kRedrawWeap] = 1;
                PlaySound(beepSnd[2], 1, gSoundVolume, gSoundVolume);
                if (AltKeyQuick())
                    j = -1;
                else
                    j = 1;
                ok = 0;
                while (!ok) {
                    player->selectedWeapon += j;
                    while (player->selectedWeapon >= kMaxWeapTypes)
                        player->selectedWeapon -= kMaxWeapTypes;
                    while (player->selectedWeapon < 0)
                        player->selectedWeapon += kMaxWeapTypes;
                    if (player->selectedWeapon >= 0 && player->selectedWeapon < kMaxWeapTypes) {
                        if (player->weap[player->selectedWeapon].count > 0
                                && (weaponStats[player->selectedWeapon].flags
                                    & kWeapSecondary)
                                && (weaponStats[player->selectedWeapon].guidance < kGuidancePointDef
                                    || weaponStats[player->selectedWeapon].guidance
                                       == kGuidanceBay)) {
                            if (weaponStats[player->selectedWeapon].flags2 & kWeapHideIfNoAmmo) {
                                if (WeaponHasAmmo(player, player->selectedWeapon))
                                    ok = 1;
                            } else
                                ok = 1;
                        }
                    } else {
                        ok = 0;
                    }
                }
            } else {
                PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);
            }
        }
    } else {
        weapSelectFlag = 0;
    }

    if (QuickKeyCheck(keyPrefs.deselectSecondary) && player->selectedWeapon != -1) {
        keyPressedThisFrame = 1;
        player->selectedWeapon = -1;
        statusRedraw[kRedrawWeap] = 1;
        PlaySound(beepSnd[2], 1, gSoundVolume, gSoundVolume);
    }

    for (i = 0; i < kMaxWeapTypes; i++) {
        if (player->weap[i].count > 0) {
            if (player->weap[i].reloadTimer > 0.0f)
                player->weap[i].reloadTimer -= gSpeedMult;
            else
                player->weap[i].reloadTimer = 0;
            if ((weaponStats[i].seeker & kWeapSeekNoFireIonized)
                    && player->IonizationFactor() > 0.0f)
                player->weap[i].reloadTimer = 1.0f;
        }
    }

    if (QuickKeyCheck(keyPrefs.faceTarget) && player->jumpTimer <= 0.0f) {
        keyPressedThisFrame = 1;
        if (player->targetShip != -1) {
            if (player->targetStellar == -1) {
                player->desiredHeading =
                    CalcInterceptAngle(&player->pos,
                        &shipData[player->targetShip].pos);
            } else if (AltKeyQuick()) {
                origin = XYShortToFloat(
                    stellarData[player->targetStellar].pos);
                player->desiredHeading =
                    CalcInterceptAngle(&player->pos, &origin);
            } else {
                player->desiredHeading =
                    CalcInterceptAngle(&player->pos,
                        &shipData[player->targetShip].pos);
            }
            turned = 1;
        } else if (player->targetStellar != -1) {
            origin = XYShortToFloat(
                stellarData[player->targetStellar].pos);
            player->desiredHeading = CalcInterceptAngle(&player->pos, &origin);
            turned = 1;
        }
    }
    if (player->navTargetKind == kNavTargetLink) {
        dockTarget = -1;
        dockPermission = -1;
        if (player->targetStellar != -1 && player->jumpTimer > 0.0f
                && (player->CanExpediteJumps()
                    || (abs((short)player->vel.x) < 2
                        && abs((short)player->vel.y) < 2))) {
            pos.x = systemData[player->curSystem].pos.h;
            pos.y = systemData[player->curSystem].pos.v;
            j = systemData[player->curSystem].links[player->targetStellar];
            dest.x = systemData[j].pos.h;
            dest.y = systemData[j].pos.v;
            player->desiredHeading = CalcInterceptAngle(&pos, &dest);
            turned = 1;
        }
    }

    jumping = 0;
    if ((QuickKeyCheck(keyPrefs.jump) && !player->IsDisabled())
            || player->jumpTimer > 0.0f) {
        keyPressedThisFrame = 1;
        if (player->navTargetKind == kNavTargetLink
                && player->targetStellar != -1
                && -999.0f != player->jumpTimer
                && player->fuel >= 100.0f) {
            canJump = 1;
            for (i = 0; i < kNavSlots; i++) {
                if (systemData[player->curSystem].navDefault[i] != -1
                        && !(stellarData[systemData[player->curSystem].navDefault[i]].flags2
                             & kSpobGateOrWormhole)) {
                    origin.x = origin.y = 0;
                    turnLim = player->HyperSafeDist();
                    if (FloatAbs(XYFloatSquaredDist(&origin,
                                &player->pos)) <= turnLim)
                        canJump = 0;
                }
            }
            if (player->tractorShip != -1 && player->tractorShip != player->index)
                canJump = 0;
            if (canJump || player->jumpTimer > 0.0f) {
                if (commMsg[0] > 0) {
                    GetIndString(msg, kGameStrList, 0x2a);
                    if (pstrcmp(msg, (unsigned char *)commMsg) == 0)
                        DisplayComm((PStrChar *)"\p", 1);
                    GetIndString(msg, kGameStrList, 0x24);
                    if (pstrcmp(msg, (unsigned char *)commMsg) == 0)
                        DisplayComm((PStrChar *)"\p", 1);
                    GetIndString(msg, kGameStrList, 0x1e);
                    if (pstrcmp(msg, (unsigned char *)commMsg) == 0)
                        DisplayComm((PStrChar *)"\p", 1);
                    GetIndString(msg, kGameStrList, 0x1f);
                    if (pstrcmp(msg, (unsigned char *)commMsg) == 0)
                        DisplayComm((PStrChar *)"\p", 1);
                }
                if (canJump && player->jumpTimer <= 0.0f) {
                    waiting = 0;
                    n = -1;
                    for (i = 1; i < kMaxShips; i++) {
                        if (shipData[i].active
                                && shipData[i].parentShip == 0
                                && shipData[i].aiType == kShipTypeFighter
                                && !AIShipHasFuelForJump(&shipData[i])
                                && !(shipClass[shipData[i].type].flags2
                                     & kShipNoFighterVoice)) {
                            waiting = waiting + 1;
                            n = shipData[i].type;
                        }
                    }
                    if (waiting > 0) {
                        pstrcpy(msg, shipClass[n].name);
                        PtoCstr(msg);
                        strcat((char *)msg, ":  ");
                        if (waiting == 1)
                            StrcatIndCString((char *)msg, kGameStrList,
                                            Rand(2) + 0xa0);
                        else
                            StrcatIndCString((char *)msg, kGameStrList,
                                            Rand(2) + 0xa2);
                        CtoPstr((char *)msg);
                        DisplayComm((PStrChar *)msg, 0xfa);
                        PlaySound(beepSnd[4], 0x32, gSoundVolume, gSoundVolume);
                    }
                }
                if (player->CanExpediteJumps()
                        || (abs((short)player->vel.x) < 2
                            && abs((short)player->vel.y) < 2)) {
                    jumping = 1;
                    if ((shipClass[player->type].shanFlags & kShanFoldingParts)
                            && !(shipClass[player->type].shanFlags & kShanUnfoldsWhenFiring)
                            && player->foldSet > 0)
                        player->foldDir = -1;
                    if (player->jumpTimer <= 1.0f) {
                        player->jumpTimer = 2.0f;
                        player->jumpTick = ReadTick();
                        statusRedraw[kRedrawNav] = 1;
                        newbieFlag = 0x7fff;
                        if (doubleTime) {
                            if (!ST_IsSoundPlaying(warpUpSnd[1])) {
                                warpUpSoundSpec.sound = warpUpSnd[1];
                                AdjustWarpSoundSpeed(
                                    shipClass[player->type].mobilityFactor);
                                ST_PlaySoundParam(&warpUpSoundSpec);
                                warpSoundDone = 0;
                            }
                        } else if (!ST_IsSoundPlaying(warpUpSnd[0])) {
                            warpUpSoundSpec.sound = warpUpSnd[0];
                            AdjustWarpSoundSpeed(
                                shipClass[player->type].mobilityFactor);
                            ST_PlaySoundParam(&warpUpSoundSpec);
                            warpSoundDone = 0;
                        }
                    }
                } else {
                    player->jumpTick = ReadTick();
                    player->jumpTimer = 1.0f;
                    if (player->IsInertialess()) {
                        player->accelMag = player->accelMag
                            - player->AccelRate() * gSpeedMult;
                        if (player->accelMag < 0.0f)
                            player->accelMag = 0;
                    } else {
                        pos.x = pos.y = 0;
                        dest.x = player->vel.x * 100.0;
                        dest.y = player->vel.y * 100.0;
                        player->desiredHeading =
                            CalcInterceptAngle(&dest, &pos);
                        turned = 1;
                        if (abs(AngularDifference(
                                 player->desiredHeading,
                                 player->heading))
                                  < (player->TurnRate() + 1.0f > 20.0f
                                     ? player->TurnRate() + 1.0f : 20.0f)) {
                            Accel(player->heading, player->AccelRate(),
                                  &player->vel);
                            if (player->engineGlow < 0x18)
                                player->engineGlow += 3;
                        }
                        player->vel.x *= 0.992;
                        player->vel.y *= 0.992;
                    }
                }
            } else {
                if (!jumpTryFlag) {
                    PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);
                    if (player->tractorShip != -1
                            && player->tractorShip != player->index)
                        GetIndString(msg, kGameStrList, 0x24);
                    else
                        GetIndString(msg, kGameStrList, 0x2a);
                    DisplayComm((PStrChar *)msg, 0x168);
                } else {
                    player->jumpTimer = 0;
                }
            }
        } else if (!jumpTryFlag) {
            if (newbieFlag <= 2 && !disabled && newbieString[7][0] != 0) {
                if (player->navTargetKind != kNavTargetLink
                        || player->targetStellar == -1) {
                    PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);
                    DisplayComm((PStrChar *)newbieString[7], 0x168);
                }
            } else if (player->navTargetKind == kNavTargetLink
                    && player->targetStellar != -1
                    && -999.0f != player->jumpTimer
                    && player->fuel < 100.0f) {
                PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);
                DisplayComm((PStrChar *)fuelString[7], kCommFrames);
            }
        }
        jumpTryFlag = 1;
    } else {
        jumpTryFlag = 0;
    }

    if ((jumping || player->jumpTimer > 1.0f) && !disabled) {
        MakeEscortsHyperWithParent(player);
        player->jumpTimer = player->jumpTimer + gSpeedMult;
        if (!player->CanExpediteJumps()) {
            player->vel.x *= 0.98;
            player->vel.y *= 0.98;
        }
        if (player->jumpTimer > 30.0f) {
            if (!ST_IsSoundPlaying(warpUpSnd[0])
                    && !ST_IsSoundPlaying(warpUpSnd[1]))
                warpSoundDone = 1;
        }
        if (player->jumpTimer >= 30.0f && warpSoundDone) {
            warpSoundDone = 0;
            if (!ST_IsSoundPlaying(warpOutSnd))
                PlaySound(warpOutSnd, 0x32, gSoundVolume, gSoundVolume);
            player->pos.x = player->pos.y = 0;
            pos.x = systemData[systemData[player->curSystem]
                         .links[player->targetStellar]].pos.h;
            pos.y = systemData[systemData[player->curSystem]
                         .links[player->targetStellar]].pos.v;
            dest.x = systemData[player->curSystem].pos.h;
            dest.y = systemData[player->curSystem].pos.v;
            Accel(((CalcInterceptAngle(&dest, &pos) + kHalfCircle) % kFullCircle),
                  1350.0f, &player->pos);
            player->vel.x = player->vel.y = 0;
            if (player->IsInertialess())
                player->accelMag = player->MaxSpeed();
            else
                Accel(player->heading, player->MaxSpeed(),
                      &player->vel);
            sys = systemData[player->curSystem].links[player->targetStellar];
            player->prevSystem = player->curSystem;
            routeDone = 0;
            if (hyperRoute[1] != -1
                    && FindActiveCoLocatedSystem(sys)
                       == FindActiveCoLocatedSystem(hyperRoute[1])
                    && (count = k = player->JumpsPerJump()) > 1) {
                for (i = 1; i < count; i++) {
                    sys = FindActiveCoLocatedSystem(hyperRoute[1]);
                    if (sys == -1)
                        break;
                    player->curSystem = sys;
                    ExploreNebulaeFromSystem(sys);
                    SpoolNextHyperRouteDest();
                    if (hyperRoute[1] == -1)
                        break;
                }
                routeDone = 1;
            }
            player->curSystem = sys;
            ExploreNebulaeFromSystem(player->curSystem);
            player->navTargetKind = -1;
            player->targetStellar = -1;
            player->targetShip = -1;
            player->fuel = player->fuel - 100.0f;
            View::SetPortToView();
            CE_SetFlashColor(whiteColor);
            PaintRect(&gSpriteWorld->workFrameP->frameRect);
            ForeColor(blackColor);
            Screen::ComposeView();
            ScreenEndScene();
            SWSetPortToWorkArea(gSpriteWorld);
            BlitPixieRectDrawProc(gSpriteWorld->workFrameP, SWWindowFrame(gSpriteWorld),
                         &gSpriteWorld->workFrameP->frameRect,
                         &SWWindowFrame(gSpriteWorld)->frameRect);
            PropagateMissionBitEffects();
            if (systemData[shipData->curSystem].murk < 0)
                ClearStarfield();
            else
                RandomizeStars();
            ScrollStarfield(0.0f, 0.0f);
            EraseParticlesOffscreen(gSpriteWorld);
            ClearParticles(1);
            statusRedraw[kRedrawNav] = 1;
            statusRedraw[kRedrawTarg] = 1;
            statusRedraw[kRedrawCargo] = 1;
            statusRedraw[kRedrawWeap] = 1;
            statusRedraw[kRedrawFuel] = 1;
            dockTarget = -1;
            dockPermission = -1;
            lastMissionStellar = -1;
            bribeRandom = -1;
            adviceRandom = Rand(0x800);
            weapLawFlag = 0;
            smuggleFlag = 0;
            newbieFlag = 0x7fff;
            antiEscortDelay = Rand(0x1e) + 0x1e;
            SWSetSpriteVisible(alertSprite, 0);
            alertSpritePending = 0;
            for (i = 0; i < kMaxMissions; i++)
                missionRandom[i] = Rand(kDailyRollRange) + 1;
            mapScroll.h = systemData[player->curSystem].pos.h;
            mapScroll.v = systemData[player->curSystem].pos.v;
            days = player->HyperTransitTime();
            lost = 0;
            for (i = 1; i < kMaxShips; i++) {
                if (shipData[i].active && shipData[i].parentShip == 0
                        && !shipData[i].IsDisabled()) {
                    if (shipData[i].aiType == kShipTypeFighter
                            && !AIShipHasFuelForJump(&shipData[i])) {
                        shipData[i].active = 0;
                        lost++;
                    }
                    if (shipData[i].active) {
                        rank = shipData[i].HyperTransitTime();
                        if (rank > days)
                            days = rank;
                    }
                }
            }
            for (i = 0; i < days; i++)
                IncrementGameTime();
            HandleGlobalPriceFlux();
            HandleLocalPriceFlux();
            MissionHandlePlayerEnteredNewSystem();
            j = FindFirstCoLocatedSystem(player->curSystem);
            if (systemData[j].exploreLevel < 1)
                systemData[j].exploreLevel = 1;
            if (systemData[player->curSystem].exploreLevel < 1)
                systemData[player->curSystem].exploreLevel = 1;
            AutoSetExploration(player->curSystem, 0, 1);
            player->jumpTimer = -999.0f;
            SetupShipsInSystem(player->curSystem, 0);
            SpawnBountyHunter();
            HandleStellarSystemVisibility();
            player->jumpTimer = 0;
            crashlog_print("entering system ");
            crashlog_print_num(j + kFirstResourceID);
            crashlog_print(" (");
            crashlog_print_pstr(systemData[j].name);
            crashlog_print(")\n");
            if (systemData[j].message != -1) {
                DisplayMessage(systemData[j].message);
            } else {
                n = Rand(3);
                strcpy((char *)msg, "");
                StrcatIndCString((char *)msg, kGameStrList, n + 0x2b);
                strcat((char *)msg, " ");
                pstrcpy(name, systemData[j].name);
                PtoCstr(name);
                strcat((char *)msg, (char *)name);
                strcat((char *)msg, " ");
                StrcatIndCString((char *)msg, kGameStrList, 0x30);
                strcat((char *)msg, " ");
                DateToLongString(gameTime, (char *)name);
                strcat((char *)msg, (char *)name);
                strcat((char *)msg, ".");
                n = 0;
                for (i = 0; i < kNavSlots; i++) {
                    if (systemData[player->curSystem].navDefault[i] != -1)
                        n++;
                }
                if (n == 0) {
                    strcat((char *)msg, " ");
                    StrcatIndCString((char *)msg, kGameStrList, 0x31);
                }
                if (lost > 0) {
                    strcat((char *)msg, "  (");
                    StrcatOrdinalNumber(lost, (char *)msg, 1);
                    strcat((char *)msg, " ");
                    if (lost == 1)
                        StrcatIndCString((char *)msg, kGameStrList, 0xa4);
                    else
                        StrcatIndCString((char *)msg, kGameStrList, 0xa5);
                    strcat((char *)msg, ")");
                }
                CtoPstr((char *)msg);
                DisplayComm((PStrChar *)msg, kCommFrames);
            }
            if (hyperRoute[1] != -1 && !routeDone) {
                j = FindActiveCoLocatedSystem(hyperRoute[1]);
                if (j != FindActiveCoLocatedSystem(shipData->curSystem)) {
                    for (i = 0; i < kMaxHyperRoute; i++)
                        hyperRoute[i] = -1;
                }
            }
            SpoolNextHyperRouteDest();
            CueNextHyperRouteDest();
            DrawMapInFloatingFrame();
            killShots = 1;
            killBoxes = 1;
            killExplods = 1;
            killSmoke = 1;
            missionSoundID = -1;
            escortMenuOnscreen = 0;
            CreateAsteroids();
            ResetPlayerPrecalcedValues();
            if (shipClass[player->type].shanFlags & kShanUnfoldsWhenFiring) {
                player->foldSet =
                    shipClass[player->type].baseSets - 1;
                player->foldDir = 0;
            } else {
                player->foldSet = 0;
                player->foldDir = 1;
            }
            player->frameTimer = 0;
            player->shieldGlow = 0;
            player->weapGlow = 0;
            SWHideAllSprites(gSpriteWorld);
            HandleStellarSystemVisibility();
            PrepareAllSprites();
            SWSetPortToWorkArea(gSpriteWorld);
            SetSystemBackgroundColor();
            PaintRect(&gSpriteWorld->backFrameP->frameRect);
            SWSetPortToWorkArea(gSpriteWorld);
            SetSystemBackgroundColor();
            PaintRect(&gSpriteWorld->workFrameP->frameRect);
            SWSetPortToWindow(gSpriteWorld);
            RefreshSAT();
            DoEscortPayment(days);
            return;
        } else if (player->jumpTimer > 30.0f) {
            if ((ReadTick() - player->jumpTick)
                    > WarpSoundDuration()
                      / shipClass[player->type].mobilityFactor) {
                if (ST_IsSoundPlaying(warpUpSnd[0]))
                    ST_HaltSound(warpUpSnd[0]);
                if (ST_IsSoundPlaying(warpUpSnd[1]))
                    ST_HaltSound(warpUpSnd[1]);
                if (!warpSoundDone) {
                    warpSoundDone = 1;
                    PlaySound(warpOutSnd, 0x32, gSoundVolume, gSoundVolume);
                }
            }
        }
    } else if (-999.0f == player->jumpTimer) {
        player->jumpTimer = 0;
    }

    if (QuickKeyCheck(keyPrefs.reverse) && player->jumpTimer <= 0.0f && !disabled) {
        keyPressedThisFrame = 1;
        if (player->IsInertialess()) {
            player->accelMag = player->accelMag
                               - player->AccelRate() * gSpeedMult;
            if (player->accelMag < 0.0)
                player->accelMag = 0;
        } else if (FloatAbs(player->vel.x) >= 0.05 || FloatAbs(player->vel.y) >= 0.05) {
            pos.x = pos.y = 0;
            dest.x = player->vel.x * 100.0;
            dest.y = player->vel.y * 100.0;
            player->desiredHeading =
                (CalcInterceptAngle(&pos, &dest) + kHalfCircle) % kFullCircle;
            turned = 1;
        }
    }

    turnStep = player->TurnRate();
    dir = 0;
    if (turned && abs(AngularDifference(player->desiredHeading, player->heading)) > turnStep * gSpeedMult
            && !disabled) {
        angle = player->desiredHeading - player->heading;
        while (angle >= kFullCircleF)
            angle = angle - kFullCircleF;
        while (angle < 0.0f)
            angle += kFullCircleF;
        if (angle > kHalfTurn) {
            player->heading -= (float)turnStep * gSpeedMult;
            dir = -1;
        } else {
            player->heading += (float)turnStep * gSpeedMult;
            dir = 1;
        }
    } else if (player->jumpTimer <= 0.0f && !disabled && !turned
            && (QuickKeyCheck(keyPrefs.turnLeft) || QuickKeyCheck(keyPrefs.turnRight))) {
        keyPressedThisFrame = 1;
        if (QuickKeyCheck(keyPrefs.turnLeft) && !QuickKeyCheck(keyPrefs.turnRight)) {
            player->heading = player->heading - (float)turnStep * gSpeedMult;
            dir = -1;
        } else if (QuickKeyCheck(keyPrefs.turnRight)
                && !QuickKeyCheck(keyPrefs.turnLeft)) {
            player->heading = player->heading + (float)turnStep * gSpeedMult;
            dir = 1;
        }
    }
    if (player->heading >= 360.0)
        player->heading -= 360.0;
    if (player->heading < 0.0)
        player->heading += 360.0;

    abOn = 0;
    if (QuickKeyCheck(keyPrefs.afterburner) && !disabled) {
        keyPressedThisFrame = 1;
        if (player->HasAfterburner() && player->jumpTimer <= 0.0f
                && player->fuel > 0.0f && player->aiTimer <= 0.0f
                && player->fuel >= player->AfterburnerFuelUsage())
            abOn = 1;
    }

    if (player->aiTimer <= 0.0f && QuickKeyCheck(keyPrefs.accelerate)
            && player->jumpTimer <= 0.0f && !disabled) {
        keyPressedThisFrame = 1;
        if (player->IsInertialess()) {
            player->accelMag = player->accelMag + player->AccelRate() * gSpeedMult;
            if (player->accelMag > player->MaxSpeed())
                player->accelMag = player->MaxSpeed();
        } else if (abOn && !gravitySourcesPresent) {
            AdjustedAccel(player->heading,
                          player->AccelRate() * gSpeedMult,
                          &player->vel,
                          player->MaxSpeed() * 1.8);
        } else {
            AdjustedAccel(player->heading,
                          player->AccelRate() * gSpeedMult,
                          &player->vel, player->MaxSpeed());
        }
        if (player->engineGlow < 0x18)
            player->engineGlow += 1;
    } else if (player->engineGlow > 0) {
        player->engineGlow -= 1;
    }

    player->bankDir = 0;
    if (shipClass[player->type].shanFlags & kShanBanks) {
        if (dir != 0) {
            if (dir > 0 && player->frameTimer < 16.0f)
                player->frameTimer += gSpeedMult;
            if (dir < 0 && player->frameTimer > -16.0f)
                player->frameTimer -= gSpeedMult;
        } else {
            if (player->frameTimer >= gSpeedMult * 2.0)
                player->frameTimer -= gSpeedMult * 2.0;
            else if (player->frameTimer <= -gSpeedMult * 2.0)
                player->frameTimer += gSpeedMult * 2.0;
            else
                player->frameTimer = 0;
        }
        if ((dir > 0 || turned) && player->frameTimer > 6.0f)
            player->bankDir = 1;
        else if ((dir < 0 || turned) && player->frameTimer < -6.0f)
            player->bankDir = -1;
        else
            player->bankDir = 0;
        if (player->bankDir != 0
                && (shipClass[player->type].shanFlags & kShanFoldingParts)
                && player->engineGlow < 0x18)
            player->engineGlow += 2;
    }

    if (player->aiTimer > 0.0f || player->jumpTimer > 0.0f) {
        if ((shipClass[player->type].shanFlags & kShanBanks)
                && player->jumpTimer > 30.0f)
            player->frameTimer = 0;
        if (player->engineGlow > 0)
            player->engineGlow -= 1;
    }

    if (player->shield < player->ShieldCapacity() && !dying && !disabled) {
        player->shield = player->shield + player->ShieldRechargeRate() * gSpeedMult;
        statusRedraw[kRedrawShield] = 1;
    }
    if (player->armor < player->ArmorCapacity() && !dying
            && !player->IsDisabled() && playerRepairInhibit < 0.0) {
        player->armor = player->armor + player->ArmorRechargeRate() * gSpeedMult;
        statusRedraw[kRedrawShield] = 1;
    }

    if (player->jumpTimer > 1.0f && !disabled) {
        pos.x = systemData[player->curSystem].pos.h;
        pos.y = systemData[player->curSystem].pos.v;
        n = systemData[player->curSystem].links[player->targetStellar];
        dest.x = systemData[n].pos.h;
        dest.y = systemData[n].pos.v;
        n = CalcInterceptAngle(&pos, &dest);
        if (abs(AngularDifference(player->heading, n))
                  <= (player->TurnRate() > 30.0f ? player->TurnRate() : 30.0f)) {
            span = (ReadTick() - player->jumpTick)
                   * shipClass[player->type].mobilityFactor / (WarpSoundDuration() * 0.01);
            step = span;
            if (shipClass[player->type].mobilityFactor != 0) {
                step = step - (doubleTime ? 1.5 : 1.0) * 35.0
                       / shipClass[player->type].mobilityFactor;
            }
            if (step > 0.0) {
                if (settings.noHyperGamma == 0)
                    hyperGamma = 5.0f * (step - 55.0f);   /* 1.1.1 */
                if (step > 50.0f)
                    step = 50.0f;
                Accel(player->heading, step * gSpeedMult,
                      &player->pos);
                if (player->engineGlow < 0x20) {
                    player->engineGlow += 4;
                    if (player->engineGlow > 0x20)
                        player->engineGlow = 0x20;
                }
            }
        }
    }

    if (player->IsInertialess()) {
        if (player->accelMag > playerMaxVel.x)
            player->accelMag = playerMaxVel.x;
        if (disabled) {
            player->accelMag *= 0.985;
            if (player->engineGlow > 0)
                player->engineGlow -= 1;
        } else {
            n = (player->accelMag * 32.0f * 0.75
                                  / player->MaxSpeed());
            if (n > 0x18)
                n = 0x18;
            if (player->engineGlow < n)
                player->engineGlow += 1;
            else
                player->engineGlow -= 1;
        }
        AdjustInertialessShipVelocity(player);
    }

    if (player->vel.x > playerMaxVel.x)
        player->vel.x = playerMaxVel.x;
    if (player->vel.x < -playerMaxVel.x)
        player->vel.x = -playerMaxVel.x;
    if (player->vel.y > playerMaxVel.y)
        player->vel.y = playerMaxVel.y;
    if (player->vel.y < -playerMaxVel.y)
        player->vel.y = -playerMaxVel.y;
    player->pos.x += player->vel.x * gSpeedMult;
    player->pos.y += player->vel.y * gSpeedMult;

    if (player->navTargetKind == kNavTargetLink
            && player->targetStellar != -1) {
        canJump = 1;
        for (i = 0; i < kNavSlots; i++) {
            if (systemData[player->curSystem].navDefault[i] != -1
                    && !(stellarData[systemData[player->curSystem].navDefault[i]].flags2
                         & kSpobGateOrWormhole)) {
                origin.x = origin.y = 0;
                turnLim = player->HyperSafeDist();
                if (FloatAbs(XYFloatSquaredDist(&origin,
                            &player->pos)) <= turnLim)
                    canJump = 0;
            }
        }
        if (canJump) {
            if (!lastJumpDistOk && player->jumpTimer <= 0.0f
                    && player->fuel >= kHyperFuelUnit) {
                PlaySound(beepSnd[4], 1, gSoundVolume, gSoundVolume);
                statusRedraw[kRedrawNav] = 1;
                if (commMsg[0] > 0) {
                    GetIndString(msg, kGameStrList, 0x2a);
                    if (pstrcmp(msg, (unsigned char *)commMsg) == 0)
                        DisplayComm((PStrChar *)"\p", 1);
                    GetIndString(msg, kGameStrList, 0x24);
                    if (pstrcmp(msg, (unsigned char *)commMsg) == 0)
                        DisplayComm((PStrChar *)"\p", 1);
                    GetIndString(msg, kGameStrList, 0x1e);
                    if (pstrcmp(msg, (unsigned char *)commMsg) == 0)
                        DisplayComm((PStrChar *)"\p", 1);
                    GetIndString(msg, kGameStrList, 0x1f);
                    if (pstrcmp(msg, (unsigned char *)commMsg) == 0)
                        DisplayComm((PStrChar *)"\p", 1);
                }
            }
            lastJumpDistOk = 1;
        } else {
            lastJumpDistOk = 0;
        }
    }

    if (player->aiTimer > 0.0f)
        player->aiTimer = player->aiTimer - 1.0f;
    if (player->deathTimer > 0.0f)
        player->deathTimer = player->deathTimer - 1.0f;

    if (player->ionization > 0.0f) {
        player->ionization = player->ionization - player->DeionizationRate() * gSpeedMult;
        step = player->IonizationFactor();
        if (step > 0.7)
            step = 0.7f;
        step = player->MaxSpeed() * (1.0f - step);
        if (player->vel.x > step)
            player->vel.x -= gSpeedMult * 0.025;
        if (player->vel.x < -step)
            player->vel.x += gSpeedMult * 0.025;
        if (player->vel.y > step)
            player->vel.y -= gSpeedMult * 0.025;
        if (player->vel.y < -step)
            player->vel.y += gSpeedMult * 0.025;
    } else {
        player->ionization = 0;
    }

    if (player->FuelGenRate(&step)) {
        n = player->fuel;
        player->fuel += step * gSpeedMult;
        if ((short)player->fuel != n)
            statusRedraw[kRedrawFuel] = 1;
    }
    fuelCap = player->FuelCapacity();
    if (player->fuel > fuelCap)
        player->fuel = fuelCap;
    if (player->fuel < 0.0f)
        player->fuel = 0;

    if (CE_CheckDebugKeys(kScanF10) && cheatsActive) {
        PlaySound(beepSnd[1], 1, gSoundVolume, gSoundVolume);
        if (AltKeyQuick()) {
            for (i = 0; i < kMaxSystems; i++)
                systemData[i].exploreLevel = 0;
            for (i = 0; i < kMaxNebulae; i++)
                nebulaData[i].explored = 0;
        } else {
            for (i = 0; i < kMaxSystems; i++)
                systemData[i].exploreLevel = 2;
            for (i = 0; i < kMaxNebulae; i++)
                nebulaData[i].explored = 1;
        }
        HandleStellarSystemVisibility();
        while (KeyCheck(kScanExploreCheat))
            BusyIdle();
    }

    if (QuickKeyCheck(keyPrefs.cheatAmmo) && cheatsActive) {
        PlaySound(beepSnd[0], 1, gSoundVolume, gSoundVolume);
        for (i = 0; i < kMaxWeapTypes; i++) {
            if (shipClass[player->type].weapAmmo[i] > 0) {
                if (weaponStats[i].ammoType >= 0
                        && weaponStats[i].ammoType < kMaxWeapTypes
                        && weaponStats[i].guidance != kGuidanceBay)
                    player->weap[weaponStats[i].ammoType].ammo =
                        shipClass[player->type].weapAmmo[i];
                else
                    player->weap[i].ammo =
                        shipClass[player->type].weapAmmo[i];
            }
        }
        if (AltKeyQuick())
            player->fuel = 0;
        else
            player->fuel = player->FuelCapacity();
        player->shield = player->ShieldCapacity();
        player->armor = player->ArmorCapacity();
        player->ionization = 0;
        player->ionColor = 0;
        player->deathTimer = -1.0f;
        shipSystems[106] = 1;
        shipSystems[69] = 1;
        shipSystems[60] = 1;
        shipSystems[143] = 1;
        shipSystems[57] = 1;
        ActivateRank((char *)&rankData[19]);
        SystemInfoToShipStats();
        statusRedraw[kRedrawFuel] = 1;
        statusRedraw[kRedrawShield] = 1;
        statusRedraw[kRedrawWeap] = 1;
        ResetPlayerPrecalcedValues();
    }

    if (cheatsActive && QuickKeyCheck(kScanF7) && player->targetShip != -1
            && shipData[player->targetShip].active) {
        if (AltKeyQuick())
            AIMakeShipLeave(&shipData[player->targetShip]);
        else
            AIMakeShipAttackPlayer(&shipData[player->targetShip]);
    }

    escortHit = 0;
    if (QuickKeyCheck(keyPrefs.dismiss)) {
        keyPressedThisFrame = 1;
        if (!ackCommFlag) {
            ackCommFlag = 1;
            if (commCount > 0)
                DisplayComm((PStrChar *)"\p", 1);
            if (showFloatingMap > 0)
                showFloatingMap = 0;
            if (escortMenuOnscreen > 0)
                escortHit = 1;
        }
    } else {
        ackCommFlag = 0;
    }

    if (QuickKeyCheck(keyPrefs.escortMenu)) {
        if (!escortMenuFlag) {
            escortMenuFlag = 1;
            if (escortMenuOnscreen > 0) {
                escortHit = 1;
            } else {
                flag = 0;
                for (i = 1; i < kMaxShips; i++) {
                    if (shipData[i].active && shipData[i].parentShip == 0) {
                        flag = 1;
                        break;
                    }
                }
                if (flag) {
                    PlaySound(beepSnd[0], 1, gSoundVolume, gSoundVolume);
                    selEscortGroup = -1;
                    GetIndString(msg, kGameStrList, 0x33);
                    if (pstrcmp(msg, (unsigned char *)commMsg) == 0)
                        DisplayComm((PStrChar *)"\p", 1);
                    ShowEscortMenu();
                    lastEscortMenuTime = ReadTick();
                } else {
                    GetIndString(msg, kGameStrList, 0x33);
                    DisplayComm((PStrChar *)msg, 0xfa);
                    PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);
                }
            }
        }
    } else {
        escortMenuFlag = 0;
    }

    if (escortMenuOnscreen > 0) {
        count = 0;
        for (j = 1; j < kMaxShips; j++) {
            if (shipData[j].active && shipData[j].parentShip == 0
                    && !shipData[j].IsDisabled() && !shipData[j].IsDying())
                count++;
        }
        if (count != lastEscortCount) {
            lastEscortCount = count;
            if (count == 0) {
                lastEscortMenuTime = 0;
                selEscortGroup = -1;
            }
            DrawEscortMenuInFrame();
        }
        if (ReadTick() > lastEscortMenuTime + 0x1e0) {
            escortMenuOnscreen -= 1;
            if (escortMenuOnscreen == 0)
                escortHit = 1;
        }
        if (escortHit) {
            escortMenuOnscreen = 0;
            if (QuickKeyCheck(keyPrefs.escortMenu))
                PlaySound(beepSnd[0], 1, gSoundVolume, gSoundVolume);
        } else {
            for (k = 0; k < 5; k++) {
                if (RawKeyCheck(keyPrefs.quickKey[k])) {
                    if (!escortGroupFlag[k]) {
                        escortGroupFlag[k] = 1;
                        if (k > 0) {
                            ok = 0;
                            for (j = 1; j < kMaxShips; j++) {
                                if (shipData[j].active
                                        && shipData[j].parentShip == 0
                                        && shipClass[shipData[j].type].escortType
                                           == k - 1)
                                    ok = 1;
                            }
                        } else {
                            ok = 1;
                        }
                        if (ok) {
                            selEscortGroup = k - 1;
                            ShowEscortMenu();
                            lastEscortMenuTime = ReadTick();
                            PlaySound(beepSnd[2], 1, gSoundVolume,
                                      gSoundVolume);
                        }
                        lastEscortMenuTime = ReadTick();
                    }
                } else {
                    escortGroupFlag[k] = 0;
                }
            }
        }
    } else {
        lastEscortCount = -1;
    }

    for (i = 0; i < 4; i++) {
        if (QuickKeyCheck(keyPrefs.escortOrder[i])) {
            if (!escortCommandFlag[i]) {
                escortCommandFlag[i] = 1;
                if (i == 0) {
                    n = kOrderAttack;
                } else if (i == 1) {
                    n = kOrderDefend;
                } else if (i == 2) {
                    n = kOrderHold;
                } else if (i == 3) {
                    if (AltKeyQuick())
                        n = kOrderRegroup;
                    else
                        n = kOrderFormation;
                }
                if (selEscortGroup == -1 || escortMenuOnscreen <= 0) {
                    for (j = 0; j < 4; j++)
                        playerEscortOrders[j] = n;
                } else {
                    playerEscortOrders[selEscortGroup] = n;
                }
                if (escortMenuOnscreen > 0) {
                    if (IssueNewEscortCommand(selEscortGroup, n, 0)) {
                        ShowEscortMenu();
                        lastEscortMenuTime = ReadTick();
                    }
                } else {
                    IssueNewEscortCommand(-1, n, 0);
                }
            }
        } else {
            escortCommandFlag[i] = 0;
        }
    }

    for (i = 0; i < 4; i++) {
        if (playerEscortOrders[i] == kOrderRegroup) {
            found = 0;
            for (j = 1; j < kMaxShips; j++) {
                if (shipData[j].active && shipData[j].parentShip == 0
                        && shipClass[shipData[j].type].escortType == i
                        && shipData[j].aiType == kShipTypeFighter
                        && shipData[j].escortMayAttack) {
                    found = 1;
                    break;
                }
            }
            if (!found) {
                playerEscortOrders[i] = kOrderFormation;
                if (escortMenuOnscreen > 0)
                    ShowEscortMenu();
            }
        }
    }

    for (i = 0; i < 4; i++)
        groupCount[i] = 0;
    for (i = 1; i < kMaxShips; i++) {
        if (shipData[i].active && shipData[i].parentShip == 0
                && shipData[i].escortMayAttack) {
            if (shipClass[shipData[i].type].escortType >= 0
                    && shipClass[shipData[i].type].escortType < 4)
                groupCount[shipClass[shipData[i].type].escortType] += 1;
        }
    }
    for (i = 0; i < 4; i++) {
        if (groupCount[i] == 0 && (int32_t)playerEscortOrders[i] != kOrderFormation) {
            playerEscortOrders[i] = kOrderFormation;
            if (escortMenuOnscreen > 0)
                ShowEscortMenu();
        }
    }

    if (QuickKeyCheck(keyPrefs.toggleFPS)) {
        if (!framerateFlag) {
            framerateFlag = 1;
            showFPS = !showFPS;
            if (showFPS) {
                lastFPSUpdateTime = 0;
                fpsCounter = 0;
                commCount = 0;
            } else {
                commCount = 1;
            }
        }
    } else {
        framerateFlag = 0;
    }

    if (QuickKeyCheck(keyPrefs.speedUp)) {
        if (!speedUpFlag) {
            speedUpFlag = 1;
            if (PlatKeyLockState((int)keyPrefs.speedUp) < 0)
                speedUpToggled = !speedUpToggled;
        }
    } else {
        speedUpFlag = 0;
    }

    if (cheatsActive) {
        if (QuickKeyCheck(kScanBackspace)) {
            if (ST_IsSoundPlaying(warpUpSnd[0])) {
                ST_HaltSound(warpUpSnd[0]);
                WarpUpCallback(1, 0, 0);
            } else if (ST_IsSoundPlaying(warpUpSnd[1])) {
                ST_HaltSound(warpUpSnd[1]);
                WarpUpCallback(1, 0, 0);
            }
        }

        if (QuickKeyCheck(kScanInfoCheat)) {
            strcpy((char *)msg, "gSpeedMult = ");
            NumToString(gSpeedMult * 100.0, name);
            PtoCstr(name);
            strcat((char *)msg, (char *)name);
            strcat((char *)msg, "%, ");
            strcat((char *)msg, "X:");
            NumToString(shipData->pos.x, name);
            PtoCstr(name);
            strcat((char *)msg, (char *)name);
            strcat((char *)msg, ", Y:");
            NumToString(shipData->pos.y, name);
            PtoCstr(name);
            strcat((char *)msg, (char *)name);
            CtoPstr((char *)msg);
            DisplayComm((PStrChar *)msg, 0x3c);
        }

        if (QuickKeyCheck(keyPrefs.cheatSpawn) || QuickKeyCheck(kScanPadEnter)) {
            if (!spawnShipFlag) {
                spawnShipFlag = 1;
                if (AltKeyQuick()) {
                    if (CtrlKeyQuick()) {
                        HyperSpawnFleet(player->curSystem, 6, -1, 1);
                    } else if (QuickKeyCheck(kScanLShift)) {
                        HyperSpawnFleet(player->curSystem, 0x15, -1, 1);
                    } else {
                        HyperSpawnFleet(player->curSystem, 1, -1, 1);
                    }
                } else if (CtrlKeyQuick()) {
                    player->targetShip = SpawnPerson(player->curSystem, 1, 0x201);
                } else {
                    player->targetShip = HyperShipSpawn(player->curSystem);
                }
                statusRedraw[kRedrawTarg] = 1;
                PlaySound(beepSnd[0], 1, gSoundVolume, gSoundVolume);
            }
        } else {
            spawnShipFlag = 0;
        }

        if (QuickKeyCheck(keyPrefs.cheatKill) && player->targetShip != -1) {
            shipData[player->targetShip].active = 0;
            statusRedraw[kRedrawTarg] = 1;
            PlaySound(beepSnd[0], 1, gSoundVolume, gSoundVolume);
        }

        if (QuickKeyCheck(keyPrefs.cheatArmor) || QuickKeyCheck(kScanArmorCheat)) {
            if (player->targetShip != -1) {
                if (AltKeyQuick()) {
                    if (shipData[player->targetShip].IsDisabled()) {
                        shipData[player->targetShip].armor =
                            shipClass[shipData[player->targetShip].type].armor;
                        shipData[player->targetShip].shield = 0;
                    }
                    shipData[player->targetShip].aiState = kAIStateHiJack;
                    shipData[player->targetShip].aiSubstate = kSubLoJack;
                    shipData[player->targetShip].targetShip = -1;
                    shipData[player->targetShip].targetStellar = -1;
                    shipData[player->targetShip].govt = -1;
                    shipData[player->targetShip].parentShip = 0;
                    shipData[player->targetShip].aiType = kShipTypeEscort;
                    shipData[player->targetShip].sellOnLanding = 0;
                    shipData[player->targetShip].upgradeOnLanding = 0;
                    shipData[player->targetShip].fuelRegenState = -1;
                    if (CmdKeyQuick())
                        shipData[player->targetShip].hired = 1;
                    else
                        shipData[player->targetShip].hired = 0;
                    shipData[player->targetShip].armor =
                        shipData[player->targetShip].ArmorCapacity();
                    AISetup(&shipData[player->targetShip]);
                    AIShipCapturedByPlayer(&shipData[player->targetShip]);
                } else {
                    if (shipData[player->targetShip].parentShip == 0) {
                        if (shipData[player->targetShip].aiType == kShipTypeEscort
                                && shipClass[shipData[player->targetShip].type]
                                   .inherentAI <= 2) {
                            DestroyPartialFleetCargo(player->targetShip);
                            statusRedraw[kRedrawCargo] = 1;
                        }
                        if (shipData[player->targetShip].aiType == kShipTypeFighter)
                            shipData[player->targetShip].formerEscort = 0;
                        else if (shipData[player->targetShip].specialMission == -1)
                            shipData[player->targetShip].formerEscort = 1;
                        else
                            shipData[player->targetShip].formerEscort = -1;
                        shipData[player->targetShip].parentShip = -1;
                        shipData[player->targetShip].aiTimer = 500.0f;
                    }
                    DamageShip(&shipData[player->targetShip], NULL, 0, 0x7d00, 0x7d00,
                               -1, 0, 0, 1, 0, 1, 0);
                }
                statusRedraw[kRedrawTarg] = 1;
            } else {
                while (!player->IsDisabled() && !player->IsDying())
                    player->armor = player->armor - 1.0f;
                player->armor = 1.0f;
                playerRepairInhibit = 300.0f;
                statusRedraw[kRedrawShield] = 1;
            }
            PlaySound(beepSnd[0], 1, gSoundVolume, gSoundVolume);
        }

        if (QuickKeyCheck(keyPrefs.cheatDamage) || QuickKeyCheck(kScanDamageCheat)) {
            if (AltKeyQuick()) {
                player->armor = 1.0f;
                statusRedraw[kRedrawShield] = 1;
            } else if (player->targetShip != -1
                    && shipData[player->targetShip].armor > 0.0f) {
                if (ShiftKeyQuick()
                        && !ShipIsDerelict(&shipData[player->targetShip])) {
                    SlapWithPenalty(shipData->curSystem,
                                    shipData[player->targetShip].govt, 3,
                                    shipData[player->targetShip].specialMission);
                    AddToPlayerKills(
                        shipClass[shipData[player->targetShip].type].strength);
                }
                if (shipData[player->targetShip].aiType == kShipTypeEscort
                        && shipClass[shipData[player->targetShip].type]
                           .inherentAI <= 2) {
                    DestroyPartialFleetCargo(player->targetShip);
                    statusRedraw[kRedrawCargo] = 1;
                    DrawStatusCargo();
                }
                shipData[player->targetShip].armor = -1.0f;
                statusRedraw[kRedrawTarg] = 1;
                PlaySound(beepSnd[0], 1, gSoundVolume, gSoundVolume);
            }
        }

        if (QuickKeyCheck(keyPrefs.cheatShipNext) || QuickKeyCheck(keyPrefs.cheatShipPrev)) {
            if (QuickKeyCheck(keyPrefs.cheatShipNext)) {
                do {
                    player->type += 1;
                    if (player->type >= kShipTypeSpecial)
                        player->type = 0;
                } while (shipClass[player->type].mass < 0);
            } else {
                do {
                    player->type -= 1;
                    if (player->type < 0)
                        player->type = kShipTypeSpecial - 1;
                } while (shipClass[player->type].mass < 0);
            }
            while (KeyCheck(keyPrefs.cheatShipNext) || KeyCheck(keyPrefs.cheatShipPrev))
                ;
            PlaySound(beepSnd[0], 1, gSoundVolume, gSoundVolume);
            player->shield = player->ShieldCapacity();
            player->armor = player->ArmorCapacity();
            player->fuel = shipClass[player->type].fuel;
            player->foldSet =
                shipClass[player->type].baseSets - 1;
            player->foldDir = 0;
            player->frameTimer = 0;
            statusRedraw[kRedrawFuel] = statusRedraw[kRedrawShield] = statusRedraw[kRedrawWeap] =
                statusRedraw[kRedrawCargo] = 1;
            for (i = 0; i < kMaxOutfits; i++)
                shipSystems[i] = 0;
            for (i = 0; i < kMaxWeapTypes; i++) {
                player->weap[i].count = 0;
                player->weap[i].ammo = 0;
            }
            for (i = 0; i < kMaxWeapTypes; i++) {
                if (shipClass[player->type].weapCount[i] > 0)
                    player->weap[i].count =
                        shipClass[player->type].weapCount[i];
                if (shipClass[player->type].weapAmmo[i] > 0) {
                    if (weaponStats[i].ammoType >= 0
                            && weaponStats[i].ammoType < kMaxWeapTypes
                            && weaponStats[i].guidance != kGuidanceBay)
                        player->weap[weaponStats[i].ammoType].ammo =
                            shipClass[player->type].weapAmmo[i];
                    else {
                        short ammo = shipClass[player->type].weapAmmo[i];
                        player->weap[i].ammo = ammo;
                    }
                }
            }
            for (i = 0; i < kDefaultOutfitSlots; i++) {
                if (shipClass[player->type].defOutfCount[i] > 0)
                    shipSystems[shipClass[player->type].defOutfID[i]] +=
                        shipClass[player->type].defOutfCount[i];
            }
            ShipStatsToSystemInfo();
            LoadIntfResource();
            player->selectedWeapon = -1;
            ResetPlayerPrecalcedValues();
            player->armor = player->ArmorCapacity();
            player->shield = player->ShieldCapacity();
        }

        if (ShiftKeyQuick()
                && QuickKeyCheck(keyPrefs.cheatCredits)) {
            PlaySound(beepSnd[0], 1, gSoundVolume, gSoundVolume);
            if (AltKeyQuick()) {
                if (player->credits >= 10000)
                    player->credits -= 10000;
                else
                    player->credits = 0;
            } else {
                player->credits += 50000;
            }
            statusRedraw[kRedrawCargo] = 1;
        }

        if (QuickKeyCheck(kScanEnd)) {
            PlaySound(beepSnd[0], 1, gSoundVolume, gSoundVolume);
            cheatsActive = 0;
        }
    }

    if (player->credits < 0) {
        player->credits = 0x5fffffff;
        statusRedraw[kRedrawCargo] = 1;
    }
    if (player->credits > 0x5fffffff) {
        player->credits = 0x5fffffff;
        statusRedraw[kRedrawCargo] = 1;
    }
    if (player->hasFormation)
        AICalcFormationPositions(player, 0);
    if (player->deathTimer > 1.0f) {
        if (!ST_IsSoundPlaying(klaxxonSnd))
            PlaySound(klaxxonSnd, 0xf, gSoundVolume, gSoundVolume);
    }

    if (disabled || dying) {
        ok = 0;
        if (dying) {
            if (player->deathTimer <= shipClass[player->type].deathDelay * 0.5
                    || player->deathTimer <= 30.0f) {
                if (player->HasAutoEject())
                    ok = 1;
            }
        }
        if ((CmdKeyQuick()
                && QuickKeyCheck(keyPrefs.eject)) || ok) {
            keyPressedThisFrame = 1;
            if (player->HasEscapePod()) {
                n = GenericRandomShipSpawn(player->curSystem, 0);
                if (n != -1) {
                shipData[n].aiType = -1;
                shipData[n].pos = player->pos;
                shipData[n].vel = player->vel;
                shipData[n].heading = player->heading;
                shipData[n].type = player->type;
                shipData[n].deathTimer = player->deathTimer;
                shipData[n].ionization = 0;
                shipData[n].ionColor = 0;
                shipData[n].lightLevel = 0;
                shipData[n].blinkCount = 0;
                shipData[n].altSet = 0;
                shipData[n].animSet = 0;
                shipData[n].bankDir = 0;
                shipData[n].animTimer = 0;
                shipData[n].shield = player->shield;
                shipData[n].armor = player->armor;
                shipData[n].boarded = 1;
                SWCloneFramesFromMaster(shipData[n].sprite,
                                        masterShipSprite[shipData[n].type]);
                SWSetCurrentFrameIndex(shipData[n].sprite, 0);
                player->deathTimer = -1.0f;
                player->targetShip = -1;
                player->targetStellar = -1;
                player->selectedWeapon = -1;
                statusRedraw[kRedrawFuel] = statusRedraw[kRedrawNav] = statusRedraw[kRedrawTarg] =
                    statusRedraw[kRedrawCargo] = statusRedraw[kRedrawWeap] = 1;
                crashlog_mission_bit_set("retiring",
                    (char *)shipClass[shipData->type].name, 1,
                    shipData->type + kFirstResourceID,
                    shipClass[shipData->type].onRetire);
                EvalMissionBitSetString(shipClass[shipData->type].onRetire);
                if (player->HasEscapeShip()) {
                    player->type = player->EscapeShipType();
                    player->shield = shipClass[player->type].shield
                        * (Rand(0x1e) + 0x32) * 0.01;
                    player->armor = shipClass[player->type].armor
                        * (Rand(0x1e) + 0x32) * 0.01;
                    player->fuel = shipClass[player->type].fuel
                        * (Rand(0x1e) + 0x32) * 0.01;
                    player->escapePods = -1;
                    strcpy((char *)msg, "");
                    StrcatIndCString((char *)msg, kGameStrList, 0x34);
                    strcat((char *)msg, " ");
                    pstrcpy(name, shipyardLongNames[player->type]);
                    PtoCstr(name);
                    strcat((char *)msg, (char *)name);
                    strcat((char *)msg, ".");
                    CtoPstr((char *)msg);
                    DisplayComm((PStrChar *)msg, 0x200);
                } else {
                    player->type = kShipTypeSpecial;
                    player->shield = shipClass[player->type].shield;
                    player->armor = shipClass[player->type].armor;
                    player->escapePods = 0x15e;
                    for (i = 1; i < kMaxShips; i++) {
                        if (shipData[i].active && shipData[i].targetShip == 0)
                            shipData[i].targetShip = -1;
                    }
                }
                for (i = 1; i < kMaxShips; i++) {
                    if (shipData[i].active) {
                        if (IsThreatToPlayer(&shipData[i])) {
                            if (shipData[i].targetShip == 0)
                                shipData[i].targetShip = n;
                            if (shipData[i].targetStellar == 0)
                                shipData[i].targetStellar = n;
                        } else if (shipData[i].parentShip == 0
                                && player->type == kShipTypeSpecial) {
                            shipData[i].parentShip = -1;
                        }
                    }
                }
                SWCloneFramesFromMaster(player->sprite,
                                        masterShipSprite[player->type]);
                SWSetCurrentFrameIndex(player->sprite, 0);
                player->vel.x = player->vel.y;
                Accel(player->heading, player->MaxSpeed(),
                      &player->vel);
                for (i = 0; i < kMaxOutfits; i++) {
                    if (!outfitSystem[i].persistent)
                        shipSystems[i] = 0;
                }
                for (i = 0; i < kMaxWeapTypes; i++) {
                    player->weap[i].count = 0;
                    player->weap[i].ammo = 0;
                }
                for (i = 0; i < kMaxWeapTypes; i++) {
                    player->weap[i].count =
                        shipClass[player->type].weapCount[i];
                    if (shipClass[player->type].weapAmmo[i] > 0) {
                        if (weaponStats[i].ammoType >= 0
                                && weaponStats[i].ammoType < kMaxWeapTypes
                                && weaponStats[i].guidance != kGuidanceBay)
                            player->weap[weaponStats[i].ammoType].ammo =
                                shipClass[player->type].weapAmmo[i];
                        else
                            player->weap[i].ammo =
                                shipClass[player->type].weapAmmo[i];
                    }
                    player->weap[i].reloadTimer = 0;
                    player->weap[i].burstCount = 0;
                }
                ShipStatsToSystemInfo();
                for (i = 0; i < kDefaultOutfitSlots; i++) {
                    if (shipClass[player->type].defOutfCount[i] > 0)
                        shipSystems[
                            shipClass[player->type].defOutfID[i]] +=
                            shipClass[player->type].defOutfCount[i];
                }
                PlaySound(escapePodSnd, 0x32, gSoundVolume, gSoundVolume);
                if (ST_IsSoundPlaying(warpUpSnd[0]))
                    ST_HaltSound(warpUpSnd[0]);
                if (ST_IsSoundPlaying(warpUpSnd[1]))
                    ST_HaltSound(warpUpSnd[1]);
                player->jumpTimer = 0;
                player->deathTimer = -1.0f;
                }
            }
        }
    }

    if (abOn) {
        if (!gravitySourcesPresent) {
            playerMaxVel.x = player->MaxSpeed() * 1.8;
            playerMaxVel.y = player->MaxSpeed() * 1.8;
        }
        if (player->IsInertialess()) {
            player->accelMag = player->accelMag + player->AccelRate() * 2.75;
            if (player->accelMag > player->MaxSpeed() * 1.8)
                player->accelMag = player->MaxSpeed() * 1.8;
        } else {
            pos.x = pos.y = 0;
            dest.x = dest.y = 0;
            Accel(player->heading, player->AccelRate() * 2.75, &pos);
            Accel(player->heading, player->MaxSpeed() * 1.8, &dest);
            if ((pos.y > 0.0f && player->vel.y < dest.y)
                    || (pos.y <= 0.0f && player->vel.y > dest.y))
                player->vel.y = player->vel.y + pos.y;
            else
                player->vel.y *= 0.99;
            if ((pos.x > 0.0f && player->vel.x < dest.x)
                    || (pos.x <= 0.0f && player->vel.x > dest.x))
                player->vel.x = player->vel.x + pos.x;
            else
                player->vel.x *= 0.99;
        }
        player->fuel = player->fuel - player->AfterburnerFuelUsage() * gSpeedMult;
        if (player->fuel < 0.0)
            player->fuel = 0;
        if (frameCounter % 4 == 0)
            statusRedraw[kRedrawFuel] = 1;
        if (player->engineGlow < 0x20) {
            player->engineGlow += 1;
            if (!QuickKeyCheck(keyPrefs.accelerate))
                player->engineGlow += 1;
        }
        if (player->engineGlow > 0x20)
            player->engineGlow -= 1;
    } else {
        step = player->MaxSpeed();
        turnLim = player->AccelRate() * 0.4;
        if (playerMaxVel.x > step)
            playerMaxVel.x = playerMaxVel.x - turnLim;
        if (playerMaxVel.y > step)
            playerMaxVel.y = playerMaxVel.y - turnLim;
        if (playerMaxVel.x < step)
            playerMaxVel.x = step;
        if (playerMaxVel.y < step)
            playerMaxVel.y = step;
        if (player->engineGlow > 0x18)
            player->engineGlow -= 1;
    }

    if (CmdKeyQuick()
            && QuickKeyCheck(keyPrefs.jettison)) {
        keyPressedThisFrame = 1;
        if (QuickKeyCheck(kScanLShift) || QuickKeyCheck(kScanRShift))
            JettisonCargo(0);
        else
            JettisonCargo(1);
        statusRedraw[kRedrawCargo] = 1;
        ResetPlayerPrecalcedValues();
    }

    if (CmdKeyQuick()
            && QuickKeyCheck(keyPrefs.selfDestruct) && destructCount < 0.0f
            && !dying) {
        keyPressedThisFrame = 1;
        PlaySound(beepSnd[3], 0xf, gSoundVolume, gSoundVolume);
        strcpy((char *)msg, "");
        StrcatIndCString((char *)msg, kGameStrList, 0x181);
        strcat((char *)msg, " ");
        StrcatIndCString((char *)msg, kGameStrList, 0x182);
        CtoPstr((char *)msg);
        DisplayComm((PStrChar *)msg, 0x28);
        destructCount = 150.0f;
    }

    if (destructCount > 0.0f) {
        if (CmdKeyQuick()
                && QuickKeyCheck(keyPrefs.selfDestruct)) {
            destructing = 1;
            if (destructCount >= 1.0)
                destructCount = destructCount - gSpeedMult;
            if ((short)destructCount % 30 == 0 && destructCount <= 120.0f) {
                strcpy((char *)msg, "");
                StrcatIndCString((char *)msg, kGameStrList, 0x181);
                strcat((char *)msg, " ");
                StrcatIndCString((char *)msg, kGameStrList, 0x18b);
                strcat((char *)msg, " ");
                NumToString(destructCount / 30.0f, name);
                PtoCstr(name);
                strcat((char *)msg, (char *)name);
                strcat((char *)msg, " ");
                if (destructCount > 30.0f)
                    StrcatIndCString((char *)msg, kGameStrList, 0x185);
                else
                    StrcatIndCString((char *)msg, kGameStrList, 0x185);
                strcat((char *)msg, ".");
                CtoPstr((char *)msg);
                PlaySound(beepSnd[2], 0xf, gSoundVolume,
                          gSoundVolume);
                DisplayComm((PStrChar *)msg, 0x46);
            }
        } else {
            PlaySound(beepSnd[1], 0xf, gSoundVolume, gSoundVolume);
            strcpy((char *)msg, "");
            StrcatIndCString((char *)msg, kGameStrList, 0x181);
            strcat((char *)msg, " ");
            StrcatIndCString((char *)msg, kGameStrList, 0x183);
            CtoPstr((char *)msg);
            DisplayComm((PStrChar *)msg, 0x28);
            destructing = 1;
            destructCount = -1.0f;
        }
    } else {
        destructing = 0;
    }

    if (destructCount <= 1.0f && destructing) {
        if (CmdKeyQuick()
                && QuickKeyCheck(keyPrefs.selfDestruct)) {
            keyPressedThisFrame = 1;
            PlaySound(beepSnd[1], 0xf, gSoundVolume, gSoundVolume);
            GetIndString(msg, kGameStrList, 0x186);
            DisplayComm((PStrChar *)msg, 0x28);
            player->shield = 0;
            player->armor = -1.0f;
            player->targetShip = -1;
            statusRedraw[kRedrawWeap] = 1;
            for (i = 1; i < kMaxShips; i++) {
                if (shipData[i].active
                        && AIShipIsDockedWithShip(&shipData[i], player))
                    shipData[i].armor = -1.0f;
            }
        }
        destructCount = -1.0f;
    }

    if (QuickKeyCheck(keyPrefs.playerInfo)) {
        keyPressedThisFrame = 1;
        if (!playerInfoFlag) {
            playerInfoFlag = 1;
            if (player->jumpTimer <= 0.0f && player->escapePods <= 0
                    && frameCounter >= 0 && !dying) {
                HideCursorSprite();
                RefreshSAT();
                ShowTheCursor();
                DoPlayerInfoDialog();
                gSpeedMult = 0;
                RedrawStatus(1);
                SetAllKeyCmdFlags();
            } else {
                PlaySound(beepSnd[3], 5, gSoundVolume, gSoundVolume);
            }
        }
    } else {
        playerInfoFlag = 0;
    }

    if (QuickKeyCheck(keyPrefs.hail)) {
        keyPressedThisFrame = 1;
        if (!commTryFlag) {
            commTryFlag = 1;
            HandlePlayerCommunication();
        }
    } else {
        commTryFlag = 0;
    }

    if (QuickKeyCheck(keyPrefs.missions) && player->jumpTimer <= 0.0f
            && player->escapePods <= 0 && !dying && player->deathTimer <= 0.0f
            && frameCounter >= 0) {
        keyPressedThisFrame = 1;
        if (!missionInfoFlag) {
            missionInfoFlag = 1;
            if (AnyMissionShown()) {
                HideCursorSprite();
                RefreshSAT();
                ShowTheCursor();
                DoMissionInfoDialog();
                gSpeedMult = 0;
                RedrawStatus(1);
                SetAllKeyCmdFlags();
            } else {
                PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);
                GetIndString(msg, kGameStrList, 0x162);
                DisplayComm((PStrChar *)msg, kCommFrames);
            }
        }
    } else {
        missionInfoFlag = 0;
    }

    if (QuickKeyCheck(keyPrefs.cloak) && player->deathTimer <= 0.0f
            && player->jumpTimer <= 0.0f && !disabled) {
        keyPressedThisFrame = 1;
        if (!cloakFlag) {
            cloakFlag = 1;
            if (player->CanCloak()) {
                if (player->cloak > 0.0f || player->cloakDir > 0)
                    player->DoUncloak();
                else
                    player->DoCloak();
            } else {
                PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);
            }
        }
    } else {
        cloakFlag = 0;
    }

    if (player->IsCloaked()) {
        if (!player->CanCloak() || disabled)
            player->DoUncloak();
        n = player->CloakFuelUsage();
        if (n > 0) {
            player->fuel = player->fuel - n * 0.03333 * gSpeedMult;
            if (frameCounter % 10 == 0)
                statusRedraw[kRedrawFuel] = 1;
            if (player->fuel <= 0.0f)
                player->fuel = 0;
        }
        if (player->CloakZeroesShields()) {
            player->shield = 0;
            if (frameCounter % 10 == 0)
                statusRedraw[kRedrawShield] = 1;
        } else {
            n = player->CloakShieldUsage();
            if (n > 0 && player->shield >= n) {
                player->shield -= n * 0.03333 * gSpeedMult;
                if (frameCounter % 10 == 0)
                    statusRedraw[kRedrawShield] = 1;
                if (player->shield <= 0.0f)
                    player->shield = 0;
            }
        }
    }

    if (playerHasTribbles && frameCounter % 250 == 0) {
        n = TotalFleetHolds() - TotalCargo(player);
        for (i = 0; i < kMaxJunks; i++) {
            if ((junkData[i].flags & kJunkTribbles) && junkData[i].held > 0) {
                if (n > 0) {
                    junkData[i].held += 1;
                    statusRedraw[kRedrawCargo] = 1;
                }
            }
        }
    }
    if (playerHasPerishableJunk && frameCounter % 250 == 0) {
        for (i = 0; i < kMaxJunks; i++) {
            if ((junkData[i].flags & kJunkPerishable) && junkData[i].held > 0) {
                if (n > 0) {
                    junkData[i].held -= 1;
                    statusRedraw[kRedrawCargo] = 1;
                }
            }
        }
    }

    if (newbieFlag < 2 && frameCounter % 60 == 0) {
        origin.x = origin.y = 0;
        dist = FloatAbs(XYFloatSquaredDist(&player->pos,
                                         &origin));
        if (newbieFlag == -3 && newbieString[0][0] && newbieString[1][0]) {
            n = FindNearestStellar(shipData);
            if (n != -1) {
                PlaySound(beepSnd[4], 1, gSoundVolume, gSoundVolume);
                if (stellarData[n].flags & kSpobIsStation)
                    pstrcpy(msg, newbieString[0]);
                else
                    pstrcpy(msg, newbieString[1]);
                PtoCstr(msg);
                strcat((char *)msg, " ");
                pstrcpy(name, stellarData[n].name);
                PtoCstr(name);
                strcat((char *)msg, (char *)name);
                strcat((char *)msg, " ");
                pstrcpy(name, newbieString[2]);
                PtoCstr(name);
                strcat((char *)msg, (char *)name);
                KeyboardScanCodeToString((char *)name, 0x100, keyPrefs.land);
                strcat((char *)msg, (char *)name);
                pstrcpy(name, newbieString[3]);
                PtoCstr(name);
                strcat((char *)msg, (char *)name);
                CtoPstr((char *)msg);
                DisplayComm((PStrChar *)msg, 0x200);
            }
            newbieFlag = -2;
        }
        if (newbieFlag == -1 && newbieString[4][0]) {
            if (FindNearestStellar(shipData) != -1) {
                PlaySound(beepSnd[4], 1, gSoundVolume, gSoundVolume);
                pstrcpy(msg, newbieString[4]);
                PtoCstr(msg);
                KeyboardScanCodeToString((char *)name, 0x100, keyPrefs.map);
                strcat((char *)msg, (char *)name);
                pstrcpy(name, newbieString[5]);
                PtoCstr(name);
                strcat((char *)msg, (char *)name);
                KeyboardScanCodeToString((char *)name, 0x100, keyPrefs.jump);
                strcat((char *)msg, (char *)name);
                pstrcpy(name, newbieString[6]);
                PtoCstr(name);
                strcat((char *)msg, (char *)name);
                CtoPstr((char *)msg);
                DisplayComm((PStrChar *)msg, 0x200);
            }
            newbieFlag = 0;
        }
        if (dist > 2000000.0 && newbieFlag <= 0) {
            if (!disabled && newbieString[8][0]) {
                PlaySound(beepSnd[4], 1, gSoundVolume, gSoundVolume);
                DisplayComm((PStrChar *)newbieString[8], 0x200);
                newbieFlag = 1;
            }
        } else if (dist > 10000000.0 && newbieFlag == 1) {
            if (!disabled && newbieString[9][0]) {
                PlaySound(beepSnd[4], 1, gSoundVolume, gSoundVolume);
                DisplayComm((PStrChar *)newbieString[9], 0x200);
                newbieFlag = 2;
            }
        }
    }

    if (player->credits < 0) {
        player->credits = 0;
        statusRedraw[kRedrawCargo] = 1;
    }
    if (player->fuel < 0.0f) {
        player->fuel = 0;
        statusRedraw[kRedrawFuel] = 1;
    }
    n = FindFirstCoLocatedSystem(player->curSystem);
    if (systemData[n].exploreLevel < 1)
        systemData[n].exploreLevel = 1;
    if (systemData[player->curSystem].exploreLevel < 1)
        systemData[player->curSystem].exploreLevel = 1;
    if (player->heading != heading)
        playerTurnedThisFrame = 1;

    if (showFloatingMap > 0 && !quickKeyCheckResult
            && !QuickKeyCheck(kScanLShift) && !QuickKeyCheck(kScanRShift)
            && !QuickKeyCheck(kScanLCtrl) && !QuickKeyCheck(kScanRCtrl)
            && !QuickKeyCheck(kScanLAlt) && !QuickKeyCheck(kScanRAlt)) {
        if ((QuickKeyCheck(kScanMinus) || QuickKeyCheck(kScanPadMinus)) && !mapZoomFlag
                && floatingMapZoom < 2.0) {
            floatingMapZoom *= 1.333333;
            floatingMapStartTime = ReadTick();
            DrawMapInFloatingFrame();
            PlaySound(beepSnd[0], 1, gSoundVolume, gSoundVolume);
        }
        if ((QuickKeyCheck(kScanEquals) || QuickKeyCheck(kScanPadPlus)) && !mapZoomFlag
                && floatingMapZoom > 0.5) {
            floatingMapZoom *= 0.75;
            floatingMapStartTime = ReadTick();
            DrawMapInFloatingFrame();
            PlaySound(beepSnd[0], 1, gSoundVolume, gSoundVolume);
        }
        if (QuickKeyCheck(kScanEquals) || QuickKeyCheck(kScanMinus) || QuickKeyCheck(kScanPadMinus)
                || QuickKeyCheck(kScanPadPlus))
            mapZoomFlag = 1;
        else
            mapZoomFlag = 0;
    }
}

#define kCommRefused        0x35
#define kCommDisabled       0x36

/*
 * The four mïsn fields this decision reads.  ONE typed window rather than
 * four per-field accessor macros: the macro form materialises the address
 * with its own add where the exe folds base and scaled index into the one
 * instruction (`cmp word ptr [R + R + 6]` against `add R,R` then
 * `cmp word ptr [R + 6]`).
 */


                                                         /* 0x0041cf40 */

// FUNCTION: EVNOVA 0x00454910
// MAC: _HandlePlayerCommunication i386 0x00061f48 ppc 0x00069e14
void HandlePlayerCommunication(void)
{
    ShipObj *me;
    short pers;
    short mission;
    Str255 buf;
    short found;
    Boolean canHail;
    short i;
    short newShip;
    Boolean canLand;

    me = shipData;

    if (me->IsCloaked() == 0 && me->IsDying() == 0 &&
            me->jumpTimer <= 0.0f) {
        if (me->targetShip == -1 || QuickKeyCheck(kScanLAlt) != 0 ||
                QuickKeyCheck(kScanRAlt) != 0) {
            if (me->navTargetKind == kNavTargetStellar && me->targetStellar != -1) {
                canLand = !(stellarData[me->targetStellar].flags
                           & kSpobUninhabited);
                if (StellarIsLandable(&stellarData[me->targetStellar]) == 0)
                    canLand = 0;
                if ((stellarData[me->targetStellar].flags2 & kSpobGateOrWormhole) != 0)
                    canLand = 0;
                if (stellarData[me->targetStellar].loaded == 0 || !canLand) {
                    PlaySound(beepSnd[3], 5, gSoundVolume, gSoundVolume);
                    GetIndString(buf, kGameStrList, kCommRefused);
                    DisplayComm((PStrChar *)buf, kCommFrames);
                } else {
                    HideCursorSprite();
                    RefreshSAT();
                    ShowTheCursor();
                    DoPlanetCommDialog(me->targetStellar);
                    SetAllKeyCmdFlags();
                }
            } else {
                PlaySound(beepSnd[3], 5, gSoundVolume, gSoundVolume);
            }
        } else if (shipData[me->targetShip].jumpTimer <= 0.0f) {
            canHail = 1;
            if (shipData[me->targetShip].parentShip != 0 ||
                    shipData[me->targetShip].specialMission != -1) {
                if (shipData[me->targetShip].govt != -1 &&
                        (govtData[shipData[me->targetShip].govt].flags
                            & kGovtNoComm) != 0)
                    canHail = 0;
                if (shipClass[shipData[me->targetShip].type].inherentGovtB != -1 &&
                        (govtData[shipClass[shipData[me->targetShip].type]
                            .inherentGovtB].flags & kGovtNoComm) != 0)
                    canHail = 0;
            }
            if (shipData[me->targetShip].type == kShipTypeEscapePod)
                canHail = 0;
            if (shipData[me->targetShip].IsDisabled() != 0)
                canHail = 0;
            pers = shipData[me->targetShip].persType;
            if (canHail) {
                if (pers != -1) {
                  if (personData[pers].linkMission != -1) {
                    if ((personData[pers].flags & kPersOfferWhenBoarded) == 0) {
                    offeringFromShip = 1;
                    offeringShipIndex = me->targetShip;
                    if (IsMissionAvailable(
                            personData[shipData[me->targetShip].persType].linkMission, 1) != 0) {
                        PlaySound(beepSnd[4], 1, gSoundVolume, gSoundVolume);
                        HideCursorSprite();
                        RefreshSAT();
                        ShowTheCursor();
                        mission = OfferOneMission(
                            personData[shipData[me->targetShip].persType].linkMission);
                        SetAllKeyCmdFlags();
                        RefreshSAT();
                        if (mission == 1) {
                            if ((personData[shipData[me->targetShip].persType].flags
                                    & kPersDeactivateOnAccept) != 0)
                                personData[shipData[me->targetShip].persType].loaded = 0;
                            if ((personData[shipData[me->targetShip].persType].flags
                                    & kPersLeavesOnAccept) != 0)
                                AIMakeShipLeave(&shipData[me->targetShip]);
                            found = -1;
                            for (i = 0; i < kMaxActiveMissions; i++) {
                                if (currentMissionObjectives[i].active != 0 &&
                                        currentMissions[i].misnIndex ==
                                            personData[shipData[me->targetShip]
                                                .persType].linkMission) {
                                    found = i;
                                    break;
                                }
                            }
                            if (found >= 0 &&
                                    (personData[shipData[me->targetShip].persType].flags
                                        & kPersSwapsShip) != 0 &&
                                    currentMissions[found].shipCount == 1) {
                                newShip = MissionDudeSpawn(currentMissions[found].shipDude,
                                                       shipData[me->targetShip].type,
                                                       shipData->curSystem, found);
                                if (newShip != -1) {
                                    shipData[newShip].pos.x =
                                        shipData[me->targetShip].pos.x;
                                    shipData[newShip].pos.y =
                                        shipData[me->targetShip].pos.y;
                                    shipData[newShip].vel.x =
                                        shipData[me->targetShip].vel.x;
                                    shipData[newShip].vel.y =
                                        shipData[me->targetShip].vel.y;
                                    shipData[newShip].heading =
                                        shipData[me->targetShip].heading;
                                    if (currentMissions[found].shipGoal == kMissionGoalEscort)
                                        AIMakeEscortFlyInForm(&shipData[newShip]);
                                    shipData[me->targetShip].active = 0;
                                    me->targetShip = newShip;
                                    weapTargetExpand = 0.0F;
                                    statusRedraw[kRedrawTarg] = 1;
                                }
                            }
                        }
                    } else {
                        HideCursorSprite();
                        RefreshSAT();
                        ShowTheCursor();
                        DoCommDialog(me->targetShip);
                        SetAllKeyCmdFlags();
                    }
                    offeringFromShip = 0;
                    offeringShipIndex = me->targetShip;
                    } else {
                    HideCursorSprite();
                    RefreshSAT();
                    ShowTheCursor();
                    DoCommDialog(me->targetShip);
                    SetAllKeyCmdFlags();
                    }
                  } else {
                    HideCursorSprite();
                    RefreshSAT();
                    ShowTheCursor();
                    DoCommDialog(me->targetShip);
                    SetAllKeyCmdFlags();
                  }
                } else {
                    HideCursorSprite();
                    RefreshSAT();
                    ShowTheCursor();
                    DoCommDialog(me->targetShip);
                    SetAllKeyCmdFlags();
                }
                RedrawStatus(1);
            } else {
                PlaySound(beepSnd[3], 5, gSoundVolume, gSoundVolume);
                GetIndString(buf, kGameStrList, kCommRefused);
                DisplayComm((PStrChar *)buf, kCommFrames);
            }
        } else if (shipData[me->targetShip].IsDisabled() != 0) {
            PlaySound(beepSnd[3], 5, gSoundVolume, gSoundVolume);
            GetIndString(buf, kGameStrList, kCommRefused);
            DisplayComm((PStrChar *)buf, kCommFrames);
        } else {
            PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);
            GetIndString(buf, kGameStrList, kCommDisabled);
            DisplayComm((PStrChar *)buf, kCommFrames);
        }
    } else {
        PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);
    }
}

/* abs is inlined only when <stdlib.h> declares it; a local extern emits a call. */


// FUNCTION: EVNOVA 0x00455150
// MAC: _FirePlayerWeapon i386 0x00062702 ppc 0x0006a510
void FirePlayerWeapon(short w)
{
    short i;
    Boolean soundOK;
    Boolean didFire;
    short shot;
    short n;
    short fired;
    short beamAngle;
    short angle;
    short diff;
    short ammoType;
    short slot;

    if (shipData->IsCloaked()) {
        if ((weaponStats[w].flags2 & kWeapFireWhileCloaked) == 0)
            return;
    }

    shipData->fireTick = ReadTick();

    if ((shipClass[shipData->type].shanFlags & kShanFoldingParts)
            && (shipClass[shipData->type].shanFlags & kShanUnfoldsWhenFiring)
            && shipData->foldSet > 0) {
        shipData->foldDir = -1;
        return;
    }

    if (shipData->weap[w].reloadTimer <= 0.0f && 0.0f == shipData->jumpTimer) {
        fired = 0;
        n = WeaponMaxSimultShots(shipData, w);
        for (shot = 0; shot < n; shot++) {
            didFire = 0;
            if (!WeaponHasAmmo(shipData, w))
                continue;

            if (weaponStats[w].guidance == kGuidanceUnguided
                    || weaponStats[w].guidance == kGuidanceHoming
                    || weaponStats[w].guidance == kGuidanceFreefall
                    || weaponStats[w].guidance == kGuidanceRocket) {
                SpawnShot(0, shipData->targetShip, w, 0);
                fired = fired + 1;
                didFire = 1;
            }
            if (weaponStats[w].guidance == kGuidanceBeam) {
                SpawnBeam(0, shipData->targetShip, w, shipData->heading, -1, -1);
                fired++;
                didFire = 1;
            }
            if (weaponStats[w].guidance == kGuidanceTurretBeam
                    && shipData->targetShip != -1) {
                beamAngle = CalcInterceptAngle(
                    &shipData->pos,
                    &shipData[shipData->targetShip].pos);
                if (!TurretBlindSpot(shipData, beamAngle, w)) {
                    SpawnBeam(0, shipData->targetShip, w, beamAngle, -1, -1);
                    fired++;
                    didFire = 1;
                }
            }
            if (weaponStats[w].guidance == kGuidanceTurret
                    && shipData->targetShip != -1) {
                if (!TurretBlindSpot(shipData, CalcInterceptAngle(
                        &shipData->pos,
                        &shipData[shipData->targetShip].pos), w)) {
                    SpawnShot(0, shipData->targetShip, w, 0);
                    fired++;
                    didFire = 1;
                }
            }
            if (weaponStats[w].guidance == kGuidanceFrontQuad
                    || weaponStats[w].guidance == kGuidanceRearQuad) {
                if (shipData->targetShip != -1) {
                    angle = CalcInterceptAngle(
                        &shipData->pos,
                        &shipData[shipData->targetShip].pos);
                    if (weaponStats[w].guidance == kGuidanceFrontQuad)
                        diff = abs(AngularDifference(
                            angle, shipData->heading));
                    if (weaponStats[w].guidance == kGuidanceRearQuad)
                        diff = abs(AngularDifference(
                            angle,
                            ((short)(shipData->heading + kHalfTurn) % kFullCircle)));
                    if (diff <= 45) {
                        SpawnShot(0, shipData->targetShip, w, 0);
                        fired++;
                        didFire = 1;
                    } else {
                        if (weaponStats[w].guidance == kGuidanceFrontQuad) {
                            SpawnShot(0, -1, w, 0);
                            fired++;
                            didFire = 1;
                        }
                    }
                } else {
                    if (weaponStats[w].guidance == kGuidanceFrontQuad) {
                        SpawnShot(0, -1, w, 0);
                        fired++;
                        didFire = 1;
                    }
                }
            }
            if (weaponStats[w].guidance == kGuidanceBay) {
                if (LaunchFighter(shipData, w)) {
                    fired++;
                    didFire = 1;
                }
            }
            if (!didFire)
                continue;

            if (weaponStats[w].flags2 & kWeapUsesShipSprite)
                shipData->weapGlow = 32.0f;
            ammoType = weaponStats[w].ammoType;
            if (ammoType == -1)
                continue;
            if (weaponStats[w].flags3 & kWeapAmmoAtBurstEnd)
                continue;
            if (ammoType >= 0) {
                if (weaponStats[w].guidance == kGuidanceBay)
                    slot = w;
                else if (ammoType >= 0 && ammoType < kMaxWeapTypes)
                    slot = ammoType;
                else
                    slot = -1;
                if (slot != -1) {
                    shipData->weap[slot].ammo--;
                    if (shipData->weap[slot].ammo < 0)
                        shipData->weap[slot].ammo = 0;
                }
                statusRedraw[kRedrawWeap] = 1;
            } else if (ammoType <= -kWeapAmmoFuelBase) {
                shipData->fuel -= (abs(ammoType) - kWeapAmmoFuelBase) * 0.1;
                if (shipData->fuel < 0.0f)
                    shipData->fuel = 0.0f;
                statusRedraw[kRedrawFuel] = 1;
            }
        }

        if (fired > 0) {
            soundOK = 1;
            if (weaponStats[w].flags & kWeapLoopSound) {
                if (weaponStats[w].sound >= 0) {
                    if (ST_IsSoundPlaying(shotSnd[weaponStats[w].sound]))
                        soundOK = 0;
                }
            }
            if (weaponStats[w].sound >= 0 && soundOK) {
                if (weaponStats[w].guidance == kGuidanceBeam
                        || weaponStats[w].guidance == kGuidanceTurretBeam) {
                    PlaySoundDistance(2, shotSnd[weaponStats[w].sound], 6,
                                      &shipData->pos, &shipData->pos);
                    soundOK = 0;
                }
            }
            if (weaponStats[w].sound < 0)
                soundOK = 0;
            if (soundOK) {
                if (weaponStats[w].guidance == kGuidanceBay)
                    PlaySoundDistance(-1, shotSnd[weaponStats[w].sound], 6,
                                      &shipData->pos, &shipData->pos);
                else if (weaponStats[w].flags & kWeapSecondary)
                    PlaySoundDistance(-1, shotSnd[weaponStats[w].sound], 6,
                                      &shipData->pos, &shipData->pos);
                else
                    PlaySoundDistance(-1, shotSnd[weaponStats[w].sound], 5,
                                      &shipData->pos, &shipData->pos);
            }

            /* recoil */
            if (weaponStats[w].recoil > 0) {
                AdjustedAccel(((short)(shipData->heading + kHalfTurn) % kFullCircle),
                              weaponStats[w].recoil
                                  / (float)shipClass[shipData->type].mass,
                              &shipData->vel,
                              shipClass[shipData->type].maxSpeed);
            }

            if (weaponStats[w].flags & kWeapFireSimultaneously)
                shipData->weap[w].reloadTimer = weaponStats[w].reload;
            else
                shipData->weap[w].reloadTimer =
                    fired * (weaponStats[w].reload / shipData->weap[w].count);

            if (weaponStats[w].flags3 & kWeapExclusive) {
                for (i = 0; i < kMaxWeapTypes; i++) {
                    if (i != w) {
                        if (shipData->weap[i].reloadTimer
                                < shipData->weap[w].reloadTimer + 2.0f)
                            shipData->weap[i].reloadTimer =
                                shipData->weap[w].reloadTimer + 2.0f;
                    }
                }
            }

            if (weaponStats[w].burstCount > 0) {
                shipData->weap[w].burstCount++;

                /* per-burst ammo is charged once the burst completes */
                if ((weaponStats[w].flags3 & kWeapAmmoAtBurstEnd)
                        && shipData->weap[w].burstCount
                            % weaponStats[w].burstCount == 0) {
                    if (weaponStats[w].flags & kWeapFireSimultaneously)
                        shot = fired;
                    else
                        shot = 1;
                    if (weaponStats[w].guidance == kGuidanceBay) {
                        shipData->weap[w].ammo -= shot;
                        if (shipData->weap[w].ammo < 0)
                            shipData->weap[w].ammo = 0;
                    } else {
                        ammoType = weaponStats[w].ammoType;
                        if (ammoType >= 0 && ammoType < kMaxWeapTypes) {
                            shipData->weap[weaponStats[w].ammoType].ammo -= shot;
                            if (shipData->weap[weaponStats[w].ammoType].ammo < 0)
                                shipData->weap[weaponStats[w].ammoType].ammo = 0;
                            statusRedraw[kRedrawWeap] = 1;
                        } else if (ammoType <= -kWeapAmmoFuelBase) {
                            shipData->fuel -=
                                (abs(ammoType) - kWeapAmmoFuelBase) * 0.1 * shot;
                            if (shipData->fuel < 0.0f)
                                shipData->fuel = 0.0f;
                            statusRedraw[kRedrawFuel] = 1;
                        }
                    }
                }

                if (shipData->weap[w].burstCount
                        >= WeaponBurstInterval(w, shipData->weap[w].count)) {
                    shipData->weap[w].burstCount = 0;
                    shipData->weap[w].reloadTimer = weaponStats[w].burstReload;
                }
            }
        }
    }
}

/*
 * WrapObject skips anything further than 15000 units from the
 * player, which is why the shift is applied to the player LAST. beamData is
 * the odd one out: its endpoints are shorts, not floats, so the shift is
 * written inline and each store carries the float-to-int rounding correction
 * mwcc inlines for a cast at level=4.
 */

/* ONE FloatPoint, not two floats: the mac names _wrapDelta at +0/+4, and the
   aggregate spelling is what pins the two zeroing stores after the frame
   setup - as two separate externs mwcc pulls them up into the register
   pushes and the prologue splits. */

#define kWrapTrigger  15000.0f
#define kWrapStep     25000.0f

#define kMaxSmoke     0x40

// FUNCTION: EVNOVA 0x0045baa0
// MAC: _WrapPlayerPosition i386 0x00067efa ppc 0x00070490
void WrapPlayerPosition(void)
{
    short i;
    Boolean wrapped;

    wrapped = 0;
    wrapDelta.x = 0.0f;
    wrapDelta.y = 0.0f;
    if (shipData->pos.x > kWrapTrigger) {
        wrapped = 1;
        wrapDelta.x = -kWrapStep;
    } else if (shipData->pos.x < -kWrapTrigger) {
        wrapped = 1;
        wrapDelta.x = kWrapStep;
    }
    if (shipData->pos.y > kWrapTrigger) {
        wrapped = 1;
        wrapDelta.y = -kWrapStep;
    } else if (shipData->pos.y < -kWrapTrigger) {
        wrapped = 1;
        wrapDelta.y = kWrapStep;
    }
    if (!wrapped)
        return;

    for (i = 1; i < kMaxShips; i++)
        WrapObject(&shipData[i].pos);
    for (i = 0; i < kMaxShots; i++)
        WrapObject(&shotData[i].pos);
    for (i = 0; i < kMaxExplods; i++)
        WrapObject(&explodData[i].pos);
    for (i = 0; i < kMaxBoxes; i++)
        WrapObject(&boxData[i].pos);
    for (i = 0; i < kMaxSmoke; i++)
        WrapObject(&smokeData[i].pos);
    for (i = 0; i < kMaxPods; i++)
        WrapObject(&podData[i].pos);

    for (i = 0; i < kMaxBeams; i++) {
        beamData[i].head.h += wrapDelta.x;
        beamData[i].head.v += wrapDelta.y;
        beamData[i].tail.h += wrapDelta.x;
        beamData[i].tail.v += wrapDelta.y;
    }

    shipData->pos.x += wrapDelta.x;
    shipData->pos.y += wrapDelta.y;
    lastFramePlayerPos.x += wrapDelta.x;
    lastFramePlayerPos.y += wrapDelta.y;
}

#define kWrapRange 15000

// FUNCTION: EVNOVA 0x0045c6d0
// MAC: _WrapObject i386 0x00066065 ppc 0x0006e268
void WrapObject(FloatPoint *pos)
{
    int32_t dx;
    int32_t dy;

    dx = (pos->x - shipData->pos.x);
    dy = (pos->y - shipData->pos.y);
    if (dx < 0)
        dx = -dx;
    if (dy < 0)
        dy = -dy;
    if (dx < kWrapRange && dy < kWrapRange) {
        pos->x += wrapDelta.x;
        pos->y += wrapDelta.y;
    }
}

/*
 * The flags keep DAT_ names on purpose: the mac reaches each one through its own
 * pointer slot, and those slots are not in the 1.1.1 symbol table. The two tail
 * loops are the mac's - its twin walks 0xdee1c for 5 bytes and then for 4.
 */

// FUNCTION: EVNOVA 0x0045c7a0
// MAC: _SetAllKeyCmdFlags i386 0x00061e6d ppc 0x00069cc8
void SetAllKeyCmdFlags(void)
{
    short i;

    navModeSelectFlag = 1;
    navTargetSelectFlag = 1;
    navDefaultSelectFlag = 1;
    helpKeyFlag = 1;
    cloakFlag = 1;
    fighterCommandFlag = 1;
    weapModeSelectFlag = 1;
    weapTargetSelectFlag = 1;
    weapDefaultSelectFlag = 1;
    playerInfoFlag = 1;
    mapTryFlag = 1;
    jumpTryFlag = 1;
    weapSelectFlag = 1;
    spawnShipFlag = 1;
    weapCloseTargFlag = 1;
    missionInfoFlag = 1;
    commTryFlag = 1;
    mapZoomFlag = 1;
    framerateFlag = 1;
    escortMenuFlag = 1;
    pauseKeyFlag = 1;
    speedUpFlag = 1;
    ackCommFlag = 1;
    for (i = 0; i < 5; i++)
        escortGroupFlag[i] = 1;
    for (i = 0; i < 4; i++)
        escortCommandFlag[i] = 1;
}

/* a lock key as its lock state; any other key on and off by its presses in flight */
Boolean SpeedUpKeyOn(void)
{
    int lock = PlatKeyLockState((int)keyPrefs.speedUp);

    return lock >= 0 ? lock != 0 : speedUpToggled;
}

#define kStrEscortPrefix    0x86   /* + the AI-type filter's own name */
#define kStrFilterFighters  0x87
#define kStrFilterWarships  0x88
#define kStrFilterFreighters 0x89
#define kStrFilterEscorts   0x8a
#define kStrFilterAll       0x8b
#define kStrLoudBase        0x96   /* the five orders, said out loud */
#define kStrQuietBase       0x9b   /* the same five, said under fire */
#define kStrAckAttack       0x9a
#define kStrAckAttackEscort 0x9f

#define kEscortAckFrames    250    /* an escort's acknowledgement stays up 250 frames */

// FUNCTION: EVNOVA 0x0045c880
// MAC: _IssueNewEscortCommand i386 0x000660d5 ppc 0x0006e2f8
Boolean IssueNewEscortCommand(short aiTypeFilter, short cmd, Boolean quiet)
{
    Boolean changed;
    Boolean sawVoice;
    short i;
    short govt;
    short voiceClass;
    short spoken;
    Str255 msg;

    changed = 0;
    sawVoice = 0;
    spoken = cmd;
    govt = -2;
    voiceClass = -1;
    i = 1;
    for (; i < kMaxShips; i++) {
        if (shipData[i].active == 0)
            continue;
        if (shipData[i].parentShip != 0)
            continue;
        if (aiTypeFilter == shipClass[shipData[i].type].escortType ||
            aiTypeFilter == -1) {
            if (cmd != shipData[i].escortOrder) {
                shipData[i].escortMayAttack = 1;
                if (shipData[i].aiType != kShipTypeFighter) {
                    if ((int32_t)cmd == kOrderRegroup) {
                        if ((int32_t)shipData[i].escortOrder != kOrderFormation) {
                            shipData[i].escortOrder = kOrderFormation;
                            spoken = kOrderFormation;
                            changed = 1;
                            if ((shipClass[shipData[i].type].flags2 &
                                 kShipNoFighterVoice) == 0)
                                sawVoice = 1;
                        }
                    } else {
                        shipData[i].escortOrder = cmd;
                        changed = 1;
                        if ((shipClass[shipData[i].type].flags2 & kShipNoFighterVoice) == 0)
                            sawVoice = 1;
                    }
                } else {
                    shipData[i].escortOrder = cmd;
                    changed = 1;
                    if ((shipClass[shipData[i].type].flags2 & kShipNoFighterVoice) == 0)
                        sawVoice = 1;
                }
            }
            if (shipData[i].escortOrder == kOrderAttack && shipData[0].targetShip != -1 &&
                shipData[0].targetShip != shipData[i].targetShip &&
                AlliedWithPlayer(&shipData[shipData[0].targetShip]) == 0) {
                shipData[i].targetShip = shipData[0].targetShip;
                changed = 1;
            }
            if ((int32_t)cmd != kOrderRegroup) {
                if (AIIsFighterReturning(&shipData[i]) != 0) {
                    shipData[i].targetShip = -1;
                    shipData[i].targetStellar = -1;
                    if (govt < -1 || Rand(2) == 0) {
                        govt = shipClass[shipData[i].type].inherentGovtB;
                        voiceClass = shipData[i].voiceClass;
                    }
                    changed = 1;
                }
            }
            if (changed != 0 &&
                (shipClass[shipData[i].type].flags2 & kShipNoFighterVoice) == 0 &&
                (govt < -1 || Rand(2) == 0)) {
                govt = shipClass[shipData[i].type].inherentGovtB;
                voiceClass = shipData[i].voiceClass;
            }
        }
    }
    if (changed != 0 && quiet == 0) {
        strcpy((char *)msg, "");
        StrcatIndCString((char *)msg, kGameStrList, kStrEscortPrefix);
        if (aiTypeFilter == 0)
            StrcatIndCString((char *)msg, kGameStrList, kStrFilterFighters);
        else if (aiTypeFilter == 1)
            StrcatIndCString((char *)msg, kGameStrList, kStrFilterWarships);
        else if (aiTypeFilter == 2)
            StrcatIndCString((char *)msg, kGameStrList, kStrFilterFreighters);
        else if (aiTypeFilter == 3)
            StrcatIndCString((char *)msg, kGameStrList, kStrFilterEscorts);
        else
            StrcatIndCString((char *)msg, kGameStrList, kStrFilterAll);
        strcat((char *)msg, " ");
        if (shipData[0].jumpTimer > 0.0f) {
            if ((int32_t)spoken == kOrderRegroup)
                StrcatIndCString((char *)msg, kGameStrList, kStrLoudBase);
            else if ((int32_t)spoken == kOrderFormation)
                StrcatIndCString((char *)msg, kGameStrList, kStrLoudBase + 1);
            else if ((int32_t)spoken == kOrderHold)
                StrcatIndCString((char *)msg, kGameStrList, kStrLoudBase + 2);
            else if ((int32_t)spoken == kOrderDefend)
                StrcatIndCString((char *)msg, kGameStrList, kStrLoudBase + 3);
            else if ((int32_t)spoken == kOrderAttack)
                StrcatIndCString((char *)msg, kGameStrList, kStrAckAttack);
        } else {
            if ((int32_t)spoken == kOrderRegroup)
                StrcatIndCString((char *)msg, kGameStrList, kStrQuietBase);
            else if ((int32_t)spoken == kOrderFormation)
                StrcatIndCString((char *)msg, kGameStrList, kStrQuietBase + 1);
            else if ((int32_t)spoken == kOrderHold)
                StrcatIndCString((char *)msg, kGameStrList, kStrQuietBase + 2);
            else if ((int32_t)spoken == kOrderDefend)
                StrcatIndCString((char *)msg, kGameStrList, kStrQuietBase + 3);
            else if ((int32_t)spoken == kOrderAttack) {
                if (shipData[0].targetShip != -1) {
                    if (shipData[shipData[0].targetShip].parentShip == 0)
                        StrcatIndCString((char *)msg, kGameStrList, kStrAckAttack);
                    else
                        StrcatIndCString((char *)msg, kGameStrList, kStrAckAttackEscort);
                } else
                    StrcatIndCString((char *)msg, kGameStrList, kStrAckAttack);
            }
        }
        CtoPstr((char *)msg);
        DisplayComm((PStrChar *)msg, kEscortAckFrames);
        PlaySound(beepSnd[1], 1, gSoundVolume, gSoundVolume);
        if (sawVoice != 0) {
            if (govt < -1)
                govt = -1;
            if ((int32_t)spoken == kOrderAttack) {
                if (shipData[0].targetShip == -1)
                    SpeakVoiceMessage(0, govt, voiceClass);
                else if (shipData[shipData[0].targetShip].parentShip == 0)
                    SpeakVoiceMessage(0, govt, voiceClass);
                else
                    SpeakVoiceMessage(1, govt, voiceClass);
            } else {
                SpeakVoiceMessage(0, govt, voiceClass);
            }
        }
    }

    return changed;
}

EXTERN_C_END
