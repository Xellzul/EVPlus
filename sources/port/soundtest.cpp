/* soundtest.cpp -- a mixer capture, run before the main screen when kSoundTest (mainscreen.cpp) is 1.
   soundtest_load.txt: every snd resource as ST_LoadSndResource decodes it (length, rate, a hash of the
   samples) and what ParseSoundFileImage makes of it.
   soundtest_sdl.raw: a scripted run of sounds (levels, pan, rates, priorities, many voices at once,
   halts by id and by sound, one sound started again each time it ends) on SDL's voices rendered
   offline, float stereo at kPlatVoicesRenderRate, no header, in pulls as long as 512 frames at 44100 Hz.
   soundtest_sdl.txt: what each call returned and the voices in use after each pull; soundtest_sdl_voices.txt:
   the ids playing after each pull.
   soundtest_fight_sdl.wav: a busy fight (panned shots, explosions, a looping sound started again, the alert),
   soundtest_warp_sdl.wav: the warp-up at mobility 1.0, 0.7 and 1.5, both at the top volume;
   soundtest_wav.txt: their peak voices, peak before clamping and clipped samples, also at the default volume. */

#include <math.h>
#include <algorithm>
#include <vector>
#include "burgerchan.h"
#include "burgerplat.h"
#include "game.h"
#include "macport.h"
#include "platform.h"
#include "player.h"
#include "render.h"
#include "sound.h"

namespace {

constexpr ULong kPullFrames = 512;
constexpr int kPulls = 320;
constexpr int kPullRate = 44100;
constexpr short kTopVolume = 8;          /* the preferences' volume: its top and its default */
constexpr short kDefaultVolume = 5;

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

/* what the script does before pull `pull' */
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


/* a fight around the player: ships firing from where they are, the player's own gun, explosions now and then,
   a looping sound started again each time it ends, the red alert twice */
constexpr int kFightPulls = 690;        /* 8 s */
constexpr uint64_t kFightSeed = 0x5eed;

struct FightShip {
    float x;
    float y;
    int period;                         /* pulls between shots */
    int phase;
    int clip;
};

constexpr FightShip kFightShips[] = {
    { -900, -300, 19, 0, kShot0 }, { 1200, 200, 22, 4, kShot1 }, { -300, 800, 25, 7, kShot2 },
    { 600, -1100, 21, 2, kShot3 }, { -1600, 900, 27, 5, kShot1 },
};

void Fight(int pull, Sound *const *clips, uint64_t *seed)
{
    FloatPoint player = { 0, 0 };
    FloatPoint at;

    for (const FightShip &ship : kFightShips) {
        if (pull % ship.period == ship.phase) {
            at.x = ship.x + (float)(PlatRand(seed, 200) - 100);
            at.y = ship.y + (float)(PlatRand(seed, 200) - 100);
            PlaySoundDistance(-1, clips[ship.clip], 3, &at, &player);
        }
    }
    if (pull < 400 && pull % 12 == 0)
        PlaySoundDistance(-1, clips[kShot0], 3, &player, &player);
    if (PlatRand(seed, 45) == 0) {
        at.x = (float)(PlatRand(seed, 3000) - 1500);
        at.y = (float)(PlatRand(seed, 2000) - 1000);
        PlaySoundDistance(-1, clips[kExplod0 + PlatRand(seed, 2)], 6, &at, &player);
    }
    if (pull >= 50 && pull < 600 && clips[kBeep] != NULL && ST_IsSoundPlaying(clips[kBeep]) == 0) {
        at.x = kFightShips[1].x;
        at.y = kFightShips[1].y;
        PlaySoundDistance(-1, clips[kBeep], 3, &at, &player);
    }
    if (pull == 120 || pull == 450)
        PlaySound(clips[kAlert], 50, gSoundVolume, gSoundVolume);
}

/* the warp-up as the player's ship plays it (handleplayer.cpp), at each mobility, half a second apart */
constexpr double kWarpMobility[] = { 1.0, 0.7, 1.5 };
constexpr int kWarpCount = 3;
constexpr int kWarpGapPulls = 43;
constexpr int kWarpMaxPulls = 4000;

struct WarpRun {
    int next;                           /* the next mobility */
    int startAt;                        /* the pull it starts at */
    bool playing;
};

/* false once the last has ended and its gap passed */
bool Warp(int pull, Sound *warp, WarpRun *w)
{
    if (w->playing && ST_IsSoundPlaying(warp) == 0) {
        w->playing = false;
        w->startAt = pull + kWarpGapPulls;
    }
    if (!w->playing && pull >= w->startAt) {
        if (w->next == kWarpCount)
            return false;
        warpUpSoundSpec.sound = warp;
        AdjustWarpSoundSpeed(kWarpMobility[w->next++]);
        ST_PlaySoundParam(&warpUpSoundSpec);
        w->playing = true;
    }
    return true;
}

/* the voice counts, the largest sample before clamping (1 = full scale), the samples clamped */
struct MixStats {
    int peakVoices;
    long voicePulls;                    /* voices summed over the pulls */
    int pulls;
    double peak;
    long clipped;
    long samples;
};

/* frames of the render before pull n: as long as n pulls of 512 frames at 44100 Hz */
long SdlFramesBefore(int n)
{
    return (long)((int64_t)n * kPullFrames * kPlatVoicesRenderRate / kPullRate);
}

/* one pull of the offline voices */
class Mixer {
public:
    Mixer() : stats_() {}

    /* renders pull `pull' into Data(); stats kept when asked */
    void Pull(int pull, bool stats)
    {
        int frames = (int)(SdlFramesBefore(pull + 1) - SdlFramesBefore(pull));

        if (gNumVoices > stats_.peakVoices)
            stats_.peakVoices = gNumVoices;
        stats_.voicePulls += gNumVoices;
        stats_.pulls++;
        float_.resize((size_t)frames * 2);
        unclamped_.resize((size_t)frames * 2);
        _ST_RenderVoices(float_.data(), stats ? unclamped_.data() : NULL, frames);
        _ST_ReapVoices();
        if (stats) {
            for (size_t j = 0; j < float_.size(); j++) {
                if (fabs(unclamped_[j]) > stats_.peak)
                    stats_.peak = fabs(unclamped_[j]);
                if (fabs(unclamped_[j] - float_[j]) > 1e-6)
                    stats_.clipped++;
            }
            stats_.samples += (long)float_.size();
        }
    }

    const void *Data() const { return float_.data(); }
    size_t Bytes() const { return float_.size() * sizeof(float); }
    const MixStats &Stats() const { return stats_; }

private:
    MixStats stats_;
    std::vector<float> float_;
    std::vector<float> unclamped_;
};

void Put16(unsigned char *p, uint32_t v)
{
    p[0] = (unsigned char)v;
    p[1] = (unsigned char)(v >> 8);
}

void Put32(unsigned char *p, uint32_t v)
{
    Put16(p, v);
    Put16(p + 2, v >> 16);
}

/* a stereo 32-bit float WAV header */
void WavHeader(FILE *f, int rate, uint32_t dataBytes)
{
    unsigned char h[44];
    uint32_t bits = 32;
    uint32_t block = 2 * bits / 8;

    memcpy(h, "RIFF", 4);
    Put32(h + 4, 36 + dataBytes);
    memcpy(h + 8, "WAVEfmt ", 8);
    Put32(h + 16, 16);
    Put16(h + 20, 3);
    Put16(h + 22, 2);
    Put32(h + 24, (uint32_t)rate);
    Put32(h + 28, (uint32_t)rate * block);
    Put16(h + 32, block);
    Put16(h + 34, bits);
    memcpy(h + 36, "data", 4);
    Put32(h + 40, dataBytes);
    fseek(f, 0, SEEK_SET);
    fwrite(h, 1, sizeof h, f);
}

void LogVoices(FILE *log, int pull)
{
    std::vector<long> ids;

    for (int i = 0; i < gNumVoices; i++)
        ids.push_back(RunId(gTheVoices[i].id));
    std::sort(ids.begin(), ids.end());
    fprintf(log, "pull %d:", pull);
    for (long id : ids)
        fprintf(log, " %ld", id);
    fprintf(log, "\n");
}

/* the script through the offline voices */
void Capture(Sound *const *clips)
{
    FILE *log = PlatFileOpen("soundtest_sdl.txt", "wb");
    FILE *out = PlatFileOpen("soundtest_sdl.raw", "wb");
    FILE *voices = PlatFileOpen("soundtest_sdl_voices.txt", "wb");
    Mixer mixer;

    if (log != NULL && out != NULL && voices != NULL) {
        fprintf(log, "sound tool open %d, %d channels\n", (int)gSoundToolOpen, kSoundChannels);
        for (int i = 0; i < kClipCount; i++)
            fprintf(log, "snd %d: %s\n", kClipIds[i], clips[i] != NULL ? "loaded" : "missing");
        gFirstId = 0;
        ST_HaltSound(0);
        for (int pull = 0; pull < kPulls; pull++) {
            Script(pull, clips, log);
            mixer.Pull(pull, false);
            fwrite(mixer.Data(), 1, mixer.Bytes(), out);
            fprintf(log, "pull %d: %d voices\n", pull, (int)gNumVoices);
            LogVoices(voices, pull);
        }
        ST_HaltSound(0);
    }
    if (voices != NULL)
        fclose(voices);
    if (out != NULL)
        fclose(out);
    if (log != NULL)
        fclose(log);
}

/* a scenario through the offline voices at a preferences volume, into a WAV file unless name is NULL, its
   numbers into `stats' under `title' */
void Render(const char *name, const char *title, Sound *const *clips, FILE *stats, bool warp, short volume)
{
    FILE *out = name != NULL ? PlatFileOpen(name, "wb") : NULL;
    Mixer mixer;
    uint64_t seed = kFightSeed;
    WarpRun run = { 0, 0, false };
    uint32_t bytes = 0;
    int pulls = 0;

    if (name != NULL && out == NULL)
        return;
    settings.volume = volume;
    AdjustSoundVolume();
    if (out != NULL)
        WavHeader(out, kPlatVoicesRenderRate, 0);
    ST_HaltSound(0);
    for (pulls = 0; warp ? pulls < kWarpMaxPulls : pulls < kFightPulls; pulls++) {
        if (warp) {
            if (!Warp(pulls, clips[kWarpUp], &run))
                break;
        } else {
            Fight(pulls, clips, &seed);
        }
        mixer.Pull(pulls, true);
        if (out != NULL)
            fwrite(mixer.Data(), 1, mixer.Bytes(), out);
        bytes += (uint32_t)mixer.Bytes();
    }
    ST_HaltSound(0);
    if (out != NULL) {
        WavHeader(out, kPlatVoicesRenderRate, bytes);
        fclose(out);
    }

    const MixStats &m = mixer.Stats();
    if (stats != NULL)
        fprintf(stats, "%s, volume %d: %d pulls, voices at most %d, %.1f on average, peak before "
                "clamping %.3f, clipped %ld of %ld samples\n", title, volume, pulls, m.peakVoices,
                m.pulls != 0 ? (double)m.voicePulls / m.pulls : 0.0, m.peak, m.clipped, m.samples);
}

void RunAll(Sound *const *clips, FILE *stats)
{
    const char *fight = "soundtest_fight_sdl.wav";
    const char *warp = "soundtest_warp_sdl.wav";

    Capture(clips);
    Render(fight, fight, clips, stats, false, kTopVolume);
    Render(NULL, fight, clips, stats, false, kDefaultVolume);
    Render(warp, warp, clips, stats, true, kTopVolume);
    Render(NULL, warp, clips, stats, true, kDefaultVolume);
}

} // namespace

void DebugSoundTest(void)
{
    Sound *clips[kClipCount];
    short volume = settings.volume;
    Boolean stereoPan = settings.stereoPan;
    SoundRequest warpSpec = warpUpSoundSpec;
    FILE *stats;

    LogLoads();
    for (int i = 0; i < kClipCount; i++)
        clips[i] = ST_LoadSndResource(kClipIds[i]);
    stats = PlatFileOpen("soundtest_wav.txt", "wb");
    settings.stereoPan = 1;

    ST_Close();
    _ST_OpenOffline();
    RunAll(clips, stats);
    ST_Close();
    ST_Open();

    settings.volume = volume;
    AdjustSoundVolume();
    settings.stereoPan = stereoPan;
    warpUpSoundSpec = warpSpec;
    if (stats != NULL)
        fclose(stats);
    for (Sound *clip : clips)
        ST_DisposeSound(clip);
}
