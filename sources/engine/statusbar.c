// COMPILE: -lang c++ -bool off
/* statusbar.c -- the status-bar driver pair (win link order 0x0045d0a0, 0x0045d320; mac 1.1.1 twins _RedrawStatus @0x4cac0, _DrawStatus @0x4c85d). Notes: Research/notes/statusbar.md */
#include "macport.h"
#include "shipobj.h"
#include "game.h"
#include "dialogs.h"
#include "ships.h"
#include "player.h"
#include "render.h"
#include "burger.h"
#include "cefix.h"

ShipClassID lastTargetedShipClass;
short lastTargetedShipShields;
short radarCount;
unsigned char lastDebugTargInfoState;
unsigned char targKeyOnNow;
uint32_t lastRadarUpdateTime;
Boolean preventStatusCopy;
RGBColor *statusBrightColor;
RGBColor *statusDimColor;
RGBColor *shieldStatusColor;
RGBColor *armorStatusColor;
RGBColor *fuelStatusColor[2];
RGBColor *radarStatusColor[2];
short interferenceThisFrame;


// FUNCTION: EVNOVA 0x0045d0a0
// MAC: _RedrawStatus i386 0x0004cac0 ppc 0x00055a18
void RedrawStatus(Boolean copyToScreen)
{
    Rect box;
    Rect box2;

    if (creatingPilotFile || gPause)
        return;
    SetRect(&box, GetCWindowPortRect(gWind)->right - ceStatusBarWidth,
            GetCWindowPortRect(gWind)->top, GetCWindowPortRect(gWind)->right,
            GetCWindowPortRect(gWind)->bottom);
    CopyBits(GWorldContainerPixMap(&statusOffPort), (BitMapPtr)GWorldContainerBits(&loadPort),
             &statusOffPort.bounds, &box, 0, 0);
    if (HasIFF(shipData)) {
        SetGWorldContainer(&loadPort);
        box2 = statusOffRect[0];
        // BUG (all versions): offsets box, but paints the unshifted box2 (status-bar coordinates);
        // CE F14.04 keeps it
        OffsetRect(&box, GetCWindowPortRect(gWind)->right - ceStatusBarWidth, 0);
        ForeColor(blackColor);
        PaintRect(&box2);
        SWSetPortToWindow(gSpriteWorld);
    }
    statusRedraw[kRedrawNav] = 1;
    statusRedraw[kRedrawShield] = 1;
    statusRedraw[kRedrawCargo] = 1;
    statusRedraw[kRedrawWeap] = 1;
    statusRedraw[kRedrawFuel] = 1;
    statusRedraw[kRedrawRadar] = 1;
    statusRedraw[kRedrawTarg] = 1;
    lastTargetedShipClass = 0x8001;
    lastTargetedShipShields = 0x8001;
    lastDebugTargInfoState = 0;
    preventStatusCopy = 1;
    DrawStatus();
    preventStatusCopy = 0;
    if (copyToScreen) {
        box = *GetCWindowPortRect((WindowRef)gWind);
        box.left = box.right - ceStatusBarWidth;
        if (gSpriteWorld->fastBlit) {
            CopyBits(GWorldContainerBits(&loadPort),
                     GetPortBitMapForCopyBits(GetCWindowGrafPort((WindowRef)gWind)),
                     &box, &box, 0, 0);
        } else {
            CopyBits(GWorldContainerBits(&loadPort),
                     GetPortBitMapForCopyBits(GetCWindowGrafPort((WindowRef)gWind)),
                     &box, &box, 0, GetCWindowVisRgn((WindowRef)gWind));
        }
    }
}

// FUNCTION: EVNOVA 0x0045d320
// MAC: _DrawStatus i386 0x0004c85d ppc 0x00055688
void DrawStatus(void)
{
    if (fadePending[0] || fadePending[1] || creatingPilotFile)
        return;
    if (shipData[0].navTargetKind != -1 && ReadTick() % 30 == 0)
        statusRedraw[kRedrawNav] = 1;
    if (shipData[0].targetShip != -1) {
        if (ReadTick() % 45 == 0)
            statusRedraw[kRedrawTarg] = statusRedraw[kRedrawCargo] = 1;
    }
    if (QuickKeyCheck(kScanLCtrl) || QuickKeyCheck(kScanRCtrl))
        targKeyOnNow = 1;
    else
        targKeyOnNow = 0;
    if (ReadTick() >= lastRadarUpdateTime + 15) {
        lastRadarUpdateTime = ReadTick();
        radarCount = (radarCount + 1) % 2;
        if (shipData[0].targetShip != -1) {
            if (shipData[shipData[0].targetShip].shield != lastTargetedShipShields ||
                (ShipClassID)shipData[shipData[0].targetShip].type != lastTargetedShipClass)
                statusRedraw[kRedrawTarg] = 1;
            if (lastDebugTargInfoState != targKeyOnNow || targKeyOnNow)
                statusRedraw[kRedrawTarg] = 1;
        } else {
            lastTargetedShipClass = lastTargetedShipShields = 0x8001;
        }
        statusRedraw[kRedrawRadar] = 1;
    }
    if (statusRedraw[kRedrawTarg]) {
        DrawStatusTarg();
        statusRedraw[kRedrawTarg] = 0;
    }
    if (statusRedraw[kRedrawRadar]) {
        if (!onPlanet && !playerIsLanded)
            DrawStatusRadar(0);
        else
            DrawStatusRadar(1);
        statusRedraw[kRedrawRadar] = 0;
    }
    if (statusRedraw[kRedrawWeap]) {
        DrawStatusWeap();
        statusRedraw[kRedrawWeap] = 0;
    }
    if (statusRedraw[kRedrawNav]) {
        DrawStatusNav();
        statusRedraw[kRedrawNav] = 0;
    }
    if (statusRedraw[kRedrawShield]) {
        DrawStatusShield();
        statusRedraw[kRedrawShield] = 0;
    }
    if (statusRedraw[kRedrawFuel]) {
        DrawStatusFuel();
        statusRedraw[kRedrawFuel] = 0;
    }
    if (statusRedraw[kRedrawCargo]) {
        DrawStatusCargo();
        statusRedraw[kRedrawCargo] = 0;
    }
}
