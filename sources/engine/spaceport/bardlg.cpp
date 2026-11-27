/* bardlg.cpp -- the bar and dialog-update object (tu-map O21, win 0x0047c8e0..0x0047e1e0). Notes: Research/notes/bardlg.md */

#include "ai.h"
#include "sound.h"
#include "game.h"
#include "stellar.h"
#include "gameutil.h"
#include "missions.h"
#include "render.h"
#include "ships.h"
#include "gamedialog.h"
#include "buttons.h"
#include "text.h"
#include "painter.h"

#define kBarDlogID          0x3fd   /* DLOG 1021, with portrait */
#define kPlainBarDlogID     0x3f5   /* DLOG 1013 */
#define kBarPictID          0x2138
#define kPlainBarPictID     0x2137
#define kBarVenue           1
#define kBrokeStrIndex      0x169
#define kBarLeaveItem       1
#define kBarRaceItem        2
#define kBarNewsItem        3
#define kBarHireItem        5
#define kBarOfferItem       6       /* a mission offer falls due */
#define kBarDescItem        7
#define kBarPortraitItem    8
#define kBarMapItem         8       /* the map quick key */
#define kBarPlayerInfoItem  9
#define kBarMissionInfoItem 10

PicHandle newsPic;
PicHandle barPic;
PicHandle barBackgroundPic;
PicHandle raceBackgroundPic;
PicHandle raceButtonPic[4][4];
DialogPtr barDialog;
DialogPtr newsDialog;
DialogPtr raceDialog;
char barDescText[2048];
std::string newsString;
std::string sponsorString;
short racePick;
short raceWinner;
short lastRaceWinner;
short raceCount;
int32_t raceBetAmount;

namespace {

// FUNCTION: EVNOVA 0x0047c8e0
// MAC: _DoBarDialog i386 0x00048ef5 ppc 0x00051898
/* The spaceport bar: the races, the news, hiring escorts and bar mission offers. */
class BarDialog final : public GameDialog {
public:
    void Run();

private:
    void Hit(short item) override;
    static Boolean Filter(DialogPtr unused, const DialogEvent &ev, short *itemHit);

    short savedStellar;     /* the nav target the map may change, restored after it */
    short savedRoute;
};

void BarDialog::Run()
{
    Picture portrait(barPic);

    savedStellar = shipData->targetStellar;
    savedRoute = shipData->navTargetKind;
    RandomizeNewsDialog();
    missionsOutside = 1;
    offeringFromShip = 0;
    if (dockedPortMissions[kBarVenue][0] == -1)
        nextPortMission = -1;
    else
        nextPortMission = 0;
    missionOfferTime = ReadTick() + 0xf;
    lastRaceWinner = -1;
    strncpy(barDescText, "", 3);
    LoadDescription(barDescText, shipData->targetStellar + 10000);
    barPic = NULL;
    if (descResourcePictId >= kFirstResourceID)
        portrait.Load(descResourcePictId);
    dialog.Open(barPic != NULL ? kBarDlogID : kPlainBarDlogID, BarDialogUpdate, Filter, &barDialog);
    if (!dialog.IsOpen())
        return;
    Picture backdrop(barBackgroundPic);
    backdrop.Load(barPic != NULL ? kBarPictID : kPlainBarPictID);
    FlushEvents(everyEvent - diskMask, 0);
    RedrawDialog(barDialog);
    BarDialogUpdate();
    Loop();
    dialog.Close();
    backdrop.Release();
    portrait.Release();
    ResetMissionOfferVenue();
}

void BarDialog::Hit(short item)
{
    if (item == kBarLeaveItem)
        done = 1;
    if (item == kBarRaceItem) {
        if (shipData->credits > 0) {
            DoRaceDialog();
        } else {
            SetShowText(IndString(kGameStrList, kBrokeStrIndex));
            ShowTextDialog(0, 0);
        }
        PortDialogUpdate();
        BarDialogUpdate();
    }
    if (item == kBarNewsItem) {
        BarButtons().DrawOnDialog(-1);
        DoNewsDialog();
        PortDialogUpdate();
        BarDialogUpdate();
    }
    if (item == kBarHireItem) {
        if (CanHireEscorts()) {
            BarButtons().DrawOnDialog(-1);
            shipBuyMode = 1;
            DoShipyardDialog();
            ShipStatsToSystemInfo();
            DrawStatusCargo();
            RedrawDialogs(1);
        }
    }
    if (item == kBarOfferItem)
        OfferFromDialog(kBarVenue, barDialog);
    if (item == kBarMapItem) {
        OpenMapKeepingNav();
        shipData->targetStellar = savedStellar;
        shipData->navTargetKind = savedRoute;
        RedrawAfterSubDialog(barDialog);
    }
    if (item == kBarPlayerInfoItem)
        PlayerInfoFromDialog(barDialog);
    if (item == kBarMissionInfoItem)
        MissionInfoFromDialog(barDialog);
}

constexpr KeyItem kBarKeys[] = {
    { 'g', kBarRaceItem }, { 'r', kBarRaceItem },
    { 'w', kBarNewsItem }, { 'n', kBarNewsItem },
    { 'h', kBarHireItem }, { 'e', kBarHireItem },
};
constexpr short kBarSlotItems[] = { kBarLeaveItem, kBarRaceItem, kBarNewsItem, -1, kBarHireItem, -1 };

// FUNCTION: EVNOVA 0x0047cdb0
// MAC: _BarFilter i386 0x00047a4b ppc 0x000503e8
Boolean BarDialog::Filter(DialogPtr unused, const DialogEvent &ev, short *itemHit)
{
    Point where;
    EventChar c;
    EventChar key;

    if (LeaveRequested(kBarLeaveItem, itemHit))
        return 1;
    if (ev.what == DialogInput::Key) {
        if (QuickKeyHit(kBarMapItem, kBarPlayerInfoItem, kBarMissionInfoItem, itemHit))
            return 1;
        c = ev.ch;
        if (IsReturnChar(c) || c == kEscapeChar) {
            *itemHit = kBarLeaveItem;
            return 1;
        }
        key = tolower(c);
        if (KeyItemHit(key, kBarKeys, itemHit))
            return 1;
    }
    if (ev.what == DialogInput::Click) {
        where = ev.where;
        BarDialogUpdate();
        return ButtonItemHit(BarButtons().Track(where), kBarSlotItems, itemHit);
    }
    return OfferDue(kBarOfferItem, itemHit);
}

} // namespace

void DoBarDialog(void)
{
    BarDialog().Run();
}

// FUNCTION: EVNOVA 0x0047cfe0
// MAC: _BarDialogUpdate i386 0x000477e8 ppc 0x0005020c
void BarDialogUpdate(void)
{
    Rect box;
    short itemType;
    Handle item;

    UI::Painter p(UI::Surface::Load());

    p.Fill(*GetDialogPortRect(barDialog), UI::kBlack);
    if (barBackgroundPic != NULL)
        p.Picture(barBackgroundPic, *GetDialogPortRect(barDialog));
    if (barPic != NULL) {
        GetDialogItem(barDialog, kBarPortraitItem, &itemType, &item, &box);
        p.Picture(barPic, box);
    }
    GetDialogItem(barDialog, kBarDescItem, &itemType, &item, &box);
    p.TextBox(box, barDescText, strlen(barDescText), { UI::Style::Small, UI::kWhite }, UI::kBlack);
    BarButtons().Draw(p, -1);
    LoadPageToDialog(barDialog);
}

#define kSponsorStrList      0x1fa4  /* STR# 8100, the sponsor lines */
#define kFillerNewsStrList   0x1fa5  /* STR# 8101, used when nothing qualifies */
#define kStrSponsor          0xbe
#define kStrNoNews           0xbf
#define kStrDisasterVerb     0xc0
#define kStrDisasterOn       0xc1
#define kStrDisasterIn       0xc2
#define kStrDisasterAffects  0xb5
#define kStrDisasterAt       0x3c

#define kDlg1014ID           0x3f6
#define kRaceDialogID        0x3ff
#define kDefaultBackdropPict 9000
#define kRaceBackdropPict    0x2151
#define kRaceRunnerPict      0x2152  /* +i per racer, +10 per animation frame */
#define kRacerCount          4
#define kLastRaceFrame       3
#define kNewsDoneItem        1
#define kRaceDoneItem        1
#define kRaceHelpItem        2
#define kRaceFirstButton     3       /* itemHit 3..6 pick a racer */
#define kRaceHelpDesc        0x7ffc
#define kRaceResultDesc      0x7ff8  /* + the winning racer */
#define kRaceWinStrIndex     370
#define kRacePayoutOdds      4
#define kMaxRaceBet          10000
#define kQuickRaceBet        1000
#define kTitleTop     0x8c
#define kTitleBottom  0xb4
#define kBodyTop      0xaa
#define kTextInset    10
#define kBodyMargin   4

namespace {

// FUNCTION: EVNOVA 0x0047d180
// MAC: _DoNewsDialog i386 0x00047f0a ppc 0x000508b8
/* The news, closed by Return, Escape or a click on it. */
class NewsDialog final : public GameDialog {
public:
    void Run();

private:
    void Hit(short item) override;
    static Boolean Filter(DialogPtr unused, const DialogEvent &ev, short *itemHit);
};

void NewsDialog::Run()
{
    short pictID;
    Picture backdrop(newsPic);

    PlaySound(beepSnd[1], 1, gSoundVolume, gSoundVolume);
    pictID = kDefaultBackdropPict;
    if (dockedPort->govt != -1) {
        if (govtData[dockedPort->govt].newsPict != -1)
            pictID = govtData[dockedPort->govt].newsPict;
    }
    if (backdrop.Load(pictID) == NULL && pictID != kDefaultBackdropPict)
        backdrop.Load(kDefaultBackdropPict);
    dialog.Open(kDlg1014ID, NewsDialogUpdate, Filter, &newsDialog);
    if (!dialog.IsOpen())
        return;
    FlushEvents(everyEvent - diskMask, 0);
    NewsDialogUpdate();
    Loop();
    dialog.Close();
    backdrop.Release();
    FlushEvents(1, 0); /* BUG: event kinds passed as masks: the Mac flushes no key events */
    FlushEvents(2, 0);
    FlushEvents(3, 0);
    FlushEvents(4, 0);
    FlushEvents(5, 0);
    PlaySound(beepSnd[2], 1, gSoundVolume, gSoundVolume);
}

void NewsDialog::Hit(short item)
{
    if (item == kNewsDoneItem)
        done = 1;
}

// FUNCTION: EVNOVA 0x0047d560
// MAC: _NewsDialogFilter i386 0x00048160 ppc 0x00050acc
Boolean NewsDialog::Filter(DialogPtr unused, const DialogEvent &ev, short *itemHit)
{
    Point where;

    *itemHit = -1;
    if (ev.what == DialogInput::Key &&
        (IsReturnChar(ev.ch) ||
         ev.ch == kEscapeChar)) {
        *itemHit = kNewsDoneItem;
        return 1;
    }
    if (ev.what == DialogInput::Click) {
        where = ev.where;
        if (PtInRectPoint(where, GetDialogPortRect(newsDialog)) != 0) {
            *itemHit = kNewsDoneItem;
            return 1;
        }
    }
    return 0;
}

} // namespace

void DoNewsDialog(void)
{
    NewsDialog().Run();
}

// FUNCTION: EVNOVA 0x0047d370
// MAC: _NewsDialogUpdate i386 0x00047c57 ppc 0x00050688
void NewsDialogUpdate(void)
{
    Rect box;

    UI::Painter p(UI::Surface::Load());

    p.Fill(*GetDialogPortRect(newsDialog), UI::kBlack);
    p.Picture(newsPic, *GetDialogPortRect(newsDialog));
    SetRect(&box, GetDialogPortRect(newsDialog)->left + kTextInset,
            GetDialogPortRect(newsDialog)->top + kTitleTop,
            GetDialogPortRect(newsDialog)->right - kTextInset,
            GetDialogPortRect(newsDialog)->top + kTitleBottom);
    p.TextBox(box, sponsorString, { UI::Style::Small, UI::kWhite }, UI::kBlack);
    SetRect(&box, GetDialogPortRect(newsDialog)->left + kTextInset,
            GetDialogPortRect(newsDialog)->top + kBodyTop,
            GetDialogPortRect(newsDialog)->right - kTextInset,
            GetDialogPortRect(newsDialog)->bottom - kBodyMargin);
    p.TextBox(box, newsString, { UI::Style::Small, UI::kWhite }, UI::kBlack);
    LoadPageToDialog(newsDialog);
}

// FUNCTION: EVNOVA 0x0047d600
// MAC: _RandomizeNewsDialog i386 0x00048238 ppc 0x00050bd0
void RandomizeNewsDialog(void)
{
    Boolean cronActive[kMaxCrons];
    Boolean anyGovtNews;
    Boolean anyActiveCron;
    short i;
    short cronNewsID[kMaxCrons];
    Str255 str;
    short eligible;
    short matching;
    short j;
    short pick;

    GetRandomIndString(str, kSponsorStrList);
    if (str[0] != 0)
        sponsorString = FromNameField(str);
    else
        sponsorString = IndString(kGameStrList, kStrSponsor);

    newsString.clear();
    i = 0;
    eligible = 0;
    matching = 0;
    for (i = 0; i < kMaxDisasters; i++) {
        if (disasterData[i].loaded != 0 && disasterData[i].countdown > 1) {
            eligible++;
            if (disasterData[i].stellarID == shipData[0].targetStellar ||
                disasterData[i].stellar == -2)
                matching++;
        }
    }
    if (eligible > 0) {
        if (matching > 0) {
            do {
                pick = Rand(kMaxDisasters);
            } while (disasterData[pick].loaded == 0 || disasterData[pick].countdown <= 1 ||
                     (disasterData[pick].stellarID != shipData[0].targetStellar &&
                      disasterData[pick].stellar != -2));
        } else {
            do {
                pick = Rand(kMaxDisasters);
            } while (disasterData[pick].loaded == 0 || disasterData[pick].countdown <= 1);
        }
        if (disasterData[pick].stellarID != -1 && disasterData[pick].name[0] != 0) {
            newsString = TextF("%s %s %s %s %s %s %s %s.", IndString(kGameStrList, kStrSponsor).c_str(),
                               FromNameField(disasterData[pick].name).c_str(),
                               IndString(kGameStrList, kStrDisasterVerb).c_str(),
                               IndString(kGameStrList, disasterData[pick].amount > 0 ? kStrDisasterOn
                                                                                     : kStrDisasterIn).c_str(),
                               IndString(kGameStrList, kStrDisasterAffects).c_str(),
                               UpToNul(TableEntry(lcCargoName, disasterData[pick].commodity)).c_str(),
                               IndString(kGameStrList, kStrDisasterAt).c_str(),
                               SpobName(disasterData[pick].stellarID).c_str());
        }
    }

    anyGovtNews = 0;
    anyActiveCron = 0;
    i = 0;
    for (i = 0; i < kMaxCrons; i++) {
        cronNewsID[i] = -2;
        cronActive[i] = 0;
        if (cronEvent[i].active != 0 && cronEvent[i].loaded != 0 &&
            cronEvent[i].holdoffLeft <= 0) {
            for (j = 0; j < 4; j++) {
                if (cronEvent[i].govts[j] != -1 &&
                    GovtAllies(cronEvent[i].govts[j],
                               stellarData[shipData[0].targetStellar].govt)
                        != 0) {
                    cronNewsID[i] = cronEvent[i].govtValues[j];
                    if (cronNewsID[i] > 0)
                        anyGovtNews = 1;
                }
            }
            if (cronNewsID[i] < -1 && cronEvent[i].indNewsStr > 0) {
                cronActive[i] = 1;
                anyActiveCron = 1;
            }
        }
    }
    if (anyGovtNews != 0 || anyActiveCron != 0) {
        if (anyGovtNews != 0) {
            do {
                pick = Rand(kMaxCrons);
            } while (cronNewsID[pick] <= 0);
            GetRandomIndString(str, cronNewsID[pick]);
        } else {
            do {
                pick = Rand(kMaxCrons);
            } while (cronActive[pick] == 0);
            GetRandomIndString(str, cronEvent[pick].indNewsStr);
        }
        newsString = FromNameField(str);
    }
    if (newsString.empty()) {
        GetRandomIndString(str, kFillerNewsStrList);
        if (str[0] != 0)
            newsString = FromNameField(str);
        else
            newsString = IndString(kGameStrList, kStrNoNews);
    }
}

namespace {

// FUNCTION: EVNOVA 0x0047dc50
// MAC: _DoRaceDialog i386 0x000489ca ppc 0x00051360
/* Betting on the races: a pick bets a fixed stake, with Alt held it asks for one. */
class RaceDialog final : public GameDialog {
public:
    void Run();

private:
    void Hit(short item) override;
    static Boolean Filter(DialogPtr unused, const DialogEvent &ev, short *itemHit);
};

void RaceDialog::Run()
{
    short i;
    short j;
    Picture racers[kRacerCount][kLastRaceFrame + 1];

    dialog.Open(kRaceDialogID, RaceDialogUpdate, Filter, &raceDialog);
    if (!dialog.IsOpen())
        return;
    raceWinner = -1;
    racePick = -1;
    raceCount = 0;
    Picture backdrop(raceBackgroundPic);
    backdrop.Load(kRaceBackdropPict);
    for (i = 0; i < kRacerCount; i++) {
        for (j = 0; j <= kLastRaceFrame; j++) {
            racers[i][j].Bind(raceButtonPic[i][j]);
            racers[i][j].Load(kRaceRunnerPict + 10 * j + i);
        }
    }
    FlushEvents(everyEvent - diskMask, 0);
    RedrawDialog(raceDialog);
    RaceDialogUpdate();
    Loop();
    dialog.Close();
    backdrop.Release();
    for (i = 0; i < kRacerCount; i++)
        for (j = 0; j <= kLastRaceFrame; j++)
            racers[i][j].Release();
}

void RaceDialog::Hit(short item)
{
    int32_t payout;
    int32_t credits;
    std::string winText;
    size_t len;

    if (item == kRaceDoneItem)
        done = 1;
    if (item == kRaceHelpItem) {
        LoadDescription(textShowString, kRaceHelpDesc);
        ShowTextDialog(0, 0);
        RaceDialogUpdate();
    }
    if (item >= kRaceFirstButton &&
        item < kRaceFirstButton + kRacerCount) {
        if (KeyCheck(kScanLAlt) != 0 || KeyCheck(kScanRAlt) != 0)
        {
            if (shipData[0].credits > kMaxRaceBet)
                credits = kMaxRaceBet;
            else
                credits = shipData[0].credits;
            raceBetAmount = DoQuantityDialog(credits, 2);
        }
        else if (shipData[0].credits >= kQuickRaceBet)
            raceBetAmount = kQuickRaceBet;
        else
            raceBetAmount = shipData[0].credits;
        if (raceWinner == -1 && raceBetAmount > 0) {
            racePick = item - kRaceFirstButton;
            shipData[0].credits -= raceBetAmount;
            statusRedraw[kRedrawCargo] = 1;
            DrawStatus();
            raceWinner = Rand(kRacerCount);
            if (raceWinner == lastRaceWinner) {
                do {
                    raceWinner = Rand(kRacerCount);
                } while (raceWinner == lastRaceWinner ||
                         raceWinner == racePick);
            }
            lastRaceWinner = raceWinner;
            raceCount++;
            RedrawDialog(raceDialog);
            LoadDescription(textShowString,
                            (raceWinner + kRaceResultDesc));
            if (raceWinner == racePick) {
                payout = raceBetAmount * kRacePayoutOdds;
                winText = TextF("\r\r%s:  %s %s.", IndString(kGameStrList, kRaceWinStrIndex).c_str(),
                                CommaNumber(payout).c_str(),
                                FromField(payout == 1 ? creditCString : creditsCString, sizeof creditsCString).c_str());
                /* appended up to the buffer's end */
                len = strlen(textShowString);
                CopyCField(textShowString + len, sizeof textShowString - len, winText.c_str(), winText.size());
            }
            ShowTextDialog(0, 1);
            if (raceWinner == racePick) {
                shipData[0].credits += payout;
                statusRedraw[kRedrawCargo] = 1;
                DrawStatus();
            }
        } else {
            raceWinner = -1;
            racePick = -1;
        }
        RedrawDialog(raceDialog);
        RaceDialogUpdate();
    }
}

// FUNCTION: EVNOVA 0x0047e150
// MAC: _RaceFilter i386 0x0004890e ppc 0x00051284
Boolean RaceDialog::Filter(DialogPtr unused, const DialogEvent &ev, short *itemHit)
{
    Point where;

    if (ev.what == DialogInput::Key &&
        (IsReturnChar(ev.ch) ||
         ev.ch == kEscapeChar)) {
        *itemHit = kRaceDoneItem;
        return 1;
    }
    if (ev.what == DialogInput::Click) {
        where = ev.where;
        RaceDialogUpdate();
        return ButtonItemHit(RaceButtons().Track(where), itemHit);
    }
    return 0;
}

} // namespace

void DoRaceDialog(void)
{
    RaceDialog().Run();
}

// FUNCTION: EVNOVA 0x0047e1e0
// MAC: _RaceDialogUpdate i386 0x0004879d ppc 0x0005115c
void RaceDialogUpdate(void)
{
    UI::Painter p(UI::Surface::Load());

    p.Fill(*GetDialogPortRect(raceDialog), UI::kBlack);
    if (raceBackgroundPic != NULL)
        p.Picture(raceBackgroundPic, *GetDialogPortRect(raceDialog));
    RaceButtons().Draw(p, -1);
    LoadPageToDialog(raceDialog);
}
