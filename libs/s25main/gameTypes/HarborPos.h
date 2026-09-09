// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "ShipDirection.h"
#include "helpers/EnumArray.h"
#include "gameTypes/MapCoordinates.h"
#include "gameTypes/MapTypes.h"
#include <vector>

struct HarborPos
{
    struct Neighbor
    {
        HarborId id;
        SeaId sea;
        unsigned distance;

        Neighbor(HarborId id, SeaId sea, unsigned distance) noexcept : id(id), sea(sea), distance(distance) {}

        bool operator<(const Neighbor& two) const noexcept
        {
            if(distance == two.distance)
            {
                if(sea != two.sea)
                    return sea.value() < two.sea.value();
            }
            return id.value() < two.id.value();
        }
    };

    MapPoint pos;
    /// Seas at the neighbor points in each direction
    helpers::EnumArray<SeaId, Direction> seaIds = {};
    helpers::EnumArray<std::vector<Neighbor>, ShipDirection> neighbors;

    explicit HarborPos(const MapPoint pt) noexcept : pos(pt) {}
};
