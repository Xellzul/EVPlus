/* portdlg.cpp -- the spaceport dialog, its preload and prices */

#include "game.h"
#include "stellar.h"
#include "gameutil.h"
#include "missions.h"
#include "player.h"
#include "render.h"
#include "ships.h"
#include "sound.h"
#include "ai.h"
#include "gamedialog.h"
#include "buttons.h"

DialogPtr portDialog;
short planetPicId;
PicHandle portBackgroundPic;
Sound *planetSnd;
Boolean canDoMissions;
Boolean didOutfit;
Boolean didBuyShip;
Boolean onPlanet = 0;
uint32_t ambientNoiseDelay;
GWorldContainer planetPicContainer;

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

namespace {

// FUNCTION: EVNOVA 0x00491f30
// MAC: _DoPortDialog i386 0x0005f911 ppc 0x000675a4
/* The spaceport: refuelling, the trade center, outfitter, shipyard, mission computer and bar. */
class PortDialog final : public GameDialog {
public:
    void Run(SpobRecord *spob, short stellar);

private:
    void Hit(short item) override;
    static Boolean Filter(DialogPtr unused, const DialogEvent &ev, short *itemHit);
};

void PortDialog::Run(SpobRecord *spob, short stellar)
{
    short i;
    short j;
    unsigned short vol;

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
    dialog.Open(kPortDlogID, PortDialogUpdate, Filter, &portDialog, DialogClose::NoFlash);
    if (!dialog.IsOpen())
        return;
    if (spob->customSound >= kFirstResourceID && settings.ambientSounds != 0) {
        planetSnd = ST_LoadSndResource(spob->customSound);
        planetSoundSpec.sound = planetSnd;
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
    Loop();
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

void PortDialog::Hit(short item)
{
    Boolean uninhabited;
    short savedStellar;
    short savedRoute;
    short n;

    if (gQuit)
        done = 1;
    if (item == 12) {
        done = 1;
    } else if (item == 2) {
        PlayerInfoFromDialog(portDialog);
    } else if (item == 3) {
        MapFromDialog(portDialog);
    } else if (item == 4) {
        uninhabited = SpobUninhabited(dockedPort);
        if (uninhabited == 0) {
            n = shipData->FuelCapacity();
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
    } else if (item == 5) {
        if (planetSnd != NULL
                && SpobAmbientLoop(dockedPort) == 0)
            ST_PlaySoundParam(&planetSoundSpec);
    } else if (item == 7) {
        if (SpobHasTrade(dockedPort) != 0) {
            SetDialogGWorld(portDialog);
            PlanetButtons().Draw(kAllButtonsOff);
            DoTradeDialog();
            statusRedraw[kRedrawCargo] = 1;
            DrawStatus();
            SetDialogGWorld(portDialog);
            RedrawDialog(portDialog);
        }
    } else if (item == 8) {
        if (SpobHasOutfitter(dockedPort) != 0) {
            InvalDialogItem(portDialog, 8);
            UpdateAllPortDialogs();
            DoOutfitDialog();
            statusRedraw[kRedrawCargo] = 1;
            DrawStatus();
            SetDialogGWorld(portDialog);
            RedrawDialog(portDialog);
        }
    } else if (item == 9) {
        if (SpobHasShipyard(dockedPort) != 0) {
            InvalDialogItem(portDialog, 9);
            UpdateAllPortDialogs();
            shipBuyMode = 0;
            DoShipyardDialog();
            statusRedraw[kRedrawCargo] = 1;
            SetDialogGWorld(portDialog);
            RedrawDialog(portDialog);
        }
    } else if (item == 10) {
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
    } else if (item == 11) {
        SetDialogGWorld(portDialog);
        RedrawDialog(portDialog);
        UpdateAllPortDialogs();
        if (SpobHasBar(dockedPort) != 0) {
            DoBarDialog();
            DrawStatusCargo();
            SetDialogGWorld(portDialog);
            RedrawDialog(portDialog);
        }
    } else if (item == 14) {
        MissionInfoFromDialog(portDialog);
    } else if (item == 15) {
        OfferFromDialog(kPortVenue, portDialog);
    }
}

#define kAmbientPeriod   480


#define kNameBaseline 0x12
#define kNameSize     0x12

constexpr KeyItem kPortKeys[] = {
    { 'r', 4 }, { 'f', 4 }, { 'c', 7 }, { 't', 7 },
    { 'o', 8 }, { 's', 9 }, { 'n', 10 }, { 'b', 11 },
};
constexpr short kPortSlotItems[] = { 12, 4, 7, 8, 9, 10, 11 };

/*
 * A click inside item 5 - the planet image - reports the image rather than
 * whatever button the tracker thought it was over.
 */
// FUNCTION: EVNOVA 0x004928a0
// MAC: _PortFilter i386 0x0004f041 ppc 0x00057c18
Boolean PortDialog::Filter(DialogPtr unused, const DialogEvent &ev, short *itemHit)
{
    Rect box;
    short itemType;
    void *item;
    Point where;
    short button;
    EventChar c;

    *itemHit = -1;
    if (LeaveRequested(12, itemHit))
        return 1;
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
    if (ev.what == DialogInput::Key) {
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
        c = ev.ch;
        c = tolower(c);
        if (IsReturnChar(c) || c == kEscapeChar) {
            *itemHit = 12;
            return 1;
        }
        if (KeyItemHit(c, kPortKeys, itemHit))
            return 1;
    }
    if (ev.what == DialogInput::Click) {
        where = ev.where;
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

} // namespace

void DoPortDialog(SpobRecord *spob, short stellar)
{
    PortDialog().Run(spob, stellar);
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

void RedrawAfterSubDialog(DialogPtr self)
{
    UpdateAllPortDialogs();
    SetDialogGWorld(self);
    RedrawDialog(self);
}

void OpenMapKeepingNav(void)
{
    short savedStellar = shipData->targetStellar;
    short savedRoute = shipData->navTargetKind;

    hypergateMapMode = 0;
    DoSystemMap();
    shipData->targetStellar = savedStellar;
    shipData->navTargetKind = savedRoute;
}

void MapFromDialog(DialogPtr self)
{
    OpenMapKeepingNav();
    RedrawAfterSubDialog(self);
}

void PlayerInfoFromDialog(DialogPtr self)
{
    DoPlayerInfoDialog();
    RedrawAfterSubDialog(self);
}

Boolean AnyMissionShown(void)
{
    short i;

    for (i = 0; i < kMaxActiveMissions; i++) {
        if (currentMissionObjectives[i].active != 0
            && (currentMissionObjectives[i].flags & kMissionHidden) == 0)
            return 1;
    }
    return 0;
}

void MissionInfoFromDialog(DialogPtr self)
{
    if (AnyMissionShown()) {
        DoMissionInfoDialog();
        RedrawAfterSubDialog(self);
    }
}

void OfferFromDialog(short venue, DialogPtr self)
{
    if (OfferMissionFromPort(venue))
        RedrawAfterSubDialog(self);
}

Boolean LeaveRequested(short leaveItem, short *itemHit)
{
    if (absquatulateStr.empty())
        return 0;
    *itemHit = leaveItem;
    return 1;
}

Boolean OfferDue(short offerItem, short *itemHit)
{
    if (ReadTick() < missionOfferTime)
        return 0;
    *itemHit = offerItem;
    missionOfferTime = ReadTick() + Rand(30) + 30;
    return 1;
}

Boolean QuickKeyHit(short mapItem, short infoItem, short missionsItem, short *itemHit)
{
    QuickKeyCheckHandler();
    if (mapItem != -1 && QuickKeyCheck(keyPrefs.map) && mapDialog == NULL) {
        *itemHit = mapItem;
        return 1;
    }
    if (infoItem != -1 && QuickKeyCheck(keyPrefs.playerInfo) && infoDialog == NULL) {
        *itemHit = infoItem;
        return 1;
    }
    if (missionsItem != -1 && QuickKeyCheck(keyPrefs.missions) && missionInfoDialog == NULL) {
        *itemHit = missionsItem;
        return 1;
    }
    return 0;
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
