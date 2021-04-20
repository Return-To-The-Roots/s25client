// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

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
