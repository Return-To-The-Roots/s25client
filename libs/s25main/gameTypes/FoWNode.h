// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Direction.h"
#include "FoWObject.h"
#include "helpers/EnumArray.h"
#include "gameTypes/MapTypes.h"
#include <cstdint>
#include <memory>
#include <stdexcept>

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
    std::unique_ptr<FOWObject> object;
    helpers::EnumArray<PointRoad, RoadDir> roads;
    unsigned char owner;
    BoundaryStones boundary_stones;

    FoWNode();
    void Serialize(SerializedGameData& sgd) const;
    void Deserialize(SerializedGameData& sgd);
};
