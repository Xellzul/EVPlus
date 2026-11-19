/* media.h -- the media decoding behind the movies (media/movie.cpp) and burgerpict.cpp, on FFmpeg (media/media.cpp,
   the only FFmpeg user). Video and audio of one movie use separate demuxers (video on the game thread, audio on the output
   thread). */
#ifndef MEDIA_H
#define MEDIA_H
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MediaFile MediaFile;
typedef struct MediaAudio MediaAudio;

/* NULL when the file is not a movie FFmpeg can open */
extern MediaFile *MediaOpen(const char *path);
extern void MediaClose(MediaFile *m);
extern int MediaHasVideo(const MediaFile *m);
extern int MediaHasAudio(const MediaFile *m);
extern void MediaVideoSize(const MediaFile *m, int *w, int *h);
extern int64_t MediaDurationMs(const MediaFile *m);
/* draws the frame shown at `ms' scaled to dstW x dstH as 16-bit 555 (big- or little-endian) at dst;
   returns 1 if a frame was drawn, 0 if the frame shown at `ms' was already drawn (or nothing decodes) */
extern int MediaVideoDraw(MediaFile *m, int64_t ms, void *dst, int32_t rowBytes, int dstW, int dstH,
                       int bigEndian, int force);

/* 44.1 kHz, stereo, signed 16-bit interleaved */
#define MEDIA_AUDIO_RATE 44100
extern MediaAudio *MediaAudioOpen(const char *path);
extern int MediaAudioRead(MediaAudio *a, int16_t *out, int frames);   /* frames read, 0 at the end */
extern void MediaAudioRewind(MediaAudio *a);
extern void MediaAudioClose(MediaAudio *a);

/* one QuickTime-compressed picture into a pixmap of dstBits (8 indexed, 16 = 555 big-endian, 32 = ARGB);
   0 on success */
extern int MediaDecodeImage(uint32_t cType, int width, int height, int depth, const void *data, int32_t size,
                         void *dst, int32_t rowBytes, int dstBits);

#ifdef __cplusplus
}
#endif
#endif
