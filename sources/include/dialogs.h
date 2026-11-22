/* dialogs.h -- the game's own dialogs and the widgets they share. Notes: Research/notes/dialogs.md */
#ifndef DIALOGS_H
#define DIALOGS_H
#include "macport.h"
#include "shipobj.h"
#include "soundtool.h"

#ifdef __cplusplus
extern "C" {
#endif

/* desc-resource display flags */
#define kDescMovieAfter     0x0001
#define kDescMovieDoubled   0x0002
#define kDescMovieFade      0x0004

/* the trade center's and outfitter's quantity limit */
#define kMaxTradeQty   0x7d00

/* the grid filters' arrow keys: virtual key codes in 1.1.1, characters on win */
#define kGridLeftKey        kLeftArrowChar
#define kGridRightKey       kRightArrowChar
#define kGridUpKey          kUpArrowChar
#define kGridDownKey        kDownArrowChar
#define kGridPageUpKey      kPageUpChar
#define kGridPageDownKey    kPageDownChar

void CommDialogUpdate(void);
extern void MissionListDefinition(short lMessage, Boolean lSelect, Rect *lRect, Point *lCell,
                         short lDataOffset, short lDataLen, ScrollRec **lHandle);
EXTERN_CPP_BEGIN
extern Boolean reenterMissionDialog;
extern short adviceDisasterRandom;
extern Boolean willTakeBribe;
extern Boolean hasDemandedTribute;
extern Boolean commShipXeno;
extern short commDialogMode;
extern Boolean shipyardEgg;
EXTERN_CPP_END
extern void DisposeButtons(void);
extern void DoBarDialog(void);
/* what the player makes of a captured ship; None when the game quits during the choice */
enum class CaptureUse : unsigned char { None, Flagship, Escort };
extern CaptureUse DoCapturedShipAssignmentDialog(void);
extern void DoCommDialog(short ship);
extern void DoEscortDialog(ShipObj *ship);
extern void DoKeyDialog(void);
extern void DoMissionDialog(void);
extern void DoMissionInfoDialog(void);
extern void DoOutfitDialog(void);
extern void DoPlayerInfoDialog(void);
extern void DoPlunderDialog(void);
extern void DoPortDialog(SpobRecord *spob, short stellar);
extern void DoPrefsDialog(void);
extern void DoRaceDialog(void);
void DoShipyardDescription(short ship);
extern void DoShipyardDialog(void);
extern void DoSystemSearch(void);
extern void DoTradeDialog(void);
extern void DrawButton(CGrafPtr port, Rect *bounds, unsigned char *label,
                       short state, short selected, PicHandle pic);
extern Boolean EVTextInputDialog(unsigned char *prompt, unsigned char *initial,
                              short maxLen);
extern Boolean DoNewPilotNameDialog(Boolean *wantStrictPlay);
void HaggleDialogUpdate(void);
extern void InitButtons(void);
extern void CreateButtonPort(void);
extern void MissionDialogUpdate(void);
short DoShipCommPayment(void);
void OutfitDialogUpdate(void);
void PlunderDialogUpdate(void);
extern void PrefsDialogUpdate(void);
extern void RandomizeNewsDialog(void);
extern void ScrollMapDisplay(short dh, short dv);
extern void CueNextHyperRouteDest(void);
extern void SetDialogGWorld(DialogPtr dlg);
extern void DrawDialog(DialogPtr dlg);
void SetPlunderValues(void);
void SetupMissionInfoList(void);
extern void SetupPlayerInfoText(void);
extern void ShowTextDialog(Boolean allowMap, Boolean showMovie);
extern void ShipDescDialogUpdate(void);
void ShipyardDialogUpdate(); /* unprototyped: the 1.1.1 DoShipyardDialog passes it the dialog */
extern void DoNewsDialog(void);
extern void BarDialogUpdate(void);
void CapturedAssignmentDialogUpdate(void);
void NewsDialogUpdate(void);
void EscortDialogUpdate(void);
void KeyDialogUpdate(void);
extern void SingleMissionDialogUpdate(void);
void MissionInfoDialogUpdate(void);
void PlayerInfoDialogUpdate(void);
void RaceDialogUpdate(void);
void PlanetCommDialogUpdate(void);
extern void DoPlanetCommDialog(short stellar);
extern void PortDialogUpdate(void);
/* after a sub-dialog closed over `self`: redraw the spaceport dialogs and flag `self` for redraw */
void RedrawAfterSubDialog(DialogPtr self);
/* the galaxy map, not in hypergate mode; the nav target is put back afterwards */
void OpenMapKeepingNav(void);
/* OpenMapKeepingNav, then RedrawAfterSubDialog(self) */
void MapFromDialog(DialogPtr self);
/* the player info dialog, then RedrawAfterSubDialog(self) */
void PlayerInfoFromDialog(DialogPtr self);
/* whether any active mission is not hidden */
Boolean AnyMissionShown(void);
/* if AnyMissionShown: the mission info dialog, then RedrawAfterSubDialog(self) */
void MissionInfoFromDialog(DialogPtr self);
/* a mission offer from `venue`; RedrawAfterSubDialog(self) when one was made */
void OfferFromDialog(short venue, DialogPtr self);
/* filter checks: each sets *itemHit and returns 1 when it applies */
/* absquatulateStr is set (forced takeoff): leaveItem */
Boolean LeaveRequested(short leaveItem, short *itemHit);
/* missionOfferTime has passed: offerItem, and missionOfferTime is set 30-59 ticks ahead */
Boolean OfferDue(short offerItem, short *itemHit);
/* QuickKeyCheckHandler, then the map, player info or missions key while that dialog is closed; -1 skips a key */
Boolean QuickKeyHit(short mapItem, short infoItem, short missionsItem, short *itemHit);
extern void TextDialogUpdate(void);
void TradeDialogUpdate(void);
EXTERN_CPP_BEGIN
extern unsigned char adviceString[];
extern PicHandle barBackgroundPic;
extern char barDescText[];
extern PicHandle barPic;
extern short basePrice[];
extern Boolean boughtID;
extern Boolean boughtMap;
extern RGBColor buttonColor[3];
extern short buttonFontID;
extern short buttonFontSize;
extern GWorldContainer buttonImagePort;
extern GWorldContainer buttonMaskPort;
#define kButtonPieceCount 9
extern Rect buttonPieceRect[kButtonPieceCount];
extern GWorldContainer buttonPort;
extern Boolean canBuy;
extern Boolean canRefuseMission;
extern Boolean canScrollTextDown;
extern Boolean canScrollTextUp;
extern Boolean canSell;
extern Boolean canSellEscort;
extern Boolean canUpgradeEscort;
extern PicHandle captureBackgroundPic;
extern short captureOdds;
extern PicHandle commBackgroundPic;
extern unsigned char commResponseString[];
extern ShipObj *commShip;
extern PicHandle commShipPic;
extern short descResourcePictId;
extern Boolean didBuyShip;
extern Boolean didOutfit;
extern short dockedPortItems[];
extern short dockedPortShipTypes[];
extern uint32_t ambientNoiseDelay;
extern Sound *planetSnd;
extern int32_t initialHaggleVal;
extern Rect listScreenRect[];
extern unsigned char stellarTypeStr[];
extern float bribeFlux;
extern char descTextPtr[];
extern PicHandle newsPic;
extern Rect listPicScreenRect[];
extern char dialogDescTextPtr[];
extern short lastClickItem;
extern short selMissionInfoListItem;
extern PicHandle missionInfoBackgroundPic;
extern PicHandle outfitBackgroundPic;
extern short selOutfitListItem;
extern short keyDialogSel;
extern short selOutfitItem;
extern short selShipyardItem;
extern unsigned char shipCommNames[768][256];
extern PicHandle shipDescBackgroundPic;
extern PicHandle bigShipPic;
extern short selShipListItem;
extern short shipyardScroll;
extern unsigned char shipyardLongNames[768][256];
extern PicHandle shipyardBackgroundPic;
extern Boolean canDoMissions;
extern DialogPtr portDialog;
extern PicHandle portBackgroundPic;
extern GWorldContainer planetPicContainer;
extern unsigned char textInputString[];
extern char textShowString[];
extern short gTimesFont;
extern unsigned char govtShortNames[256][256];
extern short greenMissionSyst;
extern PicHandle haggleBackgroundPic;
extern short haggleMode;
extern int32_t haggleVal;
extern Boolean hatesPlayer;
extern PicHandle keyPic;
extern short lastRaceWinner;
extern uint32_t lastTextScrollTime;
extern unsigned char lcOutfitName[512][256];
extern unsigned char lcPluralOutfitName[512][256];
extern Boolean mainScreenBackgroundDirty;
extern DialogPtr missionDialog;
extern short missionsOutside;
extern uint32_t missionOfferTime;
extern Boolean multiArrowsOnSingleSystem;
extern short nextPortMission;
extern DialogPtr outfitDialog;
extern short outfitScroll;
extern PicHandle playerInfoBackgroundPic[3];
extern short selPlayerInfoMode;
extern char playerInfoText[3][4096];
extern PicHandle plunderBackgroundPic;
extern Boolean portBuysNoOutfits;
extern PicHandle previewPic;
extern short previewPicID;
extern float priceFluxMult[2];
extern int32_t raceBetAmount;
extern short raceCount;
extern PicHandle raceBackgroundPic;
extern short racePick;
extern short raceWinner;
EXTERN_CPP_END
extern RGBColor *redColor;
extern RGBColor redRGB;
EXTERN_CPP_BEGIN
extern Boolean releaseCommEscort;
extern short responseRandom;
extern short salvageAmmoQty;
extern DialogPtr tradeDialog;
extern int32_t salvageCredits;
extern short salvageFuelQty;
extern short salvageQty;
extern short salvageType;
extern short selMissionListItem;
EXTERN_CPP_END
EXTERN_CPP_BEGIN
extern short selTradeItem;
extern short shipBuyMode;
extern unsigned char shipClassStr[];
extern unsigned char shipGovtStr[];
extern DialogPtr shipyardDialog;
extern RGBColor shopGridColor[2];
extern Boolean showAbortMissionButton;
extern PicHandle textDialogBackgroundPic[3];
extern PicHandle tradeBackgroundPic;
EXTERN_CPP_END
EXTERN_CPP_BEGIN
extern DialogPtr barDialog;
extern DialogPtr captureDialog;
extern DialogPtr commDialog;
extern DialogPtr haggleDialog;
extern DialogPtr infoDialog;
extern DialogPtr mapDialog;
extern DialogPtr missionInfoDialog;
extern DialogPtr plunderDialog;
extern DialogPtr raceDialog;
extern DialogPtr keyDialog;
extern DialogPtr prefsDialog;
extern DialogPtr textDialog;
extern DialogPtr qtyDialog;
extern DialogPtr depthDialog;
extern DialogPtr loadingDialog;
extern DialogPtr outfitDescDialog;
EXTERN_CPP_END

extern void EVNoteAlert(const unsigned char *text);
EXTERN_CPP_BEGIN
extern PicHandle raceButtonPic[4][4];
EXTERN_CPP_END

extern Boolean EVYesNoDialog(unsigned char *prompt);
EXTERN_CPP_BEGIN
extern Boolean gQuit;
extern uint32_t lastClickTime;
extern Boolean screenFaded;
EXTERN_CPP_END

EXTERN_CPP_BEGIN
extern ShortPoint menuTopCorner;
EXTERN_CPP_END

extern Boolean TextDialogFilter(DialogPtr unused, const DialogEvent &ev, short *itemHit);
EXTERN_CPP_BEGIN
extern TEHandle offerTextHandle;
EXTERN_CPP_END
#define kMissionButtonLabelCount 2
EXTERN_CPP_BEGIN
extern unsigned char missionButtonLabel[kMissionButtonLabelCount][256];
EXTERN_CPP_END
EXTERN_CPP_BEGIN
extern TEHandle textDialogTextHandle;
EXTERN_CPP_END
extern short DoHaggleDialog(Boolean raise, short chance);
extern void ScrollDialogText(DialogPtr dlg, short item, TEHandle hTE,
                             short dv, Boolean force);
extern void PlayQTMovie(unsigned char *name, unsigned char doubled,
                        unsigned char fade);
extern int32_t DoQuantityDialog(int32_t maxCount, short mode);
EXTERN_CPP_BEGIN
extern ScrollRec **missionList;
EXTERN_CPP_END

extern void DrawDialogBackground(PicHandle pics[3], const Rect &box, bool onePicture);  /* tradedlg.cpp */

#ifdef __cplusplus
}
#endif

#endif
