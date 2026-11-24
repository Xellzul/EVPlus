/* movie.h -- the game's movies and songs (media/movie.cpp): a file played on its own clock, the picture drawn
   into 16-bit 5-5-5 pixels when asked, the sound on an audio stream of its own. Decoding is media.cpp. */
#ifndef MOVIE_H
#define MOVIE_H
#include <stddef.h>
#include <stdint.h>

typedef struct MovieRecord *Movie;

#define kMoviePathMax 2048

/* the first of `names' found in the plug-ins folder, else the first in Nova Files; 0 when none is there */
extern int MovieFindFile(const char *const *names, int count, char *path, size_t size);

/* NULL when the file is not a movie or song FFmpeg can open */
extern Movie MovieOpen(const char *path);
extern void MovieClose(Movie m);
extern void MovieSize(Movie m, int *w, int *h);

/* where the picture goes: the top left pixel of a w x h box, not clipped */
extern void MovieSetTarget(Movie m, void *pixels, int32_t rowBytes, int w, int h, int bigEndian);
extern void MovieSetLoop(Movie m, int loop);
extern void MovieSetVolume(Movie m, short volume);  /* 0x100 = unity */

extern void MovieStart(Movie m);
extern void MovieStop(Movie m);
extern void MovieRewind(Movie m);
extern int MovieDone(Movie m);                      /* never for NULL or a looping movie */
extern int MovieDraw(Movie m);                      /* 1 when it drew a new frame */

/* quitting: silence every movie */
extern void MovieStopAll(void);

#endif
