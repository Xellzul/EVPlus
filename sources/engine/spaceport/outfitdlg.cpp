/* outfitdlg.cpp -- the outfitter dialog */

#include "game.h"
#include "stellar.h"
#include "crashlog.h"
#include "prototypes.h"
#include "gameutil.h"
#include "missions.h"
#include "player.h"
#include "render.h"
#include "ships.h"
#include "sound.h"
#include "gamedialog.h"
#include "buttons.h"

#include <math.h>
#include <stdarg.h>
#include <stdio.h>

DialogPtr outfitDialog;
char dialogDescTextPtr[8192];
short selOutfitItem;
short selOutfitListItem;
short outfitScroll;
PicHandle outfitBackgroundPic;
PicHandle previewPic;
Boolean canBuy;
Boolean canSell;
Boolean boughtMap;
Boolean boughtID;
Boolean portBuysNoOutfits;
Rect listScreenRect[kGridCells];
Rect listPicScreenRect[kGridCells];
short previewPicID;
RGBColor shopGridColor[2];

#define kOutfitDescBase 3000    /* desc ID of outfit 0's description */
#define kOutfitPictBase 6000    /* PICT ID of outfit 0's picture */

/* the item grid: four across, five down, out of item 5's box */


#define kOutfitDlogID    0x3ea
#define kOutfitPictID    0x2136
#define kOutfitVenue     6


namespace {

constexpr double kFramesPerSecond = 30.0;
constexpr int kDetailsSize = sizeof(dialogDescTextPtr) + 4096;
constexpr int kMaxSpans = 32;
constexpr int kMaxNamesListed = 4;

/* the description box's scroll in pixel rows, and its limit and step from the last draw */
int descScroll;
short descScrollOutfit = -1;
int descMaxScroll;
int descLineHeight;

/* the characters a 64-byte Pascal name holds */
int NameLength(const unsigned char *name)
{
    return name[0] > 62 ? 62 : name[0];
}

/* a Pascal name as a C string */
void NameOf(const unsigned char *name, char *out, int size)
{
    int n;

    n = NameLength(name);
    if (n > size - 1)
        n = size - 1;
    memcpy(out, name + 1, n);
    out[n] = 0;
}

bool OutfitLoaded(short outfit)
{
    return outfit >= 0 && outfit < kMaxOutfits && outfitSystem[outfit].techLevel < kOutfTechAbsent;
}

bool WeaponLoaded(short weap)
{
    return weap >= 0 && weap < kMaxWeapTypes && weaponStats[weap].guidance != kResAbsent;
}

bool IsBeam(short guidance)
{
    return guidance == kGuidanceBeam || guidance == kGuidanceTurretBeam || guidance == kGuidancePDBeam;
}

const char *GuidanceName(short guidance)
{
    switch (guidance) {
    case kGuidanceUnguided: return "unguided";
    case kGuidanceBeam: return "beam";
    case kGuidanceHoming: return "homing";
    case kGuidanceTurretBeam: return "turret beam";
    case kGuidanceTurret: return "turret";
    case kGuidanceFreefall: return "freefall bomb";
    case kGuidanceRocket: return "rocket";
    case kGuidanceFrontQuad: return "front quadrant turret";
    case kGuidanceRearQuad: return "rear quadrant turret";
    case kGuidancePointDef: return "point defence turret";
    case kGuidancePDBeam: return "point defence beam";
    case kGuidanceBay: return "fighter bay";
    default: return NULL;
    }
}

/* `value' to `decimals' places with trailing zeros dropped, signed when `sign' */
const char *Num(char *out, int size, double value, int decimals, bool sign)
{
    double scale;
    char *dot;
    char *p;

    scale = pow(10.0, decimals);
    value = floor(value * scale + 0.5) / scale;
    snprintf(out, size, sign ? "%+.*f" : "%.*f", decimals, value);
    dot = strchr(out, '.');
    if (dot != NULL) {
        p = out + strlen(out) - 1;
        while (p > dot && *p == '0')
            *p-- = 0;
        if (p == dot)
            *p = 0;
    }
    return out;
}

const RGBColor *SignColor(double value)
{
    if (value > 0)
        return &betterRGB;
    if (value < 0)
        return &worseRGB;
    return NULL;
}

/* green when lower, red when higher */
const RGBColor *LowerBetterColor(double value)
{
    return SignColor(-value);
}

/* the first n bytes equal, ASCII letters in either case */
bool EqualFold(const char *a, const char *b, uint32_t n)
{
    unsigned char x;
    unsigned char y;
    uint32_t i;

    for (i = 0; i < n; i++) {
        x = (unsigned char)a[i];
        y = (unsigned char)b[i];
        if (x >= 'A' && x <= 'Z')
            x += 'a' - 'A';
        if (y >= 'A' && y <= 'Z')
            y += 'a' - 'A';
        if (x != y)
            return false;
    }
    return true;
}

bool SameName(const unsigned char *a, const unsigned char *b)
{
    return NameLength(a) == NameLength(b) && memcmp(a + 1, b + 1, NameLength(a)) == 0;
}

/* The selected outfit's description with its unmet "Requires:" parts in red, then one line per effect of
   buying one, coloured by whether it helps. */
class OutfitDetails {
public:
    void Build(short outfit);
    void Plain(const char *desc);
    const char *Text() const { return text; }
    uint32_t Length() const { return length; }
    const TextSpan *Spans() const { return spans; }
    int SpanCount() const { return spanCount; }

private:
    void Append(const char *s, int n);
    void AddSpan(uint32_t start, uint32_t end, const RGBColor *color);
    void Line(const RGBColor *color, const char *format, ...);
    void ColorRequires(short outfit);
    void ColorPart(short outfit, uint32_t start, uint32_t end, const uint32_t *have);
    void Effect(short outfit, short slot);
    void Weapon(short weap);
    void AmmoFor(short weap);

    char text[kDetailsSize];
    uint32_t length;
    TextSpan spans[kMaxSpans];
    int spanCount;
    bool hasLines;
};

void OutfitDetails::Append(const char *s, int n)
{
    if (n > (int)(sizeof(text) - 1 - length))
        n = sizeof(text) - 1 - length;
    memcpy(text + length, s, n);
    length += n;
    text[length] = 0;
}

void OutfitDetails::AddSpan(uint32_t start, uint32_t end, const RGBColor *color)
{
    if (color == NULL || end <= start || spanCount >= kMaxSpans)
        return;
    spans[spanCount].start = start;
    spans[spanCount].end = end;
    spans[spanCount].color = *color;
    spanCount++;
}

void OutfitDetails::Line(const RGBColor *color, const char *format, ...)
{
    char line[256];
    va_list args;
    uint32_t start;

    va_start(args, format);
    vsnprintf(line, sizeof(line), format, args);
    va_end(args);
    if (length > 0)
        Append(hasLines ? "\r" : "\r\r", hasLines ? 1 : 2);
    hasLines = true;
    start = length;
    Append(line, strlen(line));
    AddSpan(start, length, color);
}

void OutfitDetails::Plain(const char *desc)
{
    length = 0;
    text[0] = 0;
    spanCount = 0;
    hasLines = false;
    Append(desc, strlen(desc));
}

void OutfitDetails::Build(short outfit)
{
    short slot;

    Plain(dialogDescTextPtr);
    while (length > 0 && (text[length - 1] == '\r' || text[length - 1] == '\n' || text[length - 1] == ' '))
        text[--length] = 0;
    ColorRequires(outfit);
    if (outfitSystem[outfit].mass < 0)
        Line(&betterRGB, "%+d %s free space", -outfitSystem[outfit].mass,
             outfitSystem[outfit].mass == -1 ? "ton" : "tons");
    for (slot = 0; slot < kOutfMods; slot++)
        Effect(outfit, slot);
}

/* each ", "-separated part of a line starting "Requires:" that names a licence outfit */
void OutfitDetails::ColorRequires(short outfit)
{
    uint32_t have[2];
    uint32_t pos;
    uint32_t start;
    uint32_t end;
    uint32_t stop;

    GetPlayerContributeBits(&have[0], &have[1]);
    for (pos = 0; pos < length; pos++) {
        if (pos > 0 && text[pos - 1] != '\r' && text[pos - 1] != '\n')
            continue;
        if (length - pos < 9 || !EqualFold(text + pos, "requires:", 9))
            continue;
        stop = pos + 9;
        while (stop < length && text[stop] != '\r' && text[stop] != '\n')
            stop++;
        start = pos + 9;
        while (start < stop) {
            end = start;
            while (end < stop && text[end] != ',')
                end++;
            ColorPart(outfit, start, end, have);
            start = end + 1;
        }
        pos = stop;
    }
}

void OutfitDetails::ColorPart(short outfit, uint32_t start, uint32_t end, const uint32_t *have)
{
    uint32_t contrib[2];
    uint32_t need[2];
    uint32_t n;
    short i;
    bool found;

    while (start < end && text[start] == ' ')
        start++;
    while (end > start && (text[end - 1] == ' ' || text[end - 1] == '.'))
        end--;
    if (end <= start)
        return;
    contrib[0] = contrib[1] = 0;
    found = false;
    for (i = 0; i < kMaxOutfits; i++) {
        if (!OutfitLoaded(i))
            continue;
        n = NameLength(outfitSystem[i].name);
        if (n == 0 || n != end - start || !EqualFold((const char *)outfitSystem[i].name + 1, text + start, n))
            continue;
        contrib[0] |= outfitSystem[i].contributes[0];
        contrib[1] |= outfitSystem[i].contributes[1];
        found = true;
    }
    if (!found)
        return;
    need[0] = contrib[0] & outfitSystem[outfit].requires[0];
    need[1] = contrib[1] & outfitSystem[outfit].requires[1];
    if (need[0] == 0 && need[1] == 0)
        return;
    if ((have[0] & need[0]) != need[0] || (have[1] & need[1]) != need[1])
        AddSpan(start, end, &worseRGB);
}

/* up to kMaxNamesListed outfit names, ", "-separated */
void NameList(char *out, int size, const short *list, int count, int total)
{
    char name[64];
    int i;

    out[0] = 0;
    for (i = 0; i < count; i++) {
        NameOf(outfitSystem[list[i]].name, name, sizeof(name));
        if (i > 0)
            strncat(out, ", ", size - strlen(out) - 1);
        strncat(out, name, size - strlen(out) - 1);
    }
    if (total > count)
        strncat(out, ", ...", size - strlen(out) - 1);
}

/* the launchers this ammunition loads, or the bays this fighter flies from */
void OutfitDetails::AmmoFor(short weap)
{
    short list[kMaxNamesListed];
    char names[256];
    int count;
    int total;
    short w;
    short i;
    int k;
    bool bay;

    if (!WeaponLoaded(weap))
        return;
    bay = weaponStats[weap].guidance == kGuidanceBay;
    count = total = 0;
    for (i = 0; i < kMaxOutfits; i++) {
        if (!OutfitLoaded(i) || outfitSystem[i].modType[0] != OutfMod::Weapon)
            continue;
        w = outfitSystem[i].modVal[0];
        if (!WeaponLoaded(w))
            continue;
        if (bay ? w != weap : (weaponStats[w].guidance == kGuidanceBay || weaponStats[w].ammoType != weap))
            continue;
        for (k = 0; k < count; k++) {
            if (SameName(outfitSystem[list[k]].name, outfitSystem[i].name))
                break;
        }
        if (k < count)
            continue;
        total++;
        if (count < kMaxNamesListed)
            list[count++] = i;
    }
    if (count == 0)
        return;
    NameList(names, sizeof(names), list, count, total);
    Line(NULL, bay ? "Fighter for: %s" : "Ammunition for: %s", names);
}

void OutfitDetails::Weapon(short weap)
{
    const WeapRecord *w;
    const char *guidance;
    char a[32];
    char b[32];
    char name[64];
    short ammo;
    short ship;
    short i;
    bool beam;
    double factor;
    double fuel;

    if (!WeaponLoaded(weap))
        return;
    w = &weaponStats[weap];
    guidance = GuidanceName(w->guidance);
    if (w->guidance == kGuidanceBay) {
        if (guidance != NULL)
            Line(NULL, "Guidance: %s", guidance);
        ship = w->ammoType - kFirstResourceID;
        if (ship >= 0 && ship < kMaxShipTypes && shipClass[ship].techLevel != kResAbsent
            && shipClass[ship].techLevel != kResUnloaded) {
            NameOf(shipClass[ship].name, name, sizeof(name));
            if (w->maxAmmo > 0)
                Line(NULL, "Launches: %s, holds %d", name, w->maxAmmo);
            else
                Line(NULL, "Launches: %s", name);
        }
        return;
    }
    beam = IsBeam(w->guidance);
    factor = beam && w->lifetime > 1 ? w->lifetime : 1.0;     /* a beam hits every frame it lasts */
    if (w->damage[0] > 0 || w->damage[1] > 0)
        Line(NULL, "Damage: %s armor / %s shield", Num(a, sizeof(a), w->damage[0] * factor, 2, false),
             Num(b, sizeof(b), w->damage[1] * factor, 2, false));
    Num(a, sizeof(a), w->reload <= 1.0f ? kFramesPerSecond : kFramesPerSecond / w->reload, 2, false);
    Line(NULL, "Fire rate: %s %s/s", a, strcmp(a, "1") == 0 ? "shot" : "shots");
    if (w->totalRange > 0)
        Line(NULL, "Range: %ld", lround(w->totalRange));
    if (!beam && lround(w->speed100 * 100.0) > 0)
        Line(NULL, "Speed: %ld", lround(w->speed100 * 100.0));
    if (guidance != NULL)
        Line(NULL, "Guidance: %s", guidance);
    if (w->ammoType >= 0 && w->ammoType < kMaxWeapTypes) {
        ammo = -1;
        for (i = 0; i < kMaxOutfits && ammo == -1; i++) {
            if (OutfitLoaded(i) && outfitSystem[i].modType[0] == OutfMod::Ammo
                && outfitSystem[i].modVal[0] == w->ammoType)
                ammo = i;
        }
        if (ammo != -1)
            NameOf(outfitSystem[ammo].name, name, sizeof(name));
        else
            strcpy(name, "uses ammunition");
        if (weaponStats[w->ammoType].maxAmmo > 0)
            Line(NULL, "Ammo: %s, max %d per launcher", name, weaponStats[w->ammoType].maxAmmo);
        else
            Line(NULL, "Ammo: %s", name);
    } else if (w->ammoType < -kWeapAmmoFuelBase) {
        fuel = (-w->ammoType - kWeapAmmoFuelBase) * 0.1;
        if ((w->flags3 & kWeapAmmoAtBurstEnd) != 0) {
            if (w->burstCount > 0)
                Line(NULL, "Fuel per burst: %s", Num(a, sizeof(a), fuel, 2, false));
        } else if (w->reload <= 1.0f) {
            Line(NULL, "Fuel: %s /s", Num(a, sizeof(a), fuel * kFramesPerSecond, 2, false));
        } else {
            Line(NULL, "Fuel per shot: %s", Num(a, sizeof(a), fuel, 2, false));
        }
    }
    if (w->blastRadius > 0)
        Line(NULL, "Blast radius: %d", w->blastRadius);
    if (w->ionization > 0)
        Line(NULL, "Ion: %s", Num(a, sizeof(a), w->ionization * factor, 2, false));
    if (w->burstCount > 0)
        Line(NULL, "Burst: %d, reload %s s", w->burstCount,
             Num(a, sizeof(a), w->burstReload / kFramesPerSecond, 2, false));
}

void OutfitDetails::Effect(short outfit, short slot)
{
    const OutfRecord &o = outfitSystem[outfit];
    short type;
    short v;
    short target;
    char a[32];
    char name[64];
    char other[64];
    char cloak[160];

    type = o.modType[slot];
    v = o.modVal[slot];
    switch (type) {
    case OutfMod::Weapon:
        if (slot == 0)
            Weapon(v);
        return;
    case OutfMod::Ammo:
        if (slot == 0)
            AmmoFor(v);
        return;
    case OutfMod::EscapePod: Line(&betterRGB, "Escape pod"); return;
    case OutfMod::DensityScanner: Line(&betterRGB, "Density scanner"); return;
    case OutfMod::IFF: Line(&betterRGB, "IFF"); return;
    case OutfMod::AutoRefuel: Line(&betterRGB, "Refuels on landing"); return;
    case OutfMod::AutoEject: Line(&betterRGB, "Ejects when the ship is destroyed"); return;
    case OutfMod::FakeID: Line(&betterRGB, "Fake ID"); return;
    case OutfMod::CloakScanner: Line(&betterRGB, "Cloak scanner"); return;
    case OutfMod::Scoop: Line(&betterRGB, "Asteroid scoop"); return;
    case OutfMod::FastJump: Line(&betterRGB, "Fast jump"); return;
    case OutfMod::Inertialess: Line(&betterRGB, "Inertialess"); return;
    case OutfMod::ResistGravity: Line(&betterRGB, "Resists gravity"); return;
    case OutfMod::ResistDeadly: Line(&betterRGB, "Resists deadly planets"); return;
    case OutfMod::Paint: Line(&betterRGB, "Paint job"); return;
    case OutfMod::BlockReinforce: Line(&betterRGB, "Blocks reinforcements"); return;
    case OutfMod::IFFScrambler: Line(&betterRGB, "IFF scrambler"); return;
    case OutfMod::RepairDroids: Line(&betterRGB, "Repair droids"); return;
    case OutfMod::BombDestroys: Line(&worseRGB, "Bomb: destroys your ship"); return;
    case OutfMod::BombDisables: Line(&worseRGB, "Bomb: disables your ship"); return;
    case OutfMod::Afterburner: Line(&betterRGB, "Afterburner (%d fuel/s)", v); return;
    case OutfMod::Map:
        if (v > 0)
            Line(&betterRGB, "Maps systems within %d %s", v, v == 1 ? "jump" : "jumps");
        else if (v == kMapIndependent)
            Line(&betterRGB, "Maps inhabited independent systems");
        else if (v <= -kMapGovtClassBase)
            Line(&betterRGB, "Maps systems of government class %d", -(v + kMapGovtClassBase));
        return;
    case OutfMod::Cloak:
        strcpy(cloak, "Cloaking device");
        if (v & kCloakRadarVisible)
            strcat(cloak, ", visible on radar");
        if (v & kCloakZeroesShields)
            strcat(cloak, ", shields drop to 0");
        if (v & kCloakDecloakOnHit)
            strcat(cloak, ", ends when hit");
        if ((v >> 4) & 0xf)
            snprintf(cloak + strlen(cloak), sizeof(cloak) - strlen(cloak), ", %d fuel/s", (v >> 4) & 0xf);
        if ((v >> 8) & 0xf)
            snprintf(cloak + strlen(cloak), sizeof(cloak) - strlen(cloak), ", %d shield/s", (v >> 8) & 0xf);
        if (v & kCloakArea)
            strcat(cloak, ", cloaks the area");
        Line(&betterRGB, "%s", cloak);
        return;
    case OutfMod::MultiJump:
        if (v > 0)
            Line(&betterRGB, "Jumps up to %d systems at once", v + 1);
        return;
    case OutfMod::RaiseMax:
        target = v - kFirstResourceID;
        if (OutfitLoaded(target)) {
            NameOf(outfitSystem[target].name, other, sizeof(other));
            NameOf(o.name, name, sizeof(name));
            Line(&betterRGB, "Max %s: %d per %s", other, outfitSystem[target].maxCount, name);
        }
        return;
    case OutfMod::Marines:
        if (v > 0)
            Line(&betterRGB, "+%d crew (boarding)", v);
        else if (v < 0)
            Line(&betterRGB, "+%d%% capture odds", -v);
        return;
    }
    if (v == 0)
        return;
    switch (type) {
    case OutfMod::Cargo:
        Line(SignColor(v), "%+d %s cargo space", v, v == 1 || v == -1 ? "ton" : "tons");
        break;
    case OutfMod::ShieldBoost: Line(SignColor(v), "%+d shield", v); break;
    case OutfMod::Armor: Line(SignColor(v), "%+d armor", v); break;
    case OutfMod::ShieldRech:
        Line(SignColor(v), "%s shield/s", Num(a, sizeof(a), v * 0.001 * kFramesPerSecond, 2, true));
        break;
    case OutfMod::ArmorRech:
        Line(SignColor(v), "%s armor/s", Num(a, sizeof(a), v * 0.001 * kFramesPerSecond, 2, true));
        break;
    case OutfMod::Accel: Line(SignColor(v), "%s acceleration", Num(a, sizeof(a), v / 2.0, 1, true)); break;
    case OutfMod::Speed: Line(SignColor(v), "%+d speed", v); break;
    case OutfMod::Turn: Line(SignColor(v), "%s turn rate", Num(a, sizeof(a), v * 0.01 * kFramesPerSecond, 1, true)); break;
    case OutfMod::FuelCapacity: Line(SignColor(v), "%+d fuel", v); break;
    case OutfMod::FuelRegen:
        Line(SignColor(v), "%s fuel/s", Num(a, sizeof(a), kFramesPerSecond / v, 2, true));
        break;
    case OutfMod::DaysPerJump:
        Line(LowerBetterColor(v), "%+d %s per jump", v, v == 1 || v == -1 ? "day" : "days");
        break;
    case OutfMod::HyperSafeDist: Line(LowerBetterColor(v), "%+d jump distance", v); break;
    case OutfMod::Interference: Line(SignColor(v), "%+d interference", -v); break;
    case OutfMod::Murk: Line(LowerBetterColor(v), "%+d murk", v); break;
    case OutfMod::Jam1:
    case OutfMod::Jam2:
    case OutfMod::Jam3:
    case OutfMod::Jam4:
        Line(SignColor(v), "Jamming type %d: %+d%%", type - OutfMod::Jam1 + 1, v);
        break;
    case OutfMod::Deionize:
        Line(SignColor(v), "%s ion/s deionization", Num(a, sizeof(a), v * 0.01 * kFramesPerSecond, 2, true));
        break;
    case OutfMod::IonCapacity: Line(SignColor(v), "%+d ion capacity", v); break;
    case OutfMod::GunSlots:
        Line(SignColor(v), "%+d gun %s", v, v == 1 || v == -1 ? "slot" : "slots");
        break;
    case OutfMod::TurretSlots:
        Line(SignColor(v), "%+d turret %s", v, v == 1 || v == -1 ? "slot" : "slots");
        break;
    }
}

// FUNCTION: EVNOVA 0x0048ea70
// MAC: _DoOutfitDialog i386 0x0005bacf ppc 0x00063b20
/* The outfitter: buying and selling outfits; outfits owned on entry sell back at half price. */
class OutfitDialog final : public GameDialog {
public:
    void Run();

private:
    void Hit(short item) override;
    static Boolean Filter(DialogPtr unused, const DialogEvent &ev, short *itemHit);

    short savedUnk6C;                   /* the nav target the map may change, restored after it */
    short savedUnk6A;
    short savedOutfits[kMaxOutfits];    /* the outfits owned on entry */
};

void OutfitDialog::Run()
{
    short itemType;
    void *item;
    short count;
    Rect box;
    short i;
    short j;
    short k;

    for (i = 0; i < kMaxOutfits; i++)
        savedOutfits[i] = shipSystems[i];
    savedUnk6C = shipData->targetStellar;
    savedUnk6A = shipData->navTargetKind;
    selOutfitItem = -1;
    selOutfitListItem = -1;
    outfitScroll = 0;
    descScroll = 0;
    descScrollOutfit = -1;
    descMaxScroll = 0;
    boughtMap = boughtID = 0;
    canBuy = canSell = 0;
    ShipStatsToSystemInfo();
    SetupPortAvailableItems(dockedPort);
    if (dockedPort->techLevel == 0) {
        portBuysNoOutfits = 1;
        for (i = 0; i < 8; i++) {
            if (dockedPort->specialTech[i] > 0) {
                portBuysNoOutfits = 0;
                break;
            }
        }
    } else
        portBuysNoOutfits = 0;
    count = 0;
    for (i = 0; i < kMaxOutfits; i++) {
        if (dockedPortItems[i] != -1) {
            count++;
            break;
        }
    }
    if (count == 0) {

        strcpy(textShowString, "");
        StrcatIndCString(textShowString, kGameStrList, 0xce);
        ShowTextDialog(0, 0);
        return;
    }
    previewPic = NULL;
    previewPicID = -1;
    missionsOutside = kOutfitVenue;
    offeringFromShip = 0;
    missionOfferTime = ReadTick() + Rand(30) + 30;
    dialog.Open(kOutfitDlogID, OutfitDialogUpdate, Filter, &outfitDialog, DialogClose::NoFlash);
    if (!dialog.IsOpen())
        return;
    Picture backdrop(outfitBackgroundPic);
    Picture preview(previewPic);
    backdrop.Load(kOutfitPictID);
    GetDialogItem(outfitDialog, kGridItem, &itemType, (Handle *)&item, &box);
    SetupListRects((Point *)&box);
    strncpy(dialogDescTextPtr, "", 3);
    SetDialogGWorld(outfitDialog);
    FlushEvents(everyEvent - diskMask, 0);
    ForeColor(blackColor);
    OutfitDialogUpdate();
    OfferMissionFromPort(kOutfitVenue);
    RedrawDialog(outfitDialog);

    Loop();

    k = shipData->FuelCapacity();
    if (shipData->fuel > k) {
        shipData->fuel = k;
        statusRedraw[kRedrawFuel] = 1;
    }
    i = shipData->ShieldCapacity();
    if (shipData->shield > i) {
        shipData->shield = i;
        statusRedraw[kRedrawShield] = 1;
    }
    j = shipData->ArmorCapacity();
    if (shipData->armor > j) {
        shipData->armor = j;
        statusRedraw[kRedrawShield] = 1;
    }
    for (i = 0; i < kMaxOutfits; i++) {
        if (shipSystems[i] > 0 && (outfitSystem[i].flags & kOutfRemoveAfterBuy) != 0)
            shipSystems[i] = 0;
    }
    SystemInfoToShipStats();
    dialog.Close();
    backdrop.Release();
    preview.Release();
    SWSetPortToBackground(gSpriteWorld);
    SetSystemBackgroundColor();
    PaintRect(&gSpriteWorld->backFrameP->frameRect);
    ForeColor(blackColor);
    RedrawStatus(0);
    SWSetPortToWindow(gSpriteWorld);
    shipData->targetStellar = savedUnk6C;
    shipData->navTargetKind = savedUnk6A;
    statusRedraw[kRedrawNav] = 1;
    ResetMissionOfferVenue();
}

void OutfitDialog::Hit(short item)
{
    short filterKey;
    int32_t qtyLimit;
    int have;
    short count;
    short weapID;
    short ammo;
    short shortfall;
    short n;
    short maxAfford;
    unsigned char nameBuf[256];
    char msg[256];
    short i;
    short j;
    short k;
    short m;
    Boolean shown;
    int32_t cost;

    filterKey = GetKeyModifiers();
    if (item == 1)
        done = 1;
    if (item == 2) {
        OpenMapKeepingNav();
        shipData->targetStellar = savedUnk6C;
        shipData->navTargetKind = savedUnk6A;
        RedrawAfterSubDialog(outfitDialog);
    }
    if (item == 13)
        MissionInfoFromDialog(outfitDialog);
    if (item == 8)
        OfferFromDialog(kOutfitVenue, outfitDialog);
    if (item == 7) {
        if (selOutfitItem != -1)
            canBuy = CanBuyOutfitItem(selOutfitItem);
        else
            canBuy = 0;
        if (canBuy && selOutfitItem != -1) {
            count = 1;
            if ((filterKey & optionKey) != 0) {
                cost = ApplyPriceAndTechnologyFlux(
                    outfitSystem[selOutfitItem].cost, priceFluxMult[0],
                    outfitSystem[selOutfitItem].techLevel,
                    stellarData[shipData->targetStellar].techLevel);
                cost = AdjustedItemCost(&outfitSystem[selOutfitItem],
                                        &shipClass[shipData->type]);
                if (cost <= 0)
                    qtyLimit = kMaxTradeQty;
                else {
                    qtyLimit = shipData->credits / cost;
                    if (qtyLimit > kMaxTradeQty)
                        qtyLimit = kMaxTradeQty;
                }
                if (outfitSystem[selOutfitItem].maxCount
                    > shipSystems[selOutfitItem]) {
                    if (qtyLimit > outfitSystem[selOutfitItem].maxCount
                                   - shipSystems[selOutfitItem])
                        qtyLimit = outfitSystem[selOutfitItem].maxCount
                                   - shipSystems[selOutfitItem];
                }
                if (qtyLimit > 1) {
                    HasMaxOfItem(selOutfitItem, 0, &maxAfford);
                    if (maxAfford < qtyLimit)
                        qtyLimit = maxAfford;
                    if (maxAfford - shipSystems[selOutfitItem] < qtyLimit)
                        qtyLimit = maxAfford - shipSystems[selOutfitItem];
                }
                if (qtyLimit > 1) {
                    count = DoQuantityDialog(qtyLimit, 0);
                    OutfitDialogUpdate();
                }
            }
            for (m = 0; m < count; m++) {
                if (canBuy) {
                    cost = ApplyPriceAndTechnologyFlux(
                        outfitSystem[selOutfitItem].cost, priceFluxMult[0],
                        outfitSystem[selOutfitItem].techLevel,
                        stellarData[shipData->targetStellar].techLevel);
                    cost = AdjustedItemCost(&outfitSystem[selOutfitItem],
                                            &shipClass[shipData->type]);
                    GrantOutfitItem(selOutfitItem);
                    shipData->credits -= cost;
                    crashlog_mission_bit_set(
                        "purchasing outfit",
                        (char *)outfitSystem[selOutfitItem].name, 1,
                        selOutfitItem + kFirstResourceID,
                        outfitSystem[selOutfitItem].onPurchase);
                    if (shipData->credits < 0) {
                        crashlog("warning: player's cash went below zero");
                        shipData->credits = 0;
                        m = count;
                    }
                    EvalMissionBitSetString(outfitSystem[selOutfitItem].onPurchase);
                }
                if (selOutfitItem != -1)
                    canBuy = CanBuyOutfitItem(selOutfitItem);
                else
                    canBuy = 0;
            }
            SystemInfoToShipStats();
            if (SetupPortAvailableItems(dockedPort)) {
                selOutfitItem = 0;
                selOutfitListItem = -1;
                outfitScroll = 0;
                canBuy = canSell = 0;
                strncpy(dialogDescTextPtr, "", 3);
                RedrawDialog(outfitDialog);
                m = 0;
                for (i = 0; i < kMaxOutfits; i++) {
                    if (dockedPortItems[i] != -1) {
                        m++;
                        break;
                    }
                }
                if (m == 0)
                    done = 1;
            }
            RedrawDialog(outfitDialog);
            didOutfit = 1;
            if (shipSystems[selOutfitItem] > 0) {
                if ((outfitSystem[selOutfitItem].flags & kOutfCantSell) != 0)
                    canSell = 0;
                else
                    canSell = 1;
            } else
                canSell = 0;
            statusRedraw[kRedrawCargo] = 1;
            DrawStatus();
            SetDialogGWorld(outfitDialog);
            OutfitButtons().Draw(-1);
        }
    }
    if (item == 4 && canSell) {
        n = 1;
        if ((filterKey & optionKey) != 0) {
            qtyLimit = shipSystems[selOutfitItem];
            if (qtyLimit > kMaxTradeQty)
                qtyLimit = kMaxTradeQty;
            if (qtyLimit > 1) {
                n = DoQuantityDialog(qtyLimit, 1);
                OutfitDialogUpdate();
            }
        }
        shown = 0;
        for (count = 0; count < n; count++) {
            if (canSell) {
                have = AdjustedItemMass(&outfitSystem[selOutfitItem],
                                        &shipClass[shipData->type]);
                if (have < 0 && ShipFreeMass() + have < 0) {
                    if (!shown) {
                        GetIndString((unsigned char *)msg, kGameStrList, 0xcf);
                        PtoCstr((unsigned char *)msg);
                        strcpy(textShowString, (char *)msg);
                        ShowTextDialog(0, 0);
                        shown = 1;
                    }
                    canSell = 0;
                }
            }
            if (canSell) {
                for (i = 0; i < kOutfMods; i++) {
                    if (outfitSystem[selOutfitItem].modType[i] == OutfMod::RaiseMax
                        && outfitSystem[selOutfitItem].modVal[i] >= kFirstResourceID
                        && outfitSystem[selOutfitItem].modVal[i] < kFirstResourceID + kMaxOutfits) {
                        shortfall = shipSystems[outfitSystem[selOutfitItem].modVal[i] - kFirstResourceID]
                            - ((shipSystems[selOutfitItem] - n)
                                      * outfitSystem[outfitSystem[selOutfitItem].modVal[i] - kFirstResourceID].maxCount);
                        if (shortfall > 0) {
                            if (!shown) {
                                strcpy((char *)msg,"");

                                StrcatIndCString((char *)msg,kGameStrList, 0xd0);
                                strcat((char *)msg," ");
                                if (shortfall > 3)
                                    StrcatCommaNumber(shortfall, (char *)msg);
                                else
                                    StrcatOrdinalNumber(shortfall, (char *)msg,0);
                                strcat((char *)msg," ");
                                if (shortfall > 1)
                                    pstrcpy(nameBuf,
                                        lcPluralOutfitName[outfitSystem[selOutfitItem].modVal[i] - kFirstResourceID]);
                                else
                                    pstrcpy(nameBuf,
                                        lcOutfitName[outfitSystem[selOutfitItem].modVal[i] - kFirstResourceID]);
                                PtoCstr(nameBuf);
                                strcat((char *)msg,(char *)nameBuf);
                                strcat((char *)msg," ");
                                StrcatIndCString((char *)msg,kGameStrList, 0xd4);
                                strcat((char *)msg," ");
                                if (count < n - 1)
                                    pstrcpy(nameBuf, lcPluralOutfitName[selOutfitItem]);
                                else
                                    pstrcpy(nameBuf, lcOutfitName[selOutfitItem]);
                                PtoCstr(nameBuf);
                                strcat((char *)msg,(char *)nameBuf);
                                strcat((char *)msg,".");
                                strcpy(textShowString, (char *)msg);
                                ShowTextDialog(0, 0);
                                shown = 1;
                            }
                            canSell = 0;
                        }
                    }
                }
                weapID = -1;
                for (i = 0; i < kOutfMods; i++) {
                    if (outfitSystem[selOutfitItem].modType[i] == OutfMod::Weapon) {
                        weapID = outfitSystem[selOutfitItem].modVal[i];
                        break;
                    }
                }
                if (weapID != -1 && weaponStats[weapID].maxAmmo > 0) {
                    if (weaponStats[weapID].ammoType >= 0
                        && weaponStats[weapID].ammoType < kMaxWeapTypes
                        && weaponStats[weapID].guidance != kGuidanceBay)
                        ammo = shipData->weap[weaponStats[weapID].ammoType].ammo;
                    else
                        ammo = shipData->weap[weapID].ammo;
                    if (weaponStats[weapID].guidance == kGuidanceBay) {
                        for (m = 1; m < kMaxShips; m++) {
                            if (shipData[m].active != 0
                                && shipData[m].parentShip == 0
                                && shipData[m].aiType == kShipTypeFighter
                                && shipData[m].type
                                   == weaponStats[weapID].ammoType - kFirstResourceID
                                && !shipData[m].IsDisabled())
                                ammo++;
                        }
                    }
                    shortfall = ammo
                        - ((shipSystems[selOutfitItem] - (count + 1))
                                  * weaponStats[weapID].maxAmmo);
                    if (shortfall > 0 && ammo > 0) {
                        if (!shown) {
                            strcpy((char *)msg,"");

                            StrcatIndCString((char *)msg,kGameStrList, 0xd0);
                            strcat((char *)msg," ");
                            if (shortfall > 3)
                                StrcatCommaNumber(shortfall, (char *)msg);
                            else
                                StrcatOrdinalNumber(shortfall, (char *)msg,0);
                            strcat((char *)msg," ");
                            k = -1;
                            for (m = 0; m < kMaxOutfits; m++) {
                                for (j = 0; j < kOutfMods; j++) {
                                    if (outfitSystem[m].modType[j] == OutfMod::Ammo
                                        && outfitSystem[m].modVal[j] == weapID) {
                                        k = m;
                                        break;
                                    }
                                }
                                if (k != -1)
                                    break;
                            }
                            if (k != -1) {
                                if (shortfall > 1)
                                    pstrcpy(nameBuf, lcPluralOutfitName[k]);
                                else
                                    pstrcpy(nameBuf, lcOutfitName[k]);
                                PtoCstr(nameBuf);
                                strcat((char *)msg,(char *)nameBuf);
                            } else {
                                if (shortfall > 1)
                                    StrcatIndCString((char *)msg,kGameStrList, 0xd2);
                                else
                                    StrcatIndCString((char *)msg,kGameStrList, 0xd1);
                                strcat((char *)msg," ");
                                StrcatIndCString((char *)msg,kGameStrList, 0xd3);
                            }
                            strcat((char *)msg," ");
                            StrcatIndCString((char *)msg,kGameStrList, 0xd4);
                            strcat((char *)msg," ");
                            if (count < n - 1)
                                pstrcpy(nameBuf, lcPluralOutfitName[selOutfitItem]);
                            else
                                pstrcpy(nameBuf, lcOutfitName[selOutfitItem]);
                            PtoCstr(nameBuf);
                            strcat((char *)msg,(char *)nameBuf);
                            strcat((char *)msg,".");
                            strcpy(textShowString, (char *)msg);
                            ShowTextDialog(0, 0);
                            shown = 1;
                        }
                        canSell = 0;
                    }
                }
            }
            if (canSell) {
                cost = ApplyPriceAndTechnologyFlux(
                    outfitSystem[selOutfitItem].cost, priceFluxMult[0],
                    outfitSystem[selOutfitItem].techLevel,
                    stellarData[shipData->targetStellar].techLevel);
                cost = AdjustedItemCost(&outfitSystem[selOutfitItem],
                                        &shipClass[shipData->type]);
                if (shipSystems[selOutfitItem]
                    <= savedOutfits[selOutfitItem])
                    cost *= 0.5;
                have = ShipFreeMass();
                shipSystems[selOutfitItem] -= 1;
                if (ShipFreeMass() < 0 && have >= 0) {
                    shipSystems[selOutfitItem] += 1;
                    PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);
                    break;
                } else {
                    shipData->credits += cost;
                    for (j = 0; j < kOutfMods; j++) {
                        if (shipSystems[selOutfitItem] <= 0) {
                            if (outfitSystem[selOutfitItem].modType[j] == OutfMod::Weapon
                                && shipData->selectedWeapon
                                   == outfitSystem[selOutfitItem].modVal[j]) {
                                shipData->selectedWeapon = -1;
                                statusRedraw[kRedrawWeap] = 1;
                                break;
                            }
                        }
                    }
                    crashlog_mission_bit_set(
                        "selling outfit",
                        (char *)outfitSystem[selOutfitItem].name, 1,
                        selOutfitItem + kFirstResourceID,
                        outfitSystem[selOutfitItem].onSell);
                    EvalMissionBitSetString(outfitSystem[selOutfitItem].onSell);
                    SystemInfoToShipStats();
                    if (SetupPortAvailableItems(dockedPort)) {
                        selOutfitItem = 0;
                        selOutfitListItem = -1;
                        outfitScroll = 0;
                        canBuy = canSell = 0;
                        strncpy(dialogDescTextPtr, "", 3);
                        RedrawDialog(outfitDialog);
                        k = 0;
                        // BUG (all versions): reuses the outer sell loop's `count', so after the item list
                        // changes a quantity sale stops early or runs the wrong number of times
                        for (count = 0; count < kMaxOutfits; count++) {
                            if (dockedPortItems[count] != -1) {
                                k++;
                                break;
                            }
                        }
                        if (k == 0)
                            done = 1;
                    }
                }
            }
            if (selOutfitItem != -1 && shipSystems[selOutfitItem] > 0) {
                if ((outfitSystem[selOutfitItem].flags & kOutfCantSell) != 0)
                    canSell = 0;
                else
                    canSell = 1;
            } else
                canSell = 0;
        }
        if (selOutfitItem != -1)
            canBuy = CanBuyOutfitItem(selOutfitItem);
        else
            canBuy = 0;
        RedrawDialog(outfitDialog);
        didOutfit = 1;
        statusRedraw[kRedrawCargo] = 1;
        DrawStatus();
        SetDialogGWorld(outfitDialog);
        OutfitButtons().Draw(-1);
    }
    if (item == 10 && outfitScroll >= kGridCols) {
        outfitScroll -= kGridCols;
        if (selOutfitListItem >= 0 && selOutfitListItem < kGridCells - kGridCols) {
            selOutfitListItem += kGridCols;
            selOutfitItem =
                dockedPortItems[outfitScroll + selOutfitListItem];
            LoadDescription(dialogDescTextPtr, selOutfitItem + kOutfitDescBase);
        } else {
            selOutfitItem = selOutfitListItem = -1;
            strncpy(dialogDescTextPtr, "", 3);
        }
        SetDialogGWorld(outfitDialog);
        RedrawDialog(outfitDialog);
    }
    if (item == 11) {
        m = 0;
        for (i = 0; i < kMaxOutfits; i++) {
            if (dockedPortItems[i] != -1)
                m++;
        }
        if (outfitScroll < m - kGridCells) {
            outfitScroll += kGridCols;
            if (selOutfitListItem >= kGridCols) {
                selOutfitListItem -= kGridCols;
                selOutfitItem =
                    dockedPortItems[outfitScroll + selOutfitListItem];
                LoadDescription(dialogDescTextPtr, selOutfitItem + kOutfitDescBase);
            } else {
                selOutfitItem = selOutfitListItem = -1;
                strncpy(dialogDescTextPtr, "", 3);
            }
            SetDialogGWorld(outfitDialog);
            RedrawDialog(outfitDialog);
        }
    }
}

constexpr KeyItem kOutfitKeys[] = { { 'b', 7 }, { 's', 4 } };
constexpr short kOutfitSlotItems[] = { 1, 7, 4, 10, 11 };

// FUNCTION: EVNOVA 0x004903c0
// MAC: _OutfitFilter i386 0x00058039 ppc 0x00060298
Boolean OutfitDialog::Filter(DialogPtr unused, const DialogEvent &ev, short *itemHit)
{
    Rect box;
    short itemType;
    Handle item;
    Point where;
    EventChar c;
    short i;
    short count;
    short old;
    short oldPos;
    short row;
    int index;
    int wheel;
    int oldScroll;

    if (LeaveRequested(1, itemHit))
        return 1;
    *itemHit = -1;
    if (selOutfitItem != -1) {
        canBuy = CanBuyOutfitItem(selOutfitItem);
        if (shipSystems[selOutfitItem] > 0) {
            if ((outfitSystem[selOutfitItem].flags & kOutfCantSell) != 0)
                canSell = 0;
            else
                canSell = 1;
        } else {
            canSell = 0;
        }
    } else {
        canSell = 0;
        canBuy = 0;
    }
    if (ev.what == DialogInput::Key) {
        if (QuickKeyHit(2, -1, 13, itemHit))
            return 1;
        c = ev.ch;
        if (IsReturnChar(c) || c == kEscapeChar) {
            *itemHit = 1;
            return 1;
        }
        c = tolower(c);
        if (KeyItemHit(c, kOutfitKeys, itemHit))
            return 1;
        if (c == kGridLeftKey || c == kGridRightKey || c == kGridUpKey ||
            c == kGridDownKey || c == kGridPageUpKey || c == kGridPageDownKey) {
            old = selOutfitItem;
            if (selOutfitListItem != -1) {
                count = 0;
                for (i = 0; i < kMaxOutfits; i++) {
                    if (dockedPortItems[i] != -1)
                        count++;
                }
                if (c == kGridRightKey) {
                    if (selOutfitListItem < kGridCells - 1) {
                        if (dockedPortItems[outfitScroll + selOutfitListItem
                                            + 1] != -1)
                            selOutfitListItem++;
                    } else if (outfitScroll < count - kGridCells) {
                        outfitScroll += kGridCols;
                        selOutfitListItem -= kGridCols - 1;
                    }
                } else if (c == kGridLeftKey) {
                    if (selOutfitListItem > 0) {
                        selOutfitListItem--;
                    } else if (outfitScroll > 0) {
                        outfitScroll -= kGridCols;
                        selOutfitListItem += kGridCols - 1;
                    }
                } else if (c == kGridUpKey || c == kGridPageUpKey) {
                    if (selOutfitListItem >= kGridCols)
                        selOutfitListItem -= kGridCols;
                    else if (outfitScroll > 0)
                        outfitScroll -= kGridCols;
                } else if (c == kGridDownKey || c == kGridPageDownKey) {
                    if (selOutfitListItem < kGridCells - kGridCols) {
                        if (dockedPortItems[outfitScroll + selOutfitListItem
                                            + kGridCols] != -1)
                            selOutfitListItem += kGridCols;
                    } else if (outfitScroll < count - kGridCells) {
                        outfitScroll += kGridCols;
                    }
                }
                selOutfitItem = dockedPortItems[outfitScroll + selOutfitListItem];
                while (selOutfitItem == -1 && selOutfitListItem > 0) {
                    selOutfitListItem--;
                    selOutfitItem = dockedPortItems[outfitScroll + selOutfitListItem];
                }
            } else {
                if (c == kGridRightKey || c == kGridDownKey ||
                    c == kGridPageDownKey)
                    selOutfitListItem = 0;
                else
                    selOutfitListItem = kGridCells - 1;
                index = selOutfitListItem;
                index += outfitScroll;
                selOutfitItem = dockedPortItems[index];
                while (selOutfitItem == -1 && selOutfitListItem > 0) {
                    index--;
                    selOutfitListItem--;
                    selOutfitItem = dockedPortItems[index];
                }
            }
            if (selOutfitItem != old) {
                if (selOutfitListItem != -1) {
                    LoadDescription(dialogDescTextPtr,
                                    selOutfitItem + kOutfitDescBase);
                    RedrawDialog(outfitDialog);
                    canBuy = CanBuyOutfitItem(selOutfitItem);
                    if (shipSystems[selOutfitItem] > 0) {
                        if ((outfitSystem[selOutfitItem].flags
                             & kOutfCantSell) != 0)
                            canSell = 0;
                        else
                            canSell = 1;
                    } else {
                        canSell = 0;
                    }
                } else {
                    canBuy = canSell = 0;
                }
            }
        }
    }
    if (ev.what == DialogInput::Click) {
        SetDialogGWorld(outfitDialog);
        RedrawDialog(outfitDialog);
        where = ev.where;
        GetDialogItem(outfitDialog, kGridItem, &itemType, &item, &box);
        if (PtInRectPoint(where, &box)) {
            count = (where.h - box.left) / kGridCellWidth;
            row = (where.v - box.top) / kGridCellHeight;
            if (count >= 0 && count <= kGridCols - 1 && row >= 0 &&
                row <= kGridCells / kGridCols - 1) {
                oldPos = selOutfitListItem;
                selOutfitListItem = row * kGridCols + count;
                if (dockedPortItems[selOutfitListItem + outfitScroll] == -1) {
                    selOutfitListItem = selOutfitItem = -1;
                } else {
                    selOutfitItem =
                        dockedPortItems[selOutfitListItem + outfitScroll];
                }
                if (selOutfitListItem != oldPos) {
                    SetDialogGWorld(outfitDialog);
                    InvalDialogItem(outfitDialog, 1);
                    InvalDialogItem(outfitDialog, 4);
                    InvalDialogItem(outfitDialog, 7);
                    InvalDialogItem(outfitDialog, 5);
                    InvalDialogItem(outfitDialog, 6);
                    InvalDialogItem(outfitDialog, 8);
                    InvalDialogItem(outfitDialog, 9);
                    InvalDialogItem(outfitDialog, 10);
                    InvalDialogItem(outfitDialog, 11);
                    if (selOutfitItem != -1) {
                        canBuy = CanBuyOutfitItem(selOutfitItem);
                        if (shipSystems[selOutfitItem] > 0) {
                            if ((outfitSystem[selOutfitItem].flags
                                 & kOutfCantSell) != 0)
                                canSell = 0;
                            else
                                canSell = 1;
                        } else {
                            canSell = 0;
                        }
                        LoadDescription(dialogDescTextPtr,
                                        selOutfitItem + kOutfitDescBase);
                    } else {
                        canBuy = canSell = 0;
                        strncpy(dialogDescTextPtr, "", 3);
                    }
                }
                lastClickItem = selOutfitListItem;
                lastClickTime = ev.when;
                *itemHit = -1;
                return 1;
            }
        }
        OutfitDialogUpdate();
        return ButtonItemHit(OutfitButtons().Track(where), kOutfitSlotItems, itemHit);
    }
    if (ev.what == DialogInput::Idle) {
        /* the wheel: over an overflowing description one notch scrolls it a line, else it presses
           scroll up (10) or down (11) while that button is lit */
        wheel = MouseTakeWheel();
        if (wheel != 0 && descMaxScroll > 0) {
            GetDialogItem(outfitDialog, 6, &itemType, &item, &box);
            if (PtInRectPoint(ev.where, &box)) {
                oldScroll = descScroll;
                descScroll -= wheel * descLineHeight;
                if (descScroll < 0)
                    descScroll = 0;
                if (descScroll > descMaxScroll)
                    descScroll = descMaxScroll;
                if (descScroll != oldScroll)
                    RedrawDialog(outfitDialog);
                wheel = 0;
            }
        }
        if (wheel > 0 && outfitScroll >= kGridCols) {
            *itemHit = 10;
            return 1;
        }
        if (wheel < 0) {
            count = 0;
            for (i = 0; i < kMaxOutfits; i++) {
                if (dockedPortItems[i] != -1)
                    count++;
            }
            if (outfitScroll < count - kGridCells) {
                *itemHit = 11;
                return 1;
            }
        }
    }
    return OfferDue(8, itemHit);
}

} // namespace

void DoOutfitDialog(void)
{
    OutfitDialog().Run();
}

// FUNCTION: EVNOVA 0x00490c70
// MAC: _OutfitDialogUpdate i386 0x000572cb ppc 0x0005f680
void OutfitDialogUpdate(void)
{
    int c;
    short skipped;
    Handle item;
    short itemType;
    unsigned char line1[256];
    Rect box;
    unsigned char line2[256];
    short i;
    short drawn;
    int32_t mass;
    int32_t freeMass;

    SetGWorldContainer(&loadPort);
    ForeColor(blackColor);
    PaintRect(GetDialogPortRect(outfitDialog));
    if (outfitBackgroundPic != NULL)
        DrawPicHandle(outfitBackgroundPic, GetDialogPortRect(outfitDialog));
    ForeColor(blackColor);
    skipped = 0;
    GetDialogItem(outfitDialog, kGridItem, &itemType, &item, &box);
    if (RectInRgn(&box, GetWindowVisRgn(FrontWindow(outfitDialog)))) {
        TextFont(smallTextFont);
        TextSize(smallTextSize);
        ForeColor(blackColor);
        drawn = 0;
        for (i = outfitScroll; i < kMaxOutfits; i++) {
            if (drawn < kGridCells && (i >= 0 && dockedPortItems[i] != -1)) {
                if (skipped < 3) {
                    if (i - outfitScroll >= 0 &&
                        i - outfitScroll < kGridCells) {
                        if (!RetrieveMenuIcon(dockedPortItems[i] + kOutfitPictBase,
                                            MenuIconPort(&loadPort),
                                            &listPicScreenRect[i - outfitScroll]))
                            skipped++;
                    } else {
                        skipped++;
                    }
                } else {
                    InvalDialogRect(outfitDialog, &box);
                    break;
                }
                if (shipSystems[dockedPortItems[i]] > 0) {
                    NumToString(shipSystems[dockedPortItems[i]], line1);
                    ForeColor(whiteColor);
                    MoveTo(listScreenRect[i - outfitScroll].right
                                  - (StringWidth(line1) + 3),
                              listScreenRect[i - outfitScroll].top + 12);
                    DrawString(line1);
                    ForeColor(blackColor);
                }
                if (cheatsActive) {
                    NumToString(dockedPortItems[i] + kFirstResourceID, line1);
                    ForeColor(whiteColor);
                    MoveTo(((listScreenRect[i - outfitScroll].left
                                    + listScreenRect[i - outfitScroll].right
                                    - StringWidth(line1)) / 2),
                           (listScreenRect[i - outfitScroll].top + 24));
                    DrawString(line1);
                    ForeColor(blackColor);
                }
                if (SplitTwoLineString(outfitShortNames[dockedPortItems[i]],
                                       line1, line2)) {
                    c = line1[1];
                    if (isalpha(HeldChar(c, line1[1])) || isdigit(HeldChar(c, line1[1])))
                        ForeColor(whiteColor);
                    else
                        RGBForeColor(&rgbLessDarkGrey);
                    MoveTo((listScreenRect[i - outfitScroll].left
                                   + listScreenRect[i - outfitScroll].right) / 2
                                  - StringWidth(line1) / 2,
                              listScreenRect[i - outfitScroll].bottom - 14);
                    DrawString(line1);
                    c = line2[1];
                    if (isalpha(HeldChar(c, line2[1])) || isdigit(HeldChar(c, line2[1])))
                        ForeColor(whiteColor);
                    else
                        RGBForeColor(&rgbLessDarkGrey);
                    MoveTo((listScreenRect[i - outfitScroll].left
                                   + listScreenRect[i - outfitScroll].right) / 2
                                  - StringWidth(line2) / 2,
                              listScreenRect[i - outfitScroll].bottom - 3);
                    DrawString(line2);
                    ForeColor(blackColor);
                } else {
                    MoveTo((listScreenRect[i - outfitScroll].left
                                   + listScreenRect[i - outfitScroll].right) / 2
                                  - StringWidth(line1) / 2,
                              listScreenRect[i - outfitScroll].bottom - 6);
                    ForeColor(whiteColor);
                    DrawString(line1);
                    ForeColor(blackColor);
                }
                RGBForeColor(&shopGridColor[1]);
                FrameRect(&listScreenRect[i - outfitScroll]);
                ForeColor(blackColor);
                drawn++;
            }
        }
        if (selOutfitListItem != -1) {
            RGBForeColor(&shopGridColor[0]);
            FrameRect(&listScreenRect[selOutfitListItem]);
            ForeColor(blackColor);
        }
    }
    if (selOutfitListItem != -1) {
        GetDialogItem(outfitDialog, 8, &itemType, &item, &box);
        if (RectInRgn(&box, GetWindowVisRgn(FrontWindow(outfitDialog)))) {
            if (previewPicID != selOutfitItem + kOutfitPictBase) {
                if (previewPic != NULL) {
                    QDReleaseResource(previewPic);
                    previewPic = NULL;
                }
                previewPicID = selOutfitItem + kOutfitPictBase;
                if (previewPicID >= kFirstResourceID)
                    previewPic = GetPicture(previewPicID);
            }
            if (previewPic != NULL) {
                ForeColor(blackColor);
                DrawPicHandle(previewPic, &box);
            } else {
                ForeColor(whiteColor);
                TextFont(smallTextFont);
                TextSize(smallTextSize);
                GetIndString(line1, kGameStrList, 0xd5);
                DrawStringCentered(line1, box.left, box.right,
                                   ((box.bottom + box.top) / 2 - 6));
                GetIndString(line1, kGameStrList, 0xd6);
                DrawStringCentered(line1, box.left, box.right,
                                   ((box.bottom + box.top) / 2 + 6));
                ForeColor(blackColor);
            }
        }
        GetDialogItem(outfitDialog, 9, &itemType, &item, &box);
        if (RectInRgn(&box, GetWindowVisRgn(FrontWindow(outfitDialog)))) {
            TextFont(smallTextFont);
            TextSize(smallTextSize);
            ForeColor(whiteColor);
            MoveTo(box.left, (box.top + 12));
            DrawIndString(kGameStrList, 0xd7);
            MoveTo((box.left + 0x46), (box.top + 12));
            ApplyPriceAndTechnologyFlux(outfitSystem[selOutfitItem].cost, priceFluxMult[0],
                            outfitSystem[selOutfitItem].techLevel,
                            stellarData[shipData->targetStellar].techLevel);
            DrawCommaNumber(AdjustedItemCost(&outfitSystem[selOutfitItem],
                                             &shipClass[shipData->type]));
            DrawString("\p ");
            DrawIndString(kGameStrList, 0x22);
            MoveTo(box.left, (box.top + 0x18));
            DrawIndString(kGameStrList, 0xd9);
            MoveTo((box.left + 0x46), (box.top + 0x18));
            DrawCommaNumber(shipData->credits);
            DrawString("\p ");
            DrawIndString(kGameStrList, 0x22);
            if (outfitSystem[selOutfitItem].mass > 0) {
                MoveTo(box.left, (box.top + 0x30));
                DrawIndString(kGameStrList, 0xd8);
                MoveTo((box.left + 0x46), (box.top + 0x30));
                mass = AdjustedItemMass(&outfitSystem[selOutfitItem],
                                        &shipClass[shipData->type]);
                DrawCommaNumber(mass);
                DrawString("\p ");
                if (mass == 1)
                    DrawString(tonString);
                else
                    DrawString(tonsString);
                MoveTo(box.left, (box.top + 0x3c));
                DrawIndString(kGameStrList, 0xda);
                MoveTo((box.left + 0x46), (box.top + 0x3c));
                freeMass = ShipFreeMass();
                if (freeMass < 0)
                    freeMass = 0;
                DrawCommaNumber(freeMass);
                DrawString("\p ");
                if (freeMass == 1)
                    DrawString(tonString);
                else
                    DrawString(tonsString);
            }
            MoveTo(box.left, (box.top + 0x5d));
            if (HasMaxOfItem(selOutfitItem, NULL, NULL)) {
                if (shipSystems[selOutfitItem] > 0)
                    DrawIndString(kGameStrList, 0xdb);
                else
                    DrawIndString(kGameStrList, 0xdc);
            } else {
                mass = AdjustedItemMass(&outfitSystem[selOutfitItem],
                                        &shipClass[shipData->type]);
                if (mass > 0 && mass > ShipFreeMass()) {
                    if (shipSystems[selOutfitItem] > 0)
                        DrawIndString(kGameStrList, 0xdd);
                    else
                        DrawIndString(kGameStrList, 0xde);
                }
            }
            ForeColor(blackColor);
        }
    }
    GetDialogItem(outfitDialog, 6, &itemType, &item, &box);
    if (selOutfitItem != descScrollOutfit) {
        descScroll = 0;
        descScrollOutfit = selOutfitItem;
    }
    if (RectInRgn(&box, GetWindowVisRgn(FrontWindow(outfitDialog)))) {
        static OutfitDetails details;

        TextFont(smallTextFont);
        TextSize(smallTextSize);
        if (selOutfitListItem != -1 && selOutfitItem >= 0 && selOutfitItem < kMaxOutfits)
            details.Build(selOutfitItem);
        else
            details.Plain(dialogDescTextPtr);
        descLineHeight = TextLineHeight();
        descMaxScroll = MeasureTextSpans(details.Text(), details.Length(), &box) - (box.bottom - box.top);
        if (descMaxScroll < 0)
            descMaxScroll = 0;
        else if (descLineHeight > 0)
            descMaxScroll = (descMaxScroll + descLineHeight - 1) / descLineHeight * descLineHeight;
        if (descScroll > descMaxScroll)
            descScroll = descMaxScroll;
        ForeColor(blackColor);
        PaintRect(&box);
        ForeColor(whiteColor);
        DrawTextSpans(details.Text(), details.Length(), &box, descScroll, details.Spans(), details.SpanCount());
        ForeColor(blackColor);
    }
    OutfitButtons().Draw(-1);
    CopyLoadPortToDialog(outfitDialog);
}

#define kSpecialTechCount  8


// FUNCTION: EVNOVA 0x00491950
// MAC: _CanBuyOutfitItem i386 0x0004e7c4 ppc 0x00057448
Boolean CanBuyOutfitItem(short outfit)
{
    Boolean isCargo;
    Boolean isMap;
    short cargoDelta;
    int32_t freeMass;
    int32_t mass;
    Boolean can;
    Boolean isFakeID;
    Boolean avail;
    short i;
    short stellar;

    if (outfit != -1) {
        freeMass = ShipFreeMass();
        if (outfitSystem[outfit].techLevel != -1) {
            can = 0;
            if (outfitSystem[outfit].techLevel <=
                    stellarData[shipData[0].targetStellar].techLevel) {
                can = 1;
            } else {
                for (i = 0; i < kSpecialTechCount; i++) {
                    if (stellarData[shipData[0].targetStellar]
                                .specialTech[i] == outfitSystem[outfit].techLevel) {
                        can = 1;
                        break;
                    }
                }
            }
            if (!can)
                return 0;
        }
        mass = AdjustedItemMass(&outfitSystem[outfit],
                                &shipClass[shipData[0].type]);
        if (!HasMaxOfItem(outfit, 0, 0)
                && (freeMass >= mass || mass <= 0)) {
            isCargo = 0;
            cargoDelta = 0;
            isMap = 0;
            isFakeID = 0;
            for (i = 0; i < kOutfMods; i++) {
                if (outfitSystem[outfit].modType[i] == OutfMod::Cargo) {
                    isCargo = 1;
                    cargoDelta = outfitSystem[outfit].modVal[i];
                }
                if (outfitSystem[outfit].modType[i] == OutfMod::Map)
                    isMap = 1;
                if (outfitSystem[outfit].modType[i] == OutfMod::FakeID)
                    isFakeID = 1;
            }
            if (outfitSystem[outfit].modType[0] == OutfMod::Ammo) {
                if (outfitSystem[outfit].modVal[0] >= 0) {
                    if (weaponStats[outfitSystem[outfit].modVal[0]].guidance ==
                            kGuidanceBay) {
                        if (CanBuyFighter(
                                weaponStats[outfitSystem[outfit].modVal[0]].ammoType
                                    - kFirstResourceID,
                                outfit, outfitSystem[outfit].modVal[0]))
                            can = 1;
                        else
                            can = 0;
                    } else {
                        can = 1;
                    }
                } else {
                    can = 1;
                }
            } else if (isCargo) {
                if (cargoDelta < 0) {
                    if (shipClass[shipData[0].type].cargoNotFixed == 0) {
                        can = 0;
                    } else if (ShipTotalHolds() - TotalCargo(shipData) >=
                            abs(cargoDelta)) {
                        can = 1;
                    } else {
                        can = 0;
                    }
                } else {
                    can = 1;
                }
            } else if (isMap) {
                if (boughtMap == 0)
                    can = 1;
                else
                    can = 0;
            } else if (isFakeID) {
                if (boughtID == 0)
                    can = 1;
                else
                    can = 0;
            } else {
                can = 1;
            }
        } else {
            can = 0;
        }
        if (can) {
            avail = 1;
            if (playerIsLanded != 0) {
                stellar = shipData[0].targetStellar;
                if (stellar != -1) {
                    if (InGovtClass(outfitSystem[outfit].govtRestrict, 0)) {
                        avail = 0;
                        if (stellarData[stellar].govt ==
                                outfitSystem[outfit].govtRestrict - ResClassBase(0))
                            avail = 1;
                        else if (stellarData[stellar].govt != -1
                                 && GovtAllies(stellarData[stellar].govt,
                                               outfitSystem[outfit].govtRestrict
                                                   - ResClassBase(0)))
                            avail = 1;
                    } else if (InGovtClass(outfitSystem[outfit].govtRestrict, 1)) {
                        avail = 0;
                        if (stellarData[stellar].govt ==
                                outfitSystem[outfit].govtRestrict - ResClassBase(1)) {
                            avail = 1;
                        } else if (stellarData[stellar].govt != -1) {
                            if (GovtAllies(stellarData[stellar].govt,
                                           outfitSystem[outfit].govtRestrict
                                               - ResClassBase(1)))
                                avail = 1;
                        } else {
                            avail = 1;
                        }
                    } else if (InGovtClass(outfitSystem[outfit].govtRestrict, 2)) {
                        avail = 1;
                        if (stellarData[stellar].govt ==
                                outfitSystem[outfit].govtRestrict - ResClassBase(2))
                            avail = 0;
                        else if (stellarData[stellar].govt != -1
                                 && GovtAllies(stellarData[stellar].govt,
                                               outfitSystem[outfit].govtRestrict
                                                   - ResClassBase(2)))
                            avail = 0;
                    } else if (InGovtClass(outfitSystem[outfit].govtRestrict, 3)) {
                        avail = 1;
                        if (stellarData[stellar].govt ==
                                outfitSystem[outfit].govtRestrict - ResClassBase(3)) {
                            avail = 0;
                        } else if (stellarData[stellar].govt != -1) {
                            if (GovtAllies(stellarData[stellar].govt,
                                           outfitSystem[outfit].govtRestrict
                                               - ResClassBase(3)))
                                avail = 0;
                        } else {
                            avail = 0;
                        }
                    }
                }
            }
            if (avail) {
                if (!PlayerMeetsRequirements(outfitSystem[outfit].requires[0],
                                             outfitSystem[outfit].requires[1]))
                    can = 0;
            }
        }
        if (can) {
            if (!EvalMissionBitTestString(outfitSystem[outfit].availability))
                can = 0;
        }
        if (can) {
            ApplyPriceAndTechnologyFlux(outfitSystem[selOutfitItem].cost, priceFluxMult[0],
                            outfitSystem[selOutfitItem].techLevel,
                            stellarData[shipData[0].targetStellar].techLevel);
            {
                int32_t cost = AdjustedItemCost(&outfitSystem[selOutfitItem],
                                             &shipClass[shipData[0].type]);

                if (shipData[0].credits < cost)
                    can = 0;
            }
        }
    } else {
        can = 0;
    }
    return can;
}
