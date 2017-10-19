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

#ifndef BuildingType_h__
#define BuildingType_h__

enum BuildingType
{
    BLD_HEADQUARTERS = 0,    // ----
    BLD_BARRACKS = 1,        // NJR
    BLD_GUARDHOUSE = 2,      // NJR
    BLD_NOTHING2 = 3,        // ----
    BLD_WATCHTOWER = 4,      // NJ
    BLD_NOTHING3 = 5,        // ----
    BLD_NOTHING4 = 6,        // ----
    BLD_NOTHING5 = 7,        // ----
    BLD_NOTHING6 = 8,        // ----
    BLD_FORTRESS = 9,        // NJR
    BLD_GRANITEMINE = 10,    // NJR
    BLD_COALMINE = 11,       // NJR
    BLD_IRONMINE = 12,       // NJR
    BLD_GOLDMINE = 13,       // NJR
    BLD_LOOKOUTTOWER = 14,   //
    BLD_NOTHING7 = 15,       // ----
    BLD_CATAPULT = 16,       //
    BLD_WOODCUTTER = 17,     // NJR
    BLD_FISHERY = 18,        // N
    BLD_QUARRY = 19,         // NJR
    BLD_FORESTER = 20,       // NJR
    BLD_SLAUGHTERHOUSE = 21, // NJR
    BLD_HUNTER = 22,         // NJ
    BLD_BREWERY = 23,        // NJR
    BLD_ARMORY = 24,         // NJR
    BLD_METALWORKS = 25,     // NJR
    BLD_IRONSMELTER = 26,    // NJR
    BLD_CHARBURNER = 27,     // new
    BLD_PIGFARM = 28,        // NJR
    BLD_STOREHOUSE = 29,     //
    BLD_NOTHING9 = 30,       // ----
    BLD_MILL = 31,           // NJR
    BLD_BAKERY = 32,         // NJR
    BLD_SAWMILL = 33,        // NJR
    BLD_MINT = 34,           // NJR
    BLD_WELL = 35,           // NJR
    BLD_SHIPYARD = 36,       //
    BLD_FARM = 37,           // NJR
    BLD_DONKEYBREEDER = 38,  //
    BLD_HARBORBUILDING = 39, //
    BLD_NOTHING              // No building. Must be the last entry and never stored as it might change values
};

/// Number of building types
const unsigned BUILDING_TYPES_COUNT = BLD_NOTHING;
/// Number of NOTHING entries (currently unused buildings)
const unsigned NUM_UNUSED_BLD_TYPES = 7;
/// First usual building (building that produces something)
const unsigned FIRST_USUAL_BUILDING = BLD_GRANITEMINE;

#endif // BuildingType_h__
