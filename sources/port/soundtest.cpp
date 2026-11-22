/* soundtest.cpp -- a mixer capture, run before the main screen when kSoundTest (mainscreen.cpp) is 1.
   soundtest_load.txt: every snd resource as ST_LoadSndResource decodes it (length, rate, a hash of the
   samples) and what ParseSoundFileImage makes of it.
   soundtest.raw: a scripted run of sounds (levels, pan, rates, priorities, more voices than are heard, halts by
   id and by sound, one sound started again each time it ends) mixed through _ST_MixerDispatch in pulls of
   512 frames: 16-bit stereo at the mixer's rate, no header. The device is kept out for the run by holding
   the mixer's lock. soundtest.txt: what each call returned and the voices in use after each pull.
   Each file must stay the same before and after a change; build/devtools/scaletest.sh runs it. */

#include <vector>
#include "burgerchan.h"
#include "burgerplat.h"
#include "macport.h"
#include "platform.h"

namespace {

constexpr ULong kPullFrames = 512;
constexpr int kPulls = 320;

/* the sounds the script plays: click, warp up, warp out, engine beep, shots, explosions, red alert */
constexpr short kClipIds[] = { 0x258, 0x80, 0x82, 0x96, 0xc8, 0xc9, 0xca, 0xcb, 0x12c, 0x12d, 0x172 };
enum { kClick, kWarpUp, kWarpOut, kBeep, kShot0, kShot1, kShot2, kShot3, kExplod0, kExplod1, kAlert,
       kClipCount };

uint64_t Hash(const void *data, size_t bytes)
{
    const unsigned char *p = (const unsigned char *)data;
    uint64_t h = 0xcbf29ce484222325ULL;

    for (size_t i = 0; i < bytes; i++) {
        h ^= p[i];
        h *= 0x100000001b3ULL;
    }
    return h;
}

void LogLoads()
{
    FILE *log = PlatFileOpen("soundtest_load.txt", "wb");
    int32_t count = CountResources(kSoundType);

    if (log == NULL)
        return;
    fprintf(log, "%d snd resources\n", (int)count);
    for (int32_t i = 1; i <= count; i++) {
        Handle h = GetIndResource(kSoundType, (short)i);
        short id = 0;
        uint32_t type = 0;
        unsigned char name[256];
        AudioDesc info;
        int32_t failed = -1;

        if (h == NULL) {
            fprintf(log, "#%d: no resource\n", (int)i);
            continue;
        }
        GetResInfo(h, &id, &type, name);
        LockAHandle(h);
        failed = ParseSoundFileImage(&info, (Word8 *)*h, GetAHandleSize(h));
        ReleaseResource(h);
        fprintf(log, "#%d id %d: parse %d count %u rate %u", (int)i, id, (int)failed,
                (unsigned)info.SampleCount, (unsigned)info.SampleRate);
        Sound *sound = ST_LoadSndResource(id);
        if (sound == NULL) {
            fprintf(log, ", not loaded\n");
            continue;
        }
        uint32_t samples = ST_SoundSamples(sound);
        fprintf(log, ", samples %u rate %08x hash %016llx\n", (unsigned)samples, (unsigned)ST_SoundRate(sound),
                (unsigned long long)Hash(ST_SoundData(sound), (size_t)samples * 2));
        ST_DisposeSound(sound);
    }
    fclose(log);
}

ULong Play(Sound *sound, unsigned short priority, unsigned short left, unsigned short right, int32_t rate,
           uint32_t id = 0)
{
    SoundRequest req;

    memset(&req, 0, sizeof(req));
    req.sound = sound;
    req.priority = priority;
    req.leftLevel = left;
    req.rightLevel = right;
    req.rate = rate;
    req.id = id;
    return ST_PlaySoundParam(&req);
}

/* what the script does before pull `pull' */
uint32_t gFirstId;                      /* the first channel id of the run */

/* a channel id counted from the run's first (1), so the log does not depend on the sounds played before it */
long RunId(uint32_t id)
{
    if (id == 0)
        return 0;
    if (gFirstId == 0)
        gFirstId = id;
    return (long)(id - gFirstId) + 1;
}

void Script(int pull, Sound *const *clips, FILE *log)
{
    static ULong warpId;
    long r;

    switch (pull) {
    case 0:
        fprintf(log, "pull %d: click %ld\n", pull, RunId(ST_PlaySound(clips[kClick], 1, 0x80)));
        break;
    case 3:
        fprintf(log, "pull %d: shot left %ld\n", pull, RunId(ST_PlaySoundStereo(clips[kShot0], 50, 0x80, 0x20)));
        break;
    case 6:
        warpId = Play(clips[kWarpUp], 100, 0x60, 0x60, 0x18000);
        fprintf(log, "pull %d: warp up at 1.5 %ld\n", pull, RunId(warpId));
        break;
    case 10:
        fprintf(log, "pull %d: explosion at 0.5 %ld\n", pull,
                RunId(Play(clips[kExplod0], 30, 0x40, 0x80, 0x8000)));
        break;
    case 14:
        fprintf(log, "pull %d: alert level and priority clamped %ld\n", pull,
                RunId(Play(clips[kAlert], 0, 0xc0, 0x100, kUnityRate)));
        break;
    case 40:
        if (warpId != 0)
            ST_HaltSoundId(warpId);
        fprintf(log, "pull %d: warp halted by id, %d playing\n", pull, (int)ST_IsSoundPlaying(0));
        break;
    case 48:
        if (clips[kExplod0] != NULL)
            ST_HaltSound(clips[kExplod0]);
        fprintf(log, "pull %d: explosion halted by sound, %d playing\n", pull, (int)ST_IsSoundPlaying(0));
        break;
    case 100:
        fprintf(log, "pull %d: warp out at 1/1.3 %ld\n", pull,
                RunId(Play(clips[kWarpOut], 80, 0x70, 0x50, (int32_t)(65536.0 / 1.3))));
        break;
    case 150:
        fprintf(log, "pull %d: explosion with id 1000 %u\n", pull,
                (unsigned)Play(clips[kExplod1], 40, 0x80, 0x80, 0x14000, 1000));
        break;
    case 170:
        ST_HaltSoundId(1000);
        fprintf(log, "pull %d: id 1000 halted, %d playing\n", pull, (int)ST_IsSoundPlaying(0));
        break;
    }
    if (pull >= 20 && pull < 32) {
        int k = pull - 20;
        r = RunId(Play(clips[kShot0 + k % 4], (unsigned short)(5 + k * 7 % 40), (unsigned short)(0x10 + k * 9),
                       (unsigned short)(0x80 - k * 9), 0x10000 + (k - 6) * 0x1800));
        fprintf(log, "pull %d: shot %d %ld, %d voices\n", pull, k, r, (int)gNumVoices);
    }
    if (pull >= 60 && pull < 280 && clips[kBeep] != NULL && ST_IsSoundPlaying(clips[kBeep]) == 0) {
        r = RunId(ST_PlaySoundStereo(clips[kBeep], 20, 0x30, 0x70));
        fprintf(log, "pull %d: beep again %ld\n", pull, r);
    }
}

} // namespace

void DebugSoundTest(void)
{
    Sound *clips[kClipCount];
    std::vector<short> mix(kPullFrames * 2);

    LogLoads();
    FILE *log = PlatFileOpen("soundtest.txt", "wb");
    FILE *out = PlatFileOpen("soundtest.raw", "wb");
    if (log == NULL || out == NULL) {
        if (log != NULL)
            fclose(log);
        if (out != NULL)
            fclose(out);
        return;
    }
    fprintf(log, "sound tool open %d, rate %08x, %d heard of %d\n", (int)gSoundToolOpen, (unsigned)gSampleRate,
            (int)gMaxVoices, kSoundChannels);
    for (int i = 0; i < kClipCount; i++) {
        clips[i] = ST_LoadSndResource(kClipIds[i]);
        fprintf(log, "snd %d: %s\n", kClipIds[i], clips[i] != NULL ? "loaded" : "missing");
    }

    CriticalEnter(&gCriticalSection);
    ST_HaltSound(0);
    for (int pull = 0; pull < kPulls; pull++) {
        Script(pull, clips, log);
        _ST_MixerDispatch(mix.data(), kPullFrames);
        fwrite(mix.data(), sizeof(short), mix.size(), out);
        fprintf(log, "pull %d: %d voices\n", pull, (int)gNumVoices);
    }
    ST_HaltSound(0);
    CriticalLeave(&gCriticalSection);

    for (Sound *clip : clips)
        ST_DisposeSound(clip);
    fclose(out);
    fclose(log);
}
