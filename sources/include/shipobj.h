/* shipobj.h -- the runtime ship object (shipData) and the shot record. Notes: Research/notes/shipobj.md */
#include "gametypes.h"
#include "gameconst.h"

#ifndef SHIPOBJ_H
#define SHIPOBJ_H

struct ShipRec;

/* a ship's state for one weapon type: owned count, ammo, reload countdown, shots in the current burst */
typedef struct ShipWeapon {
    short count;
    short ammo;
    float reloadTimer;
    short burstCount;
} ShipWeapon;

typedef struct ShipObj {
    struct SpriteRec *sprite;
    struct SpriteRec *glowSprite;
    struct SpriteRec *lightSprite;
    struct SpriteRec *weapSprite;
    struct SpriteRec *altSprite;
    struct SpriteRec *shieldSprite;
    FloatPoint pos;
    FloatPoint vel;
    FloatPoint formPos;
    float thrust;
    float speedLimit;
    float fuel;
    float deathTimer;
    float skill;
    float heading;
    float accelMag;
    float aiTimer;
    float jumpTimer;
    float shield;
    float armor;
    float ionization;
    float lightLevel;
    float cloak;
    short desiredHeading;
    short navTargetKind;
    short targetStellar;
    short unk6e;
    short targetShip;
    short selectedWeapon;
    short curSystem;
    short type;
    short dude;
    short cargo[6];
    short index;
    short aiType;
    short auxMission;
    short defenseStellar;
    short veerHeading;
    short lastInspected;
    short lastStellar;
    short prevSystem;
    short grudge;
    short govt;
    short parentShip;
    short unk9C;
    short unk9E;
    int32_t credits;
    uint32_t jumpTick;
    uint32_t fireTick;
    uint32_t hailTick;
    uint32_t ionColor;
    uint32_t tractorTick;
    Boolean active;
    Boolean boarded;
    Boolean fireQueued;
    Boolean hired;
    Boolean hailed;
    Boolean hasAfterburner;
    Boolean sellOnLanding;
    Boolean upgradeOnLanding;
    Boolean hasEscorts;
    Boolean hasSwarm;
    Boolean hasFormation;
    Boolean canScoop;
    Boolean escortMayAttack;
    char padC5[3];
    ShipWeapon weap[kMaxWeapTypes];
    short aiState;
    short aiSubstate;
    short aggression;
    short incomingDamage;
    short persType;
    short specialMission;
    short engineGlow;
    short blinkCount;
    short cloakDir;
    short aimWeapon;
    short tractorShip;
    short formerEscort;
    float animTimer;
    float frameTimer;
    float weapGlow;
    float lightTimer;
    float shieldGlow;
    short altSet;
    short animSet;
    short bankDir;
    short foldDir;
    short foldSet;
    short weapGroupLane[4];
    short swarmLeader;
    short escapePods;
    short escortOrder;
    float searchTimer;
    float annoyance;
    float odds;
    float fuelRegenRate;
    short seesCloaked[2];
    short decloaksWhenHit;
    short voiceClass;
    short fuelRegenState;
    short jamming[4];
    short formLeader;

    bool IsPlayer() const { return index == 0; }
    inline ShipRec &Class() const;

    /* shipstats.cpp */
    float ShieldCapacity();
    float ShieldRechargeRate();
    float ArmorCapacity();
    float ArmorRechargeRate();
    short FuelCapacity();
    Boolean FuelGenRate(float *rate);
    float TurnRate();
    float AccelRate();
    float MaxSpeed();
    Boolean HasEscapePod();
    Boolean HasEscapeShip();
    short EscapeShipType();
    short EscapeShipWeapType();
    Boolean KeyCarriedShipTypeOnboard();
    Boolean HasAutoEject();
    Boolean HasAfterburner();
    short ECM(short kind);
    short HyperTransitTime();
    int32_t HyperSafeDist();
    float DeionizationRate();
    float IonizationFactor();
    Boolean CanScoop();
    short JumpsPerJump();
    short MaxWeaponRange();
    Boolean CanExpediteJumps();
    Boolean IsInertialess();
    float AfterburnerFuelUsage();
    short ResistsGravity();
    short ResistsDeadlyStellars();
    Boolean CanSelfRepair();

    /* shipcloak.cpp */
    Boolean VisibleTo(ShipObj *viewer);
    Boolean HasCloak();
    Boolean HasAreaCloak();
    short CloakFuelUsage();
    Boolean CloakZeroesShields();
    Boolean DecloaksWhenHit();
    short CloakShieldUsage();
    Boolean CloakVisibleOnRadar();
    Boolean CanSeeCloakedShips(short which);
    Boolean HasDensityScanner();
    Boolean HasIFF();
    Boolean CanCloak();
    void DoCloak();
    void DoUncloak();
    Boolean IsCloaked();
    Boolean CanTargetUntargetableShips();
    Boolean CanTargetCloakedShips();

    /* combatutils.cpp */
    Boolean IsDisabled();
    unsigned char IsDying();
} ShipObj;

typedef struct ShotObj {
    struct SpriteRec      *sprite;
    FloatPoint             pos;
    FloatPoint             vel;
    float                  life;
    float                  trailTimer;
    float                  frameTimer;
    float                  angle;
    float                  murk;
    short                  weapType;
    short                  target;
    short                  owner;
    short                  curSystem;
    short                  guidanceState;
    short                  trailCount;
    short                  frame;
    short                  subDepth;
    short                  glow[4];
    short                  energy;
    short                  hitRoll;
    unsigned char          disableOnly;
    char pad45[3];
} ShotObj;
extern ShipObj *shipData;
extern short shipSystems[512];

#endif
