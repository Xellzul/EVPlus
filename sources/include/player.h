/* player.h -- the player's record, standing, pilot file and preferences. Notes: Research/notes/player.md */
#ifndef PLAYER_H
#define PLAYER_H
#include "gameconst.h"
#include "macport.h"
#include "shipobj.h"
#include "pilotfile.h"

/* the player's cached ship values; Reset() marks them not computed */
struct PlayerStats {
    float shieldCapacity;
    float shieldRechargeRate;
    float armorCapacity;
    float armorRechargeRate;
    float fuelCapacity;
    float turnRate;
    float accelRate;
    float maxSpeed;
    float ionizeMax;
    float deionizeRate;
    float abFuelUsage;
    short hasAfterburner;
    short hasDensityScanner;
    short hasIFF;
    short antiInterference;
    short effectiveMurk;
    short canTargetUntargetableShips;
    short canTargetCloakedShips;
    short hasInertialDamper;
    short resistsGravity;
    short resistsDeadlyStellars;
    short hasAreaCloak;

    void Reset();
};

extern PlayerStats playerStats;
extern Settings settings;
extern short recentActivatedRank;
extern KeyPrefs keyPrefs;
extern uint32_t lastEscortMenuTime;
extern float playerRepairInhibit;
extern Boolean playerHasIllegalJunk;
extern Boolean playerHasIllegalOutfits;
extern Boolean playerHasTribbles;
extern Boolean playerHasPerishableJunk;
extern Boolean playerTurnedThisFrame;
extern float sabotageCounter;
extern Boolean lastJumpDistOk;
extern Boolean ackCommFlag;
extern Boolean navModeSelectFlag;
extern Boolean navTargetSelectFlag;
extern Boolean navDefaultSelectFlag;
extern Boolean helpKeyFlag;
extern Boolean cloakFlag;
extern Boolean fighterCommandFlag;
extern Boolean weapModeSelectFlag;
extern Boolean weapTargetSelectFlag;
extern Boolean weapDefaultSelectFlag;
extern Boolean playerInfoFlag;
extern Boolean mapTryFlag;
extern Boolean jumpTryFlag;
extern Boolean weapSelectFlag;
extern Boolean spawnShipFlag;
extern Boolean weapCloseTargFlag;
extern Boolean missionInfoFlag;
extern Boolean commTryFlag;
extern Boolean mapZoomFlag;
extern Boolean framerateFlag;
extern Boolean escortMenuFlag;
extern Boolean escortCommandFlag[];
extern short lastEscortCount;
extern void DrawCombatRatingString(void);
extern void DrawLegalStatusString(short curSystem);
extern void DumpPilotLog(void);
extern void KeyPrefsToSlotTable(void);
extern void SlotTableToKeyPrefs(void);
extern short PlayerEffectiveMurk(void);
extern void SetDefaultPrefs(void);
extern void SetDefaultKeys(void);
extern int32_t ShipFreeMass(void);
extern void LoadStellarResponse(short which);
extern void HandlePlayer(ShipObj *ship);
extern void HandlePlayerDockRequest(void);
extern void LoadPrefs(void);
extern void PrefsJsonLoad(const char *path, PrefsRecord *rec);    /* prefsjson.cpp */
extern void PrefsJsonSave(const char *path, PrefsRecord *rec);
extern void LoadResponse(short which);
extern int32_t DoEncryption(void *buf, uint32_t len, uint32_t key);
extern int32_t SaveAFile(const char *path, const void *data, uint32_t length);
extern short LoadPilotData(char *path);
extern void OpenLastPilotFile(void);
extern Boolean OpenPilotFile(char *missingResource);
extern unsigned char QuickKeyCheck(ScanCode scanCode);
extern void QuickKeyCheckHandler(void);
extern unsigned char RawKeyCheck(ScanCode scanCode);
extern void RecursivePenaltySlap(short sys, short govt, short type,
                                 double amount);
extern void ResetPlayerPrecalcedValues(void);
extern short SWFlagRectAsChanged(SpriteWorldPtr sw, Rect *r);
extern void SWHideAllSprites(SpriteWorldPtr sw);
extern void SavePrefs(void);
extern void ScanPlayer(ShipObj *ship);
extern void SetAllKeyCmdFlags(void);
extern short ShipAntiInterference(void);
extern void ShowAlert(const char *title, const char *message);
extern void SlapWithPenalty(short sys, short govt, short kind,
                            short mission);
extern short WritePilotData(short stellar);
extern short adviceRandom;
extern short alertSpritePending;
extern short antiEscortDelay;
extern Boolean dockLand;
extern short bribeRandom;
extern Boolean cheatsActive;
extern float destructCount;
extern short dockPermission;
extern short dockTarget;
extern Boolean dontMovePlayerAfterLanding;
extern Boolean fadePending[2];
extern char gPrefsDirectory[];
extern short selEscortGroup;
extern short globalPriceFlux[2];
extern Boolean quickKeyCheckResult;
extern Boolean killAsteroids;
extern Boolean killBoxes;
extern Boolean killExplods;
extern Boolean killSmoke;
extern short lastMissionStellar;
extern Boolean leetMode;
extern short localPriceFlux[2];
extern float navTargetExpand;
extern short newbieFlag;
extern Boolean noSavePilotFiles;
extern Boolean noUpdatePilotData;
extern Boolean oldPlayerThreatened;
extern char pilotsFolder[];
extern short playerGender;
extern Boolean playerIsLanded;
extern int32_t playerKills;
extern FloatPoint playerMaxVel;
extern short playerSabotaged;
extern Boolean playerThreatened;
extern Boolean seenIntroScreen;
extern Boolean smuggleFlag;
extern Boolean strictPlay;
extern Boolean systemFlag[kMaxSystems];
extern Boolean weapLawFlag;
extern float weapTargetExpand;

extern Boolean escortGroupFlag[5];
extern ScanCode newKeyPrefs[40];
#define kBindingCount 35            /* the key bindings: Key Settings rows, prefs file slots */

extern double gSpeedMult;
extern unsigned char shipName[256];
extern PilotRec2 **altPlayerFileHand;
extern PilotRec **playerFileHand;

extern short SavePilotFile(short stellar);
extern void HandlePlayerCommunication(void);
extern void ActivateCharResource(const unsigned char *name, Boolean applyStats);
extern void DeletePilotFile(const char *name);
extern void ActivateRank(char *rank);
extern void DeactivateRank(char *rank);
extern short playerRecord[0x800];
extern void ResetPlayer(Boolean newPilot);
extern void ResetPlayerRecord(Boolean newPilot);

#endif
