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

#include "rttrDefines.h"
#include "lua/GameDataLoader.h"
#include "mapGenerator/TextureHelper.h"
#include "mapGenerator/Textures.h"
#include <boost/test/unit_test.hpp>

using namespace rttr::mapGenerator;

BOOST_AUTO_TEST_SUITE(TextureTests)

template<class T_Test>
void RunTest(T_Test test);

template<class T_Test>
void RunTest(T_Test test)
{
    MapExtent size(8, 8);
    DescIdx<LandscapeDesc> landscape(1);
    WorldDescription worldDesc;
    loadGameData(worldDesc);

    TextureMap textures(worldDesc, landscape);
    textures.Resize(size);

    test(textures, size);
}

BOOST_AUTO_TEST_CASE(AddTextures_SetsValidTexturesForEntireMap)
{
    RunTest([](TextureMap& textures, const MapExtent& size) {
        const unsigned mountainLevel = 5;
        const unsigned coastline = 1;

        ValueMap<uint8_t> z(size);

        RTTR_FOREACH_PT(MapPoint, textures.GetSize())
        {
            z[pt] = pt.x % 10;
        }

        Texturizer texturizer(z, textures);

        texturizer.AddTextures(mountainLevel, coastline);

        RTTR_FOREACH_PT(MapPoint, textures.GetSize())
        {
            BOOST_REQUIRE(textures[pt].rsu.value != DescIdx<TerrainDesc>::INVALID);
            BOOST_REQUIRE(textures[pt].lsd.value != DescIdx<TerrainDesc>::INVALID);
        }
    });
}

BOOST_AUTO_TEST_CASE(AddTextures_DoesNotOverrideAlreadySetTextures)
{
    RunTest([](TextureMap& textures, const MapExtent& size) {
        const unsigned mountainLevel = 5;
        const unsigned coastline = 1;

        ValueMap<uint8_t> z(size);

        auto water = textures.Find(IsWater);

        textures.Resize(size, TexturePair(water));

        RTTR_FOREACH_PT(MapPoint, size)
        {
            z[pt] = pt.x;
        }

        Texturizer texturizer(z, textures);

        texturizer.AddTextures(mountainLevel, coastline);

        RTTR_FOREACH_PT(MapPoint, size)
        {
            BOOST_REQUIRE(textures[pt].rsu == water);
            BOOST_REQUIRE(textures[pt].lsd == water);
        }
    });
}

BOOST_AUTO_TEST_CASE(AddTextures_AddsWaterTextureForMinimumHeight)
{
    RunTest([](TextureMap& textures, const MapExtent& size) {
        const unsigned mountainLevel = 3;
        const unsigned coastline = 2;

        ValueMap<uint8_t> z(size, 0);

        Texturizer texturizer(z, textures);

        texturizer.AddTextures(mountainLevel, coastline);

        RTTR_FOREACH_PT(MapPoint, size)
        {
            BOOST_REQUIRE(textures.Check(Triangle(true, pt), IsWater));
            BOOST_REQUIRE(textures.Check(Triangle(false, pt), IsWater));
        }
    });
}

BOOST_AUTO_TEST_CASE(AddTextures_AddsMountainOrSnowOrLavalTextureAboveMountainLevel)
{
    RunTest([](TextureMap& textures, const MapExtent& size) {
        const unsigned mountainLevel = 10;
        const unsigned coastline = 2;

        ValueMap<uint8_t> z(size, mountainLevel);

        z[0] = 1; // sea

        Texturizer texturizer(z, textures);

        texturizer.AddTextures(mountainLevel, coastline);

        RTTR_FOREACH_PT(MapPoint, size)
        {
            if(z[pt] >= mountainLevel)
            {
                BOOST_REQUIRE(textures.Check(Triangle(true, pt), IsMountainOrSnowOrLava));
                BOOST_REQUIRE(textures.Check(Triangle(false, pt), IsMountainOrSnowOrLava));
            }
        }
    });
}

BOOST_AUTO_TEST_CASE(ReplaceTextureForPoint_ReplacesAllTexturesWithSpecifiedTexture)
{
    RunTest([](TextureMap& textures, const MapExtent& size) {
        auto source = textures.Find(IsWater);
        auto target = textures.Find(IsSnowOrLava);

        auto point = MapPoint(size.x / 2, size.y / 2);

        textures.Resize(size, source);

        ReplaceTextureForPoint(textures, point, target, {});

        BOOST_REQUIRE(textures.All(point, IsSnowOrLava));
    });
}

BOOST_AUTO_TEST_CASE(ReplaceTextureForPoint_DoesNotReplaceExcludedTextures)
{
    RunTest([](TextureMap& textures, const MapExtent& size) {
        auto source = textures.Find(IsWater);
        auto target = textures.Find(IsSnowOrLava);

        auto point = MapPoint(size.x / 2, size.y / 2);

        textures.Resize(size, source);

        ReplaceTextureForPoint(textures, point, target, {source});

        BOOST_REQUIRE(textures.All(point, IsWater));
    });
}

BOOST_AUTO_TEST_CASE(ReplaceTextures_ReplacesAllTexturesWithinRadius)
{
    std::set<MapPoint, MapPoint_compare> points{
      MapPoint(0, 1),
      MapPoint(1, 0),
    };

    for(unsigned radius = 0; radius < 4; radius++)
    {
        RunTest([radius, &points](TextureMap& textures, const MapExtent& size) {
            auto source = textures.Find(IsWater);
            auto target = textures.Find(IsSnowOrLava);

            textures.Resize(size, source);

            std::set<MapPoint, MapPoint_compare> nodes(points);

            ReplaceTextures(textures, radius, nodes, target, {});

            for(const MapPoint& pt : points)
            {
                if(radius > 0)
                {
                    for(const MapPoint& p : textures.GetPointsInRadius(pt, radius))
                    {
                        BOOST_REQUIRE(textures.All(p, IsSnowOrLava));
                    }
                } else
                {
                    BOOST_REQUIRE(textures.All(pt, IsSnowOrLava));
                }
            }
        });
    }
}

BOOST_AUTO_TEST_CASE(ReplaceTextures_DoesNotReplaceExcludedTextures)
{
    std::set<MapPoint, MapPoint_compare> points{
      MapPoint(3, 1),
      MapPoint(3, 0),
    };

    for(unsigned radius = 0; radius < 4; radius++)
    {
        RunTest([radius, &points](TextureMap& textures, const MapExtent& size) {
            auto source = textures.Find(IsWater);
            auto target = textures.Find(IsSnowOrLava);

            textures.Resize(size, source);

            std::set<MapPoint, MapPoint_compare> nodes(points);

            ReplaceTextures(textures, radius, nodes, target, {source});

            RTTR_FOREACH_PT(MapPoint, size)
            {
                BOOST_REQUIRE(textures.All(pt, IsWater));
            }
        });
    }
}

BOOST_AUTO_TEST_SUITE_END()
