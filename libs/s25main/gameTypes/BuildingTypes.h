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
#include "helpers/OptionalEnum.h"
#include <cassert>

struct BuildingCost
{
    uint8_t boards;
    uint8_t stones;
};

class WaresNeeded
{
    GoodType elems_[3];
    unsigned size_;

public:
    constexpr WaresNeeded() : elems_{}, size_(0) {}
    constexpr explicit WaresNeeded(GoodType good1) : elems_{good1}, size_(1) {}
    constexpr WaresNeeded(GoodType good1, GoodType good2) : elems_{good1, good2}, size_(2) {}
    constexpr WaresNeeded(GoodType good1, GoodType good2, GoodType good3) : elems_{good1, good2, good3}, size_(3) {}
    constexpr unsigned size() const { return size_; }
    constexpr bool empty() const { return size_ == 0u; }
    constexpr GoodType operator[](unsigned i) const
    {
        assert(i < size_);
        return elems_[i];
    }
    constexpr const GoodType* begin() const { return elems_; }
    constexpr const GoodType* end() const { return elems_ + size_; }
};

/// Describes the work the building does
struct BldWorkDescription
{
    /// Worker belonging to the building, if any
    helpers::OptionalEnum<Job> job = boost::none;
    /// Ware produced, if any
    helpers::OptionalEnum<GoodType> producedWare = boost::none;
    /// Wares the building needs, if any
    WaresNeeded waresNeeded = {};
    /// How many wares of each type can be stored
    uint8_t numSpacesPerWare = 6;
    /// True if one of each waresNeeded is used per production cycle
    /// False if the ware type is used, that the building has the most of
    bool useOneWareEach = true;
};

/// Smoke definition for buildings
struct SmokeConst
{
    constexpr SmokeConst() = default;
    constexpr SmokeConst(uint8_t type, const Point<int8_t>& offset) : type(type), offset(offset) {}
    /// Smoke type (1-4), 0 = no smoke
    uint8_t type = 0;
    /// Position of the smoke relative to the buildings origin
    Point<int8_t> offset = {0, 0};
};
