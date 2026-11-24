/* ai.h -- what an AI ship decides to do: targets, threats, escorts, orders. Notes: Research/notes/ai.md */
#ifndef AI_H
#define AI_H
#include "resources.h"
#include "shipobj.h"
#include "soundtool.h"

extern void AICalcShipFormPos(ShipObj *escort, ShipObj *leader, short escortIndex,
                          short escortCount, short spacing);
extern void AICalcFormationPositions(ShipObj *ship, Boolean recalc);
extern void AICallForHelp(ShipObj *ship);
extern void AICheckParentExists(ShipObj *ship);
extern Boolean AIDoesShipLikePlayer(ShipObj *ship);
extern Boolean AIEvalHopelessChase(ShipObj *ship);
extern void AIFightEnemyThreatToPlayer(ShipObj *ship);
extern void AIFightThreatToParent(ShipObj *ship);
extern void AIFindShipToPlunder(ShipObj *ship);
extern short AIFindSwarmLeader(ShipObj *ship);
extern void AIForceBurstReload(ShipObj *ship);
extern void AIHandleCloaking(ShipObj *ship);
extern Boolean AIIsFighterReturning(ShipObj *ship);
extern Boolean AIIsShipAttemptingDisable(ShipObj *ship);
extern Boolean AIIsShipDoingCountWait(ShipObj *ship);
extern Boolean AIIsShipLeaving(ShipObj *ship);
extern void AIIssueEscortOrders(ShipObj *ship);
extern void AIMakeEscortFlyInForm(ShipObj *ship);
extern void AIMakeShipEmergeFromHyperGate(ShipObj *ship, short stellar);
extern void AIMakeShipJumpIn(ShipObj *ship);
extern void AIMakeShipLeave(ShipObj *ship);
extern void AIMakeShipStopRefuelling(ShipObj *ship);
extern void AISetup(ShipObj *ship);
extern void AIShipCapturedByPlayer(ShipObj *ship);
extern Boolean AIShipHasFuelForJump(ShipObj *ship);
extern Boolean AIShipIsAttackingAsteroid(ShipObj *ship);
extern Boolean AIShipIsDockedWithShip(ShipObj *ship, ShipObj *player);
extern Boolean AIShipIsDocking(ShipObj *ship);
extern Boolean AIShipIsEmergingFromHypergate(ShipObj *ship);
extern Boolean AIShipIsEnteringHypergate(ShipObj *ship);
extern Boolean AIShipIsPlundering(ShipObj *ship);
extern Boolean AIShipIsPlunderingShip(ShipObj *ship, ShipObj *other);
extern Boolean AIShipIsShipGoingToStellar(ShipObj *ship, short stellar);
extern Boolean AIVerifySwarmLeader(ShipObj *ship);
extern void BraveTraderAI(ShipObj *ship);
extern Boolean CanHireEscorts(void);
extern void AIShipBoardedByPlayer(ShipObj *ship);
extern Sound *hardDockSnd;
extern void DeathStarAI(ShipObj *ship);
extern void DefenseShipAI(ShipObj *ship);
extern void DoEscortLand(void);
extern void DoEscortPayment(short days);
extern void DoGoodSamaritan(ShipObj *victim, ShipObj *attacker);
extern void EscortAI(ShipObj *ship);
extern Boolean ExtendedIsThreatToPlayer(ShipObj *ship);
extern Boolean IsRefuellingPlayer(ShipObj *ship);
extern Boolean IsRepairingPlayer(ShipObj *ship);
extern Boolean IsPlayerThreatened(void);
extern void AIMakeShipJumpWithParent(ShipObj *ship);
extern void AIMakeShipRefuelPlayer(ShipObj *ship);
extern void AIMakeShipRepairPlayer(ShipObj *ship);
extern void AIMakeShipStopJumping(ShipObj *ship);
extern Boolean AIIsShipJumping(ShipObj *ship);
extern Boolean AIIsShipJumpingIn(ShipObj *ship);
extern Boolean IsShipInCloseForm(ShipObj *ship);
extern Boolean AIIsShipBusy(ShipObj *ship);
extern Boolean AIInGunRange(ShipObj *me, ShipObj *target,
                         short weapType);
extern Boolean AIDoSwarming(ShipObj *ship);
extern Boolean AIShipIsScooping(ShipObj *ship);
extern short FindNearestShipToPlayer(void);
extern short FindNearestThreatToParent(ShipObj *me, short spob);
extern int32_t IsShipANearbyThreatToParent(ShipObj *ship, ShipObj *me,
                                        short range);
extern short FindNextShipInSystem(short cur, short system);
extern short FindPrevShipInSystem(short cur, short system);
extern void FireAIShipWeapon(ShipObj *ship);
extern void HandleNavTargetSprites(void);
extern void HandlePlayerBoardAttempt(void);
extern void HandlePlayerDockApproach(void);
extern void HandleWeapTargetSprites(void);
extern void HighLevelAIHandler(ShipObj *ship);
extern void InterceptorAI(ShipObj *ship);
extern Boolean IsPlayerThreatenedByMyEnemy(ShipObj *ship);
extern Boolean IsShipAttacking(ShipObj *ship);
extern Boolean IsShipFightingMe(ShipObj *ship, ShipObj *other);
extern Boolean IsShipThreatened(ShipObj *ship);
extern Boolean IsShipThreatenedByMyEnemy(ShipObj *ship,
                                      ShipObj *me);
extern Boolean IsThreatToPlayer(ShipObj *ship);
extern Boolean IsThreatToStellar(ShipObj *ship, SpobRecord *spob);
extern void LowLevelAIHandler(ShipObj *ship);
extern void MinerAI(ShipObj *ship);
extern void PirateWarshipAI(ShipObj *ship);
extern void RespawnEscort(ShipObj *ship, unsigned char full);
extern short SelectFleetFromSystem(SystRecord *syst);
extern void SelectWarshipTarget(ShipObj *ship);
extern void WarshipAI(ShipObj *ship);
extern void WimpyTraderAI(ShipObj *ship);
extern Boolean boardTryFlag;
extern char creditCString[0x20];
extern Boolean dockTryFlag;
extern float hyperGamma;
extern Boolean miningBoxesActive;
extern Boolean miningShipsActive;
extern Boolean scanActive;
extern short aiComplexity;

extern void AIMakeShipAttackPlayer(ShipObj *ship);
extern void AIFightThreatToPlayer(ShipObj *ship);
extern short AIShipFriendStrength(ShipObj *ship);
extern short AIShipIsOutOfAmmo(ShipObj *ship);
extern Boolean AIHasDestroyingWeapons(ShipObj *ship);
extern Boolean IsShipMyEnemy(ShipObj *ship, ShipObj *other);
extern Boolean ExtendedIsThreatToShip(ShipObj *self, ShipObj *other);
extern void AIFireMissile(ShipObj *ship);
extern void AIFireGun(ShipObj *ship, Boolean allowUnguided);
extern void AIFireGunUntargeted(ShipObj *ship);
extern void AIFirePlanetGun(ShipObj *ship);
extern void AILaunchFighter(ShipObj *ship);
extern void AIEscortFireUnprovoked(ShipObj *ship);
extern void AIPlunderShipContents(ShipObj *pirate, ShipObj *victim);
extern void AIMaintainFormation(ShipObj *ship, Boolean snap);
extern void AIAddFighterToParent(ShipObj *fighter);

extern short FindNearestValidShotTarget(ShotObj *shot);
extern Boolean AICallForReinforcements(ShipObj *ship, Boolean force);
extern void AIDispatch(ShipObj *ship, Boolean noBrains);
extern void SpawnBountyHunter(void);
extern Boolean IssueNewEscortCommand(short aiTypeFilter, short cmd, Boolean quiet);
extern short SelectRandomStellarDest(ShipObj *ship, unsigned char allowSpecial,
                                     Boolean plainOnly);

/* AI tuning numbers; a constexpr object for now, a settings file may fill it later. */
struct AITuning {
    int respawnDelayTicks = 45;           /* AIDispatch: ticks after fireTick before a ship acts again */
    float scanRange = 100.0F;             /* ScanPlayer: scan distance on each axis */
    int scanChance = 75;                  /* ScanPlayer: Rand(100) above this skips the scan */
    int escortRange = 0x4e2;              /* BraveTraderAI: axis distance to defend a ship */
    float boardCashInDelay = 100.0f;      /* PirateWarshipAI: aiTimer after boarding */
    float backOffDelay = 120.0f;          /* PirateWarshipAI: aiTimer when backing off */
    double escortRangeSq = 408375.0;      /* EscortAI: squared; past it the target is dropped */
    float maxTargetRangeSq = 36000000.0f; /* DefenseShipAI: 6000^2, hand-verified at 0x00575060 */
    int playFieldLimit = 0x3e8;           /* SelectRandomStellarDest: stellar pos limit */
    double rangeMargin = 0.95;            /* AIFireMissile: share of range squared */
    double rangeScale = 2.5;              /* AIFireGun: proxRadius multiplier */
    int rangeSlack = 32;                  /* AIInGunRange: added to weapon range */
    double minShieldWeight = 0.25;        /* AIShipFriendStrength: ratio clamp */
    double maxShieldWeight = 1.0;
    float cloakDistance = 165.0f;         /* AIHandleCloaking: axis distance to stay cloaked */
    int killsPerStrength = 0x1900;        /* AICalculateOddsAgainst */
    float formDeadband = 8.0f;            /* AIMaintainFormation */
    double formAccelScale = 10.0;
    double formDampCap = 0.8;
    float rangePad = 32.0f;               /* FireAIShipWeapon: added to weapon range */
    float dockRange = 16.0f;              /* AIShipIsEnteringHypergate: axis distance */
    int jumpFuel = 100;                   /* AIShipIsEnteringHypergate: min shïp fuel */
};
inline constexpr AITuning aiTune{};

#endif
