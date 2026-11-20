/* music.cpp -- the intro-music song (win 0x004ab5d0..0x004ab960). Notes: Research/notes/music.md */
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
#include "movie.h"

#define kMusicIdleTicks   0x28   /* HandleIntroMusic services at most this often */
#define kMaxVolume        0x100
#define kVolumeScale      0x20   /* pref units -> QuickTime volume */
#define kFadeStep         8      /* volume shed per tick while fading out */
#define kStartVolumeScale 0x30   /* SetupIntroMusic starts louder than the fade */

#define kMusicStrings     130    /* STR# holding the intro song's file name */
#define kIntroSongName    2
#define kExtensionCount   3

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

// FUNCTION: EVNOVA 0x004ab5d0
// MAC: _SetupIntroMusic i386 0x00061af8 ppc 0x000698d4
void SetupIntroMusic(void)
{
    short i;
    short volume;
    char path[kMoviePathMax];
    char name[256];
    char files[kExtensionCount][256 + 8];
    const char *names[kExtensionCount];
    Str255 songName;

    if (settings.introMusic == '\0')
        return;

    GetIndString(songName, kMusicStrings, kIntroSongName);
    introMusicMovie = 0;
    PStr2CStr(name, songName);
    for (i = 0; i < kExtensionCount; i++) {
        sprintf(files[i], "%s%s", name, gMusicExtensions[i]);
        names[i] = files[i];
    }
    if (!MovieFindFile(names, kExtensionCount, path, sizeof path))
        return;
    introMusicMovie = MovieOpen(path);

    volume = settings.volume * kStartVolumeScale;
    if (volume > kMaxVolume)
        volume = kMaxVolume;
    MovieSetVolume(introMusicMovie, volume);
    MovieStart(introMusicMovie);
    lastMoviesTaskTime = 0;
}

// FUNCTION: EVNOVA 0x004ab820
// MAC: _StopIntroMusic i386 0x00061c82 ppc 0x00069a70
void StopIntroMusic(Boolean fade)
{
    short volume;
    int32_t start;

    if (introMusicMovie != 0) {
        if (fade != 0 && MovieDone(introMusicMovie) == 0) {
            volume = settings.volume * kVolumeScale;
            if (volume > kMaxVolume)
                volume = kMaxVolume;
            while (volume > 0) {
                MovieSetVolume(introMusicMovie, volume);
                start = ReadTick();
                do {
                    BusyIdle();
                } while (start == ReadTick());
                volume -= kFadeStep;
                if (volume < 0)
                    volume = 0;
            }
        }
        MovieStop(introMusicMovie);
        MovieClose(introMusicMovie);
        introMusicMovie = 0;
    }
}

// FUNCTION: EVNOVA 0x004ab8d0
// MAC: _HandleIntroMusic i386 0x00061d68 ppc 0x00069b78
void HandleIntroMusic(Boolean force)
{
    short volume;

    if (introMusicMovie != 0) {
        if (force != 0 || ReadTick() >= lastMoviesTaskTime + kMusicIdleTicks) {
            if (MovieDone(introMusicMovie) != 0) {
                MovieClose(introMusicMovie);
                introMusicMovie = 0;
            } else {
                volume = settings.volume * kVolumeScale;
                if (volume > kMaxVolume)
                    volume = kMaxVolume;
                MovieSetVolume(introMusicMovie, volume);
                lastMoviesTaskTime = ReadTick();
            }
        }
    }
}

