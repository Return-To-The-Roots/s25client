// Copyright (c) 2017 - 2020 Settlers Freaks (sf-team at siedler25.org)
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

#include "mapGenerator/AreaDesc.h"
#include "mapGenerator/VertexUtility.h"

AreaDesc::AreaDesc(const Point<double>& center, double minDist, double maxDist, double pHill, unsigned pTree,
                   unsigned pStone, unsigned minZ, unsigned maxZ, int minPlayerDist, int maxPlayerDist)
    : center(center), minDistance(minDist), maxDistance(maxDist), likelyhoodHill(pHill), likelyhoodTree(pTree),
      likelyhoodStone(pStone), minElevation(minZ), maxElevation(maxZ), minPlayerDistance(minPlayerDist),
      maxPlayerDistance(maxPlayerDist)
{}

bool AreaDesc::IsInArea(const Position& point, double playerDistance, const MapExtent& size)
{
    Position tile(size * center);
    double distance = VertexUtility::Distance(point, tile, size) / std::min(size.x / 2, size.y / 2);

    if(maxPlayerDistance <= 0)
    {
        return playerDistance >= minPlayerDistance && distance >= minDistance && distance < maxDistance;
    }

    return playerDistance >= minPlayerDistance && playerDistance < maxPlayerDistance && distance >= minDistance
           && distance < maxDistance;
}
