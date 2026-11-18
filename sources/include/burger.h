/* burger.h -- the statically linked BurgerLib (a cdecl build) and the runtime band. Notes: Research/notes/burger.md */
#ifndef BURGER_H
#define BURGER_H

#include "burgertypes.h"

#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* memory manager (MMMemoryManager) */
extern void * AllocAPointer(Word32 MemSize);
extern void DeallocAPointer(void *MemPtr);
extern FILE *FOpenExpandedPath(const char *path, const char *mode);
extern void * AllocAPointerClear(uint32_t size);
extern void * ResizeAPointer(void *mem, uint32_t size);
extern int32_t IndirectDeallocate(void **p);               /* atplatheap.c */

/* the platform heap (atplatheap.c) */
extern void MemoryClear(void *p, uint32_t n);
extern void MemoryDeallocate(void *p);
extern void MemoryCopy(void *dst, const void *src, uint32_t n);
extern void SystemSetQuitProc(void (*fn)(void));
extern int32_t PlatformOpen(void);
extern int32_t FT_Open(void);
extern int32_t SetDisplayToSize(uint32_t w, uint32_t h, uint32_t depth, uint32_t flags);
extern ULong TimerGetTicks(void);                 /* atticks.c: milliseconds; only differences mean anything */
extern ULong TimerGetTickRate(void);                 /* atticks.c: the tick rate that goes with it */
extern int32_t _IndirectValidate(void **handle);         /* atplatheap.c: the heap check */
extern void * MemoryAllocateClear(uint32_t size);
extern void DebugXString(const char *string);

/* the hash table (athashtbl.c) */
struct HashTable;
extern struct HashTable * HashCreate(uint32_t flags);
extern void HashDispose(void *ref);
extern struct HashTable *HashLoad(ULong flags, void **text);
extern int32_t _HashInsertAppend(void *ref, const void *key, ULong keyLen,
    const void *value, ULong valueLen, unsigned char tag, unsigned char front, ULong kind);
extern struct HashTable *HashInitialize(ULong flags, const char *text);
extern struct HashTable *HashInitializeUnicode(ULong flags, const unsigned short *text);
extern char *HashLookupUnicode(void *table, const unsigned short *key, void *after);
extern char *HashIndex(struct HashTable *t, ULong index, char **key, char **value);
extern char *HashIndexUnicode(struct HashTable *t, ULong index, char **key, char **value);
extern void **HashSave(void *ref);
extern struct HashTable *HashClone(void *ref);
extern unsigned char HashIsEmpty(void *ref);
extern char *HashLookup(void *reply, const char *key, void *after);
extern ULong HashGetFlags(void *ref);
extern int32_t HashAppendData(struct HashTable *t, const char *key, const void *value, ULong valueLen);
extern int32_t HashInsertData(struct HashTable *t, const char *key, const void *value, ULong valueLen);
extern int32_t HashAppendUnicodeData(struct HashTable *t, const unsigned short *key, const void *value,
    ULong valueLen);

/* the interface tool's image layer (itblit.c) */
struct ITStream;
struct ITImage;
struct ITPort;
struct ITPixels;
struct ITDib;
struct ITDevice;
struct ITPalette;
struct ITRect;
struct ITPixelInfo;
struct ITTransparent;
struct ITBackground;
struct ITColor;
struct PictColorTable;
extern struct ITPalette gDT_AlphaPalette;        /* atpalinit.c: the greyscale palette */
extern uint32_t gDT_Mults[9];            /* atpalinit.c: channel width to 16-bit factor */

/* the interface tool's image loaders, one per file format: (stream, image, mask, transparency
   answer, background answer) */
struct ITPoint;

/* string helpers (atplatstr.c) */
extern uint32_t StringGetLength(const unsigned char *s);
extern unsigned char *StringAppendSafe(unsigned char *dst, const unsigned char *src, uint32_t n);
extern void * IndirectInitialize(const void *src, uint32_t size);
extern int32_t StringCompare(const char *a, const char *b);
extern ULong unicode_toupper(ULong c);
extern char *StringFormatSafe(char *out, int32_t size, const char *fmt, ...);
extern char *StringFormat(char *out, const char *fmt, ...);
#ifdef va_start                 /* <stdarg.h> included */
extern char *StringFormatVarg(char *out, ULong size, const char *fmt, va_list args);   /* atformat.c */
#endif
extern int32_t StringCompareSafe(const char *a, const char *b, ULong n);
extern char *StringTokenize(char *s, int delim, char **next);
extern ULong StringToNumber(const char *s);
extern char *StringFindDelimiter(const char *s, const char *set);

/* the lock (atthreadtbl.c) */
extern int32_t CriticalDispose(void *lock);
extern int32_t CriticalCreate(void *lock);

/* UTF-16 / UTF-8 (atcharset.c, atutf162.c, convertutf.c) */
extern int32_t UnicodeCompare(const unsigned short *a, const unsigned short *b);
extern unsigned short *Unicode2UpperCopySafe(unsigned short *dst, const unsigned short *src,
                                    ULong dstBytes, ULong maxChars);
extern ULong UnicodeGetCharCountFromBytes(const unsigned short *s, ULong bytes);
extern ULong UnicodeGetCharByIndex(const unsigned short *s, ULong index);
extern ULong count_UTF8_to_UTF16(const unsigned char *s, ULong limit);
extern ULong count_UTF16_to_UTF8(const unsigned short *s, ULong limit);
extern unsigned short *convert_UTF8_to_UTF16(unsigned short *dst, const unsigned char *src,
                                    ULong dstBytes, int32_t budget, int32_t terminate);
extern unsigned char *convert_UTF16_to_UTF8(unsigned char *dst, const unsigned short *src,
                                   ULong dstBytes, int32_t budget, int32_t terminate);
/* the converters' character budget and NUL-stop flag: 1.0.10 counts the budget unsigned and takes a signed flag */
#define UTFBudget int32_t
#define UTFFlags ULong
extern int32_t ConvertUTF32toUTF16(const ULong **sourceStart, const ULong *sourceEnd,
                         unsigned short **targetStart, unsigned short *targetEnd,
                         UTFBudget *budget, UTFFlags flags);
extern int32_t ConvertUTF16toUTF8(const unsigned short **sourceStart, const unsigned short *sourceEnd,
                         unsigned char **targetStart, unsigned char *targetEnd,
                         UTFBudget *budget, UTFFlags flags);
extern int32_t ConvertUTF8toUTF16(const unsigned char **sourceStart, const unsigned char *sourceEnd,
                         unsigned short **targetStart, unsigned short *targetEnd,
                         UTFBudget *budget, UTFFlags flags);

/* lists, images, directories, paths and the prefs file */
struct AudioDesc;
struct DirectorySearch_t;
struct Image_t;
struct ListNode;
struct ListRec;
struct PrefsNode;
struct RezFile;
struct SessionContext;
extern uint32_t GetAPointerSize(void *mem);
extern void DetachResource(Handle h);
extern void LinkedListInit(struct ListRec *list);
extern void LinkedListAddNewEntryEnd(struct ListRec *list, void *data);
extern void *LinkedListGetEntryData(struct ListRec *list, ULong index);
extern void LinkedListAddNewEntryBegin(struct ListRec *list, void *data);   /* prepend */
extern int ImageParseCicn(struct Image_t *out, const Word8 *in);
extern Word ImageParsePict(struct Image_t *ImagePtr, const Word8 *InputPtr);
extern Word OpenADirectory(struct DirectorySearch_t *dir, const char *name);
extern Word GetADirectoryEntryExtension(struct DirectorySearch_t *dir,
                                        const char *ext);
extern void CloseADirectory(struct DirectorySearch_t *dir);
extern void ImageRemapIndexed(struct Image_t *img, const Word8 *remap);
extern Word PaletteFindColorIndex(const Word8 *pal, Word red, Word green,
                                  Word blue, Word count);
extern int32_t ParseSoundFileImage(struct AudioDesc *info, Word8 *data,
                                ULong size);
extern void ExpandAPathToBuffer(char *dst, const char *path);   /* BurgerLib form */
extern void ExpandAPathToBufferNative(char *dst, const char *path);   /* a Windows path, UTF-8 */
extern Word DoesFileExistNative(const char *name);
extern Word GetADirectoryEntry(struct DirectorySearch_t *dir);
extern Word DeleteAFileNative(const char *name);
extern Word ResourceGetRezNum(struct RezFile *rez, const char *name);
extern Handle ResourceLoadHandle(struct RezFile *rez, int32_t index);
extern Handle ResourceLoadHandleByName(struct RezFile *rez, const char *name);
extern unsigned int SetAPrefix(unsigned int prefix, const char *path);
extern void *LoadAFile(const char *fileName, uint32_t *length);
extern char *GetFileExtension(const char *name);
extern Handle StrCopyHandle(const char *text);
extern char *StrCopy(const char *text);
extern void StripLeadingAndTrailingWhiteSpace(char *text);
extern void ExtractAString2(const char *src, ULong *size, Word flags, char *dst, ULong dstSize);
extern Word BombFlag;
extern char ErrorMsg[512];                       /* burgerfatal.c */
extern Word IAmExiting;                          /* burgerfatal2.c */
extern void DebugXLongWord(Word32 value);
extern char *NumberToAsciiHex(char *dst, ULong value, ULong flags);
extern void *PrefFileNewFromMemory(const char *text, uint32_t size);
extern void ClosePrefsFile(struct PrefsNode *file);
extern void *OpenPrefsFile(const char *name);
extern struct PrefsNode *PrefsFindGroup(struct PrefsNode *file, const char *name);
extern void PrefsGetString(struct PrefsNode *group, const char *key, const char *dflt,
                           char *out, int32_t outSize);
extern int stricmp(const char *s1, const char *s2);   /* MSL Extras */
extern int32_t WritePrefsFile(struct PrefsNode *file, const char *path);

/* runtime-block globals */
extern uint32_t gSaveLeftVolume;
extern uint32_t gSaveMaxVolume;
extern int32_t MouseClicked;
/* a lock (atthreadtbl.c: CriticalCreate .. CriticalLeave take its address) */
typedef struct Mutex {
    struct PlatMutex *section;
    ULong magic;
    unsigned char live;
} Mutex;
extern Mutex gCriticalSection;                   /* the sound mixer's lock (atsound.c) */
extern unsigned char gSoundToolOpen;
extern Word8 ResourceJustLoaded;                 /* 0x00870e11, set by ResourceLoadHandle */
/* handles */
extern void **AllocAHandle2(Word32 MemSize, Word Flag);
extern void DeallocAHandle(void *MemHandle);
extern uint32_t GetAHandleSize(Handle h);
extern Handle ResizeAHandle(Handle h, uint32_t size);
extern char * LockAHandle(Handle h);
extern void UnlockAHandle(Handle h);
extern void SetHandlePurgeFlag(void **handle, Word flag);

/* QuickTime calls, media/quicktime.c */
extern void HLock(Handle h);                                         /* 0x004f1470 */
extern void HUnlock(Handle h);
extern void HPurge(Handle h);
extern short EnterMovies(void);
extern void ExitMovies(void);
extern void SetTrackGWorld(void *track, void *movieData, int32_t a3,
                           short (*proc)(void), int32_t a5);
extern int32_t GetTimeBaseFlags(void *timeBase);
extern void SetTimeBaseFlags(void *timeBase, int32_t flags);

extern void KeyboardInit(void);
extern void KeyboardDestroy(void);
extern void MouseDestroy(void);
extern void SetDefaultPrefixs(void);
extern void ST_DisposeSound(void *snd);
extern unsigned short ST_IsSoundPlaying(void *snd);


/* the resource manager (engine/resmgr.c) */
extern Handle GetResource(uint32_t type, short id);
extern int32_t CountResources(uint32_t type);
extern Handle GetIndResource(uint32_t type, short index);
extern void ReleaseResource(Handle h);
extern void GetResInfo(Handle h, short *id, uint32_t *type,
                       unsigned char *name);

extern unsigned long long TimerGetMicroseconds(void);
extern void LockVideoPage(void);
extern void ST_HaltSound(void *sound);
extern void PlatformClose(void);
extern void ST_Close(void);
extern void FT_Close(void);
extern ULong UFix32_Divide(ULong device, ULong rate);
extern ULong UFix32_Multiply(ULong ratio, ULong rate);
extern unsigned char (*gIndirectBusy)(void *p);
extern unsigned int DoesFileExist(const char *path);
extern char *NumberToAscii(char *dst, ULong value, ULong flags);
extern void OSCursorShow(void);
extern void OSCursorHide(void);
extern void OSCursorReset(void);
extern unsigned char OSCursorVisibleFlag;        /* burgermisc2.c */
extern void ReleaseVideo(void);
extern Word32 LastTick;
extern Word32 ReadTick(void);
extern void WaitTicks(Word tickCount);
#define ResetLastTick() LastTick=ReadTick()

/* BurgerLib's own constants */
#define kOSFailure      -7000
#define kTableFull      -7001
#define kBadArgument    -7002
#define kUnsupported    -7003
#define kOutOfMemory    -7004
#define kNotFound       -7005
#define kDuplicateKey   -7006
#define kNoDefault      -7400      /* sound: no default output */
#define kNoDevice       -7401      /* sound: no output device */
#define kVolumeFailed   -7403      /* sound: volume not set */
#define kMillisPerSecond 1000
#define kKeyTableSize   0x80
#define kPathBufferSize 0x800
#define kFoldLow        0x61       /* 'a' */
#define kFoldHigh       0x7a       /* 'z' */


#define Swap32(v)  ((((v) >> 24) & 0xffUL) | (((v) >> 8) & 0xff00UL) \
                    | (((v) & 0xff00UL) << 8) | (((v) & 0xffUL) << 24))
#define Swap16(v)  ((((v) >> 8) & 0xff) | (((v) & 0xff) << 8))

/* BurgerLib's file manager: the prefix table, the drive letters */
#define PREFIXMAX           35
#define PREFIXBOOT          32                   /* "*:" */
#define PREFIXPREFS         33                   /* "@:" */
#define PREFIXSYSTEM        34                   /* "$:" */
#define kPrefixCurrent      8
#define kPrefixApp          9
#define kNoVolume           ((Word)-1)

/* BurgerLib's memory manager, number and string helpers, fixed-point maths */
#define kHandleFixed        0x40                 /* fixed memory flag */
#define kLeadingZeros       0x8000               /* NumberToAscii flags */
#define kNoTerminator       0x4000
#define kExtractStrDelimitLF 1                   /* ExtractAString2: stop at the end of a line */
#define kFixedShift         16                   /* Fixed32 is 16.16 */
#define kUnityRate          0x10000              /* a sound rate of 1.0 */

/* QuickDraw's Style bits, as the font code of the port and of the interface tool spells them */
#define kFontBold           0x01
#define kFontItalic         0x02
#define kFontUnderline      0x04

/* BurgerLib's sound drivers */
#define kMaxVoices          32
#define kCookieIsVoice      0x40000000           /* a cookie naming a voice slot */
#define kDriverUserMusic    1                    /* BurgerSndExitIn: the music player */
#define kDriverUserSounds   2                    /* BurgerSndExitIn: the sound player */

/* BurgerLib's palette and PixMap code (QuickDraw's rowBytes: bit 15 marks a PixMap) */
#define kPaletteEntries     0x100
#define kPaletteBytes       (kPaletteEntries * 3)
#define kPixMapFlag         0x8000
#define kPixMapFlagMask     0xc000
#define kRowBytesMask       0x3fff

/* MacTypes.h's booleans, which no -bool off TU has built in: byte-wide for C (an enum under
   -enum min; spelled as macros here so that no TU's internal numbering moves); the win port's
   C++ has its plain ints from burgertypes.h */
#ifndef __cplusplus
#define false ((unsigned char)0)
#define true  ((unsigned char)1)
#endif

/* AmbrosiaTools' AssertMacros.h: the code is copied once, then tested */
#define require_noerr(errorCode, exceptionLabel) \
    do { int32_t evalOnceErrorCode = (errorCode); \
         if (0 != evalOnceErrorCode) goto exceptionLabel; } while (0)

/* video globals */
extern Word VideoColorDepth;
extern uint32_t VideoWidth;
extern char *VideoPointer;
extern uint32_t ScreenWidth;
extern uint32_t ScreenHeight;
extern int32_t ScreenFlags;                         /* burgergraphics.c */
extern Word Win95ApplsActive;                    /* burgeronce.c */
extern char *VideoOffscreen;

/* the engine's names for BurgerLib's display globals, as the mac build spells them */
#define videoBase VideoPointer
#define gVideoWidth ScreenWidth
#define gVideoHeight ScreenHeight

/* input globals (burgerinput.c) */
extern int32_t Win95MouseX;
extern int32_t Win95MouseY;
extern Word32 Win95MouseButton;                  /* burgerkbwin.c */
/* the hyperspace white-out (port addition, burgervideomode.c): blend from..to of 256 over 60 Hz ticks */
#define kWhiteNone          0
#define kWhiteSolid         256
#define kWhiteFadeTicks     90                      /* 1.1.1's CGDisplayFade duration, 1.5 s */
extern void VideoFadeWhite(Word from, Word to, uint32_t ticks);

/* sound globals (burgersndwin.c) */
extern Word MusicVolume;
extern Word SystemState;
extern ULong BurgerVoiceCount;                       /* BurgerVoiceCount */

struct adpcm_state;
struct AdpcmVoice;
struct TextBuilder;
struct WindowNode;

extern int32_t ST_Open(unsigned short channels, ULong device);
extern int32_t fgetstr(char *buf, int32_t size, FILE *stream);
extern void fwritelong(int32_t value, FILE *stream);
extern void fwritestr(const char *s, FILE *stream);
extern int32_t fgetlong(FILE *stream);
extern void *LoadAFileFP(FILE *fp, uint32_t *length);
extern ULong fgetfilesize(FILE *fp);     /* BurgerLib fgetfilesize */
extern int32_t CreateDirectoryPathNative(char *path);            /* BurgerLib CreateDirectoryPathNative */
extern char *ParseQuotedString(char *out, int32_t outSize, const char *src);
extern unsigned char IndirectSetLock(void **handle, unsigned char lock);
extern ULong IndirectGetSize(void **handle);
extern char *StringCopySafe(char *dst, const char *src, uint32_t size);
extern char *StringFromNumber(char *out, ULong value);
extern char *String2UpperCopySafe(char *dst, const char *src, ULong n);
extern char *String2LowerCopySafe(char *dst, const char *src, ULong n);
extern char *StringDuplicate(const char *s);
extern char *String2Upper(char *s);
extern char *String2Lower(char *s);
extern char *StringFindString(const char *s, const char *sub);
struct ITElement;

/* the interface tool's settings, window layer, drawing and input, called from its control
   procs (interfacetool.c, ittoolkit.c, itgfx.c, itinput.c) */
struct ITKeyEvent;
struct ITInputEvent;
struct ITClick;
extern struct ITColor gDT_ColorBlack;              /* atpalinit.c: default text colour, black */
extern unsigned char gDT_PatternLtGray[];           /* atpalinit.c: the light grey pattern */
extern unsigned char gDT_PatternMdGray[];           /* atpalinit.c: the 50% grey pattern */
extern unsigned char gDT_PatternBlack[];           /* atpalinit.c: the solid pattern */
extern struct ITColor gDT_ColorWhite;              /* atpalinit.c: white, the default background */
extern struct ITControl *gIT_CurrentItem;        /* ittoolkit.c: the control a proc serves */
extern struct WindowNode *gIT_CurrentView;       /* ittoolkit.c: the window a proc serves */
extern void *gIT_DefaultHash;                    /* atcursor.c */
extern struct WindowNode *gIT_ViewList;          /* atcursor.c */

/* the interface tool's image loader, drawing, rectangles, window layer, markup tree and
   attributes, called across itimage.c, itgfx.c, itrect.c, ittoolkit.c, interfacetool.c,
   itmarkup.c, itattr.c and the control procs; charset and folder lookups from atcharset.c and
   atfilespec.c */
struct ITControl;
struct ITPage;
struct FolderSpec;
#define DTSysRect struct ITRect
extern unsigned char IndirectIsUnicode(void **handle, char *nameOut);


/* the interface tool's control procs, registered by name (atwndclass.c) */
extern struct ControlType *gIT_HandlerList;
extern void StreamHandleInitPut(struct TextBuilder *b);
extern void StreamHandleDestroy(struct TextBuilder *b);
extern void StreamHandleEndSave(struct TextBuilder *b);
extern void StreamHandlePutByte(struct TextBuilder *b, int32_t ch);
extern void StreamHandlePutStringNoZero(struct TextBuilder *b, const char *s);
extern void FastMemSet16(void *dst, ULong value, ULong bytes);
extern void adpcm_coder(short indata[], char outdata[], int len,
                        struct adpcm_state *state);
extern unsigned char ADPCM_Mixer(ULong *data, short **out, ULong frames, ULong step,
                                  struct AdpcmVoice *voice, unsigned short left,
                                  unsigned short right);

extern unsigned char gPlatformOpen;
extern int32_t gNumVoices;
extern ULong gSaveRightVolume;
extern ULong gSampleRate;
extern ULong BurgerSndExitIn;                    /* burgerfile.c */
extern int32_t BurgerSndKillProcs[3];               /* burgerfile.c */

extern ULong UFix32_To_UInt32(ULong rate);
extern void CriticalEnter(void *lock);
extern void CriticalLeave(void *lock);
extern int32_t HashInsert(struct HashTable *t, const char *key, const char *value);
extern int32_t HashAppend(struct HashTable *t, const char *key, const char *value);
extern unsigned char HashRemove(struct HashTable *t, const char *key);
struct WindowNode;
extern ULong UnicodeGetByteCount(const unsigned short *s);
extern const unsigned short *UnicodeAdvance(const unsigned short *s, ULong *code);
extern unsigned short *C2UStringCopySafe(unsigned short *dst, const unsigned char *src,
                                    ULong dstBytes, ULong maxChars);
extern char UnicodePoint2Char8(ULong code);
extern char *StringCopy(char *dst, const char *src);

/* StringCopy's body, inlined where the exe copies a name byte by byte (atnet.c, atresolve.c) */
static inline char *CopyString(char *dst, const char *src)
{
    char *start;

    start = dst;
    while ((*dst++ = *src++) != 0)
        ;
    dst = start;
    return dst;
}

extern void **IndirectAllocate(ULong size);
extern void GetDateTime(uint32_t *seed);
extern void ResourceDestroy(struct RezFile *rez);
extern char *ConvertNativePathToPath(const char *path);
extern void *LoadAFileHandle(const char *path);
extern void **AllocAHandleClear(ULong size);
extern void DrawARect16(int x, int y, Word Width, Word Height, Word Color);
extern void KeyboardFlush(void);
extern int32_t CreateDirectoryPath(const char *path);
extern void Fatal(const char *fmt, ...);
extern void NonFatal(const char *fmt, ...);


/* the guarded copy the file and sound tools inline (atfilespec.c, atsndmem.c) */
static inline void BlockMoveData(const void *src, void *dst, ULong size)
{
    if (size != 0)
        memmove(dst, src, size);
}
extern short OpenEVPlugins(const char *path, const char *ext,
                           FILE *log);
extern void ImageDestroy(struct Image_t *ImagePtr);
extern void ImageDelete(struct Image_t *ImagePtr);
extern unsigned int KeyboardKbhit(void);
extern unsigned int KeyboardGetch(void);
extern void KeyboardCallPollingProcs(void);
extern void ProcessSystemEvents(void);
extern Word32 PowerOf2(Word32 Input);

/* the song decoders' front end (burgerdigsong.c) */
struct DigitalMusicReadState_t;

/* the sample expanders the song decoders call (burgermace.c, burgermsadpcm.c) and the
   8-bit law tables (burgerulaw.c, burgeralaw.c) */
struct MACEState_t;
struct ADPCMUnpackState_t;
extern const short UnpackULawTable[256];
extern const short UnpackALawTable[256];

/* the read-ahead file stream (burgerrswin.c) */
struct ReadFileStream_t;
extern unsigned int OSCursorIsVisible(void);
extern void CStr2PStr(char *dst, const char *src);
extern int32_t ST_PlaySound(void *snd, unsigned short priority, unsigned short level);
extern ULong ST_PlaySoundParam(SoundRequest *req);

extern unsigned char KeyArray[128];
extern int32_t gGDIDIBSectionFunc;

#ifdef __cplusplus
}

/* W9Win95Code's application instance, read by OCWin95Code; set nowhere in this exe. */
namespace Burger {
class Win32 {
public:
    static void *m_pInstance;
};
}
#endif

#endif
