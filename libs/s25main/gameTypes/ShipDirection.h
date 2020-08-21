// Copyright (c) 2015 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#include <cstdint>

/// "Enum" to represent one of the 6 directions a ship can go
enum class ShipDirection : uint8_t
{
    North,     // 0
    NorthEast, // 1
    SouthEast, // 2
    South,     // 3
    SouthWest, // 4
    NorthWest  // 5
};

constexpr auto maxEnumValue(ShipDirection)
{
    return ShipDirection::NorthWest;
}
