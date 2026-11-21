/* gameconst.h -- engine-wide constants: table sizes, AI states, UI metrics. */
#ifndef GAMECONST_H
#define GAMECONST_H

#define kMaxShips               64      /* shipData slots; slot 0 is the player */

/* ShipObj.aiState: the high-level goal, named as the game's own debug
   readout (DrawHighGoalAbbreviation) prints it */
enum {
    kAIStateHiJack,
    kAIStateGoStel,
    kAIStateHypOut,
    kAIStateDefRet,
    kAIStateFightSh,
    kAIStateGoHome,
    kAIStateWait,
    kAIStateInspect,
    kAIStateJumpIn,
    kAIStateRefuel,
    kAIStateFlyPr,
    kAIStateHypPr,
    kAIStateProtect,
    kAIStatePlunder,
    kAIStateUnPlunder,
    kAIStateRepair,
    kAIStateAtkAster,
    kAIStateScoop,
    kAIStateDestStel,
    kAIStateConfused,
    kAIStateGoHGate,
    kAIStateEmerge,
    kAIStateCountWait
};

#define kShipTypeEscapePod      0x2ff   /* the last shïp slot is the escape pod */

#define kScanRangeStep          0x258   /* ShipObj.aggression units -> pixels */

#define kMaxBoxes               64      /* boxData slots */

/* ShipObj.aiSubstate: the low-level goal, named as DrawLowGoalAbbreviation
   prints it */
enum {
    kSubLoJack,
    kSubKillSpd,
    kSubFlyStel,
    kSubFlyHypD,
    kSubHypJmp,
    kSubRunAwy,
    kSubTF,                 /* "T+F" */
    kSubMissile,
    kSubDock,
    kSubChase,
    kSubZoom,
    kSubChasSlw,
    kSubFormFly,
    kSubJmpPar,
    kSubWaitTarg,
    kSubBoard,
    kSubVeerOff,
    kSubAfterBurn,
    kSubChaseSwarm,
    kSubChaseAst,
    kSubKillAst,
    kSubScoop,
    kSubAttkStel,
    kSubEnterHG
};

/* ShipObj.escortOrder and playerEscortOrders[]: an escort's standing order */
#define kOrderFormation         0
#define kOrderDefend            1
#define kOrderAttack            2
#define kOrderRegroup           3       /* fighters return to the bay */
#define kOrderHold              4       /* hold position */


#define kMaxActiveMissions      16      /* the player's active-mission slots */
#define kAnimDone        0x7d00
#define kMaxMissions    1000    /* mïsn resources; also bounds every mission list */
#define kBetaPilotVersion 0x6b
#define kBoxSlots               0x40
#define kBracketInset       0x10
#define kCargoRect  7
#define kCargoTypes         6       /* the standard commodities */
#define kLastCommodity      5       /* cargo[0..kLastCommodity], the standard commodities */
#define kMaxCargo           256     /* cargo name slots, junk included */
#define kCommFrames         240     /* frames a comm message stays up */
#define kCommTextSize       12      /* comm strip point size */
#define kDailyRollRange   100       /* every daily 1..100 reroll in the game */
#define kDudeShipSlots   16
#define kExpandRate         60.0     /* units per second, a pooled double */
#define kGameYearOffset 250     /* the game opens this far after the host clock */
#define kMaxGovts               256     /* govtData entries */
#define kGridCells    20
#define kGridCols     4
#define kGridItem     5
#define kHalfCircle             180
#define kHalfTurn               180.0f
#define kHiliteFull         0x20
#define kHyperFuelUnit      100.0   /* one jump costs this much fuel */
#define kItemAccept                    1
#define kMaxJunks           128
#define kMainButtons     6
#define kMapPictItem   3
#define kMasterStellarSprites 256
#define kMaxAsteroids           1024
#define kClassicMaxAsteroids    16      /* the count limit without Asteroid Scaling and at 1024x768 */
#define kMaxRoidTypes           16
#define kWrapMargin             32      /* asteroids live this far past the screen edge */
#define kMaxBeams               64
#define kMaxExplods       0x20
#define kMaxExplodTypes   64      /* explodType[] entries: bööm 128..191 */
#define kMaxCrons            512
#define kMaxDebris        0x40
#define kMaxDudes         512
#define kMaxDisasters   0x100
#define kMaxFleets      0x100
#define kMaxHyperRoute        32
#define kMaxMissionBits    10000
#define kNavSlots               16      /* systemData[].navDefault entries */
#define kMaxSystemLinks         16      /* systemData[].links entries */
#define kSpobHyperLinks         8       /* stellarData[].hyperLinks: a gate's destinations */
#define kMaxNavTargets    4       /* navTargetSprite[] */
#define kMaxNebulae       0x20
#define kMaxOutfits         0x200
#define kMaxPersons       0x400
#define kMaxPods         32
#define kMaxRanks                   0x80
#define kMaxShipTypes    0x300 /* shipClass entries */
#define kMaxShots               0x80  /* shotData stride 0x48, 0x80 slots */
#define kMaxStars        20
#define kMaxSystems           0x800
#define kMaxWeapTypes           256     /* weaponStats entries, and ship->weap[] is indexed the same way */
#define kMinPilotVersion 0x12c
#define kMissionVenues           2
#define kMurkFull               32      /* fully lit hilite value */
#define kNavHyperLabel      0x159   /* "jumping to:" */
#define kNavNoDestination   0x156   /* "no destination" */
#define kNavRect  4
#define kNavStellarLabel    0x157   /* "landing on:" */
#define kNavUnexplored      0x15a   /* "unexplored" */
#define kNavUnknown         0x158   /* "unknown" */
#define kParticleColor          0x20
#define kParticleLifeScale      1.25
#define kParticleSpread         0x14
#define kPilotBetaFile     (-45)
#define kPilotCryptKey 0xb36a210fUL
#define kPilotDirList 0x82
#define kPilotHasJunk      (-46)
#define kPilotOK             0
#define kPilotUnreadable   (-43)
#define kPilotWrongVersion (-42)
#define kPortButtons        7
#define kPortPriceScale 0.01
#define kQuarterCircle          0x5a
#define kScrollToEnd  9999
#define kStatusDestroyed    0x18c
#define kStatusDisabled     0x15b
#define kStatusHostile      0xa9
#define kStatusNeutral      0xa8
#define kStatusNoTarget     0x15d /* "no target" */
#define kStatusNoWeapon     0x15e    /* "no weapon selected" */
#define kStatusVulnerable   0x15c
#define kStellarSprites     0x10
#define kTargRect  6
#define kVoiceMessages           3
#define kWeapRect   5


#define kVoiceSets               8


#define kMaxStellars        0x800
#define kFullCircle         360     /* degrees; the float form is kFullCircleF */
#define kFullCircleF        360.0f
#define kSpawnReserve       8       /* ship slots a spawner leaves free */
#define kDefaultOutfitSlots 8       /* shipClass[].defOutfID / defOutfCount entries */
#define kInfluenceLayers    4       /* the influence grid's last dimension */
#define kStatusBarWidth     194     /* the status bar at the screen's right edge */
#define kGridCellWidth      83      /* the outfitter and shipyard grid (tradedlg.cpp) */
#define kGridCellHeight     54
#define kGridThumbSize      32
#define kListItemBaseHeight 8       /* list rows; the Windows game had 9, Mac Nova 8 */

#endif /* GAMECONST_H */
