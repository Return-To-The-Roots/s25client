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

#include "mapGenerator/RandomMap.h"
#include "mapGenerator/Harbors.h"
#include "mapGenerator/HeadQuarters.h"
#include "mapGenerator/Islands.h"
#include "mapGenerator/Rivers.h"
#include "mapGenerator/Terrain.h"
#include "mapGenerator/TextureHelper.h"
#include "mapGenerator/Textures.h"
#include "mapGenerator/Utilities.h"

#include "lua/GameDataLoader.h"
#include "libsiedler2/libsiedler2.h"

#include <iostream>
#include <stdexcept>

namespace rttr { namespace mapGenerator {

    uint8_t ComputeGradient(const ValueMap<uint8_t>& z, const MapPoint& pt)
    {
        uint8_t gradient = 0;

        const auto& neighbors = z.GetNeighbours(pt);
        for(const MapPoint& neighbor : neighbors)
        {
            gradient = std::max(static_cast<unsigned>(std::abs(z[pt] - z[neighbor])), static_cast<unsigned>(gradient));
        }

        return gradient;
    }

    ValueMap<uint8_t> ComputeGradients(const ValueMap<uint8_t>& z)
    {
        ValueMap<uint8_t> gradient(z.GetSize(), 0);

        RTTR_FOREACH_PT(MapPoint, z.GetSize())
        {
            gradient[pt] = ComputeGradient(z, pt);
        }

        return gradient;
    }

    void PrintStatisticsForHeightMap(const ValueMap<uint8_t>& z)
    {
        const auto& range = z.GetRange();
        std::vector<unsigned> values(range.maximum + 1, 0);

        RTTR_FOREACH_PT(MapPoint, z.GetSize())
        {
            values[z[pt]]++;
        }

        double mean = 0.;
        for(unsigned z = range.minimum; z <= range.maximum; z++)
        {
            mean += values[z];
        }

        mean /= range.GetDifference();

        double stdDev = 0.;
        for(unsigned z = range.minimum; z <= range.maximum; z++)
        {
            stdDev += (values[z] - mean) * (values[z] - mean);
        }
        stdDev /= range.GetDifference();
        stdDev = std::sqrt(stdDev);

        auto gradient = ComputeGradients(z);
        auto maximumGradient = static_cast<unsigned>(gradient.GetMaximum());

        std::cout << "Distribution of Z-Values" << std::endl;
        std::cout << "> mean: " << mean << std::endl;
        std::cout << "> standard deviation: " << stdDev << std::endl;
        std::cout << "> maximum gradient: " << maximumGradient << std::endl;

        if(maximumGradient > 5)
        {
            std::cout << "WARNING: invalid height map (maximum gradient > 5)" << std::endl;
        }
    }

    unsigned GetCoastline(const MapExtent& size)
    {
        switch(size.x + size.y)
        {
            case 128:
            case 256: return 1;

            case 512: return 2;

            case 1024: return 3;

            case 2048: return 4;

            default: return 5;
        }
    }

    unsigned GetIslandRadius(const MapExtent& size)
    {
        switch(size.x + size.y)
        {
            case 128:
            case 256: return 2;

            case 512: return 3;

            case 1024: return 4;

            case 2048: return 5;

            default: return 6;
        }
    }

    void SmoothHeightMap(ValueMap<uint8_t>& z, const ValueRange<uint8_t>& range)
    {
        for(int i = 0; i < 10; i++)
        {
            Smooth(1, 2, z);
            Scale(z, range.minimum, range.maximum);
        }

        while(ComputeGradients(z).GetMaximum() > 4)
        {
            Smooth(1, 2, z);
        }

        PrintStatisticsForHeightMap(z);
    }

    void CreateContinental(RandomUtility& rnd, Map& map, Texturizer& texturizer)
    {
        MapPoint origin(0, 0);
        MapPoint center(map.size.x / 2, map.size.y / 2);

        unsigned maxDistance = map.z.CalcDistance(origin, center);

        std::vector<MapPoint> focus;

        RTTR_FOREACH_PT(MapPoint, map.size)
        {
            auto weight = 1. - static_cast<float>(map.z.CalcDistance(pt, center)) / maxDistance;
            auto percentage = static_cast<unsigned>(12 * weight);

            if(rnd.ByChance(percentage))
            {
                focus.push_back(pt);
            }
        }

        Restructure(map, focus);
        SmoothHeightMap(map.z, map.height);

        const double sea = 0.5;
        const double mountain = 0.1;
        const double land = 1. - sea - mountain;

        ResetSeaLevel(map, rnd, LimitFor(map.z, sea, map.height.minimum));

        auto mountainLevel = LimitFor(map.z, land, static_cast<uint8_t>(map.height.minimum + 1)) + 1;

        std::vector<River> rivers;

        if(rnd.ByChance(25))
        {
            const unsigned length = (map.size.x + map.size.y) / 3;
            const unsigned splitRate = rnd.Rand(0, 2);

            rivers.push_back(CreateStream(rnd, map, center, Direction(rnd.Rand(0, 5)), length, splitRate));
        }

        texturizer.AddTextures(mountainLevel, GetCoastline(map.size));

        PlaceHarbors(map, 20, 25, rivers);
        PlaceHeadQuarters(map, rnd, map.players);
    }

    void CreateIslands(RandomUtility& rnd, Map& map, Texturizer& texturizer)
    {
        MapPoint origin(0, 0);
        MapPoint center(map.size.x / 2, map.size.y / 2);

        unsigned maxDistance = map.z.CalcDistance(origin, center);

        std::vector<MapPoint> focus;

        RTTR_FOREACH_PT(MapPoint, map.size)
        {
            auto weight = 1. - static_cast<float>(map.z.CalcDistance(pt, center)) / maxDistance;
            auto percentage = static_cast<unsigned>(15 * weight * weight);

            if(rnd.ByChance(percentage))
            {
                focus.push_back(pt);
            }
        }

        Restructure(map, focus);
        SmoothHeightMap(map.z, map.height);

        // 20% of map is center island (100% - 80% water)
        const double sea = 0.80;

        // 20% of center island is mountain (4% of 20% land)
        const double mountain = 0.04;

        ResetSeaLevel(map, rnd, LimitFor(map.z, sea, map.height.minimum));

        const double land = 1. - sea - mountain;
        const auto mountainLevel = LimitFor(map.z, land, static_cast<uint8_t>(1)) + 1;

        const auto waterNodes = Count(map.z, map.height.minimum, map.height.minimum);

        // 40% of map reserved for player island (80% * 50%)
        const auto islandNodes = static_cast<unsigned>(.5 * waterNodes);

        // 5% of map per player island (40% / 8 = 5%)
        const unsigned nodesPerIsland = islandNodes / 8;

        const auto islandRadius = GetIslandRadius(map.size);
        const auto distanceToLand = islandRadius;

        std::vector<Island> islands(map.players);

        for(unsigned i = 0; i < map.players; i++)
        {
            islands[i] = CreateIsland(map, rnd, distanceToLand, nodesPerIsland, islandRadius, .2);
        }

        texturizer.AddTextures(mountainLevel, GetCoastline(map.size));

        PlaceHarbors(map, 20, 25, {});

        for(unsigned i = 0; i < map.players; i++)
        {
            PlaceHeadQuarter(map, i, islands[i]);
        }
    }

    void CreateValley(RandomUtility& rnd, Map& map, Texturizer& texturizer)
    {
        MapPoint origin(0, 0);
        MapPoint center(map.size.x / 2, map.size.y / 2);

        unsigned maxDistance = map.z.CalcDistance(origin, center);

        std::vector<MapPoint> focus;

        RTTR_FOREACH_PT(MapPoint, map.size)
        {
            auto weight = static_cast<float>(map.z.CalcDistance(pt, center)) / maxDistance;
            auto percentage = static_cast<unsigned>(15 * weight * weight);

            if(rnd.ByChance(percentage))
            {
                focus.push_back(pt);
            }
        }

        Restructure(map, focus);
        SmoothHeightMap(map.z, map.height);

        const double sea = 0.15;
        const double mountain = 0.55;
        const double land = 1. - sea - mountain;

        ResetSeaLevel(map, rnd, LimitFor(map.z, sea, map.height.minimum));

        auto mountainLevel = LimitFor(map.z, land, static_cast<uint8_t>(1)) + 1;

        std::vector<River> rivers;

        const unsigned riverLength = (map.size.x + map.size.y) / 3;

        for(unsigned i = 0; i < 6; ++i)
        {
            if(rnd.ByChance(50))
            {
                rivers.push_back(CreateStream(rnd, map, origin, Direction(i), riverLength));
            }
        }

        std::vector<Direction> horizontal{Direction::WEST, Direction::EAST};

        const MapPoint centerLeft(0, map.size.y / 2);

        for(auto dir : horizontal)
        {
            if(rnd.ByChance(50))
            {
                rivers.push_back(CreateStream(rnd, map, centerLeft, dir, riverLength));
            }
        }

        const auto totalWaterNodes = Count(map.z, map.height.minimum, map.height.minimum);
        const auto numberOfIslands = static_cast<unsigned>(rnd.Rand(0, 2));
        const auto minimumIslandSize = 50u;

        const auto islandRadius = GetIslandRadius(map.size);
        const auto distanceToLand = islandRadius * 2;

        auto islandNodes = static_cast<unsigned>(0.4 * totalWaterNodes);

        for(unsigned i = 0; i < numberOfIslands && islandNodes > minimumIslandSize; i++)
        {
            const unsigned islandSize = rnd.Rand(minimumIslandSize, islandNodes);
            const auto& island = CreateIsland(map, rnd, distanceToLand, islandSize, islandRadius, .2);
            islandNodes -= island.size();
        }

        texturizer.AddTextures(mountainLevel, GetCoastline(map.size));

        PlaceHarbors(map, 20, 25, rivers);
        PlaceHeadQuarters(map, rnd, map.players);
    }

    void CreateRandomMap(const std::string& filePath, const MapSettings& settings)
    {
        std::cout << "===== NEW RANDOM MAP =====" << std::endl;

        RandomUtility rnd;
        WorldDescription worldDesc;

        loadGameData(worldDesc);

        // determine maximum height of the map based on map size
        // - large maps should have enough variation in height
        auto maxHeightForMapSize = [](const MapExtent& size) {
            switch(size.x + size.y)
            {
                case 128: return 44;
                case 256: return 48;
                case 512: return 66;
                case 1024: return 80;
                case 2048: return 120;

                default: throw std::invalid_argument("unsupported map size");
            }
        };

        TextureMap textures(worldDesc, settings.type);

        Map map(textures, settings.size, settings.numPlayers, maxHeightForMapSize(settings.size));

        Texturizer texturizer(map.z, map.textures);

        auto defaultHeight = map.height.minimum + map.height.GetDifference() / 2;

        map.z.Resize(settings.size, defaultHeight);

        if(settings.style == MapStyle::Islands)
        {
            CreateIslands(rnd, map, texturizer);
        } else if(settings.style == MapStyle::Valley)
        {
            CreateValley(rnd, map, texturizer);
        } else if(settings.style == MapStyle::Continental)
        {
            CreateContinental(rnd, map, texturizer);
        } else
        {
            RTTR_FOREACH_PT(MapPoint, map.size)
            {
                map.z[pt] = static_cast<uint8_t>(rnd.Rand(map.height.minimum, map.height.maximum));
            }

            for(int i = 0; i < 10; i++)
            {
                Restructure(map, {rnd.RandomPoint(map.size), rnd.RandomPoint(map.size)}, 1.0 / std::pow(1.5, i));
            }

            SmoothHeightMap(map.z, map.height);

            const double sea = 0.2;
            const double mountain = 0.3;
            const double land = 1. - sea - mountain;

            ResetSeaLevel(map, rnd, LimitFor(map.z, sea, map.height.minimum));

            auto mountainLevel = LimitFor(map.z, land, static_cast<uint8_t>(map.height.minimum + 1));

            const auto waterNodes = Count(map.z, map.height.minimum, map.height.minimum);
            const auto numberOfIslands = static_cast<unsigned>(rnd.Rand(0, 5));
            const auto islandNodes = static_cast<unsigned>(rnd.DRand(.1, .5) * waterNodes);
            const auto minimumIslandSize = 10u;

            const auto islandRadius = GetIslandRadius(map.size);
            const auto distanceToLand = islandRadius * 3;

            if(numberOfIslands > 0 && islandNodes > numberOfIslands * minimumIslandSize)
            {
                const unsigned nodesPerIsland = islandNodes / numberOfIslands;

                for(unsigned i = 0; i < numberOfIslands; i++)
                {
                    CreateIsland(map, rnd, distanceToLand, nodesPerIsland, islandRadius, .2);
                }
            }

            std::vector<River> rivers;

            if(rnd.ByChance(50))
            {
                auto length = (map.size.x + map.size.y) / 2;
                auto direction = Direction(rnd.Rand(0, 5));

                rivers.push_back(CreateStream(rnd, map, rnd.RandomPoint(map.size), direction, length, 1));
            }

            texturizer.AddTextures(mountainLevel, GetCoastline(map.size));

            PlaceHarbors(map, 20, 25, rivers);
            PlaceHeadQuarters(map, rnd, settings.numPlayers);
        }

        libsiedler2::Write(filePath, map.CreateArchiv());
    }

}} // namespace rttr::mapGenerator
