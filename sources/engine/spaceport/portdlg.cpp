/* portdlg.cpp -- the spaceport dialog, its preload and prices */
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

DialogPtr portDialog;
short planetPicId;
PicHandle portBackgroundPic;
void *planetSnd;
Boolean canDoMissions;
Boolean didOutfit;
Boolean didBuyShip;
Boolean onPlanet = 0;
uint32_t ambientNoiseDelay;
GWorldContainer planetPicContainer;

EXTERN_C_BEGIN

#define kPortDlogID         0x3e8
#define kLastPriceMult      1
#define kPortVenue          3
#define kMissionOfferSpread 30
#define kLandTimeSteps      3
#define kAllButtonsOff      -8

#define SpobHasTrade(s)     ((s)->flags & kSpobTradeCenter)
#define SpobHasOutfitter(s) ((s)->flags & kSpobHasOutfitter)
#define SpobAmbientLoop(s)  ((s)->flags2 & kSpobLoopSound)

#define kStellarBackdropPictID 0x2134

// FUNCTION: EVNOVA 0x0048e970
// MAC: _PreloadPortDialog i386 0x0004e5c7 ppc 0x000571c0
void PreloadPortDialog(SpobRecord *spob, short stellar)
{
    Rect rect;
    PicHandle pic;

    portBackgroundPic = GetPicture(kStellarBackdropPictID);
    planetPicContainer.world = NULL;

    if (spob->customPict >= kFirstResourceID)
        planetPicId = spob->customPict;
    else
        planetPicId = spob->type + 10000;

    pic = GetPicture(planetPicId);
    if (pic != NULL) {
        QDGetPictureBounds(pic, &rect);
        OffsetRect(&rect, -rect.left, -rect.top);
        CreateGWorldContainer(&planetPicContainer, 0, gSpriteWorld->pixelDepth, &rect);
        if (planetPicContainer.world != NULL) {
            SetGWorldContainer(&planetPicContainer);
            ForeColor(blackColor);
            PaintRect(&planetPicContainer.bounds);
            DrawPicHandle(pic, &planetPicContainer.bounds);
            QDReleaseResource(pic);
            UnsetGWorldContainer(&planetPicContainer);
        }
    }
}

/* the mac engine invalidates a dialog item through the caller's own GetDialogItem locals */
// FUNCTION: EVNOVA 0x00491f30
// MAC: _DoPortDialog i386 0x0005f911 ppc 0x000675a4
void DoPortDialog(SpobRecord *spob, short stellar)
{
    Boolean uninhabited;
    short i;
    short itemHit;
    short savedStellar;
    short savedRoute;
    short n;
    short j;
    unsigned short vol;
    Boolean quit;

    quit = 0;
    if (screenFaded != 0)
        return;
    onPlanet = 1;
    dockedPort = spob;
    didOutfit = didBuyShip = 0;
    strncpy(descTextPtr, "", 5);
    LoadDescription(descTextPtr, stellar + kFirstResourceID);
    for (i = 0; i <= kLastPriceMult; i++)
        priceFluxMult[i] = 1.0f;
    if (spob->govt != -1) {
        for (i = 0; i < kMaxRanks; i++) {
            if (rankData[i].active != 0 && rankData[i].loaded != 0
                    && GovtAllies(spob->govt, rankData[i].govt)) {
                for (j = 0; j <= kLastPriceMult; j++)
                    priceFluxMult[j] *= (float)rankData[i].priceMod
                                        * kPortPriceScale;
            }
        }
    }
    Dialog dialog;
    dialog.Open(kPortDlogID, PortDialogUpdate, PortFilter, &portDialog, DialogClose::NoFlash);
    if (!dialog.IsOpen())
        return;
    if (spob->customSound >= kFirstResourceID && settings.ambientSounds != 0) {
        planetSnd = ST_LoadSndResource(spob->customSound);
        planetSoundSpec.handle = (void **)planetSnd;
        vol = gSoundVolume;
        planetSoundSpec.leftLevel = vol;
        planetSoundSpec.rightLevel = vol;
        ambientNoiseDelay = 0;
    } else {
        planetSnd = NULL;
    }
    if (SpobUninhabited(dockedPort) != 0)
        canDoMissions = 0;
    else
        canDoMissions = 1;
    SetDialogGWorld(portDialog);
    BackColor(blackColor);
    BackColor(whiteColor);
    SetDialogGWorld(portDialog);
    FlushEvents(everyEvent - diskMask, 0);
    ForeColor(blackColor);
    RedrawDialog(portDialog);
    PortDialogUpdate();
    SWSetPortToWorkArea(gSpriteWorld);
    ForeColor(blackColor);
    PaintRect(&gSpriteWorld->workFrameP->frameRect);
    SWSetPortToWindow(gSpriteWorld);
    DrawStatusRadar(1);
    DrawStatusCargo();
    SetDialogGWorld(portDialog);
    UpdateAllPortDialogs();
    MissionLandCheck();
    HandleStellarSystemVisibility();
    DrawStatusCargo();
    UpdateAllPortDialogs();
    missionsOutside = kPortVenue;
    offeringFromShip = 0;
    missionOfferTime = ReadTick() + Rand(kMissionOfferSpread)
                       + kMissionOfferSpread;
    OfferMissionFromPort(kPortVenue);
    do {
        if (gQuit)
            quit = 1;
        itemHit = dialog.Modal();
        if (gQuit)
            quit = 1;
        if (itemHit == 12) {
            quit = 1;
        } else if (itemHit == 2) {
            DoPlayerInfoDialog();
            SetDialogGWorld(portDialog);
            RedrawDialog(portDialog);
        } else if (itemHit == 3) {
            savedStellar = shipData->targetStellar;
            savedRoute = shipData->navTargetKind;
            hypergateMapMode = 0;
            DoSystemMap();
            shipData->targetStellar = savedStellar;
            shipData->navTargetKind = savedRoute;
            UpdateAllPortDialogs();
            SetDialogGWorld(portDialog);
            RedrawDialog(portDialog);
        } else if (itemHit == 4) {
            uninhabited = SpobUninhabited(dockedPort);
            if (uninhabited == 0) {
                n = ShipFuelCapacity(shipData);
                shipData->fuel = (short)shipData->fuel;
                n -= shipData->fuel;
                if (dockedPort->dominated == 0) {
                    if (n > shipData->credits)
                        n = shipData->credits;
                    shipData->credits -= n;
                }
                shipData->fuel += n;
                statusRedraw[kRedrawFuel] = 1;
                statusRedraw[kRedrawCargo] = 1;
                gSpriteWorld->fastBlit = 1;
                DrawStatus();
                gSpriteWorld->fastBlit = 0;
                SetDialogGWorld(portDialog);
            }
            SetDialogGWorld(portDialog);
            RedrawDialog(portDialog);
        } else if (itemHit == 5) {
            if (planetSnd != NULL
                    && SpobAmbientLoop(dockedPort) == 0)
                ST_PlaySoundParam(&planetSoundSpec);
        } else if (itemHit == 7) {
            if (SpobHasTrade(dockedPort) != 0) {
                SetDialogGWorld(portDialog);
                PlanetButtons().Draw(kAllButtonsOff);
                DoTradeDialog();
                statusRedraw[kRedrawCargo] = 1;
                DrawStatus();
                SetDialogGWorld(portDialog);
                RedrawDialog(portDialog);
            }
        } else if (itemHit == 8) {
            if (SpobHasOutfitter(dockedPort) != 0) {
                InvalDialogItem(portDialog, 8);
                UpdateAllPortDialogs();
                DoOutfitDialog();
                statusRedraw[kRedrawCargo] = 1;
                DrawStatus();
                SetDialogGWorld(portDialog);
                RedrawDialog(portDialog);
            }
        } else if (itemHit == 9) {
            if (SpobHasShipyard(dockedPort) != 0) {
                InvalDialogItem(portDialog, 9);
                UpdateAllPortDialogs();
                shipBuyMode = 0;
                DoShipyardDialog();
                statusRedraw[kRedrawCargo] = 1;
                SetDialogGWorld(portDialog);
                RedrawDialog(portDialog);
            }
        } else if (itemHit == 10) {
            if (canDoMissions != 0) {
                savedStellar = shipData->targetStellar;
                savedRoute = shipData->navTargetKind;
                SetDialogGWorld(portDialog);
                RedrawDialog(portDialog);
                UpdateAllPortDialogs();
                reenterMissionDialog = 0;
                do {
                    DoMissionDialog();
                    statusRedraw[kRedrawCargo] = 1;
                    statusRedraw[kRedrawNav] = 1;
                    DrawStatus();
                } while (reenterMissionDialog != 0);
                SetDialogGWorld(portDialog);
                RedrawDialog(portDialog);
                shipData->targetStellar = savedStellar;
                shipData->navTargetKind = savedRoute;
                statusRedraw[kRedrawCargo] = 1;
                statusRedraw[kRedrawNav] = 1;
                DrawStatus();
            }
        } else if (itemHit == 11) {
            SetDialogGWorld(portDialog);
            RedrawDialog(portDialog);
            UpdateAllPortDialogs();
            if (SpobHasBar(dockedPort) != 0) {
                DoBarDialog();
                DrawStatusCargo();
                SetDialogGWorld(portDialog);
                RedrawDialog(portDialog);
            }
        } else if (itemHit == 14) {
            n = 0;
            for (i = 0; i < kMaxActiveMissions; i++) {
                if (currentMissionObjectives[i].active != 0
                    && (currentMissionObjectives[i].flags & kMissionHidden) == 0)
                    n++;
            }
            if (n > 0) {
                DoMissionInfoDialog();
                SetDialogGWorld(portDialog);
                RedrawDialog(portDialog);
            }
        } else if (itemHit == 15) {
            if (OfferMissionFromPort(kPortVenue)) {
                SetDialogGWorld(portDialog);
                UpdateAllPortDialogs();
            }
        }
    } while (quit == 0);
    DoEscortLand();
    if (planetPicContainer.world != NULL)
        DisposeGWorldContainer(&planetPicContainer);
    if (portBackgroundPic != NULL) {
        QDReleaseResource(portBackgroundPic);
        portBackgroundPic = NULL;
    }
    if (planetSnd != NULL) {
        ST_HaltSound(planetSnd);
        ST_DisposeSound(planetSnd);
    }
    if (didOutfit != 0)
        IncrementGameTime();
    if (didBuyShip != 0) {
        for (i = 0; i <= kLandTimeSteps; i++)
            IncrementGameTime();
    }
    if (didOutfit != 0 || didBuyShip != 0) {
        shipData->targetShip = -1;
        statusRedraw[kRedrawWeap] = 1;
    }
    dialog.Close();
    onPlanet = 0;
}

#define kAmbientPeriod   480


#define kNameBaseline 0x12
#define kNameSize     0x12

static constexpr KeyItem kPortKeys[] = {
    { 'r', 4 }, { 'f', 4 }, { 'c', 7 }, { 't', 7 },
    { 'o', 8 }, { 's', 9 }, { 'n', 10 }, { 'b', 11 },
};
static constexpr short kPortSlotItems[] = { 12, 4, 7, 8, 9, 10, 11 };

/*
 * A click inside item 5 - the planet image - reports the image rather than
 * whatever button the tracker thought it was over.
 */
// FUNCTION: EVNOVA 0x004928a0
// MAC: _PortFilter i386 0x0004f041 ppc 0x00057c18
Boolean PortFilter(DialogPtr unused, EventRecord *ev, short *itemHit)
{
    Rect box;
    short itemType;
    void *item;
    Point where;
    short button;
    EventChar c;

    *itemHit = -1;
    if ((absquatulateStr)[0] != 0) {
        *itemHit = 12;
        return 1;
    }
    if (SpobUninhabited(dockedPort))
        canDoMissions = 0;
    else
        canDoMissions = 1;
    if (planetSnd != NULL) {
        if ((dockedPort->flags2 & kSpobLoopSound) != 0) {
            if (ST_IsSoundPlaying(planetSnd) == 0)
                ST_PlaySoundParam(&planetSoundSpec);
        } else if (ReadTick() >= ambientNoiseDelay) {
            ST_PlaySoundParam(&planetSoundSpec);
            ambientNoiseDelay = ReadTick() + kAmbientPeriod + Rand(kAmbientPeriod);
        }
    }
    *itemHit = -1;
    if ((int)ev->what == kKeyDown) {
        if (KeyCheck(keyPrefs.map) && mapDialog == NULL) {
            *itemHit = 3;
            return 1;
        }
        if (KeyCheck(keyPrefs.playerInfo) && infoDialog == NULL) {
            *itemHit = 2;
            return 1;
        }
        if (KeyCheck(keyPrefs.missions) && missionInfoDialog == NULL) {
            *itemHit = 14;
            return 1;
        }
        c = EventCharOf(ev);
        c = tolower(c);
        if (IsReturnChar(c) || c == kEscapeChar) {
            *itemHit = 12;
            return 1;
        }
        if (KeyItemHit(c, kPortKeys, itemHit))
            return 1;
    }
    if ((int)ev->what == kMouseDown) {
        where = ev->where;
        PortDialogUpdate();
        button = PlanetButtons().Track(where);
        GetDialogItem(portDialog, 5, &itemType, (Handle *)&item, &box);
        if (PtInRectPoint(where, &box)) {
            *itemHit = 5;
            return 1;
        }
        return ButtonItemHit(button, kPortSlotItems, itemHit);
    }
    return 0;
}

// FUNCTION: EVNOVA 0x00492c40
// MAC: _PortDialogUpdate i386 0x0004ebbd ppc 0x00057884
void PortDialogUpdate(void)
{
    Rect box;
    short itemType;
    Handle item;
    Boolean inside;

    SetGWorldContainer(&loadPort);
    ForeColor(blackColor);
    PaintRect(GetDialogPortRect(portDialog));
    if (portBackgroundPic != NULL)
        DrawPicHandle(portBackgroundPic, GetDialogPortRect(portDialog));
    GetDialogItem(portDialog, 3, &itemType, &item, &box);
    inside = RectInRgn(&box, GetWindowVisRgn(FrontWindow(portDialog)));
    if (inside) {
        ForeColor(blackColor);
        PaintRect(&box);
        ForeColor(whiteColor);
        MoveTo(box.left, (box.top + kNameBaseline));
        TextFont(gTimesFont);
        TextSize(kNameSize);
        DrawStringCentered(dockedPort->name, box.left, box.right,
                           box.top + kNameBaseline);
        ForeColor(blackColor);
    }
    GetDialogItem(portDialog, 5, &itemType, &item, &box);
    inside = RectInRgn(&box, GetWindowVisRgn(FrontWindow(portDialog)));
    if (inside) {
        if (planetPicContainer.world != NULL)
            CopyBits(GWorldContainerBits(&planetPicContainer),
                     (BitMapPtr)GWorldContainerBits(&loadPort),
                     &planetPicContainer.bounds, &box, 0,
                     GetWindowVisRgn(FrontWindow(portDialog)));
    }
    GetDialogItem(portDialog, 6, &itemType, &item, &box);
    inside = RectInRgn(&box, GetWindowVisRgn(FrontWindow(portDialog)));
    if (inside) {
        ForeColor(blackColor);
        TextFont(smallTextFont);
        TextSize(smallTextSize);
        TETextBox(descTextPtr, strlen(descTextPtr), &box, 0);
        InvertRect(&box);
    }
    SetGWorldContainer(&loadPort);
    PlanetButtons().Draw(-8);
    CopyLoadPortToDialog(portDialog);
}

// FUNCTION: EVNOVA 0x00497430
// MAC: _UpdateAllPortDialogs i386 0x000598a6 ppc 0x000619d8
void UpdateAllPortDialogs(void)
{
    if (portDialog != NULL) {
        SetDialogGWorld(portDialog);
        PortDialogUpdate();
    }
    if (shipyardDialog != NULL) {
        SetDialogGWorld(shipyardDialog);
        ShipyardDialogUpdate();
    }
    if (outfitDialog != NULL) {
        SetDialogGWorld(outfitDialog);
        OutfitDialogUpdate();
    }
    if (missionDialog != NULL) {
        SetDialogGWorld(missionDialog);
        if (missionsOutside != 0)
            SingleMissionDialogUpdate();
        else if (portDialog != NULL)
            MissionDialogUpdate();
    }
    if (portDialog != NULL || shipyardDialog != NULL || outfitDialog != NULL
            || missionDialog != NULL)
        nv_UpdateDialogWindow(1);
}

// FUNCTION: EVNOVA 0x0049d640
// MAC: _ApplyPriceAndTechnologyFlux i386 0x0004e6cd ppc 0x000572e4
int32_t ApplyPriceAndTechnologyFlux(int32_t cost, float multiplier, short itemTech, short portTech)
{
    int32_t price;

    if (cost <= 0)
        return 0;

    price = cost;
    if (itemTech <= 5 && portTech <= 5 && itemTech < portTech && price >= 100)
        price *= (100 - (portTech - itemTech) * 3) * kPortPriceScale;

    price *= multiplier;

    if (price > 100000) {
        price = price / 1000;
        price = price * 1000;
    } else if (price > 10000) {
        price = price / 100;
        price = price * 100;
    } else if (price > 100) {
        price = price / 10;
        price = price * 10;
    }

    if (cost > 0 && price <= 0)
        price = 1;

    return price;
}

EXTERN_C_END
