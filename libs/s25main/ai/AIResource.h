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

#include "s25util/warningSuppression.h"
#include <array>

enum class AIResource : unsigned
{
    Wood,
    Stones,
    Gold,
    Ironore,
    Coal,
    Granite,
    Plantspace,
    Borderland,
    Fish,
    Multiple,
    // special:
    Blocked = 254,
    Nothing = 255
};

const unsigned NUM_AIRESOURCES = 9;
const std::array<unsigned, NUM_AIRESOURCES> SUPPRESS_UNUSED RES_RADIUS = {{
  8, // Wood
  8, // Stones
  2, // Gold
  2, // Ironore
  2, // Coal
  2, // Granite
  3, // Plantspace
  5, // Borderland
  5  // Fish
}};
