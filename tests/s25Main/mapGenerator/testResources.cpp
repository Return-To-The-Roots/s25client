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

        for(unsigned y = 0; y < map.size.y; y++)
        {
            for(unsigned x = 0; x < 2; x++)
                map.textures[MapPoint(x, y)] = TexturePair(water);

            for(unsigned x = 2; x < 6; x++)
                map.textures[MapPoint(x, y)] = TexturePair(land);

            for(unsigned x = 6; x < 10; x++)
                map.textures[MapPoint(x, y)] = TexturePair(mountain);

            for(unsigned x = 10; x < 14; x++)
                map.textures[MapPoint(x, y)] = TexturePair(land);

            for(unsigned x = 14; x < 16; x++)
                map.textures[MapPoint(x, y)] = TexturePair(water);
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
        return helpers::count_if(map.objectInfos, [](const auto o) { return o != OI_Empty; });
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

    RTTR_FOREACH_PT(MapPoint, map.size)
    {
        if(map.textureMap.All(pt, IsMinableMountain))
        {
            BOOST_TEST_REQUIRE(map.resources[pt] != R_None);
        } else if(map.textureMap.All(pt, IsWater))
        {
            BOOST_TEST_REQUIRE(map.resources[pt] == R_Fish);
        } else
        {
            BOOST_TEST_REQUIRE(map.resources[pt] == R_Water);
        }
    }
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
