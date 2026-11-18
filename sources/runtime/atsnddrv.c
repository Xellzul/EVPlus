/* atsnddrv.c -- the sound output driver: the mixer's output on the platform layer's audio (platform.h), which pulls the mix as the device needs it. The exe's two outputs (DirectSound with a looping secondary buffer, waveOut with two ping-ponged headers), their mixer thread, the device rate probe and the device volume helpers are gone. Notes: Research/notes/atsnddrv.md */

#include "burgertypes.h"
#include "burger.h"
#include "ambrosia.h"
#include "platform.h"

extern void _ST_MixerDispatch(short *buffer, ULong frames);   /* atsound.c */

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
