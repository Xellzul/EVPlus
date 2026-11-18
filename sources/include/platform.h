/* platform.h -- the operating-system services under the QuickTime functions (media/quicktime.c, media.c): a
   streaming audio output on its own thread, the clocks, the debug log, path conversion. Windows today
   (platform/platform_win.c, waveOut); another platform supplies its own file. */
#ifndef PLATFORM_H
#define PLATFORM_H
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct PlatAudio PlatAudio;
/* called on the output thread with the stream locked; returns frames written, 0 at the end */
typedef int (*PlatAudioPull)(void *ctx, int16_t *out, int frames);

extern PlatAudio *PlatAudioOpen(PlatAudioPull pull, void *ctx);   /* starts playing at once */
extern void PlatAudioPause(PlatAudio *s);
extern void PlatAudioResume(PlatAudio *s);
extern void PlatAudioLock(PlatAudio *s);                   /* excludes the pull */
extern void PlatAudioUnlock(PlatAudio *s);
extern int PlatAudioDrained(PlatAudio *s);                 /* the pull ended and everything queued was played */
extern void PlatAudioClose(PlatAudio *s);                  /* synchronous: the pull is never called after */

extern uint64_t PlatMicroseconds(void);                   /* monotonic */
extern uint32_t PlatLocalSeconds1904(void);               /* Mac clock: seconds since 1904-01-01, local time */
extern void PlatDebugLog(const char *msg);
/* a path in the ANSI code page (the game's) as UTF-8 (FFmpeg's file names on Windows) */
extern void PlatPathToUtf8(const char *path, char *out, int outSize);

#ifdef __cplusplus
}
#endif
#endif
