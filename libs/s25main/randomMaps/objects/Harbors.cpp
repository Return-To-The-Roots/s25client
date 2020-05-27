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

#include "rttrDefines.h" // IWYU pragma: keep
#include "randomMaps/objects/Harbors.h"
#include "randomMaps/objects/HarborGenerator.h"
#include "randomMaps/waters/IslandCollector.h"
#include "randomMaps/waters/WaterMap.h"
#include "randomMaps/terrain/TextureType.h"
#include "randomMaps/algorithm/Filter.h"

Harbors::Harbors(int minimumIslandSize,
                 int minimumCoastSize,
                 int maximumIslandHarbors)
    : minimumIslandSize_(minimumIslandSize),
      minimumCoastSize_(minimumCoastSize),
      maximumIslandHarbors_(maximumIslandHarbors)
{
        
}

bool Harbors::IsSuitableHarborPosition(const CoastTile& pos,
                                       const Coastline& coastline,
                                       const std::vector<CoastTile>& harbors,
                                       int harborPositions,
                                       const MapExtent& size)
{
    if (harbors.empty())
    {
        return true;
    }
    
    std::vector<double> distanceToHarbor;
    for (auto harbor: harbors)
    {
        distanceToHarbor.push_back(GridUtility::Distance(harbor.coast, pos.coast, size));
    }
    
    auto distanceToNextHarbor = std::min_element(distanceToHarbor.begin(), distanceToHarbor.end());
    auto minimumDistance = (double)coastline.size() / (2 * harborPositions);
    
    return *distanceToNextHarbor > minimumDistance;
}

void Harbors::Place(Map& map)
{
    auto size = map.size;
    auto waterMap = WaterMap::For(map);

    CoastlineCalculator coastlineCalculator;
    HarborGenerator harbor;
    IslandCollector islandCollector;
    Islands islands = islandCollector.Collect(waterMap, size, minimumIslandSize_);
    
    // iterate over all islands ...
    for (auto island = islands.begin(); island != islands.end(); ++island)
    {
        auto coastline = coastlineCalculator.For(*island, waterMap, size);
        
        const int coastSize = coastline.size();
        const int harborPositions = std::max(1,
                                             std::min(maximumIslandHarbors_,
                                                      coastSize / minimumIslandSize_));
        
        std::vector<CoastTile> harbors;
        
        for (int i = 0; coastSize >= minimumCoastSize_ && i < harborPositions; i++)
        {
            for (int ci = 0; ci < coastSize; ci++)
            {
                if (IsSuitableHarborPosition(coastline[ci], coastline, harbors, harborPositions, map.size))
                {
                    harbors.push_back(coastline[ci]);
                    harbor.Build(map, coastline[ci]);
                    break;
                }
            }
        }
    }
}
