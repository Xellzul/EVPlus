/* shipyarddlg.cpp -- the shipyard dialog and the ship description */
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
#include "buttons.h"

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

EXTERN_C_BEGIN

#define kShipDescBase 13000    /* desc ID of ship 0's shipyard blurb */
#define kHireDescBase 14000     /* ...and of its hire-an-escort blurb */

/* the item grid: four across, five down, out of item 5's box */
#define kMovieItem    8


#define kShipyardPictID 0x2135

/* per-mission state, stride 0x14; buttons.cpp's MissionState only names +0x00 */

// FUNCTION: EVNOVA 0x00492f30
// MAC: _DoShipyardDialog i386 0x0005e679 ppc 0x00066438
void DoShipyardDialog(void)
{
    void *item;
    short halfHeight;
    Rect box;
    short itemHit;
    unsigned char msgText[256];
    int hireCost;
    unsigned char defaultName[256];
    short i;
    short itemType;
    short outf;
    int32_t tradeIn;
    int32_t cost;
    short weap;
    Boolean weapHasSlot;
    short savedTargetStellar;
    short n;
    short savedUnk6A;
    Boolean confirmed;
    Boolean done;
    char digitBuf[256];

    SetupPortAvailableShipTypes(dockedPort);
    done = 1;
    for (weap = 0; weap < kMaxShipTypes; weap++) {
        if (dockedPortShipTypes[weap] != -1) {
            done = 0;
            break;
        }
    }
    if (done) {
        strcpy(textShowString, "");
        if (shipBuyMode == 0)
            StrcatIndCString(textShowString, kGameStrList, 0xdf);
        else
            StrcatIndCString(textShowString, kGameStrList, 0xe0);
        ShowTextDialog(0, 0, 0);
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
    Dialog dialog;
    dialog.Open(0x3ec, ShipyardDialogUpdate, ShipyardFilter, &shipyardDialog, DialogClose::NoFlash);
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

    do {
        if (gQuit)
            done = 1;
        itemHit = dialog.Modal();
        if (itemHit == 1) {
            if (selShipyardItem != -1) {
                n = selShipyardItem;
                if (canBuy != 0) {
                    if (shipBuyMode == 0) {

                        RedrawDialog(shipyardDialog);
                        strcpy((char *)msgText, "");
                        StrcatIndCString((char *)msgText, kGameStrList, 0x78);
                        strcat((char *)msgText, " ");
                        strncpy((char *)defaultName, (char *)shipyardLongNames[n], 200);
                        PtoCstr(defaultName);
                        strcat((char *)msgText, (char *)defaultName);
                        strcat((char *)msgText, ": ");
                        CtoPstr((char *)msgText);
                        pstrcpy(defaultName,
                               shipClass[n].name);
                        PtoCstr(defaultName);
                        strcat((char *)defaultName, " ");
                        for (weap = 0; weap <= 2; weap++) {
                            NumToString(Rand(9) + 1, (unsigned char *)digitBuf);
                            PtoCstr((unsigned char *)digitBuf);
                            strcat((char *)defaultName, digitBuf);
                        }
                        CtoPstr((char *)defaultName);
                        confirmed = EVTextInputDialog(msgText,
                                                      defaultName, 0x40);
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
                                    shipData[weap].aiType == kShipTypeFighter)
                                    shipData[weap].active = 0;
                            }
                            for (weap = 0; weap < kMaxWeapTypes; weap++) {
                                weapHasSlot = 0;
                                for (outf = 0; outf < kMaxOutfits; outf++) {
                                    if (outfitSystem[outf].modType[0] == kOutfModWeapon &&
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
                            shipData->shield = ShipShieldCapacity(shipData);
                            shipData->armor = ShipArmorCapacity(shipData);
                            shipData->fuel = ShipFuelCapacity(shipData);
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
        if (itemHit == 3) {
            savedTargetStellar = shipData->targetStellar;
            savedUnk6A = shipData->navTargetKind;
            hypergateMapMode = 0;
            DoSystemMap();
            shipData->targetStellar = savedTargetStellar;
            shipData->navTargetKind = savedUnk6A;
            SetDialogGWorld(portDialog);
            RedrawDialog(portDialog);
            UpdateAllPortDialogs();
            SetDialogGWorld(shipyardDialog);
            RedrawDialog(shipyardDialog);
        }
        if (itemHit == 4) {
            DoPlayerInfoDialog();
            SetDialogGWorld(portDialog);
            RedrawDialog(portDialog);
            UpdateAllPortDialogs();
            SetDialogGWorld(shipyardDialog);
            RedrawDialog(shipyardDialog);
        }
        if (itemHit == 8 && shipBuyMode == 0) {
            if (OfferMissionFromPort(5) != 0) {
                SetDialogGWorld(portDialog);
                UpdateAllPortDialogs();
                SetDialogGWorld(shipyardDialog);
                ShipyardDialogUpdate();
            }
        }
        if (itemHit == 0xb) {
            n = 0;
            for (weap = 0; weap < kMaxActiveMissions; weap++) {
                if (currentMissionObjectives[weap].active != 0 &&
                    (currentMissionObjectives[weap].flags & kMissionHidden) == 0)
                    n++;
            }
            if (n > 0) {
                DoMissionInfoDialog();
                SetDialogGWorld(portDialog);
                RedrawDialog(portDialog);
                PortDialogUpdate();
                SetDialogGWorld(shipyardDialog);
                RedrawDialog(shipyardDialog);
            }
        }
        if (itemHit == 10 && selShipyardItem != -1) {
            ShipyardDialogUpdate();
            DoShipyardDescription(selShipyardItem);
            SetDialogGWorld(shipyardDialog);
            RedrawDialog(shipyardDialog);
        }
        if (itemHit == 0xc && shipyardScroll >= kGridCols) {
            shipyardScroll -= kGridCols;
            selShipyardItem = selShipListItem = -1;
            strncpy(dialogDescTextPtr, "", 3);
            EndLoopingMovie();
            RedrawDialog(shipyardDialog);
        }
        if (itemHit == 0xd) {
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
        if (itemHit == 7)
            done = 1;
    } while (done == 0);

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

// FUNCTION: EVNOVA 0x00493fc0
// MAC: _ShipyardFilter i386 0x00060449 ppc 0x00068154
Boolean ShipyardFilter(DialogPtr unused, EventRecord *ev, short *itemHit)
{
    Rect box;
    Rect movieBox;
    short itemType;
    Handle item;
    Point where;
    EventKey c;
    short i;
    short count;
    int32_t slot;
    short old;
    short oldPos;
    short row;
    short button;
    int32_t cell;

    if ((absquatulateStr)[0] != 0) {
        ev->what = kMouseDown;
        *itemHit = 7;
        return 1;
    }
    LoopingMovieIdle();
    *itemHit = -1;
    CalcShipCanBuy();
    if ((int)ev->what == kKeyDown || (int)ev->what == kAutoKey) {
        c = EventCharOf(ev);
        if ((int)ev->what == kKeyDown) {
            if (IsReturnChar(c) || c == kEscapeChar) {
                ev->what = kMouseDown;
                *itemHit = 7;
                return 1;
            }
            c = tolower(c);
            if ((c == 'b' && shipBuyMode == 0) || (c == 'h' && shipBuyMode == 1)) {
                ev->what = kMouseDown;
                *itemHit = 1;
                return 1;
            }
            if (c == ' ') {
                ev->what = kMouseDown;
                *itemHit = 10;
                return 1;
            }
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
                        shipMovieFilenames[selShipyardItem][0] != 0) {
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
    if ((int)ev->what == kKeyDown || (int)ev->what == kAutoKey) {
        QuickKeyCheckHandler();
        if (QuickKeyCheck(keyPrefs.map) && mapDialog == NULL) {
            *itemHit = 3;
            return 1;
        }
        if (QuickKeyCheck(keyPrefs.playerInfo) && infoDialog == NULL) {
            *itemHit = 4;
            return 1;
        }
        if (QuickKeyCheck(keyPrefs.missions) && missionInfoDialog == NULL) {
            *itemHit = 11;
            return 1;
        }
        c = tolower(EventCharOf(ev));
        return 0;
    }
    if ((int)ev->what == kMouseDown) {
        where = ev->where;
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
                        shipMovieFilenames[selShipyardItem][0] != 0) {
                        GetDialogItem(shipyardDialog, kMovieItem, &itemType,
                                      &item, &movieBox);
                        StartLoopingMovie(shipMovieFilenames[selShipyardItem],
                                          FrontWindow(shipyardDialog),
                                          &movieBox);
                    }
                }
                if (selShipListItem == lastClickItem && selShipListItem != -1 &&
                    ev->when - lastClickTime <= 15) {
                    ShipyardDialogUpdate();
                    DoShipyardDescription(selShipyardItem);
                    SetDialogGWorld(shipyardDialog);
                    RedrawDialog(shipyardDialog);
                }
                lastClickItem = selShipListItem;
                lastClickTime = ev->when;
            }
        }
        ShipyardDialogUpdate();
        button = ShipyardButtons().Track(where);
        if (button == 0) {
            *itemHit = 7;
            return 1;
        }
        if (button == 1) {
            *itemHit = 1;
            return 1;
        }
        if (button == 2) {
            *itemHit = 10;
            return 1;
        }
        if (button == 3) {
            *itemHit = 12;
            return 1;
        }
        if (button == 4) {
            *itemHit = 13;
            return 1;
        }
        *itemHit = -1;
        return 1;
    }
    if (shipBuyMode == 0) {
        if (ReadTick() >= missionOfferTime) {
            *itemHit = 8;
            missionOfferTime = ReadTick() + Rand(30) + 30;
            return 1;
        }
    }
    if ((int)ev->what == kUpdateEvt) {
        ShipyardDialogUpdate();
        return 0;
    }
    return 0;
}

// FUNCTION: EVNOVA 0x004948b0
// MAC: _ShipyardDialogUpdate i386 0x00058a10 ppc 0x00060d04
void ShipyardDialogUpdate(void)
{
    int c;
    short i;
    Handle item;
    short itemType;
    unsigned char line1[256];
    Rect box;
    unsigned char line2[256];
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
                        NumToString(dockedPortShipTypes[i] + kFirstResourceID,
                                    line1);
                        ForeColor(whiteColor);
                        MoveTo(((listScreenRect[i - shipyardScroll].left
                                        + listScreenRect[i - shipyardScroll].right
                                        - StringWidth(line1)) / 2),
                               (listScreenRect[i - shipyardScroll].top + 24));
                        DrawString(line1);
                        ForeColor(blackColor);
                    }
                    if (SplitTwoLineString(shipyardShortNames[dockedPortShipTypes[i]],
                                           line1, line2)) {
                        c = line1[1];
                        if (isalpha(HeldChar(c, line1[1])) || isdigit(HeldChar(c, line1[1])))
                            ForeColor(whiteColor);
                        else
                            RGBForeColor(&rgbLessDarkGrey);
                        MoveTo((listScreenRect[i - shipyardScroll].left
                                       + listScreenRect[i - shipyardScroll].right) / 2
                                      - StringWidth(line1) / 2,
                                  listScreenRect[i - shipyardScroll].bottom - 14);
                        DrawString(line1);
                        c = line2[1];
                        if (isalpha(HeldChar(c, line2[1])) || isdigit(HeldChar(c, line2[1])))
                            ForeColor(whiteColor);
                        else
                            RGBForeColor(&rgbLessDarkGrey);
                        MoveTo((listScreenRect[i - shipyardScroll].left
                                       + listScreenRect[i - shipyardScroll].right) / 2
                                      - StringWidth(line2) / 2,
                                  listScreenRect[i - shipyardScroll].bottom - 3);
                        DrawString(line2);
                        ForeColor(blackColor);
                    } else {
                        MoveTo((listScreenRect[i - (int32_t)shipyardScroll].left
                                       + listScreenRect[i - (int32_t)shipyardScroll].right) / 2
                                      - StringWidth(line1) / 2,
                                  listScreenRect[i - (int32_t)shipyardScroll].bottom - 6);
                        ForeColor(whiteColor);
                        DrawString(line1);
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
                    GetIndString(line1, kGameStrList, 0xd5);
                    DrawStringCentered(line1, box.left, box.right,
                                       ((box.bottom + box.top) / 2 - 6));
                    GetIndString(line1, kGameStrList, 0xd6);
                    DrawStringCentered(line1, box.left, box.right,
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
                DrawString("\p ");
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
                DrawString("\p ");
                DrawIndString(kGameStrList, 0x22);
                net = price - tradeIn;
                if (net < 0)
                    net = 0;
                MoveTo(box.left, (box.top + 0x30));
                DrawIndString(kGameStrList, 0xe3);
                MoveTo((box.left + 0x46), (box.top + 0x30));
                DrawCommaNumber(net);
                DrawString("\p ");
                DrawIndString(kGameStrList, 0x22);
                MoveTo(box.left, (box.top + 0x48));
                DrawIndString(kGameStrList, 0xd9);
                MoveTo((box.left + 0x46), (box.top + 0x48));
                DrawCommaNumber(shipData->credits);
                DrawString("\p ");
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
                DrawString("\p ");
                DrawIndString(kGameStrList, 0x22);
                MoveTo(box.left, (box.top + 0x24));
                DrawIndString(kGameStrList, 0xd9);
                MoveTo((box.left + 0x46), (box.top + 0x24));
                DrawCommaNumber(shipData->credits);
                DrawString("\p ");
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

// FUNCTION: EVNOVA 0x004956a0
// MAC: _DoShipyardDescription i386 0x0005b6d2 ppc 0x00063764
void DoShipyardDescription(short ship)
{
    Boolean done;
    short itemHit;
    short savedStellar;
    short savedRoute;
    short i;
    short shown;

    done = 0;
    Dialog dialog;
    Picture bigShip(bigShipPic);
    bigShipPic = NULL;
    if (descResourcePictId >= kFirstResourceID)
        bigShip.Load(descResourcePictId);
    dialog.Open(bigShipPic != NULL ? kDescDlogID : kPlainDescDlogID, ShipDescDialogUpdate, ShipDescFilter,
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
    do {
        if (gQuit)
            done = 1;
        itemHit = dialog.Modal();
        if (itemHit == 1)
            done = 1;
        if (itemHit == 4) {
            savedStellar = shipData->targetStellar;
            savedRoute = shipData->navTargetKind;
            hypergateMapMode = 0;
            DoSystemMap();
            shipData->targetStellar = savedStellar;
            shipData->navTargetKind = savedRoute;
            SetDialogGWorld(portDialog);
            RedrawDialog(shipDescDialog);
            PortDialogUpdate();
            SetDialogGWorld(shipDescDialog);
            RedrawDialog(shipDescDialog);
        }
        if (itemHit == 2) {
            DoPlayerInfoDialog();
            SetDialogGWorld(portDialog);
            RedrawDialog(portDialog);
            PortDialogUpdate();
            SetDialogGWorld(shipDescDialog);
            RedrawDialog(shipDescDialog);
        }
        if (itemHit == 6) {
            shown = 0;
            for (i = 0; i < kMaxActiveMissions; i++) {
                if (currentMissionObjectives[i].active != 0
                    && (currentMissionObjectives[i].flags & kMissionHidden) == 0)
                    shown++;
            }
            if (shown > 0) {
                DoMissionInfoDialog();
                SetDialogGWorld(portDialog);
                RedrawDialog(portDialog);
                PortDialogUpdate();
                SetDialogGWorld(shipDescDialog);
                RedrawDialog(shipDescDialog);
            }
        }
    } while (done == 0);
    dialog.Close();
    backdrop.Release();
    bigShip.Release();
}

// FUNCTION: EVNOVA 0x004959d0
// MAC: _ShipDescFilter i386 0x000510e5 ppc 0x00059a0c
Boolean ShipDescFilter(DialogPtr unused, EventRecord *ev, short *itemHit)
{
    short itemType;
    void *item;
    unsigned char label[256];
    Rect box;
    Point where;
    short lastHit;
    short hit;
    EventChar c;

    if ((int)ev->what == kKeyDown) {
        QuickKeyCheckHandler();
        if (QuickKeyCheck(keyPrefs.map) && mapDialog == NULL) {
            *itemHit = 4;
            return 1;
        }
        if (QuickKeyCheck(keyPrefs.playerInfo) && infoDialog == NULL) {
            *itemHit = 2;
            return 1;
        }
        if (QuickKeyCheck(keyPrefs.missions) && missionInfoDialog == NULL) {
            *itemHit = 6;
            return 1;
        }
        c = EventCharOf(ev);
        c = tolower(c);
        if (IsReturnChar(c) || c == kEscapeChar || c == ' ') {
            ev->what = kMouseDown;
            *itemHit = 1;
            return 1;
        }
    }
    if ((int)ev->what == kUpdateEvt) {
        DrawDialog(shipDescDialog);
        ShipDescDialogUpdate();
        return 0;
    }
    if ((int)ev->what == kMouseDown) {
        where = ev->where;
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

// FUNCTION: EVNOVA 0x00495c80
// MAC: _ShipDescDialogUpdate i386 0x0004f915 ppc 0x00058710
void ShipDescDialogUpdate(void)
{
    short numWeapTypes;
    short drawn;
    short sel;
    Rect box;
    Boolean hasPict;
    char line[1024];
    unsigned char buf[256];
    Rect box2;
    Handle item;
    short itemType;
    short i;
    short j;
    int32_t freeMass;
    int32_t mass;

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
            DrawStringCentered(shipClass[selShipyardItem].name, box.left,
                               box.right, box.top + 18);
        ForeColor(blackColor);
    }
    GetDialogItem(shipDescDialog, 5, &itemType, &item, &box);
    if (RectInRgn(&box, GetWindowVisRgn(FrontWindow(shipDescDialog)))) {
        sel = selShipyardItem;
        ForeColor(blackColor);
        PaintRect(&box);
        ForeColor(whiteColor);
        TextFont(smallTextFont);
        TextSize(smallTextSize);

        RGBForeColor(&rgbMedGrey);
        MoveTo(box.left, (box.top + 12));
        DrawIndString(kGameStrList, 229);
        ForeColor(whiteColor);
        MoveTo((box.left + 45), (box.top + 12));
        NumToString(CE_RoundD(shipClass[sel].maxSpeed * 100.0), buf);
        DrawString(buf);

        RGBForeColor(&rgbMedGrey);
        MoveTo(box.left, (box.top + 24));
        DrawIndString(kGameStrList, 230);
        ForeColor(whiteColor);
        MoveTo((box.left + 45), (box.top + 24));
        if (shipClass[sel].accel > 0.073)
            DrawIndString(kGameStrList, 245);
        else if (shipClass[sel].accel > 0.055)
            DrawIndString(kGameStrList, 246);
        else if (shipClass[sel].accel > 0.038)
            DrawIndString(kGameStrList, 247);
        else if (shipClass[sel].accel > 0.025)
            DrawIndString(kGameStrList, 248);
        else if (shipClass[sel].accel > 0.013)
            DrawIndString(kGameStrList, 249);
        else if (shipClass[sel].accel > 0.0)
            DrawIndString(kGameStrList, 250);
        else
            DrawIndString(kGameStrList, 396);

        RGBForeColor(&rgbMedGrey);
        MoveTo(box.left, (box.top + 36));
        DrawIndString(kGameStrList, 231);
        ForeColor(whiteColor);
        MoveTo((box.left + 45), (box.top + 36));
        if (shipClass[sel].turnRate <= 0.0f)
            DrawIndString(kGameStrList, 396);
        else if (shipClass[sel].turnRate <= 1.0f)
            DrawIndString(kGameStrList, 250);
        else if (shipClass[sel].turnRate <= 2.0f)
            DrawIndString(kGameStrList, 249);
        else if (shipClass[sel].turnRate <= 3.0f)
            DrawIndString(kGameStrList, 248);
        else if (shipClass[sel].turnRate <= 4.0f)
            DrawIndString(kGameStrList, 247);
        else if (shipClass[sel].turnRate <= 5.0f)
            DrawIndString(kGameStrList, 246);
        else
            DrawIndString(kGameStrList, 245);

        RGBForeColor(&rgbMedGrey);
        MoveTo(box.left, (box.top + 48));
        DrawString(shieldString[0]);
        ForeColor(whiteColor);
        MoveTo((box.left + 45), (box.top + 48));
        if (shipClass[sel].shield > 0)
            DrawCommaNumber(shipClass[sel].shield);
        else
            DrawIndString(kGameStrList, 335);

        RGBForeColor(&rgbMedGrey);
        MoveTo(box.left, (box.top + 60));
        DrawString(armorString[0]);
        ForeColor(whiteColor);
        MoveTo((box.left + 45), (box.top + 60));
        if (shipClass[sel].armor > 0)
            DrawCommaNumber(shipClass[sel].armor);
        else
            DrawIndString(kGameStrList, 335);

        RGBForeColor(&rgbMedGrey);
        MoveTo(box.left, (box.top + 72));
        DrawIndString(kGameStrList, 232);
        ForeColor(whiteColor);
        MoveTo((box.left + 45), (box.top + 72));
        if (shipClass[sel].maxGuns > 0) {
            NumToString(shipClass[sel].maxGuns, buf);
            DrawIndString(kGameStrList, 238);
            DrawString("\p ");
            DrawString(buf);
        } else
            DrawIndString(kGameStrList, 335);

        RGBForeColor(&rgbMedGrey);
        MoveTo(box.left, (box.top + 84));
        DrawIndString(kGameStrList, 233);
        ForeColor(whiteColor);
        MoveTo((box.left + 45), (box.top + 84));
        if (shipClass[sel].maxTurrets > 0) {
            NumToString(shipClass[sel].maxTurrets, buf);
            DrawIndString(kGameStrList, 238);
            DrawString("\p ");
            DrawString(buf);
        } else
            DrawIndString(kGameStrList, 335);

        freeMass = shipClass[sel].freeMass;
        for (i = 0; i < kMaxWeapTypes; i++) {
            if (shipClass[sel].weapCount[i] > 0) {
                for (j = 0; j < kMaxOutfits; j++) {
                    if (outfitSystem[j].modType[0] == kOutfModWeapon
                        && i == outfitSystem[j].modVal[0]
                        && outfitSystem[j].techLevel < kOutfTechAbsent) {
                        mass = AdjustedItemMass(&outfitSystem[j], &shipClass[sel]);
                        mass = shipClass[sel].weapCount[i] * mass;
                        if (mass > freeMass)
                            freeMass = 0;
                        else
                            freeMass -= mass;
                        break;
                    }
                }
            }
            if (shipClass[sel].weapAmmo[i] > 0) {
                for (j = 0; j < kMaxOutfits; j++) {
                    if (outfitSystem[j].modType[0] == kOutfModAmmo
                        && i == outfitSystem[j].modVal[0]
                        && outfitSystem[j].techLevel < kOutfTechAbsent) {
                        mass = AdjustedItemMass(&outfitSystem[j], &shipClass[sel]);
                        mass = shipClass[sel].weapAmmo[i] * mass;
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
            if (shipClass[sel].defOutfID[i] != -1
                && shipClass[sel].defOutfCount[i] > 0
                && outfitSystem[shipClass[sel].defOutfID[i]].techLevel < kOutfTechAbsent) {
                mass = AdjustedItemMass(&outfitSystem[shipClass[sel].defOutfID[i]],
                                        &shipClass[sel]);
                mass = shipClass[sel].defOutfCount[i] * mass;
                if (mass > freeMass)
                    freeMass = 0;
                else
                    freeMass -= mass;
            }
        }

        MoveTo((box.left + 130), (box.top + 12));
        RGBForeColor(&rgbMedGrey);
        DrawIndString(kGameStrList, 234);
        ForeColor(whiteColor);
        MoveTo((box.left + 175), (box.top + 12));
        NumToString(freeMass, buf);
        DrawString(buf);
        DrawString("\p ");
        if (freeMass == 1)
            DrawString(tonString);
        else
            DrawString(tonsString);

        RGBForeColor(&rgbMedGrey);
        MoveTo((box.left + 130), (box.top + 24));
        DrawIndString(kGameStrList, 110);
        ForeColor(whiteColor);
        MoveTo((box.left + 175), (box.top + 24));
        NumToString(shipClass[sel].cargo, buf);
        DrawString(buf);
        DrawString("\p ");
        if (shipClass[sel].cargo == 1)
            DrawString(tonString);
        else
            DrawString(tonsString);

        RGBForeColor(&rgbMedGrey);
        MoveTo((box.left + 130), (box.top + 36));
        DrawString(fuelString[4]);
        ForeColor(whiteColor);
        MoveTo((box.left + 175), (box.top + 36));
        if (shipClass[sel].fuel == 0)
            DrawIndString(kGameStrList, 335);
        else {
            numWeapTypes = shipClass[sel].fuel / 100;
            NumToString(numWeapTypes, buf);
            DrawString(buf);
            DrawString("\p ");
            if (numWeapTypes == 1)
                DrawIndString(kGameStrList, 239);
            else
                DrawIndString(kGameStrList, 240);
        }

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
                strcpy(line, "");
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
                        NumToString(shipClass[sel].weapCount[i], buf);
                        if (hasPict) {
                            PtoCstr(buf);
                            strcat(line, (char *)buf);
                            strcat(line, " ");
                        } else {
                            DrawString(buf);
                            DrawString("\p ");
                        }
                        for (j = 0; j < kMaxOutfits; j++) {
                            if (outfitSystem[j].modType[0] == kOutfModWeapon
                                && i == outfitSystem[j].modVal[0]) {
                                if (hasPict) {
                                    pstrcpy(buf, outfitSystem[j].name);
                                    PtoCstr(buf);
                                    strcat(line, (char *)buf);
                                } else {
                                    DrawString(outfitSystem[j].name);
                                }
                                break;
                            }
                        }
                        if (shipClass[sel].weapCount[i] > 1) {
                            if (hasPict)
                                strcat(line, "s");
                            else
                                DrawString("\ps");
                        }
                        if (shipClass[sel].weapAmmo[i] > 0) {
                            NumToString(shipClass[sel].weapAmmo[i], buf);
                            if (hasPict) {
                                strcat(line, " + ");
                                PtoCstr(buf);
                                strcat(line, (char *)buf);
                                strcat(line, " ");
                                StrcatIndCString(line, kGameStrList, 243);
                            } else {
                                DrawString("\p + ");
                                DrawString(buf);
                                DrawString("\p ");
                                DrawIndString(kGameStrList, 243);
                            }
                        }
                    } else {
                        NumToString(shipClass[sel].weapAmmo[i], buf);
                        if (hasPict) {
                            PtoCstr(buf);
                            strcat(line, (char *)buf);
                            strcat(line, " ");
                        } else {
                            DrawString(buf);
                            DrawString("\p ");
                        }
                        for (j = 0; j < kMaxOutfits; j++) {
                            if (outfitSystem[j].modType[0] == kOutfModAmmo
                                && i == outfitSystem[j].modVal[0]) {
                                if (hasPict) {
                                    pstrcpy(buf, outfitSystem[j].name);
                                    PtoCstr(buf);
                                    strcat(line, (char *)buf);
                                } else {
                                    DrawString(outfitSystem[j].name);
                                }
                                break;
                            }
                        }
                        if (shipClass[sel].weapAmmo[i] > 1) {
                            if (hasPict)
                                strcat(line, "s");
                            else
                                DrawString("\ps");
                        }
                    }
                    if (hasPict && numWeapTypes > 1
                        && drawn < numWeapTypes - 1)
                        strcat(line, ", ");
                    drawn++;
                }
            }
            if (hasPict) {
                ForeColor(blackColor);
                TETextBox(line, strlen(line), &box2, 0);
                InvertRect(&box2);
            }
        }

        RGBForeColor(&rgbMedGrey);
        MoveTo((box.left + 130), (box.top + 48));
        DrawIndString(kGameStrList, 235);
        ForeColor(whiteColor);
        MoveTo((box.left + 175), (box.top + 48));
        NumToString(shipClass[sel].shipLength, buf);
        DrawString(buf);
        DrawString("\p ");
        DrawIndString(kGameStrList, 244);

        RGBForeColor(&rgbMedGrey);
        MoveTo((box.left + 130), (box.top + 60));
        DrawIndString(kGameStrList, 236);
        ForeColor(whiteColor);
        MoveTo((box.left + 175), (box.top + 60));
        NumToString(shipClass[sel].mass, buf);
        DrawString(buf);
        DrawString("\p ");
        if (shipClass[sel].mass == 1)
            DrawString(tonString);
        else
            DrawString(tonsString);

        if (shipClass[sel].crew > 0) {
            RGBForeColor(&rgbMedGrey);
            MoveTo((box.left + 130), (box.top + 72));
            DrawIndString(kGameStrList, 237);
            ForeColor(whiteColor);
            MoveTo((box.left + 175), (box.top + 72));
            NumToString(shipClass[sel].crew, buf);
            DrawString(buf);
        }
    }
    GetDialogItem(shipDescDialog, 1, &itemType, &item, &box);
    if (RectInRgn(&box, GetWindowVisRgn(FrontWindow(shipDescDialog)))) {
        GetIndString(buf, kButtonStrList, 5);
        DrawButton(GetDialogPort(shipDescDialog), &box, buf, 0, 0, 0);
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

EXTERN_C_END
