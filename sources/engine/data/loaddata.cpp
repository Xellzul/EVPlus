/* loaddata.cpp -- pilot-folder setup, OpenDataFiles, LoadObjectData, LoadStrings, LoadPrefs/SavePrefs, the pilot file, the resource utilities, the chär record readers. Notes: Research/notes/loaddata.md */

#include "pixel.h"
#include "render.h"
#include "crashlog.h"
#include "player.h"
#include "dialogs.h"
#include "game.h"

/* a resource handle's record: 1.0.10 casts the handle, an array field then indexes off the record */
#define ResRec(T, h) ((T *)*(h))
#define ClampRGB(c) \
    if ((c).red > 0xffff) (c).red = 0xffff; \
    if ((c).green > 0xffff) (c).green = 0xffff; \
    if ((c).blue > 0xffff) (c).blue = 0xffff
#include "gameutil.h"
#include "stellar.h"
#include "platform.h"
#include "sound.h"
#include "ships.h"
#include "ai.h"
#include "prototypes.h"
#include "endianswap.h"
#include "gamepaths.h"
#include "text.h"

#define kPrefStringMax 0x100

#define kSpobResName "sp\x9a" "b"
#define kSystResName "s\xd8" "st"
#define kShipResName "sh\x95" "p"
#define kOutfResName "o\x9f" "tf"
#define kDudeResName "d\x9f" "de"
#define kPersResName "p\x91" "rs"

#define kMacPlugInsFolder   "Plug-Ins"  /* the other name of the plug-ins folder */

/* A plain record starts with a stellar index, which is below kMaxStellars; anything else
   is still encrypted. */
static int32_t DecryptIfNecessary(void *buf, uint32_t len, uint32_t key)
{
    if (*(unsigned short *)buf >= kMaxStellars)
        return DoEncryption(buf, len, key);
    return 0;
}

/* No Nova Plug-Ins: try "Plug-Ins"; paths.plugins is the one found. */
static short OpenPlugInsFallback(const char *ext)
{
    std::string mac;

    if (OpenEVPlugins(paths.plugins.c_str(), ext, NULL) == 0)
        return 0;
    mac = JoinPath(paths.root, kMacPlugInsFolder);
    if (OpenEVPlugins(mac.c_str(), ext, NULL) != 0)
        return -1;
    paths.plugins = mac;
    return 0;
}

/*
 * The pilots folder (STR# 130, 3) in the game's folder (win 0x004bd0c0); the two debug switches are read
 * here, the first thing after the resource file opens.
 */

// FUNCTION: EVNOVA 0x004bd0c0
// MAC: _InitDataFiles i386 0x0007138b ppc 0x00079dc4
void InitDataFiles(void)
{
    char name[kPrefStringMax];

    noSavePilotFiles = EVBugResourceFieldTest(12);
    noUpdatePilotData = EVBugResourceFieldTest(13);
    name[0] = 0;
    StrcatIndCString(name, kPilotDirList, 3);
    paths.pilots = JoinPath(paths.root, name);
    lastIntfResourceID = -1;
}

/* prints/logs the Pascal string GetIndString just filled; NOT the same
   address as crashlog_print_pstr (0x00470150) despite the similar role */
/* called with a constant 0 at every site here; tail-calls through a lazy
   singleton getter (0x52ce50) into 0x4fdf10 */


// FUNCTION: EVNOVA 0x004bd160
// MAC: _OpenDataFiles i386 0x00076a76 ppc 0x0007f85c
void OpenDataFiles(void)
{
    unsigned char str[kPrefStringMax];
    Handle h;
    short count;

    if (!PlatMakeFolder(paths.pilots.c_str())) {
        FadeScreenIn();
        GetIndString(str, kAlertStrList, 0x1d);
        EVNoteAlert(str);
        ExitToShell();
    }

    crashlog_print("opening files in ");
    crashlog_print("Nova Files");
    crashlog_print(" folder:\n");

    if (OpenEVPlugins(paths.data.c_str(), ".rez", NULL) != 0) {
        FadeScreenIn();
        GetIndString(str, kAlertStrList, 0x1e);
        EVNoteAlert(str);
        ExitToShell();
    }

    {
        if (pluginCount > 0) {
            crashlog_print("opening files in ");
            crashlog_print("Nova Plug-Ins");
            crashlog_print(" folder:\n");
        }
        if (OpenPlugInsFallback(".rez") != 0) {
            PlatMakeFolder(paths.plugins.c_str());
            if (OpenEVPlugins(paths.plugins.c_str(), ".rez", NULL) != 0) {
                FadeScreenIn();
                GetIndString(str, kAlertStrList, 0x1f);
                EVNoteAlert(str);
                ExitToShell();
            }
        }
    }

    if (crashlog_isactive()) {
        h = GetResource(kStringListType, kGameStrList);
        if (h != NULL) {
            count = SwapEndianShort(&ResRec(StringListRec, h)->count);
            ReleaseResource(h);
            if (count != 0x18c) {
                crashlog_print("warning: STR# resource ");
                crashlog_print_num(kGameStrList);
                crashlog_print(" is missing strings (");
                crashlog_print_num(0x18c - count);
                crashlog_print(")\n");
            }
        }
    }
}

/* the mac keeps a resource unpurgeable while it is read and purgeable again after; the port's
   resources never purge */
#define ResourceNoPurge(h)
#define ResourcePurge(h)
#define LoadingIntroMusic() HandleIntroMusic(0)

/* a resource ID to its table index; -1 below the first ID */
static inline short ResIndex(short id)
{
    return id >= kFirstResourceID ? id - kFirstResourceID : -1;
}

static void LoadStellars(unsigned char *name)
{
    short j;
    short i;
    short found;
    short resID;
    uint32_t resType;
    short count;

    found = 0;
    count = CountResources(kSpobType);
    if (count == 0)
        crashlog("warning: no " kSpobResName " resources found");
    for (i = 0; i < kMaxStellars; i++) {
        stellarData[i].loaded = 0;
        stellarFileHand = GetResource(kSpobType, i + kFirstResourceID);
        if (stellarFileHand != NULL) {
            LockAHandle(stellarFileHand);
            ResourceNoPurge(stellarFileHand);
            found++;
            const SpobRes &res = *ResRec(SpobRes, stellarFileHand);
            GetResInfo(stellarFileHand, &resID, &resType, name);
            TrimResourceName(name);
            CopyPascalName(stellarData[i].name, name, 31);
            stellarData[i].pos.h = res.xPos;
            stellarData[i].pos.v = res.yPos;
            stellarData[i].type = res.type;
            stellarData[i].flags = res.flags;
            stellarData[i].flags2 = res.flags2;
            stellarData[i].govt = res.govt;
            stellarData[i].minStatus = res.minStatus;
            stellarData[i].techLevel = res.techLevel;
            stellarData[i].customPict = res.custPicID;
            stellarData[i].customSound = res.custSndID;
            stellarData[i].landingFee = res.fee;
            stellarData[i].gravity = res.gravity;
            stellarData[i].weapon = res.weapon;
            stellarData[i].govt = ResIndex(stellarData[i].govt);
            stellarData[i].strength = res.strength;
            stellarData[i].deadCounter = stellarData[i].strength;
            stellarData[i].deadType = res.deadType;
            stellarData[i].deadDays = res.deadTime;
            stellarData[i].deadDaysLeft = 0;
            stellarData[i].explodType = res.explodType;
            if (stellarData[i].explodType < 0)
                stellarData[i].explodType = -1;
            if (stellarData[i].deadType < 0 || stellarData[i].deadType >= kMasterStellarSprites)
                stellarData[i].deadType = stellarData[i].type;
            CopyCField(stellarData[i].onDestroy, res.onDestroy);
            CopyCField(stellarData[i].onRegen, res.onRegen);
            for (j = 0; j < 3; j++)
                stellarData[i].specialTech[j] = res.specialTech[j];
            for (j = 0; j < 5; j++)
                stellarData[i].specialTech[j + 3] = res.specialTech2[j];
            stellarData[i].loaded = 1;
            stellarData[i].prepared = 1;
            stellarData[i].dominated = 0;
            stellarData[i].tribute = res.tribute;
            if (stellarData[i].tribute <= 0)
                stellarData[i].tribute = stellarData[i].techLevel * 1000;
            stellarData[i].defCount = res.defCount;
            stellarData[i].defenseDude = res.defenseDude;
            stellarData[i].animDelay = res.animDelay;
            stellarData[i].frame0Bias = res.frame0Bias;
            for (j = 0; j < kSpobHyperLinks; j++) {
                stellarData[i].hyperLinks[j] = res.hyperLink[j];
                stellarData[i].hyperLinks[j] = ResIndex(stellarData[i].hyperLinks[j]);
            }
            if (stellarData[i].defCount > 1000) {
                if (stellarData[i].defCount > 10000)
                    stellarData[i].defWaveSize = stellarData[i].defCount / 10 - 1000;
                else
                    stellarData[i].defWaveSize = stellarData[i].defCount / 10 - 100;
            } else
                stellarData[i].defWaveSize = stellarData[i].defCount;
            stellarData[i].defenseDude = ResIndex(stellarData[i].defenseDude);
            if (stellarData[i].techLevel < 0)
                stellarData[i].techLevel = 0;
            CopyCField(stellarData[i].onDominate, res.onDominate);
            CopyCField(stellarData[i].onRelease, res.onRelease);
            stellarData[i].weapon = ResIndex(stellarData[i].weapon);
            AdjustHandleSize(stellarFileHand, 0x45e);
            ResourcePurge(stellarFileHand);
            ReleaseResource(stellarFileHand);
            if (SpobCanLand(&stellarData[i]) && !SpobIsHyperOrWorm(&stellarData[i]) &&
                crashlog_isactive()) {
                if (stellarData[i].customPict >= kFirstResourceID) {
                    if (!ResourceExists(kPictType, stellarData[i].customPict)) {
                        crashlog_print("warning: ");
                        crashlog_print_pstr(stellarData[i].name);
                        crashlog_print(" (");
                        crashlog_print_num(i + kFirstResourceID);
                        crashlog_print(") custom landing picture not found\n");
                    }
                } else {
                    if (!ResourceExists(kPictType, stellarData[i].type + 10000)) {
                        crashlog_print("warning: ");
                        crashlog_print_pstr(stellarData[i].name);
                        crashlog_print(" (");
                        crashlog_print_num(i + kFirstResourceID);
                        crashlog_print(") lacks landing picture for stellar type\n");
                    }
                }
                if (!ResourceExists(kDescType, i + kFirstResourceID)) {
                    crashlog_print("warning: ");
                    crashlog_print_pstr(stellarData[i].name);
                    crashlog_print(" (");
                    crashlog_print_num(i + kFirstResourceID);
                    crashlog_print(") lacks landing description\n");
                }
                if (SpobHasBar(&stellarData[i]) &&
                    !ResourceExists(kDescType, i + 10000)) {
                    crashlog_print("warning: ");
                    crashlog_print_pstr(stellarData[i].name);
                    crashlog_print(" (");
                    crashlog_print_num(i + kFirstResourceID);
                    crashlog_print(") has a bar but no bar description\n");
                }
            }
            LoadingIntroMusic();
        } else {
            stellarData[i].loaded = 0;
            stellarData[i].prepared = 0;
        }
        if (found >= count)
            break;
    }
    if (!gameLoaded)
        IterateInGameEventLoop(10);
}

static void LoadSystems(unsigned char *name)
{
    short n;
    short j;
    short i;
    short prev;
    short found;
    short resID;
    uint32_t resType;
    short count;
    Boolean used[kMaxStellars];
    short link, target, total;

    found = 0;
    count = CountResources(kSystType);
    if (count == 0)
        crashlog("warning: no " kSystResName " resources found");
    for (i = 0; i < kMaxSystems; i++) {
        systemData[i].loaded = 0;
        systemData[i].prepared = 0;
        systemData[i].govt = -32767;
        for (j = 0; j < kMaxSystemLinks; j++)
            systemData[i].links[j] = -1;
        systemFileHand = GetResource(kSystType, i + kFirstResourceID);
        if (systemFileHand != NULL) {
            LockAHandle(systemFileHand);
            ResourceNoPurge(systemFileHand);
            found++;
            const SystRes &res = *ResRec(SystRes, systemFileHand);
            GetResInfo(systemFileHand, &resID, &resType, name);
            TrimResourceName(name);
            CopyPascalName(systemData[i].name, name, 31);
            systemData[i].loaded = 1;
            systemData[i].prepared = 1;
            for (j = 0; j < 8; j++) {
                systemData[i].dudeTypes[j] = res.dudeTypes[j];
                systemData[i].dudeProbs[j] = res.probs[j];
            }
            systemData[i].avgShips = res.avgShips;
            systemData[i].govt = res.govt;
            systemData[i].message = res.message;
            systemData[i].interference = res.interference;
            systemData[i].asteroidCount = res.asteroids;
            if (systemData[i].asteroidCount < 0)
                systemData[i].asteroidCount = 0;
            systemData[i].astTypes = res.astTypes;
            systemData[i].pos.h = res.xPos;
            systemData[i].pos.v = res.yPos;
            if (systemData[i].govt >= kFirstResourceID && systemData[i].govt < kFirstResourceID + kMaxGovts)
                systemData[i].govt -= kFirstResourceID;
            else
                systemData[i].govt = -1;
            systemData[i].rgb.red = ((res.bkgndColor >> 16) & 0xff) * 256;
            systemData[i].rgb.green = ((res.bkgndColor >> 8) & 0xff) * 256;
            systemData[i].rgb.blue = (res.bkgndColor & 0xff) * 256;
            systemData[i].rgb.red &= 0xfc00;
            systemData[i].rgb.green &= 0xfc00;
            systemData[i].rgb.blue &= 0xfc00;
            /* BUG: a bright channel (0xF8 or more) wraps to 0, so the ship tint loses it */
            systemData[i].mapPixel = Pixel::FromColorRounded(systemData[i].rgb);
            systemData[i].murk = res.murk;
            if (systemData[i].murk > 100)
                systemData[i].murk = 100;
            else if (systemData[i].murk < 8 && systemData[i].murk > 0)
                systemData[i].murk = 0;
            else if (systemData[i].murk < 0)
                systemData[i].murk = -1;
            total = 0;                       /* valid dude prob total */
            systemData[i].fleetCount = 0;
            systemData[i].fleetProbTotal = 0;
            for (j = 0; j < 8; j++) {
                short dude = systemData[i].dudeTypes[j];
                if (dude >= kFirstResourceID && dude < kFirstResourceID + kMaxDudes) {
                    systemData[i].dudeTypes[j] -= kFirstResourceID;
                    total += systemData[i].dudeProbs[j];
                } else {
                    if (dude <= -kFirstResourceID && dude > -(kFirstResourceID + kMaxFleets)) {
                        /* negative dude entries reference fleets */
                        systemData[i].fleets[systemData[i].fleetCount] = abs(dude) - kFirstResourceID;
                        systemData[i].fleetProbs[systemData[i].fleetCount] = systemData[i].dudeProbs[j];
                        systemData[i].fleetCount++;
                        systemData[i].fleetProbTotal += systemData[i].dudeProbs[j];
                    }
                    systemData[i].dudeTypes[j] = -1;
                    systemData[i].dudeProbs[j] = 0;
                }
            }
            if (total != 100 && total > 0) {
                for (j = 0; j < 8; j++)
                    systemData[i].dudeProbs[j] *= 100.0 / (float)total;
            }
            for (j = 0; j < kMaxSystemLinks; j++) {
                link = total = res.con[j];
                if (link >= kFirstResourceID && link < kFirstResourceID + kMaxSystems)
                    systemData[i].links[j] = link - kFirstResourceID;
                else
                    systemData[i].links[j] = -1;
            }
            for (j = 0; j < kNavSlots; j++) {
                target = res.nav[j];
                if (target >= kFirstResourceID && target < kFirstResourceID + kMaxStellars) {
                    systemData[i].navDefault[j] = target - kFirstResourceID;
                    if (!stellarData[systemData[i].navDefault[j]].prepared) {
                        crashlog_print("warning: ignoring reference to nonexistent stellar ID ");
                        crashlog_print_num(systemData[i].navDefault[j] + kFirstResourceID);
                        crashlog_print(" in system ");
                        crashlog_print_pstr(systemData[i].name);
                        crashlog_print(" (");
                        crashlog_print_num(i + kFirstResourceID);
                        crashlog_print(")\n");
                        systemData[i].navDefault[j] = -1;
                    }
                } else {
                    systemData[i].navDefault[j] = -1;
                }
            }
            for (j = 0; j < 8; j++) {
                systemData[i].persons[j] = res.person[j];
                systemData[i].persProbs[j] = res.personProb[j];
                if (systemData[i].persons[j] >= kFirstResourceID && systemData[i].persons[j] < kFirstResourceID + kMaxPersons - 1)
                    systemData[i].persons[j] -= kFirstResourceID;
                else
                    systemData[i].persons[j] = -1;
                if (systemData[i].persProbs[j] < 0)
                    systemData[i].persProbs[j] = 0;
                if (systemData[i].persProbs[j] > 100)
                    systemData[i].persProbs[j] = 100;
            }
            systemData[i].reinforceFleet = res.reinfFleet;
            systemData[i].reinforceTime = res.reinfTime;
            systemData[i].reinforceInterval = res.reinfInterval;
            if (systemData[i].reinforceFleet >= kFirstResourceID) {
                systemData[i].reinforceFleet -= kFirstResourceID;
                if (systemData[i].reinforceInterval < 1)
                    systemData[i].reinforceInterval = 1;
            } else {
                systemData[i].reinforceFleet = -1;
            }
            systemData[i].reinforceTimer = 0;
            systemData[i].reinforceDaysLeft = 0;
            CopyCField(systemData[i].visibility, res.visibility);
            AdjustHandleSize(systemFileHand, 0x1ac);
            ResourcePurge(systemFileHand);
            ReleaseResource(systemFileHand);
            LoadingIntroMusic();
        } else {
            strcpy(systemData[i].visibility, "");
        }
        if (found >= count)
            break;
    }
    if (!gameLoaded)
        IterateInGameEventLoop(10);

    /* co-location chains: systems sharing a map position form linked groups */
    for (i = 0; i < kMaxSystems; i++) {
        systemData[i].colocGroup = -1;
        systemData[i].colocNext = -1;
    }
    for (i = 0; i < kMaxSystems; i++) {
        if (systemData[i].loaded && systemData[i].colocGroup == -1) {
            prev = i;
            systemData[i].colocGroup = i;
            for (j = 0; j < kMaxSystems; j++) {
                if (systemData[j].loaded &&
                    systemData[j].pos.h == systemData[i].pos.h &&
                    systemData[j].pos.v == systemData[i].pos.v &&
                    (systemData[j].colocGroup == -1 || j > i)) {
                    systemData[prev].colocNext = j;
                    systemData[j].colocGroup = i;
                    prev = j;
                }
            }
        }
    }

    /* drop links to nonexistent systems */
    for (i = 0; i < kMaxSystems; i++) {
        if (systemData[i].prepared) {
            for (j = 0; j < kMaxSystemLinks; j++) {
                link = systemData[i].links[j];
                if (systemData[i].links[j] != -1 && !systemData[systemData[i].links[j]].prepared) {
                    systemData[i].links[j] = -1;
                    crashlog_print("warning: ignoring link from ");
                    crashlog_print_pstr(systemData[i].name);
                    crashlog_print(" (");
                    crashlog_print_num(i + kFirstResourceID);
                    crashlog_print(") to nonexistent system ");
                    crashlog_print_num(link + kFirstResourceID);
                    crashlog_print("\n");
                }
            }
        }
    }

    /* canonicalize links to group leaders, drop self- and duplicate links */
    for (i = 0; i < kMaxSystems; i++) {
        if (systemData[i].prepared) {
            for (n = 0; n < kMaxSystemLinks; n++) {
                link = systemData[i].links[n];
                if (link != -1) {
                    systemData[i].links[n] = FindFirstCoLocatedSystem(link);
                    if (systemData[i].links[n] == FindFirstCoLocatedSystem(i))
                        systemData[i].links[n] = -1;
                }
            }
            for (j = 0; j < kMaxSystemLinks; j++) {
                if (systemData[i].links[j] != -1) {
                    total = 0;
                    for (n = 0; n <= j; n++)
                        if (systemData[i].links[n] == systemData[i].links[j])
                            total++;
                    if (total > 1)
                        systemData[i].links[j] = -1;
                }
            }
        }
    }

    /* assume implicit reciprocal links */
    for (i = 0; i < kMaxSystems; i++) {
        if (systemData[i].prepared) {
            for (j = 0; j < kMaxSystemLinks; j++) {
                link = systemData[i].links[j];
                if (link != -1) {
                    link = FindFirstCoLocatedSystem(link);
                    if (link >= 0 && link < kMaxSystems && link != i) {
                        Boolean haveBack = 0;
                        for (n = 0; n < kMaxSystemLinks; n++) {
                            if (systemData[link].links[n] != -1 &&
                                FindFirstCoLocatedSystem(systemData[link].links[n]) ==
                                    FindFirstCoLocatedSystem(i)) {
                                haveBack = 1;
                                break;
                            }
                        }
                        if (!haveBack) {
                            crashlog_print("warning: assuming implicit reciprocal link from ");
                            crashlog_print_pstr(systemData[i].name);
                            crashlog_print(" (");
                            crashlog_print_num(i + kFirstResourceID);
                            crashlog_print(") to ");
                            crashlog_print_pstr(systemData[link].name);
                            crashlog_print(" (");
                            crashlog_print_num(link + kFirstResourceID);
                            crashlog_print(")\n");
                            for (n = 0; n < kMaxSystemLinks; n++) {
                                if (systemData[link].links[n] == -1) {
                                    systemData[link].links[n] = FindFirstCoLocatedSystem(i);
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    /* orphan stellars: every prepared spob must appear in some navDefault */
    if (crashlog_isactive()) {
        for (i = 0; i < kMaxStellars; i++)
            used[i] = 0;
        for (i = 0; i < kMaxSystems; i++) {
            for (j = 0; j < kNavSlots; j++) {
                if (systemData[i].navDefault[j] >= 0 && systemData[i].navDefault[j] < kMaxStellars)
                    used[systemData[i].navDefault[j]] = 1;
            }
        }
        for (i = 0; i < kMaxStellars; i++) {
            if (stellarData[i].prepared && !used[i]) {
                stellarData[i].loaded = 0;
                stellarData[i].prepared = 0;
                crashlog_print("warning: stellar ");
                if (stellarData[i].name[0] == 0)
                    crashlog_print("\"\"");
                else
                    crashlog_print_pstr(stellarData[i].name);
                crashlog_print(" (");
                crashlog_print_num(i + kFirstResourceID);
                crashlog_print(") does not exist in any system\n");
            }
        }
    }
    if (!gameLoaded)
        IterateInGameEventLoop(10);
}

static void LoadOutfits(unsigned char *name)
{
    short j;
    short i;
    short found;
    short resID;
    uint32_t resType;
    short count;

    found = 0;
    count = CountResources(kOutfType);
    for (i = 0; i < kMaxOutfits; i++) {
        outfitSystemFileHand = GetResource(kOutfType, i + kFirstResourceID);
        if (outfitSystemFileHand != NULL) {
            found++;
            LockAHandle(outfitSystemFileHand);
            ResourceNoPurge(outfitSystemFileHand);
            const OutfRes *res = ResRec(OutfRes, outfitSystemFileHand);
            outfitSystem[i].techLevel = res->techLevel;
            outfitSystem[i].cost = res->cost;
            outfitSystem[i].mass = res->mass;
            outfitSystem[i].modType[0] = res->modType;
            outfitSystem[i].modVal[0] = res->modVal;
            outfitSystem[i].maxCount = res->max;
            outfitSystem[i].itemClass = res->itemClass;
            outfitSystem[i].dispWeight = res->dispWeight;
            outfitSystem[i].scanMask = res->scanMask;
            outfitSystem[i].buyRandom = res->buyRandom;
            if (outfitSystem[i].buyRandom < 0)
                outfitSystem[i].buyRandom = 0;
            if (outfitSystem[i].buyRandom > 100)
                outfitSystem[i].buyRandom = 100;
            outfitSystem[i].govtRestrict = res->require3;
            if ((outfitSystem[i].govtRestrict < ResClassBase(0)
                    || outfitSystem[i].govtRestrict >= ResClassBase(0) + kMaxGovts) &&
                (outfitSystem[i].govtRestrict < ResClassBase(1)
                    || outfitSystem[i].govtRestrict >= ResClassBase(1) + kMaxGovts) &&
                (outfitSystem[i].govtRestrict < ResClassBase(2)
                    || outfitSystem[i].govtRestrict >= ResClassBase(2) + kMaxGovts) &&
                (outfitSystem[i].govtRestrict < ResClassBase(3)
                    || outfitSystem[i].govtRestrict >= ResClassBase(3) + kMaxGovts))
                outfitSystem[i].govtRestrict = -1;
            CopyCField(outfitSystem[i].availability, res->availability);
            CopyCField(outfitSystem[i].onPurchase, res->onPurchase);
            CopyCField(outfitSystem[i].onSell, res->onSell);
            AdjustHandleSize(outfitSystemFileHand, 0x404);
            res = ResRec(OutfRes, outfitSystemFileHand);   /* AdjustHandleSize may move the record */
            outfitSystem[i].persistent = 0;
            outfitSystem[i].flags = res->flags;
            if (OutfPersistent(&outfitSystem[i]))
                outfitSystem[i].persistent = 1;
            GetResInfo(outfitSystemFileHand, &resID, &resType, name);
            TrimResourceName(name);
            CopyPascalName(outfitSystem[i].name, name, 63);
            outfitSystem[i].modType[1] = res->modType2;
            outfitSystem[i].modVal[1] = res->modVal2;
            outfitSystem[i].modType[2] = res->modType3;
            outfitSystem[i].modVal[2] = res->modVal3;
            outfitSystem[i].modType[3] = res->modType4;
            outfitSystem[i].modVal[3] = res->modVal4;
            for (j = 0; j < kOutfMods; j++) {
                if ((outfitSystem[i].modType[j] == OutfMod::Weapon || outfitSystem[i].modType[j] == OutfMod::Ammo ||
                     outfitSystem[i].modType[j] == OutfMod::FakeID) &&outfitSystem[i].modVal[j] >= kFirstResourceID)
                    outfitSystem[i].modVal[j] -= kFirstResourceID;
            }
            outfitSystem[i].contributes[0] = res->contribute1;
            outfitSystem[i].contributes[1] = res->contribute2;
            outfitSystem[i].requires[0] = res->require1;
            outfitSystem[i].requires[1] = res->require2;
            outfitShortNames[i] = FromField(res->shortName, sizeof res->shortName);
            lcOutfitName[i] = FromField(res->lcName, sizeof res->lcName);
            lcPluralOutfitName[i] = FromField(res->lcPlural, sizeof res->lcPlural);
            ResourcePurge(outfitSystemFileHand);
            ReleaseResource(outfitSystemFileHand);
            if (outfitSystem[i].buyRandom > 0 && crashlog_isactive()) {
                if (!ResourceExists(kPictType, i + 6000)) {
                    crashlog_print("warning: ");
                    crashlog_print_pstr(outfitSystem[i].name);
                    crashlog_print(" (");
                    crashlog_print_num(i + kFirstResourceID);
                    crashlog_print(") lacks outfit picture\n");
                }
                if (!ResourceExists(kDescType, i + 3000)) {
                    crashlog_print("warning: ");
                    crashlog_print_pstr(outfitSystem[i].name);
                    crashlog_print(" (");
                    crashlog_print_num(i + kFirstResourceID);
                    crashlog_print(") lacks description\n");
                }
            }
            LoadingIntroMusic();
        } else {
            strcpy(outfitSystem[i].availability, "");
            strcpy(outfitSystem[i].onPurchase, "");
            strcpy(outfitSystem[i].onSell, "");
            outfitSystem[i].techLevel = kOutfTechAbsent;
            outfitSystem[i].mass = 0;
            for (j = 0; j < kOutfMods; j++) {
                outfitSystem[i].modType[j] = 0;
                outfitSystem[i].modVal[j] = 0;
            }
        }
        if (found >= count)
            break;
    }

    /* alias outfits that share a lowercase display name */
    for (i = 0; i < kMaxOutfits; i++) {
        outfitSystem[i].sameNameIndex = i;
        if (outfitSystem[i].techLevel < kOutfTechAbsent) {
            /* no `i > 0` guard: the inner loop's entry test covers i == 0 */
            if (!lcOutfitName[i].empty()) {
                for (j = 0; j < i; j++) {
                    if (outfitSystem[j].techLevel < kOutfTechAbsent && lcOutfitName[i] == lcOutfitName[j]) {
                        outfitSystem[i].sameNameIndex = j;
                        break;
                    }
                }
            }
        }
    }
    if (!gameLoaded)
        IterateInGameEventLoop(10);
}

static void LoadWeapons(unsigned char *name)
{
    short n;
    short j;
    short i;
    short found;
    short resID;
    uint32_t resType;
    short count;
    float base;
    RGBColor rgb;
    RGBColor shade;
    float factor;
    short g;

    found = 0;
    n = count = CountResources(kWeapType);
    for (i = 0; i < kMaxWeapTypes; i++)
        weaponStats[i].guidance = kResAbsent;
    for (i = 0; i < kMaxWeapTypes; i++) {
        weaponFileHand = GetResource(kWeapType, i + kFirstResourceID);
        if (weaponFileHand != NULL) {
            found++;
            LockAHandle(weaponFileHand);
            ResourceNoPurge(weaponFileHand);
            const WeapRes &res = *ResRec(WeapRes, weaponFileHand);
            weaponStats[i].reload = res.reload;
            weaponStats[i].lifetime = res.count;
            weaponStats[i].damage[0] = res.massDmg[0];
            weaponStats[i].damage[1] = res.massDmg[1];
            weaponStats[i].guidance = res.guidance;
            weaponStats[i].speed100 = res.speed / 100.0;
            weaponStats[i].ammoType = res.ammoType;
            weaponStats[i].graphic = res.graphic;
            weaponStats[i].inaccuracy = res.inaccuracy;
            weaponStats[i].sound = res.sound;
            weaponStats[i].impact = res.impact;
            weaponStats[i].recoil = res.recoil;
            weaponStats[i].explodType = res.explodType;
            weaponStats[i].proxRadius = res.proxRadius;
            weaponStats[i].blastRadius = res.blastRadius;
            weaponStats[i].flags = res.flags;
            weaponStats[i].flags2 = res.flags2;
            weaponStats[i].flags3 = res.flags3;
            weaponStats[i].durability = res.durability;
            weaponStats[i].maxAmmo = res.maxAmmo;
            weaponStats[i].seeker = res.seeker;
            weaponStats[i].ionization = res.ionization;
            weaponStats[i].exitType = res.exitType;
            weaponStats[i].burstCount = res.burstCount;
            weaponStats[i].burstReload = res.burstReload;
            weaponStats[i].guidedTurn = res.guidedTurn * 0.1;
            if (weaponStats[i].inaccuracy == -1)
                weaponStats[i].inaccuracy = 0;
            for (j = 0; j < 4; j++) {
                weaponStats[i].jamVuln[j] = res.jamVuln[j];
                if (weaponStats[i].jamVuln[j] < 0)
                    weaponStats[i].jamVuln[j] = 0;
                if (weaponStats[i].jamVuln[j] > 100)
                    weaponStats[i].jamVuln[j] = 100;
            }
            if (res.liColor == 0) {
                rgb.red = rgb.green = 0x200;
                rgb.blue = 0xffff;
            } else {
                rgb.red = ((res.liColor >> 16) & 0xff) * 256;
                rgb.green = ((res.liColor >> 8) & 0xff) * 256;
                rgb.blue = (res.liColor & 0xff) * 256;
            }
            ClampRGB(rgb);
            weaponStats[i].ionizeColor = Pixel::FromColor(rgb);
            if (weaponStats[i].burstCount <= 0)
                weaponStats[i].burstCount = -1;
            weaponStats[i].beamLength = res.beamLength;
            weaponStats[i].beamWidth = res.beamWidth;
            weaponStats[i].coronaFalloff = res.falloff;
            weaponStats[i].beamColor = res.beamColor;
            weaponStats[i].coronaColor = res.coronaColor;
            if (weaponStats[i].beamLength > 0 && weaponStats[i].coronaFalloff <= 0)
                weaponStats[i].coronaFalloff = 16;
            weaponStats[i].lightningDensity = res.liCohesive;
            weaponStats[i].lightningAmplitude = res.liAmplitude;
            if (weaponStats[i].lightningDensity > 0) {
                if (weaponStats[i].lightningDensity < 2)
                    weaponStats[i].lightningDensity = 2;
                weaponStats[i].coronaFalloff = 0;
                if (weaponStats[i].beamWidth < 1)
                    weaponStats[i].beamWidth = 1;
                if (weaponStats[i].lightningAmplitude < 1)
                    weaponStats[i].lightningAmplitude = 1;
            }
            weaponStats[i].subCount = res.subCount;
            weaponStats[i].subType = res.subType;
            weaponStats[i].subTheta = res.subTheta;
            weaponStats[i].subLimit = res.subLimit;
            if (weaponStats[i].subTheta == -1)
                weaponStats[i].subTheta = 0;
            if (weaponStats[i].subType >= kFirstResourceID && weaponStats[i].subType < kFirstResourceID + kMaxWeapTypes)
                weaponStats[i].subType -= kFirstResourceID;
            else
                weaponStats[i].subType = -1;
            if (weaponStats[i].guidance != kGuidanceBeam && weaponStats[i].guidance != kGuidanceTurretBeam
                && weaponStats[i].guidance != kGuidancePDBeam)
                weaponStats[i].decayShots = weaponStats[i].coronaFalloff;
            else
                weaponStats[i].decayShots = 0;
            weaponStats[i].proxSafety = res.proxSafety;
            if (weaponStats[i].subCount <= 0 || weaponStats[i].subType < 0) {
                weaponStats[i].subCount = 0;
                weaponStats[i].subType = -1;
            }
            if (weaponStats[i].guidance == kGuidanceBeam || weaponStats[i].guidance == kGuidanceTurretBeam
                || weaponStats[i].guidance == kGuidancePDBeam) {
                weaponStats[i].rgbScratch.red = ((weaponStats[i].beamColor >> 16) & 0xff) * 256;
                weaponStats[i].rgbScratch.green = ((weaponStats[i].beamColor >> 8) & 0xff) * 256;
                weaponStats[i].rgbScratch.blue = (weaponStats[i].beamColor & 0xff) * 256;
                weaponStats[i].beamColor = Pixel::FromRGB24(weaponStats[i].beamColor);
                weaponStats[i].coronaColor = Pixel::FromRGB24(weaponStats[i].coronaColor);
            }
            weaponStats[i].hitParticles = res.hitParticles;
            rgb.red = ((res.hitPartColor >> 16) & 0xff) * 256;
            rgb.green = ((res.hitPartColor >> 8) & 0xff) * 256;
            rgb.blue = (res.hitPartColor & 0xff) * 256;
            ClampRGB(rgb);
            weaponStats[i].hitPartColor = Pixel::FromColor(rgb);
            weaponStats[i].hitPartLife = res.hitPartLife;
            weaponStats[i].hitPartVel = res.hitPartVel * 0.01;
            GetResInfo(weaponFileHand, &resID, &resType, name);
            TrimResourceName(name);
            weapName[i] = FromPascal(name);
            weaponStats[i].smokeSet = res.smokeSet;
            weaponStats[i].decay = res.decay;
            weaponStats[i].particles = res.particles;
            weaponStats[i].partLifeMin = res.partLifeMin;
            weaponStats[i].partLifeMax = res.partLifeMax;
            base = res.partVel * 0.01;
            rgb.red = ((res.partColor >> 16) & 0xff) * 256;
            rgb.green = ((res.partColor >> 8) & 0xff) * 256;
            rgb.blue = (res.partColor & 0xff) * 256;
            for (j = 0; j < 8; j++) {
                factor = (Rand(0x51) + 60) * 0.01;
                shade.red = (int32_t)(rgb.red * factor);
                shade.green = (int32_t)(rgb.green * factor);
                shade.blue = (int32_t)(rgb.blue * factor);
                // BUG (all versions): factor reaches 1.40, so a component above ~0xb6ff wraps when stored in
                // the unsigned short -- bright particle colours turn dark (mac 1.0.10 clamped the shade, too late)
                weaponStats[i].partColor[j] = Pixel::FromColor(shade);
                weaponStats[i].partVel[j] = base * factor;
            }
            if (weaponStats[i].smokeSet >= 8)
                weaponStats[i].smokeSet = 0;
            if (weaponStats[i].guidance == 2) {
                weaponStats[i].guidance = kGuidanceHoming;
                weaponStats[i].seeker = 0x30fe;
            }
            AdjustHandleSize(weaponFileHand, 0x86);
            ResourcePurge(weaponFileHand);
            ReleaseResource(weaponFileHand);
            LoadingIntroMusic();
        }
        if (found >= n)
            break;
    }
    if (!gameLoaded)
        IterateInGameEventLoop(10);

    /* ammo-source sanity warnings */
    if (crashlog_isactive()) {
        for (i = 0; i < kMaxWeapTypes; i++) {
            if (weaponStats[i].guidance != kResAbsent) {
                short ammo = weaponStats[i].ammoType;
                if (ammo >= 0 && ammo < kMaxWeapTypes &&
                    weaponStats[i].guidance != kGuidanceBay &&
                    (ammo != weaponStats[ammo].ammoType ||
                     weaponStats[ammo].guidance == kGuidanceBay)) {
                    crashlog_print("warning: weapon ");
                    crashlog_print_num(i + kFirstResourceID);
                    crashlog_print(" probably shouldn't draw ammo from weapon ");
                    crashlog_print_num(ammo + kFirstResourceID);
                    crashlog_print("\n");
                }
            }
        }
        for (i = 0; i < kMaxOutfits; i++) {
            if (outfitSystem[i].techLevel < kOutfTechAbsent) {
                for (count = 0; count < kOutfMods; count++) {
                    if (outfitSystem[i].modType[count] == OutfMod::Ammo) {
                        j = outfitSystem[i].modVal[count];
                        g = weaponStats[j].guidance;
                        if (g != kResAbsent) {
                            if (g != kGuidanceBay && j != weaponStats[j].ammoType) {
                                crashlog_print("warning: outfit item ");
                                crashlog_print_num(i + kFirstResourceID);
                                crashlog_print(" grants ammo to weapon ");
                                crashlog_print_num(j + kFirstResourceID);
                                if (weaponStats[j].ammoType >= 0 &&
                                    weaponStats[j].ammoType < kMaxWeapTypes)
                                    crashlog_print(", which doesn't use its own ammo\n");
                                else
                                    crashlog_print(", which doesn't use ammo\n");
                            }
                        } else {
                            crashlog_print("warning: outfit item ");
                            crashlog_print_num(i + kFirstResourceID);
                            crashlog_print(" grants ammo to nonexisent weapon\n");
                        }
                    }
                }
            }
        }
    }

    /* total range: beams use beam length, shots walk the submunition chain */
    for (i = 0; i < kMaxWeapTypes; i++) {
        weaponStats[i].totalRange = 0;
        g = weaponStats[i].guidance;
        if (weaponStats[i].guidance == kGuidanceUnguided || weaponStats[i].guidance == kGuidanceHoming || weaponStats[i].guidance == kGuidanceTurret || weaponStats[i].guidance == kGuidanceRocket || weaponStats[i].guidance == kGuidanceFrontQuad || weaponStats[i].guidance == kGuidanceRearQuad || weaponStats[i].guidance == kGuidancePointDef) {
            n = i;
            do {
                weaponStats[i].totalRange += (double)(weaponStats[n].speed100 * weaponStats[n].lifetime);
                if (weaponStats[n].subCount > 0) {
                    short nxt = weaponStats[n].subType;
                    if (n == nxt) {
                        if (weaponStats[n].subLimit > 0)
                            weaponStats[i].totalRange += (double)(weaponStats[n].subLimit *
                                weaponStats[n].speed100 * weaponStats[n].lifetime);
                        n = -1;
                    } else {
                        n = nxt;
                    }
                } else {
                    n = -1;
                }
            } while (n != -1);
        } else if (g == kGuidanceBeam || g == kGuidanceTurretBeam || g == kGuidancePDBeam) {
            weaponStats[i].totalRange = weaponStats[i].beamLength;
        }
    }
}

static void LoadShipClasses(unsigned char *name)
{
    short n;
    short i;
    short resID;
    uint32_t resType;
    short count;
    short k;
    short gunTally, turretTally;
    short maxGuns, maxTurrets;
    int32_t tradeEst;
    short mountCount[8];
    short mountID[8];
    short mountAmmo[8];
    short shipCount;
    int32_t mass;
    short j;
    short weapID;

    shipCount = CountResources(kShipType);
    if (shipCount == 0)
        crashlog("warning: no " kShipResName " resources found");
    for (i = 0; i < kMaxShipTypes; i++) {
        shipClassFileHand = GetResource(kShipType, i + kFirstResourceID);
        if (shipClassFileHand != NULL) {
            LockAHandle(shipClassFileHand);
            ResourceNoPurge(shipClassFileHand);
            const ShipRes *res = ResRec(ShipRes, shipClassFileHand);
            shipClass[i].cargo = res->cargo;
            shipClass[i].shield = res->shield;
            shipClass[i].accel = res->accel / 10000.0;
            shipClass[i].maxSpeed = res->speed / 100.0;
            shipClass[i].turnRate = res->turn * 0.1;
            shipClass[i].fuel = res->energy;
            shipClass[i].freeMass = res->freeSpace;
            shipClass[i].armor = res->armor;
            shipClass[i].maxGuns = res->maxGun;
            shipClass[i].maxTurrets = res->maxTur;
            shipClass[i].techLevel = res->techLevel;
            shipClass[i].cost = res->cost;
            shipClass[i].deathDelay = res->deathDelay;
            shipClass[i].mass = res->mass;
            shipClass[i].shipLength = res->length;
            shipClass[i].inherentAI = res->inherentAI;
            shipClass[i].crew = res->crew;
            shipClass[i].strength = res->strength;
            shipClass[i].flags = res->flags;
            shipClass[i].flags2 = res->flags2;
            shipClass[i].flags3 = res->flags3;
            shipClass[i].dispWeight = res->dispWeight;
            shipClass[i].podCount = res->podCount;
            shipClass[i].buyRandom = res->buyRandom;
            shipClass[i].hireRandom = res->hireRandom;
            if (InGovtClass(res->inherentGovt, 0)) {
                shipClass[i].inherentGovt = res->inherentGovt - ResClassBase(0);
                shipClass[i].inherentGovtB = shipClass[i].inherentGovt;
            } else if (InGovtClass(res->inherentGovt, 1)) {
                shipClass[i].inherentGovt = -1;
                shipClass[i].inherentGovtB = res->inherentGovt - ResClassBase(1);
            } else if (InGovtClass(res->inherentGovt, 2)) {
                shipClass[i].inherentGovt = res->inherentGovt - ResClassBase(2);
                shipClass[i].inherentGovtB = -1;
            } else {
                shipClass[i].inherentGovt = -1;
                shipClass[i].inherentGovtB = -1;
            }
            if (shipClass[i].buyRandom > 100)
                shipClass[i].buyRandom = 100;
            if (shipClass[i].hireRandom > 100)
                shipClass[i].hireRandom = 100;
            if (shipClass[i].strength < 1)
                shipClass[i].strength = 1;
            shipClass[i].upgradeTo = res->upgradeTo;
            shipClass[i].escUpgradeCost = res->escUpgrdCost;
            shipClass[i].tradeIn = res->escSellValue;
            shipClass[i].upgradeTo = ResIndex(shipClass[i].upgradeTo);
            if (shipClass[i].escUpgradeCost < 0)
                shipClass[i].escUpgradeCost = 0;
            if (shipClass[i].tradeIn <= 0)
                shipClass[i].tradeIn = shipClass[i].cost * 0.1;
            shipClass[i].escortType = res->escortType;
            if (shipClass[i].escortType < 0 || shipClass[i].escortType > 3) {
                if (AIIsCombat(shipClass[i].inherentAI)) {
                    if (shipClass[i].mass < 50)
                        shipClass[i].escortType = 0;
                    else if (shipClass[i].mass < 200)
                        shipClass[i].escortType = 1;
                    else
                        shipClass[i].escortType = 2;
                } else {
                    shipClass[i].escortType = 3;
                }
            }
            if (shipClass[i].escortType == 0)
                shipClass[i].aiSkill = 80;
            else if (shipClass[i].escortType == 1)
                shipClass[i].aiSkill = 90;
            else
                shipClass[i].aiSkill = 100;
            shipClass[i].explode1 = res->explode[0];
            shipClass[i].explode2 = res->explode[1];
            shipClass[i].shieldRech = res->shieldRe;
            shipClass[i].armorRech = res->armorRe;
            if (shipClass[i].shieldRech > 0.0f)
                shipClass[i].shieldRech *= 0.001;
            else
                shipClass[i].shieldRech = 0;
            if (shipClass[i].armorRech > 0.0f)
                shipClass[i].armorRech *= 0.001;
            else
                shipClass[i].armorRech = 0;
            shipClass[i].deionize = res->deionize;
            shipClass[i].ionizeMax = res->ionizeMax;
            if (shipClass[i].deionize > 0.0f)
                shipClass[i].deionize *= 0.01;
            else
                shipClass[i].deionize = 1.0f;
            if (shipClass[i].ionizeMax <= 0)
                shipClass[i].ionizeMax = 0;
            shipClass[i].keyCarried = res->keyCarried;
            shipClass[i].keyCarried = ResIndex(shipClass[i].keyCarried);
            if (shipClass[i].strength < 1)
                shipClass[i].strength = 1;
            if (shipClass[i].cargo < 0) {
                shipClass[i].cargo *= -1;
                shipClass[i].cargoNotFixed = 0;
            } else {
                shipClass[i].cargoNotFixed = 1;
            }
            if (shipClass[i].shield < 0)
                shipClass[i].shield *= -5;
            if (shipClass[i].crew < 0)
                shipClass[i].crew = 0;
            for (j = 0; j < 4; j++) {
                mountID[j + 4] = -1;
                mountID[j] = mountID[j + 4];
                weapID = res->wType[j];
                if (weapID >= kFirstResourceID && weapID < kFirstResourceID + kMaxWeapTypes) {
                    mountID[j] = weapID;
                    mountCount[j] = res->wCount[j];
                    mountAmmo[j] = res->ammo[j];
                } else {
                    mountID[j] = -1;
                }
                weapID = res->wType2[j];
                if (weapID >= kFirstResourceID && weapID < kFirstResourceID + kMaxWeapTypes) {
                    mountID[j + 4] = weapID;
                    mountCount[j + 4] = res->wCount2[j];
                    mountAmmo[j + 4] = res->ammo2[j];
                } else {
                    mountID[j + 4] = -1;
                }
            }
            shipClass[i].defOutfID[0] = res->defaultItems[0];
            shipClass[i].defOutfID[1] = res->defaultItems[1];
            shipClass[i].defOutfID[2] = res->defaultItems[2];
            shipClass[i].defOutfID[3] = res->defaultItems[3];
            shipClass[i].defOutfID[4] = res->defaultItems2[0];
            shipClass[i].defOutfID[5] = res->defaultItems2[1];
            shipClass[i].defOutfID[6] = res->defaultItems2[2];
            shipClass[i].defOutfID[7] = res->defaultItems2[3];
            shipClass[i].defOutfCount[0] = res->itemCount[0];
            shipClass[i].defOutfCount[1] = res->itemCount[1];
            shipClass[i].defOutfCount[2] = res->itemCount[2];
            shipClass[i].defOutfCount[3] = res->itemCount[3];
            shipClass[i].defOutfCount[4] = res->itemCount2[0];
            shipClass[i].defOutfCount[5] = res->itemCount2[1];
            shipClass[i].defOutfCount[6] = res->itemCount2[2];
            shipClass[i].defOutfCount[7] = res->itemCount2[3];
            for (count = 0; count < kDefaultOutfitSlots; count++) {
                if (shipClass[i].defOutfID[count] >= kFirstResourceID && shipClass[i].defOutfID[count] < kFirstResourceID + kMaxOutfits) {
                    shipClass[i].defOutfID[count] -= kFirstResourceID;
                    if (shipClass[i].defOutfCount[count] < 0)
                        shipClass[i].defOutfCount[count] = 0;
                } else {
                    shipClass[i].defOutfID[count] = -1;
                    shipClass[i].defOutfCount[count] = 0;
                }
            }
            shipClass[i].fuelRegen = res->fuelRegen;
            shipClass[i].skillVar = res->skillVar;
            if (shipClass[i].skillVar < 1)
                shipClass[i].skillVar = 1;
            if (shipClass[i].skillVar > 50)
                shipClass[i].skillVar = 50;
            shipClass[i].contributes[0] = res->contribute1;
            shipClass[i].contributes[1] = res->contribute2;
            shipClass[i].requires[0] = res->require1;
            shipClass[i].requires[1] = res->require2;
            tradeEst = shipClass[i].cost * 0.25;
            gunTally = 0;
            turretTally = 0;
            for (count = 0; count < 8; count++) {
                if (mountID[count] >= kFirstResourceID && mountID[count] < kFirstResourceID + kMaxWeapTypes) {
                    shipClass[i].weapCount[mountID[count] - kFirstResourceID] = mountCount[count];
                    shipClass[i].weapAmmo[mountID[count] - kFirstResourceID] = mountAmmo[count];
                    if (mountCount[count] > 0) {
                        for (j = 0; j < kMaxOutfits; j++) {
                            if (outfitSystem[j].modType[0] == OutfMod::Weapon &&
                                outfitSystem[j].modVal[0] == mountID[count] - kFirstResourceID &&
                                outfitSystem[j].techLevel < kOutfTechAbsent) {
                                mass = AdjustedItemMass(&outfitSystem[j], &shipClass[i]) * mountCount[count];
                                shipClass[i].freeMass += mass;
                                if (!outfitSystem[j].persistent)
                                    tradeEst = tradeEst +
                                        (AdjustedItemCost(&outfitSystem[j], &shipClass[i]) * mountCount[count]) * 0.5;
                                if (outfitSystem[j].flags & kOutfFixedGun)
                                    gunTally += mountCount[count];
                                if (outfitSystem[j].flags & kOutfTurret)
                                    turretTally += mountCount[count];
                                break;
                            }
                        }
                    }
                    if (mountAmmo[count] > 0) {
                        if (weaponStats[mountID[count] - kFirstResourceID].guidance == kGuidanceBay)
                            n = mountID[count] - kFirstResourceID;
                        else {
                            n = weaponStats[mountID[count] - kFirstResourceID].ammoType;
                            if (n < 0 || n >= kMaxWeapTypes)
                                n = -1;
                        }
                        if (n != -1) {
                            for (j = 0; j < kMaxOutfits; j++) {
                                if (outfitSystem[j].modType[0] == OutfMod::Ammo &&
                                    outfitSystem[j].modVal[0] == n &&
                                    outfitSystem[j].techLevel < kOutfTechAbsent) {
                                    mass = AdjustedItemMass(&outfitSystem[j], &shipClass[i]) * mountAmmo[count];
                                    shipClass[i].freeMass += mass;
                                    // BUG (all versions): ammo priced by mountCount, not mountAmmo; only feeds
                                    // the crashlog trade-in warning
                                    if (!outfitSystem[j].persistent)
                                        tradeEst = tradeEst +
                                            (AdjustedItemCost(&outfitSystem[j], &shipClass[i]) * mountCount[count]) * 0.5;
                                    break;
                                }
                            }
                        }
                    }
                }
            }
            maxGuns = shipClass[i].maxGuns;
            maxTurrets = shipClass[i].maxTurrets;
            for (k = 0; k < kDefaultOutfitSlots; k++) {
                if (shipClass[i].defOutfID[k] != -1 && shipClass[i].defOutfCount[k] > 0 &&
                    outfitSystem[shipClass[i].defOutfID[k]].techLevel < kOutfTechAbsent) {
                    mass = AdjustedItemMass(&outfitSystem[shipClass[i].defOutfID[k]], &shipClass[i]) *
                           shipClass[i].defOutfCount[k];
                    shipClass[i].freeMass += mass;
                    if (!outfitSystem[shipClass[i].defOutfID[k]].persistent)
                        tradeEst = tradeEst +
                            (AdjustedItemCost(&outfitSystem[shipClass[i].defOutfID[k]], &shipClass[i]) *
                             shipClass[i].defOutfCount[k]) * 0.5;
                    if (crashlog_isactive()) {
                        for (j = 0; j < kOutfMods; j++) {
                            if (outfitSystem[shipClass[i].defOutfID[k]].modType[j] == OutfMod::GunSlots)
                                maxGuns += outfitSystem[shipClass[i].defOutfID[k]].modVal[j];
                            if (outfitSystem[shipClass[i].defOutfID[k]].modType[j] == OutfMod::TurretSlots)
                                maxTurrets += outfitSystem[shipClass[i].defOutfID[k]].modVal[j];
                        }
                    }
                }
            }
            if (shipClass[i].flags & kShipSlowJump)
                shipClass[i].mobilityFactor = 0.7f;
            else if (shipClass[i].flags & kShipSemiFastJump)
                shipClass[i].mobilityFactor = 1.3f;
            else if (shipClass[i].flags & kShipFastJump)
                shipClass[i].mobilityFactor = 1.6f;
            else
                shipClass[i].mobilityFactor = 1.0f;
            shipClass[i].mobilityFactor *= 1.3;
            if (shipClass[i].mobilityFactor < 0.0f)
                shipClass[i].mobilityFactor = 0.5f;
            CopyCField(shipClass[i].availability, res->availability);
            CopyCField(shipClass[i].appearOn, res->appearOn);
            CopyCField(shipClass[i].onPurchase, res->onPurchase);
            CopyCField(shipClass[i].onCapture, res->onCapture);
            CopyCField(shipClass[i].onRetire, res->onRetire);
            AdjustHandleSize(shipClassFileHand, 0x744);
            res = ResRec(ShipRes, shipClassFileHand);   /* AdjustHandleSize may move the record */
            GetResInfo(shipClassFileHand, &resID, &resType, name);
            TrimResourceName(name);
            CopyPascalName(shipClass[i].name, name, 63);
            shipyardShortNames[i] = FromField(res->shortName, sizeof res->shortName);
            shipyardLongNames[i] = FromField(res->longName, sizeof res->longName);
            shipCommNames[i] = FromField(res->commName, sizeof res->commName);
            shipMovieFilenames[i] = FromField(res->movieName, sizeof res->movieName);
            CopyCField(shipClass[i].subtitle, res->subtitle);
            CtoPstr((char *)shipClass[i].subtitle);
            if (crashlog_isactive()) {
                if (maxGuns < 0) {
                    crashlog_print("warning: ");
                    crashlog_print_pstr(shipClass[i].name);
                    crashlog_print(" (");
                    crashlog_print_num(i + kFirstResourceID);
                    crashlog_print(") has a maximum gun count of ");
                    crashlog_print_num(maxGuns);
                    crashlog_print("\n");
                }
                if (maxTurrets < 0) {
                    crashlog_print("warning: ");
                    crashlog_print_pstr(shipClass[i].name);
                    crashlog_print(" (");
                    crashlog_print_num(i + kFirstResourceID);
                    crashlog_print(") has a maximum turret count of ");
                    crashlog_print_num(maxTurrets);
                    crashlog_print("\n");
                }
                if (gunTally > maxGuns) {
                    crashlog_print("warning: ");
                    crashlog_print_pstr(shipClass[i].name);
                    crashlog_print(" (");
                    crashlog_print_num(i + kFirstResourceID);
                    crashlog_print(") has ");
                    crashlog_print_num(gunTally);
                    crashlog_print(" gun-type weapon");
                    if (gunTally > 1)
                        crashlog_print("s");
                    crashlog_print(" but its maximum is ");
                    crashlog_print_num(maxGuns);
                    crashlog_print("\n");
                }
                if (turretTally > maxTurrets) {
                    crashlog_print("warning: ");
                    crashlog_print_pstr(shipClass[i].name);
                    crashlog_print(" (");
                    crashlog_print_num(i + kFirstResourceID);
                    crashlog_print(") has ");
                    crashlog_print_num(turretTally);
                    crashlog_print(" turreted weapon");
                    if (turretTally > 1)
                        crashlog_print("s");
                    crashlog_print(" but its maximum is ");
                    crashlog_print_num(maxTurrets);
                    crashlog_print("\n");
                }
            }
            if (tradeEst > shipClass[i].cost && shipClass[i].buyRandom != 0) {
                crashlog_print("warning: ");
                crashlog_print_pstr(shipClass[i].name);
                crashlog_print(" (");
                crashlog_print_num(i + kFirstResourceID);
                crashlog_print(") has a trade-in value of ");
                crashlog_print_num(tradeEst);
                crashlog_print(" but costs only ");
                crashlog_print_num(shipClass[i].cost);
                crashlog_print("\n");
            }
            ResourcePurge(shipClassFileHand);
            ReleaseResource(shipClassFileHand);
            if (crashlog_isactive()) {
                if (shipClass[i].buyRandom != 0) {
                    if (!ResourceExists(kDescType, i + 13000)) {
                        crashlog_print("warning: ");
                        crashlog_print_pstr(shipClass[i].name);
                        crashlog_print(" (");
                        crashlog_print_num(i + kFirstResourceID);
                        crashlog_print(") lacks purchase description\n");
                    }
                    for (count = 0; count < kMaxWeapTypes; count++) {
                        if (shipClass[i].weapCount[count] > 0) {
                            short granted = 0;

                            for (j = 0; j < kMaxOutfits; j++) {
                                for (k = 0; k < kOutfMods; k++) {
                                    if (outfitSystem[j].modType[k] == OutfMod::Weapon &&
                                        outfitSystem[j].modVal[k] == count) {
                                        granted++;
                                        break;
                                    }
                                }
                                if (granted > 0)
                                    break;
                            }
                            if (granted == 0) {
                                crashlog_print("warning: weapon ID ");
                                crashlog_print_num(count + kFirstResourceID);
                                crashlog_print(" on ship type ");
                                crashlog_print_num(i + kFirstResourceID);
                                crashlog_print(" lacks corresponding " kOutfResName " resource\n");
                            }
                        }
                    }
                }
                if (shipClass[i].hireRandom != 0 &&
                    !ResourceExists(kDescType, i + 14000)) {
                    crashlog_print("warning: ");
                    crashlog_print_pstr(shipClass[i].name);
                    crashlog_print(" (");
                    crashlog_print_num(i + kFirstResourceID);
                    crashlog_print(") lacks hiring description\n");
                }
            }
            LoadingIntroMusic();
        } else {
            strcpy(shipClass[i].availability, "");
            strcpy(shipClass[i].appearOn, "");
            strcpy(shipClass[i].onPurchase, "");
            strcpy(shipClass[i].onCapture, "");
            strcpy(shipClass[i].onRetire, "");
            shipClass[i].techLevel = kResAbsent;
            shipClass[i].shield = 1;
        }
    }
    if (!gameLoaded)
        IterateInGameEventLoop(10);
}

static void LoadDudes(void)
{
    short j;
    short found;
    short count;
    short i;

    found = 0;
    count = CountResources(kDudeType);
    for (i = 0; i < kMaxDudes; i++) {
        dudeFileHand = GetResource(kDudeType, i + kFirstResourceID);
        if (dudeFileHand != NULL) {
            found++;
            LockAHandle(dudeFileHand);
            ResourceNoPurge(dudeFileHand);
            const DudeRes &res = *ResRec(DudeRes, dudeFileHand);
            dudeData[i].aiType = res.aiType;
            dudeData[i].govt = res.govt;
            dudeData[i].flags = res.booty;
            dudeData[i].adviceFlags = res.infoTypes;
            if (dudeData[i].govt >= kFirstResourceID && dudeData[i].govt < kFirstResourceID + kMaxGovts)
                dudeData[i].govt -= kFirstResourceID;
            else
                dudeData[i].govt = -1;
            for (j = 0; j < kDudeShipSlots; j++) {
                dudeData[i].shipTypes[j] = res.shipTypes[j];
                dudeData[i].shipProbs[j] = res.probs[j];
                if (dudeData[i].shipTypes[j] >= kFirstResourceID && dudeData[i].shipTypes[j] < kFirstResourceID + kMaxShipTypes) {
                    dudeData[i].shipTypes[j] -= kFirstResourceID;
                    if (shipClass[dudeData[i].shipTypes[j]].techLevel == kResAbsent) {
                        crashlog_print("warning: " kDudeResName " resource ");
                        crashlog_print_num(i + kFirstResourceID);
                        crashlog_print(" refers to nonexistent ship type ");
                        crashlog_print_num(dudeData[i].shipTypes[j] + kFirstResourceID);
                        crashlog_print("\n");
                        dudeData[i].shipTypes[j] = -1;
                    }
                } else {
                    dudeData[i].shipTypes[j] = -1;
                    dudeData[i].shipProbs[j] = 0;
                }
            }
            AdjustHandleSize(dudeFileHand, 0x58);
            ResourcePurge(dudeFileHand);
            ReleaseResource(dudeFileHand);
            dudeData[i].loaded = 1;
            LoadingIntroMusic();
        } else {
            dudeData[i].loaded = 0;
        }
        if (found >= count)
            break;
    }
    if (!gameLoaded)
        IterateInGameEventLoop(10);
}

static void LoadGovernments(unsigned char *name)
{
    short n;
    short j;
    short i;
    short found;
    short resID;
    uint32_t resType;
    short count;
    int32_t k;

    found = 0;
    count = CountResources(kGovtType);
    for (i = 0; i < kMaxGovts; i++) {
        govtFileHand = GetResource(kGovtType, i + kFirstResourceID);
        if (govtFileHand != NULL) {
            found++;
            LockAHandle(govtFileHand);
            ResourceNoPurge(govtFileHand);
            const GovtRes &res = *ResRec(GovtRes, govtFileHand);
            govtData[i].loaded = 1;
            govtData[i].flags = res.flags;
            govtData[i].flags2 = res.flags2;
            govtData[i].crimeTol = res.crimeTol;
            govtData[i].scanFine = res.scanFine;
            govtData[i].initialRec = res.initialRec;
            govtData[i].voiceType = res.voiceType;
            if (govtData[i].voiceType >= 0 && govtData[i].voiceType < kVoiceSets) {
                govtData[i].voiceClass = -1;
            } else if (govtData[i].voiceType >= 1000 && govtData[i].voiceType < 1000 + kVoiceSets) {
                govtData[i].voiceType -= 1000;
                govtData[i].voiceClass = 1;
            } else if (govtData[i].voiceType >= 2000 && govtData[i].voiceType < 2000 + kVoiceSets) {
                govtData[i].voiceType -= 2000;
                govtData[i].voiceClass = 0;
            } else {
                govtData[i].voiceType = -1;
                govtData[i].voiceClass = -1;
            }
            for (j = 0; j < 4; j++) {
                govtData[i].inhJam[j] = res.inhJam[j];
                if (govtData[i].inhJam[j] > 100)
                    govtData[i].inhJam[j] = 100;
                if (govtData[i].inhJam[j] < 0)
                    govtData[i].inhJam[j] = 0;
            }
            if (res.skillMult > 0)
                govtData[i].skillMult = res.skillMult * 0.01;
            else
                govtData[i].skillMult = 1.0f;
            govtData[i].maxOdds = res.maxOdds * 0.01;
            if (govtData[i].maxOdds < 0.01)
                govtData[i].maxOdds = 0.01f;
            for (j = 0; j <= 4; j++)
                govtData[i].penalties[j] = res.penalty[j];
            for (k = 0; k < 4; k++) {
                govtData[i].classes[k] = res.classes[k];
                govtData[i].allies[k] = res.allies[k];
                govtData[i].enemies[k] = res.enemies[k];
            }
            govtData[i].scanMask = res.scanMask;
            govtData[i].requires[0] = res.require[0];
            govtData[i].requires[1] = res.require[1];
            govtData[i].rgb.red = ((res.color >> 16) & 0xff) * 256;
            govtData[i].rgb.green = ((res.color >> 8) & 0xff) * 256;
            govtData[i].rgb.blue = (res.color & 0xff) * 256;
            govtData[i].rgb2.red = (res.shipColor >> 19) & 0x1f;
            govtData[i].rgb2.green = (res.shipColor >> 11) & 0x1f;
            govtData[i].rgb2.blue = (res.shipColor >> 3) & 0x1f;
            govtData[i].rgbBytes.red = govtData[i].rgb.red >> 8;
            govtData[i].rgbBytes.green = govtData[i].rgb.green >> 8;
            govtData[i].rgbBytes.blue = govtData[i].rgb.blue >> 8;
            govtData[i].interfaceID = res.interfaceID;
            if (govtData[i].interfaceID < kFirstResourceID)
                govtData[i].interfaceID = -1;
            govtData[i].newsPict = res.newsPic;
            if (govtData[i].newsPict < kFirstResourceID)
                govtData[i].newsPict = -1;
            GetResInfo(govtFileHand, &resID, &resType, name);
            TrimResourceName(name);
            CopyPascalName(govtData[i].name, name, 31);
            govtTargStrings[i] = FromField(res.targetCode, sizeof res.targetCode);
            govtShortNames[i] = FromField(res.commName, sizeof res.commName);
            govtMediumNames[i] = FromField(res.mediumName, sizeof res.mediumName);
            AdjustHandleSize(govtFileHand, 0xc0);
            ResourcePurge(govtFileHand);
            ReleaseResource(govtFileHand);
            LoadingIntroMusic();
        } else {
            govtData[i].loaded = 0;
        }
        if (found >= count)
            break;
    }

    /* allies/enemies consistency check over all pairs */
    if (crashlog_isactive()) {
        for (i = 0; i < kMaxGovts; i++) {
            if (govtData[i].loaded) {
                for (n = i + 1; n < kMaxGovts; n++) {
                    if (govtData[n].loaded &&
                        GovtEnemies(i, n) && GovtAllies(i, n)) {
                        crashlog_print("warning: govts ");
                        crashlog_print_num(i + kFirstResourceID);
                        crashlog_print(" and ");
                        crashlog_print_num(n + kFirstResourceID);
                        crashlog_print(" are simultaneously allies and enemies\n");
                    }
                }
            }
        }
    }
    if (!gameLoaded)
        IterateInGameEventLoop(10);
}

static void LoadPersons(unsigned char *name)
{
    short n;
    short j;
    short i;
    short found;
    short resID;
    uint32_t resType;
    short count;
    short mountCount[8];
    short mountID[8];
    short mountAmmo[8];

    found = 0;
    n = count = CountResources(kPersType);
    for (i = 0; i < kMaxPersons;i++) {
        personData[i].loaded = 0;
        personData[i].aiType = kAINone;
        personFileHand = GetResource(kPersType, i + kFirstResourceID);
        if (personFileHand != NULL) {
            found++;
            LockAHandle(personFileHand);
            ResourceNoPurge(personFileHand);
            const PersRes &res = *ResRec(PersRes, personFileHand);
            personData[i].linkSyst = res.linkSyst;
            personData[i].govt = res.govt;
            personData[i].aiType = res.aiType;
            personData[i].aggression = res.aggress;
            personData[i].coward = res.coward;
            personData[i].shipType = res.shipType;
            if (personData[i].shipType >= kFirstResourceID && personData[i].shipType < kFirstResourceID + kMaxShipTypes) {
                personData[i].shipType -= kFirstResourceID;
                if (shipClass[personData[i].shipType].techLevel == kResAbsent) {
                    crashlog_print("warning: p\x91" "rs resource ");
                    crashlog_print_num(i + kFirstResourceID);
                    crashlog_print(" refers to nonexistent ship type ");
                    crashlog_print_num(personData[i].shipType + kFirstResourceID);
                    crashlog_print("\n");
                    personData[i].shipType = -1;
                }
            } else {
                personData[i].shipType = 0;
            }
            personData[i].credits = res.credits;
            personData[i].shieldMod = res.shieldMod;
            personData[i].shieldMod = personData[i].shieldMod / 100.0;
            personData[i].commQuote = res.commQuote;
            personData[i].hailQuote = res.hailQuote;
            personData[i].linkMission = res.linkMission;
            personData[i].flags = res.flags;
            personData[i].flags2 = res.flags2;
            personData[i].hailPict = res.hailPict;
            personData[i].shipColor.red = (res.shipColor >> 19) & 0x1f;
            personData[i].shipColor.green = (res.shipColor >> 11) & 0x1f;
            personData[i].shipColor.blue = (res.shipColor >> 3) & 0x1f;
            personData[i].linkMission = ResIndex(personData[i].linkMission);
            if (personData[i].hailPict < kFirstResourceID)
                personData[i].hailPict = -1;
            if (personData[i].govt >= kFirstResourceID && personData[i].govt < kFirstResourceID + kMaxGovts)
                personData[i].govt -= kFirstResourceID;
            else
                personData[i].govt = -1;
            personData[i].grantClass = res.grantClass;
            personData[i].grantCount = res.grantCount;
            personData[i].grantProb = res.grantProb;
            if (personData[i].grantClass <= 0) {
                personData[i].grantClass = -1;
                personData[i].grantCount = 0;
                personData[i].grantProb = 0;
            } else {
                if (personData[i].grantProb < 0)
                    personData[i].grantProb = 0;
                if (personData[i].grantCount < 0)
                    personData[i].grantCount = 0;
                if (personData[i].grantProb > 100)
                    personData[i].grantProb = 100;
            }
            mountID[1] = res.weapType.left;
            mountID[0] = res.weapType.top;
            mountID[3] = res.weapType.right;
            mountID[2] = res.weapType.bottom;
            mountCount[1] = res.weapCount.left;
            mountCount[0] = res.weapCount.top;
            mountCount[3] = res.weapCount.right;
            mountCount[2] = res.weapCount.bottom;
            mountAmmo[1] = res.ammoLoad.left;
            mountAmmo[0] = res.ammoLoad.top;
            mountAmmo[3] = res.ammoLoad.right;
            mountAmmo[2] = res.ammoLoad.bottom;
            for (j = 0; j < kMaxWeapTypes; j++) {
                personData[i].weapCount[j] = 0;
                personData[i].weapAmmo[j] = 0;
            }
            for (count = 0; count <= 3; count++) {
                if (mountID[count] >= kFirstResourceID) {
                    personData[i].weapCount[mountID[count] - kFirstResourceID] = mountCount[count];
                    personData[i].weapAmmo[mountID[count] - kFirstResourceID] = mountAmmo[count];
                }
            }
            CopyCField(personData[i].subtitle, res.subtitle);
            CopyCField(personData[i].activeOn, res.activateOn);
            CtoPstr(personData[i].subtitle);
            AdjustHandleSize(personFileHand, 400);
            personData[i].loaded = 1;
            GetResInfo(personFileHand, &resID, &resType, name);
            TrimResourceName(name);
            CopyPascalNameSigned(personData[i].name, name, 30);
            ResourcePurge(personFileHand);
            ReleaseResource(personFileHand);
            personData[i].prepared = 1;
            LoadingIntroMusic();
        } else {
            strcpy(personData[i].activeOn, "");
            personData[i].name[0] = 0;
            personData[i].shipType = 0;
            personData[i].govt = -1;
            personData[i].prepared = 0;
        }
        if (found >= n)
            break;
    }

    /* alias persons that share a name prefix */
    for (count = 0; count < kMaxPersons;count++)
        personData[count].sameNameIndex = count;
    for (i = 0; i < kMaxPersons;i++) {
        if (personData[i].name[0] > 0) {
            for (j = 0; j < i; j++) {
                if (personData[j].name[0] > 0 &&
                    j == personData[j].sameNameIndex &&
                    strncmp(&personData[i].name[1],
                            &personData[j].name[1],
                            personData[i].name[0]) == 0) {
                    personData[i].sameNameIndex = j;
                    break;
                }
            }
        }
    }
    if (!gameLoaded)
        IterateInGameEventLoop(10);

    /* systems must reference existing dudes and persons */
    for (j = 0; j < kMaxSystems; j++) {
        if (systemData[j].prepared) {
            for (n = 0; n < 8; n++) {
                if (systemData[j].dudeTypes[n] != -1 && !dudeData[systemData[j].dudeTypes[n]].loaded) {
                    crashlog_print("warning: system ");
                    crashlog_print_num(j + kFirstResourceID);
                    crashlog_print(" makes reference to nonexistent " kDudeResName " resource ");
                    crashlog_print_num(systemData[j].dudeTypes[n] + kFirstResourceID);
                    crashlog_print("\n");
                    systemData[j].dudeTypes[n] = -1;
                }
            }
            for (n = 0; n < 8; n++) {
                if (systemData[j].persons[n] != -1) {
                    i = systemData[j].persons[n];
                    if (!personData[i].prepared) {
                        crashlog_print("warning: system ");
                        crashlog_print_num(j + kFirstResourceID);
                        crashlog_print(" makes reference to nonexistent " kPersResName " resource ");
                        crashlog_print_num(systemData[j].persons[n] + kFirstResourceID);
                        crashlog_print("\n");
                        systemData[j].persons[n] = -1;
                    }
                }
            }
        }
    }
}

static void LoadDisasters(unsigned char *name)
{
    short i;
    short found;
    short resID;
    uint32_t resType;
    short count;

    found = 0;
    count = CountResources(kOopsType);
    for (i = 0; i < kMaxDisasters; i++) {
        disasterData[i].freq = -1;
        disasterData[i].countdown = -1;
        disasterData[i].duration = -1;
        disasterData[i].stellarID = -1;
        disasterData[i].stellar = -32767;
        disasterData[i].loaded = 0;
    }
    for (i = 0; i < kMaxDisasters; i++) {
        disasterFileHand = GetResource(kOopsType, i + kFirstResourceID);
        if (disasterFileHand != NULL) {
            found++;
            LockAHandle(disasterFileHand);
            ResourceNoPurge(disasterFileHand);
            const OopsRes &res = *ResRec(OopsRes, disasterFileHand);
            disasterData[i].stellar = res.stellar;
            disasterData[i].commodity = res.commodity;
            disasterData[i].amount = res.priceDelta;
            disasterData[i].duration = res.duration;
            disasterData[i].freq = res.freq;
            disasterData[i].countdown = 0;
            if (disasterData[i].commodity > 5)
                disasterData[i].commodity = 5;
            if (disasterData[i].commodity < 0)
                disasterData[i].commodity = -1;
            CopyCField(disasterData[i].text, res.activateOn);
            disasterData[i].loaded = 1;
            AdjustHandleSize(disasterFileHand, 0x11a);
            GetResInfo(disasterFileHand, &resID, &resType, name);
            TrimResourceName(name);
            pstrcpy(disasterData[i].name, name);
            ResourcePurge(disasterFileHand);
            ReleaseResource(disasterFileHand);
            LoadingIntroMusic();
        } else {
            strcpy(disasterData[i].text, "");
        }
        if (found >= count)
            break;
    }
}

/* NebuRecord is loaded without LockAHandle or AdjustHandleSize. */
static void LoadNebulae(void)
{
    short i;

    for (i = 0; i < kMaxNebulae; i++) {
        nebulaData[i].width = nebulaData[i].height = 0;
        nebulaData[i].explored = 0;
        nebulaFileHand = GetResource(kNebuType, i + kFirstResourceID);
        if (nebulaFileHand != NULL) {
            ResourceNoPurge(nebulaFileHand);
            const NebuRes &res = *ResRec(NebuRes, nebulaFileHand);
            nebulaData[i].xPos = res.xPos;
            nebulaData[i].yPos = res.yPos;
            nebulaData[i].width = res.xSize;
            nebulaData[i].height = res.ySize;
            CopyCField(nebulaData[i].activeOn, res.activeOn);
            CopyCField(nebulaData[i].onExplore, res.onExplore);
            ResourcePurge(nebulaFileHand);
            ReleaseResource(nebulaFileHand);
            LoadingIntroMusic();
        }
    }
}

static void LoadExplodTypes(void)
{
    short i;

    for (i = 0; i < kMaxExplodTypes; i++) {
        explodType[i].frameRate = 0.25f;
        explodType[i].sound = -1;
        explodType[i].graphic = i;
        boomFileHand = GetResource(kBoomType, i + kFirstResourceID);
        if (boomFileHand != NULL) {
            const BoomRes &res = *ResRec(BoomRes, boomFileHand);
            explodType[i].frameRate = res.frameAdvance;
            explodType[i].sound = res.soundIndex;
            explodType[i].graphic = res.graphicIndex;
            explodType[i].frameRate *= 0.01;
            ReleaseResource(boomFileHand);
            LoadingIntroMusic();
        }
    }
    if (!gameLoaded)
        IterateInGameEventLoop(10);
}

static void LoadFleets(void)
{
    short j;
    short i;
    short found;
    short count;

    found = 0;
    count = CountResources(kFletType);
    for (i = 0; i < kMaxFleets; i++) {
        fleetData[i].leadShipType = -1;
        fleetFileHand = GetResource(kFletType, i + kFirstResourceID);
        if (fleetFileHand != NULL) {
            found++;
            LockAHandle(fleetFileHand);
            ResourceNoPurge(fleetFileHand);
            const FletRes &res = *ResRec(FletRes, fleetFileHand);
            fleetData[i].leadShipType = res.leadShip;
            if (fleetData[i].leadShipType >= kFirstResourceID)
                fleetData[i].leadShipType -= kFirstResourceID;
            else
                fleetData[i].leadShipType = 0;
            fleetData[i].govt = res.govt;
            fleetData[i].linkSyst = res.linkSyst;
            fleetData[i].quoteStrList = res.quote;
            fleetData[i].flags = res.flags;
            fleetData[i].govt = ResIndex(fleetData[i].govt);
            for (j = 0; j <= 3; j++) {   /* exe's back-edge is `cmp ax,3 / jle` */
                fleetData[i].escortTypes[j] = res.escortShip[j];
                if (fleetData[i].escortTypes[j] >= kFirstResourceID) {
                    fleetData[i].escortTypes[j] -= kFirstResourceID;
                    fleetData[i].escortMin[j] = res.minCount[j];
                    fleetData[i].escortMax[j] = res.maxCount[j];
                } else {
                    fleetData[i].escortTypes[j] = -1;
                    fleetData[i].escortMin[j] = 0;
                    fleetData[i].escortMax[j] = 0;
                }
            }
            CopyCField(fleetData[i].activateOn, res.activateOn);
            AdjustHandleSize(fleetFileHand, 0x132);
            ResourcePurge(fleetFileHand);
            ReleaseResource(fleetFileHand);
            LoadingIntroMusic();
        } else {
            strcpy(fleetData[i].activateOn, "");
        }
        if (found >= count)
            break;
    }
    if (!gameLoaded)
        IterateInGameEventLoop(10);
}

static void LoadJunks(unsigned char *name)
{
    short loaded;
    short n;
    short j;
    short i;
    short resID;
    uint32_t resType;
    short count;

    loaded = 0;
    count = CountResources(kJunkType);
    for (i = 0; i < kMaxJunks; i++) {
        for (j = 0; j < 8; j++) {
            junkData[i].boughtAt[j] = -1;
            junkData[i].soldAt[j] = -1;
        }
        junkData[i].basePrice = -1;
    }
    for (i = 0; i < kMaxJunks; i++) {
        junkData[i].held = 0;
        junkFileHand = GetResource(kJunkType, i + kFirstResourceID);
        if (junkFileHand != NULL) {
            loaded++;
            LockAHandle(junkFileHand);
            ResourceNoPurge(junkFileHand);
            const JunkRes &res = *ResRec(JunkRes, junkFileHand);
            junkData[i].basePrice = res.basePrice;
            junkData[i].flags = res.flags;
            junkData[i].scanMask = res.scanMask;
            if (junkData[i].basePrice < 0) {
                crashlog_print("warning: junk type ");
                crashlog_print_num(i + kFirstResourceID);
                crashlog_print(" has invalid base price\n");
                junkData[i].basePrice = 0;
            }
            for (j = 0; j < 8; j++) {
                junkData[i].boughtAt[j] = res.boughtAt[j];
                junkData[i].soldAt[j] = res.soldAt[j];
                junkData[i].boughtAt[j] = ResIndex(junkData[i].boughtAt[j]);
                junkData[i].soldAt[j] = ResIndex(junkData[i].soldAt[j]);
            }
            GetResInfo(junkFileHand, &resID, &resType, name);
            TrimResourceName(name);
            CopyPascalName(junkData[i].name, name, 63);
            CopyCField(junkData[i].lcName, res.lcName);
            CopyCField(junkData[i].abbrev, res.abbrev);
            CopyCField(junkData[i].buyOn, res.buyOn);
            CopyCField(junkData[i].sellOn, res.sellOn);
            ResourcePurge(junkFileHand);
            ReleaseResource(junkFileHand);
            LoadingIntroMusic();
        }
        if (loaded >= count)
            break;
    }

    /* trade centers must trade something */
    for (i = 0; i < kMaxStellars; i++) {
        if (stellarData[i].prepared &&
            (stellarData[i].flags & (kSpobCanLand | kSpobTradeCenter)) ==
                (kSpobCanLand | kSpobTradeCenter) &&
            !SpobIsHyperOrWorm(&stellarData[i]) &&
            (stellarData[i].flags & kSpobPriceBits) == 0) {
            n = 0;
            for (count = 0; count < kMaxJunks; count++) {
                for (j = 0; j < 8; j++) {
                    if (i == junkData[count].boughtAt[j] || i == junkData[count].soldAt[j]) {
                        n++;
                        break;
                    }
                }
            }
            if (n == 0) {
                crashlog_print("warning: ");
                crashlog_print_pstr(stellarData[i].name);
                crashlog_print(" (");
                crashlog_print_num(i + kFirstResourceID);
                crashlog_print(") has a trade center but trades no goods\n");
                stellarData[i].flags = stellarData[i].flags & ~kSpobTradeCenter;
            }
        }
    }
}

static void LoadCrons(void)
{
    short j;
    short i;

    CountResources(kCronType);   /* result unused; the loop walks every slot */
    for (i = 0; i < kMaxCrons; i++) {
        cronEvent[i].active = 0;
        cronEvent[i].loaded = 0;
        cronEvent[i].random = -1;
        cronEvent[i].duration = -1;
        cronEvent[i].daysLeft = 0;
        cronEvent[i].holdoffLeft = 0;
        cronFileHand = GetResource(kCronType, i + kFirstResourceID);
        if (cronFileHand != NULL) {
            LockAHandle(cronFileHand);
            const CronRes &res = *ResRec(CronRes, cronFileHand);
            cronEvent[i].firstDay = res.firstDay;
            cronEvent[i].firstMonth = res.firstMonth;
            cronEvent[i].firstYear = res.firstYear;
            cronEvent[i].lastDay = res.lastDay;
            cronEvent[i].lastMonth = res.lastMonth;
            cronEvent[i].lastYear = res.lastYear;
            cronEvent[i].random = res.random;
            cronEvent[i].duration = res.duration;
            cronEvent[i].preHoldoff = res.preHoldoff;
            cronEvent[i].postHoldoff = res.postHoldoff;
            cronEvent[i].flags = res.indNewsStr;
            for (j = 0; j < 4; j++) {
                cronEvent[i].govts[j] = res.newsGovt[j];
                cronEvent[i].govtValues[j] = res.govtNewsStr[j];
                if (cronEvent[i].govts[j] >= kFirstResourceID) {
                    cronEvent[i].govts[j] -= kFirstResourceID;
                } else {
                    cronEvent[i].govts[j] = -1;
                    cronEvent[i].govtValues[j] = -1;
                }
                if (cronEvent[i].govtValues[j] < -1)
                    cronEvent[i].govtValues[j] = -1;
            }
            cronEvent[i].indNewsStr = res.flags;
            for (j = 0; j <= 1; j++) {
                cronEvent[i].contributes[j] = res.contribute[j];
                cronEvent[i].requires[j] = res.require[j];
            }
            CopyCField(cronEvent[i].enableOn, res.enableOn);
            CopyCField(cronEvent[i].onStart, res.onStart);
            CopyCField(cronEvent[i].onEnd, res.onEnd);
            cronEvent[i].loaded = 1;
            AdjustHandleSize(cronFileHand, 0x336);
            ResourcePurge(cronFileHand);
            ReleaseResource(cronFileHand);
            LoadingIntroMusic();
        }
    }
}

static void LoadRanks(unsigned char *name)
{
    short i;
    short resID;
    uint32_t resType;

    for (i = 0; i < kMaxRanks; i++) {
        rankData[i].govt = -1;
        rankData[i].active = 0;
        rankFileHand = GetResource(kRankType, i + kFirstResourceID);
        if (rankFileHand != NULL) {
            LockAHandle(rankFileHand);
            const RankRes &res = *ResRec(RankRes, rankFileHand);
            rankData[i].loaded = 1;
            rankData[i].govt = res.attachTo;
            rankData[i].govt = ResIndex(rankData[i].govt);
            rankData[i].priority = res.weight;
            rankData[i].contributes[0] = res.contribute[0];
            rankData[i].contributes[1] = res.contribute[1];
            rankData[i].flags = res.priceMod;
            rankData[i].salary = res.salary;
            rankData[i].salaryCap = res.salaryCap;
            rankData[i].priceMod = res.flags;
            if (rankData[i].priceMod <= 0)
                rankData[i].priceMod = 100;
            GetResInfo(rankFileHand, &resID, &resType, name);
            TrimResourceName(name);
            PtoCstr(name);
            strncpy(rankData[i].name, (char *)name, 127);
            rankData[i].name[127] = 0;
            CopyCField(rankData[i].convName, res.name);
            CopyCField(rankData[i].shortName, res.shortName);
            AdjustHandleSize(rankFileHand, 0x98);
            ResourcePurge(rankFileHand);
            ReleaseResource(rankFileHand);
            LoadingIntroMusic();
        } else {
            rankData[i].loaded = 0;
            strcpy(rankData[i].name, "");
            strcpy(rankData[i].convName, "");
            strcpy(rankData[i].shortName, "");
        }
    }
}

static void LoadAsteroidTypes(void)
{
    short j;
    short i;
    RGBColor rgb;

    for (i = 0; i < kMaxRoidTypes; i++) {
        asteroidFileHand = GetResource(kRoidType, i + kFirstResourceID);
        if (asteroidFileHand != NULL) {
            LockAHandle(asteroidFileHand);
            const RoidRes &res = *ResRec(RoidRes, asteroidFileHand);
            asteroidType[i].strength = res.strength;
            if (asteroidType[i].strength < 1)
                asteroidType[i].strength = 1;
            asteroidType[i].frameRate = (float)res.spinRate * 0.01;
            asteroidType[i].yieldType = res.yieldType;
            asteroidType[i].yieldQty = res.yieldQty;
            if (asteroidType[i].yieldQty < 0) {
                asteroidType[i].yieldQty = 0;
                asteroidType[i].yieldType = -1;
            }
            if (asteroidType[i].yieldType < 0 ||
                (asteroidType[i].yieldType > 6 && asteroidType[i].yieldType < 1000) ||
                asteroidType[i].yieldType >= 1000 + kMaxJunks) {
                asteroidType[i].yieldType = -1;
                asteroidType[i].yieldQty = 0;
            }
            asteroidType[i].partCount = res.partCount;
            if (asteroidType[i].partCount < 0)
                asteroidType[i].partCount = 0;
            asteroidType[i].color = res.partColor;
            rgb.red = ((asteroidType[i].color >> 16) & 0xff) * 256;
            rgb.green = ((asteroidType[i].color >> 8) & 0xff) * 256;
            rgb.blue = (asteroidType[i].color & 0xff) * 256;
            asteroidType[i].color = Pixel::FromColor(rgb);
            asteroidType[i].fragCount = res.fragCount;
            if (asteroidType[i].fragCount < 0)
                asteroidType[i].fragCount = 0;
            for (j = 0; j < 2; j++) {
                asteroidType[i].fragType[j] = res.fragType[j];
                if (asteroidType[i].fragType[j] >= kFirstResourceID && asteroidType[i].fragType[j] < kFirstResourceID + kMaxRoidTypes)
                    asteroidType[i].fragType[j] -= kFirstResourceID;
                else if (asteroidType[i].fragType[j] < 0 || asteroidType[i].fragType[j] >= kMaxRoidTypes)
                    asteroidType[i].fragType[j] = -1;
            }
            asteroidType[i].explodType = res.explodType;
            if (asteroidType[i].explodType < 0 ||
                (asteroidType[i].explodType >= kMaxExplodTypes && asteroidType[i].explodType < 1000) ||
                asteroidType[i].explodType >= 1000 + kMaxExplodTypes)
                asteroidType[i].explodType = -1;
            asteroidType[i].mass = res.mass;
            AdjustHandleSize(asteroidFileHand, 0x28);
            ResourcePurge(asteroidFileHand);
            ReleaseResource(asteroidFileHand);
            LoadingIntroMusic();
        }
    }
}

static void LoadInterfaceColors(unsigned char *name)
{
    colorFileHand = GetIndResource(kColrType, 1);
    if (colorFileHand != NULL) {
        short k;
        int c;

        LockAHandle(colorFileHand);
        const ColrRes &res = *ResRec(ColrRes, colorFileHand);
        for (c = 0; c < 2; c++) {
            menuTextColor[c].blue = (res.menuColor[c] & 0xff) * 256;
            menuTextColor[c].green = ((res.menuColor[c] >> 8) & 0xff) * 256;
            menuTextColor[c].red = ((res.menuColor[c] >> 16) & 0xff) * 256;
        }
        for (c = 0; c < 2; c++) {
            shopGridColor[c].blue = (res.gridColor[c] & 0xff) * 256;
            shopGridColor[c].green = ((res.gridColor[c] >> 8) & 0xff) * 256;
            shopGridColor[c].red = ((res.gridColor[c] >> 16) & 0xff) * 256;
        }
        for (k = 0; k < 3; k++) {
            progressColor[k].blue = (res.progressColor[k] & 0xff) * 256;
            progressColor[k].green = ((res.progressColor[k] >> 8) & 0xff) * 256;
            progressColor[k].red = ((res.progressColor[k] >> 16) & 0xff) * 256;
        }
        for (k = 0; k < 3; k++) {
            buttonColor[k].blue = (res.buttonColor[k] & 0xff) * 256;
            buttonColor[k].green = ((res.buttonColor[k] >> 8) & 0xff) * 256;
            buttonColor[k].red = ((res.buttonColor[k] >> 16) & 0xff) * 256;
        }
        for (k = 0; k < kMainButtons; k++) {
            menuButtonPos[k].h = res.buttonPos[k * 2];
            menuButtonPos[k].v = res.buttonPos[k * 2 + 1];
        }
        floatingBorderColor.blue = (res.floatingMapColor & 0xff) * 256;
        floatingBorderColor.green = ((res.floatingMapColor >> 8) & 0xff) * 256;
        floatingBorderColor.red = ((res.floatingMapColor >> 16) & 0xff) * 256;
        escortMenuColor.blue = (res.escortHiliteColor & 0xff) * 256;
        escortMenuColor.green = ((res.escortHiliteColor >> 8) & 0xff) * 256;
        escortMenuColor.red = ((res.escortHiliteColor >> 16) & 0xff) * 256;
        ldefColors[0].blue = (res.listColor[0] & 0xff) * 256;
        ldefColors[0].green = ((res.listColor[0] >> 8) & 0xff) * 256;
        ldefColors[0].red = ((res.listColor[0] >> 16) & 0xff) * 256;
        ldefColors[1].blue = (res.listColor[1] & 0xff) * 256;
        ldefColors[1].green = ((res.listColor[1] >> 8) & 0xff) * 256;
        ldefColors[1].red = ((res.listColor[1] >> 16) & 0xff) * 256;
        ldefColors[2].blue = (res.listColor[2] & 0xff) * 256;
        ldefColors[2].green = ((res.listColor[2] >> 8) & 0xff) * 256;
        ldefColors[2].red = ((res.listColor[2] >> 16) & 0xff) * 256;
        progressDefRect.top = res.progressBar.top;
        progressDefRect.left = res.progressBar.left;
        progressDefRect.bottom = res.progressBar.bottom;
        progressDefRect.right = res.progressBar.right;
        CopyCField(name, sizeof(Str255), res.menuFont, sizeof res.menuFont);
        CtoPstr((char *)name);
        GetFNum(name, &menuFontID);
        menuTextSize = res.menuFontSize;
        CopyCField(name, sizeof(Str255), res.buttonFont, sizeof res.buttonFont);
        CtoPstr((char *)name);
        GetFNum(name, &buttonFontID);
        buttonFontSize = res.buttonFontSize;
        logoDestPoint.h = res.logoPos[0];
        logoDestPoint.v = res.logoPos[1];
        rolloverDestPoint.h = res.rolloverPos[0];
        rolloverDestPoint.v = res.rolloverPos[1];
        for (k = 0; k < 3; k++) {
            slideDestPoint[k].h = res.slidePos[k * 2];
            slideDestPoint[k].v = res.slidePos[k * 2 + 1];
        }
        AdjustHandleSize(colorFileHand, 0xf4);
        ResourcePurge(colorFileHand);
        ReleaseResource(colorFileHand);
        LoadingIntroMusic();
    }
}


// FUNCTION: EVNOVA 0x004bd3c0
// MAC: _LoadObjectData i386 0x000771b0 ppc 0x00080100
void LoadObjectData(void)
{
    /* BUG: strncpy of a Pascal name (no NUL) also copies what follows it in this buffer into the name tables
       (the tail of an earlier, longer name). Readers stop at the length byte. One buffer for all loaders keeps
       those bytes. */
    unsigned char name[256] = {};

    if (EVBugResourceFieldTest(0))
        return;

    LoadStellars(name);
    LoadSystems(name);
    LoadOutfits(name);
    LoadWeapons(name);
    LoadShipClasses(name);
    LoadDudes();
    LoadGovernments(name);
    LoadPersons(name);
    LoadDisasters(name);
    LoadNebulae();
    LoadExplodTypes();
    LoadFleets();
    LoadJunks(name);
    LoadCrons();
    LoadRanks(name);
    LoadAsteroidTypes();
    LoadInterfaceColors(name);

    /* l33t: easter egg */
    if (CountResources(kLeetType) > 0)
        leetMode = 1;
}


// FUNCTION: EVNOVA 0x004c6d50
// MAC: _LoadDescription i386 0x000717fe ppc 0x0007a284
void LoadDescription(char *dst, size_t dstSize, short id)
{
    short size;
    short count;
    size_t handleSize;
    size_t len;

    textFileHand = GetResource(kDescType, id);
    if (textFileHand != NULL) {
        handleSize = GetAHandleSize(textFileHand);
        len = strnlen(*textFileHand, handleSize);
        CopyCField(parseBuffer, sizeof parseBuffer, *textFileHand, len);
        size = (short)len;
        if (handleSize > len) {
            /* the fields after the text's NUL; bytes past the resource read as 0 */
            const unsigned char *text = ResRec(DescRes, textFileHand)->text;
            auto at = [&](size_t k) -> unsigned char { return len + k < handleSize ? text[len + k] : 0; };

            descResourcePictId = at(1) << 8;
            descResourcePictId += at(2);

            descMovieName[0] = 0;
            for (count = 0; count < 0x20; count++) {
                if ((char)at(3 + count) == 0)
                    break;
                descMovieName[0]++;
                descMovieName[count + 1] = at(3 + count);
            }

            descFlags = at(0x23) << 8;
            descFlags += at(0x24);
        } else {
            pstrcpy(descMovieName, (unsigned char *)"");
            descResourcePictId = 0;
            descFlags = 0;
        }

        ReleaseResource(textFileHand);
        descRankGovt[0] = -1;
        descRankGovt[1] = -1;

        ParseTextDescription();
        CopyCField(dst, dstSize, parseBuffer, sizeof parseBuffer);

        if (leetMode) {
            for (count = 0; count < size && (size_t)count < dstSize; count++) {
                if (dst[count] == 'a' || dst[count] == 'A') {
                    if (Rand(3) < 2)
                        dst[count] = '4';
                }
                if (dst[count] == 'e' || dst[count] == 'E') {
                    if (Rand(3) < 2)
                        dst[count] = '3';
                }
                if (dst[count] == 'i' || dst[count] == 'I') {
                    if (Rand(3) < 2)
                        dst[count] = '1';
                }
                if (dst[count] == 'o' || dst[count] == 'O') {
                    if (Rand(3) < 2)
                        dst[count] = '0';
                }
                if (count < size - 4 && (size_t)count + 2 < dstSize && (dst[count] == 'y' || dst[count] == 'Y')
                        && dst[count + 1] == 'o' && dst[count + 2] == 'u') {
                    dst[count] = 'j';
                    dst[count + 1] = '0';
                    dst[count + 2] = '0';
                }
            }
        }
    } else {
        if (dstSize > 0)
            dst[0] = 0;
        pstrcpy(descMovieName, (unsigned char *)"");
        descResourcePictId = 0;
    }
}

/* the `<` and `<=` loop bounds differ per site and are not interchangeable */
#define kCargoPluginBase  0x2328
#define kCargoFallback     0xfa0
#define kCargoCount       0x100

#define kGovtPluginBase   0x24b8
#define kGovtFallback      0xfa3
#define kGovtLast              5

#define kLcCargoPluginBase 0x238c
#define kLcCargoFallback    0xfa1
#define kLcCargoCount      0x100

#define kMiscPluginBase   0x23f0
#define kMiscFallback      0xfa2
#define kMiscCount        0x100

#define kHelpList        0x138b
#define kHelpCount        0x14
#define kLegalList          0x86
#define kLegalLast          0x11
#define kRatingList         0x8a
#define kRatingLast          0xa

#define kStrTon                1
#define kStrTons               2
#define kStrCredit          0x20
#define kStrCredits         0x21
#define kFuelFirst             3
#define kFuelCount             8
#define kSecondFirst         0xb
#define kSecondCount           6
#define kThirdFirst         0x10
#define kThirdCount            3
#define kFourthFirst        0x16
#define kFourthCount         0xa

/* STR# entry `index' of `list', every byte of it (IndString stops at a NUL) */
static std::string PascalIndString(short list, short index)
{
    Str255 str;

    GetIndString(str, list, index);
    return FromPascal(str);
}

// FUNCTION: EVNOVA 0x004c7040
// MAC: _LoadStrings i386 0x00071af5 ppc 0x0007a634
void LoadStrings(void)
{
    short i;

    for (i = 0; i < kCargoCount; i++)
        if (LoadPluginString((i + kCargoPluginBase), cargoName[i]) == 0)
            cargoName[i] = PascalIndString(kCargoFallback, i + 1);

    for (i = 0; i <= kGovtLast; i++)
        if (LoadPluginString((i + kGovtPluginBase), statusCargoName[i]) == 0)
            statusCargoName[i] = PascalIndString(kGovtFallback, i + 1);

    for (i = 0; i < kLcCargoCount; i++)
        if (LoadPluginString((i + kLcCargoPluginBase), lcCargoName[i]) == 0)
            lcCargoName[i] = PascalIndString(kLcCargoFallback, i + 1);

    for (i = 0; i < kHelpCount; i++)
        shipHelpRequests[i] = PascalIndString(kHelpList, i + 1);

    for (i = 0; i <= kLegalLast; i++)
        legalStatusStrings[i] = PascalIndString(kLegalList, i + 1);

    for (i = 0; i <= kRatingLast; i++)
        ratingStrings[i] = PascalIndString(kRatingList, i + 1);

    for (i = 0; i < kMiscCount; i++)
        if (LoadPluginString((i + kMiscPluginBase), cargoAbbrev[i]) == 0)
            cargoAbbrev[i] = PascalIndString(kMiscFallback, i + 1);

    tonString = PascalIndString(kGameStrList, kStrTon);
    tonsString = PascalIndString(kGameStrList, kStrTons);
    creditString = PascalIndString(kGameStrList, kStrCredit);
    creditsString = PascalIndString(kGameStrList, kStrCredits);
    CopyCField(creditCString, sizeof creditCString, creditString.c_str(), creditString.size());
    CopyCField(creditsCString, sizeof creditsCString, creditsString.c_str(), creditsString.size());

    for (i = 0; i < kFuelCount; i++)
        fuelString[i] = PascalIndString(kGameStrList, i + kFuelFirst);

    for (i = 0; i < kSecondCount; i++)
        shieldString[i] = PascalIndString(kGameStrList, i + kSecondFirst);

    for (i = 0; i < kThirdCount; i++)
        armorString[i] = PascalIndString(kGameStrList, i + kThirdFirst);

    for (i = 0; i < kFourthCount; i++)
        newbieString[i] = PascalIndString(kGameStrList, i + kFourthFirst);
}

// FUNCTION: EVNOVA 0x004c73b0
// MAC: _LoadPluginString i386 0x00071a8e ppc 0x0007a5b0
Boolean LoadPluginString(short id, void *dst)
{
    Handle h;
    uint32_t size;

    h = GetResource(kStringType, id);
    if (h != NULL) {
        /* a Pascal string: at most its 256 bytes */
        size = GetAHandleSize(h);
        memcpy(dst, *h, size < sizeof(Str255) ? size : sizeof(Str255));
        ReleaseResource(h);
        return 1;
    }
    return 0;
}

/* the same into a string: a length byte past the resource's end reads 0s */
Boolean LoadPluginString(short id, std::string &dst)
{
    Str255 str;

    memset(str, 0, sizeof str);
    if (LoadPluginString(id, (void *)str) == 0)
        return 0;
    dst = FromPascal(str);
    return 1;
}

/* The preferences file: the name from STR# 130, in the preferences folder (else beside the program), as JSON. */
static std::string PrefsPath(void)
{
    char name[256 + sizeof(".json")];

    name[0] = 0;
    StrcatIndCString(name, kPrefsStrList, 1);
    strcat(name, ".json");
    return JoinPath(paths.prefs.empty() ? paths.exe : paths.prefs, name);
}

/* The settings and key bindings into a PrefsRecord. */
static void PrefsToRecord(PrefsRecord *rec)
{
    short i;

    rec->introMusic = settings.introMusic != 0;
    rec->voicesOn = settings.voicesOn != 0;
    rec->noMovies = settings.noMovies != 0;
    rec->noDebris = settings.noDebris != 0;
    rec->shipAnimations = settings.shipAnimations != 0;
    rec->engineGlows = settings.engineGlows != 0;
    rec->weaponGlows = settings.weaponGlows != 0;
    rec->runningLights = settings.runningLights != 0;
    rec->parallaxStars = settings.parallaxStars != 0;
    rec->ambientSounds = settings.ambientSounds != 0;
    rec->showMapBorders = settings.showMapBorders != 0;
    rec->noHyperGamma = settings.noHyperGamma != 0;
    rec->yieldEvents = settings.yieldEvents != 0;
    rec->volume = settings.volume;
    rec->gamma = settings.gamma;
    rec->gameWidth = settings.gameWidth;
    rec->gameHeight = settings.gameHeight;
    rec->scaleFilter = settings.scaleFilter;
    rec->stereoPan = settings.stereoPan != 0;
    rec->asteroidScaling = settings.asteroidScaling != 0;
    rec->mapPrices = settings.mapPrices != 0;
    rec->mapDisasters = settings.mapDisasters != 0;
    rec->runInWindow = settings.runInWindow != 0;
    rec->gameSize = settings.gameSize;
    rec->wheelZoom = settings.wheelZoom != 0;
    rec->orBlend = settings.orBlend != 0;
    rec->uiScale = settings.uiScale;
    KeyPrefsToSlotTable();
    for (i = 0; i <= 35; i++)
        rec->keys[i] = (short)newKeyPrefs[i];
}

/* A PrefsRecord into the settings and key bindings. */
static void RecordToPrefs(const PrefsRecord *rec)
{
    short i;

    settings.introMusic = rec->introMusic != 0;
    settings.voicesOn = rec->voicesOn != 0;
    settings.clipping = 0;
    settings.noMovies = rec->noMovies != 0;
    settings.noDebris = rec->noDebris != 0;
    settings.parallaxStars = rec->parallaxStars != 0;
    settings.yieldEvents = rec->yieldEvents != 0;
    settings.windowed = 0;
    settings.ambientSounds = rec->ambientSounds != 0;
    settings.showMapBorders = rec->showMapBorders != 0;
    settings.noHyperGamma = rec->noHyperGamma != 0;
    settings.shipAnimations = rec->shipAnimations != 0;
    settings.engineGlows = rec->engineGlows != 0;
    settings.weaponGlows = rec->weaponGlows != 0;
    settings.runningLights = rec->runningLights != 0;
    settings.volume = rec->volume;
    settings.gamma = rec->gamma;
    if (settings.gamma > 6)
        settings.gamma = 3;
    settings.gameWidth = rec->gameWidth;
    settings.gameHeight = rec->gameHeight;
    if (settings.gameHeight != 0 && settings.gameHeight < kMinResolutionHeight)
        settings.gameWidth = settings.gameHeight = 0;           /* under 768 lines: the desktop's */
    settings.scaleFilter = rec->scaleFilter;
    if (settings.scaleFilter < 0 || settings.scaleFilter >= kScaleFilterCount)
        settings.scaleFilter = kScaleFilterLinear;
    settings.stereoPan = rec->stereoPan != 0;
    settings.asteroidScaling = rec->asteroidScaling != 0;
    settings.mapPrices = rec->mapPrices != 0;
    settings.mapDisasters = rec->mapDisasters != 0;
    settings.runInWindow = rec->runInWindow != 0;
    settings.gameSize = rec->gameSize;
    if (settings.gameSize < kMinGameSize || settings.gameSize > kMaxGameSize)
        settings.gameSize = kDefaultGameSize;
    settings.wheelZoom = rec->wheelZoom != 0;
    settings.orBlend = rec->orBlend != 0;
    settings.uiScale = rec->uiScale > 0 ? rec->uiScale : 1.0;
    SetDefaultKeys();
    for (i = 0; i <= 35; i++)
        newKeyPrefs[i] = ScanCode(rec->keys[i]);
    SlotTableToKeyPrefs();
}

/* The defaults, with whatever the preferences file holds on top. */
// FUNCTION: EVNOVA 0x004c7400
void LoadPrefs(void)
{
    PrefsRecord rec;

    SetDefaultPrefs();
    SetDefaultKeys();
    PrefsToRecord(&rec);
    PrefsJsonLoad(PrefsPath().c_str(), &rec);
    RecordToPrefs(&rec);
}

// FUNCTION: EVNOVA 0x004c7820
void SavePrefs(void)
{
    PrefsRecord rec;

    PrefsToRecord(&rec);
    PrefsJsonSave(PrefsPath().c_str(), &rec);
}

/* The pilot file into "Last Pilot": its name when it lies in the pilots folder, else its whole path. */
// FUNCTION: EVNOVA 0x004c7d40
// MAC: _MakeAlias i386 0x00071e36 ppc 0x0007a9bc
void MakeAlias(const char *path)
{
    char base[0x100];
    const char *tail;
    const char *text;
    size_t folder;

    base[0] = 0;
    StrcatIndCString(base, kPilotDirList, 4);
    tail = PathTail(path);
    folder = tail > path ? (size_t)(tail - path - 1) : 0;
    text = path;
    if (tail > path && paths.pilots.compare(0, std::string::npos, path, folder) == 0)
        text = tail;
    SaveWholeFile(JoinPath(paths.pilots, base).c_str(), text, strlen(text) + 1);
}

// FUNCTION: EVNOVA 0x004c7db0
// MAC: _SavePilotFile i386 0x00076876 ppc 0x0007f6b4
short SavePilotFile(short stellar)
{
    if (noSavePilotFiles != 0)
        return 0;
    return WritePilotData(stellar);
}


// FUNCTION: EVNOVA 0x004c7dd0
// MAC: _WritePilotData i386 0x00071ffb ppc 0x0007ab1c
short WritePilotData(short stellar)
{
    char name[0x100];
    std::string path;
    FILE *f;
    short j;
    short i;

    if (noUpdatePilotData != 0)
        return 0;

    playerFileHand = NULL;
    altPlayerFileHand = NULL;
    path = PilotFilePath(playerName);
    playerFileHand = (PilotRec **)AllocAHandle2(sizeof(PilotRec), 0);
    altPlayerFileHand = (PilotRec2 **)AllocAHandle2(sizeof(PilotRec2), 0);
    f = PlatFileOpen(path.c_str(), "wb");
    if (f != NULL && playerFileHand != NULL && altPlayerFileHand != NULL) {
        LockAHandle((Handle)playerFileHand);
        LockAHandle((Handle)altPlayerFileHand);

        (*playerFileHand)->stellar = stellar;
        (*playerFileHand)->shipType = shipData[0].type;

        i = 0;
        do {
            (*playerFileHand)->cargo[i] = shipData[0].cargo[i];
        } while (++i < kCargoTypes);
        (*playerFileHand)->shield = shipData[0].shield;
        (*playerFileHand)->fuel = shipData[0].fuel;
        (*playerFileHand)->month = gameTime.month;
        (*playerFileHand)->day = gameTime.day;
        (*playerFileHand)->year = gameTime.year;
        for (i = 0; i < kMaxSystems; i++)
            (*playerFileHand)->explore[i] = systemData[i].exploreLevel;
        for (i = 0; i < kMaxOutfits; i++)
            (*playerFileHand)->outfits[i] = shipSystems[i];
        for (i = 0; i < kMaxSystems; i++)
            (*playerFileHand)->legalStatus[i] = playerRecord[i];
        for (i = 0; i < kMaxWeapTypes; i++) {
            (*playerFileHand)->weapCount[i] = shipData[0].weap[i].count;
            (*playerFileHand)->weapAmmo[i] = shipData[0].weap[i].ammo;
        }
        (*playerFileHand)->credits = shipData[0].credits;
        for (j = 0; j < kMaxActiveMissions; j++) {
            (*playerFileHand)->objectives[j] = currentMissionObjectives[j];
            (*playerFileHand)->missions[j] = currentMissions[j];
        }
        for (i = 0; i < kMaxMissionBits; i++)
            (*playerFileHand)->missionBits[i] = missionBit[i];
        for (i = 0; i < kMaxStellars; i++) {
            if (stellarData[i].prepared != 0)
                (*playerFileHand)->spobFlag[i] = stellarData[i].dominated;
            else
                (*playerFileHand)->spobFlag[i] = 0;
        }
        for (i = 0; i < kMaxShips; i++) {
            (*playerFileHand)->escortType[i] = -1;
            (*playerFileHand)->fighterType[i] = -1;
            (*playerFileHand)->fighterVoice[i] = -1;
            (*playerFileHand)->escortB[i] = 0;
            (*playerFileHand)->escortA[i] = 0;
        }
        for (i = 0; i < kMaxShips; i++) {
            if (shipData[i].active != 0 && shipData[i].parentShip == 0
                && shipData[i].aiType == kAIFighter)
                (*playerFileHand)->fighterType[i] = shipData[i].type;
            else
                (*playerFileHand)->fighterType[i] = -1;
        }
        for (i = 0; i < kMaxShips; i++) {
            if (shipData[i].active != 0 && shipData[i].parentShip == 0
                && shipData[i].aiType == kAIEscort && shipData[i].specialMission == -1) {
                (*playerFileHand)->escortType[i] = shipData[i].type;
                if (shipData[i].hired != 0)
                    (*playerFileHand)->escortType[i] += 1000;
                if (shipData[i].sellOnLanding != 0)
                    (*playerFileHand)->escortB[i] = 1;
                if (shipData[i].upgradeOnLanding != 0)
                    (*playerFileHand)->escortA[i] = 1;
            } else {
                (*playerFileHand)->escortType[i] = -1;
            }
        }
        for (i = 0; i < kMaxShips; i++) {
            if (shipData[i].active != 0 && shipData[i].parentShip == 0
                && AIHasLeader(shipData[i].aiType) && shipData[i].specialMission == -1)
                (*playerFileHand)->fighterVoice[i] = shipData[i].voiceClass;
        }
        (*playerFileHand)->kills = playerKills;

        (*altPlayerFileHand)->version = 300;
        if (strictPlay != 0)
            (*altPlayerFileHand)->strict = 1;
        else
            (*altPlayerFileHand)->strict = 0;
        (*altPlayerFileHand)->gender = playerGender;
        strncpy((*altPlayerFileHand)->nickname, (char *)playerNickname, 0x40);
        (*altPlayerFileHand)->paint = playerPaint;
        for (i = 0; i < kMaxStellars; i++) {
            if (stellarData[i].loaded != 0 && stellarData[i].dominated != 0) {
                (*altPlayerFileHand)->defWave[i] = stellarData[i].defWaveSize;
                (*altPlayerFileHand)->tribute[i] = stellarData[i].daysDominated;
            } else {
                (*altPlayerFileHand)->defWave[i] = stellarData[i].defWaveSize;
                (*altPlayerFileHand)->tribute[i] = 0;
            }
        }
        for (i = 0; i < kMaxPersons; i++) {
            if (personData[i].loaded != 0 && AIIsSet(personData[i].aiType)) {
                (*altPlayerFileHand)->persActive[i] = 1;
                if (personData[i].grudge != 0)
                    (*altPlayerFileHand)->persB[i] = 1;
                else
                    (*altPlayerFileHand)->persB[i] = 0;
            } else {
                (*altPlayerFileHand)->persActive[i] = 0;
                (*altPlayerFileHand)->persB[i] = 0;
            }
        }
        for (i = 0; i < kMaxRanks; i++) {
            if (rankData[i].active != 0)
                (*altPlayerFileHand)->rankOwned[i] = 1;
            else
                (*altPlayerFileHand)->rankOwned[i] = 0;
        }
        (*altPlayerFileHand)->seenIntro = seenIntroScreen;
        for (i = 0; i < kMaxDisasters; i++) {
            if (disasterData[i].loaded != 0) {
                (*altPlayerFileHand)->newsC[i] = disasterData[i].countdown;
                (*altPlayerFileHand)->newsB[i] = disasterData[i].stellarID;
            } else {
                (*altPlayerFileHand)->newsC[i] = 0;
                (*altPlayerFileHand)->newsB[i] = -1;
            }
        }
        for (i = 0; i < kMaxJunks; i++)
            (*altPlayerFileHand)->junkHeld[i] = junkData[i].held;
        for (i = 0; i < kMaxCrons; i++) {
            if (cronEvent[i].active != 0) {
                (*altPlayerFileHand)->cronA[i] = cronEvent[i].daysLeft;
                (*altPlayerFileHand)->cronB[i] = cronEvent[i].holdoffLeft;
            } else {
                (*altPlayerFileHand)->cronA[i] = -1;
                (*altPlayerFileHand)->cronB[i] = -1;
            }
        }
        for (i = 0; i < kMaxSystems; i++)
            (*altPlayerFileHand)->systState[i] = systemData[i].reinforceDaysLeft;
        for (i = 0; i < kMaxStellars; i++)
            (*altPlayerFileHand)->spobDead[i] = stellarData[i].deadDaysLeft;
        for (i = 0; i < 4; i++)
            (*altPlayerFileHand)->escortOrders[i] = playerEscortOrders[i];
        for (i = 0; i <= 1; i++) {
            (*altPlayerFileHand)->priceFlux[0][i] = globalPriceFlux[i];
            (*altPlayerFileHand)->priceFlux[1][i] = localPriceFlux[i];
        }
        strncpy((*altPlayerFileHand)->datePrefix, datePrefix, 0xf);
        strncpy((*altPlayerFileHand)->dateSuffix, dateSuffix, 0xf);
        (*altPlayerFileHand)->datePrefix[0xf] = 0;
        (*altPlayerFileHand)->dateSuffix[0xf] = 0;
        for (i = 0; i < 0x40; i++)
            (*altPlayerFileHand)->unk2006[i] = 0;
        for (i = 0; i < 0x400; i++)
            (*altPlayerFileHand)->unk5efe[i] = 0;

        PStr2CStr(name, shipName);
        LockAHandle((Handle)playerFileHand);
        LockAHandle((Handle)altPlayerFileHand);
        fwritelong(GetAHandleSize((Handle)playerFileHand), f);
        fwrite(*playerFileHand, GetAHandleSize((Handle)playerFileHand), 1, f);
        fwritelong(GetAHandleSize((Handle)altPlayerFileHand), f);
        fwrite(*altPlayerFileHand, GetAHandleSize((Handle)altPlayerFileHand), 1, f);
        fwritestr(name, f);
        DeallocAHandle(playerFileHand);
        DeallocAHandle(altPlayerFileHand);
        fclose(f);
    } else {
        crashlog("pilot not saved: cannot open or allocate");
        if (f != NULL)
            fclose(f);
        if (playerFileHand != NULL)
            DeallocAHandle(playerFileHand);
        if (altPlayerFileHand != NULL)
            DeallocAHandle(altPlayerFileHand);
    }
    playerFileHand = NULL;
    altPlayerFileHand = NULL;
    MakeAlias(path.c_str());
    return 0;
}


// FUNCTION: EVNOVA 0x004c9e90
// MAC: _OpenPilotFile i386 0x00076644 ppc 0x0007f498
Boolean OpenPilotFile(char *missingResource)
{
    char path[kPlatPathMax];
    unsigned char text[256];
    int32_t result;
    int gotFile;

    *missingResource = 0;

    /* open dialog: an existing file; "plt" added to a name typed without one */
    path[0] = 0;
    gotFile = PlatOpenFileDialog("EV Nova Pilot", "plt", paths.pilots.c_str(), path, sizeof(path));

    while (Win95ApplsActive == 0) {
        PlatSleep(0);
        KeyboardKbhit();
    }


    if (gotFile != 0) {

        crashlog("open file setup 1");
        DeleteAllComputerShips(1, 1);
        ResetPlayer(1);
        ResetPlayerRecord(1);
        DeleteAllComputerShips(1, 1);
        crashlog("loading pilot data");
        result = LoadPilotData(path);
        OSCursorReset();
        ShowTheCursor();
        crashlog("open file setup 2");

        if (result == 0) {
            shipData[0].deathTimer = -1.0f;
            shipData[0].escapePods = -1;
            return 1;
        }
        if (result == -0x2a) {
            GetIndString(text, kAlertStrList, 0x22);
            EVNoteAlert(text);
        } else if (result == -0x2e) {
            *missingResource = 1;
            return 1;
        }
        return 0;
    } else {
        OSCursorReset();
        ShowTheCursor();
        return 0;
    }
}

/* a whole native path: "/..." on Unix; "X:\...", "X:/..." or "\\..." on Windows */
static bool IsWholePath(const char *text)
{
#ifdef _WIN32
    if (((text[0] >= 'A' && text[0] <= 'Z') || (text[0] >= 'a' && text[0] <= 'z')) && text[1] == ':'
        && (text[2] == '\\' || text[2] == '/'))
        return true;
    return text[0] == '\\' && text[1] == '\\';
#else
    return text[0] == '/';
#endif
}

/* The pilot file "Last Pilot" names: a whole path or a file name in the pilots folder; "" for anything else. */
static std::string LastPilotPath(const char *text)
{
    if (IsWholePath(text))
        return text;
#ifdef _WIN32
    if (strpbrk(text, ":/\\") != NULL)
#else
    if (strchr(text, '/') != NULL)
#endif
        return "";
    return JoinPath(paths.pilots, text);
}

// FUNCTION: EVNOVA 0x004ca120
// MAC: _OpenLastPilotFile i386 0x000764ad ppc 0x0007f2fc
void OpenLastPilotFile(void)
{
    char name[256];
    std::string path;
    uint32_t len;
    char *data;
    short i;
    short j;

    name[0] = 0;
    StrcatIndCString(name, kPilotDirList, 4);
    data = (char *)LoadWholeFile(JoinPath(paths.pilots, name).c_str(), &len);
    if (data != NULL && (len == 0 || data[len - 1] != 0)) {
        data = (char *)ResizeAPointer(data, len + 1);
        data[len] = 0;
    }
    if (data != NULL)
        path = LastPilotPath(data);
    if (data != NULL && !path.empty() && DoesPilotExist(path.c_str()) != 0) {
        DeleteAllComputerShips(1, 1);
        ResetPlayer(1);
        ResetPlayerRecord(1);
        if (LoadPilotData(path.c_str()) == 0) {
            gameInProgress = 1;
            for (i = 0; i < kMaxSystems; i++) {
                if (systemData[i].loaded != 0 && systemData[i].exploreLevel > 0)
                    ExploreNebulaeFromSystem(i);
            }
            for (j = 0; j < kMaxActiveMissions; j++) {
                if (currentMissionObjectives[j].active != 0) {
                    if ((currentMissions[j].flags & kMissionInfiniteAuxShips) != 0)
                        currentMissions[j].auxShipsLeft = currentMissions[j].auxShipCount;
                    currentMissions[j].auxShipDelay = (Rand(0x46) + 0x46);
                    currentMissions[j].auxShipsSpawned = 0;
                }
            }
        }
    }
    if (data != NULL)
        DeallocAPointer(data);
}

/* CountEVPlugins counts+logs files of one extension in a directory (filescan.cpp) */

// FUNCTION: EVNOVA 0x004ca2c0
// MAC: _DumpPilotLog i386 0x000739dc ppc 0x0007c8b0
void DumpPilotLog(void)
{
    FILE *log;
    DateTimeRec dtr;
    PStrChar name[255];
    short i;
    short mi;
    short found;
    uint32_t contribute[2];

    log = PlatFileOpen("pilotlog.txt", "r");
    if (log == NULL)
        return;
    fclose(log);
    log = PlatFileOpen("pilotlog.txt", "w");
    if (log == NULL)
        return;

    fprintf(log, "EV Nova pilot data dump\n\n");
    GetTime(&dtr);
    fprintf(log, "Output on %i/%i/%i at ", dtr.month, dtr.day, dtr.year);
    if (dtr.hour < 12)
        fprintf(log, "%i:%02i AM", dtr.hour, dtr.minute);
    else if (dtr.hour == 12)
        fprintf(log, "%i:%02i PM", dtr.hour, dtr.minute);
    else
        fprintf(log, "%i:%02i PM", dtr.hour - 12, dtr.minute);
    fprintf(log, "\n\n");
    fprintf(log, "Game version:      %s", "1.0.A");
    fprintf(log, "\n\n");

    fprintf(log, "Plugins loaded:\n");
    if (CountEVPlugins(paths.plugins.c_str(), ".rez", log) == 0)
        fprintf(log, "  none\n");
    fprintf(log, "\n");

    pstrcpy((StringPtr)name, playerName);
    PtoCstr((StringPtr)name);
    fprintf(log, "Pilot name:        %s\n", name);
    pstrcpy((StringPtr)name, playerNickname);
    PtoCstr((StringPtr)name);
    fprintf(log, "Pilot nickname:    %s\n", name);
    fprintf(log, "Pilot gender:      %s\n", playerGender != 0 ? "male" : "female");
    fprintf(log, "Strict play:       %s\n", strictPlay != 0 ? "yes" : "no");
    fprintf(log, "Seen intro screen: %s\n", seenIntroScreen != 0 ? "yes" : "no");

    pstrcpy((StringPtr)name, shipName);
    PtoCstr((StringPtr)name);
    fprintf(log, "Ship name:         %s\n", name);

    pstrcpy((StringPtr)name, shipClass[shipData->type].name);
    PtoCstr((StringPtr)name);
    fprintf(log, "Ship type:         %s (%i)\n", name, shipData->type + 0x80);

    pstrcpy((StringPtr)name, systemData[shipData->curSystem].name);
    PtoCstr((StringPtr)name);
    fprintf(log, "Current system:    %s (%i)\n", name, shipData->curSystem + 0x80);

    fprintf(log, "Legal status:      %i (in system %i)\n",
            playerRecord[shipData->curSystem], shipData->curSystem + 0x80);

    fprintf(log, "Game date:         %i/%i/%i\n",
            gameTime.month, gameTime.day, gameTime.year);

    fprintf(log, "Fuel:              %0.2f\n", shipData->fuel);
    fprintf(log, "Cash:              %i\n", shipData->credits);
    fprintf(log, "Kills:             %i\n", playerKills);

    fprintf(log, "\nCurrent cargo:\n");
    found = 0;
    for (i = 0; i < kFirstJunkItem; i++) {
        if (shipData->cargo[i] > 0) {
            fprintf(log, "  commodity %i: %i\n", i, shipData->cargo[i]);
            found++;
        }
    }
    for (i = 0; i < kMaxJunks; i++) {
        if (junkData[i].held > 0) {
            fprintf(log, "  junk type %i: %i\n", i + 0x80, junkData[i].held);
            found++;
        }
    }
    if (found == 0)
        fprintf(log, "  none\n");

    fprintf(log, "\nActive missions:\n");
    found = 0;
    for (mi = 0; mi < kMaxActiveMissions; mi++) {
        if (currentMissionObjectives[mi].active != 0) {
            found++;
            pstrcpy((StringPtr)name, (unsigned char *)currentMissions[mi].name);
            PtoCstr((StringPtr)name);
            fprintf(log, "  %s (%i):\n", name, currentMissions[mi].misnIndex + 0x80);
            fprintf(log, "    travelObjComplete:        %s\n",
                    currentMissionObjectives[mi].returnDone != 0 ? "yes" : "no");
            fprintf(log, "    shipObjComplete:          %s\n",
                    currentMissionObjectives[mi].objectivesDone != 0 ? "yes" : "no");
            fprintf(log, "    missionFailed:            %s\n",
                    currentMissionObjectives[mi].failed != 0 ? "yes" : "no");
            fprintf(log, "    flags:                    %04X\n", currentMissions[mi].flags);
            fprintf(log, "    flags2:                   %04X\n", currentMissions[mi].flags2);
            fprintf(log, "    canAbort:                 %s\n",
                    currentMissions[mi].canAbort != 0 ? "yes" : "no");
            fprintf(log, "    timeLeft:                 %i\n", currentMissions[mi].timeLimit);
            fprintf(log, "    destStellar:              %i\n",
                    currentMissions[mi].travelStellar == -1 ? -1 : currentMissions[mi].travelStellar + 0x80);
            fprintf(log, "    returnStellar:            %i\n",
                    currentMissions[mi].returnStellar == -1 ? -1 : currentMissions[mi].returnStellar + 0x80);
            fprintf(log, "    initialShipCount:         %i\n", currentMissions[mi].initialShipCount);
            fprintf(log, "    specialShipCount:         %i\n", currentMissions[mi].shipCount);
            fprintf(log, "    specialShipDude:          %i\n",
                    currentMissions[mi].shipDude == -1 ? -1 : currentMissions[mi].shipDude + 0x80);
            fprintf(log, "    specialShipGoal:          %i\n", currentMissions[mi].shipGoal);
            fprintf(log, "    specialShipBehavior:      %i\n", currentMissions[mi].shipBehav);
            fprintf(log, "    specialShipStart:         %i\n", currentMissions[mi].shipStart);
            fprintf(log, "    specialShipSyst:          %i\n", currentMissions[mi].shipSyst);
            fprintf(log, "    specialShipsKilled:       %i\n", currentMissions[mi].specialShipsKilled);
            fprintf(log, "    specialShipsBoarded:      %i\n", currentMissions[mi].specialShipsBoarded);
            fprintf(log, "    specialShipsDisabled:     %i\n", currentMissions[mi].specialShipsDisabled);
            fprintf(log, "    specialShipsJumpedIn:     %i\n", currentMissions[mi].specialShipsJumpedIn);
            fprintf(log, "    specialShipsJumpedOut:    %i\n", currentMissions[mi].specialShipsJumpedOut);
            fprintf(log, "    specialShipNameIndex:     %i\n", currentMissions[mi].destStrIndex);
            fprintf(log, "    specialShipSubtitleIndex: %i\n", currentMissions[mi].shipStrIndex);
            fprintf(log, "    specialShipPreselectType: %i\n", currentMissions[mi].shipTypeIndex);
            fprintf(log, "    auxShipCount:             %i\n", currentMissions[mi].auxShipCount);
            fprintf(log, "    auxShipsLeft:             %i\n", currentMissions[mi].auxShipsLeft);
            fprintf(log, "    auxShipDude:              %i\n",
                    currentMissions[mi].auxShipDude == -1 ? -1 : currentMissions[mi].auxShipDude + 0x80);
            fprintf(log, "    auxShipSyst:              %i\n", currentMissions[mi].auxShipSyst);
            fprintf(log, "    cargoType:                %i\n", currentMissions[mi].cargoType);
            fprintf(log, "    cargoQty:                 %i\n", currentMissions[mi].cargoQty);
            fprintf(log, "    pickupMode:               %i\n", currentMissions[mi].pickupMode);
            fprintf(log, "    dropoffMode:              %i\n", currentMissions[mi].dropoffMode);
            fprintf(log, "    cargoLoaded:              %s\n",
                    currentMissions[mi].hasCargo != 0 ? "yes" : "no");
            fprintf(log, "    scanMask:                 %04X\n", currentMissions[mi].scanMask);
            fprintf(log, "    compGovt:                 %i\n",
                    currentMissions[mi].compGovt == -1 ? -1 : currentMissions[mi].compGovt + 0x80);
            fprintf(log, "    compReward:               %i\n", currentMissions[mi].compReward);
            fprintf(log, "    datePostIncrement:        %i\n", currentMissions[mi].datePostIncrement);
            fprintf(log, "    pay:                      %i\n", currentMissions[mi].payment);
            fprintf(log, "\n");
        }
    }
    if (found == 0)
        fprintf(log, "  none\n");

    fprintf(log, "\nMission bits currently set:\n");
    found = 0;
    for (i = 0; i < kMaxMissionBits; i++) {
        if (missionBit[i] != 0) {
            fprintf(log, "  b%i\n", i);
            found++;
        }
    }
    if (found == 0)
        fprintf(log, "  none\n");

    fprintf(log, "\nContribute bits:\n");
    GetPlayerContributeBits(&contribute[0], &contribute[1]);
    fprintf(log, "  0x%08X\n", contribute[0]);
    fprintf(log, "  0x%08X\n", contribute[1]);

    fprintf(log, "\nEscorts:\n");
    found = 0;
    for (i = 1; i < kMaxShips; i++) {
        if (shipData[i].active != 0 && shipData[i].parentShip == 0) {
            found++;
            pstrcpy((StringPtr)name, shipClass[shipData[i].type].name);
            PtoCstr((StringPtr)name);
            fprintf(log, "  %s (%i) - ", name, shipData[i].type + 0x80);
            if (shipData[i].aiType == kAIFighter)
                fprintf(log, "fighter");
            else if (shipData[i].specialMission != -1)
                fprintf(log, "mission escort");
            else if (shipData[i].hired != 0)
                fprintf(log, "hired");
            else
                fprintf(log, "captured");
            if (shipData[i].upgradeOnLanding != 0)
                fprintf(log, ", marked for upgrade");
            if (shipData[i].sellOnLanding != 0)
                fprintf(log, ", marked for sale");
            fprintf(log, "\n");
        }
    }
    if (found == 0)
        fprintf(log, "  none\n");

    fprintf(log, "\nItems currently owned:\n");
    found = 0;
    for (i = 0; i < kMaxOutfits; i++) {
        if (shipSystems[i] > 0) {
            pstrcpy((StringPtr)name, outfitSystem[i].name);
            PtoCstr((StringPtr)name);
            fprintf(log, "  %i %s (%i)\n", shipSystems[i], name, i + 0x80);
            found++;
        }
    }
    if (found == 0)
        fprintf(log, "  none\n");

    fprintf(log, "\nStellars currently dominated:\n");
    found = 0;
    for (i = 0; i < kMaxStellars; i++) {
        if (stellarData[i].prepared != 0 && stellarData[i].dominated != 0) {
            pstrcpy((StringPtr)name, stellarData[i].name);
            PtoCstr((StringPtr)name);
            fprintf(log, "  %s (%i)\n", name, i + 0x80);
            found++;
        }
    }
    if (found == 0)
        fprintf(log, "  none\n");

    fprintf(log, "\nStellars currently destroyed:\n");
    found = 0;
    for (i = 0; i < kMaxStellars; i++) {
        if (stellarData[i].prepared != 0 && StellarIsDestroyed(&stellarData[i])) {
            pstrcpy((StringPtr)name, stellarData[i].name);
            PtoCstr((StringPtr)name);
            fprintf(log, "  %s (%i)\n", name, i + 0x80);
            found++;
        }
    }
    if (found == 0)
        fprintf(log, "  none\n");

    fprintf(log, "\nActive ranks:\n");
    found = 0;
    for (i = 0; i < kMaxRanks; i++) {
        if (rankData[i].loaded != 0 && rankData[i].active != 0) {
            /* the exe passes the PASCAL string straight to %s here, with no
               pstrcpy/PtoCstr conversion (unlike every other name section) -
               read directly off the call site, not a guess */
            fprintf(log, "  %s (%i)\n", rankData[i].name, i + 0x80);
            found++;
        }
    }
    if (found == 0)
        fprintf(log, "  none\n");

    fprintf(log, "\nActive disasters:\n");
    found = 0;
    for (i = 0; i < kMaxDisasters; i++) {
        if (disasterData[i].loaded != 0 && disasterData[i].stellarID != -1) {
            pstrcpy((StringPtr)name, disasterData[i].name);
            PtoCstr((StringPtr)name);
            fprintf(log, "  %s (%i) on ", name, i + 0x80);
            pstrcpy((StringPtr)name, stellarData[disasterData[i].stellarID].name);
            PtoCstr((StringPtr)name);
            fprintf(log, "%s (%i) - %i days left\n",
                    name, disasterData[i].stellarID + 0x80, disasterData[i].countdown);
            found++;
        }
    }
    if (found == 0)
        fprintf(log, "  none\n");

    fprintf(log, "\nActive cron events:\n");
    found = 0;
    for (i = 0; i < kMaxCrons; i++) {
        if (cronEvent[i].loaded != 0 && cronEvent[i].active != 0) {
            fprintf(log, "  %i - durationCounter: %i\tholdoffCounter: %i\n",
                    i + 0x80, cronEvent[i].daysLeft, cronEvent[i].holdoffLeft);
            found++;
        }
    }
    if (found == 0)
        fprintf(log, "  none\n");

    fprintf(log, "\n\n- end of log -\n");
    fflush(log);
    fclose(log);
}

// FUNCTION: EVNOVA 0x004cb260
// MAC: _LoadPilotData i386 0x000750b5 ppc 0x0007dd04
short LoadPilotData(const char *path)
{
    char buf[0x40];
    char fileText[0x100];
    Str255 text;
    FILE *f;
    const char *tail;
    short i;
    short j;
    short k;
    short n;
    short err;
    uint32_t len;
    Boolean junk;

    playerFileHand = NULL;
    altPlayerFileHand = NULL;
    junk = 0;

    f = PlatFileOpen(path, "rb");
    if (f == NULL)
        goto badFile;

    crashlog("load pilot data 1");
    len = fgetlong(f);
    if (len > 0) {
        playerFileHand = (PilotRec **)AllocAHandle2(len, 0);
        LockAHandle((Handle)playerFileHand);
        fread(*playerFileHand, len, 1, f);
    }
    if (playerFileHand != NULL) {
        LockAHandle((Handle)playerFileHand);
        err = DecryptIfNecessary(*playerFileHand, GetAHandleSize((Handle)playerFileHand),
                                 kPilotCryptKey);

        if (err == 0) {
            crashlog("load pilot data 2");
            PropagateMissionBitEffects();
            HandleStellarSystemVisibility();

            k = (*playerFileHand)->stellar;
            shipData[0].lastStellar = k;
            if (k == -1) {
                junk = 1;
                crashlog("warning: pilot file stellar ID out of range");
                k = 0;
                for (i = 0; i < kMaxStellars; i++) {
                    if (stellarData[i].prepared != 0) {
                        k = i;
                        break;
                    }
                }
            }
            shipData[0].curSystem = stellarData[k].system;
            if (shipData[0].curSystem < 0 || shipData[0].curSystem >= kMaxSystems)
                shipData[0].curSystem = FindSystemFromStellar(k);
            else if (systemData[shipData[0].curSystem].prepared == 0)
                shipData[0].curSystem = -1;
            if (shipData[0].curSystem < 0 || shipData[0].curSystem > kMaxSystems) {
                junk = 1;
                crashlog("warning: pilot file system ID out of range");
                shipData[0].curSystem = 0;
                for (i = 0; i < kMaxSystems; i++) {
                    if (systemData[i].prepared != 0) {
                        shipData[0].curSystem = i;
                        break;
                    }
                }
            }
            PropagateMissionBitEffects();
            HandleStellarSystemVisibility();

            shipData[0].pos.x = stellarData[k].pos.h;
            shipData[0].pos.y = stellarData[k].pos.v;
            shipData[0].heading = Rand(kFullCircle);
            mapScroll.h = systemData[shipData[0].curSystem].pos.h;
            mapScroll.v = systemData[shipData[0].curSystem].pos.v;

            shipData[0].type = (*playerFileHand)->shipType;
            if (shipClass[shipData[0].type].techLevel == kResAbsent) {
                junk = 1;
                crashlog("warning: pilot file ship class out of range");
                shipData[0].type = 0;
                for (i = 0; i < kMaxShipTypes; i++) {
                    if (shipClass[i].techLevel != kResAbsent) {
                        shipData[0].type = i;
                        break;
                    }
                }
            }

            for (i = 0; i <= 5; i++)
                shipData[0].cargo[i] = (*playerFileHand)->cargo[i];

            for (i = 0; i < kMaxOutfits; i++) {
                shipSystems[i] = (*playerFileHand)->outfits[i];
                if (shipSystems[i] > 0 && outfitSystem[i].techLevel == kOutfTechAbsent) {
                    junk = 1;
                    crashlog_print("warning: pilot file has invalid outfit item ID ");
                    crashlog_print_num(i + kFirstResourceID);
                    crashlog_print("\n");
                    shipSystems[i] = 0;
                }
            }

            shipData[0].shield = shipData->ShieldCapacity();
            shipData[0].armor = shipData->ArmorCapacity();
            shipData[0].fuel = (*playerFileHand)->fuel;
            gameTime.month = (*playerFileHand)->month;
            gameTime.day = (*playerFileHand)->day;
            gameTime.year = (*playerFileHand)->year;

            for (i = 0; i < kMaxSystems; i++)
                systemData[i].exploreLevel = (*playerFileHand)->explore[i];
            for (i = 0; i < kMaxSystems; i++)
                playerRecord[i] = (*playerFileHand)->legalStatus[i];

            for (i = 0; i < kMaxWeapTypes; i++) {
                shipData[0].weap[i].count = (*playerFileHand)->weapCount[i];
                shipData[0].weap[i].ammo = (*playerFileHand)->weapAmmo[i];
                if (shipData[0].weap[i].count > 0 && weaponStats[i].guidance == kResAbsent) {
                    junk = 1;
                    crashlog_print("warning: pilot file has invalid weapon ID ");
                    crashlog_print_num(i + kFirstResourceID);
                    crashlog_print("\n");
                    shipData[0].weap[i].count = 0;
                }
                if (shipData[0].weap[i].ammo > 0 && weaponStats[i].guidance == kResAbsent) {
                    junk = 1;
                    crashlog_print("warning: pilot file has invalid weapon ammo ID ");
                    crashlog_print_num(i + kFirstResourceID);
                    crashlog_print("\n");
                    shipData[0].weap[i].ammo = 0;
                }
            }

            shipData[0].credits = (*playerFileHand)->credits;
            playerKills = (*playerFileHand)->kills;

            crashlog("load pilot data 3");
            for (i = 0; i < kMaxActiveMissions; i++) {
                currentMissionObjectives[i] = (*playerFileHand)->objectives[i];
                currentMissions[i] = (*playerFileHand)->missions[i];
                if (currentMissionObjectives[i].active != 0) {
                    SetMissionDeadline(&currentMissionObjectives[i].dueDate,
                                  currentMissions[i].timeLimit);
                    currentMissions[i].shipName[0] = 0;
                    currentMissions[i].shipSubtitle[0] = 0;
                    if (currentMissions[i].destStrList >= kFirstResourceID &&
                        currentMissions[i].destStrIndex >= 1) {
                        GetIndString(text, currentMissions[i].destStrList,
                                     currentMissions[i].destStrIndex);
                        CopyPascalNameSigned(currentMissions[i].shipName, text, 0x3f);
                    }
                    if (currentMissions[i].shipStrList >= kFirstResourceID &&
                        currentMissions[i].shipStrIndex >= 1) {
                        GetIndString(text, currentMissions[i].shipStrList,
                                     currentMissions[i].shipStrIndex);
                        CopyPascalNameSigned(currentMissions[i].shipSubtitle, text, 0x3f);
                    }
                    if ((currentMissions[i].flags & kMissionInfiniteAuxShips) != 0)
                        currentMissions[i].auxShipsLeft = currentMissions[i].auxShipCount;
                    currentMissions[i].auxShipDelay = (Rand(0x46) + 0x46);
                    currentMissions[i].auxShipsSpawned = 0;
                }
            }

            i = 0;
            do {
                missionBit[i] = (*playerFileHand)->missionBits[i];
            } while (++i < kMaxMissionBits);

            for (i = 0; i < kMaxStellars; i++) {
                if (stellarData[i].prepared != 0)
                    stellarData[i].dominated = (*playerFileHand)->spobFlag[i];
                else
                    stellarData[i].dominated = 0;
            }

            DeleteAllComputerShips(1, 1);

            for (i = 0; i < kMaxShips; i++) {
                if ((*playerFileHand)->escortType[i] >= 0) {
                    n = ((*playerFileHand)->escortType[i] % 1000);
                    if (shipClass[n].techLevel == kResAbsent) {
                        junk = 1;
                        crashlog("warning: pilot file has invalid escort ship type");
                    } else {
                        j = SpawnEscort(n, -1);
                        if (j != -1) {
                            if ((*playerFileHand)->escortType[i] < 1000) {
                                shipData[j].hired = 0;
                                if ((*playerFileHand)->escortB[i] != 0)
                                    shipData[j].sellOnLanding = 1;
                            } else {
                                shipData[j].hired = 1;
                            }
                            if ((*playerFileHand)->escortA[i] != 0)
                                shipData[j].upgradeOnLanding = 1;
                        }
                    }
                }
                if ((*playerFileHand)->fighterType[i] >= 0 &&
                    (*playerFileHand)->fighterType[i] < kMaxShipTypes) {
                    n = (*playerFileHand)->fighterType[i];
                    if (shipClass[n].techLevel == kResAbsent) {
                        junk = 1;
                        crashlog("warning: pilot file has invalid fighter ship type");
                    } else {
                        j = SpawnEscort(n, -1);
                        if (j != -1) {
                            shipData[j].aiType = kAIFighter;
                            shipData[j].hired = 0;
                            AISetup(&shipData[j]);
                            if ((*playerFileHand)->fighterVoice[i] != -1)
                                shipData[j].voiceClass = (*playerFileHand)->fighterVoice[i];
                        }
                    }
                }
            }

            DeallocAHandle(playerFileHand);
            crashlog("load pilot data 4");
        }
    } else {
        fclose(f);
        return kPilotUnreadable;
    }

    crashlog("load pilot data 5");
    len = fgetlong(f);
    if (len > 0) {
        altPlayerFileHand = (PilotRec2 **)AllocAHandle2(len, 0);
        LockAHandle((Handle)altPlayerFileHand);
        fread(*altPlayerFileHand, len, 1, f);
    }
    if (altPlayerFileHand != NULL) {
        LockAHandle((Handle)altPlayerFileHand);
        err = DecryptIfNecessary(*altPlayerFileHand,
                                 GetAHandleSize((Handle)altPlayerFileHand), kPilotCryptKey);

        if (err == 0) {
            crashlog("load pilot data 6");
            if ((*altPlayerFileHand)->version == kBetaPilotVersion) {
                DeallocAHandle(altPlayerFileHand);
                fclose(f);
                return kPilotBetaFile;
            }
            if ((*altPlayerFileHand)->version < kMinPilotVersion) {
                DeallocAHandle(altPlayerFileHand);
                fclose(f);
                return kPilotWrongVersion;
            }

            if ((*altPlayerFileHand)->strict == 1)
                strictPlay = 1;
            else
                strictPlay = 0;
            if ((*altPlayerFileHand)->gender == 1)
                playerGender = 1;
            else
                playerGender = 0;

            strncpy((char *)playerNickname, (*altPlayerFileHand)->nickname, 0x40);
            playerPaint = (*altPlayerFileHand)->paint;

            for (i = 0; i < kMaxStellars; i++) {
                if (stellarData[i].prepared != 0 && stellarData[i].dominated == 0) {
                    stellarData[i].defWaveSize = (*altPlayerFileHand)->defWave[i];
                    stellarData[i].daysDominated = (*altPlayerFileHand)->tribute[i];
                } else {
                    stellarData[i].defWaveSize = 0;
                    stellarData[i].daysDominated = 0;
                }
            }

            for (i = 0; i < kMaxPersons; i++) {
                if (AIIsSet(personData[i].aiType)) {
                    if (personData[i].prepared != 0 &&
                        (*altPlayerFileHand)->persActive[i] != 0) {
                        personData[i].loaded = 1;
                        if ((*altPlayerFileHand)->persB[i] != 0)
                            personData[i].grudge = 1;
                        else
                            personData[i].grudge = 0;
                    } else {
                        personData[i].loaded = 0;
                        personData[i].grudge = 0;
                    }
                } else {
                    personData[i].loaded = 0;
                }
            }

            for (i = 0; i < kMaxRanks; i++) {
                if ((*altPlayerFileHand)->rankOwned[i] == 0) {
                    rankData[i].active = 0;
                } else if (rankData[i].loaded != 0) {
                    rankData[i].active = 1;
                } else {
                    junk = 1;
                    crashlog("warning: pilot file contains invalid rank");
                    rankData[i].active = 0;
                }
            }

            crashlog("load pilot data 7");
            seenIntroScreen = (*altPlayerFileHand)->seenIntro;
            lastMissionStellar = -1;

            for (i = 0; i < kMaxDisasters; i++) {
                if (disasterData[i].loaded != 0) {
                    disasterData[i].countdown = (*altPlayerFileHand)->newsC[i];
                    disasterData[i].stellarID = (*altPlayerFileHand)->newsB[i];
                } else {
                    disasterData[i].countdown = 0;
                    disasterData[i].stellarID = -1;
                }
            }

            for (i = 0; i < kMaxJunks; i++) {
                junkData[i].held = (*altPlayerFileHand)->junkHeld[i];
                if (junkData[i].held > 0 && junkData[i].basePrice < 0) {
                    junk = 1;
                    crashlog("warning: pilot file contains invalid junk type");
                    junkData[i].held = 0;
                }
            }

            for (i = 0; i < kMaxCrons; i++) {
                if (cronEvent[i].loaded != 0) {
                    cronEvent[i].daysLeft = (*altPlayerFileHand)->cronA[i];
                    cronEvent[i].holdoffLeft = (*altPlayerFileHand)->cronB[i];
                    if (cronEvent[i].daysLeft >= 0 || cronEvent[i].holdoffLeft >= 0)
                        cronEvent[i].active = 1;
                    else
                        cronEvent[i].active = 0;
                } else {
                    cronEvent[i].active = 0;
                }
            }

            i = 0;
            do {
                systemData[i].reinforceDaysLeft = (*altPlayerFileHand)->systState[i];
            } while (++i < kMaxSystems);

            for (i = 0; i < kMaxStellars; i++) {
                if ((*altPlayerFileHand)->spobDead[i] > 0) {
                    stellarData[i].deadDaysLeft = (*altPlayerFileHand)->spobDead[i];
                    stellarData[i].deadCounter = -1;
                } else {
                    stellarData[i].deadDaysLeft = -1;
                    stellarData[i].deadCounter = stellarData[i].strength;
                }
            }

            for (i = 0; i < 4; i++) {
                playerEscortOrders[i] = -1;
                if ((*altPlayerFileHand)->escortOrders[i] >= 0)
                    playerEscortOrders[i] = (*altPlayerFileHand)->escortOrders[i];
            }

            for (i = 1; i < kMaxShips; i++) {
                if (shipData[i].active != 0 && shipData[i].parentShip == 0)
                    shipData[i].escortOrder =
                        playerEscortOrders[shipClass[shipData[i].type].escortType];
            }

            i = 0;
            do {
                globalPriceFlux[i] = (*altPlayerFileHand)->priceFlux[0][i];
                localPriceFlux[i] = (*altPlayerFileHand)->priceFlux[1][i];
            } while (++i < 2);

            strncpy(datePrefix, (*altPlayerFileHand)->datePrefix, 0xf);
            strncpy(dateSuffix, (*altPlayerFileHand)->dateSuffix, 0xf);
            datePrefix[0xf] = 0;
            dateSuffix[0xf] = 0;

            DeallocAHandle(altPlayerFileHand);
            crashlog("load pilot data 8");
        }
    } else {
        fclose(f);
        return kPilotUnreadable;
    }

    crashlog("load pilot data 9");
    fgetstr(buf, 0x40, f);
    CStr2PStr((char *)shipName, buf);

    tail = PathTail(path);
    Utf8ToText(fileText, tail, sizeof(fileText));
    strncpy((char *)playerName, fileText, 0x1f);
    if (strrchr((char *)playerName, '.'))
        *strrchr((char *)playerName, '.') = 0;
    CStr2PStr((char *)playerName, (char *)playerName);

    crashlog("making alias");
    MakeAlias(path);

    shipData[0].foldDir = 0;
    shipData[0].frameTimer = 0;
    shipData[0].foldSet = shipClass[shipData[0].type].baseSets - 1;
    shipData[0].shieldGlow = 0;
    shipData[0].weapGlow = 0;
    shipData[0].annoyance = 0;

    for (i = 1; i < kMaxShips; i++) {
        if (shipData[i].active != 0 && shipData[i].parentShip == 0) {
            if ((shipClass[shipData[i].type].shanFlags & kShanUnfoldsWhenFiring) != 0)
                shipData[i].foldSet =
                    shipClass[shipData[0].type].baseSets - 1;
            else
                shipData[i].foldSet = 0;
        }
    }

    for (i = 0; i < kMaxShipTypes; i++) {
        shipClass[i].dailyHireRoll = (Rand(kDailyRollRange) + 1);
        shipClass[i].dailySaleRoll = (Rand(kDailyRollRange) + 1);
    }
    for (i = 0; i < kMaxOutfits; i++)
        outfitSystem[i].dailyRoll = (Rand(kDailyRollRange) + 1);

    crashlog("dumping pilot log");
    DumpPilotLog();

    fclose(f);
    return junk != 0 ? kPilotHasJunk : kPilotOK;

badFile:
    return kPilotUnreadable;
}

// FUNCTION: EVNOVA 0x004cd030
// MAC: _DoesPilotExist i386 0x00072def ppc 0x0007bc7c
Boolean DoesPilotExist(const char *path)
{
    return PlatPathKind(path, NULL) == kPlatFile;
}

// FUNCTION: EVNOVA 0x004cd040
// MAC: _DeletePilotFile i386 0x00072e6b ppc 0x0007bcec
void DeletePilotFile(const unsigned char *pname)
{
    std::string path = PilotFilePath(pname);

    if (!DoesPilotExist(path.c_str()))
        return;
    PlatRemove(path.c_str());
}

#define kEBugLastField 0x1f
typedef struct {
    short flag[kEBugLastField + 1];
} EBugRec;

#define kCharRecordSize 0x16a

/* one debug switch per field index; absent resource or out-of-range index = off */
// FUNCTION: EVNOVA 0x004cd0b0
// MAC: _EVBugResourceFieldTest i386 0x0007132c ppc 0x00079d48
Boolean EVBugResourceFieldTest(short field)
{
    Handle h;
    Boolean on;

    on = 0;
    if (field < 0 || field > kEBugLastField)
        return 0;
    h = GetResource(kEBugType, 128);
    if (h != NULL) {
        if ((*(EBugRec **)h)->flag[field] != 0)
            on = 1;
        ReleaseResource(h);
    }
    return on;
}

/* The memset is inlined by mwcc (rep stosb): it needs -opt level=4,intrinsics with <string.h>
   included. */

// FUNCTION: EVNOVA 0x004cd100
// MAC: _AdjustHandleSize i386 0x00076bca ppc 0x0007f9fc
void AdjustHandleSize(Handle h, int32_t newSize)
{
    int32_t size;

    if (h == NULL)
        return;
    size = GetAHandleSize(h);
    if (size != newSize) {
        ResizeAHandle(h, newSize);
        if (size < newSize)
            memset(*h + size, 0, newSize - size);
    }
}

/* leaves the buffer empty if the list is missing or has no entries */
// FUNCTION: EVNOVA 0x004cd140
// MAC: _GetRandomIndString i386 0x00072ef7 ppc 0x0007bd74
void GetRandomIndString(unsigned char *str, short strListID)
{
    Handle h;
    short count;

    str[0] = 0;
    h = GetResource(kStringListType, strListID);
    if (h != NULL) {
        count = SwapEndianShort(&ResRec(StringListRec, h)->count);
        ReleaseResource(h);
        if (count > 0)
            GetIndString(str, strListID, Rand(count) + 1);
    }
}

// FUNCTION: EVNOVA 0x004cd1a0
// MAC: _StrcatIndCString i386 0x00072f90 ppc 0x0007be0c
void StrcatIndCString(char *dst, short strListID, short index)
{
    strcat(dst, IndString(strListID, index).c_str());
}


/*
 * Pascal resource names carry a ';'-introduced comment (Nova Bible
 * convention); truncate at the last one and trim the spaces before it.
 */
// FUNCTION: EVNOVA 0x004cd230
// MAC: _TrimResourceName i386 0x000717ac ppc 0x0007a208
void TrimResourceName(unsigned char *name)
{
    short i;
    short len;
    Boolean seen;
    int c;

    seen = 0;
    len = *name;
    for (i = *name; i >= 1; i--) {
        if (seen == 0) {
            c = name[i];
            if ((unsigned)c == ';') {
                seen = 1;
                len = i - 1;
            }
        } else {
            c = name[i];
            if ((unsigned)c == ' ') {
                len = i - 1;
            } else if ((unsigned)c != ';') {
                len = i;
                break;
            }
        }
    }
    *name = len;
}

/* the New Pilot screen starts on whichever character carries the flag */
// FUNCTION: EVNOVA 0x004cd290
// MAC: _GetDefaultCharResourceName i386 0x00077100 ppc 0x00080030
void GetDefaultCharResourceName(unsigned char *name)
{
    short i;
    short count;
    short id;
    uint32_t type;

    count = CountResources(kCharType);
    pstrcpy(name, (unsigned char *)"");
    for (i = 0; i < count; i++) {
        if (name[0] == 0) {
            /* cleared first so a failed load leaves no stale handle behind */
            charFileHand = NULL;
            charFileHand = GetIndResource(kCharType, (i + 1));
            if (charFileHand != NULL) {
                if ((ResRec(CharRecord, charFileHand)->flags & kCharIsDefault) != 0)
                    GetResInfo(charFileHand, &id, &type, name);
                AdjustHandleSize(charFileHand, kCharRecordSize);
                ReleaseResource(charFileHand);
            }
        }
    }
}

#define kIntroSlides    4
#define kMaxIntroDelay  300

// FUNCTION: EVNOVA 0x004cd350
// MAC: _GetCharResourceShipType i386 0x000770a1 ppc 0x0007ffb4
short GetCharResourceShipType(const unsigned char *name)
{
    short index;

    charFileHand = GetNamedResource(kCharType, name);
    if (charFileHand != NULL) {
        index = ResRec(CharRecord, charFileHand)->shipType;
        if (index >= kFirstResourceID)
            index -= kFirstResourceID;
        else
            index = 0;
        AdjustHandleSize(charFileHand, kCharRecordSize);
        ReleaseResource(charFileHand);
    } else {
        index = 0;
    }
    return index;
}

// FUNCTION: EVNOVA 0x004cd3b0
// MAC: _GetCharResourceIntroIDs i386 0x00076fa9 ppc 0x0007fe5c
void GetCharResourceIntroIDs(const unsigned char *name, short *introText, short *pics,
                  short *delays)
{
    short i;

    charFileHand = GetNamedResource(kCharType, name);
    if (charFileHand != NULL) {
        *introText = ResRec(CharRecord, charFileHand)->introTextID;
        for (i = 0; i < kIntroSlides; i++) {
            pics[i] = ResRec(CharRecord, charFileHand)->introPICT[i];
            delays[i] = ResRec(CharRecord, charFileHand)->introDelay[i];
            if (pics[i] >= kFirstResourceID) {
                if (delays[i] < 0)
                    delays[i] = 0;
                if (delays[i] > kMaxIntroDelay)
                    delays[i] = kMaxIntroDelay;
            } else {
                pics[i] = -1;
                delays[i] = 0;
            }
        }
        AdjustHandleSize(charFileHand, kCharRecordSize);
        ReleaseResource(charFileHand);
    } else {
        *introText = 0x7ffd;
        pics[0] = 0x2008;
        delays[0] = 10;
        for (i = 1; i < kIntroSlides; i++) {
            pics[i] = -1;
            delays[i] = 0;
        }
    }
}

#define kStartSystSlots       4
#define kGovtSlots            4
#define kFallbackCredits 0x2710
#define kFallbackDate     0x8ca

// FUNCTION: EVNOVA 0x004cd4b0
// MAC: _ActivateCharResource i386 0x00076cb6 ppc 0x0007faf0
void ActivateCharResource(const unsigned char *name, Boolean applyStats)
{
    short i;
    short n;
    short pick;
    short govt;
    CharRecord **cr;
    CharRecord **startInfo;
    short filled;

    charFileHand = GetNamedResource(kCharType, name);
    if (charFileHand != NULL) {
    LockAHandle(charFileHand);
    if (applyStats != 0) {
        shipData->credits = ResRec(CharRecord, charFileHand)->cash;
        if (shipData->credits < 0)
            shipData->credits = 0;
        shipData->type = ResRec(CharRecord, charFileHand)->shipType;
        if (shipData->type >= kFirstResourceID)
            shipData->type -= kFirstResourceID;
        else
            shipData->type = 0;
        cr = (CharRecord **)charFileHand;
        playerKills = (*cr)->kills;

        filled = 0;
        for (i = 0; i < kStartSystSlots; i++)
            if ((*cr)->startSyst[i] >= kFirstResourceID)
                filled++;
        if (filled > 0) {
            do {
                pick = Rand(kStartSystSlots);
            } while (ResRec(CharRecord, charFileHand)->startSyst[pick] < kFirstResourceID);
            shipData->curSystem =
                (ResRec(CharRecord, charFileHand)->startSyst[pick] - kFirstResourceID);
        } else {
            shipData->curSystem = 0;
        }

        for (i = 0; i < kGovtSlots; i++) {
            govt = ResRec(CharRecord, charFileHand)->govt[i];
            if (govt >= kFirstResourceID) {
                govt -= kFirstResourceID;
                for (n = 0; n < kMaxSystems; n++) {
                    if (GovtAllies(systemData[n].govt, govt))
                        playerRecord[n] =
                            ResRec(CharRecord, charFileHand)->standing[i];
                    else if (GovtEnemies(systemData[n].govt, govt))
                        playerRecord[n] =
                            -ResRec(CharRecord, charFileHand)->standing[i];
                }
            }
        }

        startInfo = (CharRecord **)charFileHand;
        scenarioStartDate.day = (*startInfo)->startDay;
        scenarioStartDate.month = (*startInfo)->startMonth;
        scenarioStartDate.year = (*startInfo)->startYear;
        CopyCField(datePrefix, (*startInfo)->prefix);
        CopyCField(dateSuffix, ResRec(CharRecord, charFileHand)->suffix);
    } else {
        crashlog_mission_bit_set("starting character", (char *)name, 1, -1,
                                 ResRec(CharRecord, charFileHand)->onStart);
        EvalMissionBitSetString(ResRec(CharRecord, charFileHand)->onStart);
    }

    AdjustHandleSize(charFileHand, kCharRecordSize);
    ReleaseResource(charFileHand);
    } else {
        shipData->credits = kFallbackCredits;
        shipData->type = 0;
        shipData->curSystem = 0;
        playerKills = 0;
        scenarioStartDate.day = 1;
        scenarioStartDate.month = 1;
        scenarioStartDate.year = kFallbackDate;
        strcpy(datePrefix, "");
        strcpy(dateSuffix, "");
    }
}