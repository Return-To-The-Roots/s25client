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

#pragma once

#include "Direction.h"
#include "helpers/EnumArray.h"
#include "gameTypes/MapTypes.h"
#include <cstdint>
#include <stdexcept>

class FOWObject;
class SerializedGameData;

enum class BorderStonePos
{
    OnPoint,
    HalfEast,
    HalfSouthEast,
    HalfSouthWest
};

constexpr auto maxEnumValue(BorderStonePos)
{
    return BorderStonePos::HalfSouthWest;
}

inline Direction toDirection(BorderStonePos dir)
{
    switch(dir)
    {
        case BorderStonePos::HalfEast: return Direction::East;
        case BorderStonePos::HalfSouthEast: return Direction::SouthEast;
        case BorderStonePos::HalfSouthWest: return Direction::SouthWest;
        case BorderStonePos::OnPoint: break;
    }
    throw std::logic_error("Can't convert");
}

/// Border stones on 1 node: Directly on Point and halfway to E, SE and SW
using BoundaryStones = helpers::EnumArray<uint8_t, BorderStonePos>;

/// How a player sees the point in FoW
struct FoWNode
{
    /// Zeit (GF-Zeitpunkt), zu der, der Punkt zuletzt aktualisiert wurde
    unsigned last_update_time;
    /// Sichtbarkeit des Punktes
    Visibility visibility;
    /// FOW-Objekt
    FOWObject* object;
    helpers::EnumArray<PointRoad, RoadDir> roads;
    unsigned char owner;
    BoundaryStones boundary_stones;

    FoWNode();
    void Serialize(SerializedGameData& sgd) const;
    void Deserialize(SerializedGameData& sgd);
};
