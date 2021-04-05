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

#include "mapGenerator/HeadQuarters.h"
#include "helpers/containerUtils.h"
#include "mapGenerator/Algorithms.h"
#include "mapGenerator/TextureHelper.h"

namespace rttr { namespace mapGenerator {

    std::vector<MapPoint> FindLargestConnectedArea(const Map& map)
    {
        std::set<MapPoint, MapPointLess> visited;
        std::vector<MapPoint> connectedArea;

        auto partiallyBuildable = [&map](const MapPoint& pt) { return map.textureMap.Any(pt, IsBuildableLand); };

        auto partiallyConnected = [&map, &partiallyBuildable](const MapPoint& pt) {
            return helpers::contains_if(map.getTextures().GetNeighbours(pt), partiallyBuildable);
        };

        RTTR_FOREACH_PT(MapPoint, map.size)
        {
            if(visited.insert(pt).second)
            {
                auto area = Collect(map.getTextures(), pt, partiallyConnected);

                visited.insert(area.begin(), area.end());

                if(area.size() > connectedArea.size())
                {
                    connectedArea = area;
                }
            }
        }

        // remove rivers & water around the coast which have been
        // added before due to allowing tiny rivers to be part of
        // connected areas

        helpers::erase_if(connectedArea, [&map](const MapPoint& pt) { return map.textureMap.Any(pt, IsWater); });

        return connectedArea;
    }

    void PlaceHeadquarters(Map& map, RandomUtility& rnd, int number, MountainDistance distance, int retries)
    {
        auto maxRetries = retries;
        auto success = false;

        auto area = FindLargestConnectedArea(map);

        while(!success && retries > 0)
        {
            success = true;

            for(int index = 0; index < number; index++)
            {
                auto possiblePositions = FindHqPositions(map, area, distance);

                if(possiblePositions.empty())
                {
                    map.hqPositions.clear();
                    success = false;
                    break;
                }

                auto hq = retries == maxRetries ? possiblePositions.front() : rnd.RandomItem(possiblePositions);

                map.hqPositions.push_back(hq);
            }

            retries--;
        }

        if(!success)
        {
            throw std::runtime_error("Could not find any valid HQ position!");
        }
    }

}} // namespace rttr::mapGenerator
