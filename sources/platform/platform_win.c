/* platform_win.c -- platform.h on Windows: a streaming waveOut output (one per playing movie, separate from
   the Sound Tool, as QuickTime had its own), the clocks, the debug log and path conversion. */
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <mmsystem.h>
#include "platform.h"
#include "media.h"

/* 4 buffers of 20 ms: the music's fade-out (music.c StopIntroMusic) is heard until StopMovie */
#define kBuffers      4
#define kBufferFrames (MEDIA_AUDIO_RATE / 50)

struct PlatAudio {
    HWAVEOUT wo;
    HANDLE event;
    HANDLE thread;
    CRITICAL_SECTION lock;
    WAVEHDR hdr[kBuffers];
    int16_t data[kBuffers][kBufferFrames * 2];
    PlatAudioPull pull;
    void *ctx;
    volatile LONG stop;
    volatile LONG ended;         /* the pull returned 0 */
    volatile LONG queued;        /* buffers with the device */
};

/* refills every buffer the device has finished with */
static void Feed(PlatAudio *s)
{
    int i, n;

    for (i = 0; i < kBuffers; i++) {
        WAVEHDR *h = &s->hdr[i];
        if ((h->dwFlags & WHDR_INQUEUE) != 0)
            continue;
        if ((h->dwFlags & WHDR_DONE) != 0) {
            h->dwFlags &= ~WHDR_DONE;
            InterlockedDecrement(&s->queued);
        }
        if (s->ended || s->stop)
            continue;
        EnterCriticalSection(&s->lock);
        n = s->pull(s->ctx, s->data[i], kBufferFrames);
        LeaveCriticalSection(&s->lock);
        if (n <= 0) {
            InterlockedExchange(&s->ended, 1);
            continue;
        }
        h->dwBufferLength = (DWORD)n * 4;
        InterlockedIncrement(&s->queued);
        waveOutWrite(s->wo, h, sizeof *h);
    }
}

static DWORD WINAPI FeedThread(LPVOID arg)
{
    PlatAudio *s = (PlatAudio *)arg;

    while (!s->stop) {
        Feed(s);
        WaitForSingleObject(s->event, 50);
    }
    return 0;
}

PlatAudio *PlatAudioOpen(PlatAudioPull pull, void *ctx)
{
    PlatAudio *s = (PlatAudio *)calloc(1, sizeof *s);
    WAVEFORMATEX wf;
    int i;

    if (s == NULL)
        return NULL;
    s->pull = pull;
    s->ctx = ctx;
    InitializeCriticalSection(&s->lock);
    s->event = CreateEventA(NULL, FALSE, FALSE, NULL);
    memset(&wf, 0, sizeof wf);
    wf.wFormatTag = WAVE_FORMAT_PCM;
    wf.nChannels = 2;
    wf.nSamplesPerSec = MEDIA_AUDIO_RATE;
    wf.wBitsPerSample = 16;
    wf.nBlockAlign = 4;
    wf.nAvgBytesPerSec = MEDIA_AUDIO_RATE * 4;
    if (s->event == NULL ||
        waveOutOpen(&s->wo, WAVE_MAPPER, &wf, (DWORD_PTR)s->event, 0, CALLBACK_EVENT) != MMSYSERR_NOERROR) {
        if (s->event != NULL)
            CloseHandle(s->event);
        DeleteCriticalSection(&s->lock);
        free(s);
        return NULL;
    }
    for (i = 0; i < kBuffers; i++) {
        s->hdr[i].lpData = (LPSTR)s->data[i];
        s->hdr[i].dwBufferLength = sizeof s->data[i];
        waveOutPrepareHeader(s->wo, &s->hdr[i], sizeof s->hdr[i]);
        s->hdr[i].dwFlags &= ~WHDR_DONE;
    }
    s->thread = CreateThread(NULL, 0, FeedThread, s, 0, NULL);
    if (s->thread == NULL) {
        PlatAudioClose(s);
        return NULL;
    }
    return s;
}

void PlatAudioPause(PlatAudio *s)
{
    if (s != NULL)
        waveOutPause(s->wo);
}

void PlatAudioResume(PlatAudio *s)
{
    if (s != NULL)
        waveOutRestart(s->wo);
}

void PlatAudioLock(PlatAudio *s)
{
    if (s != NULL)
        EnterCriticalSection(&s->lock);
}

void PlatAudioUnlock(PlatAudio *s)
{
    if (s != NULL)
        LeaveCriticalSection(&s->lock);
}

int PlatAudioDrained(PlatAudio *s)
{
    int i;

    if (s == NULL)
        return 1;
    if (!s->ended)
        return 0;
    for (i = 0; i < kBuffers; i++)
        if ((s->hdr[i].dwFlags & WHDR_INQUEUE) != 0)
            return 0;
    return 1;
}

void PlatAudioClose(PlatAudio *s)
{
    int i;

    if (s == NULL)
        return;
    InterlockedExchange(&s->stop, 1);
    if (s->thread != NULL) {
        SetEvent(s->event);
        WaitForSingleObject(s->thread, INFINITE);
        CloseHandle(s->thread);
    }
    waveOutReset(s->wo);
    for (i = 0; i < kBuffers; i++)
        waveOutUnprepareHeader(s->wo, &s->hdr[i], sizeof s->hdr[i]);
    waveOutClose(s->wo);
    CloseHandle(s->event);
    DeleteCriticalSection(&s->lock);
    free(s);
}

uint64_t PlatMicroseconds(void)
{
    static LARGE_INTEGER freq;
    LARGE_INTEGER now;

    if (freq.QuadPart == 0)
        QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&now);
    return (uint64_t)(now.QuadPart / freq.QuadPart) * 1000000u +
           (uint64_t)(now.QuadPart % freq.QuadPart) * 1000000u / (uint64_t)freq.QuadPart;
}

uint32_t PlatLocalSeconds1904(void)
{
    FILETIME utc, local;
    ULARGE_INTEGER t;

    GetSystemTimeAsFileTime(&utc);
    FileTimeToLocalFileTime(&utc, &local);
    t.LowPart = local.dwLowDateTime;
    t.HighPart = local.dwHighDateTime;
    /* FILETIME counts 100 ns from 1601-01-01; 1601 -> 1904 is 9561628800 s */
    return (uint32_t)(t.QuadPart / 10000000u - 9561628800ull);
}

void PlatDebugLog(const char *msg)
{
    OutputDebugStringA(msg);
}

void PlatPathToUtf8(const char *path, char *out, int outSize)
{
    wchar_t wide[1024];

    out[0] = '\0';
    if (MultiByteToWideChar(CP_ACP, 0, path, -1, wide, 1024) == 0)
        return;
    if (WideCharToMultiByte(CP_UTF8, 0, wide, -1, out, outSize, NULL, NULL) == 0)
        out[0] = '\0';
}
