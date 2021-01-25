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
#include "rttr/test/random.hpp"
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

    Map map(size, 1, worldDesc, landscape);

    test(map, map.textureMap);
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

BOOST_AUTO_TEST_CASE(FindLargestConnectedArea_returns_expected_nodes)
{
    MapExtent size(32, 32);
    RunTest(size, [&size](Map& map, TextureMap& textures) {
        const auto water = textures.Find(IsShipableWater);
        const auto land = textures.Find(IsBuildableLand);
        map.textures.Resize(size, TexturePair(water));
        MapPoint centerOfLargeArea(3, 3);
        const auto largeArea = map.textures.GetPointsInRadiusWithCenter(centerOfLargeArea, 3);

        for(auto node : largeArea)
        {
            textures.Set(node, land);
        }

        MapPoint centerOfSmallArea(15, 15);
        const auto smallArea = map.textures.GetPointsInRadiusWithCenter(centerOfSmallArea, 2);

        for(auto node : smallArea)
        {
            textures.Set(node, land);
        }

        const auto result = FindLargestConnectedArea(map);

        BOOST_REQUIRE(result.size() == largeArea.size());

        for(auto node : largeArea)
        {
            BOOST_REQUIRE(helpers::contains(result, node));
        }
    });
}

BOOST_AUTO_TEST_CASE(FindHqPositions_returns_empty_for_map_without_suitable_position)
{
    MapExtent size(32, 32);
    RunTestForArea(size, [&size](Map& map, TextureMap& textures, const auto& area) {
        map.textures.Resize(size, TexturePair(textures.Find(IsShipableWater)));
        const auto mntDist = rttr::test::randomEnum<MountainDistance>();
        const auto positions = FindHqPositions(map, area, mntDist);

        BOOST_REQUIRE(positions.empty());
    });
}

BOOST_AUTO_TEST_CASE(FindHqPositions_returns_suitable_position_for_single_player)
{
    MapExtent size(8, 8);
    RunTestForArea(size, [&size](Map& map, TextureMap& textures, const auto& area) {
        const auto water = textures.Find(IsWater);
        const auto buildable = textures.Find(IsBuildableLand);

        MapPoint obstacle(0, 0);

        map.textures.Resize(size, TexturePair(buildable));
        map.textures[obstacle] = TexturePair(water);

        const auto mntDist = rttr::test::randomEnum<MountainDistance>();
        const auto positions = FindHqPositions(map, area, mntDist);

        BOOST_REQUIRE(!positions.empty());
    });
}

BOOST_AUTO_TEST_CASE(PlaceHeadquarter_with_suitable_position_for_player)
{
    MapExtent size(8, 8);
    RunTest(size, [&size](Map& map, TextureMap& textures) {
        const auto mountain = textures.Find(IsMinableMountain);
        const auto buildable = textures.Find(IsBuildableLand);

        MapPoint obstacle(0, 0);
        MapPoint hq(4, 4);

        map.textures.Resize(size, TexturePair(buildable));
        map.textures[obstacle] = TexturePair(mountain);

        std::vector<MapPoint> area{hq};
        BOOST_REQUIRE_NO_THROW(PlaceHeadquarter(map, 0, area, MountainDistance::Normal));
    });
}

BOOST_AUTO_TEST_CASE(PlaceHeadquarter_with_suitable_position_without_mountain)
{
    MapExtent size(8, 8);
    RunTest(size, [&size](Map& map, TextureMap& textures) {
        const auto mntDist = rttr::test::randomEnum<MountainDistance>();
        const auto buildable = textures.Find(IsBuildableLand);
        MapPoint hq(4, 4);
        map.textures.Resize(size, TexturePair(buildable));
        std::vector<MapPoint> area{hq};
        BOOST_REQUIRE_NO_THROW(PlaceHeadquarter(map, 0, area, mntDist));
    });
}

BOOST_AUTO_TEST_CASE(PlaceHeadquarter_places_hq_on_map_at_suitable_position)
{
    MapExtent size(8, 8);
    RunTest(size, [&size](Map& map, TextureMap& textures) {
        const auto mntDist = rttr::test::randomEnum<MountainDistance>();
        const MapPoint obstacle(0, 0);
        const MapPoint hq(4, 4);

        map.textures.Resize(size, TexturePair(textures.Find(IsBuildableLand)));
        map.textures[obstacle] = TexturePair(textures.Find(IsWater));

        std::vector<MapPoint> area{hq};

        PlaceHeadquarter(map, 3, area, mntDist);

        BOOST_REQUIRE(map.hqPositions[3] == hq);
    });
}

BOOST_AUTO_TEST_CASE(PlaceHeadquarter_with_empty_area_throws_exception)
{
    MapExtent size(8, 8);
    RunTest(size, [&size](Map& map, TextureMap& textures) {
        const auto mntDist = rttr::test::randomEnum<MountainDistance>();
        const auto water = textures.Find(IsShipableWater);
        map.textures.Resize(size, TexturePair(water));
        std::vector<MapPoint> area;
        BOOST_CHECK_THROW(PlaceHeadquarter(map, 0, area, mntDist), std::runtime_error);
    });
}

BOOST_AUTO_TEST_CASE(PlaceHeadquarter_without_suitable_position_throws_exception)
{
    MapExtent size(8, 8);
    RunTest(size, [&size](Map& map, TextureMap& textures) {
        const auto mntDist = rttr::test::randomEnum<MountainDistance>();
        const auto water = textures.Find(IsShipableWater);
        map.textures.Resize(size, TexturePair(water));
        std::vector<MapPoint> area{MapPoint(0, 0)};
        BOOST_CHECK_THROW(PlaceHeadquarter(map, 0, area, mntDist), std::runtime_error);
    });
}

BOOST_AUTO_TEST_CASE(PlaceHeadquarters_with_suitable_positions_for_all_players)
{
    MapExtent size(32, 32);
    RandomUtility rnd(0);
    const int players = rttr::test::randomValue(1, 8);
    RunTest(size, [&size, &rnd, players](Map& map, TextureMap& textures) {
        const auto mntDist = rttr::test::randomEnum<MountainDistance>();
        map.textures.Resize(size, TexturePair(textures.Find(IsBuildableLand)));
        map.textures[MapPoint(0, 0)] = TexturePair(textures.Find(IsWater));
        BOOST_REQUIRE_NO_THROW(PlaceHeadquarters(map, rnd, players, mntDist));
    });
}

BOOST_AUTO_TEST_CASE(PlaceHeadquarters_places_hqs_for_any_player_number_on_suitable_map)
{
    MapExtent size(32, 32);
    RandomUtility rnd(0);
    const int players = rttr::test::randomValue(1, 8);
    RunTest(size, [&size, &rnd, players](Map& map, TextureMap& textures) {
        const auto mntDist = rttr::test::randomEnum<MountainDistance>();
        map.textures.Resize(size, textures.Find(IsBuildableLand));
        map.textures[MapPoint(0, 0)] = TexturePair(textures.Find(IsWater));
        PlaceHeadquarters(map, rnd, players, mntDist);
        for(int index = 0; index < players - 1; index++)
        {
            BOOST_REQUIRE(map.hqPositions[index].isValid());
        }
    });
}

BOOST_AUTO_TEST_CASE(PlaceHeadquarters_without_suitable_position_throws_exception)
{
    MapExtent size(32, 32);
    RandomUtility rnd(0);
    const int players = rttr::test::randomValue(1, 8);
    RunTest(size, [&size, &rnd, players](Map& map, TextureMap& textures) {
        const auto mntDist = rttr::test::randomEnum<MountainDistance>();
        map.textures.Resize(size, textures.Find(IsWater));
        BOOST_CHECK_THROW(PlaceHeadquarters(map, rnd, players, mntDist), std::runtime_error);
    });
}

BOOST_AUTO_TEST_SUITE_END()
