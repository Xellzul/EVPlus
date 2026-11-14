// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -lang c++ -bool off
/* licensescramble.c -- ScrambleRand: seeds the random number generator from the clock (Rand(0)) at the start of InitEVNova; exe 0x004ab970, the object after music.c's .CRT hook 0x004ab960. Notes: Research/notes/music.md */

#include "macport.h"
#include "game.h"
#include "sound.h"
#include "player.h"
#include "render.h"
#include "burger.h"
#include "gameutil.h"

/* the only caller is InitEVNova, which discards the result: Rand(0) reseeds randomSeed */

/* ScrambleRand. STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004ab970
short ScrambleRand(void)
{
    return Rand(0);
}
