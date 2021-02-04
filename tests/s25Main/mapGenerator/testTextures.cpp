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

#include "mapGenFixtures.h"
#include "mapGenerator/TextureHelper.h"
#include "mapGenerator/Textures.h"
#include "gameTypes/GameTypesOutput.h"
#include <boost/test/unit_test.hpp>

using namespace rttr::mapGenerator;

class TextureMapFixture : private MapGenFixture
{
public:
    NodeMapBase<TexturePair> textures;
    TextureMap textureMap;

    TextureMapFixture() : textureMap(worldDesc, landscape, textures)
    {
        const MapExtent size(6, 8);
        textures.Resize(size);
    }
};

class TextureMapFixtureWithZ : public TextureMapFixture
{
public:
    NodeMapBase<uint8_t> z;

    TextureMapFixtureWithZ() { z.Resize(textures.GetSize()); }
};

BOOST_AUTO_TEST_SUITE(TextureTests)

BOOST_FIXTURE_TEST_CASE(AddTextures_sets_valid_textures_for_entire_map, TextureMapFixtureWithZ)
{
    const unsigned mountainLevel = 5;
    const unsigned coastline = 1;

    RTTR_FOREACH_PT(MapPoint, z.GetSize())
    {
        z[pt] = pt.x % 10;
    }

    Texturizer texturizer(z, textures, textureMap);
    texturizer.AddTextures(mountainLevel, coastline);

    RTTR_FOREACH_PT(MapPoint, textures.GetSize())
    {
        BOOST_TEST_REQUIRE((textures[pt].rsu.value != DescIdx<TerrainDesc>::INVALID));
        BOOST_TEST_REQUIRE((textures[pt].lsd.value != DescIdx<TerrainDesc>::INVALID));
    }
}

BOOST_FIXTURE_TEST_CASE(AddTextures_does_not_override_textures, TextureMapFixtureWithZ)
{
    const unsigned mountainLevel = 5;
    const unsigned coastline = 1;
    auto water = textureMap.Find(IsWater);
    textures.Resize(textures.GetSize(), TexturePair(water));

    RTTR_FOREACH_PT(MapPoint, z.GetSize())
    {
        z[pt] = static_cast<uint8_t>(pt.x % 256);
    }

    Texturizer texturizer(z, textures, textureMap);
    texturizer.AddTextures(mountainLevel, coastline);

    RTTR_FOREACH_PT(MapPoint, textures.GetSize())
    {
        BOOST_TEST_REQUIRE(textures[pt].rsu == water);
        BOOST_TEST_REQUIRE(textures[pt].lsd == water);
    }
}

BOOST_FIXTURE_TEST_CASE(AddTextures_sets_water_textures_for_minimum_height, TextureMapFixtureWithZ)
{
    const unsigned mountainLevel = 3;
    const unsigned coastline = 2;

    Texturizer texturizer(z, textures, textureMap);
    texturizer.AddTextures(mountainLevel, coastline);

    RTTR_FOREACH_PT(MapPoint, textures.GetSize())
    {
        BOOST_TEST_REQUIRE(textureMap.Check(Triangle(true, pt), IsWater));
        BOOST_TEST_REQUIRE(textureMap.Check(Triangle(false, pt), IsWater));
    }
}

BOOST_FIXTURE_TEST_CASE(AddTextures_sets_mountain_textures_above_mountain_level, TextureMapFixtureWithZ)
{
    const unsigned mountainLevel = 10;
    const unsigned coastline = 2;
    z.Resize(z.GetSize(), mountainLevel);
    z[0] = 1; // sea

    Texturizer texturizer(z, textures, textureMap);
    texturizer.AddTextures(mountainLevel, coastline);

    RTTR_FOREACH_PT(MapPoint, textures.GetSize())
    {
        if(z[pt] >= mountainLevel)
        {
            BOOST_TEST_REQUIRE(textureMap.Check(Triangle(true, pt), IsMountainOrSnowOrLava));
            BOOST_TEST_REQUIRE(textureMap.Check(Triangle(false, pt), IsMountainOrSnowOrLava));
        }
    }
}

BOOST_FIXTURE_TEST_CASE(ReplaceTextureForPoint_replaces_all_textures, TextureMapFixture)
{
    auto source = textureMap.Find(IsWater);
    auto target = textureMap.Find(IsSnowOrLava);
    auto point = MapPoint(textures.GetWidth() / 2, textures.GetHeight() / 2);
    textures.Resize(textures.GetSize(), source);

    ReplaceTextureForPoint(textures, point, target, {});

    BOOST_TEST(textureMap.All(point, IsSnowOrLava));
}

BOOST_FIXTURE_TEST_CASE(ReplaceTextureForPoint_does_not_replace_excluded_textures, TextureMapFixture)
{
    auto source = textureMap.Find(IsWater);
    auto target = textureMap.Find(IsSnowOrLava);
    auto point = MapPoint(textures.GetWidth() / 2, textures.GetHeight() / 2);
    textures.Resize(textures.GetSize(), source);

    ReplaceTextureForPoint(textures, point, target, {source});

    BOOST_TEST(textureMap.All(point, IsWater));
}

BOOST_FIXTURE_TEST_CASE(ReplaceTextures_replaces_textures_within_radius, TextureMapFixture)
{
    std::set<MapPoint, MapPointLess> points{
      MapPoint(0, 1),
      MapPoint(1, 0),
    };

    const auto source = textureMap.Find(IsWater);
    const auto target = textureMap.Find(IsSnowOrLava);
    for(unsigned radius = 0; radius < 4; radius++)
    {
        textures.Resize(textures.GetSize(), source);
        std::set<MapPoint, MapPointLess> nodes(points);

        ReplaceTextures(textures, radius, nodes, target, {});

        for(const MapPoint& pt : points)
        {
            if(radius > 0)
            {
                for(const MapPoint& p : textures.GetPointsInRadius(pt, radius))
                {
                    BOOST_TEST_REQUIRE(textureMap.All(p, IsSnowOrLava));
                }
            } else
            {
                BOOST_TEST_REQUIRE(textureMap.All(pt, IsSnowOrLava));
            }
        }
    }
}

BOOST_FIXTURE_TEST_CASE(ReplaceTextures_does_not_replace_excluded_textures, TextureMapFixture)
{
    std::set<MapPoint, MapPointLess> points{
      MapPoint(3, 1),
      MapPoint(3, 0),
    };

    const auto source = textureMap.Find(IsWater);
    const auto target = textureMap.Find(IsSnowOrLava);

    for(unsigned radius = 0; radius < 4; radius++)
    {
        textures.Resize(textures.GetSize(), source);
        std::set<MapPoint, MapPointLess> nodes(points);

        ReplaceTextures(textures, radius, nodes, target, {source});

        RTTR_FOREACH_PT(MapPoint, textures.GetSize())
        {
            BOOST_TEST_REQUIRE(textureMap.All(pt, IsWater));
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
