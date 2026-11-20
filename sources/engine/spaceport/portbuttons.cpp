/* portbuttons.cpp -- the spaceport's button pair, TrackPlanetButtons and DrawPlanetButtons. Notes: Research/notes/portbuttons.md, Research/notes/trackplanetbuttons.md */
#include <stddef.h>

#include "macport.h"
#include "shipobj.h"
#include "resources.h"
#include "gameconst.h"
#include "dialogs.h"
#include "game.h"
#include "ships.h"
#include "gametypes.h"
#include "stellar.h"
#include "buttonbar.h"


namespace {

/* a closed service's button is not drawn but still covers its place */
class PlanetButtons final : public ButtonBar {
public:
    static_assert(kPortButtons <= kMaxButtons, "one slot per spaceport button");
    PlanetButtons() : ButtonBar(portDialog, kPortButtons, HitRule::LastWins) {}

private:
    void Update(short) override
    {
        static constexpr short kItems[kPortButtons] = { 0xc, 4, 7, 8, 9, 0xa, 0xb };
        static constexpr short kLabels[kPortButtons] = { kLeaveButtonLabel, kRechargeButtonLabel,
                                                         kTradeCenterButtonLabel, kOutfitterButtonLabel,
                                                         kShipyardButtonLabel, kMissionBBSButtonLabel,
                                                         kBarButtonLabel };
        Boolean noShips = (dockedPort->flags & kSpobUninhabited);
        bool live[kPortButtons] = {
            true,
            shipData->credits > 0 && noShips == 0 && shipData->fuel < ShipFuelCapacity(shipData),
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
};

} // namespace

// FUNCTION: EVNOVA 0x0049fe10
// MAC: _TrackPlanetButtons i386 0x0002996f ppc 0x0002df5c
short TrackPlanetButtons(Point where) { return PlanetButtons().Track(where); }

// FUNCTION: EVNOVA 0x004a0220
// MAC: _DrawPlanetButtons i386 0x0002968d ppc 0x0002dc84
void DrawPlanetButtons(short selected) { PlanetButtons().Draw(selected); }
