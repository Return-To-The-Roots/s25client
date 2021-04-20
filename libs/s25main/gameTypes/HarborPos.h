// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "ShipDirection.h"
#include "helpers/EnumArray.h"
#include "gameTypes/MapCoordinates.h"
#include <vector>

struct HarborPos
{
    struct Neighbor
    {
        unsigned id;
        unsigned distance;

        Neighbor(unsigned id, unsigned distance) noexcept : id(id), distance(distance) {}

        bool operator<(const Neighbor& two) const
        {
            return (distance < two.distance) || (distance == two.distance && id < two.id);
        }
    };

    MapPoint pos;
    /// Seas at the neighbor points in each direction
    helpers::EnumArray<uint16_t, Direction> seaIds = {};
    helpers::EnumArray<std::vector<Neighbor>, ShipDirection> neighbors;

    explicit HarborPos(const MapPoint pt) noexcept : pos(pt) {}
};
