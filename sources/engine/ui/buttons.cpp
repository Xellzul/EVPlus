/* buttons.cpp -- the custom picture buttons every EV Nova dialog draws over its Dialog Manager items (win TU region 0x0049eef0..0x004a3aa0). Notes: Research/notes/buttons.md */
#include "scaleddraw.h"
#include "uiscale.h"

#include "ai.h"
#include "dialogs.h"
#include "game.h"
#include "render.h"
#include "prototypes.h"
#include "buttons.h"

#define kButtonPictID         7500
#define kButtonMaskPictID     7600

/* the mac builds the offscreens on the main screen's device; the port's take none */
#define kButtonDevice 0

/* button slots */
enum { kTradeDoneButton, kTradeBuyButton, kTradeSellButton };
enum { kMissionAcceptButton, kMissionOkButton };
enum { kMissionInfoDoneButton, kMissionInfoAbortButton };
enum { kHaggleYesButton, kHaggleNoButton };

/* the button object's globals; this file ends that object (the .CRT hook) */
GWorldContainer buttonPort;
GWorldContainer buttonImagePort;
GWorldContainer buttonMaskPort;
RGBColor buttonColor[3];
Rect buttonPieceRect[kButtonPieceCount];
unsigned char buttonLabel[kButtonLabelCount][256];
unsigned char missionButtonLabel[kMissionButtonLabelCount][256];

/* the up/down pair of a scrolling text: dim at an end, not drawn when nothing scrolls */
ButtonLook ScrollLook(bool canScroll)
{
    if (canScrollTextUp == 0 && canScrollTextDown == 0)
        return ButtonLook::Disabled;
    return canScroll ? ButtonLook::Normal : ButtonLook::Dim;
}

TradeButtons::TradeButtons() : ButtonBar(tradeDialog, 3, HitRule::LastWins) {}

void TradeButtons::Update(short)
{
    bool buy = CanBuyGoods(selTradeItem) != 0;
    bool sell = CanSellGoods(selTradeItem) != 0;

    Set(kTradeDoneButton, 1, buttonLabel[kDoneButtonLabel]);
    Set(kTradeBuyButton, 0xd, buttonLabel[kBuyButtonLabel], Lit(buy));
    Set(kTradeSellButton, 0xe, buttonLabel[kSellButtonLabel], Lit(sell));
    SetPress(kTradeBuyButton, true, buy);
    SetPress(kTradeSellButton, true, sell);
}

CapAssignButtons::CapAssignButtons() : ButtonBar(captureDialog, 2, HitRule::FirstWins) {}

void CapAssignButtons::Update(short)
{
    Set(0, 1, buttonLabel[kUseAsMyShipButtonLabel]);
    Set(1, 2, buttonLabel[kUseAsEscortButtonLabel]);
}

CommButtons::CommButtons() : ButtonBar(commDialog, 3, HitRule::FirstWins) {}

void CommButtons::Update(short)
{
    bool quiet = commShip != 0 && commShip->govt != -1
                 && (govtData[commShip->govt].flags2 & kGovtNotTalkative) != 0;
    bool hasMission = commShip != 0 && commShip->specialMission != -1
                      && currentMissionObjectives[commShip->specialMission].active != 0
                      && currentMissions[commShip->specialMission].shipBehav == kMissionModeEscort;
    short middle = kRequestAssistButtonLabel;

    if (!quiet && commShip != 0) {
        if (ExtendedIsThreatToPlayer(commShip) != 0)
            middle = kBegMercyButtonLabel;
        else if (commShip->parentShip == 0 && commShip->aiType == kShipTypeEscort && !hasMission)
            middle = kReleaseButtonLabel;
    }
    Set(0, 1, buttonLabel[kCloseChannelButtonLabel]);
    Set(1, quiet ? 4 : 2, buttonLabel[middle], quiet ? ButtonLook::Hidden : ButtonLook::Normal);
    Set(2, quiet ? 2 : 3, buttonLabel[kGreetingsButtonLabel]);
}

PlanetCommButtons::PlanetCommButtons() : ButtonBar(commDialog, 3, HitRule::FirstWins) {}

void PlanetCommButtons::Update(short)
{
    static short lastLabel;     /* an always-dominated planet's dim button keeps the label drawn last */
    const SpobRecord &spob = stellarData[shipData[0].targetStellar];
    bool third = spob.dominated == 0 || (spob.flags2 & kSpobAlwaysDominated) == 0;

    if (third && Drawing())
        lastLabel = spob.dominated != 0 ? kReleaseButtonLabel : kDemandTributeButtonLabel;
    Set(0, 1, buttonLabel[kCloseChannelButtonLabel]);
    Set(1, 2, buttonLabel[hatesPlayer != 0 ? kOfferBribeButtonLabel : kGreetingsButtonLabel]);
    Set(2, 3, buttonLabel[lastLabel], Lit(third));
}

MissionButtons::MissionButtons() : ButtonBar(missionDialog, 2, HitRule::FirstWins) {}

void MissionButtons::Update(short)
{
    bool picked = selMissionListItem != -1;

    Set(kMissionAcceptButton, 1, buttonLabel[kAcceptButtonLabel], Lit(picked));
    Set(kMissionOkButton, 7, buttonLabel[kLeaveButtonLabel]);
    SetPress(kMissionAcceptButton, picked, picked);
}

MissionInfoButtons::MissionInfoButtons() : ButtonBar(missionInfoDialog, 2, HitRule::LastWins) {}

void MissionInfoButtons::Update(short)
{
    bool picked = selMissionListItem != -1;

    Set(kMissionInfoDoneButton, 1, buttonLabel[kDoneButtonLabel]);
    Set(kMissionInfoAbortButton, 5, buttonLabel[kAbortButtonLabel],
        Lit(picked && showAbortMissionButton != 0));
    SetPress(kMissionInfoAbortButton, picked, picked);
}

SingleMissionButtons::SingleMissionButtons() : ButtonBar(missionDialog, 4, HitRule::LastWins) {}

void SingleMissionButtons::Update(short)
{
    if (canRefuseMission != 0) {
        Set(0, 1, missionButtonLabel[0]);
        Set(1, 2, missionButtonLabel[1]);
    } else {
        Rect none = { -1, -1, 0, 0 };
        Set(0, 6, missionButtonLabel[0]);
        SetBox(1, none, missionButtonLabel[1], ButtonLook::Hidden);
    }
    Set(2, 9, buttonLabel[kScrollUpButtonLabel], ScrollLook(canScrollTextUp != 0));
    Set(3, 10, buttonLabel[kScrollDownButtonLabel], ScrollLook(canScrollTextDown != 0));
}

PlayerInfoButtons::PlayerInfoButtons(short page) : ButtonBar(infoDialog, 6, HitRule::FirstWins), page(page) {}

void PlayerInfoButtons::Update(short)
{
    static constexpr short kLabels[5] = { kDoneButtonLabel, kGeneralButtonLabel, kCargoButtonLabel,
                                          kExtrasButtonLabel, kHonorsButtonLabel };
    bool jettison = page == 2 && PlayerHasJettisonableCargo() != 0;

    for (short i = 0; i < 5; i++)
        Set(i, i + 1, buttonLabel[kLabels[i]]);
    Set(5, 7, buttonLabel[kJettisonButtonLabel], jettison ? ButtonLook::Normal : ButtonLook::Hidden);
    if (page >= 0 && page < 6)
        buttons[page].latched = true;
}

EscortButtons::EscortButtons() : ButtonBar(commDialog, 4, HitRule::FirstWins) {}

void EscortButtons::Update(short)
{
    bool upgrading = commShip != 0 && commShip->upgradeOnLanding != 0;
    bool selling = commShip != 0 && commShip->sellOnLanding != 0;

    Set(0, 1, buttonLabel[kCloseChannelButtonLabel]);
    Set(1, 2, buttonLabel[kReleaseButtonLabel]);
    Set(2, 3, buttonLabel[upgrading ? kCancelUpgradeButtonLabel : kUpgradeEscortButtonLabel],
        Lit(canUpgradeEscort != 0));
    Set(3, 4, buttonLabel[selling ? kCancelSaleButtonLabel : kSellEscortButtonLabel], Lit(canSellEscort != 0));
}

HaggleButtons::HaggleButtons() : ButtonBar(haggleDialog, 2, HitRule::LastWins) {}

void HaggleButtons::Update(short)
{
    bool paying = haggleMode == 1;

    Set(kHaggleYesButton, 1, buttonLabel[paying ? kAcceptPaymentButtonLabel : kAcceptPriceButtonLabel]);
    Set(kHaggleNoButton, 2, buttonLabel[paying ? kDemandMoreButtonLabel : kLowerPriceButtonLabel]);
}

PlunderButtons::PlunderButtons() : ButtonBar(plunderDialog, 6, HitRule::LastWins) {}

void PlunderButtons::Update(short)
{
    static constexpr short kItems[6] = { 1, 2, 3, 4, 6, 7 };
    static constexpr short kLabels[6] = { kAbortButtonLabel, kPlunderCargoButtonLabel, kCreditsButtonLabel,
                                          kAmmoButtonLabel, kEnergyButtonLabel, kCaptureShipButtonLabel };
    bool enabled[6] = { true, salvageType != -1, salvageCredits > 0, (short)salvageAmmoType != -1,
                        salvageFuelQty > 0, captureOdds > 0 };

    for (short i = 0; i < 6; i++) {
        Set(i, kItems[i], buttonLabel[kLabels[i]], Lit(enabled[i]));
        SetPress(i, enabled[i], enabled[i]);
    }
}

BarButtons::BarButtons() : ButtonBar(barDialog, 6, HitRule::LastWins) {}

void BarButtons::Update(short)
{
    Set(0, 1, buttonLabel[kLeaveButtonLabel]);
    Set(1, 2, buttonLabel[kGambleButtonLabel]);
    Set(2, 3, buttonLabel[kHolovidButtonLabel]);
    Set(3, 4, buttonLabel[0], ButtonLook::Dim);
    Set(4, 5, buttonLabel[kHireEscortButtonLabel], Lit(CanHireEscorts() != 0));
    Set(5, 6, buttonLabel[0], ButtonLook::Dim);
}

TextButtons::TextButtons() : ButtonBar(textDialog, 3, HitRule::LastWins) {}

void TextButtons::Update(short)
{
    Set(0, 1, buttonLabel[kOkButtonLabel]);
    Set(1, 5, buttonLabel[kScrollUpButtonLabel], ScrollLook(canScrollTextUp != 0));
    Set(2, 6, buttonLabel[kScrollDownButtonLabel], ScrollLook(canScrollTextDown != 0));
}

RaceButtons::RaceButtons() : ButtonBar(raceDialog, 6, HitRule::FirstWins) {}

void RaceButtons::Update(short hilited)
{
    Set(0, 1, buttonLabel[raceWinner == -1 && raceCount == 0 ? kCancelButtonLabel : kDoneButtonLabel]);
    Set(1, 2, buttonLabel[kHelpButtonLabel]);
    for (short i = 0; i < 4; i++) {
        short frame = raceWinner == i ? 2 : raceWinner != -1 ? 3 : hilited == i + 2 ? 1 : 0;
        Set(i + 2, i + 3, buttonLabel[0], ButtonLook::Picture).pic = raceButtonPic[i][frame];
    }
}


// FUNCTION: EVNOVA 0x004a2f50
// MAC: _InitButtons i386 0x00025aa1 ppc 0x0002a1e0
void InitButtons(void)
{
    PicHandle buttonPic[kButtonPieceCount];
    PicHandle buttonMaskPic[kButtonPieceCount];
    Rect bounds;
    Rect *mask;
    short n;
    Rect *piece;

    for (n = 0; n < kButtonPieceCount; n++) {
        buttonPic[n] = GetPicture(n + kButtonPictID);
        buttonMaskPic[n] = GetPicture(n + kButtonMaskPictID);
        if (buttonPic[n] != 0) {
            QDGetPictureBounds(buttonPic[n], &buttonPieceRect[n]);
            OffsetRect(&buttonPieceRect[n], -buttonPieceRect[n].left, -buttonPieceRect[n].top);
        } else {
            SetRect(&buttonPieceRect[n], 0, 0, 0xc, 0x18);
        }
    }
    OffsetRect(&buttonPieceRect[1], buttonPieceRect[0].right, 0);
    OffsetRect(&buttonPieceRect[2], buttonPieceRect[1].right, 0);
    OffsetRect(&buttonPieceRect[3], 0, buttonPieceRect[0].bottom);
    OffsetRect(&buttonPieceRect[4], buttonPieceRect[3].right, buttonPieceRect[0].bottom);
    OffsetRect(&buttonPieceRect[5], buttonPieceRect[4].right, buttonPieceRect[0].bottom);
    OffsetRect(&buttonPieceRect[6], 0, buttonPieceRect[3].bottom);
    OffsetRect(&buttonPieceRect[7], buttonPieceRect[6].right, buttonPieceRect[3].bottom);
    OffsetRect(&buttonPieceRect[8], buttonPieceRect[7].right, buttonPieceRect[3].bottom);
    bounds.left = bounds.top = 0;
    bounds.right = (buttonPieceRect[2].right > buttonPieceRect[5].right) ?
                    buttonPieceRect[2].right : buttonPieceRect[5].right;
    bounds.right = (bounds.right > buttonPieceRect[8].right) ?
                    bounds.right : buttonPieceRect[8].right;
    bounds.bottom = (buttonPieceRect[6].bottom > buttonPieceRect[7].bottom) ?
                     buttonPieceRect[6].bottom : buttonPieceRect[7].bottom;
    bounds.bottom = (bounds.bottom > buttonPieceRect[8].bottom) ?
                     bounds.bottom : buttonPieceRect[8].bottom;
    CreateGWorldContainer(&buttonImagePort, kButtonDevice, gSpriteWorld->pixelDepth, &bounds);
    CreateGWorldContainer(&buttonMaskPort, kButtonDevice, 1, &bounds);
    SetGWorldContainer(&buttonImagePort);
    ForeColor(blackColor);
    mask = buttonPieceRect;
    for (n = 0, piece = buttonPieceRect; n < kButtonPieceCount; n++) {
        PaintRect(piece);
        if (buttonPic[n] != 0)
            DrawPicHandle(buttonPic[n], piece);
        piece++;
    }
    UnsetGWorldContainer(&buttonImagePort);
    SetGWorldContainer(&buttonMaskPort);
    ForeColor(blackColor);
    for (n = 0; n < kButtonPieceCount; n++) {
        PaintRect(mask);
        if (buttonMaskPic[n] != 0)
            DrawPicHandle(buttonMaskPic[n], mask);
        mask++;
    }
    UnsetGWorldContainer(&buttonMaskPort);
    for (n = 0; n < kButtonPieceCount; n++) {
        if (buttonPic[n] != 0)
            QDReleaseResource(buttonPic[n]);
        if (buttonMaskPic[n] != 0)
            QDReleaseResource(buttonMaskPic[n]);
    }
    for (n = 0; (int32_t)n < kButtonLabelCount; n++)
        GetIndString(buttonLabel[n], kButtonStrList, n + 1);
    buttonColor[0].red = buttonColor[0].green = buttonColor[0].blue = 0xffff;
    buttonColor[1].red = buttonColor[1].green = buttonColor[1].blue = 0x8000;
    buttonColor[2].red = buttonColor[2].green = buttonColor[2].blue = 0x8000;
    CreateButtonPort();
}

/* the port buttons are put together in, for the UI scale in use */
void CreateButtonPort(void)
{
    Rect bounds;

    SetRect(&bounds, 0, 0, 500, 75);
    if (UI::Enabled())
        SetRect(&bounds, 0, 0, UI::ScreenWidth(), UI::ScreenHeight());     /* buttons drawn where they lie */
    CreateGWorldContainer(&buttonPort, kButtonDevice, gSpriteWorld->pixelDepth, &bounds);
    if (buttonPort.world != 0)
        UI::ScaleUIPort(buttonPort.world);
}

// FUNCTION: EVNOVA 0x004a3300
// MAC: _DisposeButtons i386 0x00025e67 ppc 0x0002a55c
void DisposeButtons(void)
{
    if (buttonPort.world != 0)
        DisposeGWorldContainer(&buttonPort);
    if (buttonImagePort.world != 0)
        DisposeGWorldContainer(&buttonImagePort);
    if (buttonMaskPort.world != 0)
        DisposeGWorldContainer(&buttonMaskPort);
}

/* --- DrawButton (0x004a3340) --- */


#define kGlyphCaret 0x5e
#define kGlyphAmp   0x26
#define kGlyphPlus  0x2b
#define kGlyphMinus 0x2d

/* A button's face in buttonPort over `box': its left and right ends masked, the middle stretched between
   them, put together and stretched as one picture. */
static void DrawButtonFace(short mode, const Rect &box)
{
    UI::ArtComposition art(buttonPort.world, box);
    Rect piece;
    Rect mid = box;

    piece = buttonPieceRect[mode * 3];
    OffsetRect(&piece, -piece.left, -piece.top);
    OffsetRect(&piece, box.left, box.top);
    piece.bottom = box.bottom;
    CopyMask(GWorldContainerBits(&buttonImagePort), GWorldContainerBits(&buttonMaskPort),
             GWorldContainerBits(&buttonPort), &buttonPieceRect[mode * 3],
             &buttonPieceRect[mode * 3], &piece);
    mid.left = piece.right;
    piece = buttonPieceRect[mode * 3 + 2];
    OffsetRect(&piece, -piece.left, -piece.top);
    OffsetRect(&piece, box.right - piece.right, box.top);
    piece.bottom = box.bottom;
    CopyMask(GWorldContainerBits(&buttonImagePort), GWorldContainerBits(&buttonMaskPort),
             GWorldContainerBits(&buttonPort), &buttonPieceRect[mode * 3 + 2],
             &buttonPieceRect[mode * 3 + 2], &piece);
    mid.right = piece.left;
    CopyBits(GWorldContainerBits(&buttonImagePort), (BitMapPtr)GWorldContainerBits(&buttonPort),
             &buttonPieceRect[mode * 3 + 1], &mid, 0, 0);
}

// FUNCTION: EVNOVA 0x004a3340
// MAC: _DrawButton i386 0x00025ea8 ppc 0x0002a5dc
void DrawButton(CGrafPtr port, Rect *bounds, unsigned char *label,
                short state, short selected, PicHandle pic)
{
    Rect box;
    CGrafPtr savedPort;
    short mode;
    short size;
    uint32_t glyph;

    if (state == 0) {
        if (selected == 0)
            mode = 0;
        else
            mode = 1;
    } else
        mode = 2;
    GetPort((GrafPtr *)&savedPort);
    if (state == kButtonPicture) {
        GWorldContainerSetPort(&loadPort);
        ForeColor(blackColor);
        DrawPicHandle(pic, bounds);
        SetPort((GrafPtr)savedPort);
        ForeColor(blackColor);
        CopyBits(GWorldContainerBits(&loadPort), GetPortBitMapForCopyBits(savedPort),
                 bounds, bounds, 0, GetPortVisRgn(port));
    } else {
        GWorldContainerSetPort(&buttonPort);
        ForeColor(blackColor);
        box = *bounds;
        if (!UI::Enabled())
            OffsetRect(&box, -box.left, -box.top);
        CopyBits(GWorldContainerBits(&loadPort), (BitMapPtr)GWorldContainerBits(&buttonPort),
                 bounds, &box, 0, 0);
        if (state == 0 || state == kButtonDim) {
            DrawButtonFace(mode, box);
            RGBForeColor(&buttonColor[mode]);
            glyph = label[1];
            if (glyph == kGlyphCaret) {
                size = (box.bottom - box.top) / 10;
                PenSize(2, 2);
                MoveTo((box.left + box.right) / 2, (box.bottom + box.top) / 2 - size);
                Line(size * -2, size * 2);
                MoveTo((box.left + box.right) / 2, (box.bottom + box.top) / 2 - size);
                Line(size * 2, size * 2);
                PenSize(1, 1);
            } else if (glyph == kGlyphAmp) {
                size = (box.bottom - box.top) / 10;
                PenSize(2, 2);
                MoveTo((box.left + box.right) / 2, (box.bottom + box.top) / 2 + size);
                Line(size * -2, size * -2);
                MoveTo((box.left + box.right) / 2, (box.bottom + box.top) / 2 + size);
                Line(size * 2, size * -2);
                PenSize(1, 1);
            } else if (glyph == kGlyphPlus) {
                size = (box.bottom - box.top) / 8;
                PenSize(2, 2);
                MoveTo((box.left + box.right) / 2 - size, (box.bottom + box.top) / 2);
                Line(size * 2, 0);
                MoveTo((box.left + box.right) / 2, (box.bottom + box.top) / 2 - size);
                Line(0, size * 2);
                PenSize(1, 1);
            } else if (glyph == kGlyphMinus) {
                size = (box.bottom - box.top) / 9;
                PenSize(2, 2);
                MoveTo((box.left + box.right) / 2 - size, (box.bottom + box.top) / 2);
                Line(size * 2, 0);
                PenSize(1, 1);
            } else {
                TextFont(buttonFontID);
                TextSize(buttonFontSize);
                MoveTo((box.left + box.right) / 2 - StringWidth(label) / 2,
                          (box.top + box.bottom) / 2 + 5);
                DrawString(label);
            }
        }
        SetPort((GrafPtr)savedPort);
        ForeColor(blackColor);
        CopyBits(GWorldContainerBits(&buttonPort), GetPortBitMapForCopyBits(savedPort),
                 &box, bounds, 0, GetPortVisRgn(port));
    }
}
