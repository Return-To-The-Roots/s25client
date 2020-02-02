// Copyright (c) 2017 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#include "mapGenerator/DistanceByProperty.h"
#include "mapGenerator/Textures.h"
#include "helpers/containerUtils.h"

namespace rttr {
namespace mapGenerator {

// OLD WORLD

bool IsHeadQuarter(const Map_& map, int index)
{
    return helpers::contains(map.hqPositions, (MapPoint)GridPosition(index, map.size));
}

bool IsCoastLand(const Map_& map, const TextureMapping_& mapping, int index)
{
    const auto neighbors = GridNeighbors(GridPosition(index, map.size), map.size);
    return helpers::contains_if(neighbors, [&map, &mapping](auto n) {
        return IsWater(map, mapping, n.x + n.y * map.size.x);
    });
}

bool IsWater (const Map_& map, const TextureMapping_& mapping, int index)
{
    return
        mapping.IsWater(map.textureRsu[index]) ||
        mapping.IsWater(map.textureLsd[index]);
}

}}
