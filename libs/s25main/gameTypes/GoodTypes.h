// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Return To The Roots.
//
// Return To The Roots is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Return To The Roots is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Return To The Roots. If not, see <http://www.gnu.org/licenses/>.

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
    /* 35 */ Nothing         // Nothing. Is not counted as a good. TODO: Remove
};
constexpr auto maxEnumValue(GoodType)
{
    return GoodType::ShieldJapanese;
}
// Number of tools
constexpr unsigned NUM_TOOLS = 12;
/// Offset into the map image archive to get the ware stack (lying on ground) texture
constexpr unsigned WARE_STACK_TEX_MAP_OFFSET = 2200;
/// Offset into the map image archive to get the ware texture
constexpr unsigned WARES_TEX_MAP_OFFSET = 2250;
/// Offset into the map image archive to get the ware texture when carried by a donkey
constexpr unsigned WARES_DONKEY_TEX_MAP_OFFSET = 2350;
