/* shopgrid.cpp -- the outfitter's and shipyard's icon grid: icon cache and cell rects; dialog globals reset */
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

#define kPictCacheSlots 128

PictCacheSlot iconCache[kPictCacheSlots];
DialogPtr depthDialog;
DialogPtr loadingDialog;
DialogPtr outfitDescDialog;
Rect listPicOffRect[kPictCacheSlots];
GWorldContainer iconsPort;
short nagCount;

EXTERN_C_BEGIN

// FUNCTION: EVNOVA 0x00497b70
// MAC: _RetrieveMenuIcon i386 0x0004f425 ppc 0x000580cc
Boolean RetrieveMenuIcon(short picID, GWorldContainer *dest, Rect *destRect)
{
    int slot;
    int i;
    PicHandle pic;
    CGrafPtr savedPort;
    Boolean missed;
    uint32_t oldest;
    int32_t j;
    Rect *r;
    short id;

    slot = -1;
    i = 0;
    do {
        id = iconCache[(short)i].picID;
        if (id == -1)
            slot = i;
        if (picID == id) {
            CopyBits((BitMapPtr)GetGWCPixMap(&iconsPort), (BitMapPtr)GetGWCPixMap(dest),
                     &listPicOffRect[(short)i], destRect, 0, 0);
            iconCache[(short)i].stamp = ReadTick();
            return 1;
        }
        i = i + 1;
    } while ((short)i < kPictCacheSlots);
    if ((short)slot == -1) {
        oldest = iconCache[0].stamp;
        j = 0;
        do {
            if (iconCache[(short)j].stamp < oldest) {
                oldest = iconCache[(short)j].stamp;
                slot = j;
            }
            j = j + 1;
        } while ((short)j < kPictCacheSlots);
    }
    if ((short)slot < 0 || kPictCacheSlots <= (short)slot)
        slot = 0;
    GetPort(&savedPort);
    SetPort(iconsPort.world);
    ForeColor(blackColor);
    pic = GetPicture(picID);
    if (pic != 0) {
        r = &listPicOffRect[(short)slot];
        DrawPicHandle(pic, r);
        QDReleaseResource(pic);
        iconCache[(short)slot].picID = picID;
        iconCache[(short)slot].stamp = ReadTick();
        missed = 0;
    } else {
        r = &listPicOffRect[(short)slot];
        PaintRect(r);
        missed = 1;
    }
    SetPort(savedPort);
    CopyBits((BitMapPtr)GetGWCPixMap(&iconsPort), (BitMapPtr)GetGWCPixMap(dest),
             r, destRect, 0, 0);
    return missed;
}

#define kStripCells 128

// FUNCTION: EVNOVA 0x00499150
// MAC: _SetupListRects i386 0x00051a2c ppc 0x0005a340
void SetupListRects(Point *where)
{
    int i;
    Rect *cell;
    Rect *icon;

    for (i = 0; i < kGridCells; i++) {
        cell = &listScreenRect[i];
        cell->left = i % kGridCols * kGridCellWidth + where->h;
        cell->top = i / kGridCols * kGridCellHeight + where->v;
        cell->right = cell->left + kGridCellWidth + 1;
        cell->bottom = cell->top + kGridCellHeight + 1;
        icon = &listPicScreenRect[i];
        icon->left = (cell->left + cell->right - kGridThumbSize) / 2;
        icon->bottom = (cell->top + cell->bottom + kGridThumbSize / 2) / 2;
        icon->right = icon->left + kGridThumbSize;
        icon->top = icon->bottom - kGridThumbSize;
    }
    for (i = 0; i < kStripCells; i++) {
        listPicOffRect[i].left = kGridThumbSize * i;
        listPicOffRect[i].top = 0;
        listPicOffRect[i].right = listPicOffRect[i].left + kGridThumbSize;
        listPicOffRect[i].bottom = kGridThumbSize;
    }
}

// FUNCTION: EVNOVA 0x0049ed00
// MAC: _SetupDialogs i386 0x0005715d ppc 0x0005f500
void SetupDialogs(void)
{
    Rect r;
    short i;

    alertDialog = NULL;
    depthDialog = NULL;
    loadingDialog = NULL;
    tradeDialog = NULL;
    portDialog = NULL;
    outfitDialog = NULL;
    outfitDescDialog = NULL;
    shipyardDialog = NULL;
    shipDescDialog = NULL;
    infoDialog = NULL;
    missionDialog = NULL;
    barDialog = NULL;
    raceDialog = NULL;
    newsDialog = NULL;
    commDialog = NULL;
    haggleDialog = NULL;
    plunderDialog = NULL;
    mapDialog = NULL;
    captureDialog = NULL;
    for (i = 0; i < kPictCacheSlots; i++) {
        iconCache[i].picID = -1;
        iconCache[i].stamp = 0;
    }
    nagCount = 0;
    SetRect(&r, 0, 0, kGridThumbSize * 128, kGridThumbSize);
    CreateGWorldContainer(&iconsPort, 0, 0, &r);
}

// FUNCTION: EVNOVA 0x0049eec0
// MAC: _DisposeDialogs i386 0x000572b0 ppc 0x0005f668
void DisposeDialogs(void)
{
    if (iconsPort.world != NULL)
        DisposeGWorldContainer(&iconsPort);
}

EXTERN_C_END
