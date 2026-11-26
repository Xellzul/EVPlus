/* game.h -- starting, running and stopping a game. Notes: Research/notes/game.md */
#ifndef GAME_H
#define GAME_H
#include <string>
#include "resources.h"
#include "gameconst.h"
#include "spriteworld.h"
#include "movie.h"
#include "soundtool.h"

namespace UI {
struct TextSpec;
}

/* the registered name <REG> shows (mission text, the loading bar): EV Nova CE's unlicensed default */
#define kRegisteredName "EV Nova Community"

extern Boolean AIHasAfterburner(ShipObj *ship);
extern void AbortMission(short mission, Boolean evalBits);
void ActivateCron(short index);
extern void AutoAbortMission(short mission);
extern void Bye(void);
extern void ClearParticles(Boolean on);
extern void TableCos(short angle);
extern short commCount;
extern uint32_t floatingMapStartTime;
extern SpriteRec *masterStellarSprite[kMasterStellarSprites];
extern SpriteRec *masterShotSprite[256];
extern void CreateExplosion(FloatPoint pt, short type, short size, Boolean sound);
extern void SpawnExplod(FloatPoint pos, short unused, short type, short delay);
extern short ConvertSpriteToRLE(SpriteRec *set);
extern SpriteRec *masterNavTargetSprite;
extern SpriteRec *masterWeapTargetSprite;
extern SalvageAmmo salvageAmmoType;
extern SpriteRec *masterStarSprite;
extern SpriteRec *menuRolloverSprite;
extern SpriteRec *masterAsteroidSprite[16];
extern SpriteRec *menuButtonSprite[6];
extern SpriteRec *alertSprite;
extern SpriteRec *cursorSprite;
extern SpriteRec *doubleTimeSprite;
extern Sound *redAlertSnd;
extern Sound *explodSnd[];
extern Sound *klaxxonSnd;
extern Sound *clickSnd;
extern short lastShipAIDispatched;
extern void DisplayComm(PStrChar *msg, short count);
extern void DisplayCommStyled(PStrChar *msg, short count, const UI::TextSpec &spec);
extern void DoNewPilot(void);
extern void DoStellarTimePassage(void);
extern Boolean DoesPilotExist(const char *path);
extern void EnterGameFromMainScreen(void);
extern Boolean EvalMissionBitTestString(char *expr);
extern void EvalMissionBitSetString(char *expr);
extern short SelectNearestAnyStellar(ShipObj *ship, short exclude);
extern Boolean AIShouldLeaveSystem(ShipObj *ship, short sys);
extern Boolean SufficentTargetedDamage(ShipObj *ship);
extern void MakeEscortsHyperWithParent(ShipObj *ship);
extern void HandleLocalPriceFlux(void);
extern void PointDefenseCollisionHandler(SpriteRec *shotSpr, SpriteRec *otherSpr,
                         Rect *sectRect);
extern void DrawBeams(struct BeamLines &out, bool underShips);
extern short SelectMissionCargoType(short v);
extern short SelectMissionCargoQty(short v);
extern void ResetMissionOfferVenue(void);
extern void Trim(short *pos);
extern Boolean DupKeyCheck(ScanCode code, short excludeRow);
extern void CheckSpeechMgr(void);
extern Boolean PlayerHasAnyCargo(void);
extern void AddToPlayerKills(int strength);
extern short InitParticles(int numElements, int gravity);
extern void DisposeParticles(void);
extern void DoAboutBox(void);
extern void InitMainScreen(void);
/* the first bindable key held; Caps Lock only when capsLock */
extern ScanCode FirstKeyDown(bool capsLock);
extern Boolean SpeedUpKeyOn(void);           /* the speed-up key asks for double time */
extern Boolean GovtSharedClass(short a, short b);
extern void HandleAlertSprite(void);
extern void HandleDoubleTimeSprite(void);
extern void HandleGlobalPriceFlux(void);
extern void HandleMouse(void);
extern void HandleTimeAdjustment(void);
extern void IncrementGameTime(void);
extern void InitArrays(void);
extern void InitEVNova(void);
extern void InitMissions(void);
extern void InitObjects(void);
extern void InitSprites(void);
extern void KillVoiceSound(void);
extern void MainScreenIdle(void);
extern void MissionHandlePlayerEnteredNewSystem(void);
extern Boolean OfferMissionFromPort(short venue);
extern void PlayGame(void);
extern Boolean PlayerCanUseAmmoType(short weapon);
extern Boolean PlayerHasJettisonableCargo(void);
extern void RefreshWindowBuffer(void);
extern void ReportSpriteLoadError(short label, short value, short special);
extern void SpriteLoadFailed(const char *prefix, int logNum, short label, short value, short err);
extern void ResetGameData(void);
extern short SelectDudeFieldFromSystem(SystRecord *syst);
extern void AdjustWarpSoundSpeed(double rate);
extern void SetupSpriteWorld(void);
extern void SetupWind(void);
extern short ShipTotalHolds(void);
extern void TableSin(short angle);
extern short SpawnPerson(short sys, unsigned char noDisabled, short person);
extern void SpeakVoiceMessage(short kind, short govt, short voice);
extern void SpeedLimit(FloatPoint *p, float mag);
extern Boolean SuitableMissileType(short weapon, ShipObj *target);
extern Boolean SystemVisible(short sys);
void TerminateCron(short index);
extern short TotalCargo(ShipObj *ship);
extern short TrackMainScreenButtons(Point where);
extern void UpdateAllPortDialogs(); /* unprototyped: the 1.1.1 DoShipyardDialog passes it the port dialog */
extern short WeaponBurstInterval(short weap, short count);
extern Boolean WeaponDisableOnly(short weap);
extern std::string absquatulateStr;
extern SpriteLayerRec *asteroidLayer;
extern SpriteLayerRec *boxLayer;
extern float tableCos[360];
extern short cursorAlpha;
extern short cursorHidden;
extern SpriteLayerRec *disabledShipLayer;
extern Boolean doubleTime;
extern SpriteLayerRec *escortLayer;
extern short escortMenuOnscreen;
extern SpriteLayerRec *explodLayer;
extern short firstShipAltIndex;
extern short firstShipGlowIndex;
extern short firstShipLightIndex;
extern short firstShipShieldIndex;
extern short firstShipWeapIndex;
extern Boolean forceRandom;
extern short fpsCounter;
extern short frameCounter;
extern int32_t fullScreenRestore;
extern Sound *escapePodSnd;
extern Sound *cloakSnd[2];
extern Boolean redrawEscortMenu;
extern Sound *warpUpSnd[2];
extern Boolean warpSoundDone;
extern Boolean gPause;
extern short pluginCount;
extern Boolean majorPrefsChange;
extern Boolean didSpeak;
extern Boolean doMainScreenAnim;
extern Rect standardPicRect;
extern Boolean gameFrozen;
extern DateTimeRec gameTime;
extern Image_t *greenMapArrow;
extern Boolean ignoreKeys;
extern Boolean keyPressedThisFrame;
extern uint32_t lastEventLoopTime;
extern UnsignedWide lastFrameMicroseconds;
extern FloatPoint lastFramePlayerPos;
extern uint32_t lastKeypressTime;
extern uint32_t lastMouseMoveTime;
extern Point lastMousePos;
extern short lastPortMissionLoc;
extern uint32_t lastSmokeUpdateTime;
extern Boolean mBarHidden;
extern Rect mainWindRect;
extern GWorldContainer *MenuArt(void);
extern void ShowMenuArt(const Rect &area, bool onlyChanged = false);
extern void ShowLoadPage(void);
extern void SetupMonitor(void);
extern void ApplyUIScale(void);
extern void ApplyDisplaySettings(void);
extern void ApplyPilotGameSize(void);
extern short GameSizeInUse(void);           /* the game size the view has (Preferences may hold a later one) */
extern Boolean OrBlendInUse(void);          /* the blending the renderer was made for (a change needs a restart) */
extern void ApplyDisplayScaling(void);
extern void ApplyBrightness(void);
extern short menuFontID;
extern RGBColor menuTextColor[2];
extern Sound *missionSnd;
extern short missionSoundID;
extern Str255 newPilotPopupChoice[2];
extern Str255 newPlayerName[2];
extern Boolean pauseKeyFlag;
extern Rect progressDefRect;
extern Image_t *redMapArrow;
extern SpriteLayerRec *shipLayer;
extern Boolean showFPS;
extern float tableSin[360];
extern SpriteLayerRec *smokeLayer;
extern Rect spriteClipRect;
extern SpriteLayerRec *starLayer;
extern SpriteLayerRec *stellarLayer;
extern Boolean suppressMissionOffer[];
extern SpriteLayerRec *targetLayer;
extern SpriteLayerRec *interfaceLayer;
extern float trigResult;
extern Sound *voiceSnd;
extern Sound *warpOutSnd;

extern Movie introMusicMovie;

extern unsigned char lastPlayerName[256];

extern void IterateInGameEventLoop(short tickCount);
extern short CalcInterceptAngle(FloatPoint *from, FloatPoint *to);
extern short AngularDifference(short a, short b);
extern Boolean AIPerformsComplexManeuvers(ShipObj *ship);
extern void AIFireTurret(ShipObj *ship);
extern void SetMissionDeadline(DateTimeRec *out, short days);
extern void SwitchedIn(void);
extern void SwitchedOut(void);
extern void WarpUpCallback(int which, int a, int b);
extern void SpeakIt(char *msg, short flag);
extern Boolean CanBuyGoods(short item);
extern Boolean CanSellGoods(short item);
extern void PropagateMissionBitEffects(void);
extern void FadeWhiteIn(void);
extern void FadeWhiteOut(void);
extern void FlashView(void);
extern void ExploreNebulaeFromSystem(short sys);
extern short introTextID;
extern short introPictID[4];
extern short introPictDelay[4];
extern short menuTextSize;
extern uint32_t lastCursorAnimTime;
extern uint32_t lastFrameTime;
extern short TotalFleetHolds(void);
extern void DeleteAllComputerShips(Boolean force, Boolean keepPlayer);
extern Sound *beepSnd[5];

extern SpriteRec *stellarSprite[kStellarSprites];

#endif
