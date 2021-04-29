// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
                    connectedArea = area;
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
            throw std::runtime_error("Could not find any valid HQ position!");
        for(const MapPoint hqPos : map.hqPositions)
            FlattenForCastleBuilding(map.z, hqPos);
    }

}} // namespace rttr::mapGenerator
