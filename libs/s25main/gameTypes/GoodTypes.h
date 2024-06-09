// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "helpers/MaxEnumValue.h"
#include "mygettext/mygettext.h"
#include <cstdint>
#include <string>

// Warentypen
enum class GoodType : uint8_t
{
    /*  0 */ Beer,           // Bier
    /*  1 */ Tongs,          // Zange
    /*  2 */ Hammer,         // Hammer
    /*  3 */ Axe,            // Axt
    /*  4 */ Saw,            // Säge
    /*  5 */ PickAxe,        // Spitzhacke
    /*  6 */ Shovel,         // Schaufel
    /*  7 */ Crucible,       // Schmelztiegel
    /*  8 */ RodAndLine,     // Angel
    /*  9 */ Scythe,         // Sense
    /* 10 */ WaterEmpty,     // Wasser
    /* 11 */ Water,          // Wasser
    /* 12 */ Cleaver,        // Beil
    /* 13 */ Rollingpin,     // Nudelholz
    /* 14 */ Bow,            // Bogen
    /* 15 */ Boat,           // Boot
    /* 16 */ Sword,          // Schwert
    /* 17 */ Iron,           // Eisen
    /* 18 */ Flour,          // Mehl
    /* 19 */ Fish,           // Fisch
    /* 20 */ Bread,          // Brot
    /* 21 */ ShieldRomans,   // Schild
    /* 22 */ Wood,           // Holz
    /* 23 */ Boards,         // Bretter
    /* 24 */ Stones,         // Steine
    /* 25 */ ShieldVikings,  // Schild
    /* 26 */ ShieldAfricans, // Schild
    /* 27 */ Grain,          // Getreide
    /* 28 */ Coins,          // Mnzen
    /* 29 */ Gold,           // Gold
    /* 30 */ IronOre,        // Eisenerz
    /* 31 */ Coal,           // Kohle
    /* 32 */ Meat,           // Fleisch
    /* 33 */ Ham,            // Schinken ( Schwein )
    /* 34 */ ShieldJapanese, // Schild
    /* 35 */ Grapes,
    /* 36 */ Wine,
    /* 37 */ Nothing         // Nothing. Is not counted as a good. TODO: Remove
};
constexpr auto maxEnumValue(GoodType)
{
    return GoodType::Wine;
}

/// List of all tools (correspond to buttons at IO:140-163)
enum class Tool
{
    Tongs,
    Hammer,
    Axe,
    Saw,
    PickAxe,
    Shovel,
    Crucible,
    RodAndLine,
    Scythe,
    Cleaver,
    Rollingpin,
    Bow
};
constexpr auto maxEnumValue(Tool)
{
    return Tool::Bow;
}

/// Offset into the map image archive to get the ware stack (lying on ground) texture
constexpr unsigned WARE_STACK_TEX_MAP_OFFSET = 2200;
/// Offset into the map image archive to get the ware texture
constexpr unsigned WARES_TEX_MAP_OFFSET = 2250;
/// Offset into the map image archive to get the ware texture when carried by a donkey
constexpr unsigned WARES_DONKEY_TEX_MAP_OFFSET = 2350;
