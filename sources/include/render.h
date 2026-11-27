/* render.h -- sprites, GWorlds, the status bar and everything that draws. Notes: Research/notes/render.md */
#ifndef RENDER_H
#define RENDER_H
#include <string>
#include "burger.h"
#include "spriteworld.h"
#include "movie.h"
#include "soundtool.h"

namespace UI {
class Painter;
class Surface;
}

static __inline void CopyRect(Rect *d, const Rect *s)
{
    d->top = s->top;
    d->left = s->left;
    d->bottom = s->bottom;
    d->right = s->right;
}

extern void DeltaIntroProgressBar(double amount);
extern void CenterRect(Rect *r, Rect *bounds);
extern void CreateGWorldContainerFromPictResource(GWorldContainer *gwc, int32_t flags,
                          short depth, short picID);
extern SpriteRec *weapTargetSprite[4];
extern SpriteRec *navTargetSprite[4];
extern SpriteRec *masterExplodSprite[64];
extern SpriteRec *masterSmokeSprite[8];
extern SpriteRec *masterBoxSprite[5];
extern Rect statusOffRect[8];
extern unsigned char targKeyOnNow;
extern ShipClassID lastTargetedShipClass;
extern unsigned char lastDebugTargInfoState;
extern Boolean gUpdateFlag;
extern Boolean regNoticeShown;
extern void DisposeMainScreenAnim(void);
extern void CreateGWorldContainer(GWorldContainer *gwc, int32_t flags, short depth,
                                  Rect *bounds);
extern void DisposeGWorldContainer(GWorldContainer *gwc);
extern void DoMainScreen(void);
extern Boolean RetrieveMenuIcon(short picID, const UI::Surface &dest, Rect *destRect);
extern void DrawIntroProgressBar(void);
extern void DrawStatus(void);
extern void DrawStatusCargo(void);
extern void DrawStatusFuel(void);
extern void DrawStatusNav(void);
extern void DrawStatusRadar(Boolean highlight);
extern void DrawStatusShield(void);
extern void DrawStatusTarg(void);
extern void DrawStatusWeap(void);
extern void EndLoopingMovie(void);
extern SWFrameRec *CommOverlay(void);
extern void ResetCommRect(void);
extern void SetupListRects(Point *where);
extern void InitOffscreen(void);
extern void HideAmbrosiaLogo(void);
extern void PostAnimateHandler(void);
extern Boolean GetSpriteInfo(short id, short *spriteID, short *maskID, short *xSize,
                          short *ySize, short *xTiles, short *yTiles);
extern short GetStellarSpriteXSize(short id);
extern void HandleStellarSprites(void);
extern void HideTheCursor(void);
extern void InitColors(void);
extern void InitIntroProgressBar(void);
extern int KeyboardGet(void);
extern short LoadExtendedShipSprites(short shipType);
extern void LoadIntfResource(void);
extern void LoadMainScreenAnim(void);
extern void PlaceMainScreenAnim(void);
extern void MainScreenAppear(void);
extern void LoadMasterStellarSprite(short type);
extern void LoadShipSprites(void);
extern void LoadSprites(void);
extern void PlaceAlertSprite(void);
extern void LoopingMovieIdle(void);
extern void MainScreenAnimDraw(void);
extern void MainScreenChoice(short which);
extern void MainScreenAppearStart(void);
void MainScreenKey(char key, int32_t a, int32_t b);
extern void MainScreenRolloverDraw(void);
extern void MainScreenUpdate(short which);
extern void ShowMainScreen(void);
extern int MouseReadButtons(void);
extern void MouseReadPosition(int32_t *h, int32_t *v);
/* one wheel notch taken: 1 up, -1 down, 0 none */
extern int MouseTakeWheel(void);
extern void MouseFlushWheel(void);
extern void PlayMainScreenClickSound(void);
extern void PrepareAllSprites(void);
extern void SetupInterfaceBackground(void);
extern void ShowAmbrosiaLogo(void);
extern void ShowIntroScreen(void);
extern void ShowSplashScreen(void);
/* the load page black with the picture (NULL: none) centred; returns where it lies */
extern Rect PaintLoadPagePicture(PicHandle pic);
/* the screen black with the load page's part `from' at `to' (the menu's scale) */
extern void ShowLoadPage(const Rect &from, const Rect &to);
extern void DisposeMainScreenSounds(void);
extern void MainScreenAnimIdle(void);
extern void MainScreenRolloverIdle(void);
extern void TradeDialogTabKey(Boolean up);
extern RGBColor *armorStatusColor;
extern Rect commRect;
extern Rect escortMenuRect;
extern Rect floatingMapRect;
extern RGBColor *fuelStatusColor[2];
extern AnimDelta slideFrameSize[3];
extern short slideFrameCount[];
extern short slideIndex[];
extern GWorldContainer logoPort;
extern GWorldContainer slidePort[3];
extern uint32_t lastRolloverAnimTime;
extern short rolloverAlpha;
extern short rolloverIndex;
extern Handle intfFileHand;
extern uint32_t lastLogoAnimTime;
extern uint32_t lastSlideAnimTime;
extern short logoFrameCount;
extern short logoIndex;
extern short gMainPicHeightDelta;
extern short gMainPicWidthDelta;
extern short planetPicId;
extern std::string weapName[256];
extern Rect interfacePicRect;
extern short interferenceThisFrame;
extern short lastIntfResourceID;
extern int32_t lastFPSUpdateTime;
extern uint32_t lastRadarUpdateTime;
extern short lastTargetedShipShields;
extern RGBColor ldefColors[3];
extern ShortPoint logoDestPoint;
extern Rect logoDestRect;
extern ShortPoint mainMenuCenter;
extern Rect listPicOffRect[];
extern Boolean preventStatusCopy;
extern RGBColor progressColor[3];
extern double progressCount;
extern double progressMax;
extern Rect progressRect;
extern short radarCount;
extern RGBColor *radarStatusColor[2];
extern ShortPoint rolloverDestPoint;
extern Rect rolloverDestRect;
extern ShortPoint screenCenter;
extern RGBColor *shieldStatusColor;
extern short shipSpriteLoaded[][6];
extern short showFloatingMap;
extern Boolean showingIntroText;
extern ShortPoint slideDestPoint[3];
extern short smallTextFont;
extern short smallTextSize;
extern RGBColor *statusBrightColor;
extern RGBColor statusColor[8];
extern RGBColor *statusDimColor;
extern short statusFontID;
extern GWorldContainer statusOffPort;
extern uint32_t tix;

extern Movie loopingMovie;

extern Sound *mainScreenSnd[4];
extern Rect slideDestRect[3];
extern void *staticPat[10];
extern void *targetInfoPic[768];

extern void RefreshSAT(void);
extern void RedrawStatus(Boolean legacyPipeline);
extern void ShowTheCursor(void);
extern short GetMasterShipSpriteXSize(short id);
extern short gSoundVolume;

extern short statusFontSize[2];
extern void DoPlayGameWork(unsigned char advance);
extern void HandleSpriteMurkiness(short x, short y, SpriteRec *sprite);
extern void FloaterDrawCallback(SpriteWorldPtr world);
extern void StartLoopingMovie(const std::string &name, DialogPtr dlg, const Rect &box);
/* the looping movie's frame as last decoded, at its box */
extern void DrawLoopingMovie(UI::Painter &p);
extern void PreloadPortDialog(SpobRecord *spob, short stellar);

#endif
