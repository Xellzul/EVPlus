/* playerinfodlg.cpp -- the player info dialog */

#include "game.h"
#include "stellar.h"
#include "prototypes.h"
#include "gameutil.h"
#include "player.h"
#include "render.h"
#include "ships.h"
#include "ai.h"
#include "gamedialog.h"
#include "buttons.h"
#include "text.h"
#include "painter.h"

DialogPtr infoDialog;
short selPlayerInfoMode;
PicHandle playerInfoBackgroundPic[3];
std::string playerInfoText[3];

/* The player info dialog's background, stretched as one picture: the middle picture fills what the top
   and bottom ones leave (at least its own height). */
/* "  (current / maximum)" after a percentage, current kept to 0 or more */
static void DrawRawValue(UI::Painter &p, const UI::TextSpec &spec, double current, double maximum)
{
    p.Text("  (", spec);
    p.Text(NumText((int32_t)(current > 0.0 ? current : 0.0)), spec);
    p.Text(" / ", spec);
    p.Text(NumText((int32_t)maximum), spec);
    p.Text(")", spec);
}

/* days a jump takes: the slowest of the player's ship and its active escorts */
static short FleetJumpDays(void)
{
    short days = shipData->HyperTransitTime();

    for (short i = 1; i < kMaxShips; i++) {
        if (shipData[i].active && shipData[i].parentShip == 0 && !shipData[i].IsDisabled()) {
            short d = shipData[i].HyperTransitTime();
            if (d > days)
                days = d;
        }
    }
    return days;
}

static void DrawDays(UI::Painter &p, const UI::TextSpec &spec, short days)
{
    p.Text(NumText(days), spec);
    p.Text(days == 1 ? " day" : " days", spec);
}

static void DrawPlayerInfoBackground(UI::Painter &p, const Rect &port)
{
    UI::Painter::Composition background(p, port);
    Rect box;
    Rect box2;

    if (playerInfoBackgroundPic[1] != NULL) {
        QDGetPictureBounds(playerInfoBackgroundPic[1], &box);
        OffsetRect(&box, -box.left, -box.top);
        if (playerInfoBackgroundPic[0] != NULL) {
            QDGetPictureBounds(playerInfoBackgroundPic[0], &box2);
            OffsetRect(&box2, -box2.left, -box2.top);
            OffsetRect(&box, 0, box2.bottom);
        }
        if (playerInfoBackgroundPic[2] != NULL) {
            QDGetPictureBounds(playerInfoBackgroundPic[2], &box2);
            box.bottom = (port.bottom - port.top) - (box2.bottom - box2.top);
        }
        QDGetPictureBounds(playerInfoBackgroundPic[1], &box2);
        if (box.bottom - box.top < box2.bottom - box2.top)
            box.bottom = box.top + (box2.bottom - box2.top);
        OffsetRect(&box, port.left, port.top);
        p.Picture(playerInfoBackgroundPic[1], box);
    }

    if (playerInfoBackgroundPic[0] != NULL) {
        QDGetPictureBounds(playerInfoBackgroundPic[0], &box);
        OffsetRect(&box, -box.left, -box.top);
        OffsetRect(&box, port.left, port.top);
        p.Picture(playerInfoBackgroundPic[0], box);
    }

    if (playerInfoBackgroundPic[2] != NULL) {
        QDGetPictureBounds(playerInfoBackgroundPic[2], &box);
        OffsetRect(&box, -box.left, -box.top);
        OffsetRect(&box, port.left, port.bottom - (box.bottom - box.top));
        p.Picture(playerInfoBackgroundPic[2], box);
    }
}

/*
 * 0x00591518 is the player-info key itself: the main-screen event proc at
 * 0x004959d0 opens this dialog on it, guarded by infoDialog == NULL, so
 * pressing it again while the dialog is up dismisses it.
 */


#define kPlayerInfoPages 4


/* the 3-picture composite; win 0x7d4be0/e4/e8 are one mac symbol's 3 slots */


/* per-mission state / record, only the fields this scan needs */
/* rank record, only the fields the net-worth/achievement scans touch */


/* pre-filled elsewhere (whatever grants the special item / mission text);
   this function only reads and measures them */


/* --- DoPlayerInfoDialog (0x00499c10) --- */

/* the port's TE record; only the fields this driver measures */

/*
 * 0x007d5278 is one mac symbol, _playerInfoText: three 4096-byte pages,
 * declared once at the head of the file; this function walks it.
 */
#define kPlayerInfoDlogID      0x3f9
#define kPlayerInfoFirstPictID 0x2146
#define kRetireQuestionIdx     0x123


namespace {

// FUNCTION: EVNOVA 0x00499c10
// MAC: _DoPlayerInfoDialog i386 0x0005a00a ppc 0x00062244
/* The player info dialog: the pilot's record in pages, picked by the page buttons or Tab. */
class PlayerInfoDialog final : public GameDialog {
public:
    void Run();

private:
    void Hit(short item) override;
    static Boolean Filter(DialogPtr unused, const DialogEvent &ev, short *itemHit);
};

void PlayerInfoDialog::Run()
{
    Handle item;
    Rect r;
    Rect buttonBox[2];
    short itemType;
    Rect saved;
    Rect box6;
    short delta;
    short i;
    short tallest;
    short height;
    short boxHeight;
    short dv;

    if (screenFaded != 0)
        return;
    if (shipData[0].jumpTimer != 0)
        return;
    if (shipData[0].deathTimer > 0.0f)
        return;
    if (fadePending[0] != 0 || fadePending[1] != 0)
        return;
    ShipStatsToSystemInfo();
    selPlayerInfoMode = 1;
    dialog.Open(kPlayerInfoDlogID, PlayerInfoDialogUpdate, Filter, &infoDialog, DialogClose::NoFlash);
    if (!dialog.IsOpen())
        return;
    Picture backdrops[3];
    for (i = 0; i < 3; i++) {
        backdrops[i].Bind(playerInfoBackgroundPic[i]);
        backdrops[i].Load(kPlayerInfoFirstPictID + i);
    }
    SetupPlayerInfoText();
    saved = *GetDialogPortRect(infoDialog);
    GetDialogItem(infoDialog, 6, &itemType, &item, &box6);
    GetDialogItem(infoDialog, 1, &itemType, &item, &buttonBox[0]);
    GetDialogItem(infoDialog, 7, &itemType, &item, &buttonBox[1]);
    textDialogTextHandle = TENew(&box6, &box6);
    (*textDialogTextHandle)->txFont = smallTextFont;
    (*textDialogTextHandle)->txSize = smallTextSize;
    TESetLineMetrics(textDialogTextHandle);
    tallest = -1;
    {
        UI::Painter measure(UI::Surface::Dialog(infoDialog));

        for (i = 0; i < 3; i++) {
            measure.EditSetText(textDialogTextHandle, playerInfoText[i].data(), playerInfoText[i].size(),
                                UI::Style::Small);
            measure.EditMeasure(textDialogTextHandle, UI::Style::Small);
            height = TEWrappedHeight(textDialogTextHandle);
            if (height > tallest || tallest <= 0)
                tallest = height;
        }
    }
    TEDispose(textDialogTextHandle);
    boxHeight = box6.bottom - box6.top;
    if (tallest > boxHeight) {
        delta = tallest - boxHeight + 4;
        r = saved;
        r.bottom += delta;
        SizeDialogWindow(infoDialog, (r.right - r.left),
                     (r.bottom - r.top), 1);
        dv = 0;
        dv -= delta * 0.5;
        OffsetDialogWindow(infoDialog, 0, dv, 1);
        r = buttonBox[0];
        OffsetRect(&r, 0, delta);
        GetDialogItem(infoDialog, 1, &itemType, &item, &buttonBox[0]);
        SetDialogItem(infoDialog, 1, itemType, item, &r);
        r = buttonBox[1];
        GetDialogItem(infoDialog, 7, &itemType, &item, &buttonBox[1]);
        OffsetRect(&r, 0, delta);
        SetDialogItem(infoDialog, 7, itemType, item, &r);
        r = box6;
        r.bottom += delta;
        GetDialogItem(infoDialog, 6, &itemType, &item, &box6);
        SetDialogItem(infoDialog, 6, itemType, item, &r);
    }
    FlushEvents(everyEvent - diskMask, 0);
    RedrawDialog(infoDialog);
    PlayerInfoDialogUpdate();
    Loop();
    while (KeyCheck(keyPrefs.playerInfo))
        BusyIdle();
    dialog.Close();
    for (i = 0; i < 3; i++)
        backdrops[i].Release();
    RefreshSAT();
    FlushEvents(7, 0);
}

void PlayerInfoDialog::Hit(short item)
{
    unsigned char buf[256];

    if (item == 1)
        done = 1;
    if (item >= 2 && item <= 5) {
        selPlayerInfoMode = item - 1;
        RedrawDialog(infoDialog);
    }
    if (item == 7 && selPlayerInfoMode == 2) {
        if (PlayerHasJettisonableCargo()) {
            GetIndString(buf, kGameStrList, kRetireQuestionIdx);
            if (EVYesNoDialog(buf)) {
                JettisonCargo(1);
                done = 1;
            }
        }
        RedrawDialog(infoDialog);
    }
}

constexpr short kPlayerInfoSlotItems[] = { 1, 2, 3, 4, 5, 7 };

// FUNCTION: EVNOVA 0x0049a3a0
// MAC: _PlayerInfoFilter i386 0x00053ae9 ppc 0x0005c1b8
Boolean PlayerInfoDialog::Filter(DialogPtr unused, const DialogEvent &ev, short *itemHit)
{
    Point where;
    int c;

    if (ev.what == DialogInput::Key) {
        c = ev.ch;
        if (c == kTabChar) {
            if ((ev.mods & shiftKey) != 0)
                selPlayerInfoMode -= 1;
            else
                selPlayerInfoMode += 1;
            if (selPlayerInfoMode > kPlayerInfoPages)
                selPlayerInfoMode = 1;
            if (selPlayerInfoMode < 1)
                selPlayerInfoMode = kPlayerInfoPages;
            RedrawDialog(infoDialog);
            *itemHit = selPlayerInfoMode + 1;
            return 1;
        }
        if (IsReturnChar(c) || c == kEscapeChar || KeyCheck(keyPrefs.playerInfo)) {
            *itemHit = 1;
            return 1;
        }
    }
    if (ev.what == DialogInput::Click) {
        where = ev.where;
        PlayerInfoDialogUpdate();
        return ButtonItemHit(PlayerInfoButtons(selPlayerInfoMode).Track(where), kPlayerInfoSlotItems, itemHit);
    }
    return 0;
}

} // namespace

void DoPlayerInfoDialog(void)
{
    PlayerInfoDialog().Run();
}

// FUNCTION: EVNOVA 0x0049a540
// MAC: _PlayerInfoDialogUpdate i386 0x0005244f ppc 0x0005ac78
void PlayerInfoDialogUpdate(void)
{
    Rect box;
    Rect box2;
    short itemType;
    void *itemHandle;
    std::string text;
    short i;
    int32_t stellarWorth;
    int32_t escortWorth;
    short sys;
    short jumps;
    short count;
    Boolean onMission;
    const UI::TextSpec light = { UI::Style::Small, rgbLightGrey };
    const UI::TextSpec white = { UI::Style::Small, UI::kWhite };

    UI::Painter p(UI::Surface::Load());

    p.Fill(*GetDialogPortRect(infoDialog), UI::kBlack);

    DrawPlayerInfoBackground(p, *GetDialogPortRect(infoDialog));

    GetDialogItem(infoDialog, 6, &itemType, (Handle *)&itemHandle, &box);

    if (selPlayerInfoMode == 1) {
        /* --- left column --- */
        p.Text((box.left + 5), (box.top + 0xc), IndStringRaw(kGameStrList, 0xfb), light);
        p.Text((box.left + 0x50), (box.top + 0xc), FromPascal(playerName), white);

        p.Text((box.left + 5), (box.top + 0x1c), IndStringRaw(kGameStrList, 0xfc), light);
        p.Text((box.left + 0x50), (box.top + 0x1c), DateMedText(gameTime), white);

        p.Text((box.left + 5), (box.top + 0x2c), IndStringRaw(kGameStrList, 0xfd), light);
        p.Text((box.left + 0x50), (box.top + 0x2c), SystName(shipData->curSystem), white);
        if (cheatsActive) {
            p.Text(" (", white);
            p.Text(NumText(shipData->curSystem + 128), white);
            p.Text(")", white);
        }

        p.Text((box.left + 5), (box.top + 0x3c), IndStringRaw(kGameStrList, 0x146), light);
        if (SystemIsInhabited(shipData->curSystem)) {
            p.Text((box.left + 0x50), (box.top + 0x3c), LegalStatusText(shipData->curSystem), white);
            if (cheatsActive) {
                p.Text(" (", white);
                p.Text(NumText(playerRecord[shipData->curSystem]), white);
                p.Text(")", white);
            }
        } else {
            p.Text((box.left + 0x50), (box.top + 0x3c), IndStringRaw(kGameStrList, 0x18c), white);
        }

        p.Text((box.left + 5), (box.top + 0x4c), IndStringRaw(kGameStrList, 0xfe), light);
        p.Text((box.left + 0x50), (box.top + 0x4c), CombatRatingText(), white);
        if (cheatsActive) {
            p.Text(" (", white);
            p.Text(NumText(playerKills), white);
            p.Text(")", white);
        }

        /* --- right column --- */
        p.Text((box.left + 0xcd), (box.top + 0xc), IndStringRaw(kGameStrList, 0xff), light);
        p.Text((box.left + 0x118), (box.top + 0xc), FromPascal(shipName), white);

        p.Text((box.left + 0xcd), (box.top + 0x1c), IndStringRaw(kGameStrList, 0x100), light);
        p.Text((box.left + 0x118), (box.top + 0x1c), ShipClassName(shipData->type), white);
        if (cheatsActive) {
            p.Text(" (", white);
            p.Text(NumText(shipData->type + 128), white);
            p.Text(")", white);
        }

        p.Text((box.left + 0xcd), (box.top + 0x2c), IndStringRaw(kGameStrList, 0x101), light);
        p.Text((box.left + 0x118), (box.top + 0x2c), NumText(TruncF32(shipData->TurnRate() * 30.0f)), white);
        p.Text(IndStringRaw(kGameStrList, 0x102), white);

        p.Text((box.left + 0xcd), (box.top + 0x3c), IndStringRaw(kGameStrList, 0x103), light);
        p.Text((box.left + 0x118), (box.top + 0x3c), NumText(TruncF32(shipData->AccelRate() * 2500.0)), white);

        p.Text((box.left + 0xcd), (box.top + 0x4c), IndStringRaw(kGameStrList, 0x104), light);
        if (strictPlay)
            p.Text((box.left + 0x118), (box.top + 0x4c), NumText(TruncF32(shipData->MaxSpeed() * 100.0f)), white);
        else
            p.Text((box.left + 0x118), (box.top + 0x4c),
                   NumText(TruncF32(shipData->MaxSpeed() * 100.0f * 0.6666666666666666)), white);

        text = creditsString;
        /* 128..255 bytes stay as they are (a signed length test) */
        if (!text.empty() && text.size() < 128)
            text[0] = (char)toupper((unsigned char)text[0]);
        p.Text((box.left + 0xcd), (box.top + 0x5c), text, light);
        p.Text(":", light);
        p.Text((box.left + 0x118), (box.top + 0x5c), CommaNumber(shipData->credits), white);

        p.Text((box.left + 0xcd), (box.top + 0x6c), "Jump Time:", light);
        short ownDays = shipData->HyperTransitTime();
        short fleetDays = FleetJumpDays();
        p.Pen((box.left + 0x118), (box.top + 0x6c));
        DrawDays(p, white, ownDays);
        if (fleetDays > ownDays) {
            p.Text(" (", white);
            DrawDays(p, white, fleetDays);
            p.Text(" with escorts)", white);
        }

        /* --- shields / armor / fuel, left column continued --- */
        p.Text((box.left + 5), (box.top + 0x5c), shieldString[1], light);
        p.Pen((box.left + 0x50), (box.top + 0x5c));
        if (shipData->ShieldCapacity() > 0.0) {
            if (shipData->shield >= 0.0f) {
                if (shipData->shield <= shipData->ShieldCapacity()) {
                    p.Text(NumText(shipData->shield / shipData->ShieldCapacity() * 100.0), white);
                    p.Text("%", white);
                } else {
                    p.Text("100%", white);
                }
            } else {
                p.Text("0%", white);
            }
            DrawRawValue(p, white, shipData->shield, shipData->ShieldCapacity());
        } else {
            p.Text(IndStringRaw(kGameStrList, 0x18c), white);
        }

        p.Text((box.left + 5), (box.top + 0x6c), armorString[1], light);
        p.Pen((box.left + 0x50), (box.top + 0x6c));
        if (shipData->IsDying()) {
            p.Text(IndStringRaw(kGameStrList, 0x105), white);
        } else if (shipData->ArmorCapacity() > 0.0) {
            if (shipData->armor <= shipData->ArmorCapacity()) {
                p.Text(NumText(shipData->armor / shipData->ArmorCapacity() * 100.0), white);
                p.Text("%", white);
            } else {
                p.Text("100%", white);
            }
            DrawRawValue(p, white, shipData->armor, shipData->ArmorCapacity());
        } else {
            p.Text(IndStringRaw(kGameStrList, 0x18c), white);
        }

        p.Text((box.left + 5), (box.top + 0x7c), fuelString[5], light);
        p.Pen((box.left + 0x50), (box.top + 0x7c));
        if (shipData->FuelCapacity() <= 0.0) {
            p.Text(IndStringRaw(kGameStrList, 0x14f), white);
        } else {
            p.Text(NumText(shipData->fuel / shipData->FuelCapacity() * 100.0), white);
            p.Text("%", white);
            jumps = (shipData->fuel / 100.0);
            if (jumps > 0) {
                p.Text("  (", white);
                p.Text(NumText(jumps), white);
                p.Text(" ", white);
                if (jumps > 1)
                    p.Text(IndStringRaw(kGameStrList, 0xf0), white);
                else
                    p.Text(IndStringRaw(kGameStrList, 0xef), white);
                if ((short)shipData->fuel % 100 > 0) {
                    p.Text(" ", white);
                    p.Text(IndStringRaw(kGameStrList, 0x106), white);
                    p.Text(" ", white);
                    p.Text(fuelString[6], white);
                }
                p.Text(")", white);
            } else if (shipData->fuel > 0.0f) {
                p.Text("  (", white);
                p.Text(fuelString[6], white);
                p.Text(" ", white);
                p.Text(IndStringRaw(kGameStrList, 0x107), white);
                p.Text(")", white);
            }
        }

        /* --- net worth: escort resale value + known-system tribute + rank bonus --- */
        stellarWorth = 0;
        escortWorth = 0;
        for (i = 1; i < kMaxShips; i++) {
            if (shipData[i].active == 0)
                continue;
            if (shipData[i].parentShip != 0)
                continue;
            onMission = 0;
            if (shipData[i].specialMission != -1 &&
                currentMissionObjectives[shipData[i].specialMission].active != 0 &&
                currentMissions[shipData[i].specialMission].shipBehav == kMissionModeEscort)
                onMission = 1;
            if (onMission)
                continue;
            if (shipData[i].hired == 0)
                continue;
            escortWorth += shipClass[shipData[i].type].cost * 0.01;
        }

        for (i = 0; i < kMaxStellars; i++) {
            if (stellarData[i].loaded == 0 || stellarData[i].dominated == 0)
                continue;
            sys = stellarData[i].system;
            if (sys < 0 || sys >= kMaxSystems)
                continue;
            if (systemData[sys].loaded == 0 || systemData[sys].prepared == 0)
                continue;
            stellarWorth += stellarData[i].tribute;
        }

        for (i = 0; i < kMaxRanks; i++) {
            if (rankData[i].active == 0 || rankData[i].loaded == 0)
                continue;
            if (rankData[i].salary == 0)
                continue;
            if (shipData->credits < rankData[i].salaryCap || rankData[i].salaryCap <= 0)
                stellarWorth += rankData[i].salary;
        }

        if (stellarWorth < 0) {
            escortWorth -= stellarWorth;
            stellarWorth = 0;
        }
        if (escortWorth > 0 || stellarWorth > 0) {
            p.Pen((box.left + 5), (box.top + 0x8c));
            if (escortWorth > stellarWorth) {
                p.Text(IndStringRaw(kGameStrList, 0x108), light);
                p.Text("  ", light);
                p.Text(CommaNumber(escortWorth - stellarWorth), white);
            } else if (escortWorth != 0) {
                p.Text(IndStringRaw(kGameStrList, 0x10a), light);
                p.Text(" ", light);
                p.Text(IndStringRaw(kGameStrList, 0x109), light);
                p.Text("  ", light);
                p.Text(CommaNumber(stellarWorth - escortWorth), white);
            } else {
                p.Text(IndStringRaw(kGameStrList, 0x109), light);
                p.Text("  ", light);
                p.Text(CommaNumber(stellarWorth), white);
            }
            p.Text(" ", white);
            p.Text(creditsString, white);
            p.Text(" ", light);
            p.Text(IndStringRaw(kGameStrList, 0x10b), light);
        }
    } else if (selPlayerInfoMode == 2) {
        if (PlayerHasAnyCargo()) {
            box2 = box;
            box2.bottom -= 10;
            box2.right -= 5;
            box2.left += 5;
            box2.top += 5;
            p.TextBox(box2, playerInfoText[0], white, UI::kBlack);
        } else {
            p.Text(box.left, box.right, box.top + 0x37,
                   IndString(kGameStrList, TotalFleetHolds() > ShipTotalHolds() ? 0x10c : 0x10d), light,
                   UI::Align::Center);
        }
    } else if (selPlayerInfoMode == 3) {
        count = 0;
        for (i = 0; i < kMaxOutfits; i++) {
            if (shipSystems[i] > 0 && !lcOutfitName[i].empty() &&
                    (outfitSystem[i].flags & kOutfShowInRanks) == 0) {
                count++;
                break;
            }
        }
        if (count > 0) {
            box2 = box;
            box2.bottom -= 10;
            box2.right -= 5;
            box2.left += 5;
            box2.top += 5;
            p.TextBox(box2, playerInfoText[1], white, UI::kBlack);
        } else {
            p.Text(box.left, box.right, box.top + 0x37, IndString(kGameStrList, 0x10e), light, UI::Align::Center);
        }
    } else if (selPlayerInfoMode == 4) {
        count = 0;
        for (i = 0; i < kMaxRanks; i++) {
            if (rankData[i].active != 0 && rankData[i].loaded != 0) {
                count++;
                break;
            }
        }
        for (i = 0; i < kMaxOutfits; i++) {
            if (shipSystems[i] > 0 && !lcOutfitName[i].empty() &&
                    (outfitSystem[i].flags & kOutfShowInRanks) != 0)
                count++;
        }
        if (count > 0) {
            box2 = box;
            box2.bottom -= 10;
            box2.right -= 5;
            box2.left += 5;
            box2.top += 5;
            p.TextBox(box2, playerInfoText[2], white, UI::kBlack);
        } else {
            p.Text(box.left, box.right, box.top + 0x37, IndString(kGameStrList, 0x10f), light, UI::Align::Center);
        }
    }

    PlayerInfoButtons(selPlayerInfoMode).Draw(p, -1);
    LoadPageToDialog(infoDialog);
}

#define kStrCargoHeader    0x16a
#define kStrFreeSpace      0x16c
#define kStrHold           0x16d
#define kStrHolds          0x16e
#define kStrNo             0x150
#define kStrFree           0x171
#define kStrA              0x189
#define kStrAn             0x18a
#define kStrOutfitHeader   0x110
#define kStrTradeInValue   0x111
#define kStrAwardsHeader   0x112

/* "ton" or "tons" for qty */
static std::string TonsWord(int32_t qty)
{
    return UpToNul(qty == 1 ? tonString : tonsString);
}

/* the count word before qty of outfit i: "a"/"an", "two", "three" or the digits */
static std::string OutfitCountWord(short i, short qty)
{
    char c;

    if (qty == 1) {
        c = tolower((unsigned char)lcOutfitName[i][0]);
        if (c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u')
            return IndString(kGameStrList, kStrAn);
        return IndString(kGameStrList, kStrA);
    }
    if (qty == 2)
        return IndString(kOrdinalsSTR, kOrdinalsBase + 2);
    if (qty == 3)
        return IndString(kOrdinalsSTR, kOrdinalsBase + 3);
    if (qty > 3)
        return NumText(qty);
    return std::string();
}

/* upper-cases the letter at pos, if there is one */
static void CapitalizeAt(std::string &text, short pos)
{
    if (pos >= 0 && (size_t)pos < text.size())
        text[pos] = (char)toupper((unsigned char)text[pos]);
}

// FUNCTION: EVNOVA 0x0049c050
// MAC: _SetupPlayerInfoText i386 0x00053cb8 ppc 0x0005c3e4
void SetupPlayerInfoText(void)
{
    Boolean rankShown[kMaxRanks];
    int32_t credits;
    Boolean starred;
    short found;
    short nextWeight;
    short maxWeight;
    short weight;
    short slot;
    short count;
    short freeSpace;
    short space;
    short shown;
    short len;
    short numJunk;
    short numTypes;
    short type;
    short qty;
    short i;
    short cargoQty[kMaxCargo];
    short outfitQty[kMaxOutfits];
    std::string name;
    std::string &cargo = playerInfoText[0];
    std::string &outfits = playerInfoText[1];
    std::string &awards = playerInfoText[2];

    for (qty = 0; qty < 3; qty++)
        playerInfoText[qty].clear();
    numJunk = 0;
    numTypes = 0;
    for (qty = 0; qty < kMaxCargo; qty++)
        cargoQty[qty] = -1;

    for (qty = 0; qty <= kLastCommodity; qty++) {
        if (shipData->cargo[qty] > 0) {
            if (cargoQty[qty] < 0)
                cargoQty[qty] = 0;
            cargoQty[qty] += shipData->cargo[qty];
        }
    }

    for (i = 0; i < kMaxActiveMissions; i++) {
        if (currentMissionObjectives[i].active != 0 && currentMissions[i].hasCargo != 0 &&
            currentMissions[i].cargoType != -1 && currentMissions[i].cargoQty >= 0) {
            type = currentMissions[i].cargoType;
            if (cargoQty[type] < 0)
                cargoQty[type] = 0;
            cargoQty[type] += currentMissions[i].cargoQty;
        }
    }

    for (i = 0; i < kMaxCargo; i++) {
        if (cargoQty[i] >= 0)
            numTypes++;
    }

    for (i = 0; i < kMaxJunks; i++) {
        if (junkData[i].held > 0) {
            numJunk++;
            numTypes++;
        }
    }

    cargo = TextF("%s %s:\r\r", IndString(kGameStrList, kStrCargoHeader).c_str(),
                  IndString(kGameStrList, TotalFleetHolds() > ShipTotalHolds() ? kStrHolds : kStrHold).c_str());
    len = cargo.size();

    if (numTypes > 0) {
        shown = 0;
        for (i = 0; i < kMaxCargo; i++) {
            qty = cargoQty[i];
            if (qty >= 0) {
                starred = (lcCargoName[i][0] == '*') ? (Boolean)1 : (Boolean)0;
                if (qty > 0 && starred == 0)
                    cargo += TextF("%s %s %s ", OrdinalText(qty, shown == 0).c_str(), TonsWord(qty).c_str(),
                                   IndString(kGameStrList, kStrOf).c_str());
                name = UpToNul(lcCargoName[i], 63);
                if (lcCargoName[i][0] == '*') {
                    if (!name.empty())
                        cargo.append(name, 1, std::string::npos);
                } else {
                    cargo += name;
                }
                if (shown < numTypes - 2) {
                    cargo += ", ";
                } else if (shown == numTypes - 2) {
                    if (starred != 0) {
                        cargo += ", ";
                    } else {
                        if (numTypes >= 3)
                            cargo += ",";
                        cargo += " ";
                        cargo += IndString(kGameStrList, kStrAnd);
                        cargo += " ";
                    }
                }
                shown++;
            }
        }

        if (numJunk > 0) {
            for (type = 0; type < kMaxJunks; type++) {
                if (junkData[type].held > 0) {
                    cargo += TextF("%s %s %s %s", OrdinalText(junkData[type].held, shown == 0).c_str(),
                                   TonsWord(junkData[type].held).c_str(), IndString(kGameStrList, kStrOf).c_str(),
                                   FromField(junkData[type].lcName, 64).c_str());
                    if (shown < numTypes - 2)
                        cargo += ", ";
                    if (shown == numTypes - 2) {
                        if (numTypes >= 3)
                            cargo += ",";
                        cargo += " ";
                        cargo += IndString(kGameStrList, kStrAnd);
                        cargo += " ";
                    }
                    shown++;
                }
            }
        }

        CapitalizeAt(cargo, len);
        cargo += ".\r\r";

        freeSpace = TotalFleetHolds() - TotalCargo(shipData);
        if (freeSpace > 0) {
            if (TotalFleetHolds() > ShipTotalHolds()) {
                cargo += TextF("%s %s:  %d %s", IndString(kGameStrList, kStrFreeSpace).c_str(),
                               IndString(kGameStrList, kStrHolds).c_str(), freeSpace, TonsWord(freeSpace).c_str());
                space = TotalMissionCargoSpace();
                if (space < 0)
                    space = 0;
                if (space != freeSpace && space > 0)
                    cargo += TextF("  (%d %s %s)", space, TonsWord(space).c_str(),
                                   IndString(kGameStrList, kStrFree).c_str());
                else if (space == 0 && freeSpace > 0)
                    cargo += TextF("(%s %s)", IndString(kGameStrList, kStrNo).c_str(),
                                   IndString(kGameStrList, kStrFree).c_str());
            } else {
                cargo += TextF("%s: %d %s", IndString(kGameStrList, kStrFreeSpace).c_str(), freeSpace,
                               TonsWord(freeSpace).c_str());
            }
        }
    }

    outfits = IndString(kGameStrList, kStrOutfitHeader);
    outfits += "\r\r";

    for (i = 0; i < kMaxOutfits; i++)
        outfitQty[i] = 0;

    count = 0;
    for (i = 0; i < kMaxOutfits; i++) {
        qty = shipSystems[i];
        if (qty > 0 && !lcOutfitName[i].empty() &&
            (outfitSystem[i].flags & kOutfShowInRanks) == 0) {
            if (outfitSystem[i].sameNameIndex < 0 || outfitSystem[i].sameNameIndex >= i)
                type = i;
            else
                type = outfitSystem[i].sameNameIndex;
            if (outfitQty[type] == 0) {
                if (outfitSystem[type].dispWeight > maxWeight || count == 0)
                    maxWeight = outfitSystem[type].dispWeight;
                count++;
            }
            outfitQty[type] += qty;
        }
    }

    numTypes = 0;
    len = outfits.size();
    do {
        found = 0;
        type = 0;
        nextWeight = 0;
        for (i = 0; i < kMaxOutfits; i++) {
            qty = outfitQty[i];
            if (qty > 0 && !lcOutfitName[i].empty() &&
                (outfitSystem[i].flags & kOutfShowInRanks) == 0) {
                weight = outfitSystem[i].dispWeight;
                if (maxWeight == weight) {
                    found++;
                    outfits += OutfitCountWord(i, qty);
                    outfits += " ";
                    if (qty <= 3 && len > 0)
                        CapitalizeAt(outfits, len);
                    len = -1;
                    if (qty > 1)
                        outfits += lcPluralOutfitName[i];
                    else
                        outfits += lcOutfitName[i];
                    numTypes++;
                    if (numTypes <= count - 1 && count > 2)
                        outfits += ",";
                    if (numTypes < count - 1) {
                        outfits += " ";
                    } else if (numTypes == count - 1) {
                        outfits += " ";
                        outfits += IndString(kGameStrList, kStrAnd);
                        outfits += " ";
                    }
                    outfitQty[i] = 0;
                } else if (weight < maxWeight) {
                    if (weight > nextWeight || type == 0)
                        nextWeight = weight;
                }
                type++;
            }
        }
        maxWeight = nextWeight;
    } while (found > 0);

    outfits += ".\r\r";

    credits = PlayerShipTradeInPrice();
    if (credits > 0) {
        outfits += TextF("%s %s %s\r", IndString(kGameStrList, kStrTradeInValue).c_str(), CommaNumber(credits).c_str(),
                         FromField(credits == 1 ? creditCString : creditsCString, sizeof creditsCString).c_str());
    }

    count = 0;
    for (i = 0; i < kMaxRanks; i++) {
        rankShown[i] = 0;
        if (rankData[i].active != 0 && rankData[i].loaded != 0 &&
            strlen(rankData[i].name) != 0) {
            if (rankData[i].priority > maxWeight || count == 0)
                maxWeight = rankData[i].priority;
            count++;
        }
    }

    for (i = 0; i < kMaxOutfits; i++)
        outfitQty[i] = 0;

    for (i = 0; i < kMaxOutfits; i++) {
        qty = shipSystems[i];
        if (qty > 0 && !lcOutfitName[i].empty() &&
            (outfitSystem[i].flags & kOutfShowInRanks) != 0) {
            slot = outfitSystem[i].sameNameIndex;
            if (slot < 0 || slot >= i)
                slot = i;
            outfitQty[slot] += qty;
        }
    }

    for (i = 0; i < kMaxOutfits; i++) {
        if (outfitQty[i] > 0 && !lcOutfitName[i].empty() &&
            (outfitSystem[i].flags & kOutfShowInRanks) != 0)
            count++;
    }

    if (count > 0) {
        awards = IndString(kGameStrList, kStrAwardsHeader);
        awards += "\r\r";
        shown = 0;
        do {
            for (i = 0; i < kMaxRanks; i++) {
                if (rankData[i].active != 0 && rankData[i].loaded != 0 &&
                    strlen(rankData[i].name) != 0 &&
                    rankData[i].priority >= maxWeight && rankShown[i] == 0) {
                    awards += RankName(i);
                    shown++;
                    rankShown[i] = 1;
                    if (shown <= count - 1)
                        awards += ",";
                    if (shown < count - 1) {
                        awards += " ";
                    } else if (shown == count - 1) {
                        awards += " ";
                        awards += IndString(kGameStrList, kStrAnd);
                        awards += " ";
                    }
                }
            }
            found = 0;
            for (i = 0; i < kMaxRanks; i++) {
                if (rankData[i].active != 0 && rankData[i].loaded != 0 &&
                    strlen(rankData[i].name) != 0 && rankShown[i] == 0) {
                    if (rankData[i].priority > maxWeight || found == 0)
                        maxWeight = rankData[i].priority;
                    found++;
                }
            }
        } while (found > 0);

        if (shown != 0)
            len = -1;
        else
            len = awards.size();

        for (i = 0; i < kMaxOutfits; i++) {
            qty = outfitQty[i];
            if (qty > 0 && !lcOutfitName[i].empty() &&
                (outfitSystem[i].flags & kOutfShowInRanks) != 0) {
                awards += OutfitCountWord(i, qty);
                awards += " ";
                if (qty <= 3 && len > 0)
                    CapitalizeAt(awards, len);
                len = -1;
                if (qty > 1)
                    awards += lcPluralOutfitName[i];
                else
                    awards += lcOutfitName[i];
                shown++;
                if (shown <= count - 1)
                    awards += ", ";
            }
        }
        awards += "\r";
    }
}
