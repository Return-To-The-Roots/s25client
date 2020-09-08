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
#include "mapGenerator/Islands.h"
#include "mapGenerator/TextureHelper.h"
#include <boost/test/unit_test.hpp>

using namespace rttr::mapGenerator;

BOOST_AUTO_TEST_SUITE(IslandsTests)

template<class T_Test>
void RunTest(T_Test test);

template<class T_Test>
void RunTest(T_Test test)
{
    MapExtent size(32, 32);
    DescIdx<LandscapeDesc> landscape(1);
    WorldDescription worldDesc;
    loadGameData(worldDesc);

    TextureMap textures(worldDesc, landscape);
    auto water = textures.Find(IsWater);
    textures.Resize(size, TexturePair(water));

    test(textures);
}

std::vector<MapPoint> CreateTestIsland(TextureMap& textures, const MapPoint& center, unsigned radius);

std::vector<MapPoint> CreateTestIsland(TextureMap& textures, const MapPoint& center, unsigned radius)
{
    auto land = textures.Find(IsLand);
    auto landNodes = textures.GetPointsInRadiusWithCenter(center, radius);
    for(const MapPoint& pt : landNodes)
    {
        textures.Set(pt, land);
    }
    return textures.GetPointsInRadiusWithCenter(center, radius + 1);
}

BOOST_AUTO_TEST_CASE(FindIslands_returns_expected_island)
{
    RunTest([](TextureMap& textures) {
        auto expectedIsland = CreateTestIsland(textures, MapPoint(0, 0), 1);
        auto islands = FindIslands(textures, 1);

        BOOST_REQUIRE(islands.size() == 1u);
        BOOST_REQUIRE(islands[0].size() == expectedIsland.size());

        for(const MapPoint& pt : expectedIsland)
        {
            BOOST_REQUIRE(helpers::contains(islands[0], pt));
        }
    });
}

BOOST_AUTO_TEST_CASE(FindIslands_ignores_island_below_minimum_size)
{
    RunTest([](TextureMap& textures) {
        auto expectedIsland = CreateTestIsland(textures, MapPoint(0, 0), 1);
        auto islands = FindIslands(textures, expectedIsland.size() + 1);

        BOOST_REQUIRE(islands.empty());
    });
}

BOOST_AUTO_TEST_CASE(FindIslands_returns_multiple_islands_correctly)
{
    RunTest([](TextureMap& textures) {
        auto expectedIsland1 = CreateTestIsland(textures, MapPoint(0, 0), 2);
        auto expectedIsland2 = CreateTestIsland(textures, MapPoint(12, 15), 2);
        auto ignoredIsland = CreateTestIsland(textures, MapPoint(20, 24), 1);

        auto islands = FindIslands(textures, ignoredIsland.size() + 1);

        BOOST_REQUIRE(islands.size() == 2u);

        BOOST_REQUIRE(islands[0].size() == expectedIsland1.size());
        for(const MapPoint& pt : expectedIsland1)
        {
            BOOST_REQUIRE(helpers::contains(islands[0], pt));
        }
        BOOST_REQUIRE(islands[1].size() == expectedIsland2.size());
        for(const MapPoint& pt : expectedIsland2)
        {
            BOOST_REQUIRE(helpers::contains(islands[1], pt));
        }
    });
}

BOOST_AUTO_TEST_SUITE_END()
