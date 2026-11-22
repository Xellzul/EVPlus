/* burgeronce.cpp -- BurgerLib W9Win95Code: the active flag. Notes: Research/notes/burgeronce.md */

#include "burger.h"

Word Win95ApplsActive = 1;                       /* 0x00578248 */

/* The exe's guard against a second copy (Win95Allow1Instance, 0x004f9c00: a named file mapping, "Lock for
   EV Nova"; a second copy found it and ended at once without a word) is gone: two copies may run, each
   from its own folder. */
