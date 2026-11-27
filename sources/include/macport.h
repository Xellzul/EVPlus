/* macport.h -- Mac OS Toolbox types and shims for the Windows port. Notes: Research/notes/macport.md */
#include "gametypes.h"
#include "burgertypes.h"

#ifndef MACPORT_H
#define MACPORT_H

typedef unsigned char *StringPtr;

/* QuickDraw basics (partial) */
typedef struct Point {
    short v;
    short h;
} Point;

typedef struct CGrafPort *CWindowPtr;
typedef struct ControlRecord {
    int32_t kind;
    int32_t itemNo;
    int32_t value;
    int32_t minValue;
    int32_t maxValue;
    Rect box;
    void *data;
    DialogPtr owner;
    Boolean enabled;
} ControlRecord;
typedef struct ControlRecord *ControlPtr;
/* the Toolbox names the port's records answer to */
typedef ControlPtr ControlRef;
typedef Image_t *CIconHandle;
typedef Image_t *PixPatHandle;

struct PlatTarget;

typedef struct PixMap {
    Rect bounds;
    int32_t pixelSize;
    void *baseAddr;
    unsigned char *ctab;
    int32_t width;
    int32_t height;
    int32_t rowBytes;
} PixMap, *PixMapPtr;
typedef struct CGrafPort {
    Boolean offscreen;
    char filler1[3];
    PixMap pix;
    int32_t depth;
    void *pixels;                   /* the port's own pixel memory; NULL for a port over pixels it does not own */
    short lastFont;                 /* the font, size and style text was last drawn or measured with here: */
    short lastSize;                 /* what an unknown font number leaves in use (port/qdtext.cpp) */
    short lastFace;
    Boolean hasLastFont;
    RGBColor foreColor;
    RGBColor backColor;
    Point pnLoc;
    Point pnSize;
    short txFont;
    short txFace;
    short txSize;
    int32_t pnDeviceOffset;         /* text: the pen's device x minus pnLoc.h's; 0 after any pen move */
    struct PlatTarget *target;      /* the Painter's GPU canvas for this port, pix its size; NULL: its pixels */
} CGrafPort, *CGrafPtr;
typedef CWindowPtr WindowRef;
typedef struct BitMap *BitMapPtr;

typedef struct GWorldContainer {
    CGrafPtr world;
    Rect bounds;
} GWorldContainer;

extern void GetIndString(unsigned char *str, int strListID, int index);
extern void nv_UpdateDialogWindow(int all);
extern GWorldContainer iconsPort;
extern void SelectDialogItemText(DialogPtr dlg, int32_t item, int32_t start, int32_t end);
extern void FrameRect(const Rect *r);
extern void GetFNum(const unsigned char *name, short *familyID);
#if EVPLUS_TEST_PAGES
extern void DebugSoundTest(void);                 /* port/soundtest.cpp: the mixer capture */
extern void DebugGpuTest(void);                   /* port/gputest.cpp: runs when EVPLUS_GPUTEST is set */
extern void PaintOval(Rect *r);                   /* the GPU test's mask */
#endif
extern void DrawTextRun(const void *text, uint32_t byteCount, Rect *box,
                        Boolean clipped, int height);
/* bytes start..end - 1 of a text drawn in `color' */
struct TextSpan {
    uint32_t start;
    uint32_t end;
    RGBColor color;
};
extern int MeasureTextSpans(const void *text, uint32_t byteCount, Rect *box);
extern CGrafPtr CurrentGrafPort;
extern void CopyBits(BitMapPtr srcBits, BitMapPtr dstBits,
                     const Rect *srcRect, const Rect *dstRect,
                     int32_t mode, void *maskRgn);
extern BitMapPtr GetPortBitMapForCopyBits(CGrafPtr port);
/* a Point in hand: the mac passes it whole, the win port's PtInRect takes h and v */
#define PtInRectPoint(pt, r) PtInRect((pt).h, (pt).v, r)
extern short NewGWorld(CGrafPtr *out, int depth, const Rect *bounds,
                       void *theGDH);
extern PixMapPtr GetGWorldPixMap(CGrafPtr world);
extern void QDGetCIconBounds(Image_t *icon, Rect *r);
extern void (*gArtGone)(const Image_t *art);     /* set: told of each picture, icon and pattern released */
extern void (*gPortGone)(CGrafPtr port);         /* set: told of each port disposed (DisposeGWorld) */

extern void CopyBitsNone(PixMapPtr src, PixMapPtr dst,
        Rect *srcRect, Rect *dstRect,
        int32_t xStep, int32_t yStep, int32_t xFrac, int32_t yFrac);
extern void CopyBitsCopy8(PixMapPtr src, PixMapPtr dst,
        Rect *srcRect, Rect *dstRect,
        int32_t xStep, int32_t yStep, int32_t xFrac, int32_t yFrac);
extern void CopyBitsOr8(PixMapPtr src, PixMapPtr dst,
        Rect *srcRect, Rect *dstRect,
        int32_t xStep, int32_t yStep, int32_t xFrac, int32_t yFrac);
extern void CopyBitsCopy16(PixMapPtr src, PixMapPtr dst,
        Rect *srcRect, Rect *dstRect,
        int32_t xStep, int32_t yStep, int32_t xFrac, int32_t yFrac);
extern void CopyBitsAddOver16(PixMapPtr src, PixMapPtr dst,
        Rect *srcRect, Rect *dstRect,
        int32_t xStep, int32_t yStep, int32_t xFrac, int32_t yFrac);
extern void CopyBitsAdMax16(PixMapPtr src, PixMapPtr dst,
        Rect *srcRect, Rect *dstRect,
        int32_t xStep, int32_t yStep, int32_t xFrac, int32_t yFrac);
extern void CopyBitsScale8(PixMapPtr src, PixMapPtr dst, Rect *srcRect, Rect *dstRect,
        int32_t xStep, int32_t yStep, int32_t xStart, int32_t yStart);
/* blitdirect.cpp: CopyBits procs for direct-colour sources, at the same size or stretched */
extern void BlitDirect(PixMapPtr src, PixMapPtr dst, Rect *srcRect, Rect *dstRect,
                       int32_t xStep, int32_t yStep, int32_t xFrac, int32_t yFrac);
/* the bicubic stretch on 32-bit premultiplied ARGB pixel maps, the alpha stretched with the colour */
extern void BlitScaleArgb(PixMapPtr src, PixMapPtr dst, const Rect *srcRect, const Rect *dstRect);

extern CGrafPtr GetCWindowGrafPort(CWindowPtr window);
extern Rect *GetCWindowPortRect(CWindowPtr window);
extern PixMapPtr GetCWindowPixMap(CWindowPtr window);

typedef struct SWFrameRec {
    CGrafPtr framePort;
    PixMapPtr framePixHndl;
    char *frameBaseAddr;
    uint32_t frameRowBytes;
    Boolean isFrameLocked;
    short frameDepth;
    Rect frameRect;
    short hotSpotH;
    short hotSpotV;
    Rect collisionInset;
    CGrafPtr maskPort;
    PixMapPtr maskPixHndl;
    char *maskBaseAddr;
    Boolean sharesGWorld;
    Boolean usesCollisionInset;
    unsigned short useCount;
    unsigned short numScanLines;
    unsigned short pixelShift;
    uint32_t *scanLinePtrArray;
    void *rleDataH;
    void *rleTokenStart;
} SWFrameRec;

typedef SWFrameRec *FramePtr;

typedef void (*DrawProcPtr)(SWFrameRec *srcFrameP, SWFrameRec *dstFrameP,
                            Rect *srcRect, Rect *dstRect);

/* The sprite world: the layers of sprites the game moves and animates and the space view's rect. The game draws
   it through gSWDraw (viewdraw.cpp). */
typedef struct SpriteWorldRec {
    struct SpriteLayerRec *headSpriteLayerP;
    struct SpriteLayerRec *tailSpriteLayerP;
    Rect windRect;                  /* the space view, at 0,0 */
    struct CGrafPort *windowPort;   /* the game window's port */
    short pixelDepth;
    Boolean frameHasOccurred;
    Boolean fastBlit;
} SpriteWorldRec, *SpriteWorldPtr;


typedef struct SpriteLayerRec {
    struct SpriteLayerRec *nextSpriteLayerP;
    struct SpriteRec *headSpriteP;
    struct SpriteRec *tailSpriteP;
} SpriteLayerRec, *SpriteLayerPtr;
typedef struct RLEPixelData {
    unsigned short width;
    unsigned short height;
    unsigned short depth;
    unsigned short palette;
    unsigned short frames;
    unsigned short reserved1;
    unsigned short reserved2;
    unsigned short reserved3;
} RLEPixelData, *RLEPixelDataPtr, **RLEPixelDataHdl;
#define kRLEResourceHeaderSize  sizeof(RLEPixelData)

#define noErr           0
#define memFullErr      (-108)
#define resNotFound     (-192)

#define SW_ROWBYTES(frame, rows) \
        ((frame)->scanLinePtrArray[(rows) - (frame)->frameRect.top])
#define SW_PIXELBYTES(frame, pixels)    ((pixels) << (frame)->pixelShift)

#define ABS(x) (((x)<0) ? (-(x)) : (x))

#define SW_PIXELPTR(frame, x, y) \
    ((frame)->frameBaseAddr + SW_ROWBYTES(frame, y) + SW_PIXELBYTES(frame, x))

#define SW_MIN(a, b)            ((a) < (b) ? (a) : (b))
#define SW_MAX(a, b)            ((a) > (b) ? (a) : (b))

/* a sprite frame's RLE stream: a token word carries a kTokenShift-shifted opcode and a byte count */
#define kEndShapeToken          0
#define kLineStartToken         1
#define kDrawPixelsToken        2
#define kSkipPixelsToken        3
#define kSingleColorToken       4
#define kTokenShift             24
#define kCountMask              0x00ffffff

typedef struct SpriteRec SpriteRec, *SpritePtr;

typedef void (*CollideProcPtr)(SpritePtr srcSpriteP, SpritePtr dstSpriteP,
                               Rect *sectRect);

struct SpriteRec {
    SpriteLayerRec *parentSpriteLayerP;
    SpritePtr nextSpriteP;
    SpritePtr prevSpriteP;
    unsigned char isVisible;
    Rect destFrameRect;
    SWFrameRec **frameArray;
    SWFrameRec *curFrameP;
    int32_t numFrames;
    int32_t maxFrames;
    int32_t curFrameIndex;
    CollideProcPtr spriteCollideProc;
    CGrafPtr sharedPictGWorld;
    CGrafPtr sharedMaskGWorld;
    short translucency;
    short tintRed;
    short tintGreen;
    short tintBlue;
    short tintMode;
    uint32_t tintColor;
    short scaledWidth;
    short scaledHeight;
    int32_t userData;
    char **rleDataH;
    Boolean collisionsDisabled;
    Boolean ownsRLEData;
    Boolean isEmptySprite;
};

typedef struct TERec {
    char                  *text;
    int32_t                   textLength;
    short                  txFont;
    short                  txSize;
    Rect                   destRect;
    Rect                   viewRect;
    int32_t                   height;
} TERec, **TEHandle;

#define TETextHeight(te) ((*(te))->height)

struct ScrollRec;
typedef void (*ListDrawProcPtr)(short lMessage, Boolean lSelect, Rect *lRect,
                                Point *lCell, short lDataOffset, short lDataLen,
                                struct ScrollRec **lHandle);
typedef struct ScrollRec {
    CGrafPtr port;
    Rect rView;
    Rect dataBounds;
    Point indent;
    Point cellSize;
    ListDrawProcPtr drawProc;
    Boolean hasScrollBar;
    int32_t scroll;
    char **cells;
    short *cellArray;
    short maxIndex;
    Point lastClick;
    int32_t clikTime;
    char unk3c[4];
    Boolean drawIt;
} ScrollRec;

typedef struct DialogRecord {
    CGrafPtr port;
    Rect gwBounds;
    Rect bounds;
    Rect portRect;
    int32_t itemCount;
    int32_t editItem;
    int32_t defaultItem;
    int32_t cancelItem;
    void *items;
    void *updateProc;
    void *eventProc;
    int32_t needsUpdate;
    int32_t visible;
} DialogRecord;

typedef struct Particle {
    short lifeRemaining;
    int32_t horizLoc;
    int32_t vertLoc;
    int32_t oldHorizLoc;
    int32_t oldVertLoc;
    int32_t horizSpeed;
    int32_t vertSpeed;
    int32_t shade;
    uint32_t color;
} Particle, *ParticlePtr;

/* Toolbox GetTime shim — port implementation at 0x004bbb10 */
extern void GetTime(DateTimeRec *dtr);
extern void IncrementDate(DateTimeRec *date);

extern char *PtoCstr(unsigned char *str);
extern unsigned char *CtoPstr(char *str);

#define kBlue555  0x001f
#define kGreen555 0x03e0
#define kPictType 0x50494354   /* 'PICT' */
#define kSoundType 0x736e6420  /* 'snd ' */
#define kStringType 0x53545220 /* 'STR ' */
#define kStringListType 0x53545223UL /* 'STR#' */
#define kRed555   0x7c00


#define shiftKey    0x0200
#define optionKey   0x0800
#define controlKey  0x1000
#define diskMask    0x0080
#define everyEvent  0xffff

/* a byte pointer stepped through a cast lvalue: gcc then keeps the pointer in its stack slot, which 1.1.1's bytes witness */
#define PtrAddBytes(p, n) (*(char **)&(p) += (n))

/* a key character from the key queue, 0..255 */
typedef int EventChar;

/* a character held in a local across tolower's calls in the win port and 1.0.10; 1.1.1 reads its source again */
#define HeldChar(held, src) (held)

#define kRLEDResType 0x726c9144UL

extern void *NewPortPixels(int width, int rows, int bits);
extern void DropPortPixels(CGrafPtr port);       /* an offscreen port drawn through its GPU target alone */
extern short Alert(short alertID, void *filterProc);
#define StopAlert(id, filter) Alert(id, filter)
extern void BackColor(const RGBColor *color);
extern Boolean Button(void);
extern void BusyIdle(void);
extern void Delay(uint32_t numTicks, uint32_t *finalTicks);
extern void DisposeCIcon(Image_t *icon);
extern void DrawCString(const char *text);
extern void DrawPicHandle(PicHandle pic, Rect *dstRect);
extern void EraseRect(Rect *r);
extern void ExitToShell(void);
extern void DisposePixPat(Image_t *pat);
extern void InitDataFiles(void);
extern void MakeAlias(const char *path);
extern short EVOpenResourceFile(const char *path);
extern void SwapEndianByDescriptor(unsigned char *data, unsigned short *desc);
extern void LoadMenuListItem(ControlRecord *rec, int32_t menuID, unsigned char *promptTitle,
                         int32_t resType);
extern void LoadControlResource(ControlRecord *rec, uint32_t resID);
extern int32_t PopUpListSelect(DialogPtr dlg, ControlRecord *rec);
extern void EditTextKey(ControlRecord *rec, int32_t key, int32_t mods);
void DrawBevel(Rect *r, const RGBColor *fill, const RGBColor *light, const RGBColor *dark);
extern void LUpdate(void *list);
extern void WinGameActivate(int on);
extern void WinGameMinimized(void);
extern void WinGameWindowMode(int windowed);
extern void WinGameClose(void);
extern void WinGameInputSeen(void);
extern void ShutdownWinGame(void);
extern void FillRectWithColor(Rect *r, RGBColor *color);
extern short FindFirstCoLocatedSystem(short sys);
extern short FindPriorLoadedShipSprite(short value, short count, short field);
extern void DisposeClonedSprite(SpritePtr *p);
extern void FlushEvents(unsigned short whichMask, unsigned short stopMask);
extern CWindowPtr FrontWindow(void *window);
extern Image_t *GetCIcon(short iconID);
extern Rect *GetDialogPortRect(DialogPtr dlg);
#define kMinFontSize 10                     /* GetFont makes a smaller font this size */
struct FontSpec;
extern int GetFont(short fontID, short size, short style, struct FontSpec *spec);
/* what ModalDialog hands the filter on each pass: one key, one click, or nothing (Idle) */
enum class DialogInput : unsigned char { Idle, Key, Click };
struct DialogEvent {
    DialogInput what;
    EventChar ch;                       /* Key: the key character; 0 for Shift, Ctrl or Alt */
    Point where;                        /* the mouse now, dialog-local */
    int32_t mods;                       /* shiftKey, controlKey, optionKey held now */
    uint32_t when;                      /* TickCount() at this pass */
};
/* the procs a dialog record carries: its update (draw) proc and its event filter */
typedef void (*DialogUpdateProc)(void);
typedef Boolean (*DialogEventProc)(DialogPtr dlg, const DialogEvent &ev, short *itemHit);

extern void GetGlobalMouse(Point *pt);
extern void GetMouse(Point *pt);
extern DialogPtr GetNewDialog(short dlogID, DialogUpdateProc updateProc,
                              DialogEventProc eventProc);

/* one item of NewDialogFromItems, as a DITL holds it */
#define kDitlUserItem       0
#define kDitlButton         4
#define kDitlCheckBox       5
#define kDitlStaticText     8
#define kDitlEditText       0x10
#define kDitlPicture        0x40

typedef struct DialogItemSpec {
    unsigned char kind;             /* kDitl* */
    short top;
    short left;
    short bottom;
    short right;
    const char *text;               /* a button's, check box's or static text's */
    short resID;                    /* a picture's PICT */
} DialogItemSpec;

extern DialogPtr NewDialogFromItems(short width, short height, const DialogItemSpec *items, short count,
                                    DialogUpdateProc updateProc, DialogEventProc eventProc);
extern PicHandle GetPicture(int picID);
extern Image_t *GetPixPat(short id);
extern void GetPort(CGrafPtr *port);
extern void InitFonts(void);
extern void InsetRect(Rect *r, int dh, int dv);
extern void InvertRect(Rect *r);
extern unsigned char KeyCheck(ScanCode scanCode);
extern int32_t GetKeyModifiers(void);
extern void KeyboardScanCodeToString(char *buf, uint32_t size, ScanCode code);
extern unsigned int KeyIsDown(ScanCode scanCode);
extern bool KeyboardGet2(int *ch);
extern void LineTo(short h, short v);
extern short MeasureCString(const char *text);
extern short CapsCenteredPenV(short top, short bottom);
extern void MoveTo(short h, short v);
extern void OffsetRect(Rect *r, int dh, int dv);
extern void OpenDataFiles(void);
extern void PStr2CStr(char *dst, const unsigned char *src);
/* QuickDraw's PtInRect under a link name of its own: on 64-bit Windows names are not decorated, and
   user32's PtInRect (which SDL uses) would clash with it */
#define PtInRect QD_PtInRect
extern int PtInRect(int h, int v, Rect *r);
extern void QDReleaseResource(PicHandle pic);
extern void RGBForeColor(const RGBColor *color);
#define TickCount() ReadTick()
#define QDFlushPortBuffer(port, region) ScreenEndScene()
extern void RedrawDialog(DialogPtr dlg);
extern void RedrawDialogs(int a);
extern void ResetScreenRect(void);
extern void ScreenEndScene(void);
extern int SectRect(const Rect *a, const Rect *b, Rect *dst);
extern void SetDialogDefaultItem(DialogPtr dlg, int32_t value);
extern short Random(void);
extern void SetQDGlobalsRandomSeed(int32_t seed);
extern void Microseconds(UnsignedWide *now);
extern void StringToNum(const unsigned char *str, int32_t *value);
extern void SetPort(CGrafPtr port);
/* the Toolbox port's own drawing into a GWorldContainer: its port made current, the one before put back */
extern void SetGWorldContainer(GWorldContainer *gwc);
extern void UnsetGWorldContainer(GWorldContainer *gwc);
extern void SetRect(Rect *r, int left, int top, int right, int bottom);
extern Boolean StillDown(void);
extern void SysBeep(short duration);
extern void TETextBox(const void *text, int32_t length, Rect *box, short just);
extern int MeasureWrappedText(const char *text, int byteCount, int width);
extern int TextLineHeight(void);
extern void NumToString(int32_t value, unsigned char *str);
extern void QDGetPictureBounds(PicHandle pic, Rect *r);
extern void DisposeGWorld(CGrafPtr port);
extern void DebugStr(const char *message);
extern void GetMenuItemText(ControlPtr ctl, int index, unsigned char *text);
extern void *GetGDevice(void);
extern void GetListCellRect(Rect *box, Point cell, ScrollRec **h);
extern Boolean LClick(Point where, short modifiers, ScrollRec **h);
extern void ParamText(const unsigned char *p0, const unsigned char *p1,
                      const unsigned char *p2, const unsigned char *p3);
extern char gParamText0[];
extern char gParamText1[];
extern char gParamText2[];
extern char gParamText3[];
extern void FadeGammaTo(void);
extern bool SplitTwoLineString(const std::string &src, std::string &line1, std::string &line2);
extern void TextFace(short face);
extern void TextFont(short font);
extern void TextSize(short size);
extern short TextWidthMacRoman(const char *text, int byteCount);
extern int TextDeviceWidth(const char *text, int byteCount);
extern unsigned AnsiToUnicode(unsigned char c);             /* code page 1252, the typed text's */
/* port/utf8text.cpp: the game's text (a typed name, an alert) as UTF-8 for a path or the system, and a
   file's name as game text */
extern void TextToUtf8(char *out, const char *text, int outSize);
extern void Utf8ToText(char *out, const char *name, int outSize);
extern void UpdateAndPageFlip(void);
extern RGBColor *blackColor;
extern int32_t gCaretPhase;
extern int32_t gBlinkTick;
extern SWFrameRec *floatingEscortMenuFrame;
extern SWFrameRec *floatingMapFrame;
extern void *gClickDialog;
extern void *gClickItem;
extern uint32_t gClickTime;
extern int32_t gDialogCount;
extern DialogPtr gCurDialog;
extern SpriteWorldPtr gSpriteWorld;

extern SpriteRec *masterShipSprite[768];
extern SpriteRec *masterShipGlowSprite[768];
extern SpriteRec *masterShipShieldSprite[768];
extern SpriteRec *masterShipLightSprite[768];
extern SpriteRec *masterShipWeaponSprite[768];
extern SpriteRec *masterShipAltSprite[768];
extern CGrafPort gGamePort;
extern RGBColor blackRGB;
extern void nv_BlitImageOr(Rect *dstRect, Image_t *icon);
extern RGBColor *whiteColor;
extern RGBColor brightGreen;
extern RGBColor dimGreen;
extern RGBColor rgbWhite;
extern RGBColor playerPaint;
extern RGBColor rgbLightGrey;
extern RGBColor rgbMedGrey;
extern RGBColor rgbDarkGrey;
extern RGBColor rgbLessDarkGrey;
extern RGBColor rgbRealDarkGrey;
extern RGBColor rgbBlack;

extern short GetSpriteXSize(SpriteRec *spr);
extern short GetSpriteYSize(SpriteRec *spr);
extern void SWSetCurrentFrameIndex(SpritePtr srcSpriteP, short frameIndex);
extern void SWSetSpriteLocation(SpritePtr srcSpriteP, short horizLoc,
                                short vertLoc);
extern void SWSetSpriteVisible(SpritePtr srcSpriteP, unsigned char isVisible);

extern void SetDialogItemText(Handle item, const unsigned char *pstr);
extern void GetDialogItemText(Handle item, unsigned char *pstr);
extern void SetDialogVisible(DialogPtr dlg, unsigned char value);
extern void SizeDialogWindow(DialogPtr dlg, short width, short height,
                         Boolean update);
extern void OffsetDialogWindow(DialogPtr dlg, short dh, short dv, Boolean update);
extern void CreateGameWindow(void);
extern CGrafPtr OpenGamePort(Rect *bounds, const unsigned char *title);
extern int32_t GetControlValue(ControlPtr ctl);
extern void InvalDialogRect(DialogPtr dlg, Rect *box);
extern void ModalDialog(void *filterProc, short *itemHitPtr);
extern void DisposeDialog(DialogPtr dlg);
extern void GetDialogItem(DialogPtr dlg, int32_t item, short *type, Handle *handle,
                          Rect *box);
void GetControlText(ControlPtr ctl, char *text);
extern CGrafPtr GetDialogPort(DialogPtr dlg);
extern void InvalDialogItem(DialogPtr dlg, int32_t item);
void SetControlText(ControlPtr ctl, const char *text);
extern void SetControlValue(ControlPtr ctl, int32_t value);
extern void SetDialogItem(DialogPtr dlg, int32_t item, short type, void *handle,
                          const Rect *box);
extern void SetControlTitle(ControlPtr ctl, const unsigned char *text);

extern void PaintRect(Rect *r);
extern Boolean gameLoaded;
extern Boolean gameInProgress;
extern Boolean inBackground;
extern Boolean creatingPilotFile;
extern Boolean onPlanet;
extern Boolean closeRequested;
/* invalidate the whole game window: the port only raises its redraw flag */
#define InvalWindowPortRect(window) (GetCWindowPortRect(window), gNeedUpdateFlag = 1)
extern Boolean gNeedUpdateFlag;

extern void LAutoScroll(ScrollRec **list);
extern void LScroll(int32_t unused, short delta, ScrollRec **list);
extern void LSetSelect(int track, Point pt, ScrollRec **list); /* 0x004d1cd0 */
extern void LSetDrawingMode(Boolean value, ScrollRec **list);          /* 0x004d1b70 */
extern void LSetCell(void *text, int32_t length, Point cell,
                     ScrollRec **list);                          /* 0x004d1b80 */
extern Boolean LGetSelect(int32_t unused, Point *cell, ScrollRec **list); /* 0x004d1d10 */
extern void LDispose(ScrollRec **list);                     /* 0x004d1b40 */
extern ScrollRec **nv_NewDialog(const Rect *rView, const Rect *dataBounds,
                                Point cellSize, ListDrawProcPtr drawProc, CGrafPtr port,
                                unsigned char hasScrollBar);     /* 0x004d1a60 */

extern TEHandle TENew(const Rect *destRect, const Rect *viewRect);
extern void TESetText(const void *text, int32_t length, TEHandle hTE);
extern void TECalText(TEHandle hTE);
extern void TEPinScroll(int dh, int dv, TEHandle hTE);
extern void TEDispose(TEHandle hTE);
extern CWindowPtr gWind;
extern GWorldContainer loadPort;

extern char *GetPixBaseAddr(PixMapPtr pm);
extern short GetPixRowBytes(PixMapPtr pm);
extern short GetPixDepth(PixMapPtr pm);

/* the port's BackColor takes the RGBColor */
#define RGBBackColor(c) BackColor(c)

#endif
