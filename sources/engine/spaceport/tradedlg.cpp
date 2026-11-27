/* tradedlg.cpp -- the trade center dialog */

#include "game.h"
#include "stellar.h"
#include "prototypes.h"
#include "gameutil.h"
#include "missions.h"
#include "player.h"
#include "render.h"
#include "ships.h"
#include "gamedialog.h"
#include "buttons.h"
#include "text.h"
#include "painter.h"

#define kFirstRowItem  4
#define kLastRowItem   11
#define kLastRow       7
#define kBuyItem       13
#define kSellItem      14

DialogPtr tradeDialog;
short junkType[2];
PicHandle tradeBackgroundPic;

#define kTradeDlogID   0x3e9
#define kTradePictID   0x213e
#define kTradeVenue    4

#define kHoldsItem     3
#define kCargoItem     0xc
#define kNewsItem      0xf
#define kOfferItem     0xf     /* a mission offer falls due */

/* port price-status codes read off portDemand[]; the STR# 2002 index each
   selects (0xc9/0xca/0xcb) is read straight from the exe, name TBC */
#define kPortShortage 1
#define kPortSurplus  2
#define kPortEmbargo  4

#define kTradeRows    8         /* the six commodities and the two junk rows */

namespace {

// FUNCTION: EVNOVA 0x0048c730
// MAC: _DoTradeDialog i386 0x0005dbe1 ppc 0x000657f4
/* The trade center: buying and selling commodities and junk at the port's prices. */
class TradeDialog final : public GameDialog {
public:
    void Run();

private:
    void Hit(short item) override;
    static Boolean Filter(DialogPtr unused, const DialogEvent &ev, short *itemHit);

    short savedStellar;     /* the nav target the map may change, restored after it */
    short savedRoute;
};

void TradeDialog::Run()
{
    short i;
    double mult;

    savedStellar = shipData->targetStellar;
    savedRoute = shipData->navTargetKind;
    mult = 1.25;
    if (dockedPort->govt != -1
        && playerRecord[dockedPort->system] < 0) {
        mult = 1.1;
    }
    if (dockedPort->dominated != 0) {
        mult = 1.5;
    }
    for (i = 0; i <= kLastCommodity; i++) {
        portDemand[i] = CalcPortDemand(i, dockedPort->flags);
        tradePrice[i] = 0;
        if (portDemand[i] > 0) {
            if (portDemand[i] == kPortShortage)
                tradePrice[i] = basePrice[i] / mult;
            else if (portDemand[i] == kPortSurplus)
                tradePrice[i] = basePrice[i];
            else if (portDemand[i] == kPortEmbargo)
                tradePrice[i] = basePrice[i] * mult;
            if (tradePrice[i] < 5)
                tradePrice[i] = 5;
        }
    }
    for (i = 0; i < kMaxDisasters; i++) {
        if (disasterData[i].loaded != 0 && disasterData[i].countdown > 0
            && disasterData[i].stellarID == shipData->targetStellar) {
            tradePrice[disasterData[i].commodity] =
                basePrice[disasterData[i].commodity];
            tradePrice[disasterData[i].commodity] += disasterData[i].amount;
            if (tradePrice[disasterData[i].commodity] < 5)
                tradePrice[disasterData[i].commodity] = 5;
        }
    }
    tradePrice[6] = tradePrice[7] = 0;
    junkType[0] = PortJunk(shipData->targetStellar, true);
    if (junkType[0] >= 0)
        tradePrice[6] = junkData[junkType[0]].basePrice * mult;
    junkType[1] = PortJunk(shipData->targetStellar, false);
    if (junkType[1] >= 0)
        tradePrice[7] = junkData[junkType[1]].basePrice / mult;
    bool anyPrice = false;
    for (i = 0; i < kTradeRows; i++)
        anyPrice = anyPrice || tradePrice[i] != 0;
    if (!anyPrice)
        return;
    missionsOutside = kTradeVenue;
    offeringFromShip = 0;
    missionOfferTime = ReadTick() + Rand(0x1e) + 0x1e;
    dialog.Open(kTradeDlogID, TradeDialogUpdate, Filter, &tradeDialog);
    if (!dialog.IsOpen())
        return;
    Picture backdrop(tradeBackgroundPic);
    backdrop.Load(kTradePictID);
    selTradeItem = 7;
    TradeDialogTabKey(0);
    TradeDialogUpdate();
    OfferMissionFromPort(kTradeVenue);
    RedrawDialog(tradeDialog);
    Loop();
    dialog.Close();
    backdrop.Release();
    shipData->targetStellar = savedStellar;
    shipData->navTargetKind = savedRoute;
    statusRedraw[kRedrawNav] = 1;
    ResetMissionOfferVenue();
}

void TradeDialog::Hit(short item)
{
    short mods;
    short space;
    short maxSell;
    int32_t afford;
    int32_t qty;

    mods = GetKeyModifiers();
    if (item == 1)
        done = 1;
    if (item == 2) {
        OpenMapKeepingNav();
        shipData->targetStellar = savedStellar;
        shipData->navTargetKind = savedRoute;
        RedrawAfterSubDialog(tradeDialog);
    }
    if (item == 0x10)
        PlayerInfoFromDialog(tradeDialog);
    if (item == 0x11)
        MissionInfoFromDialog(tradeDialog);
    if (item == kBuyItem) {
        RedrawDialog(tradeDialog);
        space = TotalFleetHolds() - TotalCargo(shipData);
        if (space > 0) {
            afford = shipData->credits / (float)tradePrice[selTradeItem];
            if ((mods & optionKey) != 0) {
                if (afford > space)
                    qty = space;
                else
                    qty = afford;
                if (qty > kMaxTradeQty)
                    qty = kMaxTradeQty;
                qty = DoQuantityDialog(qty, 0);
                TradeDialogUpdate();
            } else {
                qty = 10;
                if (afford < 10)
                    qty = afford;
                if (qty > space)
                    qty = space;
            }
            if (qty > 0) {
                if (selTradeItem <= kLastCommodity)
                    shipData->cargo[selTradeItem] += qty;
                else if (junkType[selTradeItem - kFirstJunkItem] != -1)
                    junkData[junkType[selTradeItem - kFirstJunkItem]]
                        .held += qty;
                shipData->credits -= tradePrice[selTradeItem] * qty;
            }
            RedrawDialog(tradeDialog);
            statusRedraw[kRedrawCargo] = 1;
            DrawStatus();
        }
        RedrawDialog(tradeDialog);
    }
    if (item == kSellItem) {
        RedrawDialog(tradeDialog);
        qty = 0;
        if (selTradeItem <= kLastCommodity)
            qty = shipData->cargo[selTradeItem];
        else if (junkType[selTradeItem - kFirstJunkItem] != -1)
            qty = junkData[junkType[selTradeItem - kFirstJunkItem]].held;
        if (qty > kMaxTradeQty)
            maxSell = kMaxTradeQty;
        else
            maxSell = qty;
        qty = 0;
        if (maxSell > 0) {
            if ((mods & optionKey) != 0) {
                qty = DoQuantityDialog(maxSell, 1);
                TradeDialogUpdate();
            } else {
                qty = 10;
                if (selTradeItem <= kLastCommodity) {
                    if (shipData->cargo[selTradeItem] < qty)
                        qty = shipData->cargo[selTradeItem];
                } else if (junkType[selTradeItem - kFirstJunkItem] != -1) {
                    if (junkData[junkType[selTradeItem - kFirstJunkItem]]
                            .held < qty)
                        qty = junkData[junkType[selTradeItem
                                                - kFirstJunkItem]].held;
                }
            }
            if (qty > 0) {
                if (selTradeItem <= kLastCommodity)
                    shipData->cargo[selTradeItem] -= qty;
                else if (junkType[selTradeItem - kFirstJunkItem] != -1)
                    junkData[junkType[selTradeItem - kFirstJunkItem]]
                        .held -= qty;
                shipData->credits += tradePrice[selTradeItem] * qty;
            }
            RedrawDialog(tradeDialog);
            statusRedraw[kRedrawCargo] = 1;
            DrawStatus();
        }
        RedrawDialog(tradeDialog);
    }
    if (item == kOfferItem)
        OfferFromDialog(kTradeVenue, tradeDialog);
}

constexpr KeyItem kTradeKeys[] = { { 'b', kBuyItem }, { 's', kSellItem } };
constexpr short kTradeSlotItems[] = { 1, kBuyItem, kSellItem };

// FUNCTION: EVNOVA 0x0048d190
// MAC: _TradeFilter i386 0x0004e092 ppc 0x00056d08
Boolean TradeDialog::Filter(DialogPtr unused, const DialogEvent &ev, short *itemHit)
{
    Rect box;
    short itemType;
    Point where;
    Point mouse;
    Handle item;
    short j;
    short last;
    short i;
    EventChar c;

    if (LeaveRequested(1, itemHit))
        return 1;
    if (ev.what == DialogInput::Key) {
        if (QuickKeyHit(2, 16, 17, itemHit))
            return 1;
        c = ev.ch;
        if (IsReturnChar(c) || c == kEscapeChar) {
            *itemHit = 1;
            return 1;
        }
        if (c == kTabChar) {
            if ((ev.mods & shiftKey) != 0)
                TradeDialogTabKey(1);
            else
                TradeDialogTabKey(0);
        }
        if (c == kDownArrowChar)
            TradeDialogTabKey(0);
        if (c == kUpArrowChar)
            TradeDialogTabKey(1);
        c = tolower(c);
        if (KeyItemHit(c, kTradeKeys, itemHit))
            return 1;
    }
    if (ev.what == DialogInput::Click) {
        where = ev.where;
        for (i = kFirstRowItem; i <= kLastRowItem; i++) {
            GetDialogItem(tradeDialog, i, &itemType, &item, &box);
            if (PtInRectPoint(where, &box) &&
                tradePrice[i - kFirstRowItem] != 0) {
                InvalDialogItem(tradeDialog, selTradeItem + kFirstRowItem);
                InvalDialogItem(tradeDialog, kBuyItem);
                InvalDialogItem(tradeDialog, kSellItem);
                last = selTradeItem;
                selTradeItem = i - kFirstRowItem;
                while (StillDown()) {
                    if (selTradeItem != last) {
                        InvalDialogItem(tradeDialog,
                                     selTradeItem + kFirstRowItem);
                        InvalDialogItem(tradeDialog, last + kFirstRowItem);
                        TradeDialogUpdate();
                    }
                    GetMouse(&mouse);
                    for (j = 0; j <= kLastRow; j++) {
                        GetDialogItem(tradeDialog, j + kFirstRowItem, &itemType,
                                      &item, &box);
                        if (PtInRectPoint(mouse, &box) &&
                            tradePrice[j] != 0) {
                            last = selTradeItem;
                            selTradeItem = j;
                            break;
                        }
                    }
                }
                InvalDialogItem(tradeDialog, selTradeItem + kFirstRowItem);
                return 0;
            }
        }
        TradeDialogUpdate();
        return ButtonItemHit(TradeButtons().Track(where), kTradeSlotItems, itemHit);
    }
    return OfferDue(kOfferItem, itemHit);
}

} // namespace

void DoTradeDialog(void)
{
    TradeDialog().Run();
}



// FUNCTION: EVNOVA 0x0048d5a0
// MAC: _TradeDialogTabKey i386 0x0004cd81 ppc 0x00055cc0
void TradeDialogTabKey(Boolean back)
{
    Rect box;
    Rect bounds;
    short itemType;
    void *item;

    if (back)
        selTradeItem -= 1;
    else
        selTradeItem += 1;
    if (selTradeItem > kTradeRows - 1)
        selTradeItem = 0;
    if (selTradeItem < 0)
        selTradeItem = kTradeRows - 1;
    while (tradePrice[selTradeItem] == 0) {
        if (back)
            selTradeItem -= 1;
        else
            selTradeItem += 1;
        if (selTradeItem > kTradeRows - 1)
            selTradeItem = 0;
        if (selTradeItem < 0)
            selTradeItem = kTradeRows - 1;
    }
    InvalDialogItem(tradeDialog, 13);
    InvalDialogItem(tradeDialog, 14);
    GetDialogItem(tradeDialog, 3, &itemType, (Handle *)&item, &box);
    bounds = box;
    GetDialogItem(tradeDialog, 12, &itemType, (Handle *)&item, &box);
    bounds.bottom = box.bottom;
    InvalDialogRect(tradeDialog, &bounds);
}

// FUNCTION: EVNOVA 0x0048d6f0
// MAC: _TradeDialogUpdate i386 0x0004ceea ppc 0x00055e5c
void TradeDialogUpdate(void)
{
    void *item;
    short itemType;
    short missionTons;
    short junkTons;
    short missionSlots;
    Boolean seen[kMaxCargo];
    short junkSlots;
    short k;
    Rect insetBox;
    Rect box;
    std::string text;
    std::string state;
    RGBColor stateColour;
    Boolean found;
    short i;
    short j;
    short qty;
    short p;
    short fleetHolds;
    short holds;
    short totalCargo;
    short available;
    short over;

    UI::Painter load(UI::Surface::Load());
    const UI::TextSpec heading = { UI::Style::Small, rgbLightGrey };
    const UI::TextSpec plain = { UI::Style::Small, ldefColors[0] };

    load.Fill(*GetDialogPortRect(tradeDialog), UI::kBlack);
    if (tradeBackgroundPic != NULL)
        load.Picture(tradeBackgroundPic, *GetDialogPortRect(tradeDialog));

    GetDialogItem(tradeDialog, kHoldsItem, &itemType, (Handle *)&item, &box);
    load.Frame(box, rgbDarkGrey);
    load.Text(box.left + 7, box.top + 12, IndStringRaw(kGameStrList, 0xc5), heading);
    load.Text(box.right - 0x7b, box.top + 12,
              IndStringRaw(kGameStrList, TotalFleetHolds() > ShipTotalHolds() ? 0xc6 : 0xc7), heading);
    load.Text(box.right - 0x46, box.top + 12, IndStringRaw(kGameStrList, 0xc8), heading);

    for (i = 0; i <= kLastRow; i++) {
        GetDialogItem(tradeDialog, i + kFirstRowItem, &itemType, (Handle *)&item, &box);
        insetBox = box;
        InsetRect(&insetBox, 1, 1);
        load.Fill(insetBox, i == selTradeItem ? ldefColors[2] : ldefColors[1]);

        load.Line(box.left, box.top, box.left, (box.bottom - 1), rgbDarkGrey);
        load.Line((box.right - 1), box.top, (box.right - 1), (box.bottom - 1), rgbDarkGrey);
        if (i == 0)
            load.Line(box.left, box.top, (box.right - 1), box.top, rgbDarkGrey);
        if (i == kLastRow)
            load.Line(box.left, (box.bottom - 1), (box.right - 1), (box.bottom - 1), rgbDarkGrey);

        if (tradePrice[i] != 0) {
            if (i <= kLastCommodity) {
                load.Text(box.left + 7, box.top + 10, cargoName[i], plain);
                qty = shipData->cargo[i];
                if (qty > 0) {
                    text = NumText(qty);
                    load.Text(box.right - (load.Measure(text, UI::Style::Small) + 0x6e), box.top + 10, text, plain);
                }
            } else if (junkType[i - kFirstJunkItem] != -1) {
                load.Text(box.left + 6, box.top + 10, FromPascal(junkData[junkType[i - kFirstJunkItem]].name), plain);
                qty = junkData[junkType[i - kFirstJunkItem]].held;
                if (qty > 0) {
                    text = NumText(qty);
                    load.Text(box.right - (load.Measure(text, UI::Style::Small) + 0x6e), box.top + 10, text, plain);
                }
            }

            state.clear();
            stateColour = ldefColors[0];
            found = 0;
            for (k = 0; k < kMaxDisasters; k++) {
                if (disasterData[k].loaded != 0 && disasterData[k].countdown > 0 &&
                    disasterData[k].stellarID == shipData->targetStellar &&
                    i == disasterData[k].commodity) {
                    found = 1;
                    if (disasterData[k].amount > 0) {
                        stateColour = betterRGB;
                        state = IndStringRaw(kGameStrList, 0xcc);
                    }
                    if (disasterData[k].amount < 0) {
                        stateColour = worseRGB;
                        state = IndStringRaw(kGameStrList, 0xcd);
                    }
                    break;
                }
            }
            if (!found) {
                if (i <= kLastCommodity) {
                    if (portDemand[i] == kPortShortage)
                        state += IndStringRaw(kGameStrList, 0xc9);
                    if (portDemand[i] == kPortSurplus)
                        state += IndStringRaw(kGameStrList, 0xca);
                    if (portDemand[i] == kPortEmbargo)
                        state += IndStringRaw(kGameStrList, 0xcb);
                } else if (junkType[i - kFirstJunkItem] != -1) {
                    if (i == kFirstJunkItem)
                        state += IndStringRaw(kGameStrList, 0xcb);
                    if (i == kLastRow)
                        state += IndStringRaw(kGameStrList, 0xc9);
                }
            }
            load.Text(box.right - 0x46, box.top + 10, state, { UI::Style::Small, stateColour });

            text = NumText(tradePrice[i]);
            load.Text(box.right - (load.Measure(text, UI::Style::Small) + 10), box.top + 10, text, plain);
        }
    }

    GetDialogItem(tradeDialog, kCargoItem, &itemType, (Handle *)&item, &box);
    insetBox = box;
    InsetRect(&insetBox, 5, 3);
    text.clear();

    available = 0;
    k = 0;
    missionTons = 0;
    junkTons = 0;
    missionSlots = 0;
    junkSlots = 0;

    for (i = 0; i < kMaxCargo; i++)
        seen[i] = 0;
    for (j = 0; j < kMaxActiveMissions; j++) {
        if (currentMissionObjectives[j].active && currentMissions[j].hasCargo &&
            currentMissions[j].cargoType != -1 && currentMissions[j].cargoQty >= 0) {
            available++;
            missionSlots++;
            missionTons += currentMissions[j].cargoQty;
            if (seen[currentMissions[j].cargoType] == 0) {
                seen[currentMissions[j].cargoType] = 1;
                k++;
            }
        }
    }
    for (j = 0; j < kMaxJunks; j++) {
        if (junkData[j].held > 0 && j != junkType[0] && j != junkType[1]) {
            available++;
            junkTons += junkData[j].held;
            junkSlots++;
            k++;
        }
    }
    if (available > 0) {
        if (missionSlots + junkSlots > 0) {
            text += IndString(kGameStrList, 0x16b);
            text += " ";
            if (missionTons + junkTons > 0)
                text += TextF("%d %s %s ", missionTons + junkTons,
                              UpToNul(missionTons + junkTons == 1 ? tonString : tonsString).c_str(),
                              IndString(kGameStrList, 0x187).c_str());
            if (missionSlots > 0) {
                text += IndString(kGameStrList, 0x16f);
                if (junkSlots > 0) {
                    text += " ";
                    text += IndString(kGameStrList, 0x188);
                    text += " ";
                }
            }
            if (junkSlots > 0)
                text += IndString(kGameStrList, 0x170);
            text += "\r\r";
        }
    }

    holds = ShipTotalHolds();
    fleetHolds = TotalFleetHolds();
    totalCargo = TotalCargo(shipData);
    text += IndString(kGameStrList, 0x16c);
    if (fleetHolds > holds) {
        text += " ";
        text += IndString(kGameStrList, 0x16d);
    }
    text += ": ";
    if (fleetHolds > holds) {
        over = totalCargo - missionTons;
        over -= fleetHolds - holds;
        if (over < 0)
            over = 0;
        available = holds - (over + missionTons);
        if (available < 0)
            available = 0;
    } else {
        available = holds - totalCargo;
        if (available < 0)
            available = 0;
    }
    text += TextF("%d %s", available, UpToNul(available == 1 ? tonString : tonsString).c_str());
    if (fleetHolds > holds) {
        available = fleetHolds - totalCargo;
        if (available < 0)
            available = 0;
        text += TextF("\r%s %s: %d %s", IndString(kGameStrList, 0x16c).c_str(),
                      IndString(kGameStrList, 0x16e).c_str(), available,
                      UpToNul(available == 1 ? tonString : tonsString).c_str());
    }

    load.TextBox(insetBox, text, { UI::Style::Small, UI::kWhite }, UI::kBlack);
    load.Frame(box, rgbDarkGrey);
    TradeButtons().Draw(load, -1);

    GetDialogItem(tradeDialog, kNewsItem, &itemType, (Handle *)&item, &box);
    for (p = 0; p < kMaxDisasters; p++) {
        if (disasterData[p].loaded != 0 && disasterData[p].countdown > 0 &&
            disasterData[p].stellarID == shipData->targetStellar) {
            if (cheatsActive)
                text = "Disaster ID " + NumText(p + 0x80);
            else
                text = FromNameField(disasterData[p].name, 0x3f);
            text += TextF(" %s %s %s %s.", IndString(kGameStrList, 0xc0).c_str(),
                          IndString(kGameStrList, disasterData[p].amount > 0 ? 0xc1 : 0xc2).c_str(),
                          IndString(kGameStrList, 0xb5).c_str(),
                          UpToNul(TableEntry(lcCargoName, disasterData[p].commodity), 0x3f).c_str());
            load.TextBox(box, text, { UI::Style::Small, UI::kWhite }, UI::kBlack);
            break;
        }
    }

    LoadPageToDialog(tradeDialog);
}
