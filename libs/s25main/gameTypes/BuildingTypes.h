// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
    // Required for use in aggregate initialization
    // NOLINTBEGIN(readability-redundant-member-init)
    /// Wares the building needs, if any
    WaresNeeded waresNeeded = {};
    // NOLINTEND(readability-redundant-member-init)
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
