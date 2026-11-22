/* atsnddrv.cpp -- the Sound Toolkit's mixer and output: every playing channel resampled and added into
   16-bit stereo, pulled by the platform layer's audio (platform.h) as the device needs it.
   Notes: Research/notes/atsnddrv.md */

#include "burgerchan.h"
#include "platform.h"

#define kRateShift        12
#define kFracOne          0x100
#define kLevelShift       7

/* Read a voice at a 20.12 step, add it into *out (stereo 16-bit, frames long) with linear interpolation
   between the samples it lands on, and advance. out 0 only advances. 0xff once the voice has run out.
   Each call starts its step count afresh. */
// FUNCTION: EVNOVA 0x00507f90
unsigned char PCM_Mixer(const short *data, short **out, ULong frames,
                        ULong step, MixVoice *voice,
                        unsigned short left, unsigned short right)
{
    short *dst;
    ULong remaining;
    ULong pos;
    ULong accum;
    ULong lastAccum;
    ULong delta;
    int32_t sample;
    int32_t lastL;
    int32_t lastR;
    int32_t l;
    int32_t r;
    int32_t frac;
    int32_t s;
    char done;

    accum = lastAccum = 0;
    done = 0;
    pos = voice->pos;
    lastL = voice->lastLeft;
    lastR = voice->lastRight;
    dst = out != 0 ? *out : 0;
    remaining = voice->total - pos;

    while (remaining != 0) {
        sample = data[pos];
        remaining--;
        pos++;

        accum += step;
        delta = (accum >> kRateShift) - (lastAccum >> kRateShift);
        if (delta >= 1) {
            lastAccum = accum;
            l = (sample * left) >> kLevelShift;
            r = (sample * right) >> kLevelShift;
            for (frac = kFracOne / delta; frac <= kFracOne; frac += kFracOne / delta) {
                if (dst != 0) {
                    s = (((l - lastL) * frac) >> 8) + lastL + dst[0];
                    dst[0] = s < -32768 ? -32768 : (s > 32767 ? 32767 : s);
                    s = (((r - lastR) * frac) >> 8) + lastR + dst[1];
                    dst[1] = s < -32768 ? -32768 : (s > 32767 ? 32767 : s);
                    dst += 2;
                }
                if (--frames == 0)
                    goto finished;
            }
            lastL = l;
            lastR = r;
        }
    }

finished:
    if (remaining == 0)
        done = 0xff;
    voice->pos = pos;
    voice->lastLeft = lastL;
    voice->lastRight = lastR;
    if (out != 0)
        *out = dst;
    return done;
}

#define kVoiceStepShift   4                           /* 16.16 rate to the mixer's 20.12 */

/* Mix every playing channel into a zeroed stereo 16-bit buffer, then retire the channels that ran out.
   Channels past the voice limit are advanced without being heard. */
// FUNCTION: EVNOVA 0x004d6b90
void _ST_MixVoices(short *buffer, ULong frames)
{
    int32_t i;
    SoundChannel *chan;
    short *out;

    memset(buffer, 0, frames * 4);
    for (i = 0; i < gNumVoices; i++) {
        chan = &gTheVoices[i];
        out = buffer;
        if (i < gMaxVoices)
            chan->finished = PCM_Mixer(chan->data, &out, frames, chan->stream >> kVoiceStepShift,
                                       &chan->voice, chan->leftLevel, chan->rightLevel);
        else
            chan->finished = PCM_Mixer(chan->data, 0, frames, chan->stream >> kVoiceStepShift,
                                       &chan->voice, chan->leftLevel, chan->rightLevel);
    }

    for (i = 0; i < gNumVoices; i++) {
        if (gTheVoices[i].finished != 0) {
            _ST_HaltASound(i);
            i--;
        }
    }
}

// FUNCTION: EVNOVA 0x004d6d00
void _ST_MixerDispatch(short *buffer, ULong frames)
{
    CriticalEnter(&gCriticalSection);
    _ST_MixVoices(buffer, frames);
    CriticalLeave(&gCriticalSection);
}

static PlatAudio *gMixerOutput;

/* called on the platform's audio thread; the mix never ends */
static int MixerPull(void *ctx, int16_t *out, int frames)
{
    _ST_MixerDispatch(out, frames);
    return frames;
}

// FUNCTION: EVNOVA 0x00507300
int32_t _ST_CreateSoundPump(void)
{
    gMixerOutput = PlatAudioOpen(UFix32_To_UInt32(gSampleRate), MixerPull, 0);
    if (gMixerOutput == 0)
        return kNoDevice;
    return 0;
}

// FUNCTION: EVNOVA 0x005076a0
void _ST_DisposeSoundPump(void)
{
    PlatAudioClose(gMixerOutput);
    gMixerOutput = 0;
}
