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

#include "helpers/MaxEnumValue.h"
#include <cstdint>

/// Buildingqualities
enum BuildingQuality : uint8_t
{
    BQ_NOTHING = 0,
    BQ_FLAG,
    BQ_HUT,
    BQ_HOUSE,
    BQ_CASTLE,
    BQ_MINE,
    BQ_HARBOR
};
DEFINE_MAX_ENUM_VALUE(BuildingQuality, BuildingQuality::BQ_HARBOR)

/// Return true iff the BQ found matches a required BQ. E.g. A building with a given size can be constructed on a given node
inline bool canUseBq(BuildingQuality bqIs, BuildingQuality bqRequired)
{
    // Exact match -> OK
    if(bqIs == bqRequired)
        return true;
    // Not a special bq (mine/harbor) and we require less then we have -> OK
    return bqIs < BQ_MINE && bqRequired < bqIs;
}
