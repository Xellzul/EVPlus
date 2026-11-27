/* setup.cpp -- SpriteWorld, window and offscreen setup and teardown, sounds, display mode, menu art */

#include <math.h>
#include "burgerplat.h"
#include "canvas.h"
#include "game.h"
#include "glyphatlas.h"
#include "render.h"
#include "stellar.h"
#include "crashlog.h"
#include "dialogs.h"
#include "uiscale.h"
#include "space.h"
#include "ai.h"
#include "screen.h"
#include "prototypes.h"
#include "gameutil.h"
#include "player.h"
#include "pixel.h"
#include "platform.h"
#include "viewscale.h"
#include "prefsdisplay.h"
#include "sound.h"
#include "text.h"
#include "painter.h"
#include "uiart.h"

static void SpriteWorldFailed(void)
{
    Str255 str;

    FadeScreenIn();
    GetIndString(str, kAlertStrList, 36);
    EVNoteAlert(str);
    ST_Close();
    ExitToShell();
}

/* the floating map's and escort menu's frames and places, for the view's size and the UI scale */
static void CreateOverlayFrames(void)
{
    Rect windRect;

    windRect.top = 0;
    windRect.left = 0;
    windRect.right = View::gameWidth * 0.25;
    if (windRect.right < 200)
        windRect.right = 200;
    windRect.bottom = windRect.right;
    floatingMapRect = windRect;
    View::MakeOverlayFrame(&floatingMapFrame, View::OverlayFrameRect(windRect));
    OffsetRect(&floatingMapRect, View::width - (floatingMapRect.right + View::ScaleInView(16)), View::ScaleInView(150));
    SetRect(&windRect, 0, 0, 160, 120);
    View::MakeOverlayFrame(&floatingEscortMenuFrame, windRect);
    SetRect(&escortMenuRect, View::ScaleInView(15), View::ScaleInView(150), View::ScaleInView(15 + 160),
            View::ScaleInView(150 + 120));
}

/* The UI frames for the space view left of the status bar; returns the view's rect at 0,0. */
static Rect ViewFrames(void)
{
    Rect r = mainWindRect;

    r.right -= kStatusBarWidth;
    View::CreateUIFrames(r);
    SetRect(&r, 0, 0, View::width, View::height);
    return r;
}

// FUNCTION: EVNOVA 0x004ab980
// MAC: _SetupSpriteWorld i386 0x00021b3a ppc 0x00025fac
void SetupSpriteWorld(void)
{
    short i;
    short err;
    Rect windRect;

    mBarHidden = 1;
    windRect = ViewFrames();
    err = SWCreateSpriteWorld(&gSpriteWorld, gWind, windRect);
    if ((int32_t)err != 0)
        SpriteWorldFailed();

    SWCreateSpriteLayer(&starLayer);
    SWCreateSpriteLayer(&stellarLayer);
    SWCreateSpriteLayer(&asteroidLayer);
    SWCreateSpriteLayer(&disabledShipLayer);
    SWCreateSpriteLayer(&boxLayer);
    SWCreateSpriteLayer(&smokeLayer);
    SWCreateSpriteLayer(&shotLayer);
    SWCreateSpriteLayer(&guidedShotLayer);
    SWCreateSpriteLayer(&pointDefenseLayer);
    SWCreateSpriteLayer(&escortLayer);
    SWCreateSpriteLayer(&shipLayer);
    SWCreateSpriteLayer(&turretedShotLayer);
    SWCreateSpriteLayer(&explodLayer);
    SWCreateSpriteLayer(&targetLayer);
    SWCreateSpriteLayer(&interfaceLayer);

    SWAddSpriteLayer(gSpriteWorld, starLayer);
    SWAddSpriteLayer(gSpriteWorld, stellarLayer);
    SWAddSpriteLayer(gSpriteWorld, asteroidLayer);
    SWAddSpriteLayer(gSpriteWorld, disabledShipLayer);
    SWAddSpriteLayer(gSpriteWorld, boxLayer);
    SWAddSpriteLayer(gSpriteWorld, smokeLayer);
    SWAddSpriteLayer(gSpriteWorld, pointDefenseLayer);
    SWAddSpriteLayer(gSpriteWorld, guidedShotLayer);
    SWAddSpriteLayer(gSpriteWorld, shotLayer);
    SWAddSpriteLayer(gSpriteWorld, escortLayer);
    SWAddSpriteLayer(gSpriteWorld, shipLayer);
    SWAddSpriteLayer(gSpriteWorld, turretedShotLayer);
    SWAddSpriteLayer(gSpriteWorld, explodLayer);
    SWAddSpriteLayer(gSpriteWorld, targetLayer);
    SWAddSpriteLayer(gSpriteWorld, interfaceLayer);

    crashlog("creating scratch frames");
    CreateOverlayFrames();

    BlackScreen();

    spriteClipRect = floatingEscortMenuFrame->frameRect;
    spriteClipRect.top -= 75;
    spriteClipRect.left -= 75;

    for (i = 0; i < kMaxShipTypes; i++) {
        masterShipSprite[i] = 0;
        masterShipGlowSprite[i] = 0;
        masterShipLightSprite[i] = 0;
        masterShipWeaponSprite[i] = 0;
        masterShipAltSprite[i] = 0;
        masterShipShieldSprite[i] = 0;
    }
    for (i = 0; i < kMasterStellarSprites; i++)
        masterStellarSprite[i] = 0;
    for (i = 0; i < kMaxExplodTypes; i++)
        masterExplodSprite[i] = 0;
    for (i = 0; i < kMaxWeapTypes; i++)
        masterShotSprite[i] = 0;

    if ((int32_t)InitParticles(100000, 0) != 0)
        SWAssertFail("Setup.c", SW_LINE(0x133));
}

/* the middle of the space view */
static void SetScreenCenter(void)
{
    screenCenter.h = View::width * 0.5;
    screenCenter.v = View::height * 0.5;
}

// FUNCTION: EVNOVA 0x004ac380
// MAC: _SetupWind i386 0x0001b462 ppc 0x0001f524
void SetupWind(void)
{
    short top;
    short left;
    short bottom;
    short right;

    fullScreenRestore = 0;
    gWind = OpenGamePort(&mainWindRect, PascalLiteral("EV Nova").text);
    top = mainWindRect.top;
    left = mainWindRect.left;
    bottom = mainWindRect.bottom;
    right = mainWindRect.right;
    crashlog_print("main window rect: (");
    crashlog_print_num(left);
    crashlog_print(", ");
    crashlog_print_num(top);
    crashlog_print(", ");
    crashlog_print_num(right);
    crashlog_print(", ");
    crashlog_print_num(bottom);
    crashlog_print(")\n");
    BlackScreen();
    if (settings.windowed == 0) {
        mBarHidden = 1;
    }
    SetScreenCenter();
}

#define AllocFailed(var, expr) (var = (expr), !(var != NULL))

// FUNCTION: EVNOVA 0x004ac560
// MAC: _InitArrays i386 0x0001b796 ppc 0x0001f8b4
void InitArrays(void)
{

    if (AllocFailed(shipData, (ShipObj *)AllocAPointer(sizeof(ShipObj) * kMaxShips)))
        EV_ASSERT_FAIL("Setup.c", 0x1b6, 747);
    if (AllocFailed(stellarData, (SpobRecord *)AllocAPointer(sizeof(SpobRecord) * kMaxStellars)))
        EV_ASSERT_FAIL("Setup.c", 0x1b7, 748);
    if (AllocFailed(systemData, (SystRecord *)AllocAPointer(sizeof(SystRecord) * kMaxSystems)))
        EV_ASSERT_FAIL("Setup.c", 0x1b8, 749);
    if (AllocFailed(shipClass, (ShipRec *)AllocAPointer(sizeof(ShipRec) * kMaxShipTypes)))
        EV_ASSERT_FAIL("Setup.c", 0x1b9, 750);
    if (AllocFailed(weaponStats, (WeapRecord *)AllocAPointer(sizeof(WeapRecord) * kMaxWeapTypes)))
        EV_ASSERT_FAIL("Setup.c", 0x1ba, 751);
    if (AllocFailed(explodData, (BoomSlot *)AllocAPointer(sizeof(BoomSlot) * kMaxExplods)))
        EV_ASSERT_FAIL("Setup.c", 0x1bb, 752);
    if (AllocFailed(explodType, (BoomRecord *)AllocAPointer(sizeof(BoomRecord) * kMaxExplodTypes)))
        EV_ASSERT_FAIL("Setup.c", 0x1bc, 753);
    if (AllocFailed(smokeData, (DebrisSlot *)AllocAPointer(sizeof(DebrisSlot) * kMaxDebris)))
        EV_ASSERT_FAIL("Setup.c", 0x1bd, 754);
    if (AllocFailed(beamData, (BeamRecord *)AllocAPointer(sizeof(BeamRecord) * kMaxBeams)))
        EV_ASSERT_FAIL("Setup.c", 0x1be, 755);
    if (AllocFailed(outfitSystem, (OutfRecord *)AllocAPointer(sizeof(OutfRecord) * kMaxOutfits)))
        EV_ASSERT_FAIL("Setup.c", 0x1bf, 756);
    if (AllocFailed(dudeData, (DudeRecord *)AllocAPointer(sizeof(DudeRecord) * kMaxDudes)))
        EV_ASSERT_FAIL("Setup.c", 0x1c0, 757);
    if (AllocFailed(govtData, (GovtRecord *)AllocAPointer(sizeof(GovtRecord) * kMaxGovts)))
        EV_ASSERT_FAIL("Setup.c", 0x1c1, 758);
    if (AllocFailed(personData, (PersRecord *)AllocAPointer(sizeof(PersRecord) * kMaxPersons)))
        EV_ASSERT_FAIL("Setup.c", 0x1c2, 759);
    if (AllocFailed(asteroidData, (AsteroidObj *)AllocAPointer(sizeof(AsteroidObj) * kMaxAsteroids)))
        EV_ASSERT_FAIL("Setup.c", 0x1c3, 760);
    if (AllocFailed(disasterData, (OopsRecord *)AllocAPointer(sizeof(OopsRecord) * kMaxDisasters)))
        EV_ASSERT_FAIL("Setup.c", 0x1c4, 761);
    if (AllocFailed(nebulaData, (NebuRecord *)AllocAPointer(sizeof(NebuRecord) * kMaxNebulae)))
        EV_ASSERT_FAIL("Setup.c", 0x1c5, 762);
    if (AllocFailed(fleetData, (FletRecord *)AllocAPointer(sizeof(FletRecord) * kMaxFleets)))
        EV_ASSERT_FAIL("Setup.c", 0x1c6, 763);
    if (AllocFailed(boxData, (BoxObj *)AllocAPointer(sizeof(BoxObj) * kMaxBoxes)))
        EV_ASSERT_FAIL("Setup.c", 0x1c7, 764);
    if (AllocFailed(podData, (PodObj *)AllocAPointer(sizeof(PodObj) * kMaxPods)))
        EV_ASSERT_FAIL("Setup.c", 0x1c8, 765);
    if (AllocFailed(shotData, (ShotObj *)AllocAPointer(sizeof(ShotObj) * kMaxShots)))
        EV_ASSERT_FAIL("Setup.c", 0x1c9, 766);
    if (AllocFailed(junkData, (JunkRecord *)AllocAPointer(sizeof(JunkRecord) * kMaxJunks)))
        EV_ASSERT_FAIL("Setup.c", 0x1ca, 767);
    if (AllocFailed(cronEvent, (CronRecord *)AllocAPointer(sizeof(CronRecord) * kMaxCrons)))
        EV_ASSERT_FAIL("Setup.c", 0x1cb, 768);
    if (AllocFailed(rankData, (RankRecord *)AllocAPointer(sizeof(RankRecord) * kMaxRanks)))
        EV_ASSERT_FAIL("Setup.c", 0x1cc, 769);
    if (AllocFailed(starData, (StarSlot *)AllocAPointer(sizeof(StarSlot) * kMaxStars)))
        EV_ASSERT_FAIL("Setup.c", 0x1cd, 770);
    if (AllocFailed(missionBit, (Boolean *)AllocAPointer(sizeof(Boolean) * kMaxMissionBits)))
        EV_ASSERT_FAIL("Setup.c", 0x1ce, 771);
    if (AllocFailed(currentMissions, (MissionRecord *)AllocAPointer(sizeof(MissionRecord) * kMaxActiveMissions)))
        EV_ASSERT_FAIL("Setup.c", 0x1cf, 772);
    if (AllocFailed(currentMissionObjectives, (MissionState *)AllocAPointer(sizeof(MissionState) * kMaxActiveMissions)))
        EV_ASSERT_FAIL("Setup.c", 0x1d0, 773);
}

Rect mainWindRect;
int32_t fullScreenRestore;
GWorldContainer menuArtPort;            /* MenuArt() */
GWorldContainer menuShownPort;          /* the menu art last stretched into loadPort */

/* the status bar's picture, as high as `screen' */
static void CreateStatusOffPort(Rect screen)
{
    unsigned char msg[256];

    screen.right = screen.left + kStatusBarWidth;
    CreateGWorldContainer(&statusOffPort, GetMainDevice(), 0, &screen);
    if (statusOffPort.world != NULL) {
        UI::ScaleUIPort(statusOffPort.world);
        UI::Canvas::MakeNewTarget(statusOffPort.world);
    }
    if (statusOffPort.world == NULL) {
        ShowTheCursor();
        FadeScreenIn();
        GetIndString(msg, kAlertStrList, 0x27);
        EVNoteAlert(msg);
        ST_Close();
        ExitToShell();
    }
}

// FUNCTION: EVNOVA 0x004ac950
// MAC: _InitOffscreen i386 0x0001bd21 ppc 0x0001fe24
void InitOffscreen(void)
{
    Rect *bounds;
    unsigned char msg[256];
    Rect r;

    bounds = GetCWindowPortRect((WindowRef)gWind);
    r = *bounds;
    OffsetRect(&r, -r.left, -r.top);
    CreateGWorldContainer(&loadPort, GetMainDevice(), 0, &r);
    if (loadPort.world != NULL) {
        UI::ScaleUIPort(loadPort.world);
        UI::Canvas::MakeNewTarget(loadPort.world);
    }
    {
        UI::MenuScale menu;
        if (UI::Enabled()) {
            Rect art = *GetCWindowPortRect((WindowRef)gWind);
            OffsetRect(&art, -art.left, -art.top);
            CreateGWorldContainer(&menuArtPort, GetMainDevice(), 0, &art);
            CreateGWorldContainer(&menuShownPort, GetMainDevice(), 0, &art);
        }
    }
    if (loadPort.world == NULL) {
        ShowTheCursor();
        FadeScreenIn();
        GetIndString(msg, kAlertStrList, 0x27);
        EVNoteAlert(msg);
        ST_Close();
        ExitToShell();
    }
    CreateStatusOffPort(r);
}

#define kVoiceClipsMax           9
#define kVoiceSetStride       0x64
#define kVoiceMsgStride        0xa
#define kFirstVoiceID        0x3e8
#define kDefaultLoopTicks    0x15e
#define kTicksPerSecond         60
#define kFirstEngineID        0x96
#define kEngineSounds            5
#define kFirstShotID          0xc8
#define kShotSounds          0x100
#define kFirstExplodID       0x12c
#define kExplodSounds         0x40
#define kHighPriority        32000

/* 1.1.1 logs no shutdown steps */
#define ByeLog(s) crashlog(s)

// FUNCTION: EVNOVA 0x004acac0
// MAC: _Bye i386 0x00024b6b ppc 0x00029044
void Bye(void)
{
    short i;
    short j;
    Sound *snd;

    ByeLog("doing cleanup");
    FlushEvents(everyEvent - diskMask, 0);

    ByeLog("disposing sounds");
    ST_HaltSound(0);
    if (warpUpSnd[0] != NULL)
        ST_DisposeSound(warpUpSnd[0]);
    snd = warpUpSnd[1];
    if (snd != NULL)
        ST_DisposeSound(snd);
    if (warpOutSnd != NULL)
        ST_DisposeSound(warpOutSnd);
    for (i = 0; i <= kEngineSounds - 1; i++) {
        if (beepSnd[i] != NULL)
            ST_DisposeSound(beepSnd[i]);
    }
    for (i = 0; i < kShotSounds; i++) {
        if (shotSnd[i] != NULL)
            ST_DisposeSound(shotSnd[i]);
    }
    for (i = 0; i < kExplodSounds; i++) {
        if (explodSnd[i] != NULL)
            ST_DisposeSound(explodSnd[i]);
    }
    if (redAlertSnd != NULL)
        ST_DisposeSound(redAlertSnd);
    if (klaxxonSnd != NULL)
        ST_DisposeSound(klaxxonSnd);
    if (escapePodSnd != NULL)
        ST_DisposeSound(escapePodSnd);
    if (cloakSnd[0] != NULL)
        ST_DisposeSound(cloakSnd[0]);
    snd = cloakSnd[1];
    if (snd != NULL)
        ST_DisposeSound(snd);
    if (hardDockSnd != NULL)
        ST_DisposeSound(hardDockSnd);
    if (clickSnd != NULL)
        ST_DisposeSound(clickSnd);
    if (missionSnd != NULL)
        ST_DisposeSound(missionSnd);
    if (voiceSnd != NULL)
        ST_DisposeSound(voiceSnd);

    ByeLog("disposing buttons");
    DisposeButtons();

    ByeLog("disposing sprites");
    for (i = 0; i < kMaxShips; i++) {
        if (shipData[i].sprite)
            DisposeClonedSprite(&shipData[i].sprite);
        if (shipData[i].altSprite)
            DisposeClonedSprite(&shipData[i].altSprite);
        if (shipData[i].glowSprite)
            DisposeClonedSprite(&shipData[i].glowSprite);
        if (shipData[i].lightSprite)
            DisposeClonedSprite(&shipData[i].lightSprite);
        if (shipData[i].weapSprite)
            DisposeClonedSprite(&shipData[i].weapSprite);
        if (shipData[i].shieldSprite)
            DisposeClonedSprite(&shipData[i].shieldSprite);
    }
    for (i = 0; i < kMaxPods; i++) {
        if (podData[i].sprite)
            DisposeClonedSprite(&podData[i].sprite);
    }
    for (i = 0; i < kMaxShipTypes; i++) {
        if (masterShipSprite[i])
            SWDisposeSprite(&masterShipSprite[i]);
        if (masterShipGlowSprite[i])
            SWDisposeSprite(&masterShipGlowSprite[i]);
        if (masterShipLightSprite[i])
            SWDisposeSprite(&masterShipLightSprite[i]);
        if (masterShipWeaponSprite[i])
            SWDisposeSprite(&masterShipWeaponSprite[i]);
        if (masterShipAltSprite[i])
            SWDisposeSprite(&masterShipAltSprite[i]);
        if (masterShipShieldSprite[i])
            SWDisposeSprite(&masterShipShieldSprite[i]);
        if (shipClass[i].frameAngles != NULL)
            DeallocAPointer(shipClass[i].frameAngles);
    }
    for (i = 0; i < kStellarSprites; i++) {
        if (stellarSprite[i] != NULL)
            DeallocAPointer(stellarSprite[i]);
    }
    for (i = 0; i < kMasterStellarSprites; i++) {
        if (masterStellarSprite[i] != NULL) {
            if (masterStellarSprite[i]->frameArray != NULL) {
                for (j = 0; j < masterStellarSprite[i]->numFrames; j++) {
                    SWFreeFrameRecord(masterStellarSprite[i]->frameArray[j]);
                    masterStellarSprite[i]->frameArray[j] = NULL;
                }
                masterStellarSprite[i]->numFrames = 0;
                DeallocAPointer(masterStellarSprite[i]->frameArray);
                masterStellarSprite[i]->frameArray = NULL;
            }
            SWDisposeSprite(&masterStellarSprite[i]);
        }
    }
    for (i = 0; i < kMaxExplods; i++) {
        if (explodData[i].sprite)
            DisposeClonedSprite(&explodData[i].sprite);
    }
    for (i = 0; i < kMaxExplodTypes; i++) {
        if (masterExplodSprite[i])
            SWDisposeSprite(&masterExplodSprite[i]);
    }
    for (i = 0; i < kMaxShots; i++) {
        if (shotData[i].sprite)
            DisposeClonedSprite(&shotData[i].sprite);
    }
    for (i = 0; i < kMaxWeapTypes; i++) {
        if (masterShotSprite[i])
            SWDisposeSprite(&masterShotSprite[i]);
    }
    for (i = 0; i < kMaxDebris; i++) {
        if (smokeData[i].sprite)
            DisposeClonedSprite(&smokeData[i].sprite);
    }
    for (i = 0; i < 8; i++) {
        if (masterSmokeSprite[i])
            SWDisposeSprite(&masterSmokeSprite[i]);
    }
    for (i = 0; i < kMaxAsteroids; i++) {
        if (asteroidData[i].sprite)
            DisposeClonedSprite(&asteroidData[i].sprite);
    }
    for (i = 0; i < kMaxRoidTypes; i++) {
        if (masterAsteroidSprite[i])
            SWDisposeSprite(&masterAsteroidSprite[i]);
    }
    for (i = 0; i < kMainButtons; i++) {
        if (menuButtonSprite[i])
            SWDisposeSprite(&menuButtonSprite[i]);
    }
    if (menuRolloverSprite != NULL)
        SWDisposeSprite(&menuRolloverSprite);
    for (i = 0; i < kMaxBoxes; i++) {
        if (boxData[i].sprite)
            DisposeClonedSprite(&boxData[i].sprite);
    }
    for (i = 0; i < 5; i++) {
        if (masterBoxSprite[i])
            SWDisposeSprite(&masterBoxSprite[i]);
    }
    for (i = 0; i < kMaxStars; i++) {
        if (starData[i].sprite)
            DisposeClonedSprite(&starData[i].sprite);
    }
    if (masterStarSprite != NULL)
        SWDisposeSprite(&masterStarSprite);
    if (alertSprite != NULL)
        SWDisposeSprite(&alertSprite);
    if (doubleTimeSprite != NULL)
        SWDisposeSprite(&doubleTimeSprite);
    if (cursorSprite != NULL)
        SWDisposeSprite(&cursorSprite);
    for (i = 0; i < kMaxNavTargets; i++) {
        if (navTargetSprite[i])
            DisposeClonedSprite(&navTargetSprite[i]);
    }
    for (i = 0; i < kMaxNavTargets; i++) {
        if (weapTargetSprite[i])
            DisposeClonedSprite(&weapTargetSprite[i]);
    }
    if (masterNavTargetSprite != NULL)
        SWDisposeSprite(&masterNavTargetSprite);
    if (masterWeapTargetSprite != NULL)
        SWDisposeSprite(&masterWeapTargetSprite);
    if (redMapArrow != NULL)
        DisposeCIcon((CIconHandle)redMapArrow);
    if (greenMapArrow != NULL)
        DisposeCIcon((CIconHandle)greenMapArrow);

    ByeLog("disposing particles");
    DisposeParticles();
    SWDisposeFrame(&floatingMapFrame);
    SWDisposeFrame(&floatingEscortMenuFrame);

    ByeLog("disposing patterns & picts");
    for (i = 0; i < 0xa; i++) {
        if (staticPat[i] != NULL)
            DisposePixPat((PixPatHandle)staticPat[i]);
    }
    for (i = 0; i < kMaxShipTypes; i++) {
        if (targetInfoPic[i] != NULL) {
            QDReleaseResource((PicHandle)targetInfoPic[i]);
            targetInfoPic[i] = NULL;
        }
    }

    ByeLog("disposing sprite layers");
    if (shipLayer != NULL)
        SWDisposeSpriteLayer(&shipLayer);
    if (stellarLayer != NULL)
        SWDisposeSpriteLayer(&stellarLayer);
    if (explodLayer != NULL)
        SWDisposeSpriteLayer(&explodLayer);
    if (shotLayer != NULL)
        SWDisposeSpriteLayer(&shotLayer);
    if (guidedShotLayer != NULL)
        SWDisposeSpriteLayer(&guidedShotLayer);
    if (pointDefenseLayer != NULL)
        SWDisposeSpriteLayer(&pointDefenseLayer);
    if (targetLayer != NULL)
        SWDisposeSpriteLayer(&targetLayer);
    if (interfaceLayer != NULL)
        SWDisposeSpriteLayer(&interfaceLayer);
    if (asteroidLayer != NULL)
        SWDisposeSpriteLayer(&asteroidLayer);
    if (smokeLayer != NULL)
        SWDisposeSpriteLayer(&smokeLayer);
    if (disabledShipLayer != NULL)
        SWDisposeSpriteLayer(&disabledShipLayer);
    if (escortLayer != NULL)
        SWDisposeSpriteLayer(&escortLayer);
    if (boxLayer != NULL)
        SWDisposeSpriteLayer(&boxLayer);
    if (starLayer != NULL)
        SWDisposeSpriteLayer(&starLayer);
    if (turretedShotLayer != NULL)
        SWDisposeSpriteLayer(&turretedShotLayer);

    ByeLog("disposing sprite world");
    SWDisposeSpriteWorld(&gSpriteWorld);

    ByeLog("disposing offscreen buffers");
    DisposeGWorldContainer(&loadPort);
    DisposeGWorldContainer(&statusOffPort);

    ByeLog("disposing array pointers");
    DeallocAPointer(shipData);
    DeallocAPointer(shotData);
    DeallocAPointer(govtData);
    DeallocAPointer(dudeData);
    DeallocAPointer(beamData);
    DeallocAPointer(stellarData);
    DeallocAPointer(systemData);
    DeallocAPointer(shipClass);
    DeallocAPointer(weaponStats);
    DeallocAPointer(explodData);
    DeallocAPointer(explodType);
    DeallocAPointer(smokeData);
    DeallocAPointer(outfitSystem);
    DeallocAPointer(personData);
    DeallocAPointer(asteroidData);
    DeallocAPointer(disasterData);
    DeallocAPointer(nebulaData);
    DeallocAPointer(fleetData);
    DeallocAPointer(boxData);
    DeallocAPointer(podData);
    DeallocAPointer(junkData);
    DeallocAPointer(cronEvent);
    DeallocAPointer(rankData);
    DeallocAPointer(starData);
    DeallocAPointer(missionBit);
    DeallocAPointer(currentMissions);
    DeallocAPointer(currentMissionObjectives);

    ByeLog("disposing dialog buffer");
    DisposeDialogs();

    ByeLog("disposing window");
    gWind = NULL;

    ByeLog("closing QuickTime");
    MovieStopAll();

    ByeLog("carbon cleanup");
    ShowTheCursor();

    ByeLog("closing MT");

    ByeLog("closing other tools");
    ST_Close();

    crashlog("clean exit");

    crashlog("\n\n- end of log -");
    crashlog_close();
    ExitToShell();
}

// FUNCTION: EVNOVA 0x004ad7c0
// MAC: _InitColors i386 0x0001be73 ppc 0x0001ff78
void InitColors(void)
{
    bkgndGreen.red = bkgndGreen.blue = 0;
    bkgndGreen.green = 0x2200;
    dimGreen.red = dimGreen.blue = 0;
    dimGreen.green = 0x9900;
    dimmerGreen.red = dimmerGreen.blue = 0;
    dimmerGreen.green = 0x6666;
    brightGreen.red = brightGreen.blue = 0;
    brightGreen.green = 0xffff;
    rgbWhite.red = rgbWhite.green = rgbWhite.blue = 0xffff;
    rgbDarkGrey.red = rgbDarkGrey.green = rgbDarkGrey.blue = 0x4000;
    rgbRealDarkGrey.red = rgbRealDarkGrey.green = rgbRealDarkGrey.blue = 0xfa0;
    rgbMedGrey.red = rgbMedGrey.green = rgbMedGrey.blue = 0x8000;
    rgbLessDarkGrey.red = rgbLessDarkGrey.green = rgbLessDarkGrey.blue = 0x4e20;
    rgbLightGrey.red = rgbLightGrey.green = rgbLightGrey.blue = 0xc000;
    rgbRealDarkBlue.red = rgbRealDarkBlue.green = 0;
    rgbRealDarkBlue.blue = 0x428f;
    rgbBlack.red = rgbBlack.green = rgbBlack.blue = 0;
}


// FUNCTION: EVNOVA 0x004b0740
// MAC: _LoadSounds i386 0x0001c036 ppc 0x000201a4
void LoadSounds(void)
{
    short msg;
    short i;
    short set;
    AudioDesc info;
    Handle h;
    unsigned short vol;

    for (set = 0; set < kVoiceSets; set++) {
        for (msg = 0; msg < kVoiceMessages; msg++) {
            voiceResourceCount[set][msg] = 0;
            i = 0;
            do {
                if (ResourceExists(kSoundType,
                        kFirstVoiceID + set * kVoiceSetStride + msg * kVoiceMsgStride + i) == 0)
                    break;
                voiceResourceCount[set][msg] += 1;
                i++;
            } while (i < kVoiceClipsMax);
        }
    }

    for (msg = 0; msg <= 1; msg++) {
        jumpSoundLength[msg] = kDefaultLoopTicks;
        h = GetResource(kSoundType, msg + kFirstResourceID);
        if (h != NULL) {
            LockAHandle(h);
            if (ParseSoundFileImage(&info, (Word8 *)*h, GetAHandleSize(h)) == 0)
                jumpSoundLength[msg] = (info.SampleCount * kTicksPerSecond
                                            / info.SampleRate);
            ReleaseResource(h);
        }
    }

    warpUpSnd[0] = ST_LoadSndResource(0x80);
    warpUpSnd[1] = ST_LoadSndResource(0x81);
    warpOutSnd = ST_LoadSndResource(0x82);

    warpUpSoundSpec.sound = warpUpSnd[0];
    warpUpSoundSpec.rate = kUnityRate;
    warpUpSoundSpec.priority = kHighPriority;
    vol = gSoundVolume;
    warpUpSoundSpec.leftLevel = vol;
    warpUpSoundSpec.rightLevel = vol;
    planetSoundSpec.sound = NULL;
    planetSoundSpec.rate = kUnityRate;
    planetSoundSpec.priority = kHighPriority;
    planetSoundSpec.leftLevel = vol;
    planetSoundSpec.rightLevel = vol;

    for (msg = 0; msg <= kEngineSounds - 1; msg++)
        beepSnd[msg] = ST_LoadSndResource(msg + kFirstEngineID);

    for (msg = 0; msg < kShotSounds; msg++)
        shotSnd[msg] = ST_LoadSndResource(msg + kFirstShotID);

    for (msg = 0; msg < kExplodSounds; msg++)
        explodSnd[msg] = ST_LoadSndResource(msg + kFirstExplodID);

    redAlertSnd = ST_LoadSndResource(0x172);
    klaxxonSnd = ST_LoadSndResource(0x173);
    escapePodSnd = ST_LoadSndResource(0x174);
    cloakSnd[0] = ST_LoadSndResource(0x17c);
    cloakSnd[1] = ST_LoadSndResource(0x17d);
    hardDockSnd = ST_LoadSndResource(0x186);
    clickSnd = ST_LoadSndResource(0x258);
    missionSoundID = -1;
    missionSnd = NULL;
    voiceSnd = NULL;
    pendingVoiceMsg = -1;
    pendingVoiceGovt = -1;
    pendingVoiceParity = -1;
}

/*
 * The bounds come from the video layer's current mode rather than from a
 * window, which is why the two sources are BurgerLib globals.
 */

#define kVideoDepth         15
#define kVideoFullScreen    0x01                /* borderless, at the desktop's mode */
#define kVideoAlways        0x02

/* The main menu's picture with its logo and slide animations is put together here: at the art's own size when
   the menu is scaled, so it is stretched as one picture (no seams), else in loadPort as in the original. */
GWorldContainer *MenuArt(void)
{
    return menuArtPort.world != NULL ? &menuArtPort : &loadPort;
}

namespace {

/* the menu art's rows over `r', in the art and in what was last stretched */
bool SameAsShown(const Rect &r)
{
    const PixMap &art = menuArtPort.world->pix;
    const PixMap &shown = menuShownPort.world->pix;
    size_t bytes = (size_t)(r.right - r.left) * Pixel::kBytes;

    for (int y = r.top; y < r.bottom; y++) {
        if (memcmp(Pixel::Row(art, y) + r.left, Pixel::Row(shown, y) + r.left, bytes) != 0)
            return false;
    }
    return true;
}

void KeepAsShown(const Rect &r)
{
    const PixMap &art = menuArtPort.world->pix;
    const PixMap &shown = menuShownPort.world->pix;
    size_t bytes = (size_t)(r.right - r.left) * Pixel::kBytes;

    for (int y = r.top; y < r.bottom; y++)
        memcpy(Pixel::Row(shown, y) + r.left, Pixel::Row(art, y) + r.left, bytes);
}

} // namespace

/* The put-together picture over `area' -> loadPort, stretched to its scale. With `onlyChanged' it is left
   alone when the art there (and the pixels the stretch reads around it) is as last stretched. */
void ShowMenuArt(const Rect &area, bool onlyChanged)
{
    Rect shown;
    Rect read;

    if (menuArtPort.world == NULL || menuShownPort.world == NULL || loadPort.world == NULL
            || !SectRect(&area, &menuArtPort.bounds, &shown))
        return;
    read = shown;
    InsetRect(&read, -2, -2);
    SectRect(&read, &menuArtPort.bounds, &read);
    if (onlyChanged && SameAsShown(read))
        return;
    KeepAsShown(read);
    UI::Painter(UI::Surface::Load()).Copy(UI::Surface(menuArtPort), shown, shown);
}

/* the resolution, game size, UI scale and blending the game runs at */
static struct {
    Resolution resolution;
    short gameSize;
    double uiScale;
    Boolean orBlend;
} applied;

/* The game is drawn at the preferences' resolution (0 = the desktop's) and shows the game size's space. The
   UI is scaled to look as on a 1024 x 768 screen, times the UI scale; the main menu without the UI scale. The
   renderer is made here, with the original blending when it is on and the renderer has it. */
// FUNCTION: EVNOVA 0x004b45e0
void SetupMonitor(void)
{
    Resolution resolution = { settings.gameWidth, settings.gameHeight };

    applied.resolution = resolution;
    applied.gameSize = settings.gameSize;
    applied.orBlend = settings.orBlend;
    ApplyUIScale();
    PixelSize draw = DrawSize(resolution);
    ApplyDisplayScaling();
    PlatVideoAskOrBlend(settings.orBlend);
    long flags = (settings.runInWindow != 0 ? 0 : kVideoFullScreen) | kVideoAlways;
    if (SetDisplayToSize(draw.width, draw.height, kVideoDepth, flags) != 0) {
        /* the preferences' resolution can't be had: this run at the desktop's, the preferences kept */
        crashlog("preferences resolution not available: the default one");
        applied.resolution.width = 0;
        applied.resolution.height = 0;
        ApplyUIScale();
        draw = DrawSize(applied.resolution);
        ApplyDisplayScaling();
        if (SetDisplayToSize(draw.width, draw.height, kVideoDepth, flags) != 0)
            Fatal("Couldn't set display mode %dx%d\n", draw.width, draw.height);
    }
    if (settings.orBlend != 0 && PlatVideoOrBlend() == 0)
        crashlog("original blending not available here: sprites are added");
}

/* The scales and sizes for a UI scale at a resolution and game size, the UI scale kept to what the Preferences
   offer there (where every dialog fits); returns the UI scale used. */
static double SetUIScales(Resolution resolution, short gameSize, double uiScale)
{
    PixelSize draw = DrawSize(resolution);
    PixelSize game = GameArea(draw, gameSize);
    int maxSide = PlatMaxTextureSize();         /* the view is one texture: kept to what the GPU takes */
    if (maxSide > 0 && (game.width > maxSide || game.height > maxSide)) {
        double fit = game.width > game.height ? (double)maxSide / game.width : (double)maxSide / game.height;
        game.width = 2 * (int)(game.width * fit / 2);
        game.height = (int)(game.height * fit);
    }
    double base = BaseUIScale(draw);
    double minUI = MinUIScale(resolution);
    double maxUI = MaxUIScale(resolution, 0, 0);
    if (!(uiScale >= minUI))
        uiScale = minUI;
    if (uiScale > maxUI)
        uiScale = maxUI;
    double scale = uiScale * base;
    if (fabs(scale - 1.0) < 1e-6)
        scale = 1.0;
    UI::SetScale(scale);
    UI::SetScreenSize((short)(draw.width / scale + 0.5), (short)(draw.height / scale + 0.5), draw.width,
                      draw.height);
    UI::SetMenuScale(base);
    View::SetSizes(game.width, game.height, draw.height, scale);
    return uiScale;
}

/* The preferences' UI scale, kept to its range, to the screen's sizes; again once the plug-ins' dialogs are
   known. */
void ApplyUIScale(void)
{
    settings.uiScale = SetUIScales(applied.resolution, applied.gameSize, settings.uiScale);
    applied.uiScale = settings.uiScale;
}

short GameSizeInUse(void)
{
    return applied.gameSize;
}

Boolean OrBlendInUse(void)
{
    return applied.orBlend;
}

/* A resolution, game size and the preferences' UI scale, to the running game (from the main menu, no dialog
   open): the scales and sizes, the screen when its size changes, and every port and frame made for them. A
   screen that can't be had keeps the running resolution and UI scale. */
static void ApplyDisplay(Resolution chosen, short gameSize)
{
    short viewWidth = View::width;
    short viewHeight = View::height;

    if (chosen == applied.resolution && gameSize == applied.gameSize && settings.uiScale == applied.uiScale)
        return;
    if (gSpriteWorld == NULL || UI::InMenuScale()) {
        crashlog("display settings not applied: no sprite world, or inside the menu's scale");
        return;
    }
    PixelSize draw = DrawSize(chosen);
    if (draw.width != Screen::Get().width || draw.height != Screen::Get().height) {
        if (!Screen::Resize(draw.width, draw.height)) {
            settings.gameWidth = applied.resolution.width;
            settings.gameHeight = applied.resolution.height;
            settings.uiScale = applied.uiScale;
            SavePrefs();
            return;
        }
    }
    applied.resolution = chosen;
    applied.gameSize = gameSize;
    double asked = settings.uiScale;
    ApplyUIScale();
    if (settings.uiScale != asked)
        SavePrefs();

    ResetScreenRect();
    gGamePort.pix.bounds = mainWindRect;
    gGamePort.pnDeviceOffset = 0;
    SetScreenCenter();
    SWResizeSpriteWorld(gSpriteWorld, ViewFrames());
    SWDisposeFrame(&floatingMapFrame);
    SWDisposeFrame(&floatingEscortMenuFrame);
    CreateOverlayFrames();

    DisposeGWorldContainer(&loadPort);
    DisposeGWorldContainer(&menuArtPort);
    DisposeGWorldContainer(&menuShownPort);
    DisposeGWorldContainer(&statusOffPort);
    InitOffscreen();
    DisposeGWorldContainer(&buttonPort);
    CreateButtonPort();
    /* what the GPU kept for the sizes and ports before */
    UI::Canvas::Release();
    UI::Art::Clear();
    UI::GlyphAtlas::Clear();

    ResetCommRect();
    if (alertSprite != NULL)
        PlaceAlertSprite();
    if (gameInProgress != 0 && (View::width != viewWidth || View::height != viewHeight))
        CreateAsteroids();
    /* what the interface keeps drawn is of another size: each piece is made again where it is next shown */
    redrawEscortMenu = 1;
    lastIntfResourceID = -1;
    mainScreenBackgroundDirty = 1;
}

/* the whole screen black */
void BlackScreen(void)
{
    UI::Painter(UI::Surface::Screen()).Fill(*GetCWindowPortRect(gWind), UI::kBlack);
}

/* The Preferences' display settings after the dialog closes; a game size waits for the next pilot while one
   is loaded. */
void ApplyDisplaySettings(void)
{
    ApplyDisplay({ settings.gameWidth, settings.gameHeight },
                 gameInProgress != 0 ? applied.gameSize : settings.gameSize);
}

/* A pilot was opened or made: it plays at the preferences' game size. */
void ApplyPilotGameSize(void)
{
    ApplyDisplay(applied.resolution, settings.gameSize);
}

void ApplyBrightness(void)
{
    PlatVideoSetBrightness(100.0 / (settings.gamma * 10.0 + 70.0));
}

/* The preferences' upscaling filter, to the window. */
void ApplyDisplayScaling(void)
{
    static const int kFilters[kScaleFilterCount] = { kPlatFilterNearest, kPlatFilterLinear,
                                                     kPlatFilterPixelArt };

    PlatVideoSetScaling(kFilters[settings.scaleFilter]);
}

// FUNCTION: EVNOVA 0x004b42f0
void ResetScreenRect(void)
{
    mainWindRect.top = 0;
    mainWindRect.left = 0;
    mainWindRect.bottom = UI::ScreenHeight();
    mainWindRect.right = UI::ScreenWidth();
}

// FUNCTION: EVNOVA 0x004b6760
// MAC: _FatalStartupError i386 0x0001e62f ppc 0x00022f78
void FatalStartupError(short index)
{
    unsigned char text[256];

    GetIndString(text, kAlertStrList, index);
    ParamText(text, NULL, NULL, NULL);
    StopAlert(128, NULL);
    ExitToShell();
}
