/* movie.cpp -- the game's movies and songs (movie.h): intro music, the full-screen movies and the shipyard's
 * looping ship movie. Decoding is media.cpp (FFmpeg); audio output and the clock are platform.h.
 *
 * Timing follows QuickTime, which the game was written against: the clock runs from MovieStart, after the
 * sound has been opened, so picture and sound start together; a movie with a picture is done when the clock
 * passes its duration, a song when its sound has drained. */

#include "macport.h"
#include "media.h"
#include "movie.h"
#include "platform.h"
#include "gamepaths.h"

typedef struct MovieRecord {
    struct MovieRecord *next;
    char *path;
    MediaFile *media;
    MediaAudio *audio;                 /* decoded on the output thread */
    PlatAudio *stream;
    int width, height;                 /* the picture's own size */
    char *pixels;                      /* the target: top left pixel of the box */
    int32_t rowBytes;
    int boxW, boxH;
    int bigEndian;
    int loop;
    int playing;
    int64_t posMs;                     /* the movie time when the clock last started or stopped */
    uint64_t startUs;
    int64_t durationMs;
    volatile int volume;               /* 0x100 = unity */
    int audioFailed;                   /* the output device would not open */
    int redraw;
} MovieRecord;

static MovieRecord *gMovies;

static int FileExists(const char *path)
{
    FILE *f = PlatFileOpen(path, "rb");

    if (f == NULL)
        return 0;
    fclose(f);
    return 1;
}

int MovieFindFile(const char *const *names, int count, char *path, size_t size)
{
    int folder, i;

    for (folder = 0; folder < 2; folder++) {
        for (i = 0; i < count; i++) {
            snprintf(path, size, "%s", JoinPath(folder == 0 ? paths.plugins : paths.data, names[i]).c_str());
            if (FileExists(path))
                return 1;
        }
    }
    return 0;
}

/* the movie time now, held at the end unless looping */
static int64_t MovieMs(const MovieRecord *m)
{
    int64_t t = m->posMs;

    if (m->playing)
        t += (int64_t)((PlatMicroseconds() - m->startUs) / 1000u);
    if (m->loop) {
        if (m->durationMs > 0)
            t %= m->durationMs;
    } else if (t > m->durationMs) {
        t = m->durationMs;
    }
    return t;
}

/* the output thread's pull: decoded samples times the movie volume */
static int PullAudio(void *ctx, int16_t *out, int frames)
{
    MovieRecord *m = (MovieRecord *)ctx;
    int n = MediaAudioRead(m->audio, out, frames), i, gain = m->volume;

    if (n == 0 && m->loop) {
        MediaAudioRewind(m->audio);
        n = MediaAudioRead(m->audio, out, frames);
    }
    if (gain < 0)
        gain = 0;
    if (gain != 0x100) {
        for (i = 0; i < n * 2; i++) {
            int v = out[i] * gain / 0x100;
            out[i] = (int16_t)(v > 32767 ? 32767 : v < -32768 ? -32768 : v);
        }
    }
    return n;
}

Movie MovieOpen(const char *path)
{
    MovieRecord *m = (MovieRecord *)calloc(1, sizeof *m);

    if (m == NULL)
        return NULL;
    m->media = MediaOpen(path);
    if (m->media == NULL) {
        free(m);
        return NULL;
    }
    m->path = strdup(path);
    MediaVideoSize(m->media, &m->width, &m->height);
    m->durationMs = MediaDurationMs(m->media);
    m->volume = 0x100;
    m->next = gMovies;
    gMovies = m;
    return m;
}

void MovieClose(Movie m)
{
    MovieRecord **p;

    if (m == NULL)
        return;
    for (p = &gMovies; *p != NULL; p = &(*p)->next)
        if (*p == m) {
            *p = m->next;
            break;
        }
    PlatAudioClose(m->stream);                        /* synchronous: no pull after this */
    MediaAudioClose(m->audio);
    MediaClose(m->media);
    free(m->path);
    free(m);
}

void MovieSize(Movie m, int *w, int *h)
{
    *w = m != NULL ? m->width : 0;
    *h = m != NULL ? m->height : 0;
}

void MovieSetTarget(Movie m, void *pixels, int32_t rowBytes, int w, int h, int bigEndian)
{
    if (m == NULL)
        return;
    m->pixels = (char *)pixels;
    m->rowBytes = rowBytes;
    m->boxW = w;
    m->boxH = h;
    m->bigEndian = bigEndian;
    m->redraw = 1;
}

void MovieSetLoop(Movie m, int loop)
{
    if (m != NULL)
        m->loop = loop;
}

void MovieSetVolume(Movie m, short volume)
{
    if (m != NULL)
        m->volume = volume;
}

void MovieStart(Movie m)
{
    if (m == NULL || m->playing)
        return;
    m->playing = 1;
    if (MediaHasAudio(m->media)) {
        if (m->stream != NULL) {
            /* a song: the sound decides when it is done, so it always resumes */
            PlatAudioResume(m->stream);
        } else if (m->audio == NULL && !m->audioFailed &&
                   (m->loop || !MediaHasVideo(m->media) || m->posMs < m->durationMs || m->durationMs <= 0)) {
            m->audio = MediaAudioOpen(m->path);
            if (m->audio != NULL) {
                m->stream = PlatAudioOpen(MEDIA_AUDIO_RATE, PullAudio, m);
                if (m->stream == NULL) {        /* no audio device: play on silently, open nothing again */
                    MediaAudioClose(m->audio);
                    m->audio = NULL;
                    m->audioFailed = 1;
                }
            }
        }
    }
    /* the clock starts once the sound is open, so picture and sound start together */
    m->startUs = PlatMicroseconds();
}

void MovieStop(Movie m)
{
    if (m == NULL || !m->playing)
        return;
    m->posMs = MovieMs(m);
    m->playing = 0;
    PlatAudioPause(m->stream);
}

void MovieRewind(Movie m)
{
    if (m == NULL)
        return;
    m->posMs = 0;
    m->startUs = PlatMicroseconds();
    m->redraw = 1;
    if (m->stream != NULL) {
        PlatAudioLock(m->stream);
        MediaAudioRewind(m->audio);
        PlatAudioUnlock(m->stream);
    }
}

int MovieDone(Movie m)
{
    if (m == NULL || m->loop)
        return 0;
    if (!MediaHasVideo(m->media) && m->stream != NULL)
        return PlatAudioDrained(m->stream);
    return MovieMs(m) >= m->durationMs;
}

int MovieDraw(Movie m)
{
    if (m == NULL || m->pixels == NULL || !MediaHasVideo(m->media))
        return 0;
    if (!MediaVideoDraw(m->media, MovieMs(m), m->pixels, m->rowBytes, m->boxW, m->boxH, m->bigEndian, m->redraw))
        return 0;
    m->redraw = 0;
    return 1;
}

void MovieStopAll(void)
{
    MovieRecord *m;

    for (m = gMovies; m != NULL; m = m->next)
        PlatAudioPause(m->stream);
}
