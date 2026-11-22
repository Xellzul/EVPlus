/* ships.h -- the ship object: its rates, damage, outfits, cargo and spawning. Notes: Research/notes/ships.md */
#ifndef SHIPS_H
#define SHIPS_H
#include "burger.h"
#include "shipobj.h"

extern void AICalculateOddsAgainst(ShipObj *ship);
extern void CalcShipCanBuy(void);
extern void CalculateTargetedDamage(void);
extern Boolean abOn;
extern void DamageShip(ShipObj *ship, FloatPoint *from, short impact, short armorDmg,
                       short shieldDmg, short attacker, Boolean fromAttack, Boolean deliberate,
                       unsigned char disableOnly, Boolean clampArmor,
                       unsigned char armorOnly, short provocation);
extern void DoAutoRefuel(void);
extern void DoShipCapture(ShipObj *ship, Boolean keepShip);
extern void EnterMoreShips(short system);
extern void SetupShipsInSystem(short sys, unsigned char arriving);
extern void FirePlayerWeapon(short weapon);
extern void GrantOutfitItem(short outfit);
extern void HandleReinforcements(void);
extern void HandleShip(ShipObj *ship);
extern void HandleShipDisplay(ShipObj *ship);
extern Boolean HasMaxOfItem(short outfit, short *countOut,
                         short *maxOut);
extern short HyperShipSpawn(short sys);
extern void HyperSpawnFleet(short sys, short fleet, short escortOf,
                            unsigned char arriving);
extern void IonizeShip(ShipObj *ship, WeapRecord *weap, FloatPoint *pos);
extern void SpawnEscapePod(ShipObj *ship);
extern short MissionDudeSpawn(short dude, short wantType, short sys,
                              short mission);
extern int32_t PlayerShipTradeInPrice(void);
extern short RandomShipSpawn(short sys, short reserve);
extern void ShipStatsToSystemInfo(void);
extern Boolean ShipsShareParent(ShipObj *a, ShipObj *b);
extern void SpawnBox(short slot);
extern short SpawnEscort(short shipType, short stellar);
extern void SpawnFleet(short sys, unsigned char arriving);
extern void SpawnScoopableBox(FloatPoint *pos, short cargoType, short pict);
extern void SystemInfoToShipStats(void);
extern short TotalMissionCargoSpace(void);
extern Boolean WeaponHasAmmo(ShipObj *ship, short weaponIndex);
extern short WeaponMaxSimultShots(ShipObj *ship, short w);
extern Boolean allReinforcementsInhibited;
extern Handle charFileHand;
extern unsigned char govtMediumNames[256][256];
extern Boolean gravitySourcesPresent;
extern DateTimeRec scenarioStartDate;
extern short tradePrice[];
extern short portDemand[6];
extern short junkType[2];

extern FloatPoint XYShortToFloat(ShortPoint pt);
extern Boolean CanBuyFighter(short shipType, short outfitIndex, short weapIndex);
extern int32_t ApplyPriceAndTechnologyFlux(int32_t cost, float multiplier, short itemTech,
                            short portTech);
extern void JettisonCargo(unsigned char withMissions);
extern Boolean CanBuyOutfitItem(short outfit);
extern unsigned char TurretBlindSpot(ShipObj *ship, short heading, short weap);
/* statusRedraw[] indices: the status bar panel DrawStatus redraws */
#define kRedrawNav          0
#define kRedrawShield       1
#define kRedrawCargo        2
#define kRedrawWeap         3
#define kRedrawFuel         4
#define kRedrawRadar        5
#define kRedrawTarg         6
#define kStatusRedrawCount  7
extern Boolean statusRedraw[kStatusRedrawCount];

#endif
