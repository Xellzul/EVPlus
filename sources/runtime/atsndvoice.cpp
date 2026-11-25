/* atsndvoice.cpp -- the Sound Toolkit's channels played on SDL's voices (platform.h): each row's sound is
   stretched to the render rate once, when it starts, by the toolkit's resampler (PCM_Mixer) at the row's
   pitch and levels, and handed to the row's voice; SDL only mixes. */

#include <vector>
#include "burgerchan.h"
#include "platform.h"

#define kVoiceHeadroom  1.0f                     /* the whole mix's gain */
#define kRateShift      12                       /* PCM_Mixer's step is 20.12 */
#define kFracOne        0x100
#define kVoiceStepShift 4                        /* 16.16 to 20.12 */
#define kPassFrames     (1u << 19)               /* one PCM_Mixer call at most, so its step count stays in 32 bits */

static PlatVoices *gVoices;
static std::vector<short> gRendered;

int32_t _ST_OpenVoices(bool offline)
{
    gVoices = PlatVoicesOpen(kSoundChannels, kVoiceHeadroom, offline);
    return gVoices != NULL ? 0 : kNoDevice;
}

void _ST_CloseVoices(void)
{
    PlatVoicesClose(gVoices);
    gVoices = NULL;
    gRendered = std::vector<short>();
}

/* the frames one PCM_Mixer call writes for `count' samples at `step'; 0 = none, or the call would never end */
static uint64_t RenderedFrames(uint32_t count, uint32_t step)
{
    uint64_t accum = 0;
    uint64_t last = 0;
    uint64_t delta;
    uint64_t frames = 0;

    for (uint32_t i = 0; i < count; i++) {
        accum += step;
        delta = (accum >> kRateShift) - (last >> kRateShift);
        if (delta >= 1) {
            if (delta > kFracOne)
                return 0;
            last = accum;
            frames += kFracOne / (kFracOne / delta);
        }
    }
    return frames;
}

/* rate as in SoundRequest: 16.16, kUnityRate = as recorded, more = slower; a row not started ends at once */
void _ST_StartVoice(const SoundChannel *chan, int32_t rate)
{
    ULong step;
    uint64_t total;
    ULong frames;
    size_t done;
    MixVoice voice;
    short *out;
    unsigned char ended;

    if (rate <= 0)
        return;
    step = UFix32_Multiply(UFix32_Divide((ULong)kPlatVoicesRenderRate << 16, ST_SoundRate(chan->sound)), (ULong)rate)
           >> kVoiceStepShift;
    total = RenderedFrames(chan->voice.total, step);
    if (total == 0)
        return;
    voice = chan->voice;
    voice.pos = 0;
    voice.lastLeft = 0;
    voice.lastRight = 0;
    frames = total < kPassFrames ? (ULong)total : kPassFrames;
    gRendered.clear();
    do {
        done = gRendered.size();
        gRendered.resize(done + (size_t)frames * 2, 0);
        out = gRendered.data() + done;
        ended = PCM_Mixer(chan->data, &out, frames, step, &voice, chan->leftLevel, chan->rightLevel);
        gRendered.resize((size_t)(out - gRendered.data()));
    } while (ended == 0 && total > kPassFrames);
    if (PlatVoicesStart(gVoices, chan->out, gRendered.data(), (int)(gRendered.size() / 2), kPlatVoicesRenderRate,
                        1.0) != 0)
        PlatVoicesStop(gVoices, chan->out);
}

void _ST_StopVoice(unsigned char out)
{
    PlatVoicesStop(gVoices, out);
}

void _ST_ReapVoices(void)
{
    int32_t i;

    for (i = 0; i < gNumVoices; i++) {
        if (PlatVoicesEnded(gVoices, gTheVoices[i].out)) {
            _ST_HaltASound((short)i);
            i--;
        }
    }
}

#if EVPLUS_TEST_PAGES
void _ST_RenderVoices(float *out, float *unclamped, int frames)
{
    PlatVoicesRender(gVoices, out, unclamped, frames);
}
#endif
