// COMPILE: -lang c++ -bool off
/* loadintfres.c -- LoadIntfResource (win 0x004cda50), the status-bar-panel loader called once from EnterGameFromMainScreen. Notes: Research/notes/loadintfres.md */
#include <stddef.h>
#include <string.h>

#include "macport.h"
#include "shipobj.h"
#include "resources.h"
#include "gameconst.h"
#include "render.h"
#include "burger.h"
#include "cefix.h"


#define kIntfRec ((IntfRec *)*intfFileHand)

#define GovtInterfaceID(g) \
    if (govtData[g].interfaceID >= kFirstResourceID) \
        id = govtData[g].interfaceID

#define CopyStatusRect(n, area) \
    p = &((IntfRec *)*rec)->area; \
    statusOffRect[n] = *p

// FUNCTION: EVNOVA 0x004cda50
// MAC: _LoadIntfResource i386 0x000735f2 ppc 0x0007c470
void LoadIntfResource(void)
{
    short type;
    short id;
    short picID;
    PicHandle pict;
    Rect *p;
    Handle rec;
    IntfRec **rec2;
    Rect pictRect;
    char fontName[256];

    id = kFirstResourceID;
    type = shipData->type;
    if (type >= 0 && type < kMaxShipTypes) {
        if (shipClass[type].inherentGovtB != -1) {
            GovtInterfaceID(shipClass[type].inherentGovtB);
        } else if (shipClass[type].inherentGovt != -1) {
            GovtInterfaceID(shipClass[type].inherentGovt);
        }
    }
    if (id != lastIntfResourceID) {
    intfFileHand = GetResource(kIntfType, id);
    if (intfFileHand != NULL) {
        LockAHandle(intfFileHand);

        statusColor[0].blue = (kIntfRec->brightText.rgb & 0xff) * 256;
        statusColor[0].green = (kIntfRec->brightText.rgb >> 8 & 0xff) * 256;
        statusColor[0].red = (kIntfRec->brightText.rgb >> 16 & 0xff) * 256;
        statusColor[1].blue = (kIntfRec->dimText.rgb & 0xff) * 256;
        statusColor[1].green = (kIntfRec->dimText.rgb >> 8 & 0xff) * 256;
        statusColor[1].red = (kIntfRec->dimText.rgb >> 16 & 0xff) * 256;
        statusColor[2].blue = (kIntfRec->brightRadar.rgb & 0xff) * 256;
        statusColor[2].green = (kIntfRec->brightRadar.rgb >> 8 & 0xff) * 256;
        statusColor[2].red = (kIntfRec->brightRadar.rgb >> 16 & 0xff) * 256;
        statusColor[3].blue = (kIntfRec->dimRadar.rgb & 0xff) * 256;
        statusColor[3].green = (kIntfRec->dimRadar.rgb >> 8 & 0xff) * 256;
        statusColor[3].red = (kIntfRec->dimRadar.rgb >> 16 & 0xff) * 256;
        statusColor[4].blue = (kIntfRec->shieldColor.rgb & 0xff) * 256;
        statusColor[4].green = (kIntfRec->shieldColor.rgb >> 8 & 0xff) * 256;
        statusColor[4].red = (kIntfRec->shieldColor.rgb >> 16 & 0xff) * 256;
        statusColor[5].blue = (kIntfRec->armorColor.rgb & 0xff) * 256;
        statusColor[5].green = (kIntfRec->armorColor.rgb >> 8 & 0xff) * 256;
        statusColor[5].red = (kIntfRec->armorColor.rgb >> 16 & 0xff) * 256;
        statusColor[6].blue = (kIntfRec->fuelFull.rgb & 0xff) * 256;
        statusColor[6].green = (kIntfRec->fuelFull.rgb >> 8 & 0xff) * 256;
        statusColor[6].red = (kIntfRec->fuelFull.rgb >> 16 & 0xff) * 256;
        statusColor[7].blue = (kIntfRec->fuelPartial.rgb & 0xff) * 256;
        statusColor[7].green = (kIntfRec->fuelPartial.rgb >> 8 & 0xff) * 256;
        statusColor[7].red = (kIntfRec->fuelPartial.rgb >> 16 & 0xff) * 256;

        rec = intfFileHand;
        CopyStatusRect(0, radarArea);
        CopyStatusRect(1, shieldArea);
        CopyStatusRect(2, armorArea);
        CopyStatusRect(3, fuelArea);
        CopyStatusRect(4, navArea);
        CopyStatusRect(5, weapArea);
        CopyStatusRect(6, targArea);
        CopyStatusRect(7, cargoArea);

        strcpy(fontName, ((IntfRec *)*rec)->statusFont);
        CtoPstr(fontName);
        GetFNum((StringPtr)fontName, &statusFontID);

        rec2 = (IntfRec **)intfFileHand;
        statusFontSize[0] = (*rec2)->statFontSize[0];
        statusFontSize[1] = (*rec2)->statFontSize[1];

        picID = (*rec2)->statusBkgnd;
        if (picID < kFirstResourceID)
            picID = kFirstResourceID;

#ifdef CEFIX /* F12.84 */
        CE_ScaleStatusRects();
#endif
        UnlockAHandle(intfFileHand);
        ReleaseResource(intfFileHand);
    } else {
        picID = kFirstResourceID;
    }

    pict = GetPicture(picID);
    if (pict != NULL) {
        SetGWorldContainer(&statusOffPort);
        RGBForeColor(&rgbBlack);
        PaintRect(&statusOffPort.bounds);
        QDGetPictureBounds(pict, &pictRect);
        OffsetRect(&pictRect, -pictRect.left, -pictRect.top);
#ifdef CEFIX /* F12.85 */
        CE_ScaleOffsetRect(&pictRect, -statusOffPort.bounds.left, -statusOffPort.bounds.top);
#else
        OffsetRect(&pictRect, -statusOffPort.bounds.left, -statusOffPort.bounds.top);
#endif
        DrawPicHandle(pict, &pictRect);
        QDReleaseResource(pict);
        UnsetGWorldContainer(&statusOffPort);
        RedrawStatus(1);
    }
    lastIntfResourceID = id;
    }
}

