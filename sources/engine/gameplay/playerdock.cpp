/* playerdock.cpp -- landing, hypergates, wormholes, docking and boarding */

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

EXTERN_C_BEGIN

/* The jump, hypergate and wormhole flash, drawn black when the Hyperspace Effects
   preference is off. */
void SetFlashColor(const RGBColor *color)
{
    // BUG (CE F22): CE stores the colour without RGBForeColor's NULL-port test; no caller hits it
    CurrentGrafPort->foreColor = settings.noHyperGamma != 0 ? *blackColor : *color;
}

// FUNCTION: EVNOVA 0x00455e10
// MAC: _PlayerLandOnStellar i386 0x000631c4 ppc 0x0006b244
void PlayerLandOnStellar(void)
{
    ShipObj *ship;
    short sys;
    short i;

    ship = shipData;
    playerIsLanded = 1;
    dontMovePlayerAfterLanding = 0;
    PreloadPortDialog(&stellarData[ship->targetStellar], ship->targetStellar);
    ShipStatsToSystemInfo();
    SWHideAllSprites(gSpriteWorld);
    ClearParticles(1);
    EraseParticlesOffscreen(gSpriteWorld);
    SWUpdateSpriteWorld(gSpriteWorld, 1);
    UpdateParticlesInWindow(gSpriteWorld);
    Screen::ComposeView();
    HideCursorSprite();
    RefreshSAT();
    RedrawStatus(1);
    ShowTheCursor();
    if (!settings.noMovies)
        PlayQTMovieFromDesc(ship->targetStellar + kFirstResourceID);
    if (!SpobUninhabited(&stellarData[ship->targetStellar]))
        DoAutoRefuel();
    else
        PlaySound(beepSnd[1], 1, gSoundVolume, gSoundVolume);
    RedrawStatus(1);
    OSCursorReset();
    cursorHidden = 0;
    ShowTheCursor();
    SWSetPortToWindow(gSpriteWorld);
    ship->targetShip = -1;
    absquatulateStr[0] = 0;
    statusRedraw[kRedrawTarg] = 1;
    statusRedraw[kRedrawCargo] = 1;
    DrawStatus();
    SWSetPortToWindow(gSpriteWorld);
    gSpriteWorld->fastBlit = 0;
    ShowTheCursor();
    DoPortDialog(&stellarData[ship->targetStellar], ship->targetStellar);
    gSpriteWorld->fastBlit = 1;
    SWSetPortToWindow(gSpriteWorld);
    RedrawStatus(1);
    ship->vel.x = ship->vel.y = 0;
    if (!dontMovePlayerAfterLanding) {
        if (ship->targetStellar != -1) {
            ship->pos.x = stellarData[ship->targetStellar].pos.h;
            ship->pos.y = stellarData[ship->targetStellar].pos.v;
        } else {
            ship->pos.x = ship->pos.y = 0;
        }
    } else {
        dontMovePlayerAfterLanding = 0;
    }
    ship->accelMag = 0;
    ship->vel.x = ship->vel.y = 0;
    ship->shield = ship->ShieldCapacity();
    ship->armor = ship->ArmorCapacity();
    IncrementGameTime();
    HandleGlobalPriceFlux();
    sys = FindFirstCoLocatedSystem(ship->curSystem);
    if (systemData[sys].exploreLevel < 2)
        systemData[sys].exploreLevel = 2;
    if (systemData[ship->curSystem].exploreLevel < 2)
        systemData[ship->curSystem].exploreLevel = 2;
    AutoSetExploration(ship->curSystem, 0, 2);
    ExploreNebulaeFromSystem(ship->curSystem);
    for (i = 0; i < kMaxActiveMissions; i++) {
        if (currentMissionObjectives[i].active != 0) {
            if ((currentMissions[i].flags & kMissionInfiniteAuxShips) != 0)
                currentMissions[i].auxShipsLeft = currentMissions[i].auxShipCount;
            currentMissions[i].auxShipDelay = Rand(0x46) + 0x46;
            currentMissions[i].auxShipsSpawned = 0;
        }
    }
    SavePilotFile(ship->targetStellar);
    ship->heading = Rand(kFullCircle);
    SetAllKeyCmdFlags();
    playerIsLanded = 0;
    if (absquatulateStr[0] != 0) {
        DisplayComm((PStrChar *)absquatulateStr, 500);
        absquatulateStr[0] = 0;
    } else if (newbieFlag > 2) {
        unsigned char msg[256];
        unsigned char name[256];
        short n;

        n = Rand(5);
        strcpy((char *)msg, "");
        StrcatIndCString((char *)msg, kGameStrList, n + 0x37);
        strcat((char *)msg, " ");
        strncpy((char *)name, (char *)stellarData[dockTarget].name, 0x3f);
        PtoCstr(name);
        strcat((char *)msg, (char *)name);
        strcat((char *)msg, " ");
        StrcatIndCString((char *)msg, kGameStrList, 0x3c);
        strcat((char *)msg, " ");
        DateToLongString(gameTime, (char *)name);
        strcat((char *)msg, (char *)name);
        strcat((char *)msg, ".");
        CtoPstr((char *)msg);
        DisplayComm((PStrChar *)msg, kCommFrames);
    } else {
        newbieFlag = -1;
    }
    dockTarget = -1;
    dockPermission = -1;
    gSpeedMult = 0.0;
    killShots = 1;
    SWHideAllSprites(gSpriteWorld);
    for (i = 0; i < kMaxShots; i++)
        shotData[i].life = -2.0f;
    lastMouseMoveTime = ReadTick() - 60;
}


// FUNCTION: EVNOVA 0x00456480
// MAC: _PlayerEnterHypergate i386 0x000637bf ppc 0x0006b87c
short PlayerEnterHypergate(void)
{
    short i;
    short destStellar;
    short destSyst;
    short link;
    short n;
    ShipObj *ship;
    unsigned char msg[256];
    unsigned char name[256];

    ship = shipData;
    shipData->navTargetKind = -1;
    shipData->targetStellar = -1;
    link = StellarNumHyperLinks(&stellarData[dockTarget]);
    if (link == 0)
        return -1;
    ShowTheCursor();
    HideCursorSprite();
    RefreshSAT();
    hypergateMapMode = 1;
    hypergateMapModeStellar = dockTarget;
    DoSystemMap();
    hypergateMapMode = 0;
    hypergateMapModeStellar = -1;
    SetAllKeyCmdFlags();
    if (shipData->navTargetKind == kNavTargetGate && shipData->targetStellar != -1) {
        destSyst = -1;
        for (i = 0; i < kSpobHyperLinks; i++) {
            link = stellarData[dockTarget].hyperLinks[i];
            if (link != -1) {
                n = stellarData[link].system;
                if (n < 0 || n >= kMaxSystems)
                    n = FindSystemFromStellar(link);
                n = FindActiveCoLocatedSystem(n);
                if (n == shipData->targetStellar) {
                    destStellar = stellarData[dockTarget].hyperLinks[i];
                    destSyst = n;
                    break;
                }
            }
        }
        if (destSyst != -1) {
            HideCursorSprite();
            RefreshSAT();
            if (!settings.noMovies)
                PlayQTMovieFromDesc(ship->targetStellar + kFirstResourceID);
            SetAllKeyCmdFlags();
            ship->curSystem = destSyst;
            n = FindFirstCoLocatedSystem(destSyst);
            if (systemData[n].exploreLevel < 1)
                systemData[n].exploreLevel = 1;
            if (systemData[destSyst].exploreLevel < 1)
                systemData[destSyst].exploreLevel = 1;
            mapScroll.h = systemData[destSyst].pos.h;
            mapScroll.v = systemData[destSyst].pos.v;
            ship->pos.x = stellarData[destStellar].pos.h;
            ship->pos.y = stellarData[destStellar].pos.v;
            if (stellarData[destStellar].customSound >= 0
                && stellarData[destStellar].customSound <= kFullCircle - 1)
                ship->heading = stellarData[destStellar].customSound;
            else
                ship->heading = Rand(kFullCircle);
            ship->vel.x = ship->vel.y = 0;
            ship->accelMag = (ship->MaxSpeed() * 0.5);
            Accel(ship->heading, ship->accelMag, &ship->vel);
            View::SetPortToView();
            SetFlashColor(whiteColor);
            PaintRect(&gSpriteWorld->workFrameP->frameRect);
            ForeColor(blackColor);
            Screen::ComposeView();
            ScreenEndScene();
            SWSetPortToWorkArea(gSpriteWorld);
            BlitPixieRectDrawProc(gSpriteWorld->workFrameP, gSpriteWorld->windowFrameP,
                         &gSpriteWorld->workFrameP->frameRect,
                         &gSpriteWorld->windowFrameP->frameRect);
            strcpy((char *)msg, "");
            StrcatIndCString((char *)msg, kGameStrList, 0x2e);
            strcat((char *)msg, " ");
            pstrcpy(name, systemData[destSyst].name);
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
                if (systemData[ship->curSystem].navDefault[i] != -1)
                    n++;
            }
            if (n == 0) {
                strcat((char *)msg, " ");
                StrcatIndCString((char *)msg, kGameStrList, 0x31);
            }
            CtoPstr((char *)msg);
            if (systemData[ship->curSystem].murk < 0)
                ClearStarfield();
            else
                RandomizeStars();
            ScrollStarfield(0.0f, 0.0f);
            for (i = 1; i < kMaxHyperRoute; i++)
                hyperRoute[i] = -1;
            hyperRoute[0] = shipData->curSystem;
            PlaySound(warpOutSnd, 0x32, gSoundVolume, gSoundVolume);
            ClearParticles(1);
            EraseParticlesOffscreen(gSpriteWorld);
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
            DisplayComm((PStrChar *)msg, 0x168);
            killShots = 1;
            killBoxes = 1;
            killExplods = 1;
            killSmoke = 1;
            crashlog_print("travelled through hypergate to system ");
            crashlog_print_num(shipData->curSystem + kFirstResourceID);
            crashlog_print(" (");
            crashlog_print_pstr(systemData[shipData->curSystem].name);
            crashlog_print(")\n");
            shipData->navTargetKind = -1;
            shipData->targetStellar = -1;
            shipData->targetShip = -1;
            hyperRoute[0] = shipData->curSystem;
            for (i = 1; i < kMaxHyperRoute; i++)
                hyperRoute[i] = -1;
            statusRedraw[kRedrawRadar] = 1;
            statusRedraw[kRedrawTarg] = 1;
            statusRedraw[kRedrawNav] = 1;
            RedrawStatus(1);
            return destStellar;
        }
    }
    GetIndString(msg, kGameStrList, 0x32);
    DisplayComm((PStrChar *)msg, 250);
    return -1;
}

// FUNCTION: EVNOVA 0x00456ca0
// MAC: _PlayerEnterWormhole i386 0x00064005 ppc 0x0006c064
short PlayerEnterWormhole(void)
{
    short count;
    short i;
    short destStellar;
    short dest;
    short link;
    short pick;
    short n;
    ShipObj *ship;
    unsigned char msg[256];
    unsigned char name[256];

    ship = shipData;
    dest = -1;
    destStellar = -1;
    if (StellarNumHyperLinks(&stellarData[dockTarget]) > 0) {
        do {
            n = 0;
            for (i = 0; i < kSpobHyperLinks; i++) {
                if (stellarData[dockTarget].hyperLinks[i] != -1) {
                    if (FindActiveCoLocatedSystem(stellarData[stellarData[dockTarget].hyperLinks[i]].system) != -1)
                        n++;
                }
            }
            if (n == 0) {

                strcpy((char *)msg, "");
                StrcatIndCString((char *)msg, kGameStrList, 0x54);
                strcat((char *)msg, " ");
                StrcatIndCString((char *)msg, kGameStrList, 0x56);
                CtoPstr((char *)msg);
                DisplayComm((PStrChar *)msg, 250);
                PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);
                return -1;
            }
            pick = Rand(8);
            i = pick;
            link = stellarData[dockTarget].hyperLinks[i];
            if (link != -1) {
                destStellar = link;
                dest = stellarData[link].system;
            }
        } while (dest == -1);
    } else {
        count = 0;
        for (i = 0; i < kMaxStellars; i++) {
            if (i != dockTarget && stellarData[i].loaded != 0
                && (stellarData[i].flags2 & kSpobWormhole) != 0
                && stellarData[i].system != -1
                && ship->curSystem != stellarData[i].system
                && StellarNumHyperLinks(&stellarData[i]) == 0
                && FindActiveCoLocatedSystem(stellarData[i].system) != -1)
                count++;
        }
        if (count > 0) {
            while (dest == -1) {
                i = Rand(kMaxStellars);
                if (i != dockTarget && stellarData[i].loaded != 0
                    && (stellarData[i].flags2 & kSpobWormhole) != 0
                    && stellarData[i].system != -1
                    && ship->curSystem != stellarData[i].system) {
                    if (StellarNumHyperLinks(&stellarData[i]) == 0
                        && FindActiveCoLocatedSystem(stellarData[i].system) != -1) {
                        destStellar = i;
                        dest = stellarData[i].system;
                    }
                    break;
                }
            }
        } else {

            strcpy((char *)msg, "");
            StrcatIndCString((char *)msg, kGameStrList, 0x54);
            strcat((char *)msg, " ");
            StrcatIndCString((char *)msg, kGameStrList, 0x56);
            CtoPstr((char *)msg);
            DisplayComm((PStrChar *)msg, 250);
            PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);
            return -1;
        }
    }
    if (dest == -1) {
        dest = ship->curSystem;
        destStellar = dockTarget;
    }
    if (!settings.noMovies)
        PlayQTMovieFromDesc(ship->targetStellar + kFirstResourceID);
    RefreshSAT();
    SetAllKeyCmdFlags();
    ship->curSystem = dest;
    n = FindFirstCoLocatedSystem(dest);
    if (systemData[n].exploreLevel < 1)
        systemData[n].exploreLevel = 1;
    if (systemData[dest].exploreLevel < 1)
        systemData[dest].exploreLevel = 1;
    mapScroll.h = systemData[dest].pos.h;
    mapScroll.v = systemData[dest].pos.v;
    ship->pos.x = stellarData[destStellar].pos.h;
    ship->pos.y = stellarData[destStellar].pos.v;
    if (stellarData[destStellar].customSound >= 0
        && stellarData[destStellar].customSound <= 0x167)
        ship->heading = stellarData[destStellar].customSound;
    else
        ship->heading = Rand(kFullCircle);
    ship->vel.x = ship->vel.y = 0;
    ship->accelMag = ship->MaxSpeed() * 0.5;
    Accel(ship->heading, ship->accelMag, &ship->vel);
    View::SetPortToView();
    SetFlashColor(whiteColor);
    PaintRect(&gSpriteWorld->windRect);
    ForeColor(blackColor);
    Screen::ComposeView();
    ScreenEndScene();
    PlaySound(warpOutSnd, 0x32, gSoundVolume, gSoundVolume);
    strcpy((char *)msg, "");
    StrcatIndCString((char *)msg, kGameStrList, 0x2f);
    strcat((char *)msg, " ");
    pstrcpy(name, systemData[dest].name);
    PtoCstr(name);
    strcat((char *)msg, (char *)name);
    strcat((char *)msg, " ");
    StrcatIndCString((char *)msg, kGameStrList, 0x30);
    strcat((char *)msg, " ");
    DateToLongString(gameTime, (char *)name);
    strcat((char *)msg, (char *)name);
    strcat((char *)msg, ".");
    count = 0;
    for (i = 0; i < kNavSlots; i++) {
        if (systemData[ship->curSystem].navDefault[i] != -1)
            count++;
    }
    if (count == 0) {
        strcat((char *)msg, " ");
        StrcatIndCString((char *)msg, kGameStrList, 0x31);
    }
    CtoPstr((char *)msg);
    DisplayComm((PStrChar *)msg, kCommFrames);
    killShots = 1;
    killBoxes = 1;
    killExplods = 1;
    killSmoke = 1;
    killAsteroids = 1;
    if (systemData[ship->curSystem].murk < 0)
        ClearStarfield();
    else
        RandomizeStars();
    ScrollStarfield(0.0f, 0.0f);
    ClearParticles(1);
    EraseParticlesOffscreen(gSpriteWorld);
    for (i = 1; i < kMaxHyperRoute; i++)
        hyperRoute[i] = -1;
    hyperRoute[0] = shipData->curSystem;
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
    return destStellar;
}

// FUNCTION: EVNOVA 0x00457580
// MAC: _HandlePlayerDockRequest i386 0x00066691 ppc 0x0006e890
void HandlePlayerDockRequest(void)
{
    ShipObj *ship = shipData;
    unsigned char msg[256];
    Boolean granted;
    short i;
    int iVal2;
    short qtStellar;
    short sVal2;
    short spawnStellar;
    short commResult;
    unsigned char pbuf[256];
    unsigned int distX;
    unsigned int distY;
    short sVal;

    if (!StellarIsLandable(&stellarData[ship->targetStellar]) ||
        ship->curSystem != stellarData[ship->targetStellar].system ||
        stellarData[ship->targetStellar].loaded == 0)
        goto cantDock;

    iVal2 = stellarData[ship->targetStellar].landingFee;
    if (iVal2 > 0 && stellarData[ship->targetStellar].dominated == 0 &&
        iVal2 > shipData->credits) {
        strcpy((char *)msg, "");
        StrcatIndCString((char *)msg, kGameStrList, 0x3d);
        strcat((char *)msg, " ");
        strcat((char *)msg, creditsCString);
        strcat((char *)msg, " ");
        if (stellarData[ship->targetStellar].flags2 & kSpobHypergate) {
            StrcatIndCString((char *)msg, kGameStrList, 0x3e);
        } else if (stellarData[ship->targetStellar].flags & kSpobIsStation) {
            StrcatIndCString((char *)msg, kGameStrList, 0x3f);
        } else {
            StrcatIndCString((char *)msg, kGameStrList, 0x40);
        }
        CtoPstr((char *)msg);
        DisplayComm((PStrChar *)msg, 0xfa);
        dockTarget = -1;
        dockPermission = -1;
        goto done;
    }
    if (ship->IsCloaked()) {
        GetIndString(msg, kGameStrList, 0x49);
        DisplayComm((PStrChar *)msg, 0xfa);
        PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);
        goto done;
    }
    if (ship->IsDisabled())
        goto done;
    if (ship->IsDying())
        goto done;
    dockLand = stellarData[ship->targetStellar].flags & kSpobIsStation;
    if (stellarData[ship->targetStellar].flags & kSpobUninhabited) {
        dockPermission = 0x2ee;
        ship->aiTimer = 0;
        dockTarget = ship->targetStellar;
        GetIndString(msg, kGameStrList, 0x35);
        DisplayComm((PStrChar *)msg, 0xfa);
    }
    if (dockTarget == ship->targetStellar) {
        float fdy;
        float fdx;

        fdx = FloatAbs(ship->vel.x);
        fdy = FloatAbs(ship->vel.y);
        if (dockPermission >= 0x2ee &&
            ship->aiTimer <= 0.0f &&
            fdx <= 0.75 &&
            fdy <= 0.75) {
            distX = abs((short)(ship->pos.x
                    - stellarData[ship->targetStellar].pos.h));
            distY = abs((short)(ship->pos.y
                    - stellarData[ship->targetStellar].pos.v));
            if (stellarData[ship->targetStellar].sprite->curFrameP) {
                sVal = GetStellarSpriteXSize(ship->targetStellar) * 1.75;
            } else {
                sVal = 0x4b;
            }
            if ((short)distX < sVal && (short)distY < sVal) {

                offeringFromShip = 0;
                dockedPort = &stellarData[ship->targetStellar];
                if (lastMissionStellar != ship->targetStellar || lastMissionStellar < 0) {
                    for (sVal2 = 0; sVal2 < kMaxMissions; sVal2++)
                        missionRandom[sVal2] = Rand(kDailyRollRange) + 1;
                    RandomizeMissionData();
                }
                lastMissionStellar = ship->targetStellar;
                commCount = 0;
                SWSetSpriteVisible(alertSprite, 0);
                alertSpritePending = 0;
                HandleComm();
                DrawStatus();
                KillVoiceSound();
                AdjustSoundVolume();
                if (stellarData[dockTarget].landingFee > 0 && stellarData[dockTarget].dominated == 0) {
                    shipData->credits -= stellarData[dockTarget].landingFee;
                    if (shipData->credits < 0)
                        shipData->credits = 0;
                }
                qtStellar = -1;
                commResult = -1;
                crashlog_print("landing on spob ");
                crashlog_print_num(ship->targetStellar + kFirstResourceID);
                crashlog_print(" (");
                crashlog_print_pstr(stellarData[ship->targetStellar].name);
                crashlog_print(")\n");
                FlushEvents(1, 0); /* BUG: event kinds passed as masks: the Mac flushes no key events */
                FlushEvents(2, 0);
                FlushEvents(3, 0);
                FlushEvents(4, 0);
                FlushEvents(5, 0);
                if (stellarData[ship->targetStellar].flags2 & kSpobHypergate) {
                    qtStellar = PlayerEnterHypergate();
                    if (qtStellar == -1) {
                        dockTarget = -1;
                        dockPermission = 0;
                        granted = 0;
                    } else {
                        granted = 1;
                    }
                } else if (stellarData[ship->targetStellar].flags2 & kSpobWormhole) {
                    commResult = PlayerEnterWormhole();
                    granted = 1;
                } else {
                    PlayerLandOnStellar();
                    granted = 1;
                }
                if (!granted)
                    goto done;

                killShots = 1;
                killBoxes = 1;
                killExplods = 1;
                killSmoke = 1;
                if ((shipClass[ship->type].shanFlags & kShanFoldingParts) != 0 &&
                    (shipClass[ship->type].shanFlags & kShanUnfoldsWhenFiring) != 0) {
                    ship->foldSet = shipClass[ship->type].baseSets - 1;
                    ship->foldDir = 0;
                } else {
                    ship->foldSet = 0;
                    ship->foldDir = 1;
                }
                for (sVal = 0; sVal < kMaxBeams; sVal++) {
                    if (beamData[sVal].state > -1)
                        beamData[sVal].state = -1;
                }
                ship->frameTimer = 0;
                ship->engineGlow = 0;
                ship->shieldGlow = 0;
                ship->weapGlow = 0;
                ship->boarded = 0;
                ship->ionization = 0;
                ship->ionColor = 0;
                statusRedraw[kRedrawRadar] = 1;
                statusRedraw[kRedrawTarg] = 1;
                statusRedraw[kRedrawWeap] = 1;
                statusRedraw[kRedrawNav] = 1;
                statusRedraw[kRedrawShield] = 1;
                statusRedraw[kRedrawCargo] = 1;
                killShots = 1;
                killBoxes = 1;
                killExplods = 1;
                killSmoke = 1;
                escortMenuOnscreen = 0;
                dockPermission = 0;
                ship->targetShip = -1;
                antiEscortDelay = Rand(0x1e) + 0x1e;
                DeleteAllComputerShips(0, 1);
                ResetPlayerPrecalcedValues();
                if (systemData[ship->curSystem].murk < 0)
                    ClearStarfield();
                LoadIntfResource();
                SWSetPortToBackground(gSpriteWorld);
                SetSystemBackgroundColor();
                PaintRect(&gSpriteWorld->windRect);
                ForeColor(blackColor);
                SWSetPortToWindow(gSpriteWorld);
                SWFlagRectAsChanged(gSpriteWorld, &gSpriteWorld->windRect);
                if (qtStellar != -1 || commResult != -1) {
                    MissionHandlePlayerEnteredNewSystem();
                    for (sVal = 0; sVal < kMaxActiveMissions; sVal++) {
                        if (currentMissionObjectives[sVal].active != 0 &&
                            currentMissions[sVal].shipSyst == -6 &&
                            currentMissions[sVal].shipCount > 0) {
                            if (currentMissions[sVal].shipBehav == kMissionModeEscort) {
                                currentMissions[sVal].shipDelay = -1;
                                currentMissions[sVal].specialShipsJumpedIn = 0;
                            } else {
                                currentMissions[sVal].shipDelay = 0x7fff;
                                currentMissions[sVal].specialShipsJumpedIn =
                                    currentMissions[sVal].shipCount;
                                if (currentMissions[sVal].shipBehav == kMissionModeAttackPlayer) {
                                    if (dudeData[currentMissions[sVal].shipDude].govt != -1) {
                                        if (qtStellar != -1 &&
                                            (govtData[dudeData[currentMissions[sVal].shipDude].govt].flags2 & kGovtNoHypergates) == 0) {
                                            if ((govtData[dudeData[currentMissions[sVal].shipDude].govt].flags2 & kGovtPrefersHypergates) != 0 ||
                                                Rand(2) == 0) {
                                                currentMissions[sVal].shipDelay = 0;
                                                currentMissions[sVal].specialShipsJumpedIn = 0;
                                            }
                                        } else if (commResult != -1 &&
                                                   ((govtData[dudeData[currentMissions[sVal].shipDude].govt].flags2 & kGovtPrefersWormholes) != 0 ||
                                                    Rand(4) == 0)) {
                                            currentMissions[sVal].shipDelay = 0;
                                            currentMissions[sVal].specialShipsJumpedIn = 0;
                                        }
                                    }
                                }
                            }
                        }
                        if (currentMissions[sVal].auxShipSyst == -1)
                            currentMissions[sVal].auxShipDelay = 0x7fff;
                    }
                    SetupShipsInSystem(ship->curSystem, 0);
                    EnterMoreShips(ship->curSystem);
                    if (qtStellar != -1)
                        spawnStellar = qtStellar;
                    else
                        spawnStellar = commResult;
                    for (sVal2 = 0; sVal2 < kMaxActiveMissions; sVal2++) {
                        if (currentMissionObjectives[sVal2].active != 0 &&
                            currentMissions[sVal2].shipSyst == -6 &&
                            currentMissions[sVal2].shipCount > 0 &&
                            currentMissions[sVal2].shipBehav == kMissionModeAttackPlayer &&
                            currentMissions[sVal2].shipDelay == -1) {
                            for (i = 1; i < kMaxShips; i++) {
                                if (shipData[i].active != 0 &&
                                    sVal2 == shipData[i].specialMission) {
                                    shipData[i].vel.x = 0;
                                    shipData[i].vel.y = 0;
                                    shipData[i].accelMag = 0;
                                    shipData[i].grudge = -1;
                                    AIMakeShipEmergeFromHyperGate(&shipData[i], spawnStellar);
                                    shipData[i].pos.x = stellarData[spawnStellar].pos.h;
                                    shipData[i].pos.y = stellarData[spawnStellar].pos.v;
                                    shipData[i].heading = ship->heading;
                                    shipData[i].aiTimer = (Rand(0x32) + 100);
                                }
                            }
                        }
                    }
                } else {
                    SetupShipsInSystem(ship->curSystem, 1);
                }
                HandleStellarSystemVisibility();
                ship->targetStellar = -1;
                CueNextHyperRouteDest();
                SpawnBountyHunter();
                if (qtStellar != -1 || commResult != -1) {
                    if (qtStellar != -1)
                        spawnStellar = qtStellar;
                    else
                        spawnStellar = commResult;
                    for (sVal = 1; sVal < kMaxShips; sVal++) {
                        if (shipData[sVal].active != 0 &&
                            shipData[sVal].parentShip == 0) {
                            AIMakeShipEmergeFromHyperGate(&shipData[sVal], spawnStellar);
                            shipData[sVal].pos.x = stellarData[spawnStellar].pos.h;
                            shipData[sVal].pos.y = stellarData[spawnStellar].pos.v;
                            shipData[sVal].heading = ship->heading;
                            shipData[sVal].aiTimer = (Rand(0x14) + 0xf);
                        }
                    }
                }
                for (sVal = 0; sVal < kMaxStellars; sVal++) {
                    stellarData[sVal].curFrame = 0;
                    stellarData[sVal].nextFrame = 0;
                }
                if (qtStellar != -1) {
                    stellarData[qtStellar].curFrame = stellarData[qtStellar].numFrames - 1;
                    stellarData[qtStellar].nextFrame = stellarData[qtStellar].curFrame;
                    stellarData[qtStellar].frameTimer = -stellarData[qtStellar].animDelay;
                }
                CreateAsteroids();
                PrepareAllSprites();
                HandleStellarSystemVisibility();
                PrepareAllSprites();
                RefreshSAT();
                RedrawStatus(1);
                AdjustSoundVolume();
                frameCounter = -0xf;
                return;
            } else {
                PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);
                if (ship->targetStellar < 0)
                    goto done;
                if (stellarData[ship->targetStellar].flags2 & kSpobHypergate) {
                    GetIndString(msg, kGameStrList, 0x41);
                } else if (stellarData[ship->targetStellar].flags2 & kSpobWormhole) {
                    GetIndString(msg, kGameStrList, 0x42);
                } else if (dockLand != 0) {
                    GetIndString(msg, kGameStrList, 0x43);
                } else {
                    GetIndString(msg, kGameStrList, 0x44);
                }
                DisplayComm((PStrChar *)msg, 0x168);
                goto done;
            }
        } else {
            PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);
            if (ship->targetStellar < 0 || ship->targetStellar >= kMaxStellars)
                goto done;
            if (!stellarData[ship->targetStellar].sprite->curFrameP) {
                sVal = 0x4b;
            } else {
                sVal = GetStellarSpriteXSize(ship->targetStellar) * 1.75;
            }
            distX = abs((short)(ship->pos.x
                    - stellarData[ship->targetStellar].pos.h));
            distY = abs((short)(ship->pos.y
                    - stellarData[ship->targetStellar].pos.v));
            if ((short)distX < sVal && (short)distY < sVal &&
                dockPermission >= 0x2ee) {
                if (stellarData[ship->targetStellar].flags2 & kSpobHypergate) {
                    GetIndString(msg, kGameStrList, 0x45);
                } else if (stellarData[ship->targetStellar].flags2 & kSpobWormhole) {
                    GetIndString(msg, kGameStrList, 0x46);
                } else if (dockLand != 0) {
                    GetIndString(msg, kGameStrList, 0x47);
                } else {
                    GetIndString(msg, kGameStrList, 0x48);
                }
                DisplayComm((PStrChar *)msg, 0x168);
            } else {
                if (stellarData[ship->targetStellar].flags2 & kSpobHypergate) {
                    GetIndString(msg, kGameStrList, 0x41);
                } else if (stellarData[ship->targetStellar].flags2 & kSpobWormhole) {
                    GetIndString(msg, kGameStrList, 0x42);
                } else if (dockLand != 0) {
                    GetIndString(msg, kGameStrList, 0x43);
                } else {
                    GetIndString(msg, kGameStrList, 0x44);
                }
                DisplayComm((PStrChar *)msg, 0x168);
            }
            goto done;
        }
    }

    /* dockTarget != stellar: the player has just asked to dock here. */
    dockTarget = ship->targetStellar;
    distX = abs((short)(ship->pos.x - stellarData[dockTarget].pos.h));
    distY = abs((short)(ship->pos.y - stellarData[dockTarget].pos.v));
    granted = 0;
    if ((playerRecord[ship->curSystem] >= stellarData[dockTarget].minStatus ||
         stellarData[dockTarget].minStatus == -0x7fff) &&
        stellarData[dockTarget].minStatus != 0x7fff)
        granted = 1;
    if (stellarData[dockTarget].dominated != 0)
        granted = 1;
    if (dockPermission >= 0x2ee)
        granted = 1;
    if (stellarData[dockTarget].govt != -1 &&
        !PlayerMeetsRequirements(
            govtData[stellarData[dockTarget].govt].requires[0],
            govtData[stellarData[dockTarget].govt].requires[1]))
        granted = 0;
    if (!granted) {
        for (sVal = 0; sVal < kMaxActiveMissions; sVal++) {
            if (currentMissionObjectives[sVal].active != 0) {
                if (currentMissions[sVal].travelStellar == shipData->targetStellar)
                    granted = 1;
                if (shipData->targetStellar == currentMissions[sVal].returnStellar)
                    granted = 1;
            }
        }
    }
    if (!granted && stellarData[dockTarget].govt != -1 &&
        PlayerBlessedByGovt(stellarData[dockTarget].govt, 1))
        granted = 1;
    if (granted) {
        if ((short)distX < 0xfa && (short)distY < 0xfa) {
            dockPermission = 0x2ed;
            goto done;
        }
        dockPermission = 0;
        PlaySound(beepSnd[1], 1, gSoundVolume, gSoundVolume);
        strcpy((char *)msg, "");
        if (stellarData[dockTarget].flags2 & kSpobHypergate) {
            StrcatIndCString((char *)msg, kGameStrList, Rand(2) + 0x4a);
        } else if (stellarData[dockTarget].flags & kSpobIsStation) {
            if (Rand(3) == 0) {
                strncpy((char *)msg, (char *)stellarData[shipData->targetStellar].name, 0x1f);
                PtoCstr(msg);
                strcat((char *)msg, " ");
                StrcatIndCString((char *)msg, kGameStrList, 0x4c);
            } else {
                StrcatIndCString((char *)msg, kGameStrList, 0x4d);
            }
        } else {
            if (Rand(3) == 0) {
                strncpy((char *)msg, (char *)stellarData[shipData->targetStellar].name, 0x1f);
                PtoCstr(msg);
                strcat((char *)msg, " ");
                StrcatIndCString((char *)msg, kGameStrList, 0x4e);
            } else {
                StrcatIndCString((char *)msg, kGameStrList, 0x4f);
            }
        }
        if (Rand(2) == 0) {
            strncpy((char *)pbuf, (char *)shipName, 0x40);
            PtoCstr(pbuf);
            strcat((char *)msg, ", ");
            strcat((char *)msg, (char *)pbuf);
        }
        strcat((char *)msg, ". ");
        StrcatIndCString((char *)msg, kGameStrList, 0x50);
        CtoPstr((char *)msg);
        DisplayComm((PStrChar *)msg, 0xfa);
        goto done;
    } else {
        PlaySound(beepSnd[1], 1, gSoundVolume, gSoundVolume);
        if (stellarData[dockTarget].flags2 & kSpobHypergate) {
            GetIndString(msg, kGameStrList, 0x51);
        } else if (stellarData[dockTarget].flags & kSpobIsStation) {
            GetIndString(msg, kGameStrList, 0x52);
        } else {
            GetIndString(msg, kGameStrList, 0x53);
        }
        DisplayComm((PStrChar *)msg, 0xfa);
        dockTarget = -1;
        dockPermission = -1;
        goto done;
    }

cantDock:
    {

        strcpy((char *)msg, "");
    }
    StrcatIndCString((char *)msg, kGameStrList, 0x54);
    strcat((char *)msg, " ");
    if (stellarData[ship->targetStellar].flags2 & kSpobHypergate) {
        StrcatIndCString((char *)msg, kGameStrList, 0x55);
    } else if (stellarData[ship->targetStellar].flags2 & kSpobWormhole) {
        StrcatIndCString((char *)msg, kGameStrList, 0x56);
    } else if (stellarData[ship->targetStellar].flags & kSpobIsStation) {
        StrcatIndCString((char *)msg, kGameStrList, 0x57);
    } else {
        StrcatIndCString((char *)msg, kGameStrList, 0x58);
    }
    if ((stellarData[ship->targetStellar].flags2 & (kSpobHypergate | kSpobWormhole)) == 0) {
        strcat((char *)msg, " ");
        pstrcpy(pbuf, stellarData[ship->targetStellar].name);
        PtoCstr(pbuf);
        strcat((char *)msg, (char *)pbuf);
        strcat((char *)msg, ". ");
        if (stellarData[ship->targetStellar].flags & kSpobIsStation)
            StrcatIndCString((char *)msg, kGameStrList, 0x59);
        else
            StrcatIndCString((char *)msg, kGameStrList, 0x5a);
    }
    CtoPstr((char *)msg);
    DisplayComm((PStrChar *)msg, 0xfa);
    PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);

done:
    ;
}

/* STR# 2002 indexes */
#define kStrAutoClearance   53      /* uninhabited automated clearance */
#define kStrGreetSpecialA   91      /* flags2 & 0x1000 greetings */
#define kStrGreetSpecialB   92
#define kStrGreetSpecialC   93
#define kStrGreetBigA       94      /* flags & 0x10 greetings */
#define kStrGreetBigB       95
#define kStrGreetBigC       96
#define kStrGreetSmallA     97      /* other inhabited greetings */
#define kStrGreetSmallB     98
#define kStrGreetSmallC     99
#define kStrWelcomeShort    100
#define kStrWelcomeTo       101
#define kStrFeeSpecial      102     /* fee sentence lead-ins */
#define kStrFeeBig          103
#define kStrFeeSmall        104
#define kStrFeeTail         105

// FUNCTION: EVNOVA 0x00459950
// MAC: _HandlePlayerDockApproach i386 0x000647a6 ppc 0x0006c824
void HandlePlayerDockApproach(void)
{
    short govt;
    Boolean canDock;
    Boolean inhabited;
    ShipObj *me;
    int32_t targ;
    short i;
    short dx;
    short dy;
    short r;
    unsigned char msg[256];
    unsigned char tmp[256];

    me = shipData;
    targ = dockTarget;
    if (stellarData[targ].system == shipData[0].curSystem) {
        if (dockPermission <= -1)
            goto out;
        inhabited = !(stellarData[targ].flags & kSpobUninhabited);
        canDock = 0;
        govt = stellarData[targ].govt;
        if (govt == -1)
            canDock = 1;
        else if (govt != -1) {
            if ((playerRecord[me->curSystem] >= stellarData[targ].minStatus ||
                 stellarData[targ].minStatus == -32767) &&
                stellarData[targ].minStatus != 32767)
                canDock = 1;
        }
        if (stellarData[targ].dominated)
            canDock = 1;
        if (dockPermission >= 750)
            canDock = 1;
        if (!canDock) {
            for (i = 0; i < kMaxActiveMissions; i++) {
                if (currentMissionObjectives[i].active) {
                    if (currentMissions[i].travelStellar == shipData[0].targetStellar)
                        canDock = 1;
                    if (shipData[0].targetStellar == currentMissions[i].returnStellar)
                        canDock = 1;
                }
            }
        }
        if (!canDock) {
            if (govt != -1 && PlayerBlessedByGovt(govt, 1))
                canDock = 1;
        }
        if (inhabited && canDock) {
            if (dockPermission < 749) ;
            else
                dockPermission++;
            dx = abs((short)(me->pos.x - stellarData[shipData[0].targetStellar].pos.h));
            dy = abs((short)(me->pos.y - stellarData[shipData[0].targetStellar].pos.v));
            if (dx < 250 && dy < 250 && dockPermission < 750)
                dockPermission = 750;
            if (dockPermission != 750) ;
            else {
                PlaySound(beepSnd[1], 1, gSoundVolume, gSoundVolume);
                strcpy((char *)msg, "");
                if (stellarData[shipData[0].targetStellar].flags2 & kSpobHypergate) {
                    r = Rand(3);
                    if (r == 0) {
                        StrcatIndCString((char *)msg, kGameStrList, kStrGreetSpecialA);
                        strcat((char *)msg, ", ");
                        strncpy((char *)tmp, (char *)shipName, 0x40);
                        PtoCstr(tmp);
                        strcat((char *)msg, (char *)tmp);
                        strcat((char *)msg, ". ");
                    } else if (r == 1) {
                        strncpy((char *)msg, (char *)shipName, 0x40);
                        PtoCstr(msg);
                        strcat((char *)msg, ", ");
                        StrcatIndCString((char *)msg, kGameStrList, kStrGreetSpecialB);
                    } else
                        StrcatIndCString((char *)msg, kGameStrList, kStrGreetSpecialC);
                    CtoPstr((char *)msg);
                } else {
                    if (stellarData[shipData[0].targetStellar].flags & kSpobIsStation) {
                        r = Rand(3);
                        if (r == 0) {
                            StrcatIndCString((char *)msg, kGameStrList, kStrGreetBigA);
                            strcat((char *)msg, ", ");
                            strncpy((char *)tmp, (char *)shipName, 0x40);
                            PtoCstr(tmp);
                            strcat((char *)msg, (char *)tmp);
                            strcat((char *)msg, ". ");
                        } else if (r == 1) {
                            strncpy((char *)msg, (char *)shipName, 0x40);
                            PtoCstr(msg);
                            strcat((char *)msg, ", ");
                            StrcatIndCString((char *)msg, kGameStrList, kStrGreetBigB);
                            strcat((char *)msg, " ");
                        } else {
                            StrcatIndCString((char *)msg, kGameStrList, kStrGreetBigC);
                            strcat((char *)msg, " ");
                        }
                    } else {
                        r = Rand(3);
                        if (r == 0) {
                            StrcatIndCString((char *)msg, kGameStrList, kStrGreetSmallA);
                            strcat((char *)msg, ", ");
                            strncpy((char *)tmp, (char *)shipName, 0x40);
                            PtoCstr(tmp);
                            strcat((char *)msg, (char *)tmp);
                            strcat((char *)msg, ". ");
                        } else if (r == 1) {
                            strncpy((char *)msg, (char *)shipName, 0x40);
                            PtoCstr(msg);
                            strcat((char *)msg, ", ");
                            StrcatIndCString((char *)msg, kGameStrList, kStrGreetSmallB);
                            strcat((char *)msg, " ");
                        } else {
                            StrcatIndCString((char *)msg, kGameStrList, kStrGreetSmallC);
                            strcat((char *)msg, " ");
                        }
                    }
                    if (Rand(2) == 0) {
                        StrcatIndCString((char *)msg, kGameStrList, kStrWelcomeShort);
                    } else {
                        StrcatIndCString((char *)msg, kGameStrList, kStrWelcomeTo);
                        strcat((char *)msg, " ");
                        strncpy((char *)tmp, (char *)stellarData[shipData[0].targetStellar].name, 0x1f);
                        PtoCstr(tmp);
                        strcat((char *)msg, (char *)tmp);
                        strcat((char *)msg, ". ");
                    }
                    if (stellarData[dockTarget].landingFee > 0 &&
                        stellarData[dockTarget].dominated == 0) {
                        strcat((char *)msg, "  ");
                        if (stellarData[dockTarget].flags2 & kSpobHypergate)
                            StrcatIndCString((char *)msg, kGameStrList, kStrFeeSpecial);
                        else if (stellarData[dockTarget].flags & kSpobIsStation)
                            StrcatIndCString((char *)msg, kGameStrList, kStrFeeBig);
                        else
                            StrcatIndCString((char *)msg, kGameStrList, kStrFeeSmall);
                        strcat((char *)msg, " ");
                        StrcatCommaNumber(stellarData[dockTarget].landingFee, (char *)msg);
                        strcat((char *)msg, " ");
                        strcat((char *)msg, creditsCString);
                        StrcatIndCString((char *)msg, kGameStrList, kStrFeeTail);
                    }
                    CtoPstr((char *)msg);
                }
                DisplayComm((PStrChar *)msg, 250);
            }
            if (dockPermission >= 0x800) {
                dockPermission = -1;
                dockTarget = -1;
            }
        } else if (canDock) {
            if (dockPermission < 749) ;
            else
                dockPermission++;
            dx = abs((short)(me->pos.x - stellarData[dockTarget].pos.h));
            dy = abs((short)(me->pos.y - stellarData[dockTarget].pos.v));
            if (dx < 250 && dy < 250 && dockPermission < 750)
                dockPermission = 750;
            if (dockPermission == 750 && dockTryFlag) {
                PlaySound(beepSnd[0], 1, gSoundVolume, gSoundVolume);
                GetIndString(msg, kGameStrList, kStrAutoClearance);
                DisplayComm((PStrChar *)msg, kCommFrames);
            }
            if (dockPermission >= 0x800) {
                dockPermission = -1;
                dockTarget = -1;
            }
        }
    } else {
        dockTarget = -1;
        dockPermission = -1;
    }
out: ;
}

/* STR# 2002 indexes */
#define kStrTookCargo       106     /* "You pick up the" */
#define kStrCargoSome       107
#define kStrCargoTail       108
#define kStrDropoffDone     126
#define kStrEscortJoins     127
#define kStrRefuseYield     128
#define kStrRefuseYieldAlt  129
#define kStrCannotBoard     130
#define kStrNotAligned      131
#define kStrTooFast         132

                                                            /* 0x004694a0 fighter bay */
                                                            /* 0x00466fc0 */

/* per-mission state, stride 0x14 */

// FUNCTION: EVNOVA 0x0045a3d0
// MAC: _HandlePlayerBoardAttempt i386 0x00065000 ppc 0x0006d078
void HandlePlayerBoardAttempt(void)
{

    unsigned char msg[256];
    unsigned char pbuf[256];
    float limit;
    float dist;
    float limit2;
    ShipObj *me;
    Boolean canBoard;
    Boolean skip;
    short j;
    short m;
    short k;

    me = shipData;
    keyPressedThisFrame = 1;
    if (boardTryFlag)
        goto out;
    if (me->targetShip == -1)
        goto out;
    if (me->IsCloaked())
        goto out;

    boardTryFlag = 1;
    skip = 0;

    if (shipData[me->targetShip].boarded != 0 &&
        (shipData[me->targetShip].specialMission != -1 || shipData[me->targetShip].formerEscort < 0))
    {
reject:
        PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);
        GetIndString(msg, kGameStrList, kStrCannotBoard);
        DisplayComm(msg, 360);
        goto out;
    }
    if (!shipData[me->targetShip].IsDisabled())
        goto reject;
    if (shipData[me->targetShip].active == 0)
        goto reject;
    if (shipData[me->targetShip].curSystem != me->curSystem)
        goto reject;
    if (me->IsDying())
        goto reject;

    if (FloatAbs(shipData[me->targetShip].vel.x - me->vel.x) <= 0.5) ; else
        goto tooFast;
    if (FloatAbs(shipData[me->targetShip].vel.y - me->vel.y) <= 0.5) ; else
        goto tooFast;
    if (FloatAbs(shipData[me->targetShip].pos.x - me->pos.x)
            <= (float)(GetSpriteXSize((SpriteRec *)shipData[me->targetShip].sprite) * 0.5)) ; else
        goto notClose;
    if (FloatAbs(shipData[me->targetShip].pos.y - me->pos.y)
            <= (float)(GetSpriteYSize((SpriteRec *)shipData[me->targetShip].sprite) * 0.5)) ; else
        goto notClose;

    if (abs(AngularDifference(me->heading, shipData[me->targetShip].heading)) > 30) {
        if (abs(AngularDifference(me->heading,
                ((short)shipData[me->targetShip].heading + kHalfCircle) % kFullCircle)) > 30)
            goto out;
    }

    if (shipData[me->targetShip].specialMission != -1) {
        m = shipData[me->targetShip].specialMission;
        if (currentMissionObjectives[m].active) {
            if (currentMissions[m].pickupMode == 2) {
                HideCursorSprite();
                RefreshSAT();
                ShowTheCursor();
                canBoard = DoMissionCargoPickup(currentMissions[m].cargoType, currentMissions[m].cargoQty);
                SetAllKeyCmdFlags();
                if (canBoard) {
                    if (shipData[me->targetShip].specialMission != -1 &&
                        currentMissionObjectives[shipData[me->targetShip].specialMission].active)
                        currentMissions[shipData[me->targetShip].specialMission].specialShipsBoarded++;
                    AIShipBoardedByPlayer(&shipData[me->targetShip]);
                    shipData[me->targetShip].boarded = 1;
                    currentMissions[m].hasCargo = 1;
                    statusRedraw[kRedrawCargo] = 1;
                    strcpy((char *)msg, "");
                    StrcatIndCString((char *)msg, kGameStrList, kStrTookCargo);
                    strcat((char *)msg, " ");
                    if (lcCargoName[currentMissions[m].cargoType][1] != '*') {
                        StrcatIndCString((char *)msg, kGameStrList, kStrCargoSome);
                        strcat((char *)msg, " ");
                    }
                    pstrcpy(pbuf, lcCargoName[currentMissions[m].cargoType]);
                    PtoCstr(pbuf);
                    if (lcCargoName[currentMissions[m].cargoType][1] == '*')
                        strcat((char *)msg, (char *)pbuf + 1);
                    else
                        strcat((char *)msg, (char *)pbuf);
                    strcat((char *)msg, " ");
                    StrcatIndCString((char *)msg, kGameStrList, kStrCargoTail);
                    CtoPstr((char *)msg);
                    DisplayComm((PStrChar *)msg, 250);
                    PlaySound(beepSnd[4], 8, gSoundVolume, gSoundVolume);
                } else {
                    skip = 1;
                }
            } else if ((currentMissions[m].shipGoal == kMissionGoalBoard || currentMissions[m].shipGoal == kMissionGoalRescue) &&
                       (currentMissions[m].flags & kMissionAutoAbort) &&
                       currentMissions[m].shipCount == 1) {
                if (currentMissions[m].flags & kMissionTakesFuel) {
                    pstrcpy(msg, shipClass[shipData[me->targetShip].type].name);
                    PtoCstr(msg);
                    strcat((char *)msg, ":  ");
                    pstrcpy(pbuf, fuelString[0]);
                    PtoCstr(pbuf);
                    strcat((char *)msg, (char *)pbuf);
                    strcat((char *)msg, ", ");
                    pstrcpy(pbuf, shipName);
                    PtoCstr(pbuf);
                    strcat((char *)msg, (char *)pbuf);
                    strcat((char *)msg, ".");
                } else {
                    strcpy((char *)msg, "");
                    StrcatIndCString((char *)msg, kGameStrList, kStrDropoffDone);
                }
                CtoPstr((char *)msg);
                DisplayComm((PStrChar *)msg, 250);
                PlaySound(beepSnd[4], 8, gSoundVolume, gSoundVolume);
                if (currentMissionObjectives[shipData[me->targetShip].specialMission].active)
                    currentMissions[shipData[me->targetShip].specialMission].specialShipsBoarded++;
                shipData[me->targetShip].aiTimer = 100.0;
                shipData[me->targetShip].boarded = 1;
                AIShipBoardedByPlayer(&shipData[me->targetShip]);
                canBoard = 1;
            } else {
                canBoard = 1;
            }
        } else {
            canBoard = 1;
        }
    } else {
        if (shipClass[shipData[me->targetShip].type].crew > 0)
            canBoard = 1;
        else
            canBoard = 0;
    }

    if (canBoard == 0)
        goto reject;
    {
        me->vel.x = shipData[me->targetShip].vel.x;
        me->vel.y = shipData[me->targetShip].vel.y;
        j = 0;
        k = me->targetShip;
        m = shipData[k].specialMission;
        if (m == -1)
            j = 1;
        else if (currentMissionObjectives[m].active && currentMissionObjectives[m].failed)
            j = 1;
        if (j == 1 && (j = shipData[me->targetShip].persType) != kPersBountyHunter) {
            SlapWithPenalty(shipData[0].curSystem, shipData[me->targetShip].govt, 2, m);
            if (shipData[me->targetShip].persType != -1) {
                if (personData[shipData[me->targetShip].persType].linkMission != -1) {
                    if (personData[shipData[me->targetShip].persType].flags &
                        kPersOfferWhenBoarded) {
                        offeringFromShip = 1;
                        offeringShipIndex = me->targetShip;
                        if (IsMissionAvailable(personData[shipData[me->targetShip].persType].linkMission, 1)) {
                            HideCursorSprite();
                            RefreshSAT();
                            ShowTheCursor();
                            PlaySound(hardDockSnd, 8, gSoundVolume, gSoundVolume);
                            if (OfferOneMission(personData[shipData[me->targetShip].persType].linkMission) != 1)
                                skip = 1;
                            SetAllKeyCmdFlags();
                            RefreshSAT();
                            if (personData[shipData[me->targetShip].persType].flags &
                                kPersDeactivateOnAccept)
                                personData[shipData[me->targetShip].persType].loaded = 0;
                        } else {
                            PlaySound(hardDockSnd, 8, gSoundVolume, gSoundVolume);
                            HideCursorSprite();
                            RefreshSAT();
                            DoPlunderDialog();
                            SetAllKeyCmdFlags();
                        }
                        offeringFromShip = 0;
                        offeringShipIndex = -1;
                    } else {
                        if (CanBuyFighter(shipData[me->targetShip].type, -1, -1) &&
                            shipData[me->targetShip].formerEscort == 0) {
                            GetIndString(msg, kGameStrList, kStrRefuseYield);
                            DisplayComm((PStrChar *)msg, 250);
                            PlaySound(beepSnd[4], 8, gSoundVolume, gSoundVolume);
                            shipData[me->targetShip].formerEscort = -1;
                            shipData[me->targetShip].aiType = kShipTypeFighter;
                            shipData[me->targetShip].parentShip = 0;
                            shipData[me->targetShip].shield = 0;
                            shipData[me->targetShip].armor = shipData[me->targetShip].ArmorCapacity();
                            shipData[me->targetShip].targetStellar = -1;
                            AIAddFighterToParent(&shipData[me->targetShip]);
                        } else {
                            PlaySound(hardDockSnd, 8, gSoundVolume, gSoundVolume);
                            HideCursorSprite();
                            RefreshSAT();
                            DoPlunderDialog();
                            SetAllKeyCmdFlags();
                        }
                    }
                } else {
                    if (CanBuyFighter(shipData[me->targetShip].type, -1, -1) &&
                        shipData[me->targetShip].formerEscort == 0) {
                        GetIndString(msg, kGameStrList, kStrRefuseYield);
                        DisplayComm((PStrChar *)msg, 250);
                        PlaySound(beepSnd[4], 8, gSoundVolume, gSoundVolume);
                        shipData[me->targetShip].formerEscort = -1;
                        shipData[me->targetShip].aiType = kShipTypeFighter;
                        shipData[me->targetShip].parentShip = 0;
                        shipData[me->targetShip].shield = 0;
                        shipData[me->targetShip].targetStellar = -1;
                        shipData[me->targetShip].armor = shipData[me->targetShip].ArmorCapacity();
                        AIAddFighterToParent(&shipData[me->targetShip]);
                    } else {
                        DoGoodSamaritan(&shipData[me->targetShip], me);
                        PlaySound(hardDockSnd, 8, gSoundVolume, gSoundVolume);
                        HideCursorSprite();
                        RefreshSAT();
                        DoPlunderDialog();
                        SetAllKeyCmdFlags();
                    }
                }
            } else {
                if (shipData[me->targetShip].formerEscort >= 1 && CanHireEscorts()) {
                    shipData[me->targetShip].aiType = kShipTypeEscort;
                    shipData[me->targetShip].parentShip = 0;
                    shipData[me->targetShip].boarded = 0;
                    if (shipData[me->targetShip].formerEscort == 1)
                        shipData[me->targetShip].hired = 1;
                    else
                        shipData[me->targetShip].hired = 0;
                    shipData[me->targetShip].formerEscort = -1;
                    if (shipClass[shipData[me->targetShip].type].flags & kShipArmorDisableTenth)
                        shipData[me->targetShip].armor =
                            shipData[me->targetShip].ArmorCapacity() * 0.1 + 1.0;
                    else
                        shipData[me->targetShip].armor =
                            shipData[me->targetShip].ArmorCapacity() * 0.3333 + 1.0;
                    AISetup(&shipData[me->targetShip]);
                    for (k = 0; k < kCargoTypes; k++)
                        shipData[0].cargo[k] += shipData[me->targetShip].cargo[k];
                    statusRedraw[kRedrawCargo] = 1;
                    GetIndString(msg, kGameStrList, kStrEscortJoins);
                    DisplayComm((PStrChar *)msg, 250);
                    PlaySound(beepSnd[4], 8, gSoundVolume, gSoundVolume);
                } else if (CanBuyFighter(shipData[me->targetShip].type, -1, -1) &&
                           shipData[me->targetShip].formerEscort == 0) {
                    GetIndString(msg, kGameStrList, kStrRefuseYield);
                    DisplayComm((PStrChar *)msg, 250);
                    PlaySound(beepSnd[4], 8, gSoundVolume, gSoundVolume);
                    shipData[me->targetShip].formerEscort = -1;
                    shipData[me->targetShip].aiType = kShipTypeFighter;
                    shipData[me->targetShip].parentShip = 0;
                    shipData[me->targetShip].shield = 0;
                    shipData[me->targetShip].armor = shipData[me->targetShip].ArmorCapacity();
                    shipData[me->targetShip].targetStellar = -1;
                    AIAddFighterToParent(&shipData[me->targetShip]);
                } else if (CanBuyFighter(shipData[me->targetShip].type, -1, -1) &&
                           shipData[me->targetShip].formerEscort == -1) {
                    GetIndString(msg, kGameStrList, kStrRefuseYieldAlt);
                    DisplayComm((PStrChar *)msg, 250);
                    PlaySound(beepSnd[4], 8, gSoundVolume, gSoundVolume);
                    shipData[me->targetShip].formerEscort = -1;
                    shipData[me->targetShip].aiType = kShipTypeFighter;
                    shipData[me->targetShip].parentShip = 0;
                    shipData[me->targetShip].shield = 0;
                    shipData[me->targetShip].armor = shipData[me->targetShip].ArmorCapacity();
                    shipData[me->targetShip].targetStellar = -1;
                    AIAddFighterToParent(&shipData[me->targetShip]);
                } else {
                    DoGoodSamaritan(&shipData[me->targetShip], me);
                    PlaySound(hardDockSnd, 8, gSoundVolume, gSoundVolume);
                    HideCursorSprite();
                    RefreshSAT();
                    DoPlunderDialog();
                    SetAllKeyCmdFlags();
                }
            }
        } else if (m == -1) {
            DoGoodSamaritan(&shipData[me->targetShip], me);
            PlaySound(hardDockSnd, 8, gSoundVolume, gSoundVolume);
            HideCursorSprite();
            RefreshSAT();
            DoPlunderDialog();
            SetAllKeyCmdFlags();
        }
        if (skip)
            goto out;
        if (me->targetShip == -1)
            goto out;
        shipData[me->targetShip].boarded = 1;
        AIShipBoardedByPlayer(&shipData[me->targetShip]);
    }
    goto out;

notClose:
    PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);
    GetIndString(msg, kGameStrList, kStrNotAligned);
    DisplayComm((PStrChar *)msg, 360);
    goto out;
tooFast:
    PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);
    GetIndString(msg, kGameStrList, kStrTooFast);
    DisplayComm((PStrChar *)msg, 360);
    goto out;
out:;
}

EXTERN_C_END
