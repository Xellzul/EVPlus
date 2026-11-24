/* shipyarddlg.cpp -- the shipyard dialog and the ship description */

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
#include "space.h"
#include "gamedialog.h"
#include "buttons.h"
#include "text.h"

DialogPtr shipyardDialog;
DialogPtr shipDescDialog;
short selShipyardItem;
short selShipListItem;
short lastClickItem;
short shipyardScroll;
short shipBuyMode;
uint32_t lastClickTime;
float priceFluxMult[2];
PicHandle bigShipPic;
PicHandle shipyardBackgroundPic;
PicHandle shipDescBackgroundPic;
Boolean shipyardEgg;
unsigned char textInputString[256];

#define kShipDescBase 13000    /* desc ID of ship 0's shipyard blurb */
#define kHireDescBase 14000     /* ...and of its hire-an-escort blurb */

/* the item grid: four across, five down, out of item 5's box */
#define kMovieItem    8


#define kShipyardPictID 0x2135

namespace {

// FUNCTION: EVNOVA 0x00492f30
// MAC: _DoShipyardDialog i386 0x0005e679 ppc 0x00066438
/* The shipyard: buying a new ship for the old one's trade-in, or (shipBuyMode) hiring an escort. */
class ShipyardDialog final : public GameDialog {
public:
    void Run();

private:
    void Hit(short item) override;
    static Boolean Filter(DialogPtr unused, const DialogEvent &ev, short *itemHit);

    short savedTargetStellar;   /* the nav target the map may change, restored after it */
    short savedUnk6A;
};

void ShipyardDialog::Run()
{
    void *item;
    short halfHeight;
    Rect box;
    short itemType;
    short weap;
    short n;

    SetupPortAvailableShipTypes(dockedPort);
    done = 1;
    for (weap = 0; weap < kMaxShipTypes; weap++) {
        if (dockedPortShipTypes[weap] != -1) {
            done = 0;
            break;
        }
    }
    if (done) {
        SetShowText(IndString(kGameStrList, shipBuyMode == 0 ? 0xdf : 0xe0));
        ShowTextDialog(0, 0);
        return;
    }
    SWSetPortToWindow(gSpriteWorld);
    selShipyardItem = -1;
    selShipListItem = -1;
    lastClickItem = -1;
    shipyardScroll = 0;
    canBuy = 0;
    savedTargetStellar = shipData->targetStellar;
    savedUnk6A = shipData->navTargetKind;
    previewPic = NULL;
    previewPicID = -1;
    missionsOutside = 5;
    offeringFromShip = 0;
    missionOfferTime = ReadTick() + Rand(30) + 30;
    dialog.Open(0x3ec, ShipyardDialogUpdate, Filter, &shipyardDialog, DialogClose::NoFlash);
    if (!dialog.IsOpen())
        return;
    Picture backdrop(shipyardBackgroundPic);
    Picture preview(previewPic);
    backdrop.Load(kShipyardPictID);
    GetDialogItem(shipyardDialog, kGridItem, &itemType, (Handle *)&item, &box);
    SetupListRects((Point *)&box);
    strncpy(dialogDescTextPtr, "", 3);
    SetDialogGWorld(shipyardDialog);
    FlushEvents(everyEvent - diskMask, 0);
    ForeColor(blackColor);
    ShipyardDialogUpdate();
    if (shipBuyMode == 0)
        OfferMissionFromPort(5);
    RedrawDialog(shipyardDialog);
    GetDialogItem(shipyardDialog, kMovieItem, &itemType, (Handle *)&item, &box);

    Loop();

    if (shipBuyMode != 0) {
        n = -((GetDialogPortRect(shipyardDialog)->right -
                       GetDialogPortRect(shipyardDialog)->left) / 2);
        halfHeight = -((GetDialogPortRect(shipyardDialog)->bottom -
                        GetDialogPortRect(shipyardDialog)->top) / 2);
    }
    EndLoopingMovie();
    dialog.Close();
    shipyardEgg = 0;
    backdrop.Release();
    preview.Release();
    if (shipBuyMode != 0) {
        box.left = box.right = ((GetCWindowPortRect((WindowRef)gWind)->left +
                             GetCWindowPortRect((WindowRef)gWind)->right) / 2);
        box.top = box.bottom = ((GetCWindowPortRect((WindowRef)gWind)->top +
                              GetCWindowPortRect((WindowRef)gWind)->bottom) / 2);
        InsetRect(&box, n - 1, halfHeight - 1);
        SWSetPortToWindow(gSpriteWorld);
        ForeColor(blackColor);
        PaintRect(&box);
        RedrawStatus(1);
    }
    SWSetPortToBackground(gSpriteWorld);
    SetSystemBackgroundColor();
    PaintRect(&gSpriteWorld->backFrameP->frameRect);
    ForeColor(blackColor);
    RedrawStatus(0);
    SWSetPortToWindow(gSpriteWorld);
    shipData->targetStellar = savedTargetStellar;
    shipData->navTargetKind = savedUnk6A;
    statusRedraw[kRedrawNav] = 1;
    ResetMissionOfferVenue();
}

void ShipyardDialog::Hit(short item)
{
    std::string text;
    unsigned char prompt[256];
    int hireCost;
    unsigned char defaultName[256];
    short i;
    short outf;
    int32_t tradeIn;
    int32_t cost;
    short weap;
    Boolean weapHasSlot;
    short n;
    Boolean confirmed;

    if (item == 1) {
        if (selShipyardItem != -1) {
            n = selShipyardItem;
            if (canBuy != 0) {
                if (shipBuyMode == 0) {

                    RedrawDialog(shipyardDialog);
                    ToPascal(TextF("%s %.199s: ", IndString(kGameStrList, 0x78).c_str(), shipyardLongNames[n].c_str()),
                             prompt);
                    text = FromNameField(shipClass[n].name);
                    text += " ";
                    for (weap = 0; weap <= 2; weap++)
                        text += NumText(Rand(9) + 1);
                    ToPascal(text, defaultName);
                    confirmed = EVTextInputDialog(prompt, defaultName, 0x40);
                    SetDialogGWorld(shipyardDialog);
                    RedrawDialog(shipyardDialog);
                    if (confirmed != 0) {
                        didBuyShip = 1;
                        tradeIn = ApplyPriceAndTechnologyFlux(
                            PlayerShipTradeInPrice(), priceFluxMult[0],
                            shipClass[shipData->type].techLevel,
                            stellarData[shipData->targetStellar].techLevel);
                        tradeIn = ApplyPriceAndTechnologyFlux(
                            tradeIn, priceFluxMult[1],
                            shipClass[shipData->type].techLevel,
                            stellarData[shipData->targetStellar].techLevel);
                        shipData->credits += tradeIn;
                        crashlog_mission_bit_set(
                            "retiring ship", (char *)shipClass[shipData->type].name,
                            1, shipData->type + 128,
                            shipClass[shipData->type].onRetire);
                        EvalMissionBitSetString(shipClass[shipData->type].onRetire);
                        shipData->type = n;
                        cost = ApplyPriceAndTechnologyFlux(
                            shipClass[n].cost, priceFluxMult[1],
                            shipClass[n].techLevel,
                            stellarData[shipData->targetStellar].techLevel);
                        shipData->credits -= cost;
                        pstrcpy(shipName, textInputString);
                        CullNameString(shipName);
                        for (weap = 1; weap < kMaxShips; weap++) {
                            if (shipData[weap].active != 0 &&
                                shipData[weap].parentShip == 0 &&
                                shipData[weap].aiType == kAIFighter)
                                shipData[weap].active = 0;
                        }
                        for (weap = 0; weap < kMaxWeapTypes; weap++) {
                            weapHasSlot = 0;
                            for (outf = 0; outf < kMaxOutfits; outf++) {
                                if (outfitSystem[outf].modType[0] == OutfMod::Weapon &&
                                    outfitSystem[outf].modVal[0] == weap &&
                                    outfitSystem[outf].persistent != 0) {
                                    weapHasSlot = 1;
                                    break;
                                }
                            }
                            if (!weapHasSlot) {
                                shipData->weap[weap].count = 0;
                                shipData->weap[weap].ammo = 0;
                            }
                        }
                        for (weap = 0; weap < kMaxOutfits; weap++) {
                            if (outfitSystem[weap].persistent == 0)
                                shipSystems[weap] = 0;
                        }
                        for (outf = 0; outf < kDefaultOutfitSlots; outf++) {
                            if (shipClass[shipData->type].defOutfCount[outf] > 0)
                                shipSystems[shipClass[shipData->type].defOutfID[outf]] +=
                                    shipClass[shipData->type].defOutfCount[outf];
                        }
                        SystemInfoToShipStats();
                        for (weap = 0; weap < kMaxWeapTypes; weap++) {
                            if (shipClass[shipData->type].weapCount[weap] >
                                shipData->weap[weap].count)
                                shipData->weap[weap].count =
                                    shipClass[shipData->type].weapCount[weap];
                            if (shipClass[shipData->type].weapAmmo[weap] > 0) {
                                if (weaponStats[weap].ammoType >= 0 && weaponStats[weap].ammoType < kMaxWeapTypes
                                    && weaponStats[weap].guidance != kGuidanceBay)
                                    i = weaponStats[weap].ammoType;
                                else
                                    i = weap;
                                if (shipClass[shipData->type].weapAmmo[weap] >
                                    shipData->weap[i].ammo)
                                    shipData->weap[i].ammo =
                                        shipClass[shipData->type].weapAmmo[weap];
                            }
                        }
                        DestroyPartialFleetCargo(0);
                        crashlog_mission_bit_set(
                            "purchasing ship", (char *)shipClass[shipData->type].name,
                            1, shipData->type + 128,
                            shipClass[shipData->type].onPurchase);
                        EvalMissionBitSetString(shipClass[shipData->type].onPurchase);
                        shipData->targetShip = -1;
                        shipData->selectedWeapon = -1;
                        playerPaint.red = playerPaint.green =
                            playerPaint.blue = 0;
                        statusRedraw[kRedrawWeap] = 1;
                        statusRedraw[kRedrawNav] = 1;
                        statusRedraw[kRedrawTarg] = 1;
                        statusRedraw[kRedrawCargo] = 1;
                        shipData->shield = shipData->ShieldCapacity();
                        shipData->armor = shipData->ArmorCapacity();
                        shipData->fuel = shipData->FuelCapacity();
                        shipData->pos.x = dockedPort->pos.h;
                        shipData->pos.y = dockedPort->pos.v;
                        statusRedraw[kRedrawCargo] = 1;
                        LoadIntfResource();
                        DrawStatus();
                        SetDialogGWorld(shipyardDialog);
                        done = 1;
                    }
                    shipClass[selShipyardItem].dailySaleRoll = Rand(100) + 1;
                    SetDialogGWorld(shipyardDialog);
                    RedrawDialog(shipyardDialog);
                } else {
                    hireCost = ApplyPriceAndTechnologyFlux(shipClass[selShipyardItem].cost,
                                               priceFluxMult[1],
                                               shipClass[selShipyardItem].techLevel,
                                               stellarData[shipData->targetStellar].techLevel);
                    shipData->credits -= hireCost * 0.1;
                    statusRedraw[kRedrawCargo] = 1;
                    SpawnEscort(selShipyardItem, shipData->targetStellar);
                    shipClass[selShipyardItem].dailyHireRoll = Rand(100) + 1;
                    done = 1;
                }
            } else {
                PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);
            }
        } else {
            PlaySound(beepSnd[3], 1, gSoundVolume, gSoundVolume);
        }
    }
    if (item == 3) {
        savedTargetStellar = shipData->targetStellar;
        savedUnk6A = shipData->navTargetKind;
        MapFromDialog(shipyardDialog);
    }
    if (item == 4)
        PlayerInfoFromDialog(shipyardDialog);
    if (item == 8 && shipBuyMode == 0)
        OfferFromDialog(5, shipyardDialog);
    if (item == 0xb)
        MissionInfoFromDialog(shipyardDialog);
    if (item == 10 && selShipyardItem != -1) {
        ShipyardDialogUpdate();
        DoShipyardDescription(selShipyardItem);
        SetDialogGWorld(shipyardDialog);
        RedrawDialog(shipyardDialog);
    }
    if (item == 0xc && shipyardScroll >= kGridCols) {
        shipyardScroll -= kGridCols;
        selShipyardItem = selShipListItem = -1;
        strncpy(dialogDescTextPtr, "", 3);
        EndLoopingMovie();
        RedrawDialog(shipyardDialog);
    }
    if (item == 0xd) {
        n = 0;
        for (weap = 0; weap < kMaxShipTypes; weap++) {
            if (dockedPortShipTypes[weap] != -1)
                n++;
        }
        if (shipyardScroll < n - kGridCells) {
            shipyardScroll += kGridCols;
            selShipyardItem = selShipListItem = -1;
            strncpy(dialogDescTextPtr, "", 3);
            EndLoopingMovie();
            RedrawDialog(shipyardDialog);
        }
    }
    if (item == 7)
        done = 1;
}

constexpr short kShipyardSlotItems[] = { 7, 1, 10, 12, 13 };

// FUNCTION: EVNOVA 0x00493fc0
// MAC: _ShipyardFilter i386 0x00060449 ppc 0x00068154
Boolean ShipyardDialog::Filter(DialogPtr unused, const DialogEvent &ev, short *itemHit)
{
    Rect box;
    Rect movieBox;
    short itemType;
    Handle item;
    Point where;
    EventChar c;
    short i;
    short count;
    short old;
    short oldPos;
    short row;
    int wheel;

    if (LeaveRequested(7, itemHit))
        return 1;
    LoopingMovieIdle();
    *itemHit = -1;
    CalcShipCanBuy();
    if (ev.what == DialogInput::Key) {
        c = ev.ch;
        if (IsReturnChar(c) || c == kEscapeChar) {
            *itemHit = 7;
            return 1;
        }
        c = tolower(c);
        if ((c == 'b' && shipBuyMode == 0) || (c == 'h' && shipBuyMode == 1)) {
            *itemHit = 1;
            return 1;
        }
        if (c == ' ') {
            *itemHit = 10;
            return 1;
        }
        if (c == kGridLeftKey || c == kGridRightKey || c == kGridUpKey ||
            c == kGridDownKey || c == kGridPageUpKey || c == kGridPageDownKey) {
            old = selShipyardItem;
            if (selShipListItem != -1) {
                count = 0;
                for (i = 0; i < 512; i++) { /* BUG: not kMaxShipTypes: with over 512 ships for sale the keys stop scrolling early */
                    if (dockedPortShipTypes[i] != -1)
                        count++;
                }
                if (c == kGridRightKey) {
                    if (selShipListItem < kGridCells - 1) {
                        if (dockedPortShipTypes[shipyardScroll + selShipListItem
                                                + 1] != -1)
                            selShipListItem++;
                    } else if (shipyardScroll < count - kGridCells) {
                        shipyardScroll += kGridCols;
                        selShipListItem -= kGridCols - 1;
                    }
                } else if (c == kGridLeftKey) {
                    if (selShipListItem > 0) {
                        selShipListItem--;
                    } else if (shipyardScroll > 0) {
                        shipyardScroll -= kGridCols;
                        selShipListItem += kGridCols - 1;
                    }
                } else if (c == kGridUpKey || c == kGridPageUpKey) {
                    if (selShipListItem >= kGridCols)
                        selShipListItem -= kGridCols;
                    else if (shipyardScroll > 0)
                        shipyardScroll -= kGridCols;
                } else if (c == kGridDownKey || c == kGridPageDownKey) {
                    if (selShipListItem < kGridCells - kGridCols) {
                        if (dockedPortShipTypes[shipyardScroll + selShipListItem
                                                + kGridCols] != -1)
                            selShipListItem += kGridCols;
                    } else if (shipyardScroll < count - kGridCells) {
                        shipyardScroll += kGridCols;
                    }
                }
                selShipyardItem = dockedPortShipTypes[shipyardScroll + selShipListItem];
                while (selShipyardItem == -1 && selShipListItem > 0) {
                    selShipListItem--;
                    selShipyardItem = dockedPortShipTypes[shipyardScroll + selShipListItem];
                }
            } else {
                if (c == kGridRightKey || c == kGridDownKey ||
                    c == kGridPageDownKey)
                    selShipListItem = 0;
                else
                    selShipListItem = kGridCells - 1;
                selShipyardItem = dockedPortShipTypes[shipyardScroll + selShipListItem];
                while (selShipyardItem == -1 && selShipListItem > 0) {
                    selShipListItem--;
                    selShipyardItem = dockedPortShipTypes[shipyardScroll + selShipListItem];
                }
            }
            if (selShipyardItem != old) {
                if (selShipyardItem != -1) {
                    if (shipBuyMode != 0)
                        LoadDescription(dialogDescTextPtr,
                                        (selShipyardItem + kHireDescBase));
                    else
                        LoadDescription(dialogDescTextPtr,
                                        (selShipyardItem + kShipDescBase));
                    RedrawDialog(shipyardDialog);
                    CalcShipCanBuy();
                    EndLoopingMovie();
                    if (selShipyardItem != -1 &&
                        !shipMovieFilenames[selShipyardItem].empty()) {
                        GetDialogItem(shipyardDialog, kMovieItem, &itemType,
                                      &item, &movieBox);
                        StartLoopingMovie(shipMovieFilenames[selShipyardItem],
                                          FrontWindow(shipyardDialog),
                                          &movieBox);
                    }
                } else {
                    canBuy = 0;
                }
            }
        }
    }
    if (ev.what == DialogInput::Key) {
        if (QuickKeyHit(3, 4, 11, itemHit))
            return 1;
        c = tolower(ev.ch);
        return 0;
    }
    if (ev.what == DialogInput::Click) {
        where = ev.where;
        GetDialogItem(shipyardDialog, kGridItem, &itemType, &item, &box);
        if (PtInRectPoint(where, &box)) {
            count = (where.h - box.left) / kGridCellWidth;
            row = (where.v - box.top) / kGridCellHeight;
            if (count >= 0 && count <= kGridCols - 1 && row >= 0 &&
                row <= kGridCells / kGridCols - 1) {
                oldPos = selShipListItem;
                selShipListItem = row * kGridCols + count;
                if (dockedPortShipTypes[selShipListItem + shipyardScroll] == -1) {
                    selShipyardItem = selShipListItem = -1;
                } else {
                    selShipyardItem =
                        dockedPortShipTypes[selShipListItem + shipyardScroll];
                }
                if (selShipListItem != oldPos) {
                    InvalDialogItem(shipyardDialog, 1);
                    InvalDialogItem(shipyardDialog, 5);
                    InvalDialogItem(shipyardDialog, 6);
                    InvalDialogItem(shipyardDialog, 8);
                    InvalDialogItem(shipyardDialog, 9);
                    InvalDialogItem(shipyardDialog, 10);
                    if (selShipListItem != -1) {
                        if (shipBuyMode != 0)
                            LoadDescription(dialogDescTextPtr,
                                            (selShipyardItem + kHireDescBase));
                        else
                            LoadDescription(dialogDescTextPtr,
                                            (selShipyardItem + kShipDescBase));
                    } else {
                        strncpy(dialogDescTextPtr, "", 3);
                    }
                    CalcShipCanBuy();
                    EndLoopingMovie();
                    if (selShipyardItem != -1 &&
                        !shipMovieFilenames[selShipyardItem].empty()) {
                        GetDialogItem(shipyardDialog, kMovieItem, &itemType,
                                      &item, &movieBox);
                        StartLoopingMovie(shipMovieFilenames[selShipyardItem],
                                          FrontWindow(shipyardDialog),
                                          &movieBox);
                    }
                }
                if (selShipListItem == lastClickItem && selShipListItem != -1 &&
                    ev.when - lastClickTime <= 15) {
                    ShipyardDialogUpdate();
                    DoShipyardDescription(selShipyardItem);
                    SetDialogGWorld(shipyardDialog);
                    RedrawDialog(shipyardDialog);
                }
                lastClickItem = selShipListItem;
                lastClickTime = ev.when;
            }
        }
        ShipyardDialogUpdate();
        return ButtonItemHit(ShipyardButtons().Track(where), kShipyardSlotItems, itemHit);
    }
    if (ev.what == DialogInput::Idle) {
        /* the wheel: one notch presses scroll up (12) or down (13) while that button is lit */
        wheel = MouseTakeWheel();
        if (wheel > 0 && shipyardScroll >= kGridCols) {
            *itemHit = 0xc;
            return 1;
        }
        if (wheel < 0) {
            count = 0;
            for (i = 0; i < kMaxShipTypes; i++) {
                if (dockedPortShipTypes[i] != -1)
                    count++;
            }
            if (shipyardScroll < count - kGridCells) {
                *itemHit = 0xd;
                return 1;
            }
        }
    }
    if (shipBuyMode == 0)
        return OfferDue(8, itemHit);
    return 0;
}

/* the hire box's daily "Pay: <n> cr" line */
void DrawEscortPay(const Rect &box, int32_t pay)
{
    MoveTo(box.left, box.top + 0x18);
    DrawIndString(kGameStrList, 0x129);
    MoveTo(box.left + 0x46, box.top + 0x18);
    DrawCommaNumber(pay);
    DrawCString(" ");
    DrawIndString(kGameStrList, 0x22);
}

} // namespace

void DoShipyardDialog(void)
{
    ShipyardDialog().Run();
}

// FUNCTION: EVNOVA 0x004948b0
// MAC: _ShipyardDialogUpdate i386 0x00058a10 ppc 0x00060d04
void ShipyardDialogUpdate(void)
{
    int c;
    short i;
    Handle item;
    short itemType;
    std::string line1;
    Rect box;
    std::string line2;
    std::string id;
    short drawn;
    short skipped;
    int32_t price;
    int32_t tradeIn;
    int32_t net;

    SetGWorldContainer(&loadPort);
    ForeColor(blackColor);
    PaintRect(GetDialogPortRect(shipyardDialog));
    if (shipyardBackgroundPic != NULL)
        DrawPicHandle(shipyardBackgroundPic, GetDialogPortRect(shipyardDialog));
    skipped = 0;
    GetDialogItem(shipyardDialog, kGridItem, &itemType, &item, &box);
    box.right += 1;
    box.bottom += 1;
    if (RectInRgn(&box, GetWindowVisRgn(FrontWindow(shipyardDialog)))) {
        TextFont(smallTextFont);
        TextSize(smallTextSize);
        ForeColor(blackColor);
        drawn = 0;
        for (i = shipyardScroll; i < kMaxShipTypes; i++) {
                if (RectInRgn(&listScreenRect[i - shipyardScroll],
                              GetWindowVisRgn(FrontWindow(shipyardDialog))) &&
                    drawn < kGridCells && (i >= 0 &&
                    dockedPortShipTypes[i] != -1)) {
                    if (skipped < 3) {
                        if (i - shipyardScroll >= 0 &&
                            i - shipyardScroll < kGridCells) {
                            if (!RetrieveMenuIcon(
                                    shipClass[dockedPortShipTypes[i]].shipyardPictID,
                                    MenuIconPort(&loadPort), &listPicScreenRect[i - shipyardScroll]))
                                skipped++;
                        } else {
                            skipped++;
                        }
                    } else {
                        InvalDialogRect(shipyardDialog, &box);
                        break;
                    }
                    if (cheatsActive) {
                        id = NumText(dockedPortShipTypes[i] + kFirstResourceID);
                        ForeColor(whiteColor);
                        MoveTo(((listScreenRect[i - shipyardScroll].left
                                        + listScreenRect[i - shipyardScroll].right
                                        - MeasureCString(id)) / 2),
                               (listScreenRect[i - shipyardScroll].top + 24));
                        DrawCString(id);
                        ForeColor(blackColor);
                    }
                    if (SplitTwoLineString(shipyardShortNames[dockedPortShipTypes[i]],
                                           line1, line2)) {
                        c = (unsigned char)line1[0];
                        if (isalpha(HeldChar(c, line1[0])) || isdigit(HeldChar(c, line1[0])))
                            ForeColor(whiteColor);
                        else
                            RGBForeColor(&rgbLessDarkGrey);
                        MoveTo((listScreenRect[i - shipyardScroll].left
                                       + listScreenRect[i - shipyardScroll].right) / 2
                                      - MeasureCString(line1) / 2,
                                  listScreenRect[i - shipyardScroll].bottom - 14);
                        DrawCString(line1);
                        c = (unsigned char)line2[0];
                        if (isalpha(HeldChar(c, line2[0])) || isdigit(HeldChar(c, line2[0])))
                            ForeColor(whiteColor);
                        else
                            RGBForeColor(&rgbLessDarkGrey);
                        MoveTo((listScreenRect[i - shipyardScroll].left
                                       + listScreenRect[i - shipyardScroll].right) / 2
                                      - MeasureCString(line2) / 2,
                                  listScreenRect[i - shipyardScroll].bottom - 3);
                        DrawCString(line2);
                        ForeColor(blackColor);
                    } else {
                        MoveTo((listScreenRect[i - (int32_t)shipyardScroll].left
                                       + listScreenRect[i - (int32_t)shipyardScroll].right) / 2
                                      - MeasureCString(line1) / 2,
                                  listScreenRect[i - (int32_t)shipyardScroll].bottom - 6);
                        ForeColor(whiteColor);
                        DrawCString(line1);
                        ForeColor(blackColor);
                    }
                    RGBForeColor(&shopGridColor[1]);
                    FrameRect(&listScreenRect[i - shipyardScroll]);
                    ForeColor(blackColor);
                    drawn++;
                }
        }
        if (selShipListItem != -1) {
            RGBForeColor(&shopGridColor[0]);
            FrameRect(&listScreenRect[selShipListItem]);
            ForeColor(blackColor);
        }
    }
    if (selShipyardItem != -1) {
        GetDialogItem(shipyardDialog, kMovieItem, &itemType, &item, &box);
        if (RectInRgn(&box, GetWindowVisRgn(FrontWindow(shipyardDialog)))) {
            if (loopingMovie != NULL) {
                CopyBits(GetPortBitMapForCopyBits(
                             GetDialogPort(shipyardDialog)),
                         (BitMapPtr)GWorldContainerBits(&loadPort), &box, &box, 0,
                         GetWindowVisRgn(FrontWindow(shipyardDialog)));
            } else {
                if (previewPicID != shipClass[selShipyardItem].shipyardPictID) {
                    if (previewPic != NULL) {
                        QDReleaseResource(previewPic);
                        previewPic = NULL;
                    }
                    previewPicID = shipClass[selShipyardItem].shipyardPictID;
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
                    DrawStringCentered(IndString(kGameStrList, 0xd5), box.left, box.right,
                                       ((box.bottom + box.top) / 2 - 6));
                    DrawStringCentered(IndString(kGameStrList, 0xd6), box.left, box.right,
                                       ((box.bottom + box.top) / 2 + 6));
                    ForeColor(blackColor);
                }
            }
        }
        GetDialogItem(shipyardDialog, 9, &itemType, &item, &box);
        if (RectInRgn(&box, GetWindowVisRgn(FrontWindow(shipyardDialog)))) {
            TextFont(smallTextFont);
            TextSize(smallTextSize);
            if (shipBuyMode == 0 && selShipyardItem != shipData->type) {
                ForeColor(whiteColor);
                MoveTo(box.left, (box.top + 12));
                DrawIndString(kGameStrList, 0xe1);
                MoveTo((box.left + 0x46), (box.top + 12));
                price = ApplyPriceAndTechnologyFlux(
                    shipClass[selShipyardItem].cost, priceFluxMult[1],
                    shipClass[selShipyardItem].techLevel,
                    stellarData[shipData->targetStellar].techLevel);
                DrawCommaNumber(price);
                DrawCString(" ");
                DrawIndString(kGameStrList, 0x22);
                MoveTo(box.left, (box.top + 0x18));
                DrawIndString(kGameStrList, 0xe2);
                MoveTo((box.left + 0x46), (box.top + 0x18));
                net = tradeIn = ApplyPriceAndTechnologyFlux(
                    PlayerShipTradeInPrice(), priceFluxMult[0],
                    shipClass[shipData->type].techLevel,
                    stellarData[shipData->targetStellar].techLevel);
                tradeIn = ApplyPriceAndTechnologyFlux(
                    net, priceFluxMult[1],
                    shipClass[shipData->type].techLevel,
                    stellarData[shipData->targetStellar].techLevel);
                DrawCommaNumber(tradeIn);
                DrawCString(" ");
                DrawIndString(kGameStrList, 0x22);
                net = price - tradeIn;
                if (net < 0)
                    net = 0;
                MoveTo(box.left, (box.top + 0x30));
                DrawIndString(kGameStrList, 0xe3);
                MoveTo((box.left + 0x46), (box.top + 0x30));
                DrawCommaNumber(net);
                DrawCString(" ");
                DrawIndString(kGameStrList, 0x22);
                MoveTo(box.left, (box.top + 0x48));
                DrawIndString(kGameStrList, 0xd9);
                MoveTo((box.left + 0x46), (box.top + 0x48));
                DrawCommaNumber(shipData->credits);
                DrawCString(" ");
                DrawIndString(kGameStrList, 0x22);
                ForeColor(blackColor);
            } else if (shipBuyMode != 0) {
                ForeColor(whiteColor);
                MoveTo(box.left, (box.top + 12));
                DrawIndString(kGameStrList, 0xe4);
                MoveTo((box.left + 0x46), (box.top + 12));
                tradeIn = ApplyPriceAndTechnologyFlux(
                    shipClass[selShipyardItem].cost, priceFluxMult[1],
                    shipClass[selShipyardItem].techLevel,
                    stellarData[shipData->targetStellar].techLevel);
                DrawCommaNumber(0.1 * tradeIn);
                DrawCString(" ");
                DrawIndString(kGameStrList, 0x22);
                DrawEscortPay(box, (int32_t)(0.01 * shipClass[selShipyardItem].cost));
                MoveTo(box.left, (box.top + 0x30));
                DrawIndString(kGameStrList, 0xd9);
                MoveTo((box.left + 0x46), (box.top + 0x30));
                DrawCommaNumber(shipData->credits);
                DrawCString(" ");
                DrawIndString(kGameStrList, 0x22);
                ForeColor(blackColor);
            }
        }
    }
    GetDialogItem(shipyardDialog, 6, &itemType, &item, &box);
    if (RectInRgn(&box, GetWindowVisRgn(FrontWindow(shipyardDialog)))) {
        TextFont(smallTextFont);
        TextSize(smallTextSize);
        TETextBox(dialogDescTextPtr, strlen(dialogDescTextPtr), &box, 0);
        InvertRect(&box);
    }
    ShipyardButtons().Draw(-1);
    CopyLoadPortToDialog(shipyardDialog);
}

#define kLabelStrList 150
#define kLabelStrIdx  5


#define kDescDlogID      0x3fb   /* 1019: the layout with a picture well */
#define kPlainDescDlogID 0x3ed   /* 1005: the layout without one */
#define kDescPictID      8507    /* backdrop to match each layout */
#define kPlainDescPictID 8506

namespace {

// FUNCTION: EVNOVA 0x004956a0
// MAC: _DoShipyardDescription i386 0x0005b6d2 ppc 0x00063764
/* A ship's full description, opened from the shipyard. */
class ShipDescDialog final : public GameDialog {
public:
    void Run();

private:
    void Hit(short item) override;
    static Boolean Filter(DialogPtr unused, const DialogEvent &ev, short *itemHit);
};

void ShipDescDialog::Run()
{
    Picture bigShip(bigShipPic);
    bigShipPic = NULL;
    if (descResourcePictId >= kFirstResourceID)
        bigShip.Load(descResourcePictId);
    dialog.Open(bigShipPic != NULL ? kDescDlogID : kPlainDescDlogID, ShipDescDialogUpdate, Filter,
                &shipDescDialog, DialogClose::NoFlash);
    if (!dialog.IsOpen())
        return;
    Picture backdrop(shipDescBackgroundPic);
    backdrop.Load(descResourcePictId >= kFirstResourceID ? kDescPictID : kPlainDescPictID);
    SetDialogGWorld(shipDescDialog);
    FlushEvents(everyEvent - diskMask, 0);
    ForeColor(blackColor);
    RedrawDialog(shipDescDialog);
    ShipDescDialogUpdate();
    Loop();
    dialog.Close();
    backdrop.Release();
    bigShip.Release();
}

void ShipDescDialog::Hit(short item)
{
    if (item == 1)
        done = 1;
    if (item == 4)
        MapFromDialog(shipDescDialog);
    if (item == 2)
        PlayerInfoFromDialog(shipDescDialog);
    if (item == 6)
        MissionInfoFromDialog(shipDescDialog);
}

// FUNCTION: EVNOVA 0x004959d0
// MAC: _ShipDescFilter i386 0x000510e5 ppc 0x00059a0c
Boolean ShipDescDialog::Filter(DialogPtr unused, const DialogEvent &ev, short *itemHit)
{
    short itemType;
    void *item;
    unsigned char label[256];
    Rect box;
    Point where;
    short lastHit;
    short hit;
    EventChar c;

    if (ev.what == DialogInput::Key) {
        if (QuickKeyHit(4, 2, 6, itemHit))
            return 1;
        c = ev.ch;
        c = tolower(c);
        if (IsReturnChar(c) || c == kEscapeChar || c == ' ') {
            *itemHit = 1;
            return 1;
        }
    }
    if (ev.what == DialogInput::Click) {
        where = ev.where;
        GetDialogItem(shipDescDialog, 1, &itemType, (Handle *)&item, &box);
        if (PtInRectPoint(where, &box)) {
            GetIndString(label, kLabelStrList, kLabelStrIdx);
            DrawButton(GetDialogPort(shipDescDialog), &box, label, 0, 1, 0);
            nv_UpdateDialogWindow(0);
            lastHit = 1;
            while (StillDown()) {
                BusyIdle();
                GetMouse(&where);
                if (PtInRectPoint(where, &box))
                    hit = 1;
                else
                    hit = -1;
                if (hit != lastHit) {
                    if (hit == 1)
                        DrawButton(GetDialogPort(shipDescDialog), &box, label, 0, 1, 0);
                    else
                        DrawButton(GetDialogPort(shipDescDialog), &box, label, 0, 0, 0);
                    nv_UpdateDialogWindow(0);
                }
                lastHit = hit;
            }
            if (lastHit == 1) {
                *itemHit = 1;
                return 1;
            }
            *itemHit = -1;
            return 1;
        }
    }
    return 0;
}

} // namespace

void DoShipyardDescription(short ship)
{
    ShipDescDialog().Run();
}

static const char kTotalSpaceLabel[] = "Total Space:";

/* the value lines of the ship description */
enum {
    kStatSpeed,
    kStatAccel,
    kStatTurn,
    kStatShield,
    kStatArmor,
    kStatGuns,
    kStatTurrets,
    kStatSpace,
    kStatTotalSpace,
    kStatCargo,
    kStatFuel,
    kStatLength,
    kStatMass,
    kStatCrew
};

/* a value line's text: the words before the number, the number (or the whole text when there is none), the
   words after it; and the number the line compares by */
struct StatText {
    std::string pre;
    std::string num;
    std::string post;
    int32_t value;
};

/* The stock ship of a shipyard variant: the first buyable or hirable ship with its name and display weight.
   -1 when that is `type' itself or there is none. */
static short StockShipOf(short type)
{
    short i;
    short len;

    len = shipClass[type].name[0] < 63 ? shipClass[type].name[0] : 63;
    if (len == 0)
        return -1;
    for (i = 0; i < kMaxShipTypes; i++) {
        if (shipClass[i].techLevel >= 0 && shipClass[i].techLevel != kResUnloaded
            && (shipClass[i].buyRandom != 0 || shipClass[i].hireRandom != 0)
            && shipClass[i].dispWeight == shipClass[type].dispWeight
            && memcmp(shipClass[i].name, shipClass[type].name, len + 1) == 0)
            return i == type ? -1 : i;
    }
    return -1;
}

/* the free space a ship has left with its standard weapons, ammo and outfits */
static int32_t FreeMassAfterLoadout(short type)
{
    int32_t freeMass;
    int32_t mass;
    short i;
    short j;

    freeMass = shipClass[type].freeMass;
    for (i = 0; i < kMaxWeapTypes; i++) {
        if (shipClass[type].weapCount[i] > 0) {
            for (j = 0; j < kMaxOutfits; j++) {
                if (outfitSystem[j].modType[0] == OutfMod::Weapon
                    && i == outfitSystem[j].modVal[0]
                    && outfitSystem[j].techLevel < kOutfTechAbsent) {
                    mass = AdjustedItemMass(&outfitSystem[j], &shipClass[type]);
                    mass = shipClass[type].weapCount[i] * mass;
                    if (mass > freeMass)
                        freeMass = 0;
                    else
                        freeMass -= mass;
                    break;
                }
            }
        }
        if (shipClass[type].weapAmmo[i] > 0) {
            for (j = 0; j < kMaxOutfits; j++) {
                if (outfitSystem[j].modType[0] == OutfMod::Ammo
                    && i == outfitSystem[j].modVal[0]
                    && outfitSystem[j].techLevel < kOutfTechAbsent) {
                    mass = AdjustedItemMass(&outfitSystem[j], &shipClass[type]);
                    mass = shipClass[type].weapAmmo[i] * mass;
                    if (mass > freeMass)
                        freeMass = 0;
                    else
                        freeMass -= mass;
                    break;
                }
            }
        }
    }
    for (i = 0; i < kDefaultOutfitSlots; i++) {
        if (shipClass[type].defOutfID[i] != -1
            && shipClass[type].defOutfCount[i] > 0
            && outfitSystem[shipClass[type].defOutfID[i]].techLevel < kOutfTechAbsent) {
            mass = AdjustedItemMass(&outfitSystem[shipClass[type].defOutfID[i]],
                                    &shipClass[type]);
            mass = shipClass[type].defOutfCount[i] * mass;
            if (mass > freeMass)
                freeMass = 0;
            else
                freeMass -= mass;
        }
    }
    return freeMass;
}

/* the kGameStrList word for an acceleration */
static short AccelWord(float accel)
{
    if (accel > 0.073)
        return 245;
    if (accel > 0.055)
        return 246;
    if (accel > 0.038)
        return 247;
    if (accel > 0.025)
        return 248;
    if (accel > 0.013)
        return 249;
    if (accel > 0.0)
        return 250;
    return 396;
}

/* the kGameStrList word for a turn rate */
static short TurnWord(float turnRate)
{
    if (turnRate <= 0.0f)
        return 396;
    if (turnRate <= 1.0f)
        return 250;
    if (turnRate <= 2.0f)
        return 249;
    if (turnRate <= 3.0f)
        return 248;
    if (turnRate <= 4.0f)
        return 247;
    if (turnRate <= 5.0f)
        return 246;
    return 245;
}

/* " ton" or " tons" */
static std::string TonsText(int32_t value)
{
    return " " + UpToNul(value == 1 ? tonString : tonsString);
}

/* Value line `stat' of ship `type'. Accel and turn show the word of ship `wordType' (its own when either is N/A)
   and Player Info's number. */
static void StatOf(StatText *s, short stat, short type, short wordType)
{
    ShipRec *ship;
    float rate;
    short word;
    int32_t freeMass;

    ship = &shipClass[type];
    s->pre.clear();
    s->num.clear();
    s->post.clear();
    s->value = 0;
    switch (stat) {
    case kStatSpeed:
        s->value = RoundD(ship->maxSpeed * 100.0);
        s->num = NumText(s->value);
        break;
    case kStatAccel:
    case kStatTurn:
        if (stat == kStatAccel) {
            word = AccelWord(shipClass[wordType].accel);
            if (word == 396 || AccelWord(ship->accel) == 396)
                word = AccelWord(ship->accel);
            rate = ship->accel;
            if (rate < 0.0f)
                rate = 0.0f;
            rate *= 2.0;
            s->value = TruncF32(rate * 2500.0);
        } else {
            word = TurnWord(shipClass[wordType].turnRate);
            if (word == 396 || TurnWord(ship->turnRate) == 396)
                word = TurnWord(ship->turnRate);
            rate = ship->turnRate;
            if (rate < 1.0f && ship->turnRate > 0.0f)
                rate = 1.0f;
            if (rate < 0.0f)
                rate = 0.0f;
            s->value = TruncF32(rate * 30.0f);
        }
        if (word == 396) {
            s->value = 0;
            s->num = IndString(kGameStrList, 396);
        } else {
            s->pre = IndString(kGameStrList, word) + " ";
            s->num = NumText(s->value);
        }
        break;
    case kStatShield:
    case kStatArmor:
        s->value = stat == kStatShield ? ship->shield : ship->armor;
        if (s->value > 0) {
            s->num = CommaNumber(s->value);
        } else {
            s->value = 0;
            s->num = IndString(kGameStrList, 335);
        }
        break;
    case kStatGuns:
    case kStatTurrets:
        s->value = stat == kStatGuns ? ship->maxGuns : ship->maxTurrets;
        if (s->value > 0) {
            s->pre = IndString(kGameStrList, 238) + " ";
            s->num = NumText(s->value);
        } else {
            s->value = 0;
            s->num = IndString(kGameStrList, 335);
        }
        break;
    case kStatSpace:
        s->value = ship->freeMass;
        freeMass = FreeMassAfterLoadout(type);
        s->num = NumText(freeMass);
        s->post = TonsText(freeMass);
        break;
    case kStatTotalSpace:
        s->value = ship->freeMass;
        s->num = NumText(s->value);
        s->post = TonsText(s->value);
        break;
    case kStatCargo:
        s->value = ship->cargo;
        s->num = NumText(ship->cargo);
        s->post = TonsText(ship->cargo);
        break;
    case kStatFuel:
        if (ship->fuel == 0) {
            s->num = IndString(kGameStrList, 335);
        } else {
            s->value = ship->fuel / 100;
            s->num = NumText(s->value);
            s->post = " " + IndString(kGameStrList, s->value == 1 ? 239 : 240);
        }
        break;
    case kStatLength:
        s->num = NumText(ship->shipLength);
        s->post = " " + IndString(kGameStrList, 244);
        break;
    case kStatMass:
        s->num = NumText(ship->mass);
        s->post = TonsText(ship->mass);
        break;
    case kStatCrew:
        s->value = ship->crew;
        s->num = NumText(ship->crew);
        break;
    }
}

/* Value line `stat' of ship `type' at (h, v), its text up to `right'. On a variant of ship `stock', a value
   whose text differs from the stock ship's shows the stock text struck through, then its own in green when
   better, red when worse; words both share are drawn once, or dropped when the line is too wide, and when
   it still is only the new value is shown. */
static void DrawStatLine(short stat, short type, short stock, short h, short v, short right)
{
    StatText now;
    StatText old;
    std::string nowText;
    std::string oldText;
    std::string text;
    const std::string *pre;
    const std::string *struck;
    const std::string *shown;
    const std::string *post;
    const std::string none;
    short left;
    Rect strike;

    StatOf(&now, stat, type, type);
    nowText = now.pre + now.num + now.post;
    MoveTo(h, v);
    ForeColor(whiteColor);
    if (stock != -1) {
        StatOf(&old, stat, stock, type);
        oldText = old.pre + old.num + old.post;
    }
    if (stock == -1 || oldText == nowText) {
        DrawCString(nowText);
        return;
    }

    if (old.pre == now.pre && old.post == now.post) {
        pre = &now.pre;
        struck = &old.num;
        shown = &now.num;
        post = &now.post;
    } else {
        pre = &none;
        struck = &oldText;
        shown = &nowText;
        post = &none;
    }
    text = *pre + *struck + " " + *shown + *post;
    if (TextWidthMacRoman(text.data(), text.size()) > right - h) {
        pre = &none;
        struck = &old.num;
        shown = &now.num;
        post = &none;
        text = *struck + " " + *shown;
        if (TextWidthMacRoman(text.data(), text.size()) > right - h)
            struck = NULL;
    }

    if (struck != NULL) {
        /* a 1 px bar through the middle of the digits, from the pen's start and the widths before it */
        left = h + TextWidthMacRoman(pre->data(), pre->size());
        text = *pre + *struck;
        SetRect(&strike, left, v - 4, h + TextWidthMacRoman(text.data(), text.size()), v - 3);
        DrawCString(*pre);
        DrawCString(*struck);
        PaintRect(&strike);
        DrawCString(" ");
    }
    if (stat == kStatLength || stat == kStatMass || now.value == old.value)
        ForeColor(whiteColor);
    else if (now.value > old.value)
        RGBForeColor(&betterRGB);
    else
        RGBForeColor(&worseRGB);
    DrawCString(*shown);
    ForeColor(whiteColor);
    DrawCString(*post);
}

// FUNCTION: EVNOVA 0x00495c80
// MAC: _ShipDescDialogUpdate i386 0x0004f915 ppc 0x00058710
void ShipDescDialogUpdate(void)
{
    short numWeapTypes;
    short drawn;
    short sel;
    Rect box;
    Boolean hasPict;
    std::string line;
    Str255 label;
    Rect box2;
    Handle item;
    short itemType;
    short i;
    short j;
    short stock;
    short valueRight;
    short valueLeft;
    short totalH;

    SetGWorldContainer(&loadPort);
    ForeColor(blackColor);
    PaintRect(GetDialogPortRect(shipDescDialog));
    if (shipDescBackgroundPic != NULL)
        DrawPicHandle(shipDescBackgroundPic, GetDialogPortRect(shipDescDialog));
    if (descResourcePictId >= kFirstResourceID)
        hasPict = 1;
    else
        hasPict = 0;
    ForeColor(blackColor);
    GetDialogItem(shipDescDialog, 3, &itemType, &item, &box);
    if (RectInRgn(&box, GetWindowVisRgn(FrontWindow(shipDescDialog)))) {
        TextFont(gTimesFont);
        TextSize(18);
        MoveTo(box.left, (box.top + 18));
        ForeColor(whiteColor);
        if (hasPict)
            DrawStringCentered(shipyardLongNames[selShipyardItem], box.left,
                               box.right, box.top + 18);
        else
            DrawStringCentered(ShipClassName(selShipyardItem), box.left,
                               box.right, box.top + 18);
        ForeColor(blackColor);
    }
    GetDialogItem(shipDescDialog, 5, &itemType, &item, &box);
    if (RectInRgn(&box, GetWindowVisRgn(FrontWindow(shipDescDialog)))) {
        sel = selShipyardItem;
        stock = StockShipOf(sel);
        valueRight = box.right - 2;
        valueLeft = box.left + 128;              /* 2 px before the right column's labels */
        ForeColor(blackColor);
        PaintRect(&box);
        ForeColor(whiteColor);
        TextFont(smallTextFont);
        TextSize(smallTextSize);

        RGBForeColor(&rgbMedGrey);
        MoveTo(box.left, (box.top + 12));
        DrawIndString(kGameStrList, 229);
        DrawStatLine(kStatSpeed, sel, stock, box.left + 45, box.top + 12, valueLeft);

        RGBForeColor(&rgbMedGrey);
        MoveTo(box.left, (box.top + 24));
        DrawIndString(kGameStrList, 230);
        DrawStatLine(kStatAccel, sel, stock, box.left + 45, box.top + 24, valueLeft);

        RGBForeColor(&rgbMedGrey);
        MoveTo(box.left, (box.top + 36));
        DrawIndString(kGameStrList, 231);
        DrawStatLine(kStatTurn, sel, stock, box.left + 45, box.top + 36, valueLeft);

        RGBForeColor(&rgbMedGrey);
        MoveTo(box.left, (box.top + 48));
        DrawCString(shieldString[0]);
        DrawStatLine(kStatShield, sel, stock, box.left + 45, box.top + 48, valueLeft);

        RGBForeColor(&rgbMedGrey);
        MoveTo(box.left, (box.top + 60));
        DrawCString(armorString[0]);
        DrawStatLine(kStatArmor, sel, stock, box.left + 45, box.top + 60, valueLeft);

        RGBForeColor(&rgbMedGrey);
        MoveTo(box.left, (box.top + 72));
        DrawIndString(kGameStrList, 232);
        DrawStatLine(kStatGuns, sel, stock, box.left + 45, box.top + 72, valueLeft);

        RGBForeColor(&rgbMedGrey);
        MoveTo(box.left, (box.top + 84));
        DrawIndString(kGameStrList, 233);
        DrawStatLine(kStatTurrets, sel, stock, box.left + 45, box.top + 84, valueLeft);

        MoveTo((box.left + 130), (box.top + 12));
        RGBForeColor(&rgbMedGrey);
        DrawIndString(kGameStrList, 234);
        DrawStatLine(kStatSpace, sel, stock, box.left + 175, box.top + 12, valueRight);

        RGBForeColor(&rgbMedGrey);
        MoveTo((box.left + 130), (box.top + 24));
        DrawCString(kTotalSpaceLabel);
        totalH = box.left + 130 + TextWidthMacRoman(kTotalSpaceLabel, strlen(kTotalSpaceLabel)) + 4;
        DrawStatLine(kStatTotalSpace, sel, stock, totalH > box.left + 175 ? totalH : box.left + 175, box.top + 24,
                     valueRight);

        RGBForeColor(&rgbMedGrey);
        MoveTo((box.left + 130), (box.top + 36));
        DrawIndString(kGameStrList, 110);
        DrawStatLine(kStatCargo, sel, stock, box.left + 175, box.top + 36, valueRight);

        RGBForeColor(&rgbMedGrey);
        MoveTo((box.left + 130), (box.top + 48));
        DrawCString(fuelString[4]);
        DrawStatLine(kStatFuel, sel, stock, box.left + 175, box.top + 48, valueRight);
        numWeapTypes = 0;
        for (i = 0; i < kMaxWeapTypes; i++) {
            if (shipClass[sel].weapCount[i] > 0)
                numWeapTypes++;
        }
        if (numWeapTypes == 0) {
            ForeColor(whiteColor);
            if (hasPict) {
                GetDialogItem(shipDescDialog, 8, &itemType, &item, &box2);
                MoveTo(box2.left, (box2.top + 12));
            } else {
                MoveTo(box.left, (box.top + 108));
            }
            DrawIndString(kGameStrList, 242);
        } else {
            ForeColor(whiteColor);
            drawn = 0;
            if (hasPict) {
                line.clear();
                GetDialogItem(shipDescDialog, 8, &itemType, &item, &box2);
                MoveTo(box2.left, (box2.top + 12));
                box2.top += 16;
            } else {
                MoveTo(box.left, (box.top + 108));
            }
            DrawIndString(kGameStrList, 241);
            for (i = 0; i < kMaxWeapTypes; i++) {
                if (shipClass[sel].weapCount[i] > 0) {
                    if (!hasPict)
                        MoveTo(box.left + 8, box.top + 120 + drawn * 12);
                    if (weaponStats[i].guidance != kGuidanceBay) {
                        if (hasPict) {
                            line += NumText(shipClass[sel].weapCount[i]);
                            line += " ";
                        } else {
                            DrawCString(NumText(shipClass[sel].weapCount[i]));
                            DrawCString(" ");
                        }
                        for (j = 0; j < kMaxOutfits; j++) {
                            if (outfitSystem[j].modType[0] == OutfMod::Weapon
                                && i == outfitSystem[j].modVal[0]) {
                                if (hasPict) {
                                    line += FromNameField(outfitSystem[j].name);
                                } else {
                                    DrawCString(OutfitName(j));
                                }
                                break;
                            }
                        }
                        if (shipClass[sel].weapCount[i] > 1) {
                            if (hasPict)
                                line += "s";
                            else
                                DrawCString("s");
                        }
                        if (shipClass[sel].weapAmmo[i] > 0) {
                            if (hasPict) {
                                line += TextF(" + %d %s", shipClass[sel].weapAmmo[i],
                                              IndString(kGameStrList, 243).c_str());
                            } else {
                                DrawCString(" + ");
                                DrawCString(NumText(shipClass[sel].weapAmmo[i]));
                                DrawCString(" ");
                                DrawIndString(kGameStrList, 243);
                            }
                        }
                    } else {
                        if (hasPict) {
                            line += NumText(shipClass[sel].weapAmmo[i]);
                            line += " ";
                        } else {
                            DrawCString(NumText(shipClass[sel].weapAmmo[i]));
                            DrawCString(" ");
                        }
                        for (j = 0; j < kMaxOutfits; j++) {
                            if (outfitSystem[j].modType[0] == OutfMod::Ammo
                                && i == outfitSystem[j].modVal[0]) {
                                if (hasPict) {
                                    line += FromNameField(outfitSystem[j].name);
                                } else {
                                    DrawCString(OutfitName(j));
                                }
                                break;
                            }
                        }
                        if (shipClass[sel].weapAmmo[i] > 1) {
                            if (hasPict)
                                line += "s";
                            else
                                DrawCString("s");
                        }
                    }
                    if (hasPict && numWeapTypes > 1
                        && drawn < numWeapTypes - 1)
                        line += ", ";
                    drawn++;
                }
            }
            if (hasPict) {
                ForeColor(blackColor);
                TETextBox(line.data(), line.size(), &box2, 0);
                InvertRect(&box2);
            }
        }

        RGBForeColor(&rgbMedGrey);
        MoveTo((box.left + 130), (box.top + 60));
        DrawIndString(kGameStrList, 235);
        DrawStatLine(kStatLength, sel, stock, box.left + 175, box.top + 60, valueRight);

        RGBForeColor(&rgbMedGrey);
        MoveTo((box.left + 130), (box.top + 72));
        DrawIndString(kGameStrList, 236);
        DrawStatLine(kStatMass, sel, stock, box.left + 175, box.top + 72, valueRight);

        if (shipClass[sel].crew > 0 || (stock != -1 && shipClass[stock].crew > 0)) {
            RGBForeColor(&rgbMedGrey);
            MoveTo((box.left + 130), (box.top + 84));
            DrawIndString(kGameStrList, 237);
            DrawStatLine(kStatCrew, sel, stock, box.left + 175, box.top + 84, valueRight);
        }
    }
    GetDialogItem(shipDescDialog, 1, &itemType, &item, &box);
    if (RectInRgn(&box, GetWindowVisRgn(FrontWindow(shipDescDialog)))) {
        GetIndString(label, kButtonStrList, 5);
        DrawButton(GetDialogPort(shipDescDialog), &box, label, 0, 0, 0);
    }
    if (bigShipPic != NULL) {
        GetDialogItem(shipDescDialog, 7, &itemType, &item, &box);
        if (RectInRgn(&box, GetWindowVisRgn(FrontWindow(shipDescDialog))))
            DrawPicHandle(bigShipPic, &box);
    }
    CopyLoadPortToDialog(shipDescDialog);
}

// FUNCTION: EVNOVA 0x00498dc0
// MAC: _CalcShipCanBuy i386 0x0004f5eb ppc 0x000582b4
void CalcShipCanBuy(void)
{
    int32_t price;
    int32_t need;
    int32_t tradeIn;

    if (shipBuyMode == 0) {
        if (selShipyardItem != -1) {
            if (shipData[0].type == selShipyardItem) {
                canBuy = 0;
            } else {
                price = ApplyPriceAndTechnologyFlux(
                            shipClass[selShipyardItem].cost, priceFluxMult[1],
                            shipClass[selShipyardItem].techLevel,
                            stellarData[shipData[0].targetStellar].techLevel);
                tradeIn = ApplyPriceAndTechnologyFlux(
                            PlayerShipTradeInPrice(), priceFluxMult[0],
                            shipClass[shipData[0].type].techLevel,
                            stellarData[shipData[0].targetStellar].techLevel);
                tradeIn = ApplyPriceAndTechnologyFlux(
                            tradeIn, priceFluxMult[1],
                            shipClass[shipData[0].type].techLevel,
                            stellarData[shipData[0].targetStellar].techLevel);
                need = price - tradeIn;
                if (need < 0)
                    need = 0;
                if (shipData[0].credits >= need)
                    canBuy = 1;
                else
                    canBuy = 0;
            }
        } else {
            canBuy = 0;
        }
        if (canBuy != 0) {
            if (PlayerMeetsRequirements(shipClass[selShipyardItem].requires[0],
                                        shipClass[selShipyardItem].requires[1]) == 0)
                canBuy = 0;
        }
    } else {
        if (selShipyardItem != -1) {
            need = ApplyPriceAndTechnologyFlux(
                        shipClass[selShipyardItem].cost, priceFluxMult[1],
                        shipClass[selShipyardItem].techLevel,
                        stellarData[shipData[0].targetStellar].techLevel);
            need = need * 0.1;
            if (shipData[0].credits >= need)
                canBuy = 1;
            else
                canBuy = 0;
        } else {
            canBuy = 0;
        }
    }
    if (canBuy != 0) {
        if (EvalMissionBitTestString(shipClass[selShipyardItem].availability) == 0)
            canBuy = 0;
    }
}
