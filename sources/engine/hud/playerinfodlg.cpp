/* playerinfodlg.cpp -- the player info dialog */

#include "scaleddraw.h"
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

DialogPtr infoDialog;
short selPlayerInfoMode;
PicHandle playerInfoBackgroundPic[3];
char playerInfoText[3][4096];

/* The player info dialog's background, stretched as one picture: the middle picture fills what the top
   and bottom ones leave (at least its own height). */
/* "  (current / maximum)" after a percentage, current kept to 0 or more */
static void DrawRawValue(double current, double maximum)
{
    Str255 num;

    DrawString("\p  (");
    NumToString((int32_t)(current > 0.0 ? current : 0.0), num);
    DrawString(num);
    DrawString("\p / ");
    NumToString((int32_t)maximum, num);
    DrawString(num);
    DrawString("\p)");
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

static void DrawDays(short days)
{
    Str255 num;

    NumToString(days, num);
    DrawString(num);
    DrawString(days == 1 ? "\p day" : "\p days");
}

static void DrawPlayerInfoBackground(const Rect &port)
{
    UI::ArtComposition background(CurrentGrafPort, port);
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
        DrawPicHandle(playerInfoBackgroundPic[1], &box);
    }

    if (playerInfoBackgroundPic[0] != NULL) {
        QDGetPictureBounds(playerInfoBackgroundPic[0], &box);
        OffsetRect(&box, -box.left, -box.top);
        OffsetRect(&box, port.left, port.top);
        DrawPicHandle(playerInfoBackgroundPic[0], &box);
    }

    if (playerInfoBackgroundPic[2] != NULL) {
        QDGetPictureBounds(playerInfoBackgroundPic[2], &box);
        OffsetRect(&box, -box.left, -box.top);
        OffsetRect(&box, port.left, port.bottom - (box.bottom - box.top));
        DrawPicHandle(playerInfoBackgroundPic[2], &box);
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
    char *p;

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
    SetDialogGWorld(infoDialog);
    SetupPlayerInfoText();
    saved = *GetDialogPortRect(infoDialog);
    GetDialogItem(infoDialog, 6, &itemType, &item, &box6);
    GetDialogItem(infoDialog, 1, &itemType, &item, &buttonBox[0]);
    GetDialogItem(infoDialog, 7, &itemType, &item, &buttonBox[1]);
    textDialogTextHandle = TENew(&box6, &box6);
    (*textDialogTextHandle)->txFont = smallTextFont;
    (*textDialogTextHandle)->txSize = smallTextSize;
    TextFont((*textDialogTextHandle)->txFont);
    TextSize((*textDialogTextHandle)->txSize);
    TESetLineMetrics(textDialogTextHandle);
    tallest = -1;
    for (i = 0; i < 3; i++) {
        p = playerInfoText[i];
        TESetText(p, strlen(p), textDialogTextHandle);
        TECalText(textDialogTextHandle);
        height = TEWrappedHeight(textDialogTextHandle);
        if (height > tallest || tallest <= 0)
            tallest = height;
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
    SetDialogGWorld(infoDialog);
    FlushEvents(everyEvent - diskMask, 0);
    RedrawDialog(infoDialog);
    PlayerInfoDialogUpdate();
    Loop();
    SWSetPortToWindow(gSpriteWorld);
    while (KeyCheck(keyPrefs.playerInfo))
        BusyIdle();
    dialog.Close();
    for (i = 0; i < 3; i++)
        backdrops[i].Release();
    SWSetPortToWindow(gSpriteWorld);
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
    unsigned char cheatBuf[256];
    PStrChar buf[2048];
    short i;
    int32_t stellarWorth;
    int32_t escortWorth;
    short sys;
    short jumps;
    short count;
    Boolean onMission;

    SetGWorldContainer(&loadPort);
    ForeColor(blackColor);
    PaintRect(GetDialogPortRect(infoDialog));

    DrawPlayerInfoBackground(*GetDialogPortRect(infoDialog));

    TextFont(smallTextFont);
    TextSize(smallTextSize);
    GetDialogItem(infoDialog, 6, &itemType, (Handle *)&itemHandle, &box);
    if (!RectInRgn(&box, GetWindowVisRgn(FrontWindow(infoDialog))))
        goto blit_back;

    if (selPlayerInfoMode == 1) {
        /* --- left column --- */
        RGBForeColor(&rgbLightGrey);
        MoveTo((box.left + 5), (box.top + 0xc));
        DrawIndString(kGameStrList, 0xfb);
        ForeColor(whiteColor);
        MoveTo((box.left + 0x50), (box.top + 0xc));
        DrawString(playerName);

        RGBForeColor(&rgbLightGrey);
        MoveTo((box.left + 5), (box.top + 0x1c));
        DrawIndString(kGameStrList, 0xfc);
        ForeColor(whiteColor);
        MoveTo((box.left + 0x50), (box.top + 0x1c));
        DateToMedString(gameTime, (char *)buf);
        CtoPstr((char *)buf);
        DrawString((const unsigned char *)buf);

        RGBForeColor(&rgbLightGrey);
        MoveTo((box.left + 5), (box.top + 0x2c));
        DrawIndString(kGameStrList, 0xfd);
        ForeColor(whiteColor);
        MoveTo((box.left + 0x50), (box.top + 0x2c));
        DrawString(systemData[shipData->curSystem].name);
        if (cheatsActive) {
            DrawString("\p (");
            NumToString(shipData->curSystem + 128, cheatBuf);
            DrawString(cheatBuf);
            DrawString("\p)");
        }

        RGBForeColor(&rgbLightGrey);
        MoveTo((box.left + 5), (box.top + 0x3c));
        DrawIndString(kGameStrList, 0x146);
        ForeColor(whiteColor);
        MoveTo((box.left + 0x50), (box.top + 0x3c));
        if (SystemIsInhabited(shipData->curSystem)) {
            DrawLegalStatusString(shipData->curSystem);
            if (cheatsActive) {
                DrawString("\p (");
                NumToString(playerRecord[shipData->curSystem], cheatBuf);
                DrawString(cheatBuf);
                DrawString("\p)");
            }
        } else {
            DrawIndString(kGameStrList, 0x18c);
        }

        RGBForeColor(&rgbLightGrey);
        MoveTo((box.left + 5), (box.top + 0x4c));
        DrawIndString(kGameStrList, 0xfe);
        ForeColor(whiteColor);
        MoveTo((box.left + 0x50), (box.top + 0x4c));
        DrawCombatRatingString();
        if (cheatsActive) {
            DrawString("\p (");
            NumToString(playerKills, cheatBuf);
            DrawString(cheatBuf);
            DrawString("\p)");
        }

        /* --- right column --- */
        RGBForeColor(&rgbLightGrey);
        MoveTo((box.left + 0xcd), (box.top + 0xc));
        DrawIndString(kGameStrList, 0xff);
        ForeColor(whiteColor);
        MoveTo((box.left + 0x118), (box.top + 0xc));
        DrawString(shipName);

        RGBForeColor(&rgbLightGrey);
        MoveTo((box.left + 0xcd), (box.top + 0x1c));
        DrawIndString(kGameStrList, 0x100);
        ForeColor(whiteColor);
        MoveTo((box.left + 0x118), (box.top + 0x1c));
        DrawString(shipClass[shipData->type].name);
        if (cheatsActive) {
            DrawString("\p (");
            NumToString(shipData->type + 128, cheatBuf);
            DrawString(cheatBuf);
            DrawString("\p)");
        }

        RGBForeColor(&rgbLightGrey);
        MoveTo((box.left + 0xcd), (box.top + 0x2c));
        DrawIndString(kGameStrList, 0x101);
        ForeColor(whiteColor);
        MoveTo((box.left + 0x118), (box.top + 0x2c));
        NumToString(TruncF32(shipData->TurnRate() * 30.0f), (StringPtr)buf);
        DrawString((const unsigned char *)buf);
        DrawIndString(kGameStrList, 0x102);

        RGBForeColor(&rgbLightGrey);
        MoveTo((box.left + 0xcd), (box.top + 0x3c));
        DrawIndString(kGameStrList, 0x103);
        ForeColor(whiteColor);
        MoveTo((box.left + 0x118), (box.top + 0x3c));
        NumToString(TruncF32(shipData->AccelRate() * 2500.0), (StringPtr)buf);
        DrawString((const unsigned char *)buf);

        RGBForeColor(&rgbLightGrey);
        MoveTo((box.left + 0xcd), (box.top + 0x4c));
        DrawIndString(kGameStrList, 0x104);
        ForeColor(whiteColor);
        MoveTo((box.left + 0x118), (box.top + 0x4c));
        if (strictPlay)
            NumToString(TruncF32(shipData->MaxSpeed() * 100.0f), (StringPtr)buf);
        else
            NumToString(TruncF32(shipData->MaxSpeed() * 100.0f *
                                    0.6666666666666666), (StringPtr)buf);
        DrawString((const unsigned char *)buf);

        RGBForeColor(&rgbLightGrey);
        MoveTo((box.left + 0xcd), (box.top + 0x5c));
        pstrcpy((StringPtr)buf, creditsString);
        if ((char)buf[0] > 0)
            buf[1] = toupper((char)buf[1]);
        DrawString((const unsigned char *)buf);
        DrawString("\p:");
        ForeColor(whiteColor);
        MoveTo((box.left + 0x118), (box.top + 0x5c));
        DrawCommaNumber(shipData->credits);

        RGBForeColor(&rgbLightGrey);
        MoveTo((box.left + 0xcd), (box.top + 0x6c));
        DrawString("\pJump Time:");
        ForeColor(whiteColor);
        MoveTo((box.left + 0x118), (box.top + 0x6c));
        short ownDays = shipData->HyperTransitTime();
        short fleetDays = FleetJumpDays();
        DrawDays(ownDays);
        if (fleetDays > ownDays) {
            DrawString("\p (");
            DrawDays(fleetDays);
            DrawString("\p with escorts)");
        }

        /* --- shields / armor / fuel, left column continued --- */
        RGBForeColor(&rgbLightGrey);
        MoveTo((box.left + 5), (box.top + 0x5c));
        DrawString(shieldString[1]);
        ForeColor(whiteColor);
        MoveTo((box.left + 0x50), (box.top + 0x5c));
        if (shipData->ShieldCapacity() > 0.0) {
            if (shipData->shield >= 0.0f) {
                if (shipData->shield <= shipData->ShieldCapacity()) {
                    NumToString((shipData->shield /
                                       shipData->ShieldCapacity() * 100.0), (StringPtr)buf);
                    DrawString((const unsigned char *)buf);
                    DrawString("\p%");
                } else {
                    DrawString("\p100%");
                }
            } else {
                DrawString("\p0%");
            }
            DrawRawValue(shipData->shield, shipData->ShieldCapacity());
        } else {
            DrawIndString(kGameStrList, 0x18c);
        }

        RGBForeColor(&rgbLightGrey);
        MoveTo((box.left + 5), (box.top + 0x6c));
        DrawString(armorString[1]);
        ForeColor(whiteColor);
        MoveTo((box.left + 0x50), (box.top + 0x6c));
        if (shipData->IsDying()) {
            DrawIndString(kGameStrList, 0x105);
        } else if (shipData->ArmorCapacity() > 0.0) {
            if (shipData->armor <= shipData->ArmorCapacity()) {
                NumToString((shipData->armor /
                                   shipData->ArmorCapacity() * 100.0), (StringPtr)buf);
                DrawString((const unsigned char *)buf);
                DrawString("\p%");
            } else {
                DrawString("\p100%");
            }
            DrawRawValue(shipData->armor, shipData->ArmorCapacity());
        } else {
            DrawIndString(kGameStrList, 0x18c);
        }

        RGBForeColor(&rgbLightGrey);
        MoveTo((box.left + 5), (box.top + 0x7c));
        DrawString(fuelString[5]);
        ForeColor(whiteColor);
        MoveTo((box.left + 0x50), (box.top + 0x7c));
        if (shipData->FuelCapacity() <= 0.0) {
            DrawIndString(kGameStrList, 0x14f);
        } else {
            NumToString((shipData->fuel /
                               shipData->FuelCapacity() * 100.0), (StringPtr)buf);
            DrawString((const unsigned char *)buf);
            DrawString("\p%");
            jumps = (shipData->fuel / 100.0);
            if (jumps > 0) {
                DrawString("\p  (");
                NumToString(jumps, (StringPtr)buf);
                DrawString((const unsigned char *)buf);
                DrawString("\p ");
                if (jumps > 1)
                    DrawIndString(kGameStrList, 0xf0);
                else
                    DrawIndString(kGameStrList, 0xef);
                if ((short)shipData->fuel % 100 > 0) {
                    DrawString("\p ");
                    DrawIndString(kGameStrList, 0x106);
                    DrawString("\p ");
                    DrawString(fuelString[6]);
                }
                DrawString("\p)");
            } else if (shipData->fuel > 0.0f) {
                DrawString("\p  (");
                DrawString(fuelString[6]);
                DrawString("\p ");
                DrawIndString(kGameStrList, 0x107);
                DrawString("\p)");
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
            RGBForeColor(&rgbLightGrey);
            MoveTo((box.left + 5), (box.top + 0x8c));
            if (escortWorth > stellarWorth) {
                DrawIndString(kGameStrList, 0x108);
                DrawString("\p  ");
                ForeColor(whiteColor);
                DrawCommaNumber(escortWorth - stellarWorth);
            } else if (escortWorth != 0) {
                DrawIndString(kGameStrList, 0x10a);
                DrawString("\p ");
                DrawIndString(kGameStrList, 0x109);
                DrawString("\p  ");
                ForeColor(whiteColor);
                DrawCommaNumber(stellarWorth - escortWorth);
            } else {
                DrawIndString(kGameStrList, 0x109);
                DrawString("\p  ");
                ForeColor(whiteColor);
                DrawCommaNumber(stellarWorth);
            }
            DrawString("\p ");
            DrawString(creditsString);
            RGBForeColor(&rgbLightGrey);
            DrawString("\p ");
            DrawIndString(kGameStrList, 0x10b);
        }
    } else if (selPlayerInfoMode == 2) {
        if (PlayerHasAnyCargo()) {
            ForeColor(blackColor);
            box2 = box;
            box2.bottom -= 10;
            box2.right -= 5;
            box2.left += 5;
            box2.top += 5;
            TETextBox(playerInfoText[0], strlen(playerInfoText[0]), &box2, 0);
            InvertRect(&box2);
        } else {
            RGBForeColor(&rgbLightGrey);
            if (TotalFleetHolds() > ShipTotalHolds())
                GetIndString((StringPtr)buf, kGameStrList, 0x10c);
            else
                GetIndString((StringPtr)buf, kGameStrList, 0x10d);
            DrawStringCentered((StringPtr)buf, box.left, box.right, box.top + 0x37);
        }
    } else if (selPlayerInfoMode == 3) {
        count = 0;
        for (i = 0; i < kMaxOutfits; i++) {
            if (shipSystems[i] > 0 && lcOutfitName[i][0] != 0 &&
                    (outfitSystem[i].flags & kOutfShowInRanks) == 0) {
                count++;
                break;
            }
        }
        if (count > 0) {
            ForeColor(blackColor);
            box2 = box;
            box2.bottom -= 10;
            box2.right -= 5;
            box2.left += 5;
            box2.top += 5;
            TETextBox(playerInfoText[1], strlen(playerInfoText[1]), &box2, 0);
            InvertRect(&box2);
        } else {
            RGBForeColor(&rgbLightGrey);
            GetIndString((StringPtr)buf, kGameStrList, 0x10e);
            DrawStringCentered((StringPtr)buf, box.left, box.right, box.top + 0x37);
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
            if (shipSystems[i] > 0 && lcOutfitName[i][0] != 0 &&
                    (outfitSystem[i].flags & kOutfShowInRanks) != 0)
                count++;
        }
        if (count > 0) {
            ForeColor(blackColor);
            box2 = box;
            box2.bottom -= 10;
            box2.right -= 5;
            box2.left += 5;
            box2.top += 5;
            TETextBox(playerInfoText[2], strlen(playerInfoText[2]), &box2, 0);
            InvertRect(&box2);
        } else {
            RGBForeColor(&rgbLightGrey);
            GetIndString((StringPtr)buf, kGameStrList, 0x10f);
            DrawStringCentered((StringPtr)buf, box.left, box.right, box.top + 0x37);
        }
    }

blit_back:
    PlayerInfoButtons(selPlayerInfoMode).Draw(-1);
    CopyLoadPortToDialog(infoDialog);
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

// FUNCTION: EVNOVA 0x0049c050
// MAC: _SetupPlayerInfoText i386 0x00053cb8 ppc 0x0005c3e4
void SetupPlayerInfoText(void)
{
    Boolean rankShown[kMaxRanks];
    int32_t credits;
    char c;
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
    unsigned char str[256];

    for (qty = 0; qty < 3; qty++)
        playerInfoText[qty][0] = 0;
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

    StrcatIndCString(playerInfoText[0], kGameStrList, kStrCargoHeader);
    strcat(playerInfoText[0], " ");
    if (TotalFleetHolds() > ShipTotalHolds())
        StrcatIndCString(playerInfoText[0], kGameStrList, kStrHolds);
    else
        StrcatIndCString(playerInfoText[0], kGameStrList, kStrHold);
    strcat(playerInfoText[0], ":\r\r");
    len = strlen(playerInfoText[0]);

    if (numTypes > 0) {
        shown = 0;
        for (i = 0; i < kMaxCargo; i++) {
            qty = cargoQty[i];
            if (qty >= 0) {
                starred = (lcCargoName[i][1] == '*') ? (Boolean)1 : (Boolean)0;
                if (qty > 0 && starred == 0) {
                    if (shown == 0)
                        StrcatOrdinalNumber(qty, playerInfoText[0], 1);
                    else
                        StrcatOrdinalNumber(qty, playerInfoText[0], 0);
                    strcat(playerInfoText[0], " ");
                    if (qty == 1)
                        pstrcpy(str, tonString);
                    else
                        pstrcpy(str, tonsString);
                    PtoCstr(str);
                    strcat(playerInfoText[0], (char *)str);
                    strcat(playerInfoText[0], " ");
                    StrcatIndCString(playerInfoText[0], kGameStrList, kStrOf);
                    strcat(playerInfoText[0], " ");
                }
                /* BUG: copies 64 bytes of a Pascal name, so a name over 63 characters keeps stale bytes */
                strncpy((char *)str, (char *)lcCargoName[i], 64);
                PtoCstr(str);
                if (lcCargoName[i][1] == '*')
                    strcat(playerInfoText[0], (char *)&str[1]);
                else
                    strcat(playerInfoText[0], (char *)str);
                if (shown < numTypes - 2) {
                    strcat(playerInfoText[0], ", ");
                } else if (shown == numTypes - 2) {
                    if (starred != 0) {
                        strcat(playerInfoText[0], ", ");
                    } else {
                        if (numTypes >= 3)
                            strcat(playerInfoText[0], ",");
                        strcat(playerInfoText[0], " ");
                        StrcatIndCString(playerInfoText[0], kGameStrList, kStrAnd);
                        strcat(playerInfoText[0], " ");
                    }
                }
                shown++;
            }
        }

        if (numJunk > 0) {
            for (type = 0; type < kMaxJunks; type++) {
                if (junkData[type].held > 0) {
                    if (shown == 0)
                        StrcatOrdinalNumber(junkData[type].held, playerInfoText[0], 1);
                    else
                        StrcatOrdinalNumber(junkData[type].held, playerInfoText[0], 0);
                    strcat(playerInfoText[0], " ");
                    if (junkData[type].held == 1)
                        pstrcpy(str, tonString);
                    else
                        pstrcpy(str, tonsString);
                    PtoCstr(str);
                    strcat(playerInfoText[0], (char *)str);
                    strcat(playerInfoText[0], " ");
                    StrcatIndCString(playerInfoText[0], kGameStrList, kStrOf);
                    strcat(playerInfoText[0], " ");
                    /* BUG: a junk name of 64+ characters is left unterminated for the strcat below */
                    strncpy((char *)str, junkData[type].lcName, 64);
                    strcat(playerInfoText[0], (char *)str);
                    if (shown < numTypes - 2)
                        strcat(playerInfoText[0], ", ");
                    if (shown == numTypes - 2) {
                        if (numTypes >= 3)
                            strcat(playerInfoText[0], ",");
                        strcat(playerInfoText[0], " ");
                        StrcatIndCString(playerInfoText[0], kGameStrList, kStrAnd);
                        strcat(playerInfoText[0], " ");
                    }
                    shown++;
                }
            }
        }

        if (len != -1)
            playerInfoText[0][len] = toupper(playerInfoText[0][len]);
        strcat(playerInfoText[0], ".");
        strcat(playerInfoText[0], "\r\r");

        freeSpace = TotalFleetHolds() - TotalCargo(shipData);
        if (freeSpace > 0) {
            if (freeSpace < 0)
                freeSpace = 0;
            NumToString(freeSpace, str);
            if (TotalFleetHolds() > ShipTotalHolds()) {
                StrcatIndCString(playerInfoText[0], kGameStrList, kStrFreeSpace);
                strcat(playerInfoText[0], " ");
                StrcatIndCString(playerInfoText[0], kGameStrList, kStrHolds);
                strcat(playerInfoText[0], ":  ");
                PtoCstr(str);
                strcat(playerInfoText[0], (char *)str);
                strcat(playerInfoText[0], " ");
                if (freeSpace == 1)
                    pstrcpy(str, tonString);
                else
                    pstrcpy(str, tonsString);
                PtoCstr(str);
                strcat(playerInfoText[0], (char *)str);
                space = TotalMissionCargoSpace();
                if (space < 0)
                    space = 0;
                if (space != freeSpace && space > 0) {
                    strcat(playerInfoText[0], "  (");
                    NumToString(space, str);
                    PtoCstr(str);
                    strcat(playerInfoText[0], (char *)str);
                    strcat(playerInfoText[0], " ");
                    if (space == 1)
                        pstrcpy(str, tonString);
                    else
                        pstrcpy(str, tonsString);
                    PtoCstr(str);
                    strcat(playerInfoText[0], (char *)str);
                    strcat(playerInfoText[0], " ");
                    StrcatIndCString(playerInfoText[0], kGameStrList, kStrFree);
                    strcat(playerInfoText[0], ")");
                } else if (space == 0 && freeSpace > 0) {
                    strcat(playerInfoText[0], "(");
                    StrcatIndCString(playerInfoText[0], kGameStrList, kStrNo);
                    strcat(playerInfoText[0], " ");
                    StrcatIndCString(playerInfoText[0], kGameStrList, kStrFree);
                    strcat(playerInfoText[0], ")");
                }
            } else {
                StrcatIndCString(playerInfoText[0], kGameStrList, kStrFreeSpace);
                strcat(playerInfoText[0], ": ");
                PtoCstr(str);
                strcat(playerInfoText[0], (char *)str);
                strcat(playerInfoText[0], " ");
                if (freeSpace == 1)
                    pstrcpy(str, tonString);
                else
                    pstrcpy(str, tonsString);
                PtoCstr(str);
                strcat(playerInfoText[0], (char *)str);
            }
        }
    }

    StrcatIndCString(playerInfoText[1], kGameStrList, kStrOutfitHeader);
    strcat(playerInfoText[1], "\r\r");

    for (i = 0; i < kMaxOutfits; i++)
        outfitQty[i] = 0;

    count = 0;
    for (i = 0; i < kMaxOutfits; i++) {
        qty = shipSystems[i];
        if (qty > 0 && lcOutfitName[i][0] != 0 &&
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
    len = strlen(playerInfoText[1]);
    do {
        found = 0;
        type = 0;
        nextWeight = 0;
        for (i = 0; i < kMaxOutfits; i++) {
            qty = outfitQty[i];
            if (qty > 0 && lcOutfitName[i][0] != 0 &&
                (outfitSystem[i].flags & kOutfShowInRanks) == 0) {
                weight = outfitSystem[i].dispWeight;
                if (maxWeight == weight) {
                    found++;
                    if (qty == 1) {
                        c = tolower(lcOutfitName[i][1]);
                        if (c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u')
                            StrcatIndCString(playerInfoText[1], kGameStrList, kStrAn);
                        else
                            StrcatIndCString(playerInfoText[1], kGameStrList, kStrA);
                    } else if (qty == 2) {
                        StrcatIndCString(playerInfoText[1], kOrdinalsSTR, kOrdinalsBase + 2);
                    } else if (qty == 3) {
                        StrcatIndCString(playerInfoText[1], kOrdinalsSTR, kOrdinalsBase + 3);
                    } else if (qty > 3) {
                        NumToString(qty, str);
                        PtoCstr(str);
                        strcat(playerInfoText[1], (char *)str);
                    }
                    strcat(playerInfoText[1], " ");
                    if (qty <= 3 && len > 0)
                        playerInfoText[1][len] = toupper(playerInfoText[1][len]);
                    len = -1;
                    if (qty > 1)
                        pstrcpy(str, lcPluralOutfitName[i]);
                    else
                        pstrcpy(str, lcOutfitName[i]);
                    PtoCstr(str);
                    strcat(playerInfoText[1], (char *)str);
                    numTypes++;
                    if (numTypes <= count - 1 && count > 2)
                        strcat(playerInfoText[1], ",");
                    if (numTypes < count - 1) {
                        strcat(playerInfoText[1], " ");
                    } else if (numTypes == count - 1) {
                        strcat(playerInfoText[1], " ");
                        StrcatIndCString(playerInfoText[1], kGameStrList, kStrAnd);
                        strcat(playerInfoText[1], " ");
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

    strcat(playerInfoText[1], ".\r\r");

    credits = PlayerShipTradeInPrice();
    if (credits > 0) {
        StrcatIndCString(playerInfoText[1], kGameStrList, kStrTradeInValue);
        strcat(playerInfoText[1], " ");
        StrcatCommaNumber(credits, playerInfoText[1]);
        strcat(playerInfoText[1], " ");
        if (credits == 1)
            strcat(playerInfoText[1], creditCString);
        else
            strcat(playerInfoText[1], creditsCString);
        strcat(playerInfoText[1], "\r");
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
        if (qty > 0 && lcOutfitName[i][0] != 0 &&
            (outfitSystem[i].flags & kOutfShowInRanks) != 0) {
            slot = outfitSystem[i].sameNameIndex;
            if (slot < 0 || slot >= i)
                slot = i;
            outfitQty[slot] += qty;
        }
    }

    for (i = 0; i < kMaxOutfits; i++) {
        if (outfitQty[i] > 0 && lcOutfitName[i][0] != 0 &&
            (outfitSystem[i].flags & kOutfShowInRanks) != 0)
            count++;
    }

    if (count > 0) {
        strcpy(playerInfoText[2], "");
        StrcatIndCString(playerInfoText[2], kGameStrList, kStrAwardsHeader);
        strcat(playerInfoText[2], "\r\r");
        shown = 0;
        do {
            for (i = 0; i < kMaxRanks; i++) {
                if (rankData[i].active != 0 && rankData[i].loaded != 0 &&
                    strlen(rankData[i].name) != 0 &&
                    rankData[i].priority >= maxWeight && rankShown[i] == 0) {
                    strcat(playerInfoText[2], rankData[i].name);
                    shown++;
                    rankShown[i] = 1;
                    if (shown <= count - 1)
                        strcat(playerInfoText[2], ",");
                    if (shown < count - 1) {
                        strcat(playerInfoText[2], " ");
                    } else if (shown == count - 1) {
                        strcat(playerInfoText[2], " ");
                        StrcatIndCString(playerInfoText[2], kGameStrList, kStrAnd);
                        strcat(playerInfoText[2], " ");
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
            len = strlen(playerInfoText[2]);

        for (i = 0; i < kMaxOutfits; i++) {
            qty = outfitQty[i];
            if (qty > 0 && lcOutfitName[i][0] != 0 &&
                (outfitSystem[i].flags & kOutfShowInRanks) != 0) {
                if (qty == 1) {
                    c = tolower(lcOutfitName[i][1]);
                    if (c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u')
                        StrcatIndCString(playerInfoText[2], kGameStrList, kStrAn);
                    else
                        StrcatIndCString(playerInfoText[2], kGameStrList, kStrA);
                } else if (qty == 2) {
                    StrcatIndCString(playerInfoText[2], kOrdinalsSTR, kOrdinalsBase + 2);
                } else if (qty == 3) {
                    StrcatIndCString(playerInfoText[2], kOrdinalsSTR, kOrdinalsBase + 3);
                } else if (qty > 3) {
                    NumToString(qty, str);
                    PtoCstr(str);
                    strcat(playerInfoText[2], (char *)str);
                }
                strcat(playerInfoText[2], " ");
                if (qty <= 3 && len > 0)
                    playerInfoText[2][len] = toupper(playerInfoText[2][len]);
                len = -1;
                if (qty > 1)
                    pstrcpy(str, lcPluralOutfitName[i]);
                else
                    pstrcpy(str, lcOutfitName[i]);
                PtoCstr(str);
                strcat(playerInfoText[2], (char *)str);
                shown++;
                if (shown <= count - 1)
                    strcat(playerInfoText[2], ", ");
            }
        }
        strcat(playerInfoText[2], "\r");
    }
}
