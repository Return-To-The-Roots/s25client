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

#include "BuildingQuality.h"
#include "GoodTypes.h"
#include "JobTypes.h"
#include "Point.h"
#include <boost/optional/optional.hpp>
#include <array>

struct BuildingCost
{
    uint8_t boards;
    uint8_t stones;
};

// Größe der Gebäude
enum BuildingSize
{
    BZ_HUT = 0,
    BZ_HOUSE,
    BZ_CASTLE,
    BZ_MINE
};

struct WaresNeeded : std::array<GoodType, 3>
{
    WaresNeeded(GoodType good1 = GD_NOTHING, GoodType good2 = GD_NOTHING, GoodType good3 = GD_NOTHING)
    {
        (*this)[0] = good1;
        (*this)[1] = good2;
        (*this)[2] = good3;
    }
    /// Return number of non-empty entries (assumes GD_NOTHING implies all others are GD_NOTHING too)
    unsigned getNum() const
    {
        for(unsigned i = 0; i < size(); i++)
        {
            if((*this)[i] == GD_NOTHING)
                return i;
        }
        return size();
    }
};

/// Describes the work the building does
struct BldWorkDescription
{
    BldWorkDescription(boost::optional<Job> job = boost::none, GoodType producedWare = GD_NOTHING,
                       WaresNeeded waresNeeded = WaresNeeded(), uint8_t numSpacesPerWare = 6,
                       bool useOneWareEach = true)
        : job(std::move(job)), producedWare(producedWare), waresNeeded(waresNeeded), numSpacesPerWare(numSpacesPerWare),
          useOneWareEach(useOneWareEach)
    {}
    /// Worker belonging to the building
    boost::optional<Job> job;
    /// Ware produced (maybe nothing or invalid)
    GoodType producedWare;
    /// Wares the building needs (maybe nothing)
    WaresNeeded waresNeeded;
    /// How many wares of each type can be stored
    uint8_t numSpacesPerWare;
    /// True if one of each waresNeeded is used per production cycle
    /// False if the ware type is used, that the building has the most of
    bool useOneWareEach;
};

/// Smoke definition for buildings
struct SmokeConst
{
    SmokeConst() : type(0), offset(Point<int8_t>::Invalid()) {}
    SmokeConst(uint8_t type, const DrawPoint& offset) : type(type), offset(offset) {}
    SmokeConst(uint8_t type, int8_t x, int8_t y) : type(type), offset(x, y) {}
    /// Smoke type (1-4), 0 = no smoke
    uint8_t type;
    /// Position of the smoke relativ to the buildings origin
    Point<int8_t> offset;
};
