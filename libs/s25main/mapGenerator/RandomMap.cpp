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

#include "mapGenerator/RandomMap.h"
#include "mapGenerator/Harbors.h"
#include "mapGenerator/HeadQuarters.h"
#include "mapGenerator/Islands.h"
#include "mapGenerator/Resources.h"
#include "mapGenerator/Rivers.h"
#include "mapGenerator/Terrain.h"
#include "mapGenerator/TextureHelper.h"
#include "mapGenerator/Textures.h"

#include "lua/GameDataLoader.h"
#include "libsiedler2/libsiedler2.h"

#include <iostream>
#include <stdexcept>

namespace rttr { namespace mapGenerator {

    unsigned GetCoastline(const MapExtent& size)
    {
        switch(size.x + size.y)
        {
            case 128:
            case 256: return 1;

            case 512:
            case 1024: return 2;

            case 2048: return 3;

            default: return 4;
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

    unsigned GetSmoothRadius(const MapExtent& size)
    {
        switch(size.x + size.y)
        {
            case 128: return 2;

            case 256: return 3;

            case 512: return 4;

            case 1024: return 5;

            case 2048: return 6;

            default: return 7;
        }
    }

    unsigned GetSmoothIterations(const MapExtent& size)
    {
        switch(size.x + size.y)
        {
            case 128: return 10;

            case 256: return 12;

            case 512: return 15;

            case 1024: return 20;

            case 2048: return 25;

            default: return 50;
        }
    }

    void SmoothHeightMap(ValueMap<uint8_t>& z, const ValueRange<uint8_t>& range)
    {
        int radius = GetSmoothRadius(z.GetSize());
        int iterations = GetSmoothIterations(z.GetSize());

        Smooth(iterations, radius, z);
        Scale(z, range.minimum, range.maximum);
    }

    RandomMap::RandomMap(RandomUtility& rnd, Map& map) : rnd_(rnd), map_(map), texturizer_(map.z, map.textures) {}

    void RandomMap::Create(const MapSettings& settings)
    {
        auto defaultHeight = map_.height.minimum + map_.height.GetDifference() / 2;

        map_.z.Resize(settings.size, defaultHeight);

        if(settings.style == MapStyle::Water)
        {
            CreateWaterMap();
        } else if(settings.style == MapStyle::Mixed)
        {
            CreateMixedMap();
        } else
        {
            CreateLandMap();
        }

        AddObjects(map_, rnd_);
        AddResources(map_, rnd_, settings);
        AddAnimals(map_, rnd_);
    }

    void RandomMap::CreateMixedMap()
    {
        MapPoint origin(0, 0);
        MapPoint center(map_.size.x / 2, map_.size.y / 2);

        unsigned maxDistance = map_.z.CalcDistance(origin, center);

        std::vector<MapPoint> focus;
        RTTR_FOREACH_PT(MapPoint, map_.size)
        {
            auto weight = 1. - static_cast<float>(map_.z.CalcDistance(pt, center)) / maxDistance;
            auto percentage = static_cast<unsigned>(12 * weight);

            if(rnd_.ByChance(percentage))
            {
                focus.push_back(pt);
            }
        }

        Restructure(map_, focus);
        SmoothHeightMap(map_.z, map_.height);

        const double sea = 0.5;
        const double mountain = 0.1;
        const double land = 1. - sea - mountain;

        ResetSeaLevel(map_, rnd_, LimitFor(map_.z, WholeMap(), sea, map_.height.minimum));

        auto mountainLevel = LimitFor(map_.z, WholeMap(), land, static_cast<uint8_t>(map_.height.minimum + 1)) + 1;

        std::vector<River> rivers;

        if(rnd_.ByChance(25))
        {
            const unsigned length = (map_.size.x + map_.size.y) / 3;
            const unsigned splitRate = rnd_.RandomInt(0, 2);

            rivers.push_back(CreateStream(rnd_, map_, center, Direction(rnd_.RandomInt(0, 5)), length, splitRate));
        }

        texturizer_.AddTextures(mountainLevel, GetCoastline(map_.size));

        PlaceHarbors(map_, rivers, (map_.size.x + map_.size.y) / 2);
        PlaceHeadQuarters(map_, rnd_, map_.players);
    }

    void RandomMap::CreateWaterMap()
    {
        MapPoint origin(0, 0);
        MapPoint center(map_.size.x / 2, map_.size.y / 2);

        unsigned maxDistance = map_.z.CalcDistance(origin, center);

        std::vector<MapPoint> focus;

        RTTR_FOREACH_PT(MapPoint, map_.size)
        {
            auto weight = 1. - static_cast<float>(map_.z.CalcDistance(pt, center)) / maxDistance;
            auto percentage = static_cast<unsigned>(15 * weight * weight);

            if(rnd_.ByChance(percentage))
            {
                focus.push_back(pt);
            }
        }

        Restructure(map_, focus);
        SmoothHeightMap(map_.z, map_.height);

        // 20% of map is center island (100% - 80% water)
        const double sea = 0.80;

        // 20% of center island is mountain (4% of 20% land)
        const double mountain = 0.04;

        ResetSeaLevel(map_, rnd_, LimitFor(map_.z, WholeMap(), sea, map_.height.minimum));

        const double land = 1. - sea - mountain;
        const auto mountainLevel = LimitFor(map_.z, WholeMap(), land, static_cast<uint8_t>(1)) + 1;

        const auto waterNodes = Count(map_.z, WholeMap(), map_.height.minimum, map_.height.minimum);

        // 40% of map reserved for player island (80% * 50%)
        const auto islandNodes = static_cast<unsigned>(.5 * waterNodes);

        // 5% of map per player island (40% / 8 = 5%)
        const unsigned nodesPerIsland = islandNodes / 8;

        const auto islandRadius = GetIslandRadius(map_.size);
        const auto distanceToLand = islandRadius;

        std::vector<Island> islands(map_.players);

        for(unsigned i = 0; i < map_.players; i++)
        {
            islands[i] = CreateIsland(map_, rnd_, distanceToLand, nodesPerIsland, islandRadius, .2);
        }

        texturizer_.AddTextures(mountainLevel, GetCoastline(map_.size));

        PlaceHarbors(map_, {}, (map_.size.x + map_.size.y) / 4);

        for(unsigned i = 0; i < map_.players; i++)
        {
            PlaceHeadQuarter(map_, i, islands[i]);
        }
    }

    void RandomMap::CreateLandMap()
    {
        std::vector<MapPoint> focus;

        RTTR_FOREACH_PT(MapPoint, map_.size)
        {
            if(rnd_.ByChance(5))
            {
                focus.push_back(pt);
            }
        }

        Restructure(map_, focus);
        SmoothHeightMap(map_.z, map_.height);

        const double sea = rnd_.RandomDouble(0.1, 0.2);
        const double mountain = rnd_.RandomDouble(0.2, 0.7 - sea);
        const double land = 1. - sea - mountain;

        ResetSeaLevel(map_, rnd_, LimitFor(map_.z, WholeMap(), sea, map_.height.minimum));

        auto mountainLevel = LimitFor(map_.z, WholeMap(), land, static_cast<uint8_t>(1)) + 1;

        texturizer_.AddTextures(mountainLevel, GetCoastline(map_.size));

        PlaceHeadQuarters(map_, rnd_, map_.players);
    }

    Map GenerateRandomMap(RandomUtility& rnd, const WorldDescription& worldDesc, const MapSettings& settings)
    {
        Map map(settings.size, settings.numPlayers, worldDesc, settings.type);
        RandomMap randomMap(rnd, map);
        randomMap.Create(settings);
        return map;
    }

    void CreateRandomMap(const boost::filesystem::path& filePath, const MapSettings& settings)
    {
        RandomUtility rnd;
        WorldDescription worldDesc;
        loadGameData(worldDesc);

        Map map = GenerateRandomMap(rnd, worldDesc, settings);
        libsiedler2::Write(filePath, map.CreateArchiv());
    }

}} // namespace rttr::mapGenerator
