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
#include "lua/GameDataLoader.h"
#include "mapGenerator/Resources.h"
#include "mapGenerator/TextureHelper.h"
#include <boost/test/unit_test.hpp>

using namespace rttr::mapGenerator;
using namespace libsiedler2;

BOOST_AUTO_TEST_SUITE(ResourcesTests)

template<class T_Test>
void RunTest(T_Test test);

template<class T_Test>
void RunTest(T_Test test)
{
    MapExtent size(16, 16);
    DescIdx<LandscapeDesc> landscape(1);
    WorldDescription worldDesc;
    loadGameData(worldDesc);

    TextureMap textures(worldDesc, landscape);
    Map map(textures, size, 1, 44);

    auto mountain = map.textures.Find(IsMountainOrSnowOrLava);
    auto water = map.textures.Find(IsWater);
    auto land = map.textures.Find(IsBuildableCoast);

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
        {
            map.textures[MapPoint(x, y)] = TexturePair(water);
        }

        for(unsigned x = 2; x < 6; x++)
        {
            map.textures[MapPoint(x, y)] = TexturePair(land);
        }

        for(unsigned x = 6; x < 10; x++)
        {
            map.textures[MapPoint(x, y)] = TexturePair(mountain);
        }

        for(unsigned x = 10; x < 14; x++)
        {
            map.textures[MapPoint(x, y)] = TexturePair(land);
        }

        for(unsigned x = 14; x < 16; x++)
        {
            map.textures[MapPoint(x, y)] = TexturePair(water);
        }
    }

    test(map);
}

BOOST_AUTO_TEST_CASE(AddObjects_KeepsAreaAroundHeadQuarterEmpty)
{
    RunTest([](Map& map) {
        RandomUtility rnd(0);
        MapPoint hq(3, 3);
        map.MarkAsHeadQuarter(hq, 0);

        AddObjects(map, rnd);

        auto& objectTypes = map.objectTypes;
        auto& objectInfos = map.objectInfos;
        auto forbiddenArea = objectTypes.GetPointsInRadius(hq, 5);
        for(const MapPoint& pt : forbiddenArea)
        {
            BOOST_REQUIRE(objectTypes[pt] == OT_Empty);
            BOOST_REQUIRE(objectInfos[pt] == OI_Empty);
        }
    });
}

BOOST_AUTO_TEST_CASE(AddObjects_KeepsAreaAroundHarborEmpty)
{
    RunTest([](Map& map) {
        RandomUtility rnd(0);
        MapPoint harbor(3, 3);
        map.harbors.push_back(Triangle(true, harbor));

        AddObjects(map, rnd);

        auto& objectTypes = map.objectTypes;
        auto& objectInfos = map.objectInfos;
        auto forbiddenArea = objectTypes.GetPointsInRadius(harbor, 5);
        for(const MapPoint& pt : forbiddenArea)
        {
            BOOST_REQUIRE(objectTypes[pt] == OT_Empty);
            BOOST_REQUIRE(objectInfos[pt] == OI_Empty);
        }
    });
}

BOOST_AUTO_TEST_CASE(AddObjects_AddsObjectsToTheMap)
{
    RunTest([](Map& map) {
        RandomUtility rnd(0);

        auto countObjects = [&map]() {
            unsigned objects = 0;
            RTTR_FOREACH_PT(MapPoint, map.size)
            {
                if(map.objectInfos[pt] != OI_Empty)
                {
                    objects++;
                }
            }
            return objects;
        };

        const unsigned objectsBefore = countObjects();

        AddObjects(map, rnd);

        const unsigned objectsAfter = countObjects();

        BOOST_REQUIRE(objectsAfter > objectsBefore);
    });
}

BOOST_AUTO_TEST_CASE(AddResources_UpdatesResourceMapAccordingToTexture)
{
    RunTest([](Map& map) {
        RandomUtility rnd(0);
        MapSettings settings;

        AddResources(map, rnd, settings);

        RTTR_FOREACH_PT(MapPoint, map.size)
        {
            if(map.textures.All(pt, IsMinableMountain))
            {
                BOOST_REQUIRE(map.resources[pt] != R_None);
            } else if(map.textures.All(pt, IsWater))
            {
                BOOST_REQUIRE(map.resources[pt] == R_Fish);
            } else
            {
                BOOST_REQUIRE(map.resources[pt] == R_Water);
            }
        }
    });
}

BOOST_AUTO_TEST_SUITE_END()
