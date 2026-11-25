/* initevnova.cpp -- tu-map object O2: game start, PlayGame, ship spawning, damage, escorts and fleets, then the pirated-key check and the .CRT hook that ends the object. Notes: Research/notes/initevnova.md */

#include "burgermath.h"
#include "platform.h"
#include "profiler.h"
#include "sound.h"
#include "ai.h"
#include "crashlog.h"
#include "dialogs.h"
#include "game.h"
#include "gameutil.h"
#include "missions.h"
#include "player.h"
#include "render.h"
#include "ships.h"
#include "space.h"
#include "stellar.h"
#include "viewscale.h"
#include "screen.h"
#include "viewdraw.h"
#include "gamepaths.h"
#include "prototypes.h"
#include "outfitmods.h"
#include "text.h"

#define kMaxGamma     6
#define kDefaultGamma 3

#define kEventLoopTicks     10

short pluginCount;
Rect backUpdateRect;
uint32_t lastSmokeUpdateTime;
uint32_t lastKeypressTime;
uint32_t lastEventLoopTime;
ShipObj *shipData;
ShipRec *shipClass;
SpobRecord *stellarData;
SpobRecord *dockedPort;
SystRecord *systemData;
WeapRecord *weaponStats;
BoomSlot *explodData;
DebrisSlot *smokeData;
BoomRecord *explodType;
BeamRecord *beamData;
OutfRecord *outfitSystem;
DudeRecord *dudeData;
GovtRecord *govtData;
PersRecord *personData;
AsteroidObj *asteroidData;
RoidRecord asteroidType[16];
OopsRecord *disasterData;
NebuRecord *nebulaData;
FletRecord *fleetData;
BoxObj *boxData;
PodObj *podData;
ShotObj *shotData;
MissionRecord *currentMissions;
MissionState *currentMissionObjectives;
JunkRecord *junkData;
CronRecord *cronEvent;
RankRecord *rankData;
StarSlot *starData;
Boolean *missionBit;
Settings settings;
KeyPrefs keyPrefs;
Sound *warpUpSnd[2];
Sound *warpOutSnd;
Sound *redAlertSnd;
Sound *beepSnd[5];
Sound *shotSnd[256];
Sound *explodSnd[64];
Sound *hardDockSnd;
Sound *klaxxonSnd;
Sound *escapePodSnd;
Sound *cloakSnd[2];
Sound *voiceSnd;
Sound *clickSnd;
Sound *missionSnd;
SoundRequest warpUpSoundSpec;
SoundRequest planetSoundSpec;
SpriteRec *navTargetSprite[4];
SpriteRec *weapTargetSprite[4];
SpriteRec *doubleTimeSprite;
SpriteRec *alertSprite;
SpriteRec *cursorSprite;
SpriteRec *masterShipSprite[kMaxShipTypes];
SpriteRec *masterShipGlowSprite[kMaxShipTypes];
SpriteRec *masterShipShieldSprite[kMaxShipTypes];
SpriteRec *masterStarSprite;
SpriteRec *masterShipLightSprite[kMaxShipTypes];
SpriteRec *masterShipWeaponSprite[kMaxShipTypes];
SpriteRec *masterShipAltSprite[kMaxShipTypes];
SpriteRec *masterStellarSprite[kMasterStellarSprites];
SpriteRec *masterExplodSprite[kMaxExplodTypes];
SpriteRec *masterShotSprite[256];
SpriteRec *masterAsteroidSprite[16];
SpriteRec *masterSmokeSprite[8];
SpriteRec *menuRolloverSprite;
SpriteRec *stellarSprite[kStellarSprites];
SpriteRec *masterBoxSprite[5];
SpriteRec *menuButtonSprite[kMainButtons];
SpriteRec *masterWeapTargetSprite;
SpriteRec *masterNavTargetSprite;
SpriteLayerRec *shipLayer;
SpriteLayerRec *stellarLayer;
SpriteLayerRec *explodLayer;
SpriteLayerRec *shotLayer;
SpriteLayerRec *guidedShotLayer;
SpriteLayerRec *pointDefenseLayer;
SpriteLayerRec *targetLayer;
SpriteLayerRec *asteroidLayer;
SpriteLayerRec *smokeLayer;
SpriteLayerRec *disabledShipLayer;
SpriteLayerRec *boxLayer;
SpriteLayerRec *starLayer;
SpriteLayerRec *turretedShotLayer;
SpriteLayerRec *escortLayer;
SpriteWorldPtr gSpriteWorld;
SWFrameRec *floatingMapFrame;
SWFrameRec *floatingEscortMenuFrame;
CWindowPtr gWind;
Boolean statusRedraw[kStatusRedrawCount];
Boolean gameLoaded;
Boolean gameInProgress;
Boolean killShots;
Boolean killBoxes;
Boolean killExplods;
Boolean killAsteroids;
Boolean killSmoke;
Boolean mBarHidden;
Boolean strictPlay;
Boolean weapLawFlag;
Boolean smuggleFlag;
Boolean fadePending[2];
Boolean doubleTime;
Boolean seenIntroScreen;
Boolean playerThreatened;
Boolean oldPlayerThreatened;
Boolean gPause;
Boolean gQuit;
Boolean cheatsActive;
Boolean gameFrozen;
Boolean ignoreKeys;
Boolean updateSmokeThisFrame;
Boolean closeRequested;
void *targetInfoPic[kMaxShipTypes];
GWorldContainer statusOffPort;
GWorldContainer loadPort;
uint32_t tix;
uint32_t floatingMapStartTime;
uint32_t lastMouseMoveTime;
uint32_t lastEscortMenuTime;
uint32_t lastFrameTime;
int32_t lastFPSUpdateTime;
uint32_t lastCursorAnimTime;
short frameCounter;
short lastShipAIDispatched;
short cursorHidden;
short selTradeItem;
short portDemand[kCargoTypes];
short basePrice[kCargoTypes];
short tradePrice[10];
short voiceResourceCount[8][kVoiceMessages];
short dockPermission;
short dockTarget;
short commCount;
short dockedPortItems[kMaxOutfits];
short dockedPortShipTypes[kMaxShipTypes];
short dockedPortMissions[kMissionVenues][kMaxMissions];
short shipSystems[kMaxOutfits];
MapScroll mapScroll;
ShortPoint screenCenter;
FloatPoint playerMaxVel;
FloatPoint lastFramePlayerPos;
unsigned char playerName[256];
unsigned char lastPlayerName[256];
unsigned char playerNickname[256];
unsigned char shipName[256];
std::string weapName[kMaxWeapTypes];
std::string shipyardLongNames[kMaxShipTypes];
std::string shipHelpRequests[20];
std::string legalStatusStrings[18];
std::string outfitShortNames[kMaxOutfits];
std::string shipyardShortNames[kMaxShipTypes];
std::string ratingStrings[11];
std::string govtTargStrings[kMaxGovts];
std::string lcOutfitName[kMaxOutfits];
std::string lcPluralOutfitName[kMaxOutfits];
std::string cargoName[kMaxCargo];
std::string statusCargoName[kCargoTypes];
std::string cargoAbbrev[kMaxCargo];
std::string lcCargoName[kMaxCargo];
std::string govtShortNames[kMaxGovts];
std::string shipCommNames[kMaxShipTypes];
std::string shipMovieFilenames[kMaxShipTypes];
unsigned char descMovieName[256];
std::string govtMediumNames[kMaxGovts];
std::string tonString;
std::string tonsString;
std::string fuelString[8];
std::string shieldString[6];
std::string armorString[3];
std::string newbieString[10];
std::string creditString;
std::string creditsString;
char creditCString[0x20];
char creditsCString[0x20];
char descTextPtr[kLongTextSize];
char commMsg[2048];
char textShowString[kLongTextSize];
char datePrefix[0x10];
char dateSuffix[0x10];
RGBColor bkgndGreen;
RGBColor brightGreen;
RGBColor dimGreen;
RGBColor dimmerGreen;
RGBColor rgbWhite;
RGBColor playerPaint;
RGBColor rgbLightGrey;
RGBColor rgbMedGrey;
RGBColor rgbDarkGrey;
RGBColor rgbLessDarkGrey;
RGBColor rgbRealDarkGrey;
RGBColor rgbRealDarkBlue;
RGBColor rgbBlack;
void *staticPat[10];
Rect commRect;
Rect spriteClipRect;
Rect floatingMapRect;
Rect escortMenuRect;
short playerRecord[kMaxSystems];
ScanCode newKeyPrefs[40];
short alertSpritePending;
short missionSoundID;
short playerGender;
short lastMissionStellar;
short missionRandom[kMaxMissions];
short descResourcePictId;
short gSoundVolume;
short antiEscortDelay;
short globalPriceFlux[2];
short localPriceFlux[2];
short pendingVoiceMsg;
short pendingVoiceGovt;
short pendingVoiceParity;
short hyperRoute[kMaxHyperRoute];
int32_t playerKills;
double gSpeedMult;
DateTimeRec gameTime;
DateTimeRec scenarioStartDate;
Boolean inBackground;
Boolean miningShipsActive;
Boolean miningBoxesActive;
Boolean gravitySourcesPresent;
Boolean showFPS;
float navTargetExpand;
float weapTargetExpand;
float destructCount;
float playerRepairInhibit;
float hyperGamma;
Boolean leetMode;
Boolean keyPressedThisFrame;
unsigned char hypergateMapMode;
Boolean redrawEscortMenu;
Boolean screenFaded;
Boolean playerIsLanded;
Boolean quickKeyCheckResult;
Point lastMousePos;
short descFlags;
short showFloatingMap;
short escortMenuOnscreen;
short recentActivatedRank;
short descRankGovt[2];
short fpsCounter;
short cursorAlpha;
short jumpSoundLength[2];
short playerEscortOrders[4];
short lastIntfResourceID;
short hypergateMapModeStellar;
std::string absquatulateStr;
UnsignedWide lastFrameMicroseconds;
Rect statusOffRect[8];
RGBColor statusColor[8];
RGBColor menuTextColor[2];
RGBColor floatingBorderColor;
RGBColor escortMenuColor;
RGBColor ldefColors[3];
short statusFontID;
short statusFontSize[2];
short buttonFontID;
short buttonFontSize;
short menuFontID;
short menuTextSize;
short smallTextFont;
short smallTextSize;
PlayerStats playerStats;
short playerSabotaged;
Boolean playerHasIllegalJunk;
Boolean playerHasIllegalOutfits;
Boolean allReinforcementsInhibited;
Boolean playerHasTribbles;
Boolean playerHasPerishableJunk;

                                                         /* 0x004d9e10 BurgerLib */
                                                         /* 0x0046f690 */




// FUNCTION: EVNOVA 0x00416100
// MAC: _InitEVNova i386 0x0004666c ppc 0x0004eb44
void InitEVNova(void)
{
    unsigned char alertText[256];
    long grow;
    int err;
    short i;

    gameLoaded = gameInProgress = 0;
    preventStatusCopy = 0;
    gWind = 0;
    gSpriteWorld = 0;
    screenFaded = 0;

    RandSeedFromClock();
    KeyboardInit();

    crashlog_open("debuglog.txt");
    crashlog_print("game engine was compiled ");
    crashlog_print("Jun 24 2006");
    crashlog_print("\n");
    crashlog_print("game engine version is ");
    crashlog_print("1.0.A");
    crashlog_print(" Windows");
    crashlog_print("\n\n");

    crashlog("opening data files & loading prefs");
    InitDataFiles();
    LoadPrefs();
    ViewDraw::Install();

    crashlog("initializing monitor");
    SetupMonitor();
    ResetScreenRect();

    crashlog("building trig table");
    BuildTrigTable();
    if (settings.gamma > kMaxGamma)
        settings.gamma = kDefaultGamma;
    BuildGammaRamp(100.0);                      /* neutral: the brightness is the whole frame's */
    ApplyBrightness();
    gWind = 0;

    crashlog("initializing arrays");
    InitArrays();
    IterateInGameEventLoop(kEventLoopTicks);

    crashlog("opening data files");
    IterateInGameEventLoop(kEventLoopTicks);
    InitColors();

    SetupDialogs();


    pluginCount = CountEVPlugins(paths.plugins.c_str(), ".rez", 0);
    OpenDataFiles();
    pluginCount = CountEVPlugins(paths.plugins.c_str(), ".rez", 0);
    ApplyUIScale();
    ResetScreenRect();
    FadeScreenOut();

    cursorHidden = 0;
    if (settings.windowed == 0)
        HideTheCursor();
    ST_Open();

    crashlog("loading sounds");
    LoadSounds();

    crashlog("creating window");
    SetupWind();

    crashlog("initializing sprite world");
    SetupSpriteWorld();
    inBackground = 0;
    AdjustSoundVolume();

    crashlog("creating offscreen buffers");
    InitOffscreen();

    crashlog("starting music");
    SetupIntroMusic();
    ShowAmbrosiaLogo();
    IterateInGameEventLoop(kEventLoopTicks);

    crashlog("initializing strings");
    LoadStrings();
    IterateInGameEventLoop(kEventLoopTicks);

    crashlog("initializing status bar");
    InitStatus();
    IterateInGameEventLoop(kEventLoopTicks);

    crashlog("initializing objects");
    InitObjects();
    IterateInGameEventLoop(kEventLoopTicks);

    crashlog("initializing buttons");
    InitButtons();

    crashlog("loading object data");
    LoadObjectData();
    IterateInGameEventLoop(kEventLoopTicks);

    crashlog("loading mission data");
    InitMissions();
    IterateInGameEventLoop(kEventLoopTicks);

    crashlog("propagating mission bit effects");
    PropagateMissionBitEffects();
    IterateInGameEventLoop(kEventLoopTicks);

    HideAmbrosiaLogo();

    crashlog("showing splash screen");
    ShowSplashScreen();
    HandleIntroMusic(1);
    FadeScreenIn();
    IterateInGameEventLoop(kEventLoopTicks);
    IterateInGameEventLoop(kEventLoopTicks);
    InitIntroProgressBar();

    crashlog("loading sprites");
    LoadSprites();

    crashlog("checking memory");

    crashlog("loading ship sprites");
    LoadShipSprites();

    crashlog("configuring sprites");
    InitSprites();

    crashlog("initializing player");
    ResetPlayer(1);
    ResetPlayerRecord(1);
    gameLoaded = gameInProgress = 0;
    IterateInGameEventLoop(kEventLoopTicks);

    crashlog("initializing main screen");
    HandleIntroMusic(1);
    FadeScreenOut();
    InitMainScreen();

    crashlog("opening last pilot file");
    OpenLastPilotFile();
    IterateInGameEventLoop(kEventLoopTicks);

    crashlog("propagating mission bit effects");
    PropagateMissionBitEffects();
    gameLoaded = 1;

    crashlog("---");
    gPause = 1;
    HandleIntroMusic(1);
    DoMainScreen();

    crashlog("main screen disposed");

    FadeScreenOut();
    ShowTheCursor();
    Bye();
}

#define kEscapePodType  0x2ff

// FUNCTION: EVNOVA 0x00417600
// MAC: _PlayGame i386 0x000457ba ppc 0x0004da00
void PlayGame(void)
{
    std::string str;
    DateTimeRec d;
    float dx;
    float dy;
    float deathTime;

    GetTime(&d);
    RedrawStatus(1);
    PropagateMissionBitEffects();
    HandleStellarSystemVisibility();
    gameFrozen = 1;
    ignoreKeys = 1;
    DoPlayGameWork(1);
    gameFrozen = 0;
    ignoreKeys = 0;
    hyperGamma = -1.0f;
    cursorAlpha = 0;
    PrepareAllSprites();
    RefreshWindowBuffer();
    SWUpdateSpriteWorld(gSpriteWorld);
    Screen::ComposeView();
    RefreshSAT();
    FadeScreenIn();
    lastSmokeUpdateTime = ReadTick();
    lastEventLoopTime = ReadTick();
    lastKeypressTime = ReadTick();
    AdjustSoundVolume();
    gSpriteWorld->fastBlit = 1;
    inBackground = 0;
    gPause = 0;
    FlushEvents(8, 0);
    FlushEvents(0xf, 0);

    while (gQuit == 0 && gPause == 0) {
        Profiler::Begin(Profiler::Frame);
        Profiler::Begin(Profiler::Wait);
        HandleTimeAdjustment();
        Profiler::End(Profiler::Wait);
        keyPressedThisFrame = 0;
        if (inBackground == 0) {
            if (gSpriteWorld->fastBlit != 0) {
                lastFramePlayerPos.x = shipData[0].pos.x;
                lastFramePlayerPos.y = shipData[0].pos.y;
                if (frameCounter % 3 == 0) {
                    updateSmokeThisFrame = 1;
                    lastSmokeUpdateTime = ReadTick();
                } else {
                    updateSmokeThisFrame = 0;
                }
                Profiler::Begin(Profiler::Sim);
                DoPlayGameWork(1);
                Profiler::End(Profiler::Sim);
                fpsCounter += 1;
                if (Profiler::On()) {
                    if (Profiler::LiveLine(str))
                        DisplayComm(str, 999);
                } else if (showFPS != 0) {
                    if (ReadTick() > lastFPSUpdateTime + 120) {
                        if ((uint32_t)lastFPSUpdateTime == 0) {
                            DisplayComm(IndString(kGameStrList, 282), 100);
                        } else {
                            str = NumText(fpsCounter * 60.0 / (float)(ReadTick() - lastFPSUpdateTime));
                            str += " ";
                            str += IndString(kGameStrList, 283);
                            DisplayComm(str, 999);
                        }
                        lastFPSUpdateTime = ReadTick();
                        fpsCounter = 0;
                    }
                } else {
                    lastFPSUpdateTime = ReadTick();
                }
                dx = lastFramePlayerPos.x - shipData[0].pos.x;
                dy = lastFramePlayerPos.y - shipData[0].pos.y;
                if (doubleTime != 0) {
                    dx *= 2.0f;
                    dy *= 2.0f;
                }
                ScrollStarfield(dx, dy);
                HandleMouse();
                Profiler::Begin(Profiler::Sprites);
                SWProcessSpriteWorld(gSpriteWorld);
                SWAnimateSpriteWorld(gSpriteWorld);
                PostAnimateHandler();
                Profiler::End(Profiler::Sprites);
                Screen::ComposeView();
                if (KeyCheck(keyPrefs.pause)) {
                    if (pauseKeyFlag == 0 && gPause == 0) {
                        pauseKeyFlag = 1;
                        if (shipData[0].jumpTimer <= 0.0f
                                && shipData[0].type != kEscapePodType
                                && (deathTime = shipData[0].deathTimer) <= 0.0f
                                && shipData[0].escapePods <= 0) {
                            if (shipData[0].active != 0 || deathTime < -250.0f)
                                gPause = 1;
                        }
                    }
                } else {
                    pauseKeyFlag = 0;
                }
                if (doubleTime != 0) {
                    Profiler::Begin(Profiler::Sim);
                    DoPlayGameWork(0);
                    Profiler::End(Profiler::Sim);
                }
                frameCounter += 1;
                if (frameCounter > 1024) {
                    frameCounter = 0;
                    if (d.day == 30 && d.month == 6 && commCount <= 0)
                        DisplayCommStyled(std::string("Happy birthday Matt"), 5, 3, 9, &rgbDarkGrey);
                }
                if (missionSoundID != -1 && missionSnd == 0) {
                    missionSnd = ST_LoadSndResource(missionSoundID);
                    if (missionSnd != 0)
                        PlaySound(missionSnd, 50, gSoundVolume, gSoundVolume);
                    missionSoundID = -1;
                }
                if (missionSnd != 0 && ST_IsSoundPlaying(missionSnd) == 0) {
                    ST_DisposeSound(missionSnd);
                    missionSnd = 0;
                }
                killShots = 0;
                killBoxes = 0;
                killExplods = 0;
                killSmoke = 0;
                killAsteroids = 0;
                missionSoundID = -1;
                if (ST_IsSoundPlaying(warpUpSnd[1]) != 0) {
                    doubleTime = 1;
                } else if (SpeedUpKeyOn()) {
                    if (abs(AngularDifference(shipData[0].heading,
                                shipData[0].desiredHeading)) <= 10
                            && ST_IsSoundPlaying(warpUpSnd[0]) != 0) {
                        doubleTime = 0;
                    } else {
                        doubleTime = 1;
                    }
                } else {
                    doubleTime = 0;
                    gameFrozen = 0;
                }
                if (cheatsActive == 0)
                    gameFrozen = 0;
                if (keyPressedThisFrame != 0)
                    lastKeypressTime = ReadTick();
                if (shipData[0].jumpTimer <= 0.0f || shipData[0].active == 0) {
                    if (ReadTick() - lastEventLoopTime > 30) {
                        IterateInGameEventLoop(1);
                        lastEventLoopTime = ReadTick();
                    }
                } else {
                    BusyIdle();
                }
            } else {
                SWUpdateSpriteWorld(gSpriteWorld);
                Screen::ComposeView();
                IterateInGameEventLoop(3);
                if (gSpriteWorld->fastBlit != 0) {
                    RefreshSAT();
                    RedrawStatus(1);
                }
            }
        } else {
            IterateInGameEventLoop(-1);
            if (inBackground == 0)
                RefreshWindowBuffer();
        }
        if (!absquatulateStr.empty()) {
            PlaySound(beepSnd[4], 1, gSoundVolume, gSoundVolume);
            DisplayComm(absquatulateStr, 500);
            absquatulateStr.clear();
        }
        Screen::EndFrame();
        Profiler::End(Profiler::Frame);
        Profiler::FrameEnd();
    }

    FadeScreenOut();
    ShowTheCursor();
    FlushEvents(everyEvent - diskMask, 0);
    SWSetPortToWindow(gSpriteWorld);
    ForeColor(blackColor);
    PaintRect(GetCWindowPortRect((WindowRef)gWind));
}

#define kMaxNavDefaults 0x10

// FUNCTION: EVNOVA 0x004186b0
// MAC: _DoPlayGameWork i386 0x00043de5 ppc 0x0004bbe4
void DoPlayGameWork(unsigned char advance)
{
    short i;
    short n;

    QuickKeyCheckHandler();
    gravitySourcesPresent = 0;
    HandlePlayer(shipData);
    HandleShipDisplay(shipData);

    Profiler::Begin(Profiler::Collisions);
    if (miningBoxesActive != 0)
        SWCollideSpriteLayer(gSpriteWorld, shipLayer, boxLayer);
    SWCollideSpriteLayer(gSpriteWorld, shipLayer, shotLayer);
    SWCollideSpriteLayer(gSpriteWorld, escortLayer, shotLayer);
    SWCollideSpriteLayer(gSpriteWorld, disabledShipLayer, shotLayer);
    SWCollideSpriteLayer(gSpriteWorld, asteroidLayer, shotLayer);
    SWCollideSpriteLayer(gSpriteWorld, shipLayer, guidedShotLayer);
    SWCollideSpriteLayer(gSpriteWorld, escortLayer, guidedShotLayer);
    SWCollideSpriteLayer(gSpriteWorld, disabledShipLayer, guidedShotLayer);
    SWCollideSpriteLayer(gSpriteWorld, asteroidLayer, guidedShotLayer);
    SWCollideSpriteLayer(gSpriteWorld, shipLayer, turretedShotLayer);
    SWCollideSpriteLayer(gSpriteWorld, escortLayer, turretedShotLayer);
    SWCollideSpriteLayer(gSpriteWorld, disabledShipLayer, turretedShotLayer);
    SWCollideSpriteLayer(gSpriteWorld, asteroidLayer, turretedShotLayer);
    SWCollideSpriteLayer(gSpriteWorld, asteroidLayer, pointDefenseLayer);
    SWCollideSpriteLayer(gSpriteWorld, pointDefenseLayer, guidedShotLayer);
    CheckShotProximities();
    Profiler::End(Profiler::Collisions);

    if (advance != 0) {
        HandleStatus();
        DrawStatus();

        Profiler::Begin(Profiler::AI);
        for (i = 0; i < kMaxShips; i++) {
            shipData[i].hasEscorts = 0;
            shipData[i].hasSwarm = 0;
            shipData[i].hasFormation = 0;
        }
        for (i = 1; i < kMaxShips; i++)
            prevParent[i] = shipData[i].parentShip;
        for (i = 1; i < kMaxShips; i++) {
            if (shipData[i].active != 0 && AIHasLeader(shipData[i].aiType) &&
                    shipData[i].curSystem == shipData[0].curSystem) {
                if (shipData[i].parentShip > 0) {
                    AICheckParentExists(&shipData[i]);
                    if (shipData[i].parentShip != -1)
                        shipData[shipData[i].parentShip].hasEscorts = 1;
                } else if (shipData[i].parentShip == 0) {
                    shipData[0].hasEscorts = 1;
                }
                if (shipData[i].swarmLeader > i && shipData[i].swarmLeader < kMaxShips)
                    shipData[shipData[i].swarmLeader].hasSwarm = 1;
                if (AIHasLeader(shipData[i].aiType)) {
                    shipData[i].formLeader = -1;
                    if (shipData[i].targetShip != -1 && IsShipAttacking(&shipData[i]) != 0)
                        shipData[i].formLeader = shipData[i].swarmLeader;
                    if (shipData[i].formLeader == -1)
                        shipData[i].formLeader = shipData[i].parentShip;
                } else {
                    shipData[i].formLeader = shipData[i].swarmLeader;
                }
                if (shipData[i].formLeader >= 0 && shipData[i].formLeader < kMaxShips)
                    shipData[shipData[i].formLeader].hasFormation = 1;
            }
        }
        Profiler::End(Profiler::AI);

        MissionHandler();
        HandleVoiceSound();
        HandleComm();
        CalculateTargetedDamage();
        EnterMoreShips(shipData[0].curSystem);
        SpawnAsteroid(1);
        scanActive = 0;
        miningShipsActive = 0;

        Profiler::Begin(Profiler::AI);
        for (i = 1; i < kMaxShips; i++) {
            if (shipData[i].active != 0 && AIIsSet(shipData[i].aiType) &&
                    shipData[i].curSystem == shipData[0].curSystem)
                AIDispatch(&shipData[i], 0);
        }
        for (i = 1; i < kMaxShips; i++) {
            if (shipData[i].odds < 0.0f) {
                if (i < kMaxShips - 1)
                    shipData[i + 1].odds = -1.0f;
                else
                    shipData[1].odds = -1.0f;
                if (shipData[i].active != 0 && AIIsSet(shipData[i].aiType) &&
                        shipData[i].curSystem == shipData[0].curSystem) {
                    AICalculateOddsAgainst(&shipData[i]);
                    break;
                }
                shipData[i].odds = 0.0f;
            }
        }
        Profiler::End(Profiler::AI);
    }

    for (i = 0; i < kMaxShots; i++) {
        if (shotData[i].life >= -1.0f) {
            HandleShot(&shotData[i]);
        } else if (shotData[i].sprite != 0 && shotData[i].sprite->isVisible != 0) {
            HandleShot(&shotData[i]);
        }
    }

    for (i = 0; i < kMaxNavDefaults; i++) {
        n = systemData[shipData[0].curSystem].navDefault[i];
        if (n != -1)
            stellarData[n].defenseActive = 0;
    }
    for (i = 1; i < kMaxShips; i++) {
        if (shipData[i].active != 0 && shipData[i].defenseStellar != -1)
            stellarData[shipData[i].defenseStellar].defenseActive = 1;
    }
    for (i = 1; i < kMaxShips; i++) {
        if (shipData[i].active != 0 && shipData[i].curSystem == shipData[0].curSystem)
            HandleShip(&shipData[i]);
        HandleShipDisplay(&shipData[i]);
    }

    HandleStellarSprites();
    HandleStellarWeapons();
    HandleDoubleTimeSprite();
    HandleExplods();
    HandleStars();
    HandleSmoke();
    HandleBoxes();
    HandleAsteroids();
    HandleNavTargetSprites();
    HandleWeapTargetSprites();
    HandleReinforcements();
    HandleGravity();
    HandleDeadlyStellars();
    HandleEscapePods();
    if (advance != 0)
        HandleAlertSprite();
    HandleBeams(advance);
}

// FUNCTION: EVNOVA 0x00419210
// MAC: _PrepareAllSprites i386 0x0003a75a ppc 0x00040d04
void PrepareAllSprites(void)
{
    Boolean wasFrozen;
    short i;

    wasFrozen = gameFrozen;
    gameFrozen = 1;
    for (i = 0; i < kMaxShips; i++)
        HandleShipDisplay(&shipData[i]);
    for (i = 0; i < kMaxShots; i++) {
        if (shotData[i].life >= -1.0f) {
            HandleShot(&shotData[i]);
        } else if (shotData[i].sprite != 0 && shotData[i].sprite->isVisible != 0) {
            HandleShot(&shotData[i]);
        }
    }
    HandleStellarSprites();
    HandleExplods();
    HandleSmoke();
    HandleBoxes();
    HandleAsteroids();
    HandleStars();
    HandleNavTargetSprites();
    HandleWeapTargetSprites();
    HandleEscapePods();
    gameFrozen = wasFrozen;
}

#define kGovtLegalScale         2       /* govt crimeTol is halved legal status */

#define kStrMissionFailed       0x11c
#define kStrShipDestroyed       0x11f
#define kStrPlayerKilled        0x120
#define kImpactCancelRange      50.0f
#define kEscortDefendRange      320.0f
#define kFullCloak              32.0f
#define kMaxJumpCharge          20.0f
#define kDisableArmorFraction   0.3333
#define kTenthArmorFraction     0.1
#define kShieldOvershoot        0.1
#define kPlayerKnockbackScale   1.8
#define kProvocationScale       1.75
#define kAnnoyanceThreshold     50.0f


// FUNCTION: EVNOVA 0x004192d0
// MAC: _DamageShip i386 0x0003a807 ppc 0x00040df8
void DamageShip(ShipObj *ship, FloatPoint *from, short impact, short armorDmg,
                short shieldDmg, short attacker, Boolean fromAttack, Boolean deliberate,
                unsigned char disableOnly, Boolean clampArmor,
                unsigned char armorOnly, short provocation)
{
    short theirTop;
    Boolean wasDisabled;
    unsigned char wasDying;
    char msg[256];
    Boolean selfHit;
    Boolean hostile;
    Boolean enemy;
    int32_t aligned;
    short angle;
    float capacity;
    short myGovt;
    short myTop;
    FloatPoint spob;
    short i;

    if (ship->index < 0 || ship->index >= kMaxShips)
        return;
    if (ship->type < 0 || ship->type >= kShipTypeEscapePod)
        return;
    if (ship->active == 0)
        return;

    if (fromAttack != 0 && attacker > 0 && attacker < kMaxShips) {
        if (ship->index == shipData[attacker].targetShip) {
            if (AIShipIsPlundering(&shipData[attacker])) {
                disableOnly = 1;
            } else {
                if (shipData[attacker].parentShip > 0 && shipData[attacker].parentShip < kMaxShips
                        && AIShipIsPlundering(&shipData[shipData[attacker].parentShip]))
                    disableOnly = 1;
            }
        }
    }

    wasDisabled = ship->IsDisabled();

    if (impact < 0 && fromAttack != 0 && attacker >= 0 && attacker < kMaxShips) {
        if (FloatAbs(shipData[attacker].pos.x - ship->pos.x) < kImpactCancelRange
                || FloatAbs(shipData[attacker].pos.y - ship->pos.y) < kImpactCancelRange)
            impact = 0;
    }

    if (impact != 0 && ship->jumpTimer <= 0.0f && from != NULL) {
        angle = CalcInterceptAngle(from, &ship->pos);
        if (shipClass[ship->type].mass > 0
                && (shipClass[ship->type].flags & kShipPlanetType) == 0) {
            AdjustedAccel(angle, impact / (float)shipClass[ship->type].mass,
                          &ship->vel, shipClass[ship->type].maxSpeed);
            if (ship->index == 0 && abOn != 0 && gravitySourcesPresent == 0)
                SpeedLimit(&ship->vel, ship->MaxSpeed() * kPlayerKnockbackScale);
            else
                SpeedLimit(&ship->vel, ship->MaxSpeed());
        }
    }

    wasDying = ship->IsDying();

    if (armorOnly != 0) {
        if (armorDmg > 0) {
            if (disableOnly != 0 && ship->armor > 0.0f
                    && ship->armor - armorDmg <= 0.0f)
                ship->armor = 1.0f;
            else
                ship->armor -= armorDmg;
        }
    } else {
        if (shieldDmg > 0)
            ship->shield -= shieldDmg;
        if (ship->shield <= 0.0f) {
            if (armorDmg > 0) {
                if (disableOnly != 0 && ship->armor > 0.0f
                        && ship->armor - armorDmg <= 0.0f)
                    ship->armor = 1.0f;
                else
                    ship->armor -= armorDmg;
            }
            capacity = ship->ShieldCapacity();
            if (ship->shield < -capacity * kShieldOvershoot)
                ship->shield = -capacity * kShieldOvershoot;
        }
    }

    if (clampArmor != 0 && ship->IsDisabled() && wasDisabled == 0) {
        if ((shipClass[ship->type].flags & kShipArmorDisableTenth) != 0)
            ship->armor = ship->ArmorCapacity() * kTenthArmorFraction + 1.0;
        else
            ship->armor = ship->ArmorCapacity() * kDisableArmorFraction + 1.0;
    }

    enemy = 0;
    if (attacker == 0) {
        enemy = 1;
    } else if (attacker > 0 && attacker < kMaxShips
               && shipData[attacker].parentShip == 0) {
        enemy = 1;
    }
    if (enemy != 0 && ship->defenseStellar == -1) {
        if (ship->IsDisabled() && wasDisabled == 0)
            SlapWithPenalty(shipData->curSystem, ship->govt, 1, ship->specialMission);
        if (ship->armor <= 0.0f && wasDying == 0 && !ShipIsDerelict(ship)) {
            SlapWithPenalty(shipData->curSystem, ship->govt, 3, ship->specialMission);
            AddToPlayerKills(shipClass[ship->type].strength);
        }
    }

    if (ship->IsDisabled()) {
        if (ship->specialMission >= 0 && ship->specialMission < kMaxActiveMissions
                && ship->defenseStellar == -1 && wasDisabled == 0) {
            if (currentMissionObjectives[ship->specialMission].active != 0) {
                if (currentMissionObjectives[ship->specialMission].failed == 0
                        && currentMissions[ship->specialMission].specialShipsDisabled == 0
                        && currentMissions[ship->specialMission].shipGoal == kMissionGoalEscort
                        && (currentMissions[ship->specialMission].flags & kMissionHidden) == 0) {
                    PlaySound(beepSnd[1], 1, gSoundVolume, gSoundVolume);
                    GetIndString((unsigned char *)msg, kGameStrList, kStrMissionFailed);
                    DisplayComm((PStrChar *)msg, kCommFrames);
                    QuickMissionFailure(ship->specialMission);
                }
                currentMissions[ship->specialMission].specialShipsDisabled++;
            }
        }
        if (ship->parentShip == 0 && ship->specialMission == -1) {
            if (ship->aiType == kAIFighter)
                ship->formerEscort = 0;
            else if (ship->hired != 0)
                ship->formerEscort = 1;
            else
                ship->formerEscort = 2;
            ship->parentShip = -1;
            if (ship->aiType == kAIEscort
                    && AIIsTrader(shipClass[ship->type].inherentAI)) {
                DestroyPartialFleetCargo(ship->index);
                statusRedraw[kRedrawCargo] = 1;
                DrawStatusCargo();
            }
            ship->aiType = shipClass[ship->type].inherentAI;
        }
        if (ship->index == 0 && wasDisabled == 0) {
            PlaySound(beepSnd[1], 1, gSoundVolume, gSoundVolume);
            GetIndString((unsigned char *)msg, kGameStrList, kStrShipDestroyed);
            DisplayComm((PStrChar *)msg, kCommFrames);
            playerRepairInhibit = 300.0f;
            ship->armor = (int32_t)ship->armor;
            for (i = 0; i < kMaxActiveMissions; i++) {
                if (currentMissionObjectives[i].active != 0 && currentMissionObjectives[i].failed == 0
                        && (currentMissions[i].flags2 & kMissionFailIfDisabled) != 0) {
                    PlaySound(beepSnd[1], 1, gSoundVolume, gSoundVolume);
                    QuickMissionFailure(i);
                    GetIndString((unsigned char *)msg, kGameStrList, kStrMissionFailed);
                    DisplayComm((PStrChar *)msg, kCommFrames);
                    break;
                }
            }
        }
    }

    /* `&`, not `&&`: the exe ANDs the two booleans with `test eax,ebx` */
    if (ship->index == 0 && (ship->IsDying() & (wasDying == 0))) {
        PlaySound(beepSnd[1], 1, gSoundVolume, gSoundVolume);
        GetIndString((unsigned char *)msg, kGameStrList, kStrPlayerKilled);
        DisplayComm((PStrChar *)msg, kCommFrames);
        for (i = 0; i < kMaxActiveMissions; i++) {
            if (currentMissionObjectives[i].active != 0 && currentMissionObjectives[i].failed == 0
                    && (currentMissions[i].flags2 & kMissionFailIfDisabled) != 0) {
                PlaySound(beepSnd[1], 1, gSoundVolume, gSoundVolume);
                QuickMissionFailure(i);
                GetIndString((unsigned char *)msg, kGameStrList, kStrMissionFailed);
                DisplayComm((PStrChar *)msg, kCommFrames);
                break;
            }
        }
    }

    selfHit = 0;
    enemy = 0;
    if (attacker != ship->index) {
        enemy = 1;
    } else if (ship->defenseStellar != -1) {
        enemy = 1;
        selfHit = 1;
    } else if (attacker >= 0 && attacker < kMaxShips
               && shipData[attacker].defenseStellar != -1) {
        enemy = 1;
        selfHit = 1;
    }

    if (enemy && fromAttack != 0 && AIIsSet(ship->aiType) && ship->jumpTimer <= 0.0f) {
        hostile = 0;
        if (selfHit != 0) {
            if (attacker >= 0 && attacker < kMaxShips) {
                if (ship->defenseStellar != -1 && shipData[attacker].defenseStellar != -1)
                    hostile = 0;
                else
                    hostile = 1;
            } else
                hostile = 1;
        } else {
            if (attacker >= 0 && attacker < kMaxShips
                    && shipData[attacker].govt != ship->govt)
                hostile = 1;
            if (ship->parentShip >= 0 && ship->parentShip < kMaxShips) {
                if (attacker != ship->parentShip || attacker == -1)
                    hostile = 1;
                if (attacker >= 0 && attacker < kMaxShips
                        && shipData[attacker].parentShip != -1
                        && ship->parentShip != shipData[attacker].parentShip)
                    hostile = 1;
            }
            if (ship->govt < 0 || ship->govt >= kMaxGovts) {
                if (attacker >= 0 && attacker < kMaxShips
                        && (attacker == 0 || shipData[attacker].parentShip == 0))
                    hostile = 1;
            } else {
                if (systemData[shipData->curSystem].govt >= 0
                        && systemData[shipData->curSystem].govt < kMaxGovts
                        && shipData->targetShip != ship->index
                        && playerRecord[shipData->curSystem]
                           >= govtData[ship->govt].crimeTol * kGovtLegalScale) {
                    if (attacker == 0)
                        hostile = 0;
                    else if (attacker > 0 && attacker < kMaxShips
                                && shipData[attacker].parentShip == 0)
                        hostile = 0;
                }
                if (attacker >= 0 && attacker < kMaxShips
                        && ship->govt == shipData[attacker].govt)
                    hostile = 0;
            }
            if (ship->parentShip >= 0 && ship->parentShip < kMaxShips
                    && attacker >= 0 && attacker < kMaxShips
                    && ship->parentShip == shipData[attacker].parentShip
                    && shipData[attacker].defenseStellar != -1)
                hostile = 0;
            if (hostile && shieldDmg <= 0 && armorDmg <= 0
                    && shieldDmg > -32000 && armorDmg > -32000)
                hostile = 0;
            if (deliberate == 0 && hostile) {
                if (attacker == 0) {
                    if (shipData[attacker].targetShip >= 0
                            && shipData[attacker].targetShip < kMaxShips) {
                        if (ship->index == shipData[attacker].targetShip)
                            hostile = 1;
                        else if (IsShipMyEnemy(ship,
                                               &shipData[shipData[attacker].targetShip]))
                            hostile = 0;
                    }
                } else if (attacker > 0 && attacker < kMaxShips
                           && ship->index != shipData[attacker].targetShip) {
                    hostile = 0;
                }
                if (hostile && (ship->govt >= 0 && ship->govt < kMaxGovts
                        && attacker == 0
                        && govtData[ship->govt].penalties[4] <= 0
                        && Rand(4) != 0))
                    hostile = 0;
                if (attacker == 0 && ship->annoyance < kAnnoyanceThreshold)
                    hostile = 0;
            }
            if (attacker == 0 && deliberate == 0)
                ship->annoyance += provocation * kProvocationScale;
            if (attacker >= 0 && attacker < kMaxShips) {
                if (shipData[attacker].parentShip == 0
                        && shipData[attacker].aiType == kAIFighter
                        && ship->targetShip == 0) {
                    if (FloatAbs(ship->pos.x - shipData->pos.x) <= kEscortDefendRange) {
                        if (FloatAbs(ship->pos.y - shipData->pos.y) <= kEscortDefendRange)
                            hostile = 0;
                    }
                }
                if (shipData[attacker].parentShip == ship->parentShip && ship->parentShip != -1)
                    hostile = 0;
            }
            if (ship->jumpTimer > 0.0f)
                hostile = 0;
            if (deliberate == 0 && hostile
                    && attacker >= 0 && attacker < kMaxShips) {
                if (shipData[attacker].govt >= 0
                        && shipData[attacker].govt < kMaxGovts
                        && AlliedWithPlayer(&shipData[attacker])
                        && PlayerBlessedByGovt(shipData[attacker].govt, 0))
                    hostile = 0;
            }
            myTop = theirTop = -1;
            if (ship->parentShip >= 0 && ship->parentShip < kMaxShips
                    && (shipData[ship->parentShip].parentShip >= 0
                        && shipData[ship->parentShip].parentShip < kMaxShips))
                myTop = shipData[ship->parentShip].parentShip;
            if (attacker > 0 && attacker < kMaxShips
                    && (shipData[attacker].parentShip > 0
                        && shipData[attacker].parentShip < kMaxShips))
                theirTop = shipData[shipData[attacker].parentShip].parentShip;
            if (myTop == theirTop)
                if (myTop >= 0)
                    if (myTop < kMaxShips)
                        if (theirTop >= 0)
                            if (theirTop < kMaxShips)
                                hostile = 0;
        }

        if (ship->jumpTimer > 0.0f) {
            hostile = 0;
        } else {
            if (ship->parentShip > 0 && ship->parentShip < kMaxShips && shipData[ship->parentShip].jumpTimer > 0.0f)
                hostile = 0;
        }
        if (hostile && ship->index > 0 && attacker > 0 && attacker < kMaxShips
                && AIShipIsDocking(ship))
            hostile = 0;
        if (hostile && (attacker >= 0 && attacker < kMaxShips
                && shipClass[shipData[attacker].type].escortType == 0
                && shipClass[ship->type].escortType != 0
                && ship->targetShip != -1
                && IsShipAttacking(ship)))
            hostile = 0;

        if (hostile) {
            ship->annoyance = 0;
            if ((shipClass[ship->type].flags2 & kShipCloaksWhenAttacked) != 0
                    && (attacker >= 0 && attacker < kMaxShips
                        && ship->CanCloak()
                        && !IsShipFightingMe(ship, &shipData[attacker]))) {
                AIForceBurstReload(ship);
                ship->DoCloak();
            }
            wasDying = 0;
            if (ship->defenseStellar == -1) {
                wasDying = 1;
                if (ship->targetShip != -1 && attacker >= 0 && attacker < kMaxShips
                        && IsShipAttacking(ship)) {
                    /* BUG: abs() takes the comparison, so any negative difference counts as aligned */
                    aligned = AngularDifference(ship->heading,
                                                ship->desiredHeading) < 45;
                    if (abs(aligned)) {
                        if (XYFloatSquaredDist(&ship->pos, &shipData[attacker].pos)
                                  > XYFloatSquaredDist(&ship->pos, &shipData[ship->targetShip].pos) / 4)
                            wasDying = 0;
                    }
                }
                if (wasDying != 0 && ship->index != 0) {
                    if (armorDmg > 0)
                        ship->grudge += armorDmg;
                    if (shieldDmg > 0)
                        ship->grudge += shieldDmg;
                    if (attacker >= 0 && attacker < kMaxShips)
                        ship->targetShip = attacker;
                    if (ship->parentShip > 0 && ship->parentShip < kMaxShips
                            && shipData[ship->parentShip].defenseStellar == -1) {
                        if (armorDmg > 0)
                            shipData[ship->parentShip].grudge += armorDmg;
                        if (shieldDmg > 0)
                            shipData[ship->parentShip].grudge += shieldDmg;
                        if (attacker >= 0 && attacker < kMaxShips)
                            shipData[ship->parentShip].targetShip = attacker;
                    }
                }
            } else if (attacker >= 0 && attacker < kMaxShips) {
                spob.x = stellarData[ship->defenseStellar].pos.h;
                spob.y = stellarData[ship->defenseStellar].pos.v;
                if (XYFloatSquaredDist(&ship->pos, &shipData[attacker].pos)
                         < XYFloatSquaredDist(&ship->pos, &spob) / 2) {
                    if (ship->index != 0) {
                        if (armorDmg > 0)
                            ship->grudge += armorDmg * 30;
                        if (shieldDmg > 0)
                            ship->grudge += shieldDmg * 30;
                        ship->targetShip = attacker;
                    }
                    wasDying = 1;
                }
            }
            if (wasDying != 0) {
                if (attacker == 0)
                    AIMakeShipAttackPlayer(ship);
                if (ship->aiTimer > kMaxJumpCharge)
                    ship->aiTimer = kMaxJumpCharge;
                if (attacker == 0 && ship->defenseStellar == -1) {
                    if (ship->persType != -1
                            && (personData[ship->persType].flags & kPersHoldsGrudge) != 0)
                        personData[ship->persType].grudge = 1;
                    if (ship->index != 0) {
                        AIMakeShipStopRefuelling(ship);
                        ship->targetShip = attacker;
                    }
                }
            }
            if (deliberate != 0 && attacker == 0)
                DoGoodSamaritan(ship, shipData);
        }
    }

    if (armorOnly == 0)
        ship->shieldGlow = kFullCloak;
    if (ship->index == 0)
        statusRedraw[kRedrawShield] = 1;
    if (fromAttack != 0 && ship->decloaksWhenHit == 1
            && (ship->cloak == kFullCloak || ship->cloakDir >= 1))
        ship->DoUncloak();
}

// FUNCTION: EVNOVA 0x0041ad50
// MAC: _DeleteAllComputerShips i386 0x0003bf5f ppc 0x00042594
void DeleteAllComputerShips(Boolean force, Boolean keepPlayer)
{
    short i;
    Boolean remove;

    for (i = 1; i < kMaxShips; i++) {
        remove = 1;
        if (AIHasLeader(shipData[i].aiType) && shipData[i].parentShip == 0
                && shipData[i].defenseStellar == -1 && shipData[i].specialMission == -1) {
            if (shipData[i].IsDisabled())
                remove = 1;
            else if (force == 0)
                remove = 0;
        }
        if (remove) {
            if (shipData[i].active != 0 && shipData[i].auxMission != -1) {
                    if (currentMissionObjectives[shipData[i].auxMission].active != 0)
                        currentMissions[shipData[i].auxMission].auxShipsLeft++;
                    if (currentMissions[shipData[i].auxMission].auxShipsLeft
                            > currentMissions[shipData[i].auxMission].auxShipCount)
                        currentMissions[shipData[i].auxMission].auxShipsLeft =
                            currentMissions[shipData[i].auxMission].auxShipCount;
            } else if (shipData[i].active != 0 && shipData[i].defenseStellar != -1) {
                stellarData[shipData[i].defenseStellar].defWaveSize++;
                if (stellarData[shipData[i].defenseStellar].defWaveSize
                        > stellarData[shipData[i].defenseStellar].defCount)
                    stellarData[shipData[i].defenseStellar].defWaveSize =
                        stellarData[shipData[i].defenseStellar].defCount;
            }
            shipData[i].active = 0;
            shipData[i].specialMission = -1;
            shipData[i].defenseStellar = -1;
            shipData[i].tractorShip = -1;
            shipData[i].hired = 0;
            shipData[i].hailed = 0;
            shipData[i].auxMission = -1;
            shipData[i].curSystem = -1;
            shipData[i].parentShip = -1;
        }
    }
}

#define kSystemPersSlots        8
#define kMissionShipSystemHere  (-6)    /* mïsn +0x10: spawn in the player's system */
#define kMissionShipCloaked     2       /* mïsn +0x0e */
#define kNavStelFirst           (-1)    /* mïsn +0x0e: -1..-16 index navDefault */
#define kNavStelLast            (-16)
#define kSpawnOdds              7
#define kRandomSpawnReserve     8


// FUNCTION: EVNOVA 0x0041af90
// MAC: _SetupShipsInSystem i386 0x00042b61 ppc 0x0004a668
void SetupShipsInSystem(short sys, unsigned char arriving)
{
    float total;
    float step;
    short i;
    short j;
    short m;
    short k;
    short slot;
    short sysOf;
    short count;
    short govt;
    short navIndex;
    short shipSyst;

    DeleteAllComputerShips(0, 1);

    for (i = 1; i < kMaxShips; i++) {
        if (shipData[i].parentShip == 0 && shipData[i].active != 0) {
            if (shipData[i].IsDisabled() != 0) {
                shipData[i].active = 0;
                if (shipData[i].parentShip == 0
                        && shipData[i].aiType == kAIEscort
                        && shipData[i].specialMission == -1
                        && AIIsTrader(shipClass[shipData[i].type].inherentAI))
                    DestroyPartialFleetCargo(i);
            } else {
                if (shipData[i].specialMission == -1) {
                    if (shipData[i].IsDisabled()) {
                        shipData[i].active = 0;
                        shipData[i].parentShip = -1;
                    } else {
                        RespawnEscort(&shipData[i], arriving);
                    }
                }
            }
        }
    }

    AICalcFormationPositions(shipData, 1);

    if (shipData->jumpTimer != 0.0f) {
        total = 0.0f;
        step = 45.0f;
        while (step > 0.0f) {
            total += step;
            step = step - 1.165;
        }
        for (i = 1; i < kMaxShips; i++) {
            if (shipData[i].parentShip == 0 && shipData[i].active != 0) {
                Accel(((short)shipData[i].heading + kHalfCircle) % kFullCircle,
                      total, &shipData[i].pos);
                shipData[i].jumpTimer = -999.0f;
                shipData[i].jumpTick = 0;
                Accel(shipData[i].heading, 50.0f,
                      &shipData[i].vel);
            }
        }
    }

    k = 0;
    for (m = 0; m < kMaxActiveMissions; m++) {
        if (currentMissionObjectives[m].active != 0) {
            shipSyst = currentMissions[m].shipSyst;
            if (shipSyst >= 0 && shipSyst < kMaxSystems)
                sysOf = FindActiveCoLocatedSystem(shipSyst);
            else
                sysOf = -1;
            if ((sysOf == sys || currentMissions[m].shipSyst == kMissionShipSystemHere)
                    && currentMissions[m].shipCount > 0
                    && currentMissions[m].shipDelay < 0) {
                count = currentMissions[m].shipCount;
                if (currentMissions[m].shipSyst == kMissionShipSystemHere
                        && currentMissions[m].shipBehav == kMissionModeEscort) {
                    for (j = 1; j < kMaxShips; j++) {
                        if (shipData[j].active != 0 && m == shipData[j].specialMission)
                            count--;
                    }
                }
                if (count > 0) {
                    for (i = 0; i < count; i++) {
                        slot = MissionDudeSpawn(currentMissions[m].shipDude, -1, sys, m);
                        if (slot != -1) {
                            k++;
                            if (currentMissions[m].shipGoal == kMissionGoalEscort) {
                                shipData[slot].pos.x = Rand(0x200) - 0x100;
                                shipData[slot].pos.y = Rand(0x200) - 0x100;
                            }
                            if (currentMissions[m].shipGoal == kMissionGoalRescue) {
                                shipData[slot].heading = Rand(kFullCircle);
                                shipData[slot].vel.x = shipData[slot].vel.y = 0.0f;
                                shipData[slot].accelMag = 0.0f;
                                shipData[slot].shield = 0.0f;
                                if ((shipClass[shipData[slot].type].flags
                                        & kShipArmorDisableTenth) != 0)
                                    shipData[slot].armor =
                                        shipClass[shipData[slot].type].armor * 0.1 - 1.0;
                                else
                                    shipData[slot].armor =
                                        shipClass[shipData[slot].type].armor * 0.33 - 1.0;
                                if (currentMissions[m].timeLimit <= 0
                                        && currentMissions[m].timeLimit > -32000)
                                    shipData[slot].boarded = 1;
                            }
                            if (currentMissions[m].shipStart <= kNavStelFirst
                                    && currentMissions[m].shipStart >= kNavStelLast) {
                                navIndex = -1 - currentMissions[m].shipStart;
                                if (systemData[sys].navDefault[navIndex] != -1) {
                                    shipData[slot].pos.x = stellarData[
                                        systemData[sys].navDefault[navIndex]].pos.h;
                                    shipData[slot].pos.y = stellarData[
                                        systemData[sys].navDefault[navIndex]].pos.v;
                                }
                            }
                            if (currentMissions[m].shipBehav == kMissionModeEscort) {
                                shipData[slot].aiType = kAIEscort;
                                shipData[slot].parentShip = 0;
                                shipData[slot].formLeader = 0;
                                shipData[slot].swarmLeader = 0;
                                if (arriving != 0)
                                    shipData[slot].heading = shipData->heading;
                            }
                            shipData[slot].lastStellar = -2;
                            if (currentMissions[m].shipStart == kMissionShipCloaked)
                                shipData[slot].DoCloak();
                        } else {
                            crashlog_print("warning: couldn't create ship for mission ID ");
                            crashlog_print_num(currentMissions[m].misnIndex + kFirstResourceID);
                            crashlog_print("\n");
                        }
                    }
                    if ((currentMissions[m].flags & kMissionAutoAbort) != 0)
                        AutoAbortMission(m);
                }
            }
        }
    }

    if (arriving != 0)
        AICalcFormationPositions(shipData, 1);

    for (i = 0; i < systemData[sys].avgShips; i++) {
        slot = -1;
        if (Rand(kSpawnOdds) == 0) {
            slot = SpawnPerson(shipData->curSystem, 0, -1);
            if (slot != -1) {
                govt = shipData[slot].govt;
                if (govt != -1
                        && (govtData[govt].flags & kGovtAlwaysDisabled) != 0) {
                    shipData[slot].accelMag = 0.0f;
                    shipData[slot].vel.x = 0.0f;
                    shipData[slot].vel.y = 0.0f;
                }
            }
        } else if (Rand(kSpawnOdds) == 0) {
            SpawnFleet(shipData->curSystem, 0);
        } else {
            slot = RandomShipSpawn(sys, kRandomSpawnReserve);
        }
        if (slot != -1) {
            AdjustedAccel(shipData[slot].heading, shipClass[shipData[slot].type].maxSpeed,
                          &shipData[slot].vel, shipClass[shipData[slot].type].maxSpeed);
        }
    }

    for (i = 0; i < kSystemPersSlots; i++) {
        if (systemData[sys].persons[i] != -1
                && personData[systemData[sys].persons[i]].loaded != 0
                && EvalMissionBitTestString(personData[systemData[sys].persons[i]].activeOn)
                && Rand(100) + 1 <= systemData[sys].persProbs[i]) {
            slot = SpawnPerson(shipData->curSystem, 0, systemData[sys].persons[i]);
            if (slot != -1) {
                AdjustedAccel(shipData[slot].heading, shipClass[shipData[slot].type].maxSpeed,
                              &shipData[slot].vel, shipClass[shipData[slot].type].maxSpeed);
                govt = shipData[slot].govt;
                if (govt != -1
                        && (govtData[govt].flags & kGovtAlwaysDisabled) != 0) {
                    shipData[slot].vel.x = 0.0f;
                    shipData[slot].vel.y = 0.0f;
                    shipData[slot].accelMag = 0.0f;
                }
            }
        }
    }
}

#define kSpawnSpread     1500    /* Rand(1500) - 750 on each axis */
#define kSpawnOrigin     750
#define kSpawnSpeed      100.0f
#define kStartCredits    10000

// FUNCTION: EVNOVA 0x0041ba80
// MAC: _RandomShipSpawn i386 0x0003c0f3 ppc 0x00042798
short RandomShipSpawn(short sys, short reserve)
{
    short slot;
    short dude;
    short pick;
    short link;
    short i;

    reserve = abs(reserve);
    for (slot = 1; slot < kMaxShips - reserve; slot++) {
        if (shipData[slot].active != 0)
            continue;
        dude = SelectDudeFieldFromSystem(&systemData[sys]);
        if (dude == -1)
            continue;
        dude = systemData[sys].dudeTypes[dude];
        pick = SelectShipFieldFromDude(&dudeData[dude], 0);
        if (pick < 0 || pick >= kDudeShipSlots)
            goto fail;
        shipData[slot].active = 1;
        shipData[slot].curSystem = sys;
        shipData[slot].dude = dude;
        shipData[slot].type = dudeData[dude].shipTypes[pick];
        shipData[slot].govt = dudeData[dude].govt;
        if (AIIsSet(dudeData[dude].aiType))
            shipData[slot].aiType = dudeData[dude].aiType;
        else
            shipData[slot].aiType = shipClass[shipData[slot].type].inherentAI;
        /* a stationary warship starts beside the system's first stellar */
        if (shipData[slot].aiType == kAIWarship &&
            systemData[shipData[slot].curSystem].navDefault[0] != -1 &&
            shipClass[shipData[slot].type].maxSpeed == 0.0) {
            link = systemData[shipData[slot].curSystem].navDefault[0];
            shipData[slot].pos.x = stellarData[link].pos.h;
            shipData[slot].pos.y =
                stellarData[systemData[shipData[slot].curSystem].navDefault[0]].pos.v;
            Accel(Rand(kFullCircle), kSpawnSpeed, &shipData[slot].pos);
        } else {
            shipData[slot].pos.x = Rand(kSpawnSpread) - kSpawnOrigin;
            shipData[slot].pos.y = Rand(kSpawnSpread) - kSpawnOrigin;
        }
        shipData[slot].vel.x = shipData[slot].vel.y = 0;
        shipData[slot].accelMag = 0;
        shipData[slot].navTargetKind = -1;
        shipData[slot].jumpTimer = 0;
        shipData[slot].aiState = kAIStateHiJack;
        shipData[slot].aiSubstate = kSubLoJack;
        shipData[slot].boarded = 0;
        shipData[slot].formerEscort = -1;
        shipData[slot].cloakDir = 0;
        shipData[slot].cloak = 0;
        shipData[slot].specialMission = -1;
        shipData[slot].grudge = 0;
        shipData[slot].aiTimer = 0;
        shipData[slot].heading = Rand(kFullCircle);
        shipData[slot].parentShip = -1;
        shipData[slot].targetShip = -1;
        shipData[slot].targetStellar = -1;
        shipData[slot].deathTimer = 0;
        shipData[slot].ionization = 0;
        shipData[slot].ionColor = 0;
        shipData[slot].lightLevel = 0.0f;
        shipData[slot].blinkCount = 0;
        shipData[slot].altSet = 0;
        shipData[slot].animSet = 0;
        shipData[slot].escapePods = shipClass[shipData[slot].type].podCount;
        shipData[slot].foldSet =
            shipClass[shipData[slot].type].baseSets - 1;
        shipData[slot].foldDir = 0;
        shipData[slot].frameTimer = 0;
        shipData[slot].shieldGlow = 0;
        shipData[slot].weapGlow = 0;
        shipData[slot].annoyance = 0.0f;
        shipData[slot].bankDir = 0;
        shipData[slot].skill = RandomSkillLevel(shipData[slot].type);
        shipData[slot].credits = kStartCredits;
        shipData[slot].aggression = Rand(3) ^ 2;
        shipData[slot].shield = shipClass[shipData[slot].type].shield;
        shipData[slot].armor = shipClass[shipData[slot].type].armor;
        shipData[slot].fuel = shipClass[shipData[slot].type].fuel;
        shipData[slot].defenseStellar = -1;
        shipData[slot].tractorShip = -1;
        shipData[slot].hired = 0;
        shipData[slot].hailed = 0;
        shipData[slot].hasAfterburner = AIHasAfterburner(&shipData[slot]);
        shipData[slot].canScoop = shipData[slot].CanScoop();
        shipData[slot].auxMission = -1;
        shipData[slot].escortOrder = -1;
        shipData[slot].persType = -1;
        shipData[slot].thrust = 0;
        shipData[slot].speedLimit = 0;
        shipData[slot].fireQueued = 0;
        shipData[slot].odds = -1.0f;
        shipData[slot].engineGlow = 0x20;
        shipData[slot].aimWeapon = -1;
        shipData[slot].fireTick = 0;
        shipData[slot].hailTick = 0;
        shipData[slot].aimWeapon = -1;
        shipData[slot].seesCloaked[0] = -1;
        shipData[slot].seesCloaked[1] = -1;
        shipData[slot].decloaksWhenHit = -1;
        shipData[slot].animSet = 0;
        shipData[slot].animTimer = 0;
        shipData[slot].sellOnLanding = 0;
        shipData[slot].upgradeOnLanding = 0;
        shipData[slot].fuelRegenState = -1;
        shipData[slot].escortMayAttack = 0;
        shipData[slot].voiceClass = Rand(2);
        for (i = 0; i < 4; i++)
            shipData[slot].jamming[i] = -1;
        if (shipData[slot].govt != -1 &&
            (govtData[shipData[slot].govt].flags & kGovtAlwaysDisabled) != 0)
            shipData[slot].engineGlow = 0;
        if (shipClass[shipData[slot].type].baseSets > 0)
            shipData[slot].animSet =
                Rand(shipClass[shipData[slot].type].baseSets);
        if (shipClass[shipData[slot].type].animDelay > 0)
            shipData[slot].animTimer =
                Rand(shipClass[shipData[slot].type].animDelay);
        for (i = 0; i < kMaxWeapTypes; i++) {
            shipData[slot].weap[i].count =
                shipClass[shipData[slot].type].weapCount[i];
            shipData[slot].weap[i].ammo =
                shipClass[shipData[slot].type].weapAmmo[i];
        }
        AISetup(&shipData[slot]);
        return slot;
fail:
        shipData[slot].active = 0;
        return -1;
    }
    return -1;
}

/*
 * 0x004235c0 = SpawnPerson (mac `_SpawnPerson` at the same call position, same
 * (sys, 1, -1) arguments); 0x00425280 = SpawnFleet, call-position proven.
 */

// FUNCTION: EVNOVA 0x0041c710
// MAC: _HyperShipSpawn i386 0x0004291a ppc 0x0004a36c
short HyperShipSpawn(short sys)
{
    short slot;
    short stellar;
    float speed;
    float dist;
    FloatPoint pt;

    if (Rand(7) == 0) {
        slot = SpawnPerson(sys, 1, -1);
    } else if (Rand(7) == 0) {
        slot = -1;
        SpawnFleet(sys, 1);
    } else {
        slot = RandomShipSpawn(sys, kSpawnReserve);
        if (slot != -1) {
            if (shipData[slot].FuelCapacity() <= 0) {
                shipData[slot].active = 0;
                return -1;
            }
        }
    }
    if (slot != -1) {
        dist = 0;
        speed = 50.0f;
        while (speed > 0) {
            dist += speed;
            speed -= 1.165;
        }
        shipData[slot].pos.x = shipData[slot].pos.y = 0;
        Accel(Rand(kFullCircle), dist + 1000.0f, &shipData[slot].pos);
        pt.x = pt.y = 0;
        shipData[slot].heading =
            CalcInterceptAngle(&shipData[slot].pos, &pt);
        shipData[slot].lastStellar = -2;
        shipData[slot].prevSystem = -1;
        shipData[slot].vel.x = 0;
        shipData[slot].vel.y = 0;
        shipData[slot].accelMag = 0;
        stellar = PickEmergeStellar(&shipData[slot]);
        if (stellar != -1) {
            AIMakeShipEmergeFromHyperGate(&shipData[slot], stellar);
            shipData[slot].pos.x = stellarData[stellar].pos.h;
            shipData[slot].pos.y = stellarData[stellar].pos.v;
        } else {
            shipData[slot].jumpTimer = -999;
            shipData[slot].jumpTick = ReadTick();
            Accel(shipData[slot].heading, 50.0f,
                  &shipData[slot].vel);
            AIMakeShipJumpIn(&shipData[slot]);
        }
        return slot;
    }
    return -1;
}

// FUNCTION: EVNOVA 0x0041c9f0
// MAC: _SpecificDudeSpawn i386 0x0003d0eb ppc 0x00043c0c
short SpecificDudeSpawn(short dude, short sys, short reserve,
                        unsigned char allowAll)
{
    short slot;
    short pick;
    short i;

    slot = GenericRandomShipSpawn(sys, reserve);
    if (slot == -1)
        goto none;
    pick = SelectShipFieldFromDude(&dudeData[dude], allowAll);
    if (pick < 0 || pick >= kDudeShipSlots)
        goto fail;
    shipData[slot].dude = dude;
    shipData[slot].type = dudeData[dude].shipTypes[pick];
    shipData[slot].govt = dudeData[dude].govt;
    if (AIIsSet(dudeData[dude].aiType))
        shipData[slot].aiType = dudeData[dude].aiType;
    else
        shipData[slot].aiType = shipClass[shipData[slot].type].inherentAI;
    AISetup(&shipData[slot]);
    for (i = 0; i < kMaxWeapTypes; i++) {
        shipData[slot].weap[i].count = shipClass[shipData[slot].type].weapCount[i];
        shipData[slot].weap[i].ammo = shipClass[shipData[slot].type].weapAmmo[i];
    }
    shipData[slot].shield = shipClass[shipData[slot].type].shield;
    shipData[slot].armor = shipClass[shipData[slot].type].armor;
    return slot;

fail:
    shipData[slot].active = 0;
    return -1;
none:
    return -1;
}

// FUNCTION: EVNOVA 0x0041cf40
// MAC: _MissionDudeSpawn i386 0x0003cd3b ppc 0x00043784
short MissionDudeSpawn(short dude, short wantType, short sys, short mission)
{
    short slot;
    short pick;
    short i;

    slot = GenericRandomShipSpawn(sys, kSpawnReserve);
    if (slot == -1)
        goto none;
    Rand(100);
    if (wantType != -1 &&
        (currentMissions[mission].flags & kMissionFixedShipType) != 0 &&
        currentMissions[mission].shipCount == 1) {
        for (i = 0; i < kDudeShipSlots; i++) {
            if (wantType == dudeData[dude].shipTypes[i]) {
                currentMissions[mission].shipTypeIndex = i;
                break;
            }
        }
    }
    if (currentMissions[mission].shipTypeIndex != -1)
        pick = currentMissions[mission].shipTypeIndex;
    else {
        pick = SelectShipFieldFromDude(&dudeData[dude], 0);
        if (pick == -1)
            pick = SelectShipFieldFromDude(&dudeData[dude], 1);
    }
    if (pick < 0 || pick > kDudeShipSlots)
        goto fail;
    shipData[slot].specialMission = mission;
    shipData[slot].dude = dude;
    shipData[slot].type = dudeData[dude].shipTypes[pick];
    shipData[slot].govt = dudeData[dude].govt;
    shipData[slot].boarded = 0;
    shipData[slot].formerEscort = -1;
    shipData[slot].cloakDir = 0;
    shipData[slot].cloak = 0;
    shipData[slot].aiTimer = 0;
    if (AIIsSet(dudeData[dude].aiType))
        shipData[slot].aiType = dudeData[dude].aiType;
    else
        shipData[slot].aiType = shipClass[shipData[slot].type].inherentAI;
    AISetup(&shipData[slot]);
    for (i = 0; i < kMaxWeapTypes; i++) {
        shipData[slot].weap[i].count = shipClass[shipData[slot].type].weapCount[i];
        shipData[slot].weap[i].ammo = shipClass[shipData[slot].type].weapAmmo[i];
    }
    shipData[slot].shield = shipClass[shipData[slot].type].shield;
    shipData[slot].armor = shipClass[shipData[slot].type].armor;
    shipData[slot].escapePods = shipClass[shipData[slot].type].podCount;
    shipData[slot].foldSet =
        shipClass[shipData[slot].type].baseSets - 1;
    shipData[slot].skill = RandomSkillLevel(shipData[slot].type);
    if (shipClass[shipData[slot].type].baseSets > 0)
        shipData[slot].animSet =
            Rand(shipClass[shipData[slot].type].baseSets);
    if (shipClass[shipData[slot].type].animDelay > 0)
        shipData[slot].animTimer =
            Rand(shipClass[shipData[slot].type].animDelay);
    if (currentMissions[mission].shipBehav == kMissionModeAttackPlayer)
        AIMakeShipAttackPlayer(&shipData[slot]);
    return slot;

fail:
    shipData[slot].active = 0;
    return -1;
none:
    return -1;
}

#define kSystemStellars     16
#define kSysHere            (-6)
#define kMisnTimerMax       32000

#define kFleetOdds          500
#define kFleetProbRange     100
#define kEscortFleetHeavy   0xff
#define kEscortFleetLight   0xfe
#define kEscortCargoHeavy   200
#define kEscortCargoLight   50
#define kDefWaveModulus     10

#define kEntryHeadings      360
#define kEntryJitter        512
#define kEntryJitterBias    256.0f
#define kEntryRadius        1000.0f
#define kEntryStep          50.0f
#define kEntryStepDecay     1.165
#define kEntryShove         50.0f
#define kEntryDepth         (-999.0f)


// FUNCTION: EVNOVA 0x0041d6e0
// MAC: _EnterMoreShips i386 0x00043459 ppc 0x0004b11c
void EnterMoreShips(short sys)
{
    FloatPoint spot;
    FloatPoint origin;
    short count;
    short n;
    short i;
    short left;
    short j;
    short toSpawn;
    short which;
    short heading;
    FloatPoint dest;
    short cargo;
    short roll;
    short pick;
    float dist;
    float step;

    count = 0;
    for (i = 0; i < kMaxActiveMissions; i++) {
        if (currentMissionObjectives[i].active) {
            if (currentMissions[i].auxShipDelay <= 0) {
                if (currentMissions[i].auxShipDude != -1
                    && currentMissions[i].auxShipsSpawned < currentMissions[i].auxShipsLeft) {
                    left = currentMissions[i].auxShipsLeft - currentMissions[i].auxShipsSpawned;
                    if (ValidAuxShipSystem(shipData[0].curSystem, i)
                        && left > 0) {
                        dist = 0.0f;
                        for (step = kEntryStep; step > 0.0f;
                             step = step - kEntryStepDecay)
                            dist = dist + step;
                        heading = Rand(kEntryHeadings);
                        spot.x = spot.y = 0.0f;
                        Accel(heading, dist + kEntryRadius, &spot);
                        for (count = 0; count < left; count++) {
                            which = SpecificDudeSpawn(currentMissions[i].auxShipDude, sys,
                                                      kSpawnReserve, 0);
                            if (which != -1) {
                                shipData[which].auxMission = i;
                                currentMissions[i].auxShipsSpawned += 1;
                                if ((currentMissions[i].flags & kMissionInfiniteAuxShips) == 0)
                                    currentMissions[i].auxShipsLeft -= 1;
                                shipData[which].pos.x =
                                    spot.x + Rand(kEntryJitter) - kEntryJitterBias;
                                shipData[which].pos.y =
                                    spot.y + Rand(kEntryJitter) - kEntryJitterBias;
                                dist = 0.0f;
                                for (step = kEntryStep; step > 0.0f;
                                     step = step - kEntryStepDecay)
                                    dist = dist + step;
                                origin.x = origin.y = 0.0f;
                                shipData[which].heading = CalcInterceptAngle(
                                    &shipData[which].pos, &origin);
                                shipData[which].lastStellar = -2;
                                shipData[which].prevSystem = -2;
                                shipData[which].vel.x = shipData[which].vel.y = 0;
                                shipData[which].accelMag = 0;
                                shipData[which].jumpTimer = kEntryDepth;
                                shipData[which].jumpTick = ReadTick();
                                Accel(shipData[which].heading, kEntryShove,
                                      &shipData[which].vel);
                            }
                        }
                        if (currentMissions[i].flags & kMissionAutoAbort)
                            AutoAbortMission(i);
                    }
                }
            }
            count = 0;
            toSpawn = 0;
            if (currentMissions[i].shipSyst >= 0 && currentMissions[i].shipSyst < kMaxSystems)
                pick = FindActiveCoLocatedSystem(currentMissions[i].shipSyst);
            else
                pick = -1;
            if (currentMissions[i].shipSyst == kSysHere
                || pick == shipData[0].curSystem)
                toSpawn = currentMissions[i].shipCount;
            if (currentMissions[i].shipStart == 1) {
                toSpawn = currentMissions[i].shipCount - currentMissions[i].specialShipsJumpedIn;
                if (currentMissions[i].shipSyst != kSysHere
                    && pick != shipData[0].curSystem
                    && currentMissions[i].specialShipsJumpedIn <= 0)
                    toSpawn = 0;
            }
            if (toSpawn > 0) {
                if (currentMissions[i].shipDelay > 0)
                    if (currentMissions[i].shipDelay <= kMisnTimerMax)
                        currentMissions[i].shipDelay -= 1;
                if (currentMissions[i].shipCount > 0 && currentMissions[i].shipDelay == 0) {
                    currentMissions[i].shipDelay = -1;
                    dist = 0.0f;
                    for (step = kEntryStep; step > 0.0f;
                         step = step - kEntryStepDecay)
                        dist = dist + step;
                    if (shipData[0].prevSystem == -1) {
                        heading = Rand(kEntryHeadings);
                    } else {
                        origin.x =
                            systemData[shipData[0].curSystem].pos.h;
                        origin.y =
                            systemData[shipData[0].curSystem].pos.v;
                        dest.x =
                            systemData[shipData[0].prevSystem].pos.h;
                        dest.y =
                            systemData[shipData[0].prevSystem].pos.v;
                        heading = CalcInterceptAngle(&origin, &dest);
                    }
                    spot.x = spot.y = 0.0f;
                    Accel(heading, dist + kEntryRadius, &spot);
                    for (n = 0; n < toSpawn; n++) {
                        /* BUG: reuses count for the ship index, so the tally below inherits it */
                        count = MissionDudeSpawn(currentMissions[i].shipDude, -1, sys, i);
                        if (count != -1) {
                            currentMissions[i].specialShipsJumpedIn += 1;
                            shipData[count].pos.x =
                                spot.x + Rand(kEntryJitter) - kEntryJitterBias;
                            shipData[count].pos.y =
                                spot.y + Rand(kEntryJitter) - kEntryJitterBias;
                            dist = 0.0f;
                            for (step = kEntryStep; step > 0.0f;
                                 step = step - kEntryStepDecay)
                                dist = dist + step;
                            origin.x = origin.y = 0.0f;
                            shipData[count].heading = CalcInterceptAngle(
                                &shipData[count].pos, &origin);
                            shipData[count].lastStellar = -2;
                            shipData[count].prevSystem = -2;
                            shipData[count].vel.x = shipData[count].vel.y = 0;
                            shipData[count].accelMag = 0;
                            shipData[count].jumpTimer = kEntryDepth;
                            shipData[count].jumpTick = ReadTick();
                            Accel(shipData[count].heading, kEntryShove,
                                  &shipData[count].vel);
                        }
                    }
                    if (currentMissions[i].flags & kMissionAutoAbort)
                        AutoAbortMission(i);
                }
            }
            if (currentMissions[i].auxShipDelay > 0)
                if (currentMissions[i].auxShipDelay <= kMisnTimerMax)
                    currentMissions[i].auxShipDelay -= 1;
        }
    }

    for (n = 1; n < kMaxShips; n++)
        if (shipData[n].active && sys == shipData[n].curSystem
            && shipData[n].parentShip != 0)
            count++;
    if (count < systemData[sys].avgShips) {
        roll = Rand(kFleetOdds);
        if (roll == 1) {
            if (systemData[sys].fleetCount > 0
                && Rand(kFleetProbRange) + 1 <= systemData[sys].fleetProbTotal) {
                pick = SelectFleetFromSystem(&systemData[sys]);
                if (pick != -1)
                    HyperSpawnFleet(sys, pick, kAINone, 1);
                else
                    roll = 0;
            } else {
                roll = 0;
            }
        }
        if (roll == 0) {
            pick = SelectDudeFieldFromSystem(&systemData[sys]);
            if (pick != -1)
                HyperShipSpawn(sys);
        }
    }

    for (j = 0; j < kSystemStellars; j++) {
        if (systemData[sys].navDefault[j] != -1
            && stellarData[systemData[sys].navDefault[j]].defenseActive != 0
            && stellarData[systemData[sys].navDefault[j]].defWaveSize > 0) {
            count = 0;
            for (n = 1; n < kMaxShips; n++)
                if (shipData[n].active && shipData[n].defenseStellar != -1
                    && shipData[n].defenseStellar == systemData[sys].navDefault[j])
                    count++;
            i = stellarData[systemData[sys].navDefault[j]].defCount % kDefWaveModulus;
            if (count < i) {
                SpawnStellarDefenseShip(systemData[sys].navDefault[j]);
                stellarData[systemData[sys].navDefault[j]].defWaveSize -= 1;
                break;
            }
        }
    }

    if (antiEscortDelay == 0) {
        antiEscortDelay = -1;
        cargo = 0;
        for (n = 1; n < kMaxShips; n++)
            if (shipData[n].active && shipData[n].parentShip == 0
                && shipData[n].aiType == kAIEscort && shipData[n].specialMission == -1
                && AIIsTrader(shipClass[shipData[n].type].inherentAI))
                cargo += shipClass[shipData[n].type].cargo;
        if (cargo >= kEscortCargoHeavy)
            HyperSpawnFleet(shipData[0].curSystem, kEscortFleetHeavy,
                            kAIInterceptor, 1);
        else if (cargo >= kEscortCargoLight)
            HyperSpawnFleet(shipData[0].curSystem, kEscortFleetLight,
                            kAIInterceptor, 1);
    } else {
        antiEscortDelay -= 1;
    }
}

// FUNCTION: EVNOVA 0x0041e240
// MAC: _RespawnEscort i386 0x0003d4b8 ppc 0x00044098
void RespawnEscort(ShipObj *ship, unsigned char full)
{
    short i;

    if (ship->parentShip != 0)
        return;

    ship->lastStellar = -2;
    ship->prevSystem = -2;
    ship->thrust = 0;
    ship->speedLimit = 0;
    ship->persType = -1;
    ship->fireQueued = 0;
    ship->odds = -1.0f;
    ship->engineGlow = 0;
    ship->fireTick = 0;
    ship->hailTick = 0;
    ship->aimWeapon = -1;
    ship->dude = -1;
    ship->persType = -1;
    ship->specialMission = -1;
    ship->ionization = 0;
    ship->escortMayAttack = 0;
    if (full) {
        ship->shield = ship->ShieldCapacity();
        ship->armor = ship->ArmorCapacity();
    }
    ship->curSystem = shipData->curSystem;
    ship->heading = shipData->heading;
    ship->swarmLeader = -1;
    ship->formLeader = 0;
    ship->formPos.x = 0;
    ship->formPos.y = 0;
    shipData->hasFormation = 1;
    ship->vel.x = ship->vel.y = 0;
    ship->accelMag = 0;
    if (full) {
        for (i = 0; i < kMaxWeapTypes; i++) {
            ship->weap[i].count = shipClass[ship->type].weapCount[i];
            ship->weap[i].ammo = shipClass[ship->type].weapAmmo[i];
        }
    }
    for (i = 0; i < kCargoTypes; i++)
        ship->cargo[i] = 0;
    AIMakeEscortFlyInForm(ship);
}

/*
 * The fighterType parameter is the WEAPON index of the carrier's bay: the
 * spawned hull is that weapon's ammoType - 128, the heading jitter is its
 * inaccuracy, the launch speed its speed100, and unk4C its lifetime.
 */

// FUNCTION: EVNOVA 0x0041e640
// MAC: _LaunchFighter i386 0x0003d62d ppc 0x00044238
unsigned char LaunchFighter(ShipObj *parent, short fighterType)
{
    short slot;
    short i;
    short n;
    short inacc;

    if (fighterType < 0 || fighterType >= kMaxWeapTypes)
        return 0;
    slot = GenericRandomShipSpawn(parent->curSystem, kSpawnReserve);
    if (slot == -1)
        goto none;
    shipData[slot].active = 1;
    shipData[slot].curSystem = parent->curSystem;
    shipData[slot].pos.x = parent->pos.x;
    shipData[slot].pos.y = parent->pos.y;
    shipData[slot].vel.x = parent->vel.x;
    shipData[slot].vel.y = parent->vel.y;
    shipData[slot].accelMag = parent->accelMag;
    shipData[slot].type = weaponStats[fighterType].ammoType - kFirstResourceID;
    shipData[slot].defenseStellar = -1;
    shipData[slot].tractorShip = -1;
    shipData[slot].hailed = 0;
    shipData[slot].hasAfterburner = AIHasAfterburner(&shipData[slot]);
    shipData[slot].canScoop = shipData[slot].CanScoop();
    shipData[slot].hired = 0;
    shipData[slot].auxMission = -1;
    shipData[slot].aiType = kAIFighter;
    shipData[slot].govt = parent->govt;
    shipData[slot].navTargetKind = -1;
    shipData[slot].jumpTimer = 0;
    shipData[slot].aiState = kAIStateHiJack;
    shipData[slot].aiSubstate = kSubLoJack;
    shipData[slot].boarded = 0;
    shipData[slot].formerEscort = -1;
    shipData[slot].cloakDir = 0;
    shipData[slot].cloak = 0;
    shipData[slot].grudge = 0;
    shipData[slot].lastStellar = -2;
    shipData[slot].prevSystem = -2;
    shipData[slot].shield = shipClass[shipData[slot].type].shield;
    shipData[slot].armor = shipClass[shipData[slot].type].armor;
    shipData[slot].aiTimer = weaponStats[fighterType].lifetime;
    shipData[slot].parentShip = parent->index;
    shipData[slot].heading = parent->heading;
    shipData[slot].persType = -1;
    shipData[slot].dude = -1;
    shipData[slot].specialMission = -1;
    shipData[slot].ionization = 0;
    shipData[slot].ionColor = 0;
    shipData[slot].lightLevel = 0;
    shipData[slot].blinkCount = 0;
    shipData[slot].altSet = 0;
    shipData[slot].animSet = 0;
    shipData[slot].escapePods = shipClass[shipData[slot].type].podCount;
    shipData[slot].foldSet =
        shipClass[shipData[slot].type].baseSets - 1;
    shipData[slot].foldDir = 0;
    shipData[slot].frameTimer = 0;
    shipData[slot].shieldGlow = 0;
    shipData[slot].weapGlow = 0;
    shipData[slot].annoyance = 0;
    shipData[slot].bankDir = 0;
    shipData[slot].animTimer = 0;
    shipData[slot].seesCloaked[0] = -1;
    shipData[slot].seesCloaked[1] = -1;
    shipData[slot].decloaksWhenHit = -1;
    shipData[slot].escortOrder = -1;
    shipData[slot].sellOnLanding = 0;
    shipData[slot].upgradeOnLanding = 0;
    shipData[slot].fuelRegenState = -1;
    shipData[slot].escortMayAttack = 0;
    shipData[slot].voiceClass = Rand(2);
    shipData[slot].shield = shipData[slot].ShieldCapacity();
    shipData[slot].armor = shipData[slot].ArmorCapacity();
    if (shipClass[shipData[slot].type].inherentGovtB != -1
        && govtData[shipClass[shipData[slot].type].inherentGovtB].voiceClass != -1)
        shipData[slot].voiceClass =
            govtData[shipClass[shipData[slot].type].inherentGovtB].voiceClass;
    for (i = 0; i < 4; i++)
        shipData[slot].jamming[i] = -1;
    if (shipClass[shipData[slot].type].baseSets > 0)
        shipData[slot].animSet =
            Rand(shipClass[shipData[slot].type].baseSets);
    if (shipClass[shipData[slot].type].animDelay > 0)
        shipData[slot].animTimer =
            Rand(shipClass[shipData[slot].type].animDelay);
    if (shipData[slot].parentShip == 0
        && shipClass[shipData[slot].type].escortType == 0) {
        n = 0;
        for (i = 1; i < kMaxShips; i++) {
            if (i != slot && shipData[i].active != 0
                && shipData[i].parentShip == shipData[slot].parentShip) {
                n++;
                break;
            }
        }
        if (n == 0 && playerEscortOrders[0] == kOrderRegroup)
            playerEscortOrders[0] = -1;
    } else {
        for (i = 1; i < kMaxShips; i++) {
            if (i != slot && shipData[i].active != 0
                && shipData[i].parentShip == shipData[slot].parentShip
                && shipClass[shipData[i].type].escortType ==
                   shipClass[shipData[slot].type].escortType
                && shipData[i].escortOrder != -1) {
                shipData[slot].escortOrder = shipData[i].escortOrder;
                break;
            }
        }
    }
    if (parent->index == 0)
        shipData[slot].targetShip = -1;
    else
        shipData[slot].targetShip = parent->targetShip;
    inacc = weaponStats[fighterType].inaccuracy;
    if (inacc > 0)
        shipData[slot].heading += Rand(inacc * 2) - inacc;
    AdjustedAccel(shipData[slot].heading, weaponStats[fighterType].speed100,
                  &shipData[slot].vel,
                  shipData[slot].MaxSpeed());
    for (i = 0; i < kMaxWeapTypes; i++) {
        shipData[slot].weap[i].count = shipClass[shipData[slot].type].weapCount[i];
        shipData[slot].weap[i].ammo = shipClass[shipData[slot].type].weapAmmo[i];
    }
    if (shipData[slot].targetShip != -1) {
        if (shipData[slot].parentShip == shipData[slot].targetShip
            || (shipData[slot].parentShip ==
                shipData[shipData[slot].targetShip].parentShip
                && shipData[slot].parentShip != -1))
            shipData[slot].targetShip = -1;
    }
    AISetup(&shipData[slot]);
    return 1;
none:
    return 0;
}

#define kUnitsPerBox        5.0      /* pooled double */
#define kMaxBoxesPerShip    0xc

#define kMsgMissionAborted  0x11c
#define kMsgJettisonPlain   0x122
#define kMsgJettisonMission 0x121

#define kBoxFree            (-1.0F)

#define kBoxSpinMax         0x24
#define kBoxTypes           4
#define kBoxHeadingBase     0xb4
#define kBoxHeadingSpread   0x5a
#define kBoxKickBase        0x1e
#define kBoxKickSpread      0x28
#define kBoxKickScale       100.0    /* pooled double */
#define kBoxSpreadA         0xf
#define kBoxSpreadB         0x1e

// FUNCTION: EVNOVA 0x0041f330
// MAC: _JettisonCargo i386 0x0003dffd ppc 0x00044e9c
void JettisonCargo(unsigned char withMissions)
{
    short boxes;
    short total;
    short escortCargo;
    short share;
    double holds;
    double fleetHolds;
    double units;
    short i;
    short j;
    short k;
    unsigned char text[256];
    Boolean toldMission;

    toldMission = 0;
    total = 0;
    for (i = 0; i <= kLastCommodity; i++)
        total += shipData[0].cargo[i];
    for (i = 0; i < kMaxJunks; i++) {
        if (junkData[i].held > 0) {
            total += junkData[i].held;
            junkData[i].held = 0;
        }
    }
    escortCargo = share = total;
    if (withMissions != 0) {
        for (k = 0; k < kMaxActiveMissions; k++) {
            if (currentMissionObjectives[k].active != 0 && currentMissions[k].hasCargo != 0
                && currentMissions[k].cargoType != -1 && currentMissions[k].cargoQty >= 0
                && currentMissions[k].canAbort != 0) {
                total += currentMissions[k].cargoQty;
                share += currentMissions[k].cargoQty;
                currentMissions[k].hasCargo = 0;
                if ((currentMissions[k].flags & kMissionHidden) == 0
                    && playerIsLanded == 0) {
                    GetIndString(text, kGameStrList, kMsgMissionAborted);
                    DisplayComm((PStrChar *)text, kCommFrames);
                    toldMission = 1;
                }
                QuickMissionFailure(k);
                if (total == 0) {
                    total += 1;
                    share += 1;
                }
            }
        }
    }
    fleetHolds = TotalFleetHolds();
    for (i = 0; i < kMaxShips; i++) {
        if ((shipData[i].active != 0 && shipData[i].parentShip == 0
             && shipData[i].aiType == kAIEscort && shipData[i].specialMission == -1
             && AIIsTrader(shipClass[shipData[i].type].inherentAI))
            || i == 0) {
            if (i == 0)
                holds = ShipTotalHolds();
            else
                holds = shipClass[shipData[i].type].cargo;
            units = holds / fleetHolds;
            units = (i == 0) ? share : units * escortCargo;
            if (units > 0.0) {
                if (playerIsLanded == 0) {
                    boxes = (units / kUnitsPerBox);
                    if (boxes <= 0)
                        boxes += 1;
                    if (boxes > kMaxBoxesPerShip)
                        boxes = kMaxBoxesPerShip;
                    for (j = 0; j < boxes; j++)
                        SpawnBox(i);
                }
                statusRedraw[kRedrawCargo] = 1;
            }
        }
    }
    for (i = 0; i <= kLastCommodity; i++)
        shipData[0].cargo[i] = 0;
    if (total > 0 && playerIsLanded == 0) {
        PlaySound(beepSnd[4], 1, gSoundVolume, gSoundVolume);
        if (toldMission == 0) {
            if (withMissions)
                GetIndString(text, kGameStrList, kMsgJettisonMission);
            else
                GetIndString(text, kGameStrList, kMsgJettisonPlain);
            DisplayComm((PStrChar *)text, kCommFrames);
        }
    }
    ResetPlayerPrecalcedValues();
}

// FUNCTION: EVNOVA 0x0041f800
// MAC: _SpawnBox i386 0x0003dd74 ppc 0x00044b44
void SpawnBox(short from)
{
    short slot;
    short i;
    short mag;
    short kind;
    slot = -1;
    for (i = 0; i < kBoxSlots; i++) {
        if (boxData[i].timer < kBoxFree) {
            slot = i;
            break;
        }
    }
    if (slot == -1)
        return;

    boxData[slot].pos.x = shipData[from].pos.x;
    boxData[slot].pos.y = shipData[from].pos.y;
    boxData[slot].vel.x = shipData[from].vel.x;
    boxData[slot].vel.y = shipData[from].vel.y;
    boxData[slot].timer = (Rand(kBoxHeadingSpread) + kBoxHeadingBase);
    boxData[slot].system = shipData[from].curSystem;
    boxData[slot].angle = Rand(kBoxSpinMax);
    boxData[slot].mining = 0;
    boxData[slot].pict = 0;
    kind = Rand(kBoxTypes);
    if (kind == 0)
        boxData[slot].spin = -1;
    if (kind == 1)
        boxData[slot].spin = 0;
    if (kind == 2)
        boxData[slot].spin = 0;
    if (kind == 3)
        boxData[slot].spin = 1;
    mag = (GetSpriteXSize((SpriteRec *)shipData[from].sprite) * 0.5);
    Accel((((short)shipData[from].heading + kBoxHeadingBase) % kFullCircle),
          mag,
          &boxData[slot].pos);
    Accel((((short)shipData[from].heading + kBoxHeadingBase + Rand(kBoxSpreadB) - Rand(kBoxSpreadA)) % kFullCircle),
          (float)(Rand(kBoxKickSpread) + kBoxKickBase) / kBoxKickScale,
          &boxData[slot].vel);
}

/* boxData (0x005914a8) is 64 records of the 0x28 stride both twins index by.
   The four spin tests are INDEPENDENT ifs, not an else-if chain: each arm is
   parked out of line and jumps back to the NEXT test rather than to the join. */


// FUNCTION: EVNOVA 0x0041fb50
// MAC: _SpawnScoopableBox i386 0x0003e3c5 ppc 0x00045328
void SpawnScoopableBox(FloatPoint *pos, short cargoType, short pict)
{
    short slot;
    short i;
    short r;
    slot = -1;
    for (i = 0; i < kMaxBoxes; i++) {
        if (boxData[i].timer < -1.0f) {
            slot = i;
            break;
        }
    }
    if (slot != -1) {
        boxData[slot].pos.x = pos->x;
        boxData[slot].pos.y = pos->y;
        boxData[slot].timer = Rand(200) + 300;
        boxData[slot].system = shipData[0].curSystem;
        boxData[slot].angle = Rand(36);
        boxData[slot].mining = 1;
        boxData[slot].cargoType = cargoType;
        boxData[slot].pict = pict;
        r = Rand(4);
        if (r == 0)
            boxData[slot].spin = -1;
        if (r == 1)
            boxData[slot].spin = 0;
        if (r == 2)
            boxData[slot].spin = 0;
        if (r == 3)
            boxData[slot].spin = 1;
        boxData[slot].vel.x = boxData[slot].vel.y = 0;
        Accel(Rand(kFullCircle), (Rand(81) + 60) * 0.2 * 0.01, &boxData[slot].vel);
        return;
    }
}

// FUNCTION: EVNOVA 0x0041fd30
// MAC: _SpawnShot i386 0x0003e550 ppc 0x00045560
short SpawnShot(short ship, short target, short weapType, Boolean flag)
{
    short slot;
    short guidance;
    short i;
    short angle;
    short collision;
    Boolean intercept;

    if (weapType < 0 || weapType >= kMaxWeapTypes) {
        return -1;
    }
    if (weaponStats[weapType].graphic < 0 || weaponStats[weapType].graphic >= kWeapTypeTop) {
        return -1;
    }

    /* claim the first slot whose life has run under the free threshold */
    slot = -1;
    for (i = 0; i < kMaxShots; i++) {
        if (shotData[i].life < -1.0f) {
            slot = i;
            break;
        }
    }
    if (slot == -1) {
        return -1;
    }
    collision = 0;
    angle = 0;

    /* seed position/velocity/heading from the firing ship, or clear */
    if (0 <= ship && ship < kMaxShips && flag == 0) {
        shotData[slot].pos.x = shipData[ship].pos.x;
        shotData[slot].pos.y = shipData[ship].pos.y;
        shotData[slot].vel.x = shipData[ship].vel.x;
        shotData[slot].vel.y = shipData[ship].vel.y;
        angle = shipData[ship].heading;
        intercept = 0;
        guidance = weaponStats[weapType].guidance;
        if (guidance == kGuidanceTurret || guidance == kGuidanceFrontQuad || guidance == kGuidanceRearQuad) {
            if (target != -1) {
                angle = CalcInterceptAngle(&shipData[ship].pos,
                                           &shipData[target].pos);
                intercept = 1;
            }
            else {
                if (guidance == kGuidanceFrontQuad) {
                    angle = shipData[ship].heading;
                }
            }
        }
        if (ship == 0) {
            guidance = weaponStats[weapType].guidance;
            if (guidance != kGuidanceTurret && guidance != kGuidanceFrontQuad
                    && guidance != kGuidanceRearQuad) {
                shotData[slot].vel.x = shipData[ship].vel.x;
                shotData[slot].vel.y = shipData[ship].vel.y;
            }
            if (weaponStats[weapType].guidance == kGuidanceFreefall) {
                shotData[slot].vel.x *= 0.8;
                shotData[slot].vel.y *= 0.8;
            }
        }
    }
    else {
        shotData[slot].pos.x = 0;
        shotData[slot].pos.y = 0;
        shotData[slot].vel.x = 0;
        shotData[slot].vel.y = 0;
        angle = 0;
    }

    /* identity + lifetime */
    shotData[slot].weapType = weapType;
    shotData[slot].owner = ship;
    shotData[slot].target = target;
    shotData[slot].disableOnly = WeaponDisableOnly(weapType);
    shotData[slot].life = weaponStats[weapType].lifetime;
    shotData[slot].subDepth = 0;
    shotData[slot].murk = 0;

    /* the disable-only re-check */
    if (shotData[slot].disableOnly == 0 && 0 < ship && ship < kMaxShips &&
        target != -1) {
        if (shipData[target].IsDisabled() == 0 &&
            AIShipIsPlunderingShip(&shipData[ship], &shipData[target]) != 0) {
            shotData[slot].disableOnly = 1;
        }
    }

    /* hitRoll: aiSkill-driven roll for the guided families */
    if (0 <= ship && ship < kMaxShips && flag == 0) {
        guidance = weaponStats[weapType].guidance;
        if (guidance == kGuidanceTurret || guidance == kGuidancePointDef) {
            if (shipClass[shipData[ship].type].aiSkill > 0) {
                shotData[slot].hitRoll =
                    Rand(shipClass[shipData[ship].type].aiSkill) + 1;
            }
            else {
                shotData[slot].hitRoll = Rand(100) + 1;
            }
        }
        else {
            shotData[slot].hitRoll = -1;
        }
    }

    if ((weaponStats[weapType].flags & kWeapCycleFromFirstFrame) != 0)
        shotData[slot].frame = 0;
    else
        shotData[slot].frame = Rand(0x24);

    shotData[slot].frameTimer = 0;
    shotData[slot].trailCount = 0;
    if (weaponStats[weapType].decay > 0) {
        shotData[slot].trailTimer = 0.0f;
    }
    else {
        shotData[slot].trailTimer = -1.0f;
    }

    /* an AI that is trying to disable its prey marks the shot disable-only */
    if (0 < ship && ship < kMaxShips) {
        if (AIIsShipAttemptingDisable(&shipData[ship]) != 0) {
            shotData[slot].disableOnly = 1;
        }
    }

    if (0 <= ship && ship < kMaxShips) {
        shotData[slot].curSystem = shipData[ship].curSystem;
    }
    else {
        shotData[slot].curSystem = shipData[0].curSystem;
    }

    shotData[slot].angle = angle;
    shotData[slot].guidanceState = 0;

    /* guidance-1 "one shot" roll against the player system's explore level */
    if (weaponStats[weapType].guidance == kGuidanceHoming && (weaponStats[weapType].seeker & 8) != 0) {
        short roll;
        if (gSpeedMult > 0.0) {
            roll = (100.0 / gSpeedMult);
        }
        else {
            roll = 100;
        }
        if (roll < 1) {
            roll = 1;
        }
        if (Rand(roll) + 1 <= systemData[shipData[0].curSystem].interference) {
            shotData[slot].guidanceState = 999;
        }
    }

    if (weaponStats[weapType].durability > 0) {
        shotData[slot].energy = weaponStats[weapType].durability;
    }
    else {
        shotData[slot].energy = 0;
    }

    /* where the round actually starts */
    if (0 <= ship && ship < kMaxShips && flag == 0) {
        if (target != -1)
            angle = ModifyShotStartPosition(&shipData[ship], &shotData[slot].pos, weapType,
                                            &shipData[target].pos);
        else
            angle = ModifyShotStartPosition(&shipData[ship], &shotData[slot].pos, weapType, NULL);
        if (weaponStats[weapType].inaccuracy < 0 && angle != -1
                && weaponStats[weapType].exitType >= 0 && weaponStats[weapType].exitType <= 3) {
            if (shipClass[shipData[ship].type].weapPos[weaponStats[weapType].exitType][angle].h < 0)
                collision = -1;
            else if (shipClass[shipData[ship].type].weapPos[weaponStats[weapType].exitType][angle].h > 0)
                collision = 1;
        }
        if (intercept != 0)
            shotData[slot].angle = CalcLeadAngle(&shipData[ship], &shipData[target], weapType,
                                                 &shotData[slot].pos);
    }

    /* inaccuracy jitter, first of the two identical blocks (guidance != kGuidanceFreefall) */
    if (0 < weaponStats[shotData[slot].weapType].inaccuracy
            && weaponStats[shotData[slot].weapType].guidance != kGuidanceFreefall) {
        shotData[slot].angle += Rand(weaponStats[shotData[slot].weapType].inaccuracy * 2)
                    - weaponStats[shotData[slot].weapType].inaccuracy;
        if (shotData[slot].angle < 0.0f) {
            shotData[slot].angle += kFullCircleF;
        }
        if (shotData[slot].angle >= kFullCircleF) {
            shotData[slot].angle = shotData[slot].angle - kFullCircleF;
        }
    }

    /* the speed vector */
    guidance = weaponStats[shotData[slot].weapType].guidance;
    if ((guidance != kGuidanceFreefall && guidance != kGuidanceRocket) || 0 < ship) {
        if (0 <= ship && ship < kMaxShips && collision != 0) {
            float f;
            if (collision == 1) {
                f = shipData[ship].heading
                    + abs(weaponStats[shotData[slot].weapType].inaccuracy);
            }
            else {
                f = shipData[ship].heading
                    - abs(weaponStats[shotData[slot].weapType].inaccuracy);
            }
            while (f < 0.0) {
                f += 360.0;
            }
            while (f >= 360.0) {
                f = f - 360.0;
            }
            Accel(f, weaponStats[weapType].speed100,
                  &shotData[slot].vel);
        }
        else {
            Accel(shotData[slot].angle, weaponStats[weapType].speed100,
                  &shotData[slot].vel);
        }
    }

    /* inaccuracy jitter, second of the two identical blocks (guidance == kGuidanceFreefall) */
    if (0 < weaponStats[shotData[slot].weapType].inaccuracy
            && weaponStats[shotData[slot].weapType].guidance == kGuidanceFreefall) {
        shotData[slot].angle += Rand(weaponStats[shotData[slot].weapType].inaccuracy * 2)
                    - weaponStats[shotData[slot].weapType].inaccuracy;
        if (shotData[slot].angle < 0.0f) {
            shotData[slot].angle += kFullCircleF;
        }
        if (shotData[slot].angle >= kFullCircleF) {
            shotData[slot].angle = shotData[slot].angle - kFullCircleF;
        }
    }

    /*
     * The "clear the ship" block.  The win exedis compares weaponStats[shotData
     * [slot].weapType].+0x08 (the header's `ammoType`) against -999, not the
     * header-documented -9999 sentinel on `guidance` (+0x06).  Named for the
     * header field it reads; the -9999/-999 split is a live TBC.
     */
    if (weaponStats[shotData[slot].weapType].ammoType == -999 &&
        0 <= ship && ship < kMaxShips && flag == 0) {
        shipData[ship].shield = 0;
        shipData[ship].armor = 0;
        statusRedraw[kRedrawTarg] = 1;
    }

    /* the four glow rolls */
    for (i = 0; i < 4; i++) {
        if (weaponStats[shotData[slot].weapType].jamVuln[i] > 0)
            shotData[slot].glow[i] = Rand(weaponStats[shotData[slot].weapType].jamVuln[i] + 1);
        else
            shotData[slot].glow[i] = 0;
    }

    /* sprite layer */
    guidance = weaponStats[weapType].guidance;
    if (guidance == kGuidanceHoming) {
        PutSpriteInLayer(shotData[slot].sprite, guidedShotLayer);
    }
    else if (guidance == kGuidancePointDef) {
        PutSpriteInLayer(shotData[slot].sprite, pointDefenseLayer);
    }
    else if (guidance == kGuidanceTurret) {
        if (0 <= ship && ship < kMaxShips) {
            if ((shipClass[shipData[ship].type].flags3 & kShipTurretShotsAbove) != 0)
                PutSpriteInLayer(shotData[slot].sprite, turretedShotLayer);
            else
                PutSpriteInLayer(shotData[slot].sprite, shotLayer);
        } else
            PutSpriteInLayer(shotData[slot].sprite, shotLayer);
    }
    else {
        PutSpriteInLayer(shotData[slot].sprite, shotLayer);
    }

    if (weaponStats[weapType].guidance == kGuidancePointDef) {
        SWSetSpriteCollideProc(shotData[slot].sprite, PointDefenseCollisionHandler);
    }
    else {
        SWSetSpriteCollideProc(shotData[slot].sprite, 0);
    }

    SWCloneFramesFromMaster(shotData[slot].sprite,
                 masterShotSprite[weaponStats[shotData[slot].weapType].graphic]);

    return slot;
}

// FUNCTION: EVNOVA 0x00420d30
// MAC: _SpawnShotSubmunitions i386 0x0003f1d5 ppc 0x00046360
void SpawnShotSubmunitions(ShotObj *shot, short ship)
{
    short subType;
    short n;
    short slot;

    if (weaponStats[shot->weapType].subType == -1
            || weaponStats[shot->weapType].subCount <= 0)
        return;
    if (weaponStats[shot->weapType].subLimit > 0
            && shot->subDepth >= weaponStats[shot->weapType].subLimit)
        return;
    subType = weaponStats[shot->weapType].subType;

    /* the launch sound, played at most once per volley */
    if (weaponStats[subType].sound >= 0 && weaponStats[subType].sound < kWeapTypeTop
            && shotSnd[weaponStats[subType].sound] != 0) {
        if ((weaponStats[subType].flags & kWeapLoopSound) != 0) {
            if (ST_IsSoundPlaying(shotSnd[weaponStats[subType].sound]) == 0)
                PlaySoundDistance(-1, shotSnd[weaponStats[subType].sound], 4,
                                &shot->pos, &shipData[0].pos);
        }
        else {
            PlaySoundDistance(-1, shotSnd[weaponStats[subType].sound], 4,
                            &shot->pos, &shipData[0].pos);
        }
    }

    for (n = 0; n < weaponStats[shot->weapType].subCount; n++) {
        slot = SpawnShot(shot->owner, ship, subType, 1);
        if (slot == -1)
            break;

        shotData[slot].pos.x = shot->pos.x;
        shotData[slot].pos.y = shot->pos.y;
        shotData[slot].vel.x = 0;
        shotData[slot].vel.y = 0;
        shotData[slot].subDepth = shot->subDepth + 1;
        shotData[slot].angle = shot->angle;

        if (weaponStats[shot->weapType].guidance == kGuidancePointDef) {
            shotData[slot].target = shot->target;
        }
        else if ((weaponStats[shot->weapType].flags2 & kWeapSubsSeekNearest) != 0) {
            shotData[slot].target = -1;
            shotData[slot].target = FindNearestValidShotTarget(&shotData[slot]);
            if (shotData[slot].target == -1)
                shotData[slot].target = ship;
            if (shotData[slot].target != -1) {
                shotData[slot].angle = CalcInterceptAngle(
                    &shot->pos,
                    &shipData[shotData[slot].target].pos);
            }
            else {
                shotData[slot].angle = shot->angle;
            }
        }

        /* fan the volley out by subTheta */
        if (weaponStats[shot->weapType].subTheta > 0) {
            shotData[slot].angle += weaponStats[shot->weapType].subTheta
                   - Rand(weaponStats[shot->weapType].subTheta * 2 + 1);
        }
        else if (weaponStats[shot->weapType].subTheta < 0) {
            shotData[slot].angle -= abs(weaponStats[shot->weapType].subTheta)
                          * (weaponStats[shot->weapType].subCount - 1) / 2;
            shotData[slot].angle += abs(weaponStats[shot->weapType].subTheta) * n;
        }
        if (shotData[slot].angle < 0.0f)
            shotData[slot].angle += kFullCircleF;
        if (shotData[slot].angle >= kFullCircleF)
            shotData[slot].angle -= kFullCircleF;

        if (weaponStats[shotData[slot].weapType].guidance == kGuidanceRocket) {
            shotData[slot].vel = shot->vel;
        }
        else {
            Accel(shotData[slot].angle,
                  weaponStats[shotData[slot].weapType].speed100,
                  &shotData[slot].vel);
        }
    }
}

#define kBoomTypes              64
#define kBigBoomBase            1000
#define kBoomTypeTop            1064
#define kBigScatterFraction     0.04
#define kSmallScatterFraction   0.16
#define kScatterSpread          0.5
#define kBigScatterSpread       0.25
#define kBigAngleRange          8
#define kBigAngleBase           4
#define kSmallAngleRange        16
#define kSmallAngleBase         8

// FUNCTION: EVNOVA 0x004211d0
// MAC: _CreateExplosion i386 0x0003f698 ppc 0x000468cc
void CreateExplosion(FloatPoint pt, short type, short size, Boolean sound)
{
    FloatPoint pos;
    short i;
    short sz;
    short bigCount;
    short smallCount;
    short snd;
    short j;
    /* FloatPoint *pos, not sound.h's float *: this body passes &pt */

    if (type < 0 || type >= kBoomTypeTop
            || (type >= kBoomTypes && type < kBigBoomBase))
        return;
    if (type >= kBigBoomBase) {
        type -= kBigBoomBase;
        sz = size;
        bigCount = (size * kBigScatterFraction);
        for (i = 0; i < bigCount; i++) {
            pos.x = pt.x + Rand(size * kScatterSpread) - size * kBigScatterSpread;
            pos.y = pt.y + Rand(size * kScatterSpread) - size * kBigScatterSpread;
            SpawnExplod(pos, -1, 1, Rand(kBigAngleRange) + kBigAngleBase);
        }
        smallCount = (size * kSmallScatterFraction);
        for (j = 0; j < smallCount; j++) {
            pos.x = pt.x + Rand(sz) - size * kScatterSpread;
            pos.y = pt.y + Rand(sz) - size * kScatterSpread;
            SpawnExplod(pos, -1, 0, Rand(kSmallAngleRange) + kSmallAngleBase);
        }
    }
    SpawnExplod(pt, -1, type, 0);
    if (sound != 0) {
        snd = explodType[type].sound;
        if (snd >= 0) {
            if (snd < kMaxShips)
                PlaySoundDistance(-1, explodSnd[snd], 6, &pt, &shipData->pos);
        }
    }
}

#define kMaxActiveBooms 32

// FUNCTION: EVNOVA 0x00421500
// MAC: _SpawnExplod i386 0x0003f5dc ppc 0x000467e8
void SpawnExplod(FloatPoint pos, short unused, short type, short delay)
{
    short i;

    for (i = 0; i < kMaxActiveBooms; i++) {
        if (explodData[i].timer < 0.0f) {
            explodData[i].type = type;
            explodData[i].timer = 0;
            explodData[i].delay = delay;
            explodData[i].pos.x = pos.x;
            explodData[i].pos.y = pos.y;
            SWCloneFramesFromMaster(explodData[i].sprite,
                         masterExplodSprite[explodType[type].graphic]);
            break;
        }
    }
}


// FUNCTION: EVNOVA 0x004215d0
// MAC: _SpawnSmoke i386 0x0003f927 ppc 0x00046ba4
void SpawnSmoke(FloatPoint pos, short type, short sprite)
{
    short i;

    if (settings.noDebris != 0)
        return;
    for (i = 0; i < kMaxDebris; i++) {
        if (smokeData[i].timer < 0.0f) {
            if (masterSmokeSprite[sprite] != 0) {
                smokeData[i].type = type;
                smokeData[i].timer = 0;
                smokeData[i].angle = 0;
                smokeData[i].pos.x = pos.x;
                smokeData[i].pos.y = pos.y;
                SWCloneFramesFromMaster(smokeData[i].sprite, masterSmokeSprite[sprite]);
                SWLockSprite(smokeData[i].sprite);
                break;
            }
        }
    }
}

/* the area asteroids live in (the view plus kWrapMargin around it) over that of the 830x768 reference view;
   50 = an asteroid's size */
static double AsteroidDensity(void)
{
    const double margins = 2 * kWrapMargin + 50;

    return (View::width + margins) * (View::height + margins) / ((830 + margins) * (768 + margins));
}

/* a system's asteroid count at a 1024x768 game size */
short AsteroidReferenceCount(short count)
{
    double roids;

    if (count <= 0)
        return 0;
    roids = count * 1024 / 640.0;
    return roids > kClassicMaxAsteroids ? kClassicMaxAsteroids : (short)roids;
}

/* the asteroids the current system keeps around the player; without Asteroid Scaling by game width, at most 16 */
short AsteroidTarget(void)
{
    short count = systemData[shipData[0].curSystem].asteroidCount;
    int target;

    if (count <= 0)
        return 0;
    if (settings.asteroidScaling == 0) {
        target = (int)(count * (View::gameWidth / 640.0));
        return target > kClassicMaxAsteroids ? kClassicMaxAsteroids : target;
    }
    target = (int)(AsteroidReferenceCount(count) * AsteroidDensity() + 0.5);
    if (target < 1)
        target = 1;
    if (target > kMaxAsteroids)
        target = kMaxAsteroids;
    return target;
}

// FUNCTION: EVNOVA 0x004216b0
// MAC: _CreateAsteroids i386 0x0003fa05 ppc 0x00046ca8
void CreateAsteroids(void)
{
    short i;

    for (i = 0; i < kMaxAsteroids; i++)
        asteroidData[i].active = 0;
    killAsteroids = 0;
    SpawnAsteroid(0);
}

/* slot i becomes a fresh asteroid of one of the current system's types: type, frame, spin, strength;
   false when the system has no asteroid types */
bool RollAsteroid(short i)
{
    short types = systemData[shipData[0].curSystem].astTypes;

    if (types == 0)
        return false;
    do {
        asteroidData[i].type = Rand(kMaxRoidTypes);
    } while (!(types & (1 << asteroidData[i].type)));
    asteroidData[i].frame = Rand(masterAsteroidSprite[asteroidData[i].type]->numFrames);
    asteroidData[i].frameRate = asteroidType[asteroidData[i].type].frameRate * (Rand(41) + 80) * 0.01;
    if (Rand(2) == 0)
        asteroidData[i].frameRate *= -1.0;
    asteroidData[i].strength = asteroidType[asteroidData[i].type].strength;
    return true;
}

/* type 0: the whole field at once; else a few new asteroids just off screen */
// FUNCTION: EVNOVA 0x00421830
// MAC: _SpawnAsteroid i386 0x0003a233 ppc 0x000406a0
void SpawnAsteroid(unsigned char type)
{
    short i;
    short room;
    short rate;
    short sw;
    short sh;
    short hw;
    short hh;
    short dx;
    short dy;
    SpriteRec *master;

    if (systemData[shipData[0].curSystem].asteroidCount <= 0
        || systemData[shipData[0].curSystem].astTypes == 0)
        return;

    room = AsteroidTarget();
    for (i = 0; i < kMaxAsteroids; i++)
        if (asteroidData[i].active != 0)
            room--;
    if (type != 0) {
        rate = settings.asteroidScaling != 0 ? (short)(AsteroidDensity() + 0.5) : 1;
        if (rate < 1)
            rate = 1;
        if (room > rate)
            room = rate;
    }

    for (i = 0; i < kMaxAsteroids && room > 0; i++) {
        if (asteroidData[i].active != 0)
            continue;
        asteroidData[i].active = 1;
        room--;

        RollAsteroid(i);
        master = masterAsteroidSprite[asteroidData[i].type];
        /* half the area this asteroid lives in: the view plus kWrapMargin, by its sprite's middle */
        hw = screenCenter.h + GetSpriteXSize(master) / 2 + kWrapMargin - 2;
        hh = screenCenter.v + GetSpriteYSize(master) / 2 + kWrapMargin - 2;

        if (type != 0) {
            /* fully off screen anywhere along an edge (each edge by its length) */
            sw = GetSpriteXSize(master);
            sh = GetSpriteYSize(master);
            if (Rand(screenCenter.h + screenCenter.v) < screenCenter.h) {
                dx = Rand(2 * hw) - hw;
                dy = screenCenter.v + sh / 2 + 2 + Rand(kWrapMargin - 1);
                if (Rand(2) == 0)
                    dy = -dy;
            } else {
                dy = Rand(2 * hh) - hh;
                dx = screenCenter.h + sw / 2 + 2 + Rand(kWrapMargin - 1);
                if (Rand(2) == 0)
                    dx = -dx;
            }
            asteroidData[i].pos.x = shipData[0].pos.x + dx;
            asteroidData[i].pos.y = shipData[0].pos.y + dy;
            asteroidData[i].vel.x = Rand(200) * (dx > 0 ? -0.01 : 0.01);
            asteroidData[i].vel.y = Rand(200) * (dy > 0 ? -0.01 : 0.01);
        } else {
            /* anywhere in the area */
            asteroidData[i].pos.x = shipData[0].pos.x + Rand(2 * hw) - hw;
            asteroidData[i].pos.y = shipData[0].pos.y + Rand(2 * hh) - hh;
            asteroidData[i].vel.x = (Rand(400) - 200) * 0.01;
            asteroidData[i].vel.y = (Rand(400) - 200) * 0.01;
        }
    }
}

// FUNCTION: EVNOVA 0x00421e60
// MAC: _SpawnSubAsteroid i386 0x0003fbc7 ppc 0x00046ef4
void SpawnSubAsteroid(FloatPoint *pos, short type)
{
    short i;

    for (i = 0; i < kMaxAsteroids; i++) {
        if (asteroidData[i].active == 0) {
            asteroidData[i].active = 1;
            asteroidData[i].type = type;
            asteroidData[i].pos.x = pos->x;
            asteroidData[i].pos.y = pos->y;
            asteroidData[i].vel.x = (Rand(200) - 100) * 0.01;
            asteroidData[i].vel.y = (Rand(200) - 100) * 0.01;
            asteroidData[i].frame = Rand(masterAsteroidSprite[type]->numFrames);
            asteroidData[i].frameRate = asteroidType[type].frameRate * (Rand(41) + 80) * 0.01;
            if (Rand(2) == 0)
                asteroidData[i].frameRate *= -1.0;
            asteroidData[i].strength = asteroidType[type].strength;
            break;
        }
    }
}

// FUNCTION: EVNOVA 0x00421fd0
// MAC: _SpawnStellarDefenseShip i386 0x0003d2c9 ppc 0x00043e50
short SpawnStellarDefenseShip(short stellar)
{
    short ship;
    ship = -1;
    if (stellarData[stellar].defenseDude != -1) {
        ship = SpecificDudeSpawn(stellarData[stellar].defenseDude,
                                 stellarData[stellar].system, 2, 0);
        if (ship == -1)
            ship = SpecificDudeSpawn(stellarData[stellar].defenseDude,
                                     stellarData[stellar].system, 2, 1);
        if (ship != -1) {
            shipData[ship].defenseStellar = stellar;
            shipData[ship].boarded = 1;
            shipData[ship].cloakDir = 0;
            shipData[ship].cloak = 0;
            shipData[ship].aiTimer = 0;
            shipData[ship].aiType = kAIWarship;
            shipData[ship].specialMission = -1;
            shipData[ship].parentShip = -1;
            shipData[ship].govt = stellarData[stellar].govt;
            shipData[ship].pos.x = stellarData[stellar].pos.h;
            shipData[ship].pos.y = stellarData[stellar].pos.v;
            shipData[ship].heading = Rand(kFullCircle);
            shipData[ship].vel.x = shipData[ship].vel.y = 0;
            shipData[ship].accelMag = 0;
            Accel(shipData[ship].heading, shipData[ship].MaxSpeed(),
                  &shipData[ship].vel);
            AIMakeShipAttackPlayer(&shipData[ship]);
            stellarData[stellar].defenseActive = 1;
        }
    }
    return ship;
}

// FUNCTION: EVNOVA 0x004221d0
// MAC: _SufficentTargetedDamage i386 0x0003fd22 ppc 0x000470f8
Boolean SufficentTargetedDamage(ShipObj *ship)
{
    if (ship->incomingDamage >= (ship->shield + ship->armor) * 1.05)
        return 1;
    return 0;
}

// FUNCTION: EVNOVA 0x00422210
// MAC: _CalculateTargetedDamage i386 0x0003a153 ppc 0x00040554
void CalculateTargetedDamage(void)
{
    short i;
    short j;

    for (i = 0; i < kMaxShips; i++) {
        if (shipData[i].active) {
            shipData[i].incomingDamage = 0;
            for (j = 0; j < kMaxShots; j++) {
                if (shotData[j].life > 0.0f && shotData[j].target == i
                    && shotData[j].guidanceState == 0)
                    shipData[i].incomingDamage +=
                        (weaponStats[shotData[j].weapType].damage[0]
                         + weaponStats[shotData[j].weapType].damage[1]) * 0.5;
            }
        }
    }
}

#define kNudgeBase        0x32
#define kNudgeSpread      0x32

// FUNCTION: EVNOVA 0x00422340
// MAC: _MakeEscortsHyperWithParent i386 0x0003fd54 ppc 0x0004714c
void MakeEscortsHyperWithParent(ShipObj *ship)
{
    short i;

    for (i = 1; i < kMaxShips; i++) {
        if (shipData[i].active != 0 && shipData[i].parentShip == ship->index &&
            shipData[i].defenseStellar == -1) {
            shipData[i].jumpTimer = shipData[shipData[i].parentShip].jumpTimer;
            shipData[i].jumpTick = shipData[shipData[i].parentShip].jumpTick;
            shipData[i].targetShip = -2;
            AIMakeShipJumpWithParent(&shipData[i]);
        }
    }
}

// FUNCTION: EVNOVA 0x00422400
// MAC: _SpawnEscort i386 0x0003fe5b ppc 0x000472bc
short SpawnEscort(short shipType, short fromSpob)
{
    short slot;
    short k;
    short j;

    slot = GenericRandomShipSpawn(shipData->curSystem, kSpawnReserve);
    if (slot != -1 && shipType > -1) {

        shipData[slot].type = shipType;
        shipData[slot].aiType = kAIEscort;
        shipData[slot].parentShip = 0;
        shipData[slot].govt = -1;
        shipData[slot].aggression = 2;
        shipData[slot].shield = shipClass[shipData[slot].type].shield;
        shipData[slot].armor = shipClass[shipData[slot].type].armor;
        shipData[slot].dude = -1;
        shipData[slot].hailed = 0;
        shipData[slot].hasAfterburner = AIHasAfterburner(&shipData[slot]);
        shipData[slot].canScoop = shipData[slot].CanScoop();
        shipData[slot].hired = 1;
        shipData[slot].specialMission = -1;
        shipData[slot].auxMission = -1;
        shipData[slot].upgradeOnLanding = 0;
        shipData[slot].sellOnLanding = 0;
        shipData[slot].fuelRegenState = -1;
        shipData[slot].escortOrder = -1;
        for (j = 0; j < 4; j++)
            shipData[slot].jamming[j] = -1;

        if (fromSpob != -1) {
            shipData[slot].pos.x = stellarData[fromSpob].pos.h;
            shipData[slot].pos.y = stellarData[fromSpob].pos.v;
        } else {
            shipData[slot].pos.x = shipData->pos.x;
            shipData[slot].pos.y = shipData->pos.y;
            shipData[slot].heading = shipData->heading;
            Accel(Rand(kFullCircle),
                  (Rand(kNudgeSpread) + kNudgeBase),
                  &shipData[slot].pos);
        }

        for (k = 0; k < kMaxWeapTypes; k++) {
            shipData[slot].weap[k].count = shipClass[shipType].weapCount[k];
            shipData[slot].weap[k].ammo = shipClass[shipType].weapAmmo[k];
        }

        AISetup(&shipData[slot]);
        AIMakeEscortFlyInForm(&shipData[slot]);
        return slot;
    }
    return -1;
}

#define kStrEscortOne      298
#define kStrEscortMany     299
#define kStrSoldFor        300
#define kStrUpgradedFor    301

// FUNCTION: EVNOVA 0x004229d0
// MAC: _DoEscortLand i386 0x0004031f ppc 0x00047904
void DoEscortLand(void)
{
    std::string msg;
    short days;
    int32_t total;
    short i;
    short numSold;
    short numUpgraded;
    int32_t saleValue;
    int32_t upgradeCost;
    short type;
    short j;

    numUpgraded = 0;
    numSold = 0;
    saleValue = 0;
    upgradeCost = 0;

    if (SpobHasShipyard(&stellarData[shipData[0].targetStellar])) {
        for (i = 1; i < kMaxShips; i++) {
            if (shipData[i].active
                && shipData[i].parentShip == 0
                && !shipData[i].IsDisabled()
                && shipData[i].aiType == kAIEscort
                && shipData[i].sellOnLanding
                && shipData[i].specialMission == -1) {
                shipData[0].credits += shipClass[shipData[i].type].tradeIn;
                saleValue += shipClass[shipData[i].type].tradeIn;
                shipData[i].active = 0;
                shipData[i].parentShip = -1;
                statusRedraw[kRedrawCargo] = 1;
                numSold++;
            }
        }

        for (i = 1; i < kMaxShips; i++) {
            if (shipData[i].active
                && shipData[i].parentShip == 0
                && shipData[i].aiType == kAIEscort
                && shipData[i].upgradeOnLanding) {
                if (shipClass[shipData[i].type].upgradeTo != -1
                    && shipData[i].specialMission == -1
                    && shipData[0].credits >= shipClass[shipData[i].type].escUpgradeCost) {
                    numUpgraded++;
                    upgradeCost += shipClass[shipData[i].type].escUpgradeCost;
                    shipData[0].credits -= shipClass[shipData[i].type].escUpgradeCost;
                    statusRedraw[kRedrawCargo] = 1;
                    shipData[i].type = shipClass[shipData[i].type].upgradeTo;
                    for (j = 0; j < kMaxWeapTypes; j++) {
                        shipData[i].weap[j].count = 0;
                        shipData[i].weap[j].ammo = 0;
                    }
                    shipData[i].upgradeOnLanding = 0;
                }
            }
            shipData[i].shield = shipClass[shipData[i].type].shield;
            shipData[i].armor = shipClass[shipData[i].type].armor;
            for (j = 0; j < kMaxWeapTypes; j++)
                shipData[i].weap[j].ammo = shipClass[shipData[i].type].weapAmmo[j];
            type = shipData[i].specialMission;
            if (type != -1 && currentMissionObjectives[type].active != 0
                && currentMissions[type].shipBehav == kMissionModeEscort)
                j = 0;
        }

        if (numSold > 0 || numUpgraded > 0) {
            msg.clear();
            if (numSold > 0) {
                msg += TextF("%s %s %s %s %s.", OrdinalText(numSold, true).c_str(),
                             IndString(kGameStrList, numSold == 1 ? kStrEscortOne : kStrEscortMany).c_str(),
                             IndString(kGameStrList, kStrSoldFor).c_str(), CommaNumber(saleValue).c_str(),
                             FromField(saleValue > 1 ? creditsCString : creditCString, sizeof creditCString).c_str());
                if (numUpgraded > 0)
                    msg += "\r\r";
            }
            if (numUpgraded > 0) {
                msg += TextF("%s %s %s %s %s.", OrdinalText(numUpgraded, true).c_str(),
                             IndString(kGameStrList, numUpgraded == 1 ? kStrEscortOne : kStrEscortMany).c_str(),
                             IndString(kGameStrList, kStrUpgradedFor).c_str(), CommaNumber(upgradeCost).c_str(),
                             FromField(upgradeCost > 1 ? creditsCString : creditCString, sizeof creditCString).c_str());
            }
            SetShowText(msg);
            ShowTextDialog(0, 0);
            total = numSold + numUpgraded;
            days = total / 2;
            if (days > 0) {
                crashlog_print_mdy(&gameTime);
                crashlog_print(": incrementing date ");
                crashlog_print_num(days);
                crashlog_print(" days for upgrade/sale of ");
                crashlog_print_num(total);
                crashlog_print(" escorts\n");
                for (i = 0; i < days; i++)
                    IncrementGameTime();
            }
        }
    }
    DoEscortPayment(1);
}

// FUNCTION: EVNOVA 0x004232d0
// MAC: _DoEscortPayment i386 0x00040111 ppc 0x00047658
void DoEscortPayment(short days)
{
    short i;
    short day;
    short checked;
    short numDismissed;
    int32_t cost;
    Boolean skip;

    checked = 0;
    numDismissed = 0;
    for (day = 0; day < days; day++) {
        for (i = 1; i < kMaxShips; i++) {
            if (shipData[i].active == 0) continue;
            if (shipData[i].parentShip != 0) continue;
            if (shipData[i].aiType != kAIEscort) continue;
            if (shipData[i].IsDisabled()) continue;

            skip = 0;
            if (shipData[i].specialMission != -1) {
                if (currentMissionObjectives[shipData[i].specialMission].active != 0) {
                    if (currentMissions[shipData[i].specialMission].shipBehav == kMissionModeEscort) {
                        skip = 1;
                    }
                }
            }
            if (skip == 0 && shipData[i].hired) {
            cost = (0.01 * shipClass[shipData[i].type].cost);

            if (shipData[0].credits >= cost) {
                shipData[0].credits -= cost;
                statusRedraw[kRedrawCargo] = 1;
            } else {
                numDismissed++;
                if (shipData[i].aiType == kAIEscort && shipData[i].specialMission == -1 && AIIsTrader(shipClass[shipData[i].type].inherentAI)) {
                    DestroyPartialFleetCargo(i);
                }
                statusRedraw[kRedrawCargo] = 1;
                shipData[i].active = 0;
                shipData[i].parentShip = -1;
                shipData[i].aiType = kAIWimpyTrader;
            }
            }
            checked++;
        }
    }

    if (numDismissed > 0) {
        if (hyperGamma >= 0.0) {
            hyperGamma = -1.0f;
            FadeGammaTo(100);
        }
        SetShowText(IndString(kGameStrList, numDismissed == 1 ? 302 : 303));
        statusRedraw[kRedrawCargo] = 1;
        ShowTextDialog(0, 0);
        DrawStatus();
    }
}

// FUNCTION: EVNOVA 0x00423540
// MAC: _DoStellarTimePassage i386 0x00040867 ppc 0x00047ee8
void DoStellarTimePassage(void)
{
    short i;

    for (i = 0; i < kMaxStellars; i++) {
        if (stellarData[i].loaded != 0 && stellarData[i].dominated != 0) {
            if ((stellarData[shipData[0].targetStellar].flags2 & kSpobAlwaysDominated) == 0)
                stellarData[i].daysDominated += 1;
            shipData[0].credits += stellarData[i].tribute;
            statusRedraw[kRedrawCargo] = 1;
        }
    }
}

// FUNCTION: EVNOVA 0x004235c0
// MAC: _SpawnPerson i386 0x000408d5 ppc 0x00047f84
short SpawnPerson(short sys, unsigned char noDisabled, short person)
{
    short i;
    short slot;
    short pick;
    short n;
    short j;
    Boolean eligible[kMaxPersons];

    n = 0;
    eligible[kMaxPersons - 1] = 0;
    if (person >= 0 && person < 0x3ff) {
        for (i = 0; i < 0x3ff; i++)
            eligible[i] = 0;
        eligible[person] = 1;
        n = 1;
    } else {
        for (i = 0; i < 0x3ff; i++) {
            eligible[i] = 0;
            if (personData[i].loaded != 0 && AIIsSet(personData[i].aiType)
                && personData[i].canAppear != 0 && personData[i].prepared != 0) {
                if (personData[i].linkSyst == -1)
                    eligible[i] = 1;
                if (personData[i].linkSyst == sys)
                    eligible[i] = 1;
                if (personData[i].linkSyst >= kFirstResourceID && personData[i].linkSyst < 0x2710
                    && personData[i].linkSyst - kFirstResourceID == sys)
                    eligible[i] = 1;
                if (personData[i].linkSyst >= 0x270f && personData[i].linkSyst < 0x3a98
                    && personData[i].linkSyst - 0x2710 == systemData[sys].govt)
                    eligible[i] = 1;
                if (personData[i].linkSyst >= 0x3a98 && personData[i].linkSyst < 0x4e20
                    && systemData[sys].govt >= 0
                    && GovtAllies(personData[i].linkSyst - 0x3a98, systemData[sys].govt) != 0)
                    eligible[i] = 1;
                if (personData[i].linkSyst >= 0x4e20 && personData[i].linkSyst < 0x61a8
                    && systemData[sys].govt >= 0
                    && personData[i].linkSyst - 0x4e20 != systemData[sys].govt)
                    eligible[i] = 1;
                if (personData[i].linkSyst >= 0x61a8 && personData[i].linkSyst < 0x7530
                    && systemData[sys].govt >= 0
                    && GovtEnemies(personData[i].linkSyst - 0x61a8, systemData[sys].govt) != 0)
                    eligible[i] = 1;
                if (personData[i].govt >= 0
                    && (govtData[personData[i].govt].flags & kGovtAlwaysDisabled) != 0
                    && noDisabled != 0)
                    eligible[i] = 0;
                if (eligible[i] != 0) {
                    if (EvalMissionBitTestString(personData[i].activeOn) == 0)
                        eligible[i] = 0;
                }
            }
            if (eligible[i] != 0)
                n++;
        }
    }
    for (i = 1; i < kMaxShips; i++) {
        if (shipData[i].active != 0 && shipData[i].specialMission == -1
            && (shipData[i].persType >= 0 && shipData[i].persType < kMaxPersons)) {
            for (j = 0; j < kMaxPersons; j++) {
                if (eligible[j] != 0
                    && personData[j].name[0] ==
                       personData[shipData[i].persType].name[0]
                    && personData[j].sameNameIndex ==
                       personData[shipData[i].persType].sameNameIndex) {
                    eligible[j] = 0;
                    n--;
                }
            }
        }
    }
    if (n > 0) {
        if (person != -1) {
            pick = person;
            eligible[pick] = 1;
            personData[pick].loaded = 1;
        } else {
            pick = Rand(0x3fe); /* BUG: Rand never returns 0x3fe, so person 1150 never spawns at random */
        }
        if (eligible[pick] != 0) {
            slot = GenericRandomShipSpawn(sys, kSpawnReserve);
            if (slot != -1) {
                shipData[slot].persType = pick;
                shipData[slot].type = personData[pick].shipType;
                shipData[slot].dude = -1;
                shipData[slot].govt = personData[pick].govt;
                shipData[slot].aiType = personData[pick].aiType;
                shipData[slot].aggression = personData[pick].aggression;
                shipData[slot].hailed = 0;
                shipData[slot].hasAfterburner = AIHasAfterburner(&shipData[slot]);
                shipData[slot].canScoop = shipData[slot].CanScoop();
                shipData[slot].escortOrder = -1;
                if ((personData[pick].flags & kPersEscapePod) != 0)
                    shipData[slot].hasAfterburner = 1;
                if (shipData[slot].aggression < 1)
                    shipData[slot].aggression = 1;
                if (shipData[slot].aggression >= 3)
                    shipData[slot].aggression = 4;
                for (j = 0; j < kMaxWeapTypes; j++) {
                    shipData[slot].weap[j].count =
                        shipClass[shipData[slot].type].weapCount[j];
                    shipData[slot].weap[j].ammo =
                        shipClass[shipData[slot].type].weapAmmo[j];
                    shipData[slot].weap[j].count += personData[pick].weapCount[j];
                    shipData[slot].weap[j].ammo += personData[pick].weapAmmo[j];
                }
                shipData[slot].shield = shipClass[shipData[slot].type].shield;
                shipData[slot].armor = shipClass[shipData[slot].type].armor;
                if ((personData[pick].flags2 & kPersStartsNoFuel) != 0)
                    shipData[slot].fuel = 0;
                else
                    shipData[slot].fuel = shipClass[shipData[slot].type].fuel;
                if (personData[pick].shieldMod > 0) {
                    shipData[slot].shield =
                        shipData[slot].shield * personData[pick].shieldMod;
                    shipData[slot].armor =
                        shipData[slot].armor * personData[pick].shieldMod;
                }
                if (shipData[slot].govt != -1
                    && (govtData[shipData[slot].govt].flags & kGovtAlwaysDisabled) != 0) {
                    shipData[slot].shield = 0;
                    if ((shipClass[shipData[slot].type].flags & kShipArmorDisableTenth) != 0)
                        shipData[slot].armor =
                            shipClass[shipData[slot].type].armor * 0.1 - 1.0;
                    else
                        shipData[slot].armor =
                            shipClass[shipData[slot].type].armor * 0.33 - 1.0;
                    shipData[slot].vel.x = shipData[slot].vel.y = 0;
                    shipData[slot].accelMag = 0;
                }
                AISetup(&shipData[slot]);
                if (personData[shipData[slot].persType].linkMission != -1)
                    RandomizeOneMission(personData[shipData[slot].persType].linkMission);
                if (personData[pick].govt >= 0
                    && (govtData[personData[pick].govt].flags & kGovtAlwaysDisabled) != 0) {
                    shipData[slot].heading = Rand(kFullCircle);
                    /* BUG: vel.x is cleared twice and vel.y keeps whatever the slot held */
                    shipData[slot].vel.x = 0;
                    shipData[slot].vel.x = 0;
                    shipData[slot].accelMag = 0;
                    shipData[slot].engineGlow = 0;
                    if ((shipClass[shipData[slot].type].shanFlags & kShanFoldingParts) != 0)
                        shipData[slot].foldSet =
                            shipClass[shipData[slot].type].baseSets - 1;
                }
                return slot;
            }
        }
    }
    return -1;
}

/* win reads the index in three widths, each a CSE class; the mac's slotL and ((int)slotL) are the short itself */
#define slotR ((int32_t)slot)

// FUNCTION: EVNOVA 0x00423fa0
// MAC: _DoShipCapture i386 0x00041120 ppc 0x0004889c
void DoShipCapture(ShipObj *me, Boolean asEscort)
{
    unsigned char buf[256];
    int32_t slotL;
    int32_t capType;
    short m;
    short w;
    short i;
    short slot;
    short val;

    slot = GenericRandomShipSpawn(shipData[0].curSystem, 1);
    if (slot != -1) {
        if (me->persType != -1)
            personData[me->persType].loaded = 0;
        slotL = slot;
        shipData[slotL].type = shipData[0].type;
        shipData[slotL].dude = -1;
        shipData[slotL].persType = -1;
        shipData[slotL].pos = shipData[0].pos;
        shipData[slotL].vel = shipData[0].vel;
        shipData[slotL].heading = shipData[0].heading;
        shipData[slotL].desiredHeading = shipData[slotL].heading;
        shipData[slotL].targetStellar = -1;
        shipData[slotL].targetShip = -1;
        shipData[slotL].selectedWeapon = -1;
        shipData[slotL].curSystem = shipData[0].curSystem;
        shipData[slotL].govt = -1;
        shipData[slotL].hired = 0;
        shipData[slotL].hailed = 0;
        shipData[slotL].hasAfterburner = AIHasAfterburner(&shipData[slotL]);
        shipData[slotL].canScoop = shipData[slotL].CanScoop();
        shipData[slotL].auxMission = -1;
        shipData[slotL].escortOrder = -1;
        shipData[slotL].index = slot;
        shipData[slotL].voiceClass = Rand(2);
        val = shipClass[shipData[slotL].type].inherentGovtB;
        if (val != -1) {
            val = govtData[val].voiceClass;
            if (val != -1)
                shipData[((int)slotL)].voiceClass = val;
        }
        for (i = 0; i <= kLastCommodity; i++)
            shipData[slotL].cargo[i] = shipData[0].cargo[i];
        for (i = 1; i < kMaxShips; i++) {
            if (shipData[i].active != 0 && i != me->index && i != slot
                    && me->index == shipData[i].parentShip) {
                shipData[i].targetShip = shipData[i].targetStellar = -1;
                shipData[i].grudge = 0;
                shipData[i].parentShip = -1;
                shipData[i].aiType = shipClass[shipData[i].type].inherentAI;
            }
        }
        shipData[slotL].specialMission = -1;
        if (asEscort != 0) {
            shipData[slotR].shield = 0;
            shipData[slotR].armor = 1.0f;
            shipData[slotR].boarded = 1;
            shipData[slotR].formerEscort = -1;
            shipData[slotR].hired = 0;
            shipData[slotR].hailed = 0;
            shipData[slotR].parentShip = 0;
            shipData[slotR].aiType = kAIEscort;
            DestroyPartialFleetCargo(slot);
            shipData[slotR].parentShip = -1;
            shipData[slotR].aiType = kAINone;
        } else {
            shipData[slotR].deathTimer = 0;
            shipData[slotR].parentShip = 0;
            shipData[slotR].aiType = kAIEscort;
            shipData[slotR].defenseStellar = -1;
            shipData[slotR].grudge = 0;
            shipData[slotR].fuel = shipData[slotR].FuelCapacity();
            if (shipData[slotR].IsDisabled() != 0) {
                if ((shipClass[shipData[slotL].type].flags & kShipArmorDisableTenth) != 0)
                    shipData[slotR].armor =
                        shipData[slotR].ArmorCapacity() * 0.1 + 1.0;
                else
                    shipData[slotR].armor =
                        shipData[slotR].ArmorCapacity() * 0.3333 + 1.0;
            }
            prevParent[slotL] = 0;
            AISetup(&shipData[((int)slotL)]);
            AIMakeEscortFlyInForm(&shipData[((int)slotL)]);
        }
        statusRedraw[kRedrawCargo] = 1;
        statusRedraw[kRedrawTarg] = 1;
        statusRedraw[kRedrawWeap] = 1;
        for (i = 0; i < kMaxWeapTypes; i++) {
            shipData[slotL].weap[i].count =
                shipClass[shipData[slotL].type].weapCount[i];
            shipData[slotL].weap[i].ammo =
                shipClass[shipData[slotL].type].weapAmmo[i];
        }
        {
            short t = shipData[0].type;
            ShipRec *rec = &shipClass[t];

            crashlog_mission_bit_set("retiring", (char *)rec->name, 1,
                                     t + kFirstResourceID, rec->onRetire);
        }
        EvalMissionBitSetString(shipClass[shipData[0].type].onRetire);
        {
            short t = me->type;
            ShipRec *rec = &shipClass[t];

            crashlog_mission_bit_set("capturing", (char *)rec->name, 1,
                                     t + kFirstResourceID, rec->onCapture);
        }
        EvalMissionBitSetString(shipClass[me->type].onCapture);
        shipData[0].type = me->type;
        shipData[0].pos = me->pos;
        shipData[0].vel = me->vel;
        shipData[0].heading = me->heading;
        shipData[0].desiredHeading = shipData[0].heading;
        shipData[0].targetStellar = -1;
        shipData[0].targetShip = -1;
        shipData[0].selectedWeapon = -1;
        GetShipPaintColor(me, &playerPaint.red, &playerPaint.green,
                     &playerPaint.blue);
        for (i = 0; i <= kLastCommodity; i++)
            shipData[0].cargo[i] = me->cargo[i];
        if (shipData->FuelCapacity() > 0)
            shipData[0].fuel = Rand(shipData->FuelCapacity());
        else
            shipData[0].fuel = 0;
        shipData[0].aiType = kAINone;
        shipData[0].parentShip = -1;
        shipData[0].govt = -1;
        shipData[0].shield = 0;
        if ((shipClass[shipData[0].type].flags & kShipArmorDisableTenth) != 0)
            shipData[0].armor = shipClass[shipData[0].type].armor * 0.1 + 2.0;
        else
            shipData[0].armor = shipClass[shipData[0].type].armor * 0.3333 + 2.0;
        for (w = 0; w < kMaxWeapTypes; w++) {
            Boolean keep;

            keep = 0;
            for (i = 0; i < kMaxOutfits; i++) {
                for (m = 0; m < kOutfMods; m++) {
                    if (outfitSystem[i].modType[m] == OutfMod::Weapon && w == outfitSystem[i].modVal[m]
                            && outfitSystem[i].persistent != 0) {
                        keep = 1;
                        break;
                    }
                }
            }
            if (keep == 0) {
                shipData[0].weap[w].count = 0;
                shipData[0].weap[w].ammo = 0;
            }
        }
        for (w = 0; w < kMaxWeapTypes; w++) {
            if (me->weap[w].count > 0 && shipData[0].weap[w].count <= 0)
                shipData[0].weap[w].count = me->weap[w].count;
            if (me->weap[w].ammo > 0 && shipData[0].weap[w].ammo <= 0) {
                if (weaponStats[w].guidance == kGuidanceBay) {
                    shipData[0].weap[w].ammo = me->weap[w].ammo;
                } else if (weaponStats[w].ammoType >= 0 && weaponStats[w].ammoType < kMaxWeapTypes) {
                    shipData[0].weap[weaponStats[w].ammoType].ammo = me->weap[w].ammo;
                }
            }
        }
        for (w = 0; w < kMaxOutfits; w++) {
            if (outfitSystem[w].persistent == 0)
                shipSystems[w] = 0;
        }
        ShipStatsToSystemInfo();
        for (i = 0; i < kDefaultOutfitSlots; i++) {
            if (shipClass[shipData[0].type].defOutfCount[i] > 0)
                shipSystems[shipClass[shipData[0].type].defOutfID[i]] +=
                    shipClass[shipData[0].type].defOutfCount[i];
        }
        SystemInfoToShipStats();
        me->active = 0;
        GetIndString(buf, kGameStrList, 0x130);
        DisplayComm((PStrChar *)buf, 360);

    } else {
        GetIndString(buf, kGameStrList, 0x131);
        DisplayComm((PStrChar *)buf, 360);
    }
}
#undef slotL
#undef slotR


// FUNCTION: EVNOVA 0x00424f90
// MAC: _DisasterHandler i386 0x00041ab9 ppc 0x00049324
void DisasterHandler(void)
{
    short i;
    short target;

    for (i = 0; i < kMaxDisasters; i++) {
        if (disasterData[i].loaded) {
            if (disasterData[i].countdown > 0) {
                disasterData[i].countdown -= 1;
                continue;
            }
            if (disasterData[i].unk20D) {
                if (disasterData[i].duration >= 0)
                    continue;
                disasterData[i].countdown = 2;
                continue;
            }
            if (Rand(100) + 1 > disasterData[i].freq)
                continue;
            if (!EvalMissionBitTestString(disasterData[i].text))
                continue;

            target = disasterData[i].stellar;
            if (target >= kFirstResourceID) {
                if (disasterData[i].unk20D) {
                    disasterData[i].stellarID = disasterData[i].stellar - kFirstResourceID;
                    disasterData[i].countdown = disasterData[i].duration;
                } else {
                    disasterData[i].stellarID = disasterData[i].stellar - kFirstResourceID;
                    disasterData[i].countdown = disasterData[i].duration;
                }
            } else if (target == -1) {
                do {
                    target = Rand(kMaxStellars);
                } while (!stellarData[target].loaded ||
                         (stellarData[target].flags & kSpobUninhabited) != 0);
                if (disasterData[i].unk20D) {
                    disasterData[i].stellarID = target;
                    disasterData[i].countdown = disasterData[i].duration;
                } else {
                    disasterData[i].stellarID = target;
                    disasterData[i].countdown = disasterData[i].duration;
                }
            }
        } else {
            disasterData[i].stellarID = -1;
            disasterData[i].countdown = -1;
            disasterData[i].unk20D = 0;
        }
    }
}

// FUNCTION: EVNOVA 0x004250f0
// MAC: _DoAutoRefuel i386 0x00041bc5 ppc 0x0004949c
void DoAutoRefuel(void)
{
    Boolean found;
    short amount;

    found = PlayerHasMod(OutfMod::AutoRefuel);

    amount = shipData->FuelCapacity();
    if (found) {
        if ((short)shipData->fuel < amount) {
            shipData->fuel = (short)shipData->fuel;
            amount -= shipData->fuel;
            if (amount > shipData->credits)
                amount = shipData->credits;
            shipData->credits -= amount;
            shipData->fuel += amount;
            statusRedraw[kRedrawFuel] = 1;
            statusRedraw[kRedrawCargo] = 1;
        }
    }
}

#define kFletCount 256


// FUNCTION: EVNOVA 0x00425280
// MAC: _SpawnFleet i386 0x00042704 ppc 0x0004a164
void SpawnFleet(short sys, unsigned char arriving)
{
    short i;
    short count;
    short pick;
    Boolean ok[kFletCount];

    count = 0;
    for (i = 0; i < kFletCount; i++) {
        ok[i] = 0;
        if (fleetData[i].leadShipType != -1 && fleetData[i].usable != 0) {
            if (fleetData[i].linkSyst == -1)
                ok[i] = 1;
            if (sys == fleetData[i].linkSyst)
                ok[i] = 1;
            if (fleetData[i].linkSyst >= kFirstResourceID && fleetData[i].linkSyst < 10000 &&
                fleetData[i].linkSyst - kFirstResourceID == sys)
                ok[i] = 1;
            if (fleetData[i].linkSyst >= 10000 && fleetData[i].linkSyst < 15000 &&
                fleetData[i].linkSyst - 10000 == systemData[sys].govt)
                ok[i] = 1;
            if (fleetData[i].linkSyst >= 15000 && fleetData[i].linkSyst < 20000 &&
                systemData[sys].govt >= 0 &&
                GovtAllies(fleetData[i].linkSyst - 15000, systemData[sys].govt))
                ok[i] = 1;
            if (fleetData[i].linkSyst >= 20000 && fleetData[i].linkSyst < 25000 &&
                systemData[sys].govt >= 0 &&
                fleetData[i].linkSyst - 20000 != systemData[sys].govt)
                ok[i] = 1;
            if (fleetData[i].linkSyst >= 25000 && fleetData[i].linkSyst < 30000 &&
                systemData[sys].govt >= 0 &&
                GovtEnemies(fleetData[i].linkSyst - 25000, systemData[sys].govt))
                ok[i] = 1;
            if (fleetData[i].usable == 0)
                ok[i] = 0;
        }
        if (ok[i])
            count++;
    }

    if (count > 0) {
        pick = Rand(kFletCount);
        if (ok[pick]) {
            HyperSpawnFleet(sys, pick, kAINone, arriving);
            return;
        }
    }
}

// FUNCTION: EVNOVA 0x004254b0
// MAC: _GenericRandomShipSpawn i386 0x0003c89f ppc 0x0004314c
short GenericRandomShipSpawn(short sys, short reserve)
{
    short slot;
    short n;
    short k;

    slot = -1;
    for (n = 1; n < kMaxShips - abs(reserve); n++) {
        if (shipData[n].active == 0) {
            slot = n;
            break;
        }
    }

    if (slot != -1) {
        shipData[slot].active = 1;
        shipData[slot].curSystem = sys;
        shipData[slot].dude = -1;
        shipData[slot].type = 0;
        shipData[slot].aiType = kAIWimpyTrader;
        shipData[slot].govt = -1;
        shipData[slot].vel.x = shipData[slot].vel.y = 0;
        shipData[slot].accelMag = 0;
        shipData[slot].navTargetKind = -1;
        shipData[slot].jumpTimer = 0;
        shipData[slot].aiState = kAIStateHiJack;
        shipData[slot].aiSubstate = kSubLoJack;
        shipData[slot].boarded = 0;
        shipData[slot].formerEscort = -1;
        shipData[slot].cloakDir = 0;
        shipData[slot].cloak = 0;
        shipData[slot].specialMission = -1;
        shipData[slot].grudge = 0;
        shipData[slot].aiTimer = 0;
        shipData[slot].parentShip = -1;
        shipData[slot].targetShip = -1;
        shipData[slot].targetStellar = -1;
        shipData[slot].deathTimer = 0;
        shipData[slot].ionization = 0;
        shipData[slot].ionColor = 0;
        shipData[slot].lightLevel = 0;
        shipData[slot].blinkCount = 0;
        shipData[slot].altSet = 0;
        shipData[slot].animSet = 0;
        shipData[slot].escapePods = shipClass[shipData[slot].type].podCount;
        shipData[slot].bankDir = 0;
        shipData[slot].frameTimer = 0;
        shipData[slot].shieldGlow = 0;
        shipData[slot].weapGlow = 0;
        shipData[slot].annoyance = 0;
        shipData[slot].bankDir = 0;
        shipData[slot].foldDir = 0;
        shipData[slot].foldSet = shipClass[shipData[slot].type].baseSets - 1;
        shipData[slot].animTimer = 0;
        shipData[slot].skill = RandomSkillLevel(shipData[slot].type);
        shipData[slot].credits = 0;
        shipData[slot].aggression = Rand(3) ^ 2;
        shipData[slot].defenseStellar = -1;
        shipData[slot].tractorShip = -1;
        shipData[slot].hired = 0;
        shipData[slot].hailed = 0;
        shipData[slot].auxMission = -1;
        shipData[slot].persType = -1;
        shipData[slot].thrust = 0;
        shipData[slot].speedLimit = 0;
        shipData[slot].fireQueued = 0;
        shipData[slot].odds = -1.0f;
        shipData[slot].engineGlow = 0x20;
        shipData[slot].hasAfterburner = 0;
        shipData[slot].fireTick = 0;
        shipData[slot].hailTick = 0;
        shipData[slot].aimWeapon = -1;
        shipData[slot].escortOrder = -1;
        shipData[slot].sellOnLanding = 0;
        shipData[slot].upgradeOnLanding = 0;
        shipData[slot].seesCloaked[0] = -1;
        shipData[slot].seesCloaked[1] = -1;
        shipData[slot].decloaksWhenHit = -1;
        shipData[slot].fuelRegenState = -1;
        for (k = 0; k < 4; k++)
            shipData[slot].jamming[k] = -1;
        if (shipData[slot].govt != -1 &&
            (govtData[shipData[slot].govt].flags & kGovtAlwaysDisabled))
            shipData[slot].engineGlow = 0;
        if (shipClass[shipData[slot].type].baseSets > 0)
            shipData[slot].animSet = Rand(shipClass[shipData[slot].type].baseSets);
        if (shipClass[shipData[slot].type].animDelay > 0)
            shipData[slot].animTimer = Rand(shipClass[shipData[slot].type].animDelay);
        for (n = 0; n < kCargoTypes; n++)
            shipData[slot].cargo[n] = 0;
        shipData[slot].pos.x = Rand(kSpawnSpread) - kSpawnOrigin;
        shipData[slot].pos.y = Rand(kSpawnSpread) - kSpawnOrigin;
    }
    return slot;
}

                                                         /* 0x0043b4a0 */
                                                         /* 0x004159e0 */
                                                         /* 0x00413990 */

// FUNCTION: EVNOVA 0x004259b0
// MAC: _HyperSpawnFleet i386 0x00041c8d ppc 0x0004960c
void HyperSpawnFleet(short sys, short fleet, short aiType,
                     unsigned char arriving)
{
    short i;
    float speed;
    float dist;
    FletRecord *fd = fleetData;
    size_t f = fleet;
    uint32_t idx = f;
    short j;
    short k;
    short slot;
    short count;
    short stellar;
    unsigned char buf[256];
    short esc;
    FloatPoint pt;
    short type;
    Boolean digit;
    unsigned char c;

    if (fd[f].leadShipType == -1)
        return;
    if (fleetData[f].usable == 0)
        return;

    slot = GenericRandomShipSpawn(sys, kSpawnReserve);
    if (slot != -1) {
        shipData[slot].type = fleetData[idx].leadShipType;
        shipData[slot].govt = fleetData[idx].govt;
        if (aiType != kAINone)
            shipData[slot].aiType = aiType;
        else
            shipData[slot].aiType = shipClass[shipData[slot].type].inherentAI;
        shipData[slot].shield = shipClass[shipData[slot].type].shield;
        shipData[slot].armor = shipClass[shipData[slot].type].armor;
        shipData[slot].persType = -1;
        shipData[slot].specialMission = -1;
        shipData[slot].hasAfterburner = AIHasAfterburner(&shipData[slot]);
        shipData[slot].canScoop = shipData[slot].CanScoop();
        AISetup(&shipData[slot]);
        if ((fleetData[f].flags & kFleetFlagRandomCargo) &&
            AIIsTrader(shipClass[shipData[slot].type].inherentAI) &&
            shipClass[shipData[slot].type].cargo > 0)
            shipData[slot].cargo[Rand(kCargoTypes)] =
                Rand(shipClass[shipData[slot].type].cargo) + 1;
        shipData[slot].lastStellar = -2;
        shipData[slot].prevSystem = -1;
        shipData[slot].vel.x = 0;
        shipData[slot].vel.y = 0;
        shipData[slot].accelMag = 0;
        stellar = PickEmergeStellar(&shipData[slot]);
        if (stellar != -1) {
            AIMakeShipEmergeFromHyperGate(&shipData[slot], stellar);
            shipData[slot].pos.x = stellarData[stellar].pos.h;
            shipData[slot].pos.y = stellarData[stellar].pos.v;
        } else {
            AIMakeShipJumpIn(&shipData[slot]);
            dist = 0;
            speed = 50.0f;
            while (speed > 0) {
                dist += speed;
                speed -= 1.165;
            }
            shipData[slot].pos.x = shipData[slot].pos.y = 0;
            Accel(Rand(kFullCircle), dist + 1000.0f,
                  &shipData[slot].pos);
            pt.x = pt.y = 0;
            shipData[slot].heading =
                CalcInterceptAngle(&shipData[slot].pos, &pt);
            shipData[slot].jumpTimer = -999;
            shipData[slot].jumpTick = ReadTick();
            Accel(shipData[slot].heading, 50.0f,
                  &shipData[slot].vel);
        }
        for (k = 0; k < kMaxWeapTypes; k++) {
            shipData[slot].weap[k].count =
                shipClass[shipData[slot].type].weapCount[k];
            shipData[slot].weap[k].ammo =
                shipClass[shipData[slot].type].weapAmmo[k];
        }
        for (j = 0; j <= 3; j++) {
            count = fleetData[idx].escortMin[j] +
                    Rand(fleetData[idx].escortMax[j] -
                         fleetData[idx].escortMin[j] + 1);
            type = fleetData[idx].escortTypes[j];
            if (type == -1)
                count = 0;
            else if (shipClass[type].canAppear == 0)
                count = 0;
            if (count > 0) {
                for (k = 0; k < count; k++) {
                    esc = GenericRandomShipSpawn(sys, kSpawnReserve);
                    if (esc != -1) {
                        shipData[esc].aiType = kAIEscort;
                        shipData[esc].parentShip = slot;
                        shipData[esc].type = fleetData[idx].escortTypes[j];
                        shipData[esc].dude = -1;
                        shipData[esc].persType = -1;
                        shipData[esc].govt = fleetData[f].govt;
                        shipData[esc].shield =
                            shipClass[shipData[esc].type].shield;
                        shipData[esc].armor =
                            shipClass[shipData[esc].type].armor;
                        AISetup(&shipData[esc]);
                        for (i = 0; i < kMaxWeapTypes; i++) {
                            shipData[esc].weap[i].count =
                                shipClass[shipData[esc].type].weapCount[i];
                            shipData[esc].weap[i].ammo =
                                shipClass[shipData[esc].type].weapAmmo[i];
                        }
                        shipData[esc].heading = shipData[slot].heading;
                        shipData[esc].pos.x =
                            shipData[slot].pos.x + Rand(300) - 150.0f;
                        shipData[esc].pos.y =
                            shipData[slot].pos.y + Rand(300) - 150.0f;
                        shipData[esc].vel.x = shipData[slot].vel.x;
                        shipData[esc].vel.y = shipData[slot].vel.y;
                        shipData[esc].accelMag = shipData[slot].accelMag;
                        shipData[esc].lastStellar = -2;
                        shipData[esc].prevSystem = -2;
                        shipData[esc].swarmLeader = -1;
                        shipData[esc].formLeader = slot;
                        shipData[slot].hasFormation = 1;
                        if ((fleetData[f].flags & kFleetFlagRandomCargo) &&
                            AIIsTrader(shipClass[shipData[esc].type].inherentAI) &&
                            shipClass[shipData[esc].type].cargo > 0)
                            shipData[esc].cargo[Rand(kCargoTypes)] =
                                Rand(shipClass[shipData[esc].type].cargo) + 1;
                        if (stellar != -1) {
                            AIMakeShipEmergeFromHyperGate(&shipData[esc],
                                                          stellar);
                            shipData[esc].pos.x = stellarData[stellar].pos.h;
                            shipData[esc].pos.y = stellarData[stellar].pos.v;
                            shipData[esc].heading = shipData[slot].heading;
                            shipData[esc].aiTimer += Rand(15) + 5;
                            shipData[esc].lastStellar = -2;
                        } else {
                            shipData[esc].jumpTimer = -999;
                            shipData[esc].jumpTick = ReadTick();
                            AIMakeShipJumpIn(&shipData[esc]);
                        }
                    }
                }
            }
        }
        AICalcFormationPositions(&shipData[slot], 1);
        if (arriving && fleetData[f].quoteStrList > 0) {
            GetRandomIndString(buf, fleetData[f].quoteStrList);
            digit = 0;
            for (j = 1; j <= buf[0]; j++) {
                if (buf[j] == '#') {
                    if (digit)
                        c = Rand(10) + '0';
                    else
                        c = Rand(9) + '1';
                    buf[j] = c;
                    digit = 1;
                } else
                    digit = 0;
            }
            PlaySound(beepSnd[4], 1, gSoundVolume, gSoundVolume);
            DisplayComm((PStrChar *)buf, 360);
        }
    }
}

// FUNCTION: EVNOVA 0x00426ce0
// MAC: _SpeakVoiceMessage i386 0x00044558 ppc 0x0004c3cc
void SpeakVoiceMessage(short a, short b, short c)
{
    pendingVoiceMsg = a;
    pendingVoiceGovt = b;
    pendingVoiceParity = c;
}

/* The hail quote is entry <persID> of STR# 7101, unless a plugin overrides it
   as resource 4999+persID. */

#define kHailQuoteResBase 0x1387 /* 4999; the plugin override, ID + persID */
#define kHailQuoteStrList 0x1bbd /* 7101; the built-in STR# */
#define kHailCommFrames   420

// FUNCTION: EVNOVA 0x00426d10
// MAC: _ShowPersonHailQuote i386 0x0004457e ppc 0x0004c3f4
void ShowPersonHailQuote(short persID)
{
    PlaySound(beepSnd[4], 1, gSoundVolume, gSoundVolume);
    if (LoadPluginString(persID + kHailQuoteResBase, parseBuffer) == 0)
        GetIndString((unsigned char *)parseBuffer, kHailQuoteStrList, persID);
    PtoCstr((unsigned char *)parseBuffer);
    ParseTextDescription();
    strcpy(briefingTextPtr, parseBuffer);
    MungeBriefing(0, -1);
    CtoPstr(briefingTextPtr);
    DisplayComm((PStrChar *)briefingTextPtr, kHailCommFrames);
}

// FUNCTION: EVNOVA 0x00426dd0
// MAC: _SpawnBountyHunter i386 0x00044661 ppc 0x0004c4c4
void SpawnBountyHunter(void)
{
    short i;
    short eligible;
    Boolean doHail;
    short shipIndex;

    if (personData[kPersBountyHunter].loaded == 0 || personData[kPersBountyHunter].prepared == 0)
        return;

    eligible = 0;
    doHail = 0;
    for (i = 0; i < kMaxStellars; i++) {
        if (stellarData[i].loaded != 0 && stellarData[i].dominated != 0
            && (stellarData[i].flags2 & kSpobAlwaysDominated) == 0)
            eligible++;
    }

    if (eligible == 1) {
        if (Rand(10) == 0)
            doHail = 1;
    }
    if (eligible > 1) {
        if (Rand(5) == 0)
            doHail = 1;
    }

    if (doHail) {
        shipIndex = SpawnPerson(shipData->curSystem, 1, kPersBountyHunter);
        if (shipIndex != -1) {
            commCount = -1;
            AIMakeShipAttackPlayer(&shipData[shipIndex]);
            offeringShipIndex = shipIndex;
            ShowPersonHailQuote(personData[shipData[shipIndex].persType].hailQuote);
            offeringShipIndex = -1;
        }
    }
}

// FUNCTION: EVNOVA 0x00426ef0
// MAC: _ShotCanHitShip i386 0x0004477e ppc 0x0004c60c
Boolean ShotCanHitShip(ShotObj *shot, ShipObj *ship)
{
    short reachesPlayer;
    short nearPlayer;
    short cur;

    if (shot == 0 || ship == 0)
        return 0;
    if (shot->weapType < 0 || shot->weapType >= kMaxWeapTypes)
        return 0;
    if (ship->type < 0 || ship->type >= kMaxShipTypes)
        return 0;
    if (shot->life > 0.0f && ship->type != kMaxShipTypes - 1) {
        if (shot->owner == ship->index)
            return 0;
        if (shot->guidanceState == 0x3e6)
            return 0;

        if (weaponStats[shot->weapType].guidance == kGuidanceHoming
                && ship->index != shot->target
                && (weaponStats[shot->weapType].flags2 & kWeapProxHitsAnyShip) == 0)
            return 0;

        if (shot->owner >= 0 && shot->owner < kMaxShips) {
            if (shipData[shot->owner].parentShip != -1 && ship->parentShip != -1
                    && ship->parentShip == shipData[shot->owner].parentShip)
                return 0;
            if ((shipData[shot->owner].govt >= 0 && shipData[shot->owner].govt < kMaxGovts)
                    && ship->govt == shipData[shot->owner].govt)
                return 0;
            if (shipData[shot->owner].defenseStellar != -1
                    && ship->defenseStellar == shipData[shot->owner].defenseStellar)
                return 0;

            if (shot->hitRoll > shipClass[ship->type].aiSkill && !ship->IsDisabled())
                return 0;
            if (ship->index == 0)
                nearPlayer = 1;
            else if (AlliedWithPlayer(ship))
                nearPlayer = 1;
            else
                nearPlayer = 0;

            reachesPlayer = 0;
            cur = shot->owner;
            while (1) {
                if (cur == 0) {
                    reachesPlayer = 1;
                    break;
                }
                if (cur < 0 || cur >= kMaxShips) {
                    cur = -1;
                    break;
                }
                cur = shipData[cur].parentShip;
                if (cur == -1)
                    break;
            }

            if (reachesPlayer && nearPlayer
                    && (ship->parentShip >= 0 && ship->parentShip < kMaxShips)
                    && shipData[ship->parentShip].parentShip == 0)
                return 0;
            if (reachesPlayer && shipData[shot->owner].govt >= 0 && shipData[shot->owner].govt < kMaxGovts
                    && (govtData[shipData[shot->owner].govt].flags & kGovtNeverAttacksPlayer) != 0)
                return 0;
            if (reachesPlayer && ship->govt >= 0 && ship->govt < kMaxGovts
                    && (govtData[ship->govt].flags & kGovtImmuneToPlayer) != 0)
                return 0;
            if (reachesPlayer && ship->dude >= 0 && ship->dude < kMaxDudes
                    && (dudeData[ship->dude].flags & kDudeUntouchable) != 0)
                return 0;

            if (nearPlayer) {
                if (shipData[shot->owner].dude >= 0 && shipData[shot->owner].dude < kMaxDudes) {
                    if ((dudeData[shipData[shot->owner].dude].flags & kDudeUntouchable) != 0)
                        return 0;
                } else if (shipData[shot->owner].parentShip >= 0
                        && shipData[shot->owner].parentShip < kMaxDudes
                        && (shipData[shipData[shot->owner].parentShip].dude >= 0
                            && shipData[shipData[shot->owner].parentShip].dude < kMaxDudes)
                        && (dudeData[shipData[shipData[shot->owner].parentShip].dude].flags & kDudeUntouchable) != 0) {
                    return 0;
                }
            }

            if (shot->owner > 0 && AIShipIsAttackingAsteroid(&shipData[shot->owner]))
                return 0;
        } else {
            if (shot->target >= 0 && shot->target < kMaxShips
                    && ship->index != shot->target && ship->defenseStellar != shot->target)
                return 0;
        }

        if ((shipClass[ship->type].flags & kShipPlanetType) != (weaponStats[shot->weapType].flags2 & kWeapPlanetType))
            return 0;

        if ((ship->govt >= 0 && ship->govt < kMaxGovts) && ship->index != 0
                && (govtData[ship->govt].flags & kGovtNeverAttacksPlayer) != 0) {
            if (shot->owner == 0)
                return 0;
            if (shot->owner > 0 && shot->owner < kMaxShips
                    && shipData[shot->owner].parentShip == 0)
                return 0;
        }

        if (shot->owner >= 0 && shot->owner < kMaxShips
                && ShipsShareParent(ship, &shipData[shot->owner]))
            return 0;

        return 1;
    }
    return 0;
}

#define kFixedOne   256.0f      /* NewParticle's 24.8 scale */

// FUNCTION: EVNOVA 0x004274d0
// MAC: _SpawnParticles i386 0x00044b70 ppc 0x0004cad4
void SpawnParticles(short x, short y, float speed, short spreadPct,
                  short minLife, short maxLife, int32_t color, short shade,
                  short count, short jitter)
{
    FloatPoint vel;
    FloatPoint pos;
    short i;
    short life;
    float mag;

    for (i = 0; i < count; i++) {
        vel.x = vel.y = 0;
        mag = speed;
        if (spreadPct > 0 && spreadPct < 100)
            mag *= (Rand(spreadPct * 2 + 1) + (100 - spreadPct)) * 0.01;
        Accel(Rand(kFullCircle), mag, &vel);

        life = minLife;
        if (maxLife > minLife)
            life += Rand(maxLife - minLife + 1);

        pos.x = x;
        pos.y = y;
        if (jitter > 0)
            Accel(Rand(kFullCircle), (Rand(jitter * 100) * 0.01), &pos);

        NewParticle(color, (int32_t)pos.x << 8, (int32_t)pos.y << 8,
                    (vel.x * kFixedOne), (vel.y * kFixedOne),
                    shade, life);
    }
}

// FUNCTION: EVNOVA 0x00427770
// MAC: _GrantOutfitItem i386 0x00044d4f ppc 0x0004cd00
void GrantOutfitItem(short outfit)
{
    short i;
    short s;
    short k;
    short v;
    short govtClass;
    Boolean paintSet;
    Boolean fakeID;

    if (outfit < 0 || outfit >= kMaxOutfits)
        return;
    boughtMap = 0;
    boughtID = 0;
    paintSet = 0;
    fakeID = 0;
    for (i = 0; i < kOutfMods; i++) {
        if (outfitSystem[outfit].modType[i] == OutfMod::Map) {
            v = outfitSystem[outfit].modVal[i];
            if (v > 0) {
                AutoSetExploration(shipData[0].curSystem, v, 2);
            } else if (v == kMapIndependent) {
                for (s = 0; s < kMaxSystems; s++) {
                    if (systemData[s].loaded != 0 && systemData[s].govt == -1
                            && SystemIsInhabited(s) != 0)
                        AutoSetExploration(s, 0, 2);
                }
            } else if (v <= -kMapGovtClassBase) {
                govtClass = -1 * (v + kMapGovtClassBase);
                for (s = 0; s < kMaxSystems; s++) {
                    if (systemData[s].loaded != 0 && systemData[s].govt != -1) {
                        for (k = 0; k < kGovtClasses; k++) {
                            if (govtData[systemData[s].govt].classes[k]
                                    == govtClass) {
                                AutoSetExploration(s, 0, 2);
                                break;
                            }
                        }
                    }
                }
            }
            boughtMap = 1;
            break;
        }
    }
    for (i = 0; i < kOutfMods; i++) {
        if (outfitSystem[outfit].modType[i] == OutfMod::Paint) {
            playerPaint.red =
                ((unsigned short)outfitSystem[outfit].modVal[i] >> 10) & 0x1f;
            playerPaint.green =
                ((unsigned short)outfitSystem[outfit].modVal[i] >> 5) & 0x1f;
            playerPaint.blue =
                (unsigned short)outfitSystem[outfit].modVal[i] & 0x1f;
            paintSet = 1;
            break;
        }
    }
    for (i = 0; i < kOutfMods; i++) {
        if (outfitSystem[outfit].modType[i] == OutfMod::FakeID) {
            fakeID = 1;
            break;
        }
    }
    if (boughtMap == 0) {
        if (paintSet == 0) {
            if (fakeID != 0) {
                for (i = 0; i < kOutfMods; i++) {
                    if (outfitSystem[outfit].modType[i] == OutfMod::FakeID) {
                        for (s = 0; s < kMaxSystems; s++) {
                            if (systemData[s].loaded != 0) {
                                if (outfitSystem[outfit].modVal[i] == -1) {
                                    if (playerRecord[s] < 0)
                                        playerRecord[s] = 0;
                                } else if (systemData[s].govt
                                        == outfitSystem[outfit].modVal[i]) {
                                    if (playerRecord[s] < 0)
                                        playerRecord[s] = 0;
                                }
                            }
                        }
                    }
                }
                boughtID = 1;
            } else {
                shipSystems[outfit]++;
            }
        }
    }
}

/*
 * Fills the first beamData slot whose state is < -1.
 */

// FUNCTION: EVNOVA 0x00427a90
// MAC: _SpawnBeam i386 0x00044ff3 ppc 0x0004d048
void SpawnBeam(short ship, short target, short weapType, short unused, short mount, short flag)
{
    short i;

    for (i = 0; i < kMaxBeams; i++) {
        if (beamData[i].state < -1) {
            beamData[i].state = weaponStats[weapType].lifetime;
            beamData[i].frame = 0;
            beamData[i].weapType = weapType;
            beamData[i].owner = ship;
            beamData[i].targetIsShot = flag;
            beamData[i].mount = weaponStats[weapType].exitType;
            if (mount >= 0 && mount <= 3) {
                beamData[i].lane = mount;
            } else {
                if (shipData[ship].weapGroupLane[weaponStats[weapType].exitType] < 0
                        || shipData[ship].weapGroupLane[weaponStats[weapType].exitType] > 3)
                    shipData[ship].weapGroupLane[weaponStats[weapType].exitType] = Rand(4);
                if ((weaponStats[weapType].flags3 & kWeapClosestExit)
                        && ship != -1 && flag != 1 && target != -1) {
                    beamData[i].lane = SelectClosestShotStartPosition(
                        &shipData[ship], &shipData[ship].pos, weapType,
                        shipData[ship].heading, weaponStats[weapType].exitType,
                        &shipData[target].pos);
                    if (shipData[ship].weapGroupLane[weaponStats[weapType].exitType] < 0
                            || shipData[ship].weapGroupLane[weaponStats[weapType].exitType] > 3)
                        shipData[ship].weapGroupLane[weaponStats[weapType].exitType] = Rand(4);
                } else {
                    beamData[i].lane =
                        shipData[ship].weapGroupLane[weaponStats[weapType].exitType];
                }
                shipData[ship].weapGroupLane[weaponStats[weapType].exitType] = 
                    ((shipData[ship].weapGroupLane[weaponStats[weapType].exitType] + 1) % 4);
            }
            beamData[i].target = target;
            beamData[i].disableOnly = WeaponDisableOnly(weapType);
            if (beamData[i].disableOnly == 0 && ship > 0 && flag != 1 && target != -1
                    && !shipData[target].IsDisabled()
                    && AIShipIsPlunderingShip(&shipData[ship], &shipData[target]))
                beamData[i].disableOnly = 1;
            return;
        }
    }
}

#define kNoRank                 (-1)

// FUNCTION: EVNOVA 0x00427df0
// MAC: _ActivateRank i386 0x000452f0 ppc 0x0004d358
void ActivateRank(char *rankp)
{
    RankRecord *rank = (RankRecord *)rankp;
    short i;

    if (rank->active == 0) {
        crashlog_print("activating rank ");
        crashlog_print(rank->name);
        crashlog_print("\n");
        rank->active = 1;
        if ((rank->flags & kRankGrantDropsSameGovt) != 0) {
            for (i = 0; i < kMaxRanks; i++) {
                if (rankData[i].active != 0
                        && rankData[i].loaded != 0
                        && i != rank->index
                        && rankData[i].govt == rank->govt
                        && (rankData[i].flags & kRankNeverDropped) == 0) {
                    rankData[i].active = 0;
                    if (recentActivatedRank == i)
                        recentActivatedRank = kNoRank;
                }
            }
        }
        if ((rank->flags & kRankGrantDropsLowerPriority) != 0) {
            for (i = 0; i < kMaxRanks; i++) {
                if (rankData[i].active != 0
                        && rankData[i].loaded != 0
                        && i != rank->index
                        && rankData[i].govt == rank->govt
                        && (rankData[i].flags & kRankNeverDropped) == 0
                        && rankData[i].priority < rank->priority) {
                    rankData[i].active = 0;
                    if (recentActivatedRank == i)
                        recentActivatedRank = kNoRank;
                }
            }
        }
    }
    recentActivatedRank = rank->index;
}

// FUNCTION: EVNOVA 0x00427f40
// MAC: _DeactivateRank i386 0x0004541e ppc 0x0004d53c
void DeactivateRank(char *rankp)
{
    RankRecord *rank = (RankRecord *)rankp;
    short i;

    if (rank->active != 0) {
        crashlog_print("deactivating rank ");
        crashlog_print(rank->name);
        crashlog_print("\n");
        rank->active = 0;
        if ((rank->flags & kRankDropsSameGovt) != 0) {
            for (i = 0; i < kMaxRanks; i++) {
                if (rankData[i].active != 0
                        && rankData[i].loaded != 0
                        && rankData[i].govt == rank->govt
                        && (rankData[i].flags & kRankNeverDropped) == 0) {
                    rankData[i].active = 0;
                    if (recentActivatedRank == i)
                        recentActivatedRank = kNoRank;
                }
            }
        }
        if ((rank->flags & kRankDropsLowerPriority) != 0) {
            for (i = 0; i < kMaxRanks; i++) {
                if (rankData[i].active != 0
                        && rankData[i].loaded != 0
                        && rankData[i].govt == rank->govt
                        && (rankData[i].flags & kRankNeverDropped) == 0
                        && rankData[i].priority < rank->priority) {
                    rankData[i].active = 0;
                    if (recentActivatedRank == i)
                        recentActivatedRank = kNoRank;
                }
            }
        }
    }
    if (recentActivatedRank == rank->index)
        recentActivatedRank = kNoRank;
}

/*
 * podData is the 32-slot pod array InitArrays allocates as AllocAPointer(0x300); a slot is
 * free once its life counter has run below zero.
 */

// FUNCTION: EVNOVA 0x00428090
// MAC: _SpawnEscapePod i386 0x00045547 ppc 0x0004d704
void SpawnEscapePod(ShipObj *ship)
{
    short i;
    short angle;
    short heading;
    FloatPoint origin;
    FloatPoint drift;

    for (i = 0; i < kMaxPods; i++) {
        if (podData[i].life < 0.0f) {
            podData[i].life = (Rand(100) + 150);
            podData[i].pos.x = ship->pos.x;
            podData[i].pos.y = ship->pos.y;
            podData[i].vel.x = ship->vel.x;
            podData[i].vel.y = ship->vel.y;

            angle = Rand(kFullCircle);
            SWSetSpriteVisible(podData[i].sprite, 1);
            Accel(angle, (Rand(10) + 10) * 0.1, &podData[i].vel);

            origin.x = origin.y = 0;
            drift.x = podData[i].vel.x * 100.0;
            drift.y = podData[i].vel.y * 100.0;
            heading = CalcInterceptAngle(&origin, &drift);
            SWSetCurrentFrameIndex(podData[i].sprite,
                         (heading * shipClass[kShipTypeEscapePod].framesPer / 360.0));
            break;
        }
    }
}
