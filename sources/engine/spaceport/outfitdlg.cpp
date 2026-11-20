/* outfitdlg.cpp -- the outfitter dialog */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "macport.h"
#include "scaleddraw.h"
#include "gametypes.h"
#include "shipobj.h"
#include "resources.h"
#include "gameconst.h"
#include "dialogs.h"
#include "game.h"
#include "stellar.h"
#include "crashlog.h"
#include "prototypes.h"
#include "gameutil.h"
#include "missions.h"
#include "player.h"
#include "render.h"
#include "ships.h"
#include "burger.h"
#include "sound.h"
#include "cefix.h"
#include "viewscale.h"
#include "uiscale.h"
#include "movie.h"
#include "ai.h"
#include "space.h"
#include "dialog.h"
#include "gamedialog.h"
#include "buttons.h"

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

#define kOutfModRaiseMax 0x1b   /* outfit ModType 27: raises another outfit's maximum */


namespace {

// FUNCTION: EVNOVA 0x0048ea70
// MAC: _DoOutfitDialog i386 0x0005bacf ppc 0x00063b20
/* The outfitter: buying and selling outfits; outfits owned on entry sell back at half price. */
class OutfitDialog final : public GameDialog {
public:
    void Run();

private:
    void Hit(short item) override;
    static Boolean Filter(DialogPtr unused, EventRecord *ev, short *itemHit);

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

    k = ShipFuelCapacity(shipData);
    if (shipData->fuel > k) {
        shipData->fuel = k;
        statusRedraw[kRedrawFuel] = 1;
    }
    i = ShipShieldCapacity(shipData);
    if (shipData->shield > i) {
        shipData->shield = i;
        statusRedraw[kRedrawShield] = 1;
    }
    j = ShipArmorCapacity(shipData);
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
                    if (outfitSystem[selOutfitItem].modType[i] == kOutfModRaiseMax
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
                    if (outfitSystem[selOutfitItem].modType[i] == kOutfModWeapon) {
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
                                && !IsDisabled(&shipData[m]))
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
                                    if (outfitSystem[m].modType[j] == kOutfModAmmo
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
                            if (outfitSystem[selOutfitItem].modType[j] == kOutfModWeapon
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
Boolean OutfitDialog::Filter(DialogPtr unused, EventRecord *ev, short *itemHit)
{
    Rect box;
    short itemType;
    Handle item;
    Point where;
    EventKey c;
    short i;
    short count;
    short old;
    short oldPos;
    short row;
    int what;
    int wheel;

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
    if ((int)ev->what == kKeyDown) {
        if (QuickKeyHit(2, -1, 13, itemHit))
            return 1;
        c = EventCharOf(ev);
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
                what = selOutfitListItem;
                what += outfitScroll;
                selOutfitItem = dockedPortItems[what];
                while (selOutfitItem == -1 && selOutfitListItem > 0) {
                    what--;
                    selOutfitListItem--;
                    selOutfitItem = dockedPortItems[what];
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
    what = ev->what;
    if (what == kMouseDown) {
        SetDialogGWorld(outfitDialog);
        RedrawDialog(outfitDialog);
        where = ev->where;
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
                lastClickTime = ev->when;
                *itemHit = -1;
                return 1;
            }
        }
        OutfitDialogUpdate();
        return ButtonItemHit(OutfitButtons().Track(where), kOutfitSlotItems, itemHit);
    }
    if (what == kNullEvent) {
        /* the wheel: one notch presses scroll up (10) or down (11) while that button is lit */
        wheel = MouseTakeWheel();
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
    if (RectInRgn(&box, GetWindowVisRgn(FrontWindow(outfitDialog)))) {
        TextFont(smallTextFont);
        TextSize(smallTextSize);
        TETextBox(dialogDescTextPtr, strlen(dialogDescTextPtr), &box, 0);
        InvertRect(&box);
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
                if (outfitSystem[outfit].modType[i] == kOutfModCargo) {
                    isCargo = 1;
                    cargoDelta = outfitSystem[outfit].modVal[i];
                }
                if (outfitSystem[outfit].modType[i] == kModTypeMap)
                    isMap = 1;
                if (outfitSystem[outfit].modType[i] == kModTypeFakeID)
                    isFakeID = 1;
            }
            if (outfitSystem[outfit].modType[0] == kOutfModAmmo) {
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
