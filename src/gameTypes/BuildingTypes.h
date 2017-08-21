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

#ifndef BuildingTypes_h__
#define BuildingTypes_h__

#include "BuildingQuality.h"
#include "GoodTypes.h"
#include "JobTypes.h"
#include <boost/array.hpp>
#include <stdint.h>

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

struct WaresNeeded : boost::array<GoodType, 3>
{
    WaresNeeded(GoodType good1 = GD_NOTHING, GoodType good2 = GD_NOTHING, GoodType good3 = GD_NOTHING)
    {
        elems[0] = good1;
        elems[1] = good2;
        elems[2] = good3;
    }
    /// Return number of non-empty entries (assumes GD_NOTHING implies all others are GD_NOTHING too)
    unsigned getNum() const
    {
        for(unsigned i = 0; i < size(); i++)
        {
            if(elems[i] == GD_NOTHING)
                return i;
        }
        return size();
    }
};

/// Describes the work the building does
struct BldWorkDescription
{
    BldWorkDescription(Job job = JOB_NOTHING, GoodType producedWare = GD_NOTHING, WaresNeeded waresNeeded = WaresNeeded(),
                       uint8_t numSpacesPerWare = 6, bool useOneWareEach = true)
        : job(job), producedWare(producedWare), waresNeeded(waresNeeded), numSpacesPerWare(numSpacesPerWare), useOneWareEach(useOneWareEach)
    {
    }
    /// Worker belonging to the building
    Job job;
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

/// Rauch-Konstanten zu den "normalen Gebäuden" (Betrieben), beginnt erst mit Granitmine
struct SmokeConst
{
    /// Art des Rauches (von 1-4), 0 = kein Rauch!
    uint8_t type;
    /// Position des Rauches relativ zum Nullpunkt des Gebäudes
    int8_t x, y;
};

#endif // BuildingTypes_h__
