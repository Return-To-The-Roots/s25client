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

#include "lua/GameDataLoader.h"
#include "mapGenerator/HeadQuarters.h"
#include "mapGenerator/TextureHelper.h"
#include <boost/test/unit_test.hpp>

using namespace rttr::mapGenerator;

BOOST_AUTO_TEST_SUITE(HeadQuartersTests)

template<class T_Test>
void RunTest(const MapExtent& size, T_Test test);

template<class T_Test>
void RunTest(const MapExtent& size, T_Test test)
{
    DescIdx<LandscapeDesc> landscape(1);
    WorldDescription worldDesc;
    loadGameData(worldDesc);

    TextureMap textures(worldDesc, landscape);

    Map map(textures, size, 1, 44);

    test(map, textures);
}

template<class T_Test>
void RunTestForArea(const MapExtent& size, T_Test test);

template<class T_Test>
void RunTestForArea(const MapExtent& size, T_Test test)
{
    RunTest(size, [&test](Map& map, TextureMap& textures) {
        std::vector<MapPoint> allPositions;

        RTTR_FOREACH_PT(MapPoint, map.size)
        {
            allPositions.push_back(pt);
        }

        test(map, textures, allPositions);
    });
}

BOOST_AUTO_TEST_CASE(FindLargestConnectedArea_ReturnsExpectedNodes)
{
    MapExtent size(32, 32);
    RunTest(size, [&size](Map& map, TextureMap& textures) {
        auto water = textures.Find(IsShipableWater);
        auto land = textures.Find(IsBuildableLand);

        textures.Resize(size, TexturePair(water));

        MapPoint centerOfLargeArea(3, 3);
        auto largeArea = map.textures.GetPointsInRadiusWithCenter(centerOfLargeArea, 3);

        for(auto node : largeArea)
        {
            textures.Set(node, land);
        }

        MapPoint centerOfSmallArea(15, 15);
        auto smallArea = map.textures.GetPointsInRadiusWithCenter(centerOfSmallArea, 2);

        for(auto node : smallArea)
        {
            textures.Set(node, land);
        }

        auto result = FindLargestConnectedArea(map);

        BOOST_REQUIRE(result.size() == largeArea.size());

        for(auto node : largeArea)
        {
            BOOST_REQUIRE(helpers::contains(result, node));
        }
    });
}

BOOST_AUTO_TEST_CASE(FindHqPositions_WithoutSuitablePosition_ReturnsEmpty)
{
    MapExtent size(32, 32);
    RunTestForArea(size, [&size](Map& map, TextureMap& textures, const auto& area) {
        textures.Resize(size, TexturePair(textures.Find(IsShipableWater)));

        auto positions = FindHqPositions(map, area);

        BOOST_REQUIRE(positions.empty());
    });
}

BOOST_AUTO_TEST_CASE(FindHqPositions_ForSinglePlayer_ReturnsSuitablePositions)
{
    MapExtent size(8, 8);
    RunTestForArea(size, [&size](Map& map, TextureMap& textures, const auto& area) {
        auto water = textures.Find(IsShipableWater);
        auto buildable = textures.Find(IsBuildableLand);

        MapPoint obstacle(0, 0);

        map.textures.Resize(size, TexturePair(buildable));
        map.textures[obstacle] = TexturePair(water);

        std::vector<MapPoint> positions = FindHqPositions(map, area);
        std::vector<MapPoint> expectedPositions{MapPoint(4, 2), MapPoint(3, 3), MapPoint(4, 3), MapPoint(3, 4),
                                                MapPoint(4, 4), MapPoint(5, 4), MapPoint(3, 5), MapPoint(4, 5),
                                                MapPoint(5, 5), MapPoint(4, 6), MapPoint(5, 6), MapPoint(4, 7)};

        BOOST_REQUIRE(positions.size() == expectedPositions.size());
        for(const MapPoint& expectedPosition : expectedPositions)
        {
            BOOST_REQUIRE(helpers::contains(positions, expectedPosition));
        }
    });
}

BOOST_AUTO_TEST_CASE(PlaceHeadQuarter_ForAreaWithSuitablePosition_ReturnsTrue)
{
    MapExtent size(8, 8);
    RunTest(size, [&size](Map& map, TextureMap& textures) {
        auto water = textures.Find(IsShipableWater);
        auto buildable = textures.Find(IsBuildableLand);

        MapPoint obstacle(0, 0);
        MapPoint hq(4, 4);

        map.textures.Resize(size, TexturePair(buildable));
        map.textures[obstacle] = TexturePair(water);

        std::vector<MapPoint> area{hq};

        auto success = PlaceHeadQuarter(map, 0, area);

        BOOST_REQUIRE(success);
    });
}

BOOST_AUTO_TEST_CASE(PlaceHeadQuarter_ForAreaWithSuitablePosition_PlacesHqOnMap)
{
    MapExtent size(8, 8);
    RunTest(size, [&size](Map& map, TextureMap& textures) {
        MapPoint obstacle(0, 0);
        MapPoint hq(4, 4);

        textures.Resize(size, TexturePair(textures.Find(IsBuildableLand)));
        textures[obstacle] = TexturePair(textures.Find(IsShipableWater));

        std::vector<MapPoint> area{hq};

        PlaceHeadQuarter(map, 3, area);

        BOOST_REQUIRE(map.objectInfos[hq] == libsiedler2::OI_HeadquarterMask);
        BOOST_REQUIRE(map.objectTypes[hq] == libsiedler2::ObjectType(3));
    });
}

BOOST_AUTO_TEST_CASE(PlaceHeadQuarters_ForNPlayersOnEmptyMap_ReturnsTrue)
{
    MapExtent size(32, 32);
    RandomUtility rnd(0);

    for(int players = 1; players < 8; players++)
    {
        RunTest(size, [&size, &rnd, players](Map& map, TextureMap& textures) {
            textures.Resize(size, TexturePair(textures.Find(IsBuildableLand)));

            auto success = PlaceHeadQuarters(map, rnd, players);

            BOOST_REQUIRE(success);
        });
    }
}

BOOST_AUTO_TEST_CASE(PlaceHeadQuarters_ForNPlayersOnEmptyMap_PlacesNHqs)
{
    MapExtent size(32, 32);
    RandomUtility rnd(0);

    for(int players = 1; players < 8; players++)
    {
        RunTest(size, [&size, &rnd, players](Map& map, TextureMap& textures) {
            textures.Resize(size, textures.Find(IsBuildableLand));

            PlaceHeadQuarters(map, rnd, players);

            int hqs = 0;

            RTTR_FOREACH_PT(MapPoint, size)
            {
                if(map.objectInfos[pt] == libsiedler2::OI_HeadquarterMask)
                {
                    hqs++;
                }
            }

            BOOST_REQUIRE(hqs == players);
        });
    }
}

BOOST_AUTO_TEST_SUITE_END()