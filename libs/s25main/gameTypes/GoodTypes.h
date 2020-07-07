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

#ifndef GoodTypes_h__
#define GoodTypes_h__

#include "helpers/MaxEnumValue.h"
#include "mygettext/mygettext.h"
#include <string>

// Warentypen
enum GoodType
{
    /*  0 */ GD_BEER,           // Bier
    /*  1 */ GD_TONGS,          // Zange
    /*  2 */ GD_HAMMER,         // Hammer
    /*  3 */ GD_AXE,            // Axt
    /*  4 */ GD_SAW,            // Säge
    /*  5 */ GD_PICKAXE,        // Spitzhacke
    /*  6 */ GD_SHOVEL,         // Schaufel
    /*  7 */ GD_CRUCIBLE,       // Schmelztiegel
    /*  8 */ GD_RODANDLINE,     // Angel
    /*  9 */ GD_SCYTHE,         // Sense
    /* 10 */ GD_WATEREMPTY,     // Wasser
    /* 11 */ GD_WATER,          // Wasser
    /* 12 */ GD_CLEAVER,        // Beil
    /* 13 */ GD_ROLLINGPIN,     // Nudelholz
    /* 14 */ GD_BOW,            // Bogen
    /* 15 */ GD_BOAT,           // Boot
    /* 16 */ GD_SWORD,          // Schwert
    /* 17 */ GD_IRON,           // Eisen
    /* 18 */ GD_FLOUR,          // Mehl
    /* 19 */ GD_FISH,           // Fisch
    /* 20 */ GD_BREAD,          // Brot
    /* 21 */ GD_SHIELDROMANS,   // Schild
    /* 22 */ GD_WOOD,           // Holz
    /* 23 */ GD_BOARDS,         // Bretter
    /* 24 */ GD_STONES,         // Steine
    /* 25 */ GD_SHIELDVIKINGS,  // Schild
    /* 26 */ GD_SHIELDAFRICANS, // Schild
    /* 27 */ GD_GRAIN,          // Getreide
    /* 28 */ GD_COINS,          // Mnzen
    /* 29 */ GD_GOLD,           // Gold
    /* 30 */ GD_IRONORE,        // Eisenerz
    /* 31 */ GD_COAL,           // Kohle
    /* 32 */ GD_MEAT,           // Fleisch
    /* 33 */ GD_HAM,            // Schinken ( Schwein )
    /* 34 */ GD_SHIELDJAPANESE, // Schild
    /* 35 */ GD_NOTHING         // Nothing. Is not counted as a good. TODO: Remove
};
DEFINE_MAX_ENUM_VALUE(GoodType, GD_SHIELDJAPANESE)
/// Number of goods
constexpr unsigned NUM_WARE_TYPES = helpers::NumEnumValues_v<GoodType>;
// Number of tools
constexpr unsigned NUM_TOOLS = 12;

const std::string WARE_NAMES[NUM_WARE_TYPES] = {
  /*  0 */ gettext_noop("Beer"),         // Bier
  /*  1 */ gettext_noop("Tongs"),        // Zange
  /*  2 */ gettext_noop("Hammer"),       // Hammer
  /*  3 */ gettext_noop("Axe"),          // Axt
  /*  4 */ gettext_noop("Saw"),          // Säge
  /*  5 */ gettext_noop("Pick-axe"),     // Spitzhacke
  /*  6 */ gettext_noop("Shovel"),       // Schaufel
  /*  7 */ gettext_noop("Crucible"),     // Schmelztiegel
  /*  8 */ gettext_noop("Rod and line"), // Angel
  /*  9 */ gettext_noop("Scythe"),       // Sense
  /* 10 */ gettext_noop("Water"),        // Wasser
  /* 11 */ gettext_noop("Water"),        // Wasser
  /* 12 */ gettext_noop("Cleaver"),      // Beil
  /* 13 */ gettext_noop("Rolling pin"),  // Nudelholz
  /* 14 */ gettext_noop("Bow"),          // Bogen
  /* 15 */ gettext_noop("Boat"),         // Boot
  /* 16 */ gettext_noop("Sword"),        // Schwert
  /* 17 */ gettext_noop("Iron"),         // Eisen
  /* 18 */ gettext_noop("Flour"),        // Mehl
  /* 19 */ gettext_noop("Fish"),         // Fisch
  /* 20 */ gettext_noop("Bread"),        // Brot
  /* 21 */ gettext_noop("Shield"),       // Schild
  /* 22 */ gettext_noop("Wood"),         // Holz
  /* 23 */ gettext_noop("Boards"),       // Bretter
  /* 24 */ gettext_noop("Stones"),       // Steine
  /* 25 */ "",                           // Schild
  /* 26 */ "",                           // Schild
  /* 27 */ gettext_noop("Grain"),        // Getreide
  /* 28 */ gettext_noop("Coins"),        // Mnzen
  /* 29 */ gettext_noop("Gold"),         // Gold
  /* 30 */ gettext_noop("Iron ore"),     // Eisenerz
  /* 31 */ gettext_noop("Coal"),         // Kohle
  /* 32 */ gettext_noop("Meat"),         // Fleisch
  /* 33 */ gettext_noop("Ham"),          // Schinken ( Schwein )
  /* 34 */ "",                           // Schild
};

#endif // GoodTypes_h__
