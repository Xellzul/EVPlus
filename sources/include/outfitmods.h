/* outfitmods.h -- ShipObj::Class() and the walks over outfit mod slots */
#pragma once

#include "gameconst.h"
#include "resources.h"
#include "shipobj.h"

inline ShipRec &ShipObj::Class() const
{
    return shipClass[type];
}

/*
 * Calls fn(modVal, owned count) for every mod slot of type `type` on the
 * player's owned outfits, outfit by outfit; stops when fn returns true.
 * Returns whether it stopped.
 */
template <class Fn>
bool ForEachPlayerMod(short type, Fn fn)
{
    short i;
    short j;

    for (i = 0; i < kMaxOutfits; i++) {
        if (shipSystems[i] > 0) {
            for (j = 0; j < kOutfMods; j++) {
                if (outfitSystem[i].modType[j] == type && fn(outfitSystem[i].modVal[j], shipSystems[i]))
                    return true;
            }
        }
    }
    return false;
}

/* The same over a ship class's default outfits: fn(modVal, default count). */
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
