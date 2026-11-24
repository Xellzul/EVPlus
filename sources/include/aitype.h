/* aitype.h -- AI type values (ShipObj.aiType, shïp inherentAI, düde / përs aiType) and their groups */
#pragma once

constexpr short kAINone        = -1;   /* no AI (player, derelict, removed ship); as data / argument: unset, keep inherentAI */
constexpr short kAIDefault     = 0;    /* cleared slot; in düde / përs data: use the shïp's inherentAI */
constexpr short kAIWimpyTrader = 1;    /* shïp inherentAI 1..4 */
constexpr short kAIBraveTrader = 2;
constexpr short kAIWarship     = 3;
constexpr short kAIInterceptor = 4;
constexpr short kAIFighter     = 5;    /* launched from a bay */
constexpr short kAIEscort      = 6;    /* a hired escort */

/* The groups below are ranges: kAINone and kAIDefault count as traders. */
constexpr bool AIIsTrader(short t)  { return t <= kAIBraveTrader; }
constexpr bool AIIsCombat(short t)  { return t >= kAIWarship; }
constexpr bool AIIsOwnAI(short t)   { return t <= kAIInterceptor; } /* not a fighter or escort */
constexpr bool AIHasLeader(short t) { return t >= kAIFighter; }    /* bay fighter or escort */
constexpr bool AIIsSet(short t)     { return t >= kAIWimpyTrader; }
