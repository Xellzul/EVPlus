// COMPILE: -lang c++ -bool off
/* music.c -- the intro-music song (win 0x004ab5d0..0x004ab960). Notes: Research/notes/music.md */
/*
 * Both volume computations read settings.volume through movzx even though the pref
 * is a signed short, because the product is truncated back to a short and the
 * upper half cannot matter.
 */
#include <stdio.h>
#include "macport.h"
#include "game.h"
#include "sound.h"
#include "player.h"
#include "render.h"
#include "burger.h"
#include "gameutil.h"
#include "cefix.h"

#define kMusicIdleTicks   0x28   /* HandleIntroMusic services at most this often */
#define kMaxVolume        0x100
#define kVolumeScale      0x20   /* pref units -> QuickTime volume */
#define kFadeStep         8      /* volume shed per tick while fading out */
#define kStartVolumeScale 0x30   /* SetupIntroMusic starts louder than the fade */

#define kMusicStrings     130    /* STR# holding the intro song's file name */
#define kIntroSongName    2
#define kExtensionCount   3

#define fsRdPerm          1
#define newMovieActive    1

/* the load-bar and intro-music object's globals */
Rect progressRect;
unsigned char fireOffRect[144];
Rect progressDefRect;
double progressCount;
double progressMax;
Rect gLoadBarBase;
RGBColor progressColor[3];
uint32_t lastMoviesTaskTime;
Movie introMusicMovie;

/* "", ".mov", ".mp3" — tried in turn under each folder */
char *gMusicExtensions[kExtensionCount] = { "", ".mov", ".mp3" };  /* 0x00575a70 (name invented) */

/* SetupIntroMusic (mac _SetupIntroMusic). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004ab5d0
// MAC: _SetupIntroMusic i386 0x00061af8 ppc 0x000698d4
void SetupIntroMusic(void)
{
    short result;
    short i;
    short volume;
    int err;
    int specErr;
    short resID;
    char path[256];
    FSSpec spec;
    short refNum;
    char name[256];
    Str255 songName;

    if (settings.introMusic == '\0')
        return;

    GetIndString(songName, kMusicStrings, kIntroSongName);
    introMusicMovie = 0;
    i = 0;
    do {
        PStr2CStr(name, songName);
        sprintf(path, "9:Nova Plug-ins:%s%s", name, gMusicExtensions[i]);
#ifdef CEFIX /* F07.05 */
        CE_ExpandPlugInsPath(path, path);
#else
        ExpandAPathToBufferNative(path, path);
#endif
        CStr2PStr(path, path);
        result = FSMakeFSSpec(0, 0, path, &spec);
        specErr = result;
        if (specErr == 0)
            result = OpenMovieFile(&spec, &refNum, fsRdPerm);
        err = result;
    } while (err != 0 && ++i < kExtensionCount);

    if (err != 0) {
        i = 0;
        do {
            PStr2CStr(name, songName);
            sprintf(path, "9:Nova Files:%s%s", name, gMusicExtensions[i]);
#ifdef CEFIX /* F07.06 */
            CE_ExpandFilesPath(path, path);
#else
            ExpandAPathToBufferNative(path, path);
#endif
            CStr2PStr(path, path);
            result = FSMakeFSSpec(0, 0, path, &spec);
            specErr = result;
            if (specErr == 0)
                result = OpenMovieFile(&spec, &refNum, fsRdPerm);
            err = result;
        } while (err != 0 && ++i < kExtensionCount);
        if (err != 0)
            return;
    }

    resID = 0;
    // BUG: NewMovieFromFile's result is dropped and the err test below is stale (always 0 here);
    // a file that opens but is not a playable movie leaves introMusicMovie NULL and it is still used
    NewMovieFromFile(&introMusicMovie, refNum, &resID, 0, newMovieActive, 0);
    CloseMovieFile(refNum);
    if (err != 0)
        return;

    volume = settings.volume * kStartVolumeScale;
    if (volume > kMaxVolume)
        volume = kMaxVolume;
    SetMovieVolume(introMusicMovie, volume);
    StartMovie(introMusicMovie);
    lastMoviesTaskTime = 0;
}

/* StopIntroMusic (mac _StopIntroMusic). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004ab820
// MAC: _StopIntroMusic i386 0x00061c82 ppc 0x00069a70
void StopIntroMusic(Boolean fade)
{
    short volume;
    int32_t start;

    if (introMusicMovie != 0) {
        if (fade != 0 && IsMovieDone(introMusicMovie) == 0) {
            volume = settings.volume * kVolumeScale;
            if (volume > kMaxVolume)
                volume = kMaxVolume;
            while (volume > 0) {
                SetMovieVolume(introMusicMovie, volume);
                start = ReadTick();
                do {
                    BusyIdle();
                } while (start == ReadTick());
                volume -= kFadeStep;
                if (volume < 0)
                    volume = 0;
            }
        }
        StopMovie(introMusicMovie);
        DisposeMovie(introMusicMovie);
        introMusicMovie = 0;
    }
}

/* HandleIntroMusic (mac _HandleIntroMusic). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004ab8d0
// MAC: _HandleIntroMusic i386 0x00061d68 ppc 0x00069b78
void HandleIntroMusic(Boolean force)
{
    short volume;

    if (introMusicMovie != 0) {
        if (force != 0 || ReadTick() >= lastMoviesTaskTime + kMusicIdleTicks) {
            if (IsMovieDone(introMusicMovie) != 0) {
                DisposeMovie(introMusicMovie);
                introMusicMovie = 0;
            } else {
                volume = settings.volume * kVolumeScale;
                if (volume > kMaxVolume)
                    volume = kMaxVolume;
                SetMovieVolume(introMusicMovie, volume);
                MoviesTask(introMusicMovie, 0);
                lastMoviesTaskTime = ReadTick();
            }
        }
    }
}

