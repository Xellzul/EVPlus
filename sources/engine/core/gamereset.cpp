/* gamereset.cpp -- game-object init, player and game reset, default prefs and keys */

#include "game.h"
#include "render.h"
#include "stellar.h"
#include "dialogs.h"
#include "space.h"
#include "prototypes.h"
#include "gameutil.h"
#include "player.h"
#include "ships.h"

#define kEscortCount     4

#define kBasePriceStringID 9300     /* plugin string, else STR# 4004 item i+1 */
#define kBasePriceListID   4004
#define kSystemGovtUnset   (-32767)
#define kRoidPixel         0x1ce7   /* the default asteroid colour, a direct 15-bit pixel */

// FUNCTION: EVNOVA 0x004b0c20
// MAC: _InitObjects i386 0x0001c2e5 ppc 0x000204b8
void InitObjects(void)
{
    short i, j, k;
    Str255 buf;
    int32_t value;

    for (i = 0; i < kMaxShips; i++) {
        shipData[i].index = i;
        shipData[i].heading = 0.0f;
        shipData[i].desiredHeading = 0;
        shipData[i].curSystem = -1;
        shipData[i].type = 0;
        shipData[i].boarded = 0;
        shipData[i].active = 0;
        shipData[i].cloakDir = 0;
        shipData[i].cloak = 0.0f;
        shipData[i].credits = 0;
        shipData[i].defenseStellar = -1;
        shipData[i].unk6e = 1;
        shipData[i].targetShip = -1;
        shipData[i].selectedWeapon = -1;
        shipData[i].shield = 0.0f;
        shipData[i].armor = 0.0f;
        shipData[i].aiType = 0;
        shipData[i].aiTimer = 0.0f;
        shipData[i].navTargetKind = -1;
        shipData[i].targetStellar = -1;
        shipData[i].lastInspected = -1;
        shipData[i].lastStellar = -2;
        shipData[i].jumpTimer = 0.0f;
        shipData[i].fuel = 100.0f;
        shipData[i].parentShip = -1;
        shipData[i].deathTimer = 0.0f;
        shipData[i].ionization = 0.0f;
        shipData[i].ionColor = 0;
        shipData[i].lightLevel = 0;
        shipData[i].blinkCount = 0;
        shipData[i].skill = 1.0f;
        shipData[i].unk9C = 0;
        shipData[i].formerEscort = -1;
        shipData[i].sprite = 0;
        shipData[i].glowSprite = 0;
        shipData[i].weapSprite = 0;
        shipData[i].lightSprite = 0;
        shipData[i].altSprite = 0;
        shipData[i].shieldSprite = 0;
        shipData[i].bankDir = 0;
        shipData[i].foldDir = 0;
        shipData[i].foldSet = 0;
        shipData[i].frameTimer = 0.0f;
        shipData[i].shieldGlow = 0;
        shipData[i].weapGlow = 0;
        shipData[i].annoyance = 0;
        for (j = 0; j < kMaxWeapTypes; j++) {
            shipData[i].weap[j].reloadTimer = 0.0f;
            shipData[i].weap[j].burstCount = 0;
            shipData[i].weap[j].count = 0;
            shipData[i].weap[j].ammo = 0;
        }
        for (j = 0; j <= kCargoTypes - 1; j++)
            shipData[i].cargo[j] = 0;
        shipData[i].sprite = 0;
        shipData[i].altSprite = 0;
        shipData[i].glowSprite = 0;
        shipData[i].lightSprite = 0;
        shipData[i].weapSprite = 0;
        shipData[i].shieldSprite = 0;
    }

    for (i = 0; i < kMaxShipTypes; i++) {
        shipClass[i].techLevel = kResUnloaded;
        shipClass[i].shield = 1;
        shipClass[i].armor = 1;
        shipClass[i].strength = 1;
        shipClass[i].dispWeight = 1;
        shipClass[i].upgradeTo = -1;
        shipClass[i].altSets = 0;
        shipClass[i].baseSets = 0;
        shipClass[i].transparency = 0;
        shipClass[i].keyCarried = -1;
        shipClass[i].shanFlags = 0;
        shipClass[i].contributes[0] = 0;
        shipClass[i].contributes[1] = 0;
        shipClass[i].spriteIndex = i;
        shipClass[i].mass = -1;
        shipClass[i].hireRandom = 100;
        shipClass[i].buyRandom = 100;
        shipClass[i].dailyHireRoll = 50;
        shipClass[i].dailySaleRoll = 50;
        for (j = 0; j < kMaxWeapTypes; j++) {
            shipClass[i].weapCount[j] = 0;
            shipClass[i].weapAmmo[j] = 0;
        }
        for (j = 0; j < 4; j++) {
            for (k = 0; k < 4; k++) {
                shipClass[i].weapPos[j][k].h = 0;
                shipClass[i].weapPos[j][k].v = 0;
                shipClass[i].weapPosZ[j][k] = 0;
            }
        }
        for (j = 0; j < 2; j++) {
            shipClass[i].compress[j].x = 1.0f;
            shipClass[i].compress[j].y = 0.71f;
        }
        shipClass[i].frameAngles = 0;
        masterShipSprite[i] = 0;
        masterShipGlowSprite[i] = 0;
        masterShipLightSprite[i] = 0;
        masterShipWeaponSprite[i] = 0;
        masterShipAltSprite[i] = 0;
        masterShipShieldSprite[i] = 0;
    }

    for (i = 0; i < kMaxOutfits; i++) {
        outfitSystem[i].techLevel = kOutfTechAbsent;
        outfitSystem[i].dispWeight = 0;
        outfitSystem[i].contributes[0] = 0;
        outfitSystem[i].contributes[1] = 0;
        outfitSystem[i].requires[0] = 0;
        outfitSystem[i].requires[1] = 0;
        outfitSystem[i].itemClass = -1;
        outfitSystem[i].scanMask = 0;
        outfitSystem[i].buyRandom = 0;
        outfitSystem[i].dailyRoll = 50;
        outfitSystem[i].sameNameIndex = i;
        outfitSystem[i].govtRestrict = -1;
    }

    for (i = 0; i < kMaxStellars; i++) {
        stellarData[i].index = i;
        stellarData[i].loaded = 0;
        stellarData[i].prepared = 0;
        stellarData[i].type = 0;
        stellarData[i].techLevel = 0;
        stellarData[i].dominated = 0;
        stellarData[i].daysDominated = 0;
        stellarData[i].system = -1;
        stellarData[i].numFrames = 1;
        stellarData[i].curFrame = 0;
        stellarData[i].frameTimer = 0;
        stellarData[i].gravity = 0;
        stellarData[i].deadDays = 0;
        stellarData[i].frame0Bias = 1;
        stellarData[i].reloadTimer = 0;
        stellarData[i].burstShots = 0;
        stellarData[i].deadCounter = 0;
        stellarData[i].strength = 0;
        stellarData[i].deadDays = 0;
        stellarData[i].deadDaysLeft = 0;
        stellarData[i].tribute = 1000;
        stellarData[i].landingFee = 0;
        for (j = 0; j < kSpobHyperLinks; j++)
            stellarData[i].hyperLinks[j] = -1;
        for (j = 0; j < 8; j++)
            stellarData[i].specialTech[j] = 0;
    }

    for (i = 0; i < kMaxSystems; i++) {
        systemData[i].loaded = 0;
        systemData[i].prepared = 0;
        systemData[i].govt = kSystemGovtUnset;
        systemData[i].asteroidCount = 0;
        systemData[i].astTypes = 0;
        systemData[i].interference = 0;
        systemData[i].exploreLevel = 0;
        systemData[i].govt = -1;
        systemData[i].pos.h = systemData[i].pos.v = 0;
        for (j = 0; j < kMaxSystemLinks; j++)
            systemData[i].links[j] = -1;
        for (j = 0; j < kNavSlots; j++)
            systemData[i].navDefault[j] = -1;
        for (j = 0; j < 8; j++) {
            systemData[i].fleets[j] = -1;
            systemData[i].dudeTypes[j] = -1;
            systemData[i].dudeProbs[j] = 0;
        }
        systemData[i].fleetCount = 0;
        systemData[i].fleetProbTotal = 0;
        systemData[i].known = 0;
        systemData[i].colocNext = -1;
        systemData[i].colocGroup = -1;
        systemData[i].rgb.red = 0;
        systemData[i].rgb.green = 0;
        systemData[i].rgb.blue = 0;
        systemData[i].murk = 0;
        systemData[i].reinforceFleet = -1;
        systemData[i].reinforceTime = 0;
        systemData[i].reinforceDaysLeft = 0;
        systemData[i].reinforceInterval = 0;
        systemData[i].reinforceTimer = 0.0f;
        strncpy((char *)systemData[i].name, (char *)"\p", 31);
    }

    for (i = 0; i < kMaxDudes; i++) {
        dudeData[i].loaded = 0;
        for (j = 0; j < kDudeShipSlots; j++)
            dudeData[i].shipTypes[j] = -1;
    }

    for (i = 0; i < kMaxPersons; i++) {
        personData[i].loaded = 0;
        personData[i].canAppear = 0;
        personData[i].prepared = 0;
        personData[i].linkSyst = 0;
        personData[i].grantClass = -1;
        personData[i].grantProb = 0;
        personData[i].grantCount = 0;
        for (j = 0; j < kMaxWeapTypes; j++) {
            personData[i].weapCount[j] = 0;
            personData[i].weapAmmo[j] = 0;
        }
        personData[i].subtitle[0] = 0;
    }

    for (i = 0; i < kMaxFleets; i++) {
        fleetData[i].leadShipType = -1;
        fleetData[i].govt = -1;
        fleetData[i].usable = 0;
    }

    for (i = 0; i < kMaxRanks; i++) {
        rankData[i].index = i;
        rankData[i].active = 0;
        rankData[i].govt = -1;
        rankData[i].flags = 0;
    }

    for (i = 0; i < kMaxAsteroids; i++)
        asteroidData[i].active = 0;

    for (i = 0; i < kMaxRoidTypes; i++) {
        asteroidType[i].strength = 100;
        asteroidType[i].yieldQty = 0;
        asteroidType[i].yieldType = -1;
        asteroidType[i].fragType[0] = -1;
        asteroidType[i].fragType[1] = -1;
        asteroidType[i].fragCount = 0;
        asteroidType[i].partCount = 50;
        asteroidType[i].frameRate = 1.0f;
        asteroidType[i].color = kRoidPixel;
    }

    for (i = 0; i < kMaxNebulae; i++) {
        nebulaData[i].loaded = 0;
        nebulaData[i].explored = 0;
    }

    for (i = 0; i < kMaxJunks; i++) {
        junkData[i].scanMask = 0;
        for (j = 0; j < 8; j++) {
            junkData[i].boughtAt[j] = -1;
            junkData[i].soldAt[j] = -1;
            pstrcpy(junkData[i].name, "\p");
            pstrcpy((unsigned char *)junkData[i].abbrev, "\p");
            pstrcpy((unsigned char *)junkData[i].lcName, "\p");
        }
    }

    for (i = 0; i < kMaxShots; i++) {
        shotData[i].weapType = 0;
        shotData[i].life = -2.0f;
    }

    for (i = 0; i < kMaxBoxes; i++)
        boxData[i].timer = -2.0f;

    for (i = 0; i < kMaxStars; i++) {
        starData[i].live = 0;
        starData[i].frame = 0;
        starData[i].depth = 1.0f;
    }

    for (i = 0; i < kMaxPods; i++)
        podData[i].life = -1.0f;

    for (i = 0; i < kMaxBeams; i++) {
        beamData[i].state = -2;
        beamData[i].owner = -1;
    }

    for (i = 0; i < kMaxWeapTypes; i++) {
        weaponStats[i].guidance = kResAbsent;
        for (j = 0; j < 4; j++)
            weaponStats[i].jamVuln[j] = 0;
    }

    for (i = 0; i < kEscortCount; i++)
        playerEscortOrders[i] = -1;

    for (i = 0; i < kMaxHyperRoute; i++)
        hyperRoute[i] = -1;

    for (i = 0; i <= kCargoTypes - 1; i++) {
        if (!LoadPluginString(i + kBasePriceStringID, buf))
            GetIndString(buf, kBasePriceListID, i + 1);
        StringToNum(buf, &value);
        basePrice[i] = value;
    }

    for (i = 0; i < kMaxMissions; i++)
        missionRandom[i] = Rand(kDailyRollRange) + 1;

    for (i = 0; i < kMaxCrons; i++)
        cronEvent[i].loaded = 0;

    for (i = 0; i < kMaxDisasters; i++)
        disasterData[i].loaded = 0;

    for (i = 0; i < kMaxGovts; i++) {
        for (j = 0; j < 4; j++) {
            govtData[i].allies[j] = -1;
            govtData[i].enemies[j] = -1;
            govtData[i].classes[j] = -1;
        }
        for (j = 0; j < 4; j++)
            govtData[i].inhJam[j] = 0;
        govtData[i].maxOdds = 2.0f;
        govtData[i].skillMult = 1.0f;
        govtData[i].scanFine = 0;
        govtData[i].iffScrambled = 0;
        govtData[i].reinfInhibited = 0;
        govtData[i].voiceType = -1;
        govtData[i].voiceClass = -1;
        govtData[i].interfaceID = -1;
        govtData[i].loaded = 0;
    }

    strictPlay = 1;
    allReinforcementsInhibited = 0;
    alertSpritePending = -1;
    dockPermission = dockTarget = -1;
    GetTime(&gameTime);
    gameTime.year += kGameYearOffset;
    pstrcpy(lastPlayerName, "\p");
    pstrcpy(playerName, "\pNo Name");
    pstrcpy(shipName, "\pNo Name");
    descResourcePictId = 0;
    pstrcpy(descMovieName, "\p");
    leetMode = 0;
    escortMenuOnscreen = 0;
    showFloatingMap = -1;
    showFPS = 0;
    playerSabotaged = 0;
    recentActivatedRank = -1;
    lastCursorAnimTime = 0;
    lastFrameTime = 0;
    Microseconds(&lastFrameMicroseconds);
    SetRect(&statusOffRect[0], 30, 8, 163, 142);
    SetRect(&statusOffRect[1], 99, 155, 173, 161);
    SetRect(&statusOffRect[2], 99, 170, 173, 176);
    SetRect(&statusOffRect[3], 99, 185, 173, 191);
    SetRect(&statusOffRect[4], 9, 204, 184, 239);
    SetRect(&statusOffRect[5], 9, 251, 184, 268);
    SetRect(&statusOffRect[6], 9, 280, 184, 395);
    SetRect(&statusOffRect[7], 9, 407, 184, 495);
    SetRect(&progressDefRect, -95, -15, 95, 15);
    statusColor[0].red = 0xffff;
    statusColor[0].green = 0xffff;
    statusColor[0].blue = 0xffff;
    statusColor[1].red = 0x7fff;
    statusColor[1].green = 0x7fff;
    statusColor[1].blue = 0x7fff;
    statusColor[2].red = 0xffff;
    statusColor[2].green = 0xffff;
    statusColor[2].blue = 0xffff;
    statusColor[3].red = 0x7fff;
    statusColor[3].green = 0x7fff;
    statusColor[3].blue = 0x7fff;
    statusColor[4].red = 0xbd00;
    statusColor[4].green = 0;
    statusColor[4].blue = 0;
    statusColor[5].red = 0xa600;
    statusColor[5].green = 0xa600;
    statusColor[5].blue = 0xa600;
    statusColor[6].red = 0x4c00;
    statusColor[6].green = 0x5c00;
    statusColor[6].blue = 0x6f00;
    statusColor[7].red = 0x3c00;
    statusColor[7].green = 0x4900;
    statusColor[7].blue = 0x5800;
    menuTextColor[0].red = 0xffff;
    menuTextColor[0].green = 0;
    menuTextColor[0].blue = 0;
    menuTextColor[1].red = 0x84d0;
    menuTextColor[1].green = 0;
    menuTextColor[1].blue = 0;
    statusFontID = 0;
    statusFontSize[0] = 11;
    menuFontID = 3;
    menuTextSize = 9;
    playerIsLanded = 0;
    ignoreKeys = 0;
    GetFNum("\pGeneva", &smallTextFont);
    smallTextSize = 9;
    hypergateMapMode = 0;
    hypergateMapModeStellar = -1;
}

#define kStartingCredits  10000
#define kAdviceRange      0x800     /* Rand() range for the opening advice pick */
#define kPriceFluxBase    100       /* 100% == no price movement yet */
#define kNewbieUnset      0x7fff
#define kPlayerPaintLevel 0x20      /* the default ship tint, applied to all three guns */

#define kStartY           50.0f
#define kNoDeathTimer     (-999.0f)
#define kFreeSlot         (-1.0f)   /* boom/debris timer: < 0 means unused */

// FUNCTION: EVNOVA 0x004b3350
// MAC: _ResetPlayer i386 0x0001d40d ppc 0x000219c8
void ResetPlayer(Boolean newPilot)
{
    short i;
    short link;
    short coloc;
    short j;

    shipData->pos.x = shipData->pos.y = kStartY;
    shipData->vel.x = shipData->vel.y = 0.0f;
    shipData->accelMag = 0.0f;
    shipData->heading = 0.0f;
    shipData->type = 0;
    shipData->active = 1;
    shipData->cloakDir = 0;
    shipData->cloak = 0.0f;
    shipData->navTargetKind = -1;
    shipData->govt = -1;
    shipData->aiType = -1;
    shipData->curSystem = 0;
    shipData->shield = ShipShieldCapacity(shipData);
    shipData->armor = ShipArmorCapacity(shipData);
    shipData->fuel = ShipFuelCapacity(shipData);
    shipData->targetShip = -1;
    shipData->targetStellar = -1;
    shipData->navTargetKind = -1;
    shipData->selectedWeapon = -1;
    shipData->parentShip = -1;
    shipData->specialMission = -1;
    shipData->persType = -1;
    shipData->aiState = shipData->aiSubstate = kSubLoJack;
    shipData->deathTimer = kNoDeathTimer;
    shipData->ionization = 0.0f;
    shipData->ionColor = 0;
    shipData->lightLevel = 0.0f;
    shipData->blinkCount = 0;
    shipData->altSet = 0;
    shipData->animSet = 0;
    shipData->foldDir = 1;
    shipData->foldSet = 0;
    shipData->frameTimer = 0.0f;
    shipData->bankDir = 0;
    shipData->shieldGlow = 0;
    shipData->weapGlow = 0;
    shipData->annoyance = 0.0f;
    shipData->animTimer = 0.0f;
    shipData->skill = 1.0f;
    shipData->jumpTimer = 0.0f;
    shipData->prevSystem = -1;
    shipData->engineGlow = 0;
    shipData->fireTick = 0;
    shipData->tractorShip = -1;
    shipData->tractorTick = 0;

    for (i = 0; i < kMaxWeapTypes; i++) {
        shipData->weap[i].count = 0;
        shipData->weap[i].ammo = 0;
    }
    for (i = 0; i < kMaxWeapTypes; i++) {
        shipData->weap[i].count = shipClass[shipData->type].weapCount[i];
        if (shipClass[shipData->type].weapAmmo[i] > 0) {
            if (weaponStats[i].ammoType >= 0 && weaponStats[i].ammoType < kMaxWeapTypes)
                shipData->weap[weaponStats[i].ammoType].ammo = shipClass[shipData->type].weapAmmo[i];
            else
                shipData->weap[i].ammo = shipClass[shipData->type].weapAmmo[i];
        }
    }
    for (i = 0; i < kMaxHyperRoute; i++)
        hyperRoute[i] = -1;
    for (i = 0; i < kMaxOutfits; i++) {
        if (newPilot || outfitSystem[i].persistent == 0)
            shipSystems[i] = 0;
    }
    for (i = 0; i <= kCargoTypes - 1; i++)
        shipData->cargo[i] = 0;
    for (i = 0; i < kMaxJunks; i++)
        junkData[i].held = 0;

    antiEscortDelay = -1;
    lastMissionStellar = -1;
    shipData->escapePods = -1;
    destructCount = kFreeSlot;
    dockTarget = -1;
    dockPermission = -1;
    adviceRandom = Rand(kAdviceRange);
    bribeRandom = -1;
    playerThreatened = oldPlayerThreatened = 0;
    fadePending[0] = fadePending[1] = 0;
    newbieFlag = kNewbieUnset;
    alertSpritePending = -1;

    for (i = 0; i < kMaxMissions; i++)
        missionRandom[i] = Rand(kDailyRollRange) + 1;
    for (i = 0; i < kMaxExplods; i++) {
        explodData[i].timer = kFreeSlot;
        explodData[i].delay = 0.0f;
    }
    for (i = 0; i < kMaxDebris; i++) {
        smokeData[i].timer = kFreeSlot;
        smokeData[i].angle = 0.0f;
    }
    for (i = 0; i < kMaxShipTypes; i++) {
        shipClass[i].dailyHireRoll = Rand(kDailyRollRange) + 1;
        shipClass[i].dailySaleRoll = Rand(kDailyRollRange) + 1;
    }
    for (i = 0; i < kMaxOutfits; i++)
        outfitSystem[i].dailyRoll = Rand(kDailyRollRange) + 1;

    killShots = 1;
    killBoxes = 1;
    killExplods = 1;
    killSmoke = 1;
    weapLawFlag = 0;
    smuggleFlag = 0;
    greenMissionSyst = -1;
    commCount = 0;
    missionSoundID = -1;

    for (j = 0; j < kMaxActiveMissions; j++) {
        if (currentMissionObjectives[j].active != 0)
            AbortMission(j, 0);
        currentMissionObjectives[j].active = 0;
    }
    for (j = 0; j < kMaxMissions; j++)
        missionRandom[j] = Rand(kDailyRollRange) + 1;

    for (i = 0; i <= 1; i++) {
        globalPriceFlux[i] = kPriceFluxBase;
        localPriceFlux[i] = kPriceFluxBase;
    }

    if (newPilot) {
        shipData->credits = kStartingCredits;
        for (i = 0; i < kMaxSystems; i++) {
            systemData[i].exploreLevel = 0;
            if (systemData[i].govt >= 0 && systemData[i].govt < kMaxGovts)
                playerRecord[i] = govtData[systemData[i].govt].initialRec;
            else
                playerRecord[i] = 0;
            systemData[i].reinforceDaysLeft = 0;
            systemData[i].reinforceTimer = 0.0f;
        }
        for (i = 0; i < kNavSlots; i++) {
            if (systemData[shipData->curSystem].links[i] != -1 && systemData[systemData[shipData->curSystem].links[i]].loaded != 0) {
                link = systemData[shipData->curSystem].links[i];
                systemData[link].exploreLevel = 1;
                coloc = FindFirstCoLocatedSystem(link);
                if (coloc >= 0 && coloc < kMaxSystems)
                    systemData[coloc].exploreLevel = 1;
            }
        }
        for (i = 0; i < kMaxNebulae; i++)
            nebulaData[i].explored = 0;
        for (i = 0; i < kMaxPersons; i++)
            personData[i].grudge = 0;
        for (i = 0; i < kEscortCount; i++)
            playerEscortOrders[i] = -1;
    }
    mapScroll.h = systemData[shipData->curSystem].pos.h;
    mapScroll.v = systemData[shipData->curSystem].pos.v;
    systemData[shipData->curSystem].reinforceTimer = kFreeSlot;
    systemData[shipData->curSystem].reinforceDaysLeft = 0;
    lastMissionStellar = -1;
    recentActivatedRank = -1;
    playerPaint.red = playerPaint.green = playerPaint.blue =
        kPlayerPaintLevel;
    ResetPlayerPrecalcedValues();
}

// FUNCTION: EVNOVA 0x004b4220
// MAC: _ResetPlayerRecord i386 0x0001db70 ppc 0x0002231c
void ResetPlayerRecord(Boolean newPilot)
{
    short i;

    if (newPilot != 0)
        playerKills = 0;

    for (i = 0; i < kMaxSystems; i++) {
        short minStatus;

        if (systemData[i].govt >= 0)
            minStatus = govtData[systemData[i].govt].initialRec;
        else
            minStatus = 0;
        if (playerRecord[i] < minStatus)
            playerRecord[i] = minStatus;
    }
    for (i = 0; i < kMaxStellars; i++) {
        if ((stellarData[i].flags2 & kSpobAlwaysDominated) != 0)
            stellarData[i].dominated = 1;
        else
            stellarData[i].dominated = 0;
    }
    recentActivatedRank = 0xffff;
}

/*
 * No mac twin: 1.1.1 rewrote its preferences layer for OS X (_LoadPrefs is 2243
 * bytes and EVPrefsDialogController is Objective-C), so these two names
 * describe the body.
 */

// FUNCTION: EVNOVA 0x004b4320
void SetDefaultPrefs(void)
{
    settings.introMusic = 1;
    settings.voicesOn = 1;
    settings.volume = 5;
    settings.clipping = 0;
    settings.noMovies = 0;
    settings.noDebris = 0;
    settings.windowed = 0;
    settings.yieldEvents = 1;
    settings.unused = 0;
    settings.shipAnimations = 1;
    settings.engineGlows = 1;
    settings.parallaxStars = 1;
    settings.ambientSounds = 1;
    settings.noHyperGamma = 0;
    settings.showMapBorders = 0;
    settings.weaponGlows = 1;
    settings.runningLights = 1;
    settings.gamma = 3;
    settings.gameWidth = 0;
    settings.gameHeight = 0;
    settings.scaleFilter = kScaleFilterLinear;
    settings.wholePixelScaling = 0;
    settings.stereoPan = 1;
    settings.asteroidScaling = 1;
    settings.mapPrices = 0;
    settings.mapDisasters = 0;
    settings.originalSound = 0;
    settings.uiScale = 1.0;
    pstrcpy(lastPlayerName, "\p");
}

// FUNCTION: EVNOVA 0x004b4400
void SetDefaultKeys(void)
{
    keyPrefs.selectSecondary = kScanW;
    keyPrefs.deselectSecondary = kScanS;
    keyPrefs.firePrimary = kScanSpace;
    keyPrefs.fireSecondary = kScanLCtrl;
    keyPrefs.hail = kScanY;
    keyPrefs.land = kScanL;
    keyPrefs.dismiss = kScanReturn;
    keyPrefs.faceTarget = kScanA;
    keyPrefs.clearTarget = kScanN;
    keyPrefs.map = kScanM;
    keyPrefs.targetNext = kScanGrave;
    keyPrefs.targetNearest = kScanR;
    keyPrefs.hyperspace = kScanH;
    keyPrefs.nextSystem = kScanBackslash;
    keyPrefs.jump = kScanJ;
    keyPrefs.board = kScanB;
    keyPrefs.turnLeft = kScanLeft;
    keyPrefs.turnRight = kScanRight;
    keyPrefs.accelerate = kScanUp;
    keyPrefs.reverse = kScanDown;
    keyPrefs.pause = kScanEscape;
    keyPrefs.afterburner = kScanZ;
    keyPrefs.playerInfo = kScanP;
    keyPrefs.navPreset[0] = kScanF1;
    keyPrefs.navPreset[1] = kScanF2;
    keyPrefs.navPreset[2] = kScanF3;
    keyPrefs.navPreset[3] = kScanF4;
    keyPrefs.cheatSpawn = kScanInsert;
    keyPrefs.cheatKill = kScanDelete;
    keyPrefs.cheatRevive = kScanHome;
    keyPrefs.cheatArmor = kScanNone;
    keyPrefs.cheatDamage = kScanNone;
    keyPrefs.cheatAmmo = kScanF6;
    keyPrefs.cheatCredits = kScan4;
    keyPrefs.cheatSpare = kScanEnd;
    keyPrefs.cheatShipNext = kScanPageUp;
    keyPrefs.cheatShipPrev = kScanPageDown;
    keyPrefs.missions = kScanI;
    keyPrefs.cloak = kScanU;
    keyPrefs.toggleFPS = kScanF12;
    keyPrefs.escortMenu = kScanE;
    keyPrefs.jettison = kScanK;
    keyPrefs.eject = kScanX;
    keyPrefs.selfDestruct = kScanMinus;
    keyPrefs.quickKey[0] = kScan1;
    keyPrefs.quickKey[1] = kScan2;
    keyPrefs.quickKey[2] = kScan3;
    keyPrefs.quickKey[3] = kScan4;
    keyPrefs.quickKey[4] = kScan5;
    keyPrefs.escortOrder[0] = kScanF;
    keyPrefs.escortOrder[1] = kScanD;
    keyPrefs.escortOrder[2] = kScanV;
    keyPrefs.escortOrder[3] = kScanC;
    keyPrefs.speedUp = kScanCapsLock;
}

// FUNCTION: EVNOVA 0x004b4690
// MAC: _ResetGameData i386 0x0001de0c ppc 0x00022640
void ResetGameData(void)
{
    short i;

    for (i = 0; i < kMaxPersons; i++) {
        if (personData[i].aiType >= 1) {
            personData[i].loaded = 1;
            personData[i].grudge = 0;
        }
    }
    for (i = 0; i < kMaxStellars; i++) {
        if ((stellarData[i].flags2 & kSpobAlwaysDominated) != 0)
            stellarData[i].dominated = 1;
        else
            stellarData[i].dominated = 0;
        if ((stellarData[i].flags2 & kSpobStartsDestroyed) != 0) {
            stellarData[i].deadCounter = -1;
            if (stellarData[i].deadDays >= 0)
                stellarData[i].deadDaysLeft = stellarData[i].deadDays;
            else
                stellarData[i].deadDaysLeft = 1;
        } else {
            stellarData[i].deadCounter = stellarData[i].strength;
            stellarData[i].deadDaysLeft = 0;
        }
    }
    for (i = 0; i < kMaxMissionBits; i++)
        missionBit[i] = 0;
    for (i = 0; i < kMaxDisasters; i++)
        disasterData[i].countdown = 0;
    for (i = 0; i < kMaxBeams; i++)
        beamData[i].state = -2;
    for (i = 0; i < kMaxShots; i++)
        shotData[i].life = -2.0f;
    for (i = 0; i < kMaxBoxes; i++)
        boxData[i].timer = -2.0f;
    for (i = 0; i < kMaxNebulae; i++) {
        nebulaData[i].explored = 0;
        nebulaData[i].loaded = 0;
    }
    RandomizeStars();
    dockPermission = dockTarget = -1;
    GetTime(&gameTime);
    gameTime.year += kGameYearOffset;
    pstrcpy(lastPlayerName, "\p");
    pstrcpy(playerName, "\pNo Name");
    pstrcpy(shipName, "\pNo Name");
    lastShipAIDispatched = 1;
    hypergateMapMode = 0;
    seenIntroScreen = 0;
}
