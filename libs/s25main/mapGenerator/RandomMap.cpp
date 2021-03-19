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
#include "mapGenerator/Terrain.h"
#include "mapGenerator/TextureHelper.h"
#include "mapGenerator/Textures.h"

#include "lua/GameDataLoader.h"
#include "libsiedler2/libsiedler2.h"

#include <stdexcept>

namespace rttr { namespace mapGenerator {

    unsigned GetMaximumHeight(const MapExtent& size)
    {
        const unsigned combinedSize = size.x * size.y;
        if(combinedSize <= 64 * 64)
            return 32;
        else if(combinedSize <= 128 * 128)
            return 64;
        else if(combinedSize <= 256 * 256)
            return 128;
        else if(combinedSize <= 512 * 512)
            return 150;
        else if(combinedSize <= 1024 * 1024)
            return 200;
        else
            return 60;
    }

    unsigned GetCoastline(const MapExtent& size)
    {
        const unsigned combinedSize = size.x * size.y;
        if(combinedSize <= 128 * 128)
            return 1;
        else if(combinedSize <= 512 * 512)
            return 2;
        else if(combinedSize <= 1024 * 1024)
            return 3;
        else
            return 4;
    }

    unsigned GetIslandRadius(const MapExtent& size)
    {
        const unsigned combinedSize = size.x * size.y;
        if(combinedSize <= 128 * 128)
            return 2;
        else if(combinedSize <= 256 * 256)
            return 3;
        else if(combinedSize <= 512 * 512)
            return 4;
        else if(combinedSize <= 1024 * 1024)
            return 5;
        else
            return 6;
    }

    unsigned GetIslandSize(const MapExtent& size)
    {
        const unsigned combinedSize = size.x * size.y;
        if(combinedSize <= 64 * 64)
            return 200;
        else if(combinedSize <= 128 * 128)
            return 400;
        else if(combinedSize <= 256 * 256)
            return 600;
        else if(combinedSize <= 512 * 512)
            return 900;
        else
            return 1200;
    }

    unsigned GetSmoothRadius(const MapExtent& size)
    {
        const unsigned combinedSize = size.x * size.y;
        if(combinedSize <= 128 * 128)
            return 2;
        else if(combinedSize <= 256 * 256)
            return 3;
        else if(combinedSize <= 512 * 512)
            return 4;
        else if(combinedSize <= 1024 * 1024)
            return 6;
        else
            return 7;
    }

    unsigned GetSmoothIterations(const MapExtent& size)
    {
        const unsigned combinedSize = size.x * size.y;
        if(combinedSize <= 64 * 64)
            return 10;
        else if(combinedSize <= 128 * 128)
            return 11;
        else if(combinedSize <= 256 * 256)
            return 9;
        else if(combinedSize <= 512 * 512)
            return 12;
        else if(combinedSize <= 1024 * 1024)
            return 15;
        else
            return 13;
    }

    void SmoothHeightMap(NodeMapBase<uint8_t>& z, const ValueRange<uint8_t>& range)
    {
        int radius = GetSmoothRadius(z.GetSize());
        int iterations = GetSmoothIterations(z.GetSize());

        Smooth(iterations, radius, z);
        Scale(z, range.minimum, range.maximum);
    }

    RandomMap::RandomMap(RandomUtility& rnd, Map& map)
        : rnd_(rnd), map_(map), texturizer_(map.z, map.textures, map.textureMap)
    {}

    void RandomMap::Create(const MapSettings& settings)
    {
        auto defaultHeight = map_.height.minimum + map_.height.GetDifference() / 2;

        settings_ = settings;
        map_.z.Resize(settings.size, defaultHeight);

        switch(settings.style)
        {
            case MapStyle::Water: CreateWaterMap(); break;

            case MapStyle::Mixed: CreateMixedMap(); break;

            case MapStyle::Land: CreateLandMap(); break;
        }

        AddObjects(map_, rnd_, settings_);
        AddResources(map_, rnd_, settings_);
        AddAnimals(map_, rnd_);
    }

    std::vector<River> RandomMap::CreateRivers(const MapPoint source)
    {
        std::vector<River> rivers;

        const MapExtent size = settings_.size;
        const unsigned length = size.x + size.y;

        for(const auto dir : helpers::EnumRange<Direction>())
        {
            if(rnd_.ByChance(settings_.rivers))
            {
                const unsigned splitRate = rnd_.RandomValue(0u, 2u);
                rivers.push_back(
                  CreateStream(rnd_, map_, source.isValid() ? source : rnd_.Point(map_.size), dir, length, splitRate));
            }
        }
        return rivers;
    }

    void RandomMap::CreateFreeIslands(unsigned waterNodes)
    {
        const auto islandRadius = GetIslandRadius(map_.size);
        const auto islandAmount = static_cast<double>(settings_.islands) / 100;
        const auto maxIslandSize = GetIslandSize(map_.size);
        const auto minIslandSize = std::min(200u, maxIslandSize);
        auto islandNodes = static_cast<unsigned>(islandAmount * waterNodes);
        auto islandSize = rnd_.RandomValue(minIslandSize, maxIslandSize);
        while(islandNodes >= islandSize)
        {
            islandNodes -= islandSize;
            CreateIsland(map_, rnd_, islandSize, islandRadius, .2);
            islandSize = rnd_.RandomValue(minIslandSize, maxIslandSize);
        }
    }

    void RandomMap::CreateMixedMap()
    {
        const auto center = rnd_.Point(map_.size);
        const unsigned maxDistance = map_.z.CalcMaxDistance();

        Restructure(map_, [this, &center, maxDistance](const MapPoint& pt) {
            auto weight = 1. - static_cast<float>(map_.z.CalcDistance(pt, center)) / maxDistance;
            auto percentage = static_cast<unsigned>(12 * weight);
            return rnd_.ByChance(percentage);
        });
        SmoothHeightMap(map_.z, map_.height);

        const double sea = 0.5;
        const double mountain = 0.1;
        const double land = 1. - sea - mountain;

        ResetSeaLevel(map_, rnd_, LimitFor(map_.z, sea, map_.height.minimum));

        const auto mountainLevel = LimitFor(map_.z, land, static_cast<uint8_t>(map_.height.minimum + 1)) + 1;
        const auto rivers = CreateRivers(center);
        const unsigned waterNodes = helpers::count(map_.z, map_.height.minimum);

        CreateFreeIslands(waterNodes);

        texturizer_.AddTextures(mountainLevel, GetCoastline(map_.size));

        PlaceHarbors(map_, rivers);
        PlaceHeadquarters(map_, rnd_, map_.players, settings_.mountainDistance);
    }

    void RandomMap::CreateWaterMap()
    {
        const auto center = rnd_.Point(map_.size);
        const unsigned maxDistance = map_.z.CalcMaxDistance();

        Restructure(map_, [this, &center, maxDistance](const MapPoint& pt) {
            const auto weight = 1. - static_cast<float>(map_.z.CalcDistance(pt, center)) / maxDistance;
            const auto percentage = static_cast<unsigned>(15 * weight * weight);
            return rnd_.ByChance(percentage);
        });
        SmoothHeightMap(map_.z, map_.height);

        const double sea = 0.80;      // 20% of map is center island (100% - 80% water)
        const double mountain = 0.05; // 20% of center island is mountain (5% of 20% land)
        const auto seaLevel = LimitFor(map_.z, sea, map_.height.minimum);

        ResetSeaLevel(map_, rnd_, seaLevel);

        const unsigned waterNodes = helpers::count(map_.z, map_.height.minimum);
        const auto land = 1. - static_cast<double>(waterNodes) / (map_.size.x * map_.size.y) - mountain;
        const auto mountainLevel = LimitFor(map_.z, land, static_cast<uint8_t>(1)) + 1;
        const auto islandSize = GetIslandSize(map_.size);
        const auto islandRadius = GetIslandRadius(map_.size);

        std::vector<Island> islands(map_.players);

        for(unsigned i = 0; i < map_.players; i++)
        {
            islands[i] = CreateIsland(map_, rnd_, islandSize, islandRadius, .2);
        }

        if(waterNodes > map_.players * islandSize)
        {
            CreateFreeIslands(waterNodes - map_.players * islandSize);
        }

        const auto rivers = CreateRivers(center);

        texturizer_.AddTextures(mountainLevel, GetCoastline(map_.size));

        PlaceHarbors(map_, rivers);

        for(unsigned i = 0; i < map_.players; i++)
        {
            PlaceHeadquarter(map_, islands[i], settings_.mountainDistance);
        }
    }

    void RandomMap::CreateLandMap()
    {
        Restructure(map_, [this](auto&&) { return rnd_.ByChance(5); });
        SmoothHeightMap(map_.z, map_.height);

        const double sea = rnd_.RandomDouble(0.1, 0.2);
        const double mountain = rnd_.RandomDouble(0.15, 0.4 - sea);
        const double land = 1. - sea - mountain;

        ResetSeaLevel(map_, rnd_, LimitFor(map_.z, sea, map_.height.minimum));

        const auto mountainLevel = LimitFor(map_.z, land, static_cast<uint8_t>(1)) + 1;
        CreateRivers();

        texturizer_.AddTextures(mountainLevel, GetCoastline(map_.size));

        PlaceHeadquarters(map_, rnd_, map_.players, settings_.mountainDistance);
    }

    Map GenerateRandomMap(RandomUtility& rnd, const WorldDescription& worldDesc, const MapSettings& settings)
    {
        auto height = GetMaximumHeight(settings.size);
        Map map(settings.size, settings.numPlayers, worldDesc, settings.type, height);
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
