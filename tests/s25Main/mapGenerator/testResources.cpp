// Copyright (c) 2017 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "RttrForeachPt.h"
#include "mapGenFixtures.h"
#include "mapGenerator/Resources.h"
#include "mapGenerator/TextureHelper.h"
#include <boost/test/unit_test.hpp>

using namespace rttr::mapGenerator;
using namespace libsiedler2;

struct ResourceTestFixture : MapGenFixture
{
    MapExtent size = MapExtent(16, 16);
    Map map;

    ResourceTestFixture() : map(createMap(size))
    {
        auto mountain = map.textureMap.Find(IsMinableMountain);
        auto water = map.textureMap.Find(IsWater);
        auto land = map.textureMap.Find(IsBuildableCoast);

        /*
         * ========== Test Map ===========
         * W = water / L = land / M = mountain
         *
         * W W L L L L M M M M L L L L W W
         * W W L L L L M M M M L L L L W W
         * W W L L L L M M M M L L L L W W
         * W W L L L L M M M M L L L L W W
         *              ....
         * W W L L L L M M M M L L L L W W
         * W W L L L L M M M M L L L L W W
         * W W L L L L M M M M L L L L W W
         * W W L L L L M M M M L L L L W W
         * W W L L L L M M M M L L L L W W
         * W W L L L L M M M M L L L L W W
         */
        auto& textureTriangles = map.getTextures();
        for(unsigned y = 0; y < map.size.y; y++)
        {
            for(unsigned x = 0; x < 2; x++)
                textureTriangles[MapPoint(x, y)] = TexturePair(water);

            for(unsigned x = 2; x < 6; x++)
                textureTriangles[MapPoint(x, y)] = TexturePair(land);

            for(unsigned x = 6; x < 10; x++)
                textureTriangles[MapPoint(x, y)] = TexturePair(mountain);

            for(unsigned x = 10; x < 14; x++)
                textureTriangles[MapPoint(x, y)] = TexturePair(land);

            for(unsigned x = 14; x < 16; x++)
                textureTriangles[MapPoint(x, y)] = TexturePair(water);
        }
    }
};

BOOST_FIXTURE_TEST_SUITE(ResourcesTests, ResourceTestFixture)

BOOST_AUTO_TEST_CASE(AddObjects_keeps_area_around_hqs_empty)
{
    RandomUtility rnd(0);
    MapSettings settings;
    MapPoint hq(3, 3);
    map.hqPositions.push_back(hq);

    AddObjects(map, rnd, settings);

    auto& objectTypes = map.objectTypes;
    auto& objectInfos = map.objectInfos;
    auto forbiddenArea = objectTypes.GetPointsInRadius(hq, 5);
    for(const MapPoint& pt : forbiddenArea)
    {
        BOOST_TEST_REQUIRE(objectTypes[pt] == OT_Empty);
        BOOST_TEST_REQUIRE(objectInfos[pt] == OI_Empty);
    }
}

BOOST_AUTO_TEST_CASE(AddObjects_keeps_area_around_harbors_empty)
{
    RandomUtility rnd(0);
    MapSettings settings;
    MapPoint harbor(3, 3);
    map.harbors.push_back(Triangle(true, harbor));

    AddObjects(map, rnd, settings);

    auto& objectTypes = map.objectTypes;
    auto& objectInfos = map.objectInfos;
    auto forbiddenArea = objectTypes.GetPointsInRadius(harbor, 5);
    for(const MapPoint& pt : forbiddenArea)
    {
        BOOST_TEST_REQUIRE(objectTypes[pt] == OT_Empty);
        BOOST_TEST_REQUIRE(objectInfos[pt] == OI_Empty);
    }
}

BOOST_AUTO_TEST_CASE(AddObjects_adds_objects_to_the_map)
{
    RandomUtility rnd(0);
    MapSettings settings;

    auto countObjects = [this]() {
        return helpers::count_if(map.objectTypes, [](const auto o) { return o != OT_Empty; });
    };

    const unsigned objectsBefore = countObjects();

    AddObjects(map, rnd, settings);

    const unsigned objectsAfter = countObjects();

    BOOST_TEST(objectsAfter > objectsBefore);
}

BOOST_AUTO_TEST_CASE(AddResources_updates_resources_according_to_textures)
{
    RandomUtility rnd(0);
    MapSettings settings;

    AddResources(map, rnd, settings);

    unsigned numMountainWithRes = 0;
    unsigned numMountainWithoutRes = 0;
    RTTR_FOREACH_PT(MapPoint, map.size)
    {
        if(map.textureMap.All(pt, IsMinableMountain))
        {
            if(map.resources[pt] == R_None)
                numMountainWithoutRes++; // LCOV_EXCL_LINE
            else
                numMountainWithRes++;
        } else if(map.textureMap.All(pt, IsWater))
            BOOST_TEST_REQUIRE(map.resources[pt] == R_Fish);
        else
            BOOST_TEST_REQUIRE(map.resources[pt] == R_Water);
    }
    BOOST_TEST(numMountainWithRes > 0u);
    // TODO: How much do we actually want?
    BOOST_TEST(numMountainWithRes > 4u * numMountainWithoutRes);
}

BOOST_AUTO_TEST_CASE(AddAnimals_updates_animals_according_to_textures)
{
    RandomUtility rnd(0);

    AddAnimals(map, rnd);

    RTTR_FOREACH_PT(MapPoint, map.size)
    {
        if(map.textureMap.All(pt, IsWater))
        {
            BOOST_TEST_REQUIRE(
              (map.animals[pt] == Animal::None || map.animals[pt] == Animal::Duck || map.animals[pt] == Animal::Duck2));
        } else
            BOOST_TEST_REQUIRE((map.animals[pt] != Animal::Duck && map.animals[pt] != Animal::Duck2));
    }
}

BOOST_AUTO_TEST_SUITE_END()
