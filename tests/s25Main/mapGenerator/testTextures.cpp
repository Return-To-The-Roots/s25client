// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "mapGenFixtures.h"
#include "mapGenerator/TextureHelper.h"
#include "mapGenerator/Textures.h"
#include "gameTypes/GameTypesOutput.h"
#include <boost/test/unit_test.hpp>

using namespace rttr::mapGenerator;

class TextureMapFixture : private MapGenFixture
{
public:
    TextureMap textureMap;
    NodeMapBase<TexturePair>& textures;

    TextureMapFixture() : textureMap(worldDesc, landscape, MapExtent(0, 0)), textures(textureMap.textures_)
    {
        const MapExtent size(6, 8);
        textures.resize(size);
    }
};

class TextureMapFixtureWithZ : public TextureMapFixture
{
public:
    NodeMapBase<uint8_t> z;

    TextureMapFixtureWithZ() { z.resize(textures.getSize()); }
};

BOOST_AUTO_TEST_SUITE(TextureTests)

BOOST_FIXTURE_TEST_CASE(AddTextures_sets_valid_textures_for_entire_map, TextureMapFixtureWithZ)
{
    const unsigned mountainLevel = 5;
    const unsigned coastline = 1;

    RTTR_FOREACH_PT(MapPoint, z.getSize())
    {
        z[pt] = pt.x % 10;
    }

    Texturizer texturizer(z, textures, textureMap);
    texturizer.addTextures(mountainLevel, coastline);

    RTTR_FOREACH_PT(MapPoint, textures.getSize())
    {
        BOOST_TEST(textures[pt].rsu);
        BOOST_TEST(textures[pt].lsd);
    }
}

BOOST_FIXTURE_TEST_CASE(AddTextures_does_not_override_textures, TextureMapFixtureWithZ)
{
    const unsigned mountainLevel = 5;
    const unsigned coastline = 1;
    auto water = textureMap.Find(IsWater);
    textures.resize(textures.getSize(), TexturePair(water));

    RTTR_FOREACH_PT(MapPoint, z.getSize())
    {
        z[pt] = static_cast<uint8_t>(pt.x % 256);
    }

    Texturizer texturizer(z, textures, textureMap);
    texturizer.addTextures(mountainLevel, coastline);

    RTTR_FOREACH_PT(MapPoint, textures.getSize())
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
    texturizer.addTextures(mountainLevel, coastline);

    RTTR_FOREACH_PT(MapPoint, textures.getSize())
    {
        BOOST_TEST_REQUIRE(textureMap.check(Triangle(true, pt), IsWater));
        BOOST_TEST_REQUIRE(textureMap.check(Triangle(false, pt), IsWater));
    }
}

BOOST_FIXTURE_TEST_CASE(AddTextures_sets_mountain_textures_above_mountain_level, TextureMapFixtureWithZ)
{
    const unsigned mountainLevel = 10;
    const unsigned coastline = 2;
    z.resize(z.getSize(), mountainLevel);
    z[0] = 1; // sea

    Texturizer texturizer(z, textures, textureMap);
    texturizer.addTextures(mountainLevel, coastline);

    RTTR_FOREACH_PT(MapPoint, textures.getSize())
    {
        if(z[pt] >= mountainLevel)
        {
            BOOST_TEST_REQUIRE(textureMap.check(Triangle(true, pt), isMountainOrSnowOrLava));
            BOOST_TEST_REQUIRE(textureMap.check(Triangle(false, pt), isMountainOrSnowOrLava));
        }
    }
}

BOOST_FIXTURE_TEST_CASE(ReplaceTextureForPoint_replaces_all_textures, TextureMapFixture)
{
    auto source = textureMap.Find(IsWater);
    auto target = textureMap.Find(isSnowOrLava);
    auto point = MapPoint(textures.getWidth() / 2, textures.getHeight() / 2);
    textures.resize(textures.getSize(), source);

    replaceTextureForPoint(textures, point, target, {});

    BOOST_TEST(textureMap.all(point, isSnowOrLava));
}

BOOST_FIXTURE_TEST_CASE(ReplaceTextureForPoint_does_not_replace_excluded_textures, TextureMapFixture)
{
    auto source = textureMap.Find(IsWater);
    auto target = textureMap.Find(isSnowOrLava);
    auto point = MapPoint(textures.getWidth() / 2, textures.getHeight() / 2);
    textures.resize(textures.getSize(), source);

    replaceTextureForPoint(textures, point, target, {source});

    BOOST_TEST(textureMap.all(point, IsWater));
}

BOOST_FIXTURE_TEST_CASE(ReplaceTextures_replaces_textures_within_radius, TextureMapFixture)
{
    std::set<MapPoint, MapPointLess> points{
      MapPoint(0, 1),
      MapPoint(1, 0),
    };

    const auto source = textureMap.Find(IsWater);
    const auto target = textureMap.Find(isSnowOrLava);
    for(unsigned radius = 0; radius < 4; radius++)
    {
        textures.resize(textures.getSize(), source);
        std::set<MapPoint, MapPointLess> nodes(points);

        replaceTextures(textures, radius, nodes, target, {});

        for(const MapPoint& pt : points)
        {
            if(radius > 0)
            {
                for(const MapPoint& p : textures.getPointsInRadius(pt, radius))
                {
                    BOOST_TEST_REQUIRE(textureMap.all(p, isSnowOrLava));
                }
            } else
            {
                BOOST_TEST_REQUIRE(textureMap.all(pt, isSnowOrLava));
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
    const auto target = textureMap.Find(isSnowOrLava);

    for(unsigned radius = 0; radius < 4; radius++)
    {
        textures.resize(textures.getSize(), source);
        std::set<MapPoint, MapPointLess> nodes(points);

        replaceTextures(textures, radius, nodes, target, {source});

        RTTR_FOREACH_PT(MapPoint, textures.getSize())
        {
            BOOST_TEST_REQUIRE(textureMap.all(pt, IsWater));
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
