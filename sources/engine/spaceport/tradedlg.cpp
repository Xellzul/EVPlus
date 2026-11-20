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
    short j;

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
    junkType[0] = -1;
    junkType[1] = -1;
    tradePrice[6] = tradePrice[7] = 0;
    for (i = 0; i < kMaxJunks; i++) {
        for (j = 0; j < 8; j++) {
            if (junkData[i].boughtAt[j] == shipData->targetStellar) {
                if (EvalMissionBitTestString(junkData[i].buyOn) != 0) {
                    junkType[0] = i;
                    tradePrice[6] = junkData[i].basePrice * mult;
                    break;
                }
            }
        }
    }
    for (i = 0; i < kMaxJunks; i++) {
        for (j = 0; j < 8; j++) {
            if (junkData[i].soldAt[j] == shipData->targetStellar) {
                if (EvalMissionBitTestString(junkData[i].sellOn) != 0) {
                    junkType[1] = i;
                    tradePrice[7] = junkData[i].basePrice / mult;
                    break;
                }
            }
        }
    }
    missionsOutside = kTradeVenue;
    offeringFromShip = 0;
    missionOfferTime = ReadTick() + Rand(0x1e) + 0x1e;
    dialog.Open(kTradeDlogID, TradeDialogUpdate, Filter, &tradeDialog, DialogClose::NoFlash);
    if (!dialog.IsOpen())
        return;
    Picture backdrop(tradeBackgroundPic);
    backdrop.Load(kTradePictID);
    selTradeItem = 7;
    TradeDialogTabKey(0);
    SetDialogGWorld(tradeDialog);
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
            SetDialogGWorld(tradeDialog);
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
            SetDialogGWorld(tradeDialog);
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

    SetDialogGWorld(tradeDialog);
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
        SetDialogGWorld(tradeDialog);
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
                SetDialogGWorld(tradeDialog);
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

#define kMapFilterCount 8


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
    if (selTradeItem > kMapFilterCount - 1)
        selTradeItem = 0;
    if (selTradeItem < 0)
        selTradeItem = kMapFilterCount - 1;
    while (tradePrice[selTradeItem] == 0) {
        if (back)
            selTradeItem -= 1;
        else
            selTradeItem += 1;
        if (selTradeItem > kMapFilterCount - 1)
            selTradeItem = 0;
        if (selTradeItem < 0)
            selTradeItem = kMapFilterCount - 1;
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

    unsigned char message[256];
    void *item;
    short itemType;
    short missionTons;
    short junkTons;
    short missionSlots;
    Boolean seen[kMaxCargo];
    short junkSlots;
    short k;
    Rect insetBox;
    unsigned char num[256];
    Rect box;
    unsigned char text[2048];
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

    SetGWorldContainer(&loadPort);
    ForeColor(blackColor);
    PaintRect(GetDialogPortRect(tradeDialog));
    if (tradeBackgroundPic != NULL)
        DrawPicHandle(tradeBackgroundPic, GetDialogPortRect(tradeDialog));

    ForeColor(blackColor);
    TextFont(smallTextFont);
    TextSize(smallTextSize);
    GetDialogItem(tradeDialog, kHoldsItem, &itemType, (Handle *)&item, &box);
    if (RectInRgn(&box, GetWindowVisRgn(FrontWindow(tradeDialog)))) {
        RGBForeColor(&rgbDarkGrey);
        FrameRect(&box);
        RGBForeColor(&rgbLightGrey);
        MoveTo(box.left + 7, box.top + 12);
        DrawIndString(kGameStrList, 0xc5);
        MoveTo(box.right - 0x7b, box.top + 12);
        if (TotalFleetHolds() > ShipTotalHolds())
            DrawIndString(kGameStrList, 0xc6);
        else
            DrawIndString(kGameStrList, 0xc7);
        MoveTo(box.right - 0x46, box.top + 12);
        DrawIndString(kGameStrList, 0xc8);
        ForeColor(blackColor);
    }

    for (i = 0; i <= kLastRow; i++) {
        GetDialogItem(tradeDialog, i + kFirstRowItem, &itemType, (Handle *)&item, &box);
        if (RectInRgn(&box, GetWindowVisRgn(FrontWindow(tradeDialog)))) {
            insetBox = box;
            InsetRect(&insetBox, 1, 1);
            if (i == selTradeItem)
                RGBForeColor((RGBColor *)&ldefColors[2]);
            else
                RGBForeColor((RGBColor *)&ldefColors[1]);
            PaintRect(&insetBox);
            ForeColor(blackColor);

            RGBForeColor(&rgbDarkGrey);
            MoveTo(box.left, box.top);
            LineTo(box.left, (box.bottom - 1));
            MoveTo((box.right - 1), box.top);
            LineTo((box.right - 1), (box.bottom - 1));
            if (i == 0) {
                MoveTo(box.left, box.top);
                LineTo((box.right - 1), box.top);
            }
            if (i == kLastRow) {
                MoveTo(box.left, (box.bottom - 1));
                LineTo((box.right - 1), (box.bottom - 1));
            }

            if (tradePrice[i] != 0) {
                RGBForeColor((RGBColor *)&ldefColors[0]);
                if (i <= kLastCommodity) {
                    MoveTo(box.left + 7, box.top + 10);
                    DrawString(cargoName[i]);
                    qty = shipData->cargo[i];
                    if (qty > 0) {
                        NumToString(qty, (StringPtr)text);
                        MoveTo(box.right - (StringWidth((StringPtr)text) + 0x6e),
                                  box.top + 10);
                        DrawString((StringPtr)text);
                    }
                } else if (junkType[i - kFirstJunkItem] != -1) {
                    MoveTo(box.left + 6, box.top + 10);
                    DrawString(junkData[junkType[i - kFirstJunkItem]].name);
                    qty = junkData[junkType[i - kFirstJunkItem]].held;
                    if (qty > 0) {
                        NumToString(qty, (StringPtr)text);
                        MoveTo(box.right - (StringWidth((StringPtr)text) + 0x6e),
                                  box.top + 10);
                        DrawString((StringPtr)text);
                    }
                }

                MoveTo(box.right - 0x46, box.top + 10);
                found = 0;
                for (k = 0; k < kMaxDisasters; k++) {
                    if (disasterData[k].loaded != 0 && disasterData[k].countdown > 0 &&
                        disasterData[k].stellarID == shipData->targetStellar &&
                        i == disasterData[k].commodity) {
                        found = 1;
                        if (disasterData[k].amount > 0)
                            DrawIndString(kGameStrList, 0xcc);
                        if (disasterData[k].amount < 0)
                            DrawIndString(kGameStrList, 0xcd);
                        break;
                    }
                }
                if (!found) {
                    if (i <= kLastCommodity) {
                        if (portDemand[i] == kPortShortage)
                            DrawIndString(kGameStrList, 0xc9);
                        if (portDemand[i] == kPortSurplus)
                            DrawIndString(kGameStrList, 0xca);
                        if (portDemand[i] == kPortEmbargo)
                            DrawIndString(kGameStrList, 0xcb);
                    } else if (junkType[i - kFirstJunkItem] != -1) {
                        if (i == kFirstJunkItem)
                            DrawIndString(kGameStrList, 0xcb);
                        if (i == kLastRow)
                            DrawIndString(kGameStrList, 0xc9);
                    }
                }

                NumToString(tradePrice[i], (StringPtr)text);
                MoveTo(box.right - (StringWidth((StringPtr)text) + 10),
                          box.top + 10);
                DrawString((StringPtr)text);
            }
            ForeColor(blackColor);
        }
    }

    GetDialogItem(tradeDialog, kCargoItem, &itemType, (Handle *)&item, &box);
    if (RectInRgn(&box, GetWindowVisRgn(FrontWindow(tradeDialog)))) {
        ForeColor(blackColor);
        insetBox = box;
        InsetRect(&insetBox, 5, 3);
        strcpy((char *)text, "");

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
                StrcatIndCString((char *)text, kGameStrList, 0x16b);
                strcat((char *)text, " ");
                if (missionTons + junkTons > 0) {
                    NumToString(missionTons + junkTons, num);
                    PtoCstr(num);
                    strcat((char *)text, (char *)num);
                    strcat((char *)text, " ");
                    if (missionTons + junkTons == 1)
                        pstrcpy(num, tonString);
                    else
                        pstrcpy(num, tonsString);
                    PtoCstr(num);
                    strcat((char *)text, (char *)num);
                    strcat((char *)text, " ");
                    StrcatIndCString((char *)text, kGameStrList, 0x187);
                    strcat((char *)text, " ");
                }
                if (missionSlots > 0) {
                    StrcatIndCString((char *)text, kGameStrList, 0x16f);
                    if (junkSlots > 0) {
                        strcat((char *)text, " ");
                        StrcatIndCString((char *)text, kGameStrList, 0x188);
                        strcat((char *)text, " ");
                    }
                }
                if (junkSlots > 0)
                    StrcatIndCString((char *)text, kGameStrList, 0x170);
                strcat((char *)text, "\r\r");
            }
        }

        holds = ShipTotalHolds();
        fleetHolds = TotalFleetHolds();
        totalCargo = TotalCargo(shipData);
        StrcatIndCString((char *)text, kGameStrList, 0x16c);
        if (fleetHolds > holds) {
            strcat((char *)text, " ");
            StrcatIndCString((char *)text, kGameStrList, 0x16d);
        }
        strcat((char *)text, ": ");
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
        NumToString(available, num);
        PtoCstr(num);
        strcat((char *)text, (char *)num);
        strcat((char *)text, " ");
        if (available == 1)
            pstrcpy(num, tonString);
        else
            pstrcpy(num, tonsString);
        PtoCstr(num);
        strcat((char *)text, (char *)num);
        if (fleetHolds > holds) {
            available = fleetHolds - totalCargo;
            if (available < 0)
                available = 0;
            strcat((char *)text, "\r");
            StrcatIndCString((char *)text, kGameStrList, 0x16c);
            strcat((char *)text, " ");
            StrcatIndCString((char *)text, kGameStrList, 0x16e);
            strcat((char *)text, ": ");
            NumToString(available, num);
            PtoCstr(num);
            strcat((char *)text, (char *)num);
            strcat((char *)text, " ");
            if (available == 1)
                pstrcpy(num, tonString);
            else
                pstrcpy(num, tonsString);
            PtoCstr(num);
            strcat((char *)text, (char *)num);
        }

        TETextBox(text, strlen((char *)text), &insetBox, 0);
        InvertRect(&insetBox);
        RGBForeColor(&rgbDarkGrey);
        FrameRect(&box);
        ForeColor(blackColor);
    }
    TradeButtons().Draw(-1);

    GetDialogItem(tradeDialog, kNewsItem, &itemType, (Handle *)&item, &box);
    if (RectInRgn(&box, GetWindowVisRgn(FrontWindow(tradeDialog)))) {
        for (p = 0; p < kMaxDisasters; p++) {
            if (disasterData[p].loaded != 0 && disasterData[p].countdown > 0 &&
                disasterData[p].stellarID == shipData->targetStellar) {
                if (cheatsActive) {
                    strcpy((char *)text, "Disaster ID ");
                    NumToString(p + 0x80, num);
                    PtoCstr(num);
                    strcat((char *)text, (char *)num);
                } else {
                    strncpy((char *)text, (char *)disasterData[p].name, 0x3f);
                    PtoCstr((StringPtr)text);
                }
                strcat((char *)text, " ");
                StrcatIndCString((char *)text, kGameStrList, 0xc0);
                strcat((char *)text, " ");
                if (disasterData[p].amount > 0)
                    StrcatIndCString((char *)text, kGameStrList, 0xc1);
                else
                    StrcatIndCString((char *)text, kGameStrList, 0xc2);
                strcat((char *)text, " ");
                StrcatIndCString((char *)text, kGameStrList, 0xb5);
                strcat((char *)text, " ");
                strncpy((char *)num, (char *)lcCargoName[disasterData[p].commodity], 0x40);
                PtoCstr(num);
                strcat((char *)text, (char *)num);
                strcat((char *)text, ".");

                strcpy((char *)message, (char *)text);
                ForeColor(blackColor);
                TextFont(smallTextFont);
                TextSize(smallTextSize);
                TETextBox(message, strlen((char *)message), &box, 0);
                InvertRect(&box);
                break;
            }
        }
    }

    CopyLoadPortToDialog(tradeDialog);
}
