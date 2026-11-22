/* portbuttons.cpp -- the spaceport's buttons, PlanetButtons. Notes: Research/notes/portbuttons.md, Research/notes/trackplanetbuttons.md */

#include "dialogs.h"
#include "game.h"
#include "ships.h"
#include "stellar.h"
#include "buttons.h"


PlanetButtons::PlanetButtons() : ButtonBar(portDialog, kPortButtons, HitRule::LastWins) {}

void PlanetButtons::Update(short)
{
    static_assert(kPortButtons <= kMaxButtons, "one slot per spaceport button");
    static constexpr short kItems[kPortButtons] = { 0xc, 4, 7, 8, 9, 0xa, 0xb };
    static constexpr short kLabels[kPortButtons] = { kLeaveButtonLabel, kRechargeButtonLabel,
                                                     kTradeCenterButtonLabel, kOutfitterButtonLabel,
                                                     kShipyardButtonLabel, kMissionBBSButtonLabel,
                                                     kBarButtonLabel };
    Boolean noShips = (dockedPort->flags & kSpobUninhabited);
    bool live[kPortButtons] = {
        true,
        shipData->credits > 0 && noShips == 0 && shipData->fuel < shipData->FuelCapacity(),
        (dockedPort->flags & kSpobTradeCenter) != 0,
        (dockedPort->flags & kSpobHasOutfitter) != 0,
        (dockedPort->flags & kSpobHasShipyard) != 0,
        canDoMissions != 0,
        (dockedPort->flags & kSpobHasBar) != 0,
    };

    for (short i = 0; i < kPortButtons; i++) {
        Set(i, kItems[i], buttonLabel[kLabels[i]], live[i] ? ButtonLook::Normal : ButtonLook::Disabled);
        SetPress(i, live[i], live[i]);
    }
}

