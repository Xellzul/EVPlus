/* shipyardbuttons.cpp -- the head of the buttons TU (win 0x0049eef0..0x0049fbb0): the map's, the shipyard's and the outfitter's buttons. Notes: Research/notes/shipyardbuttons.md */
#include "dialogs.h"
#include "player.h"
#include "stellar.h"
#include "buttons.h"

#define kGridPageCells 20    /* the scroll-down guard's slack */
#define kGridScrollStep 4    /* ...and the scroll-up guard's */

/* button slots */
enum { kMapDoneButton, kMapZoomOutButton, kMapZoomInButton, kMapClearRouteButton,
       kMapInfluenceButton, kMapSearchButton };
enum { kShipyardDoneButton, kShipyardBuyButton, kShipyardInfoButton, kShipyardUpButton,
       kShipyardDownButton };
enum { kOutfitDoneButton, kOutfitBuyButton, kOutfitSellButton, kOutfitUpButton,
       kOutfitDownButton };

namespace {

/* entries of a grid list (-1 = empty) */
short CountListed(const short *list, short size)
{
    short count = 0;

    for (short i = 0; i < size; i++)
        if (list[i] != -1)
            count++;
    return count;
}

/* Buy is hit-tested before Done */
constexpr short kShipyardHitOrder[5] = { kShipyardBuyButton, kShipyardDoneButton, kShipyardInfoButton,
                                         kShipyardUpButton, kShipyardDownButton };

} // namespace

MapButtons::MapButtons() : ButtonBar(mapDialog, 6, HitRule::LastWins) {}

void MapButtons::Update(short)
{
    Set(kMapDoneButton, 1, buttonLabel[kDoneButtonLabel]);
    Set(kMapZoomOutButton, 4, buttonLabel[kMinusButtonLabel], Lit(canZoomOut != 0));
    Set(kMapZoomInButton, 5, buttonLabel[kPlusButtonLabel], Lit(canZoomIn != 0));
    Set(kMapClearRouteButton, 8, buttonLabel[kClearRouteButtonLabel], Lit(showClearRouteButton != 0));
    Set(kMapInfluenceButton, 9,
        buttonLabel[settings.showMapBorders != 0 ? kHideBordersButtonLabel : kShowBordersButtonLabel]);
    Set(kMapSearchButton, 0xa, buttonLabel[kFindButtonLabel]);
    SetPress(kMapZoomOutButton, canZoomOut != 0, canZoomOut != 0);
    SetPress(kMapZoomInButton, canZoomIn != 0, canZoomIn != 0);
    SetPress(kMapClearRouteButton, showClearRouteButton != 0, showClearRouteButton != 0);
}

ShipyardButtons::ShipyardButtons() : ButtonBar(shipyardDialog, 5, HitRule::LastWins, kShipyardHitOrder) {}

void ShipyardButtons::Update(short)
{
    bool enabled[5] = { true, canBuy != 0, selShipyardItem != -1, shipyardScroll >= kGridScrollStep,
                        shipyardScroll < CountListed(dockedPortShipTypes, kMaxShipTypes) - kGridPageCells };

    Set(kShipyardDoneButton, 7, buttonLabel[kDoneButtonLabel], Lit(enabled[0]));
    Set(kShipyardBuyButton, 1, buttonLabel[shipBuyMode == 0 ? kBuyShipButtonLabel : kHireEscortButtonLabel],
        Lit(enabled[1]));
    Set(kShipyardInfoButton, 0xa, buttonLabel[kInfoButtonLabel], Lit(enabled[2]));
    Set(kShipyardUpButton, 0xc, buttonLabel[kScrollUpButtonLabel], Lit(enabled[3]));
    Set(kShipyardDownButton, 0xd, buttonLabel[kScrollDownButtonLabel], Lit(enabled[4]));
    for (short i = 0; i < 5; i++)
        SetPress(i, enabled[i], enabled[i]);
}

OutfitButtons::OutfitButtons() : ButtonBar(outfitDialog, 5, HitRule::LastWins) {}

void OutfitButtons::Update(short)
{
    bool enabled[5] = { true, canBuy != 0, canSell != 0, outfitScroll >= kGridScrollStep,
                        outfitScroll < CountListed(dockedPortItems, kMaxOutfits) - kGridPageCells };

    Set(kOutfitDoneButton, 1, buttonLabel[kDoneButtonLabel], Lit(enabled[0]));
    Set(kOutfitBuyButton, 7, buttonLabel[kBuyButtonLabel],
        portBuysNoOutfits != 0 ? ButtonLook::Hidden : Lit(enabled[1]));
    Set(kOutfitSellButton, 4, buttonLabel[kSellButtonLabel], Lit(enabled[2]));
    Set(kOutfitUpButton, 0xa, buttonLabel[kScrollUpButtonLabel], Lit(enabled[3]));
    Set(kOutfitDownButton, 0xb, buttonLabel[kScrollDownButtonLabel], Lit(enabled[4]));
    for (short i = 0; i < 5; i++)
        SetPress(i, true, enabled[i]);
}


