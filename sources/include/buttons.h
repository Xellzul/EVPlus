/* buttons.h -- each dialog's ButtonBar: Draw(hilited) draws it, Track(where) returns the slot pressed */
#pragma once
#include "buttonbar.h"

// FUNCTION: EVNOVA 0x004a04d0
// MAC: _TrackTradeButtons i386 0x000294cd ppc 0x0002dae4
// FUNCTION: EVNOVA 0x004a06a0
// MAC: _DrawTradeButtons i386 0x000292e2 ppc 0x0002d960
class TradeButtons final : public ButtonBar {
public:
    TradeButtons();

private:
    void Update(short) override;
};

// FUNCTION: EVNOVA 0x004a0810
// MAC: _TrackCapAssignButtons i386 0x000291c7 ppc 0x0002d824
// FUNCTION: EVNOVA 0x004a0950
// MAC: _DrawCapAssignButtons i386 0x000290c7 ppc 0x0002d738
class CapAssignButtons final : public ButtonBar {
public:
    CapAssignButtons();

private:
    void Update(short) override;
};

// FUNCTION: EVNOVA 0x004a09e0
// MAC: _TrackCommDialogButtons i386 0x00028eff ppc 0x0002d55c
// FUNCTION: EVNOVA 0x004a0bc0
// MAC: _DrawCommDialogButtons i386 0x00028c43 ppc 0x0002d2a8
/* Close / Assist / Greetings; a ship that won't talk has no middle button and its items are 1, 4, 2 */
class CommButtons final : public ButtonBar {
public:
    CommButtons();

private:
    void Update(short) override;
};

// FUNCTION: EVNOVA 0x004a0e50
// MAC: _TrackPlanetCommDialogButtons i386 0x00028b28 ppc 0x0002d16c
// FUNCTION: EVNOVA 0x004a0f90
// MAC: _DrawPlanetCommDialogButtons i386 0x0002893c ppc 0x0002cf8c
class PlanetCommButtons final : public ButtonBar {
public:
    PlanetCommButtons();

private:
    void Update(short) override;
};

// FUNCTION: EVNOVA 0x004a1130
// MAC: _TrackMissionButtons i386 0x000287f5 ppc 0x0002ce34
// FUNCTION: EVNOVA 0x004a1290
// MAC: _DrawMissionButtons i386 0x00028657 ppc 0x0002ccc8
class MissionButtons final : public ButtonBar {
public:
    MissionButtons();

private:
    void Update(short) override;
};

// FUNCTION: EVNOVA 0x004a13c0
// MAC: _TrackMissionInfoButtons i386 0x0002850e ppc 0x0002cb74
// FUNCTION: EVNOVA 0x004a1520
// MAC: _DrawMissionInfoButtons i386 0x00028359 ppc 0x0002c9f4
class MissionInfoButtons final : public ButtonBar {
public:
    MissionInfoButtons();

private:
    void Update(short) override;
};

// FUNCTION: EVNOVA 0x004a1670
// MAC: _TrackSingleMissionButtons i386 0x00028188 ppc 0x0002c820
// FUNCTION: EVNOVA 0x004a1820
// MAC: _DrawSingleMissionButtons i386 0x00027db8 ppc 0x0002c4d8
/* Yes / No (or a lone OK, then slot 1 is an off-screen box) and the scroll pair */
class SingleMissionButtons final : public ButtonBar {
public:
    SingleMissionButtons();

private:
    void Update(short) override;
};

// FUNCTION: EVNOVA 0x004a1ae0
// MAC: _TrackPlayerInfoButtons i386 0x00027c4d ppc 0x0002c370
// FUNCTION: EVNOVA 0x004a1c40
// MAC: _DrawPlayerInfoButtons i386 0x00027aae ppc 0x0002c1e4
/* the page tabs (the open page drawn pressed) and Jettison, shown on the cargo page only */
class PlayerInfoButtons final : public ButtonBar {
public:
    explicit PlayerInfoButtons(short page);

private:
    void Update(short) override;

    short page;
};

// FUNCTION: EVNOVA 0x004a1d90
// MAC: _TrackEscortButtons i386 0x00027987 ppc 0x0002c0a0
// FUNCTION: EVNOVA 0x004a1ed0
// MAC: _DrawEscortButtons i386 0x000277a4 ppc 0x0002bea0
class EscortButtons final : public ButtonBar {
public:
    EscortButtons();

private:
    void Update(short) override;
};

// FUNCTION: EVNOVA 0x004a2090
// MAC: _TrackHaggleButtons i386 0x00027670 ppc 0x0002bd78
// FUNCTION: EVNOVA 0x004a21e0
// MAC: _DrawHaggleButtons i386 0x0002750d ppc 0x0002bc34
class HaggleButtons final : public ButtonBar {
public:
    HaggleButtons();

private:
    void Update(short) override;
};

// FUNCTION: EVNOVA 0x004a22e0
// MAC: _TrackPlunderButtons i386 0x0002732a ppc 0x0002b9e0
// FUNCTION: EVNOVA 0x004a24e0
// MAC: _DrawPlunderButtons i386 0x00027109 ppc 0x0002b784
class PlunderButtons final : public ButtonBar {
public:
    PlunderButtons();

private:
    void Update(short) override;
};

// FUNCTION: EVNOVA 0x004a26e0
// MAC: _TrackBarButtons i386 0x00026fd5 ppc 0x0002b644
// FUNCTION: EVNOVA 0x004a2810
// MAC: _DrawBarButtons i386 0x00026e1d ppc 0x0002b4d0
/* slots 3 and 5 have no button of their own: dim, with label 0 */
class BarButtons final : public ButtonBar {
public:
    BarButtons();

private:
    void Update(short) override;
};

// FUNCTION: EVNOVA 0x004a2960
// MAC: _TrackTextButtons i386 0x00026bb0 ppc 0x0002b25c
// FUNCTION: EVNOVA 0x004a2ac0
// MAC: _DrawTextButtons i386 0x000269ea ppc 0x0002b090
class TextButtons final : public ButtonBar {
public:
    TextButtons();

private:
    void Update(short) override;
};

// FUNCTION: EVNOVA 0x004a2c30
// MAC: _DrawRaceButtons i386 0x00026617 ppc 0x0002acc4
// FUNCTION: EVNOVA 0x004a2de0
// MAC: _TrackRaceButtons i386 0x0002689d ppc 0x0002af18
/* Done (Cancel before the first race) and Help, then the four racers' pictures */
class RaceButtons final : public ButtonBar {
public:
    RaceButtons();

private:
    void Update(short) override;
};

// FUNCTION: EVNOVA 0x0049fe10
// MAC: _TrackPlanetButtons i386 0x0002996f ppc 0x0002df5c
// FUNCTION: EVNOVA 0x004a0220
// MAC: _DrawPlanetButtons i386 0x0002968d ppc 0x0002dc84
/* a closed service's button is not drawn but still covers its place */
class PlanetButtons final : public ButtonBar {
public:
    PlanetButtons();

private:
    void Update(short) override;
};

// FUNCTION: EVNOVA 0x0049eef0
// MAC: _TrackMapButtons i386 0x0002aafa ppc 0x0002f048
// FUNCTION: EVNOVA 0x0049f1f0
// MAC: _DrawMapButtons i386 0x0002a86e ppc 0x0002ee28
class MapButtons final : public ButtonBar {
public:
    MapButtons();

private:
    void Update(short) override;
};

// FUNCTION: EVNOVA 0x0049f3f0
// MAC: _TrackShipyardButtons i386 0x0002a54e ppc 0x0002eb08
// FUNCTION: EVNOVA 0x0049f6f0
// MAC: _DrawShipyardButtons i386 0x0002a2e2 ppc 0x0002e8b8
class ShipyardButtons final : public ButtonBar {
public:
    ShipyardButtons();

private:
    void Update(short) override;
};

// FUNCTION: EVNOVA 0x0049f8f0
// MAC: _TrackOutfitButtons i386 0x0002a029 ppc 0x0002e628
// FUNCTION: EVNOVA 0x0049fbb0
// MAC: _DrawOutfitButtons i386 0x00029d6a ppc 0x0002e398
/* a mouse-down may land on any button, even a dim one or the hidden Buy */
class OutfitButtons final : public ButtonBar {
public:
    OutfitButtons();

private:
    void Update(short) override;
};
