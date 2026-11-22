/* engineutils.cpp -- keys, cursor, screen fades, sound and speech, random numbers, starfield */

#include "game.h"
#include "player.h"
#include "render.h"
#include "sound.h"
#include "gameutil.h"
#include "space.h"
#include "dialogs.h"
#include "stellar.h"
#include "viewscale.h"
#include "platform.h"
#include "screen.h"

EXTERN_C_BEGIN

/* fills `frame` with the picture's bounds (name invented) */

// FUNCTION: EVNOVA 0x00466540
// MAC: _CenterPict i386 0x00008652 ppc 0x0000a6c8
void CenterPict(void *pict, Rect *bounds)
{
    Rect r;
    Rect frame;

    if (pict != 0) {
        r = *bounds;
        QDGetPictureBounds((PicHandle)pict, &frame);
        bounds->top = ((r.bottom - r.top) - (frame.bottom - frame.top)) / 2 + r.top;
        bounds->bottom = (frame.bottom - frame.top) + bounds->top;
        bounds->left = ((r.right - r.left) - (frame.right - frame.left)) / 2 + r.left;
        bounds->right = (frame.right - frame.left) + bounds->left;
    }
}


// FUNCTION: EVNOVA 0x00466810
ScanCode FirstKeyDown(bool capsLock)
{
    ScanCode code;
    char name[16];
    int32_t i;
    bool capsPressed;

    KeyboardKbhit();
    capsPressed = PlatCapsLockPressed() != 0;
    i = 0;
    do {
        code = ScanCode(i);
        switch (code) {
        case kScan1: case kScan2: case kScan3: case kScan4:
        case kScanF1: case kScanF2: case kScanF3: case kScanF4: case kScanF5:
        case kScanLGui: case kScanRGui:
            break;
        case kScanCapsLock:
            if (capsLock && (capsPressed || KeyIsDown(code) != 0))
                return code;
            break;
        default:
            if (KeyIsDown(code) != 0) {
                /* refused: a key without a name should not be shown in Key Settings */
                KeyboardScanCodeToString(name, sizeof(name), code);
                if (name[0] != 0)
                    return code;
            }
            break;
        }
        i = i + 1;
    } while (i < kScanCount);
    return kScanNone;
}

// FUNCTION: EVNOVA 0x00466850
void KeyPrefsToSlotTable(void)
{
    newKeyPrefs[0] = keyPrefs.accelerate;
    newKeyPrefs[1] = keyPrefs.reverse;
    newKeyPrefs[2] = keyPrefs.turnRight;
    newKeyPrefs[3] = keyPrefs.turnLeft;
    newKeyPrefs[4] = keyPrefs.afterburner;
    newKeyPrefs[5] = keyPrefs.faceTarget;
    newKeyPrefs[6] = keyPrefs.hyperspace;
    newKeyPrefs[7] = keyPrefs.nextSystem;
    newKeyPrefs[8] = keyPrefs.jump;
    newKeyPrefs[9] = keyPrefs.clearTarget;
    newKeyPrefs[10] = keyPrefs.hail;
    newKeyPrefs[11] = keyPrefs.land;
    newKeyPrefs[12] = keyPrefs.firePrimary;
    newKeyPrefs[13] = keyPrefs.fireSecondary;
    newKeyPrefs[14] = keyPrefs.selectSecondary;
    newKeyPrefs[15] = keyPrefs.deselectSecondary;
    newKeyPrefs[16] = keyPrefs.targetNext;
    newKeyPrefs[17] = keyPrefs.targetNearest;
    newKeyPrefs[18] = keyPrefs.escortMenu;
    newKeyPrefs[19] = keyPrefs.escortOrder[0];
    newKeyPrefs[20] = keyPrefs.escortOrder[1];
    newKeyPrefs[21] = keyPrefs.escortOrder[2];
    newKeyPrefs[22] = keyPrefs.escortOrder[3];
    newKeyPrefs[23] = keyPrefs.pause;
    newKeyPrefs[24] = keyPrefs.dismiss;
    newKeyPrefs[25] = keyPrefs.board;
    newKeyPrefs[26] = keyPrefs.jettison;
    newKeyPrefs[27] = keyPrefs.eject;
    newKeyPrefs[28] = keyPrefs.selfDestruct;
    newKeyPrefs[29] = keyPrefs.cloak;
    newKeyPrefs[30] = keyPrefs.map;
    newKeyPrefs[31] = keyPrefs.playerInfo;
    newKeyPrefs[32] = keyPrefs.missions;
    newKeyPrefs[33] = keyPrefs.toggleFPS;
    newKeyPrefs[34] = keyPrefs.speedUp;
}

// FUNCTION: EVNOVA 0x00466a30
void SlotTableToKeyPrefs(void)
{
    keyPrefs.accelerate = newKeyPrefs[0];
    keyPrefs.reverse = newKeyPrefs[1];
    keyPrefs.turnRight = newKeyPrefs[2];
    keyPrefs.turnLeft = newKeyPrefs[3];
    keyPrefs.afterburner = newKeyPrefs[4];
    keyPrefs.faceTarget = newKeyPrefs[5];
    keyPrefs.hyperspace = newKeyPrefs[6];
    keyPrefs.nextSystem = newKeyPrefs[7];
    keyPrefs.jump = newKeyPrefs[8];
    keyPrefs.clearTarget = newKeyPrefs[9];
    keyPrefs.hail = newKeyPrefs[10];
    keyPrefs.land = newKeyPrefs[11];
    keyPrefs.firePrimary = newKeyPrefs[12];
    keyPrefs.fireSecondary = newKeyPrefs[13];
    keyPrefs.selectSecondary = newKeyPrefs[14];
    keyPrefs.deselectSecondary = newKeyPrefs[15];
    keyPrefs.targetNext = newKeyPrefs[16];
    keyPrefs.targetNearest = newKeyPrefs[17];
    keyPrefs.escortMenu = newKeyPrefs[18];
    keyPrefs.escortOrder[0] = newKeyPrefs[19];
    keyPrefs.escortOrder[1] = newKeyPrefs[20];
    keyPrefs.escortOrder[2] = newKeyPrefs[21];
    keyPrefs.escortOrder[3] = newKeyPrefs[22];
    keyPrefs.pause = newKeyPrefs[23];
    keyPrefs.dismiss = newKeyPrefs[24];
    keyPrefs.board = newKeyPrefs[25];
    keyPrefs.jettison = newKeyPrefs[26];
    keyPrefs.eject = newKeyPrefs[27];
    keyPrefs.selfDestruct = newKeyPrefs[28];
    keyPrefs.cloak = newKeyPrefs[29];
    keyPrefs.map = newKeyPrefs[30];
    keyPrefs.playerInfo = newKeyPrefs[31];
    keyPrefs.missions = newKeyPrefs[32];
    keyPrefs.toggleFPS = newKeyPrefs[33];
    keyPrefs.speedUp = newKeyPrefs[34];
}


// FUNCTION: EVNOVA 0x00466c10
// MAC: _DupKeyCheck i386 0x00004d69 ppc 0x00006284
Boolean DupKeyCheck(ScanCode value, short skip)
{
    short i;

    for (i = 0; i < kBindingCount; i++) {
        if (value == newKeyPrefs[i] && i != skip)
            return 1;
    }
    return 0;
}

#define kFadeMillis 500
#define kMessageStrings 1000    /* STR# holding the numbered player messages */

EXTERN_C_END

Boolean speechMgrPresent;
uint64_t randomSeed;

EXTERN_C_BEGIN

/* screen fade state (names invented; the mac fades via CGDisplayFade instead) */
int32_t screenGamma = 0x00010000;           /* 0x00575878 — 16.16, 1.0 = full brightness */

/*
 * The windows port drives the palette from the DirectDraw side, so the mac
 * gamma write is compiled out here. The ramp is still walked at the original
 * rate, which is what paces the fade.
 */
#define SetScreenGamma(level)  ((void)(level))

/* Show numbered message `index`, letting a plug-in override the built-in text. */
// FUNCTION: EVNOVA 0x00467cf0
// MAC: _DisplayMessage i386 0x00005289 ppc 0x000067d0
void DisplayMessage(short index)
{
    char text[256];

    if (LoadPluginString(index + kMessageStrings - 1, (StringPtr)text) == 0)
        GetIndString((StringPtr)text, kMessageStrings, index);
    DisplayComm((PStrChar *)text, 480);
}

/* Walk the gamma ramp to `percent` brightness over `duration` milliseconds. */
// FUNCTION: EVNOVA 0x00467d60
void FadeGammaTo(uint32_t percent, uint32_t duration)
{
    uint32_t target;
    uint32_t start;
    uint32_t elapsed;
    int32_t delta;

    /* 1.1.1 */
    Screen::FadeWhite(kWhiteNone, kWhiteNone, 0);
    target = (percent << 16) / 100;
    delta = target - screenGamma;
    if (duration != 0) {
        start = ReadTickMilliseconds();
        do {
            elapsed = ReadTickMilliseconds() - start;
            if (elapsed > duration)
                elapsed = duration;
            SetScreenGamma(FixedMultiply((elapsed << 16) / duration, delta));
        } while (elapsed < duration);
    }
    screenGamma = target;
}

// FUNCTION: EVNOVA 0x00467de0
// MAC: _FadeScreenOut i386 0x0000530f ppc 0x00006848
void FadeScreenOut(void)
{
    if (screenFaded != 0 || inBackground != 0)
        return;
    ScreenEndScene();
    if (settings.windowed == 0)
        FadeGammaTo(0, kFadeMillis);
    screenFaded = 1;
}

// FUNCTION: EVNOVA 0x00467e20
// MAC: _FadeScreenIn i386 0x000053b2 ppc 0x00006924
void FadeScreenIn(void)
{
    if (screenFaded == 0)
        return;
    ScreenEndScene();
    if (settings.windowed == 0)
        FadeGammaTo(100, kFadeMillis);
    screenFaded = 0;
}

/* 1.1.1's FadeWhiteIn/Out */
static Boolean whiteReserved;

void FadeWhiteIn(void)
{
    screenFaded = true;
    whiteReserved = true;
    Screen::FadeWhite(kWhiteNone, kWhiteSolid, kWhiteFadeTicks);
}

void FadeWhiteOut(void)
{
    if (whiteReserved)
        Screen::FadeWhite(kWhiteSolid, kWhiteNone, kWhiteFadeTicks);
    whiteReserved = false;
    screenFaded = false;
}


/* A random value in [0, range); 0 for an empty range (range <= 0). */
// FUNCTION: EVNOVA 0x004683b0
// MAC: _Rand i386 0x000a4c76 ppc 0x000b08b4
short Rand(short range)
{
    if (range <= 0)
        return 0;
    return (short)PlatRand(&randomSeed, range);
}

/* Seeds the generator from the clock. */
void RandSeedFromClock(void)
{
    uint32_t seconds;

    GetDateTime(&seconds);
    randomSeed = seconds + ReadTick();
}

#define kPanCutoffSq   40000
#define kPanNear      722500
#define kPanHalfWidth    200

// FUNCTION: EVNOVA 0x004692e0
// MAC: _PlaySoundDistance i386 0x0000e007 ppc 0x000116f4
Boolean PlaySoundDistance(int32_t unused, Sound *sound, short flags, FloatPoint *pos,
                     FloatPoint *listener)
{
    int dy;
    int dx;
    int64_t dist;
    short floorVol;
    short leftVol;
    short rightVol;

    leftVol = rightVol = gSoundVolume;
    if (gSoundVolume > 0) {
        dx = (pos->x - listener->x);
        dy = (pos->y - listener->y);
        dist = (int64_t)dx * dx + (int64_t)dy * dy;
        if (dist > 0) {
            if (dist > kPanCutoffSq) {
                if (dx > kPanHalfWidth) {               /* to the right: louder on the right */
                    leftVol = (gSoundVolume * kPanCutoffSq / dist);
                    rightVol = (gSoundVolume * kPanNear / dist);
                } else if (dx < -kPanHalfWidth) {
                    leftVol = (gSoundVolume * kPanNear / dist);
                    rightVol = (gSoundVolume * kPanCutoffSq / dist);
                } else {
                    rightVol = (gSoundVolume * kPanNear / dist);
                    leftVol = rightVol;
                }
                floorVol = (gSoundVolume / 8);
                if (floorVol < 1)
                    floorVol = 1;
                if (leftVol < floorVol)
                    leftVol = floorVol;
                if (rightVol < floorVol)
                    rightVol = floorVol;
                if (leftVol > gSoundVolume)
                    leftVol = gSoundVolume;
                if (rightVol > gSoundVolume)
                    rightVol = gSoundVolume;
            }
        }
    }
    PlaySound(sound, flags, leftVol, rightVol);
    return 1;
}

// FUNCTION: EVNOVA 0x00469480
// MAC: _CheckSpeechMgr i386 0x00005906 ppc 0x00006fd0
void CheckSpeechMgr(void)
{
}

// FUNCTION: EVNOVA 0x00469490
// MAC: _SpeakIt i386 0x0000b029 ppc 0x0000d948
void SpeakIt(char *msg, short flag)
{
}


/* The flag is tested (with its companion at 0x007354b2) right before the
   shift/control/alt scancodes are scanned, so clearing it re-arms those. */

// FUNCTION: EVNOVA 0x00469c90
// MAC: _QuickKeyCheckHandler i386 0x00005d5a ppc 0x000074b8
void QuickKeyCheckHandler(void)
{
    quickKeyCheckResult = 0;
}


// FUNCTION: EVNOVA 0x00469ca0
// MAC: _QuickKeyCheck i386 0x0000a0f5 ppc 0x0000c6e4
unsigned char QuickKeyCheck(ScanCode scanCode)
{
    short i;

    if (ignoreKeys)
        return 0;
    if (escortMenuOnscreen > 0) {
        for (i = 0; i < 5; i++) {
            if (scanCode == keyPrefs.quickKey[i])
                return 0;
        }
    }
    return KeyIsDown(scanCode);
}

// FUNCTION: EVNOVA 0x00469cf0
unsigned char RawKeyCheck(ScanCode scanCode)
{
    if (ignoreKeys)
        return 0;
    return KeyIsDown(scanCode);
}

// FUNCTION: EVNOVA 0x00469d10
// MAC: _KeyCheck i386 0x0000a19b ppc 0x0000c7c8
unsigned char KeyCheck(ScanCode scanCode)
{
    KeyboardKbhit();
    return KeyIsDown(scanCode);
}

// FUNCTION: EVNOVA 0x0046a870
// MAC: _RefreshSAT i386 0x00006b59 ppc 0x000085e0
void RefreshSAT(void)
{
    Rect *r;
    Rect box;
    Rect strip;

    if (creatingPilotFile != 0 || gPause != 0)
        return;

    SWSetPortToBackground(gSpriteWorld);
    SetSystemBackgroundColor();
    PaintRect(&gSpriteWorld->windRect);
    ForeColor(blackColor);
    SWSetPortToWorkArea(gSpriteWorld);
    SetSystemBackgroundColor();
    PaintRect(&gSpriteWorld->windRect);
    ForeColor(blackColor);
    SWSetPortToWindow(gSpriteWorld);

    if (playerIsLanded == 0)
        SWUpdateSpriteWorld(gSpriteWorld, 0);

    CopyBits(GetPortBitMapForCopyBits(*(CGrafPtr *)gSpriteWorld->workFrameP),
             GetPortBitMapForCopyBits(gSpriteWorld->windowFrameP->framePort),
             &gSpriteWorld->workFrameP->frameRect,
             &gSpriteWorld->windowFrameP->frameRect,
             0, 0);
    Screen::ComposeView();

    r = GetCWindowPortRect((WindowRef)gWind);
    box = *r;
    box.left = box.right - kStatusBarWidth;
    strip = box;
    OffsetRect(&strip, -strip.left, -strip.top);

    RedrawStatus(0);
    DrawStatusRadar(0);
    SWSetPortToWindow(gSpriteWorld);
    CopyBits((BitMapPtr)GetGWCPixMap(&loadPort),
             GetPortBitMapForCopyBits(GetCWindowGrafPort(gWind)),
             &box, &box, 0, 0);

    if (playerIsLanded == 0) {
        FloaterDrawCallback(gSpriteWorld);
    }
    SWSetPortToWindow(gSpriteWorld);
    QDFlushPortBuffer(GetWindowPort(gWind), NULL);
}

// FUNCTION: EVNOVA 0x0046aad0
// MAC: _PlaySound i386 0x00005e9a ppc 0x0000768c
void PlaySound(Sound *a, short b, short lo, short hi)
{
    if (settings.stereoPan != 0) {
        ST_PlaySoundStereo(a, b, lo, hi);
        return;
    }
    ST_PlaySound(a, b, (lo + hi) / 2);              /* Positional Stereo off: mono */
}


// FUNCTION: EVNOVA 0x0046ab00
// MAC: _AdjustWarpSoundSpeed i386 0x00005ecc ppc 0x000076a8
void AdjustWarpSoundSpeed(double rate)
{
    unsigned short vol;

    warpUpSoundSpec.rate = (65536.0 / rate);
    vol = gSoundVolume;
    warpUpSoundSpec.leftLevel = vol;
    warpUpSoundSpec.rightLevel = vol;
}

/* eight steps per unit, clamped to a byte's worth */
// FUNCTION: EVNOVA 0x0046ab60
// MAC: _AdjustSoundVolume i386 0x00005efb ppc 0x000076e4
void AdjustSoundVolume(void)
{
    gSoundVolume = settings.volume * 8;
    if (gSoundVolume > 0x100)
        gSoundVolume = 0x100;
    if (gSoundVolume < 0)
        gSoundVolume = 0;
}

// FUNCTION: EVNOVA 0x0046b9f0
// MAC: _HideTheCursor i386 0x00006a01 ppc 0x00008404
void HideTheCursor(void)
{
    OSCursorHide();
    cursorHidden = (OSCursorIsVisible() == 0);
}

// FUNCTION: EVNOVA 0x0046ba10
// MAC: _ShowTheCursor i386 0x00006a1f ppc 0x00008424
void ShowTheCursor(void)
{
    OSCursorShow();
    cursorHidden = (OSCursorIsVisible() == 0);
}

/* paint in the current system's colour, and tell SpriteWorld about it too */

// FUNCTION: EVNOVA 0x0046bbf0
// MAC: _SetSystemBackgroundColor i386 0x00006af4 ppc 0x00008568
void SetSystemBackgroundColor(void)
{
    short sys;

    if (shipData[0].curSystem < 0)
        sys = 0;
    else
        sys = shipData[0].curSystem;
    RGBForeColor(&systemData[sys].rgb);
    SWSetBackgroundColor(gSpriteWorld, &systemData[sys].rgb);
}

// FUNCTION: EVNOVA 0x0046bc50
// MAC: _SetSystemBackgroundBackColor i386 0x00006e06 ppc 0x00008850
void SetSystemBackgroundBackColor(void)
{
    if (shipData[0].curSystem < 0) {
        BackColor(blackColor);
        return;
    }
    RGBBackColor(&systemData[shipData[0].curSystem].rgb);
}

// FUNCTION: EVNOVA 0x0046cc60
// MAC: _PutSpriteInLayer i386 0x0000766c ppc 0x000091c0
void PutSpriteInLayer(SpriteRec *sprite, SpriteLayerRec *layer)
{
    if (sprite == NULL || layer == NULL)
        return;
    if (sprite->parentSpriteLayerP == layer)
        return;
    SWRemoveSprite(sprite);
    SWAddSprite(layer, sprite);
}

// FUNCTION: EVNOVA 0x0046e840
// MAC: _BusyIdle i386 0x00005981 ppc 0x0000707c
/* one turn of a wait loop: handle pending events, then sleep until the next event or a millisecond */
void BusyIdle(void)
{
    ProcessSystemEvents();
    PlatWaitEvent(1);
}

// FUNCTION: EVNOVA 0x0046e8e0
// MAC: _DisposeDialogNoFlash i386 0x00007f91 ppc 0x00009db4
void DisposeDialogNoFlash(void *dlg)
{
    int saved;

    saved = gDisableUpdatesMode;
    gDisableUpdatesMode = 1;
    DisposeDialog((DialogRecord *)dlg);
    gDisableUpdatesMode = saved;
}

#define kStarLayers kMaxStars


// FUNCTION: EVNOVA 0x0046ebf0
// MAC: _RandomizeStars i386 0x0000b145 ppc 0x0000db10
void RandomizeStars(void)
{
    short i;
    short count;
    double stars;
    short width;
    short height;

    if (systemData[shipData->curSystem].murk < 0) {
        ClearStarfield();
    } else if (gSpriteWorld != NULL) {
        width = gSpriteWorld->windRect.right - gSpriteWorld->windRect.left;
        height = gSpriteWorld->windRect.bottom - gSpriteWorld->windRect.top;
        /* kClassicStars over the 830x768 view of a 1024x768 game size, the same density on any view */
        stars = kClassicStars * ((double)width * height / (830.0 * 768.0)) + 0.5;
        count = stars > kStarLayers ? kStarLayers : (short)stars;
        for (i = 0; i < kStarLayers; i++) {
            if (i < count) {
                starData[i].live = 1;
                starData[i].frame = Rand(masterStarSprite->numFrames);
                starData[i].pos.x = shipData->pos.x + Rand(width) - width / 2;
                starData[i].pos.y = shipData->pos.y + Rand(height) - height / 2;
                if (settings.parallaxStars)
                    starData[i].depth = Rand(0x23) * 0.01;
                else
                    starData[i].depth = 0;
            } else {
                starData[i].live = 0;
            }
        }
    }
}

/* clears every record's active flag (mac symtab has no name here — likely
   a static in the mac build; keep the address name) */
// FUNCTION: EVNOVA 0x0046ede0
// MAC: _ClearStarfield i386 0x00008152 ppc 0x0000a008
void ClearStarfield(void)
{
    short i;

    for (i = 0; i < kStarLayers; i++)
        starData[i].live = 0;
}

// FUNCTION: EVNOVA 0x0046ee50
// MAC: _ScrollStarfield i386 0x00008171 ppc 0x0000a038
void ScrollStarfield(float dx, float dy)
{
    short i;

    for (i = 0; i < kStarLayers; i++) {
        if (starData[i].live != 0) {
            if (starData[i].depth > 0.0) {
                starData[i].pos.x -= dx * starData[i].depth;
                starData[i].pos.y -= dy * starData[i].depth;
            }
        }
    }
}

// FUNCTION: EVNOVA 0x0046ef80
// MAC: _HideCursorSprite i386 0x000081d6 ppc 0x0000a0a4
void HideCursorSprite(void)
{
    if (cursorSprite != NULL) {
        SWSetSpriteVisible(cursorSprite, 0);
        cursorAlpha = 0;
    }
}

// FUNCTION: EVNOVA 0x0046efb0
// MAC: _WarpSoundDuration i386 0x00008203 ppc 0x0000a0f4
short WarpSoundDuration(void)
{
    if (doubleTime != 0)
        return jumpSoundLength[1];
    return jumpSoundLength[0];
}

EXTERN_C_END
