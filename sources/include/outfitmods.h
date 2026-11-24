/* outfitmods.h -- ShipObj::Class() and the walks over outfit mod slots */
#pragma once

#include "gameconst.h"
#include "resources.h"
#include "shipobj.h"

inline ShipRec &ShipObj::Class() const
{
    return shipClass[type];
}

/* What a ForEachPlayerOutfitMod callback wants next. */
enum class ModWalk { Next, NextOutfit, Stop };

/*
 * Calls fn(outfit index, modVal, owned count) for every mod slot of type
 * `type` on the player's owned outfits, outfit by outfit. fn returns Next
 * (go on), NextOutfit (skip this outfit's other slots) or Stop.
 * Returns whether it stopped.
 */
template <class Fn>
bool ForEachPlayerOutfitMod(short type, Fn fn)
{
    short i;
    short j;
    ModWalk next;

    for (i = 0; i < kMaxOutfits; i++) {
        if (shipSystems[i] > 0) {
            for (j = 0; j < kOutfMods; j++) {
                if (outfitSystem[i].modType[j] == type) {
                    next = fn(i, outfitSystem[i].modVal[j], shipSystems[i]);
                    if (next == ModWalk::Stop)
                        return true;
                    if (next == ModWalk::NextOutfit)
                        break;
                }
            }
        }
    }
    return false;
}

/* The same without the index: fn(modVal, owned count), true stops. */
template <class Fn>
bool ForEachPlayerMod(short type, Fn fn)
{
    return ForEachPlayerOutfitMod(type, [&](short, short val, short owned) {
        return fn(val, owned) ? ModWalk::Stop : ModWalk::Next;
    });
}

/* ForEachPlayerMod over a ship class's default outfits: fn(modVal, default count). */
template <class Fn>
bool ForEachDefaultMod(const ShipRec &cls, short type, Fn fn)
{
    short i;
    short j;

    for (i = 0; i < kDefaultOutfitSlots; i++) {
        if (cls.defOutfCount[i] > 0) {
            for (j = 0; j < kOutfMods; j++) {
                if (outfitSystem[cls.defOutfID[i]].modType[j] == type
                        && fn(outfitSystem[cls.defOutfID[i]].modVal[j], cls.defOutfCount[i]))
                    return true;
            }
        }
    }
    return false;
}

inline bool PlayerHasMod(short type)
{
    return ForEachPlayerMod(type, [](short, short) { return true; });
}

inline bool DefaultHasMod(const ShipRec &cls, short type)
{
    return ForEachDefaultMod(cls, type, [](short, short) { return true; });
}

/*
 * Marines mod totals: positive values add boarding crew, negative ones add
 * -value percent to capture odds; each times the outfit count.
 */
struct MarineTotals {
    int32_t crew = 0;
    int32_t captureBonus = 0;
};

/* The player's owned outfits; owned counts are not tested (unlike ForEachPlayerMod). */
inline MarineTotals PlayerMarines()
{
    MarineTotals m;
    short i;
    short j;
    short val;

    for (i = 0; i < kMaxOutfits; i++) {
        for (j = 0; j < kOutfMods; j++) {
            if (outfitSystem[i].modType[j] != OutfMod::Marines)
                continue;
            val = outfitSystem[i].modVal[j];
            if (val > 0)
                m.crew += val * shipSystems[i];
            else if (val < 0)
                m.captureBonus += -val * shipSystems[i];
        }
    }
    return m;
}

/* A ship class's default outfits. */
inline MarineTotals ShipClassMarines(const ShipRec &cls)
{
    MarineTotals m;
    short i;
    short j;
    short val;

    for (i = 0; i < kDefaultOutfitSlots; i++) {
        if (cls.defOutfCount[i] <= 0)
            continue;
        for (j = 0; j < kOutfMods; j++) {
            if (outfitSystem[cls.defOutfID[i]].modType[j] != OutfMod::Marines)
                continue;
            val = outfitSystem[cls.defOutfID[i]].modVal[j];
            if (val > 0)
                m.crew += val * cls.defOutfCount[i];
            else if (val < 0)
                m.captureBonus += -val * cls.defOutfCount[i];
        }
    }
    return m;
}
