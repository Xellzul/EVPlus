/* atsound.cpp -- the Sound Toolkit's channel table: bringing the sound layer up and down, and starting and
   halting sounds over the channels, a new sound taking the place of a quieter or lower-priority one.
   Loading is in atsndload.cpp, playing in atsndvoice.cpp.
   Notes: Research/notes/atsound.md */

#include "burgerchan.h"

int32_t gCookieCounter;                               /* 0x00870df4 */
unsigned char gSoundToolOpen;                      /* 0x00870e12, live flag */
int32_t gNumVoices;                               /* 0x00870db0, in use */
SoundChannel gTheVoices[kSoundChannels];       /* 0x0086f718 */

static int32_t OpenTool(bool offline)
{
    int32_t err;
    int32_t i;

    err = 0;
    if (gSoundToolOpen != 0)
        goto done;

    gCookieCounter = 1;
    gNumVoices = 0;
    memset(gTheVoices, 0, sizeof gTheVoices);
    for (i = 0; i < kSoundChannels; i++)
        gTheVoices[i].out = (unsigned char)i;

    err = _ST_OpenVoices(offline);
    if (err != 0)
        goto done;

    gSoundToolOpen = 1;
    ST_HaltSound(0);
done:
    return err;
}

// FUNCTION: EVNOVA 0x004d6290
int32_t ST_Open(void)
{
    return OpenTool(false);
}

#if EVPLUS_TEST_PAGES
int32_t _ST_OpenOffline(void)
{
    return OpenTool(true);
}
#endif

// FUNCTION: EVNOVA 0x004d6440
void ST_Close(void)
{
    if (gSoundToolOpen != 0) {
        ST_HaltSound(0);
        _ST_CloseVoices();
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
        _ST_ReapVoices();
        while (i < kSoundChannels && gTheVoices[i].sound != 0 && preempt == 0) {
            if (req->priority >= gTheVoices[i].priority
                && req->rightLevel + req->leftLevel
                   >= gTheVoices[i].leftLevel + gTheVoices[i].rightLevel)
                preempt = 1;
            else
                i++;
        }

        if (i >= kSoundChannels)
            return 0;

        if (gNumVoices >= kSoundChannels)
            gNumVoices = kSoundChannels - 1;

        /* the new row takes the free row's voice, or the voice of the row it pushes off the end */
        out = gTheVoices[gNumVoices].out;
        if (gTheVoices[gNumVoices].sound != 0)
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

        gTheVoices[i].priority = req->priority;
        gTheVoices[i].leftLevel = req->leftLevel;
        gTheVoices[i].rightLevel = req->rightLevel;
        gTheVoices[i].out = out;

        if (req->id != 0) {
            gTheVoices[i].id = req->id;
        } else {
            gTheVoices[i].id = gCookieCounter;
            gCookieCounter += kIdStep;
        }

        id = gTheVoices[i].id;
        gNumVoices++;
        _ST_StartVoice(&gTheVoices[i], req->rate);
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
        _ST_ReapVoices();
        while (i < gNumVoices) {
            if (ChannelMatches(&gTheVoices[i], key))
                found = found + 1;
            i = i + 1;
        }
    }
    return found;
}

/* Counts the channels this sound occupies; a null sound counts them all. */
// FUNCTION: EVNOVA 0x004d6770
unsigned short ST_IsSoundPlaying(const Sound *sound)
{
    return CountPlaying((uintptr_t)sound);
}

static void HaltMatching(uintptr_t key)
{
    ULong i;

    if (gSoundToolOpen != 0) {
        i = 0;
        while (i < gNumVoices) {
            if (ChannelMatches(&gTheVoices[i], key)) {
                _ST_HaltASound((short)i);
            } else {
                i++;
            }
        }
    }
}

// FUNCTION: EVNOVA 0x004d67d0
void ST_HaltSound(const Sound *sound)
{
    HaltMatching((uintptr_t)sound);
}

#if EVPLUS_TEST_PAGES
void ST_HaltSoundId(uint32_t id)
{
    if (id != 0)
        HaltMatching(id);
}
#endif

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
