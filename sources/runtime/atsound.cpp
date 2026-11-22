/* atsound.cpp -- the Sound Toolkit's channel table: bringing the sound layer up and down, and starting and
   halting sounds over the channels, a new sound taking the place of a quieter or lower-priority one.
   Loading is in atsndload.cpp, the mix in atsnddrv.cpp (original) or atsndvoice.cpp (SDL).
   Notes: Research/notes/atsound.md */

#include "burgerchan.h"

int32_t gMaxVoices;                               /* 0x00870958 channels */
int32_t gCookieCounter;                               /* 0x00870df4 */
unsigned char gSoundToolOpen;                      /* 0x00870e12, live flag */
int32_t gNumVoices;                               /* 0x00870db0, in use */
SoundChannel gTheVoices[kMaxVoiceSlots];       /* 0x0086f718 */
int32_t gVoiceSlots = kSoundChannels;
bool gOriginalMixer;
static bool gWantOriginalMixer;
Mutex gCriticalSection;                             /* 0x0086dc88, the lock (16 bytes of storage in the exe) */
ULong gSampleRate;                                   /* 0x0086ff90 */

/* the mix is 16-bit stereo at this rate (16.16): what the exe chose on a machine with DirectSound */
#define kOutputRate     0xac440000UL                     /* 44100.0 */

void ST_UseOriginalMixer(bool on)
{
    gWantOriginalMixer = on;
}

static int32_t OpenTool(unsigned short channels, uint32_t device, bool original, bool offline)
{
    int32_t err, e;
    int32_t i;

    err = 0;
    if (gSoundToolOpen != 0)
        goto done;

    e = CriticalCreate(&gCriticalSection);
    if (e != 0) {
        err = e;
        goto done;
    }

    gOriginalMixer = original;
    if (gOriginalMixer) {
        gVoiceSlots = kSoundChannels;
        gMaxVoices = channels;
        if (gMaxVoices > kSoundChannels)
            gMaxVoices = kSoundChannels;
    } else {
        gVoiceSlots = kMaxVoiceSlots;
        gMaxVoices = kMaxVoiceSlots;
    }

    gCookieCounter = 1;
    gNumVoices = 0;
    memset(gTheVoices, 0, sizeof gTheVoices);
    for (i = 0; i < kMaxVoiceSlots; i++)
        gTheVoices[i].out = (unsigned char)i;

    if (device == 0)
        gSampleRate = kOutputRate;
    else
        gSampleRate = device;

    e = gOriginalMixer ? _ST_CreateSoundPump() : _ST_OpenVoices(offline);
    if (e != 0) {
        CriticalDispose(&gCriticalSection);
        err = e;
        goto done;
    }

    gSoundToolOpen = 1;
    ST_HaltSound(0);
done:
    return err;
}

// FUNCTION: EVNOVA 0x004d6290
int32_t ST_Open(unsigned short channels, uint32_t device)
{
    return OpenTool(channels, device, gWantOriginalMixer, false);
}

int32_t _ST_OpenOffline(void)
{
    return OpenTool(kMaxVoiceSlots, 0, false, true);
}

// FUNCTION: EVNOVA 0x004d6440
void ST_Close(void)
{
    if (gSoundToolOpen != 0) {
        ST_HaltSound(0);
        if (gOriginalMixer)
            _ST_DisposeSoundPump();
        else
            _ST_CloseVoices();
        CriticalDispose(&gCriticalSection);
        gSoundToolOpen = 0;
    }
}

// FUNCTION: EVNOVA 0x004d64f0
int32_t ST_PlaySound(Sound *snd, unsigned short priority, unsigned short level)
{
    return ST_PlaySoundStereo(snd, priority, level, level);
}

/* ST_PlaySound with its own level for each channel */
int32_t ST_PlaySoundStereo(Sound *snd, unsigned short priority, unsigned short left, unsigned short right)
{
    SoundRequest req;

    req.sound = snd;
    req.priority = priority;
    req.leftLevel = left;
    req.rightLevel = right;
    req.rate = kUnityRate;
    req.id = 0;
    return ST_PlaySoundParam(&req);
}

#define kMinPriority 1
#define kMaxLevel    0x80
#define kIdStep      2

// FUNCTION: EVNOVA 0x004d6550
ULong ST_PlaySoundParam(SoundRequest *req)
{
    ULong id;
    ULong i;
    char preempt;
    SoundChannel *src;
    SoundChannel *dst;
    int32_t tail;
    unsigned char out;

    tail = 0;
    id = 0;
    preempt = 0;
    if (req->sound != 0 && gSoundToolOpen != 0) {
        if (req->priority < kMinPriority)
            req->priority = kMinPriority;
        if (req->rightLevel > kMaxLevel)
            req->rightLevel = kMaxLevel;
        if (req->leftLevel > kMaxLevel)
            req->leftLevel = kMaxLevel;

        i = 0;
        CriticalEnter(&gCriticalSection);
        if (!gOriginalMixer)
            _ST_ReapVoices();
        while (i < gVoiceSlots && gTheVoices[i].sound != 0 && preempt == 0) {
            if (req->priority >= gTheVoices[i].priority
                && req->rightLevel + req->leftLevel
                   >= gTheVoices[i].leftLevel + gTheVoices[i].rightLevel)
                preempt = 1;
            else
                i++;
        }

        if (i >= gVoiceSlots) {
            CriticalLeave(&gCriticalSection);
            return 0;
        }

        if (gNumVoices >= gVoiceSlots)
            gNumVoices = gVoiceSlots - 1;

        /* the new row takes the free row's voice, or the voice of the row it pushes off the end */
        out = gTheVoices[gNumVoices].out;
        if (gTheVoices[gNumVoices].sound != 0 && !gOriginalMixer)
            _ST_StopVoice(out);
        src = &gTheVoices[i];
        dst = &gTheVoices[i + 1];
        tail = (gNumVoices - i) * kChannelBytes;
        if (tail != 0)
            memmove(dst, src, tail);

        gTheVoices[i].sound = req->sound;
        gTheVoices[i].data = ST_SoundData(req->sound);
        gTheVoices[i].voice.pos = 0;
        gTheVoices[i].voice.total = ST_SoundSamples(req->sound);
        gTheVoices[i].voice.lastLeft = 0;
        gTheVoices[i].voice.lastRight = 0;

        gTheVoices[i].stream = UFix32_Multiply(UFix32_Divide(gSampleRate, ST_SoundRate(req->sound)), req->rate);
        gTheVoices[i].priority = req->priority;
        gTheVoices[i].leftLevel = req->leftLevel;
        gTheVoices[i].rightLevel = req->rightLevel;
        gTheVoices[i].finished = 0;
        gTheVoices[i].out = out;

        if (req->id != 0) {
            gTheVoices[i].id = req->id;
        } else {
            gTheVoices[i].id = gCookieCounter;
            gCookieCounter += kIdStep;
        }

        id = gTheVoices[i].id;
        gNumVoices++;
        if (!gOriginalMixer)
            _ST_StartVoice(&gTheVoices[i], req->rate);
        CriticalLeave(&gCriticalSection);
    }
    return id;
}

/* a channel is picked by its id or by its sound; 0 picks them all */
static bool ChannelMatches(const SoundChannel *chan, uintptr_t key)
{
    return (uintptr_t)chan->id == key || (uintptr_t)chan->sound == key || key == 0;
}

static unsigned short CountPlaying(uintptr_t key)
{
    int32_t found;
    ULong i;

    found = 0;
    if (gSoundToolOpen != 0) {
        i = 0;
        CriticalEnter(&gCriticalSection);
        if (!gOriginalMixer)
            _ST_ReapVoices();
        while (i < gNumVoices) {
            if (ChannelMatches(&gTheVoices[i], key))
                found = found + 1;
            i = i + 1;
        }
        CriticalLeave(&gCriticalSection);
    }
    return found;
}

/* Counts the channels this sound occupies; a null sound counts them all. */
// FUNCTION: EVNOVA 0x004d6770
unsigned short ST_IsSoundPlaying(const Sound *sound)
{
    return CountPlaying((uintptr_t)sound);
}

unsigned short ST_IsSoundIdPlaying(uint32_t id)
{
    return id != 0 ? CountPlaying(id) : 0;
}

static void HaltMatching(uintptr_t key)
{
    ULong i;

    if (gSoundToolOpen != 0) {
        i = 0;
        CriticalEnter(&gCriticalSection);
        while (i < gNumVoices) {
            if (ChannelMatches(&gTheVoices[i], key)) {
                _ST_HaltASound((short)i);
            } else {
                i++;
            }
        }
        CriticalLeave(&gCriticalSection);
    }
}

// FUNCTION: EVNOVA 0x004d67d0
void ST_HaltSound(const Sound *sound)
{
    HaltMatching((uintptr_t)sound);
}

void ST_HaltSoundId(uint32_t id)
{
    if (id != 0)
        HaltMatching(id);
}

/* Drop one channel and close the table up behind it; the row freed at the end keeps the channel's voice. */
// FUNCTION: EVNOVA 0x004d6890
void _ST_HaltASound(short index)
{
    SoundChannel *src;
    SoundChannel *dst;
    int32_t tail;
    unsigned char out;

    if (gSoundToolOpen != 0) {
        out = gTheVoices[index].out;
        if (!gOriginalMixer)
            _ST_StopVoice(out);
        gNumVoices--;
        src = &gTheVoices[index + 1];
        dst = &gTheVoices[index];
        tail = (gNumVoices - index) * kChannelBytes;
        if (tail != 0)
            memmove(dst, src, tail);
        dst = &gTheVoices[gNumVoices];
        memset(dst, 0, kChannelBytes);
        dst->out = out;
    }
}
