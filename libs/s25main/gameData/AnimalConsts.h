// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#ifndef AnimalConsts_h__
#define AnimalConsts_h__

#include "helpers/EnumArray.h"
#include "gameTypes/AnimalTypes.h"

/// Informationen über die  einzelnen Tierarten
struct AnimalConst
{
    /// IDs in der map_lst, wo die Lauf-IDs von der jeweiligen Tierart beginnen
    unsigned short walking_id;
    /// IDs in der map_lst, wo die Schatten-IDs von der jeweiligen Tierart beginnen
    unsigned short shadow_id;
    /// IDs in der map_lst, wo die Totes-ID der jeweiligen Tierart liegt
    unsigned short dead_id;
    /// IDs in der map_lst, wo die Schatten-Totes-ID der jeweiligen Tierart liegt
    unsigned short shadow_dead_id;
    /// Anzahl Animationsschritte der einzelnen Tierarten
    unsigned short animation_steps;
    /// Schnelligkeit (Laufzeit in GF)
    unsigned short speed;
};

// 0 bedeutet --> kein Bild!

const helpers::EnumArray<AnimalConst, Species> ANIMALCONSTS{{
  {1600, 0, 1648, 1649, 8, 20}, // Polarbär
  {1700, 0, 1736, 1737, 6, 20}, // Hase hell
  {1740, 0, 1776, 1777, 6, 20}, // Hase dunkel
  {1800, 1840, 1836, 0, 6, 20}, // Fuchs
  {1850, 1900, 1898, 0, 8, 20}, // Hirsch
  {1910, 1960, 1958, 0, 8, 20}, // Reh
  {1970, 1976, 0, 0, 1, 50},    // Ente
  {2060, 0, 2072, 2073, 2, 40}  // Schaf
}};

const unsigned ANIMAL_MAX_ANIMATION_STEPS = 8;

#endif // AnimalConsts_h__
