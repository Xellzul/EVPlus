/* capturedlg.cpp -- the captured-ship assignment dialog */
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

DialogPtr captureDialog;
PicHandle captureBackgroundPic;

EXTERN_C_BEGIN

/*
 * mwcc 418 marks a name intrinsic-eligible on the declaration it gets from the
 * SYSTEM header, so any redeclaration that does not merge with it clears the
 * marking. Block scope is the only spelling that works.
 */


#define kCaptureStrIdx  0x76

Boolean CapturedAssignmentFilter(DialogPtr unused, EventRecord *ev, short *itemHit);

#define kCaptureDlogID   0x3fa
#define kCapturePictID   0x2144

// FUNCTION: EVNOVA 0x00497eb0
// MAC: _DoCapturedShipAssignmentDialog i386 0x0005b508 ppc 0x000635a8
Boolean DoCapturedShipAssignmentDialog(void)
{
    Boolean done;
    Boolean result;
    short itemHit;

    done = 0;
    Dialog dialog;
    dialog.Open(kCaptureDlogID, CapturedAssignmentDialogUpdate, CapturedAssignmentFilter, &captureDialog,
                DialogClose::NoFlash);
    if (!dialog.IsOpen())
        return 0;
    Picture backdrop(captureBackgroundPic);
    backdrop.Load(kCapturePictID);
    SetDialogGWorld(captureDialog);
    CapturedAssignmentDialogUpdate();
    FlushEvents(everyEvent - diskMask, 0);
    do {
        if (gQuit)
            done = 1;
        itemHit = dialog.Modal();
        if (itemHit == 1) {
            PlaySound(beepSnd[1], 1, gSoundVolume, gSoundVolume);
            result = 1;
            done = 1;
        }
        if (itemHit == 2) {
            PlaySound(beepSnd[1], 1, gSoundVolume, gSoundVolume);
            result = 0;
            done = 1;
        }
    } while (done == 0);
    SWSetPortToWindow(gSpriteWorld);
    dialog.Close();
    backdrop.Release();
    return result;
}

// FUNCTION: EVNOVA 0x00498040
// MAC: _CapturedAssignmentFilter i386 0x00051782 ppc 0x0005a024
Boolean CapturedAssignmentFilter(DialogPtr unused, EventRecord *ev, short *itemHit)
{
    Point where;
    short button;
    EventKey key;
    EventWhat what;

    what = ev->what;
    if (what == kKeyDown) {
        key = EventCharOf(ev);
        key = tolower(key);
        if (key == 'm') {
            *itemHit = 1;
            return 1;
        }
        if (key == 'e') {
            *itemHit = 2;
            return 1;
        }
    }
    if (what == kMouseDown) {
        where = ev->where;
        CapturedAssignmentDialogUpdate();
        button = CapAssignButtons().Track(where);
        if (button == 0) {
            *itemHit = 1;
            return 1;
        }
        if (button == 1) {
            *itemHit = 2;
            return 1;
        }
        *itemHit = -1;
        return 1;
    }
    if (what == kUpdateEvt) {
        CapturedAssignmentDialogUpdate();
    }
    return 0;
}

// FUNCTION: EVNOVA 0x00498110
// MAC: _CapturedAssignmentDialogUpdate i386 0x0005157d ppc 0x00059e90
void CapturedAssignmentDialogUpdate(void)
{
    short itemType;
    void *item;
    Rect box;
    char text[255];

    SetGWorldContainer(&loadPort);
    TextFont(smallTextFont);
    TextSize(smallTextSize);
    ForeColor(blackColor);
    PaintRect(GetDialogPortRect(captureDialog));
    if (captureBackgroundPic != NULL)
        DrawPicHandle(captureBackgroundPic, GetDialogPortRect(captureDialog));
    ForeColor(whiteColor);
    BackColor(blackColor);
    GetDialogItem(captureDialog, 3, &itemType, (Handle *)&item, &box);
    strcpy(text, "");
    StrcatIndCString(text, kGameStrList, kCaptureStrIdx);
    TETextBox(text, strlen(text), &box, 0);
    ForeColor(blackColor);
    BackColor(whiteColor);
    CapAssignButtons().Draw(-1);
    CopyLoadPortToDialog(captureDialog);
}

EXTERN_C_END
