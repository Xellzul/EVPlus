/* textdlg.cpp -- the scrolling text dialog and the shared dialog backdrop */

#include "scaleddraw.h"
#include "dialogs.h"
#include "game.h"
#include "stellar.h"
#include "gameutil.h"
#include "player.h"
#include "render.h"
#include "dialog.h"
#include "buttons.h"
#include "scrolltext.h"

DialogPtr textDialog;
PicHandle textDialogBackgroundPic[3];
TEHandle textDialogTextHandle;

/* the text item, its scroll buttons and TextButtons' slots for them */
constexpr ScrollingText::Layout kTextLayout = { 3, 5, 6, 1, 2, 10, 250 };

static ScrollingText DialogText()
{
    return ScrollingText(textDialog, textDialogTextHandle, kTextLayout);
}

/*
 * The port's TE record. textedit.cpp calls +0x08 one `long unk8`; this driver
 * writes the two halves separately from smallTextFont/smallTextSize, which names
 * them.
 */

#define kTextDialogPictID      0xbbc     /* dlog 3004, with a picture item */
#define kTextDialogPlainID     0xbbb     /* dlog 3003 */
#define kTextDlgBackdropPictID 0x214c    /* three stacked backdrop pieces */
#define kTextDlgSinglePictID   0x214f
#define kTextDlgMinHeight      0x30
#define kTextDlgSlackMargin    16
#define kTextDlgRecentre       0.3


                                                         /* 0x004d18b0 (mac _SizeWindow) */
                                                         /* 0x004d1a30 (mac _MoveWindow) */

/* on mac ShowTextDialog paints gWind through GetWindowPortRect and invalidates items itself */
// FUNCTION: EVNOVA 0x004982a0
// MAC: _ShowTextDialog i386 0x0005a752 ppc 0x000628d4
void ShowTextDialog(Boolean allowMap, Boolean showMovie)
{
    Rect box;
    short itemHit;
    short itemType;
    Handle item;
    Boolean savedUp;
    Rect *win;
    Boolean savedDown;
    Point pt;
    Boolean done;
    short slack;
    short viewHeight;
    short textHeight;
    short minHeight;
    short i;

    done = 0;
    if (showMovie && settings.noMovies == 0 && descMovieName[0] != 0
        && (descFlags & kDescMovieAfter) == 0)
        PlayQTMovie(descMovieName, (descFlags & kDescMovieDoubled),
                    (descFlags & kDescMovieFade));

    if (strlen(textShowString) == 0) {
        if (screenFaded) {
            if (gPause) {
                if (showingIntroText) {
                    SetGWorldContainer(&loadPort);
                    ForeColor(blackColor);
                    PaintRect(&loadPort.bounds);
                    SWSetPortToWindow(gSpriteWorld);
                    ForeColor(blackColor);
                    PaintRect(GetCWindowPortRect((WindowRef)gWind));
                } else {
                    mainScreenBackgroundDirty = 1;
                    MainScreenUpdate(1);
                }
            } else {
                UpdateAllPortDialogs();
                RefreshSAT();
                RedrawStatus(1);
            }
            if (showingIntroText == 0)
                FadeScreenIn();
        }
        return;
    }

    strcat(textShowString, "\r");
    Dialog dialog;
    if (descResourcePictId >= kFirstResourceID && showMovie) {
        dialog.Open(kTextDialogPictID, TextDialogUpdate, TextDialogFilter, &textDialog, DialogClose::NoFlash);
    } else {
        dialog.Open(kTextDialogPlainID, TextDialogUpdate, TextDialogFilter, &textDialog, DialogClose::NoFlash);
        descResourcePictId = -1;
    }
    if (!dialog.IsOpen())
        return;

    Picture backdrops[3];
    for (i = 0; i < 3; i++)
        backdrops[i].Bind(textDialogBackgroundPic[i]);
    if (descResourcePictId >= kFirstResourceID && showMovie) {
        backdrops[0].Load(kTextDlgSinglePictID);
        textDialogBackgroundPic[1] = NULL;
        textDialogBackgroundPic[2] = NULL;
    } else {
        for (i = 0; i < 3; i++)
            backdrops[i].Load(kTextDlgBackdropPictID + i);
    }
    SetDialogGWorld(textDialog);
    GetDialogItem(textDialog, 3, &itemType, &item, &box);
    textDialogTextHandle = TENew(&box, &box);
    (*textDialogTextHandle)->txFont = smallTextFont;
    (*textDialogTextHandle)->txSize = smallTextSize;
    TextFont((*textDialogTextHandle)->txFont);
    TextSize((*textDialogTextHandle)->txSize);
    TESetLineMetrics(textDialogTextHandle);
    TESetText(textShowString, strlen(textShowString), textDialogTextHandle);
    textHeight = TEMeasureText(textDialogTextHandle, box.right - box.left);
    GetDialogItem(textDialog, 3, &itemType, &item, &box);
    viewHeight = box.bottom - box.top;
    if (textHeight < viewHeight && descResourcePictId < kFirstResourceID) {
        if (textHeight >= kTextDlgMinHeight)
            minHeight = textHeight;
        else
            minHeight = kTextDlgMinHeight;
        win = GetDialogPortRect(textDialog);
        box = *win;
        slack = viewHeight - minHeight - kTextDlgSlackMargin;
        box.bottom -= slack;
        SizeDialogWindow(textDialog, (box.right - box.left),
                     (box.bottom - box.top), 1);
        pt.v = 0;
        pt.h = 0;
        pt.v += slack * kTextDlgRecentre;
        OffsetDialogWindow(textDialog, pt.h, pt.v, 1);
        GetDialogItem(textDialog, 1, &itemType, &item, &box);
        OffsetRect(&box, 0, -slack);
        SetDialogItem(textDialog, 1, itemType, item, &box);
        GetDialogItem(textDialog, 5, &itemType, &item, &box);
        OffsetRect(&box, 0, -slack);
        SetDialogItem(textDialog, 5, itemType, item, &box);
        GetDialogItem(textDialog, 6, &itemType, &item, &box);
        OffsetRect(&box, 0, -slack);
        SetDialogItem(textDialog, 6, itemType, item, &box);
        GetDialogItem(textDialog, 3, &itemType, &item, &box);
        box.bottom -= slack;
        SetDialogItem(textDialog, 3, itemType, item, &box);
        GetDialogItem(textDialog, 3, &itemType, &item,
                      &(*textDialogTextHandle)->destRect);
        GetDialogItem(textDialog, 3, &itemType, &item,
                      &(*textDialogTextHandle)->viewRect);
        TECalText(textDialogTextHandle);
    }
    UpdateAllPortDialogs();
    SetDialogGWorld(textDialog);
    lastTextScrollTime = ReadTick();
    FlushEvents(everyEvent - diskMask, 0);
    DialogText().ProbeRange();
    savedUp = canScrollTextUp;
    savedDown = canScrollTextDown;
    TextDialogUpdate();
    ShowTheCursor();

    if (gPause) {
        if (showingIntroText) {
            SetGWorldContainer(&loadPort);
            ForeColor(blackColor);
            PaintRect(&loadPort.bounds);
            SWSetPortToWindow(gSpriteWorld);
            ForeColor(blackColor);
            PaintRect(GetCWindowPortRect((WindowRef)gWind));
        } else {
            mainScreenBackgroundDirty = 1;
            MainScreenUpdate(1);
        }
    }
    if (screenFaded) {
        if (gPause) {
            if (showingIntroText) {
                SetGWorldContainer(&loadPort);
                ForeColor(blackColor);
                PaintRect(&loadPort.bounds);
                SWSetPortToWindow(gSpriteWorld);
                ForeColor(blackColor);
                PaintRect(GetCWindowPortRect((WindowRef)gWind));
            } else {
                mainScreenBackgroundDirty = 1;
                MainScreenUpdate(1);
            }
        } else {
            RefreshSAT();
            RedrawStatus(1);
        }
        FadeScreenIn();
    }

    do {
        if ((*textDialogTextHandle)->viewRect.top
            > (*textDialogTextHandle)->destRect.top)
            canScrollTextUp = 1;
        else
            canScrollTextUp = 0;
        if (gQuit != 0)
            done = 1;
        itemHit = dialog.Modal();
        if (itemHit == 1)
            done = 1;
        DialogText().Step(itemHit);
        if (itemHit == 4 && allowMap)
            MapFromDialog(textDialog);
        SetDialogGWorld(textDialog);
        DialogText().InvalidateChanged(savedUp, savedDown);
    } while (done == 0);

    for (i = 0; i < 3; i++)
        backdrops[i].Release();
    TEDispose(textDialogTextHandle);
    SWSetPortToWindow(gSpriteWorld);
    dialog.Close();

    if (gPause) {
        if (showingIntroText) {
            SetGWorldContainer(&loadPort);
            ForeColor(blackColor);
            PaintRect(&loadPort.bounds);
            SWSetPortToWindow(gSpriteWorld);
            ForeColor(blackColor);
            PaintRect(GetCWindowPortRect((WindowRef)gWind));
        } else {
            mainScreenBackgroundDirty = 1;
            MainScreenUpdate(1);
        }
    } else {
        UpdateAllPortDialogs();
    }
    if (descMovieName[0] != 0 && settings.noMovies == 0
        && (descFlags & kDescMovieAfter) != 0) {
        PlayQTMovie(descMovieName, (descFlags & kDescMovieDoubled),
                    (descFlags & kDescMovieFade));
        if (screenFaded) {
            if (gPause) {
                if (showingIntroText) {
                    SetGWorldContainer(&loadPort);
                    ForeColor(blackColor);
                    PaintRect(&loadPort.bounds);
                    SWSetPortToWindow(gSpriteWorld);
                    ForeColor(blackColor);
                    PaintRect(GetCWindowPortRect((WindowRef)gWind));
                } else {
                    mainScreenBackgroundDirty = 1;
                    MainScreenUpdate(1);
                }
            } else {
                UpdateAllPortDialogs();
                RefreshSAT();
                RedrawStatus(1);
            }
            FadeScreenIn();
        }
    }
}

static constexpr short kTextSlotItems[] = { 1, 5, 6 };

// FUNCTION: EVNOVA 0x00499440
// MAC: _TextDialogFilter i386 0x00051f77 ppc 0x0005a820
Boolean TextDialogFilter(DialogPtr unused, const DialogEvent &ev, short *itemHit)
{
    Point where;
    EventChar c;

    if (ev.what == DialogInput::Key) {
        if (QuickKeyHit(4, -1, -1, itemHit))
            return 1;
        c = ev.ch;
        if (IsReturnChar(c) || c == kEscapeChar) {
            *itemHit = 1;
            return 1;
        }
        if (DialogText().Key(c, itemHit))
            return 1;
    }
    if (ev.what == DialogInput::Click) {
        SetDialogGWorld(textDialog);
        where = ev.where;
        TextButtons buttons;
        if (DialogText().Press(where, itemHit, buttons))
            return 1;
        TextDialogUpdate();
        ButtonItemHit(buttons.Track(where), kTextSlotItems, itemHit);
        SetDialogGWorld(textDialog);
        buttons.Draw(-1);
        return 1;
    }
    if (ev.what == DialogInput::Idle && DialogText().Wheel(itemHit))
        return 1;
    return 0;
}

/* A dialog's background, laid over `box' of the current port and stretched as one picture: pics[0] at the
   top, pics[1] under it, pics[2] at the bottom -- or, with `onePicture', pics[0] over the whole box. */
void DrawDialogBackground(PicHandle pics[3], const Rect &box, bool onePicture)
{
    UI::ArtComposition background(CurrentGrafPort, box);
    Rect r;
    Rect top;

    if (onePicture) {
        if (pics[0] != NULL)
            DrawPicHandle(pics[0], (Rect *)&box);
        return;
    }
    if (pics[1] != NULL) {
        QDGetPictureBounds(pics[1], &r);
        OffsetRect(&r, -r.left, -r.top);
        if (pics[0] != NULL) {
            QDGetPictureBounds(pics[0], &top);
            OffsetRect(&top, -top.left, -top.top);
            OffsetRect(&r, 0, top.bottom);
        }
        OffsetRect(&r, box.left, box.top);
        DrawPicHandle(pics[1], &r);
    }
    if (pics[0] != NULL) {
        QDGetPictureBounds(pics[0], &r);
        OffsetRect(&r, -r.left, -r.top);
        OffsetRect(&r, box.left, box.top);
        DrawPicHandle(pics[0], &r);
    }
    if (pics[2] != NULL) {
        QDGetPictureBounds(pics[2], &r);
        OffsetRect(&r, -r.left, -r.top);
        OffsetRect(&r, box.left, box.bottom - (r.bottom - r.top));
        DrawPicHandle(pics[2], &r);
    }
}

// FUNCTION: EVNOVA 0x00499870
// MAC: _TextDialogUpdate i386 0x00051b40 ppc 0x0005a474
void TextDialogUpdate(void)
{
    PicHandle loaded;
    Rect r;
    short itemType;
    Handle item;

    SetGWorldContainer(&loadPort);
    ForeColor(blackColor);
    PaintRect(GetDialogPortRect(textDialog));
    DrawDialogBackground(textDialogBackgroundPic, *GetDialogPortRect(textDialog),
                         descResourcePictId >= kFirstResourceID);
    TextButtons().Draw(-1);
    TextFont(smallTextFont);
    TextSize(smallTextSize);
    if (descResourcePictId >= kFirstResourceID) {
        loaded = GetPicture(descResourcePictId);
        if (loaded != NULL) {
            GetDialogItem(textDialog, 2, &itemType, &item, &r);
            DrawPicHandle(loaded, &r);
            QDReleaseResource(loaded);
        }
    }
    GetDialogItem(textDialog, 3, &itemType, &item, &r);
    ForeColor(whiteColor);
    BackColor(blackColor);
    TEUpdate(&r, textDialogTextHandle);
    ForeColor(blackColor);
    BackColor(whiteColor);
    CopyLoadPortToDialog(textDialog);
}
