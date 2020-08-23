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
#include "mapGenerator/Harbors.h"
#include "mapGenerator/Terrain.h"
#include "mapGenerator/TextureHelper.h"
#include <boost/test/unit_test.hpp>

using namespace rttr::mapGenerator;

BOOST_AUTO_TEST_SUITE(HarborsTests)

template<class T_Test>
void RunTest(T_Test test);

template<class T_Test>
void RunTest(T_Test test)
{
    DescIdx<LandscapeDesc> landscape(1);
    WorldDescription worldDesc;
    loadGameData(worldDesc);
    TextureMap textures(worldDesc, landscape);

    MapExtent size(8, 8);
    Map map(textures, size, 1, 44);

    test(map, textures);
}

BOOST_AUTO_TEST_CASE(PlaceHarborPosition_FlattensGroundInNeighborhood)
{
    RunTest([](Map& map, TextureMap& textures) {
        auto land = TexturePair(textures.Find(IsBuildableLand));

        map.textures.Resize(map.size, land);

        MapPoint position(3, 4);

        map.z.Resize(map.size, map.height.maximum);

        const unsigned minHeight = map.height.minimum;
        const auto neighbors = map.z.GetNeighbours(position);

        for(unsigned i = 0; i < neighbors.size(); i++)
        {
            map.z[neighbors[i]] = minHeight + i;
        }

        // run actual test
        PlaceHarborPosition(map, position);

        BOOST_REQUIRE(map.z[position] == minHeight);
        for(auto neighbor : neighbors)
        {
            BOOST_REQUIRE(map.z[neighbor] == minHeight);
        }
    });
}

BOOST_AUTO_TEST_CASE(PlaceHarborPosition_AppliesBuildableTerrainAroundPosition)
{
    RunTest([](Map& map, TextureMap& textures) {
        auto coast = TexturePair(textures.Find(IsCoastTerrain));
        auto buildable = textures.Find(IsBuildableCoast);

        map.textures.Resize(map.size, coast);

        MapPoint position(3, 4);

        PlaceHarborPosition(map, position);

        auto triangles = GetTriangles(position, map.size);
        for(auto triangle : triangles)
        {
            if(triangle.rsu)
            {
                BOOST_REQUIRE(map.textures[triangle.position].rsu == buildable);
            } else
            {
                BOOST_REQUIRE(map.textures[triangle.position].lsd == buildable);
            }
        }
    });
}

BOOST_AUTO_TEST_CASE(PlaceHarbors_PlacesNoHarborsOnIslandsBelowMinimumSize)
{
    RunTest([](Map& map, TextureMap& textures) {
        MapPoint island(3, 3);

        const int minimumIslandSize = 200;
        const int minimumCoastSize = 5;

        auto water = TexturePair(textures.Find(IsWater));

        map.textures.Resize(map.size, water);

        auto coast = textures.Find(IsCoastTerrain);
        textures.Set(island, coast);

        PlaceHarbors(map, minimumIslandSize, minimumCoastSize, {});

        BOOST_REQUIRE(map.harbors.empty());
    });
}

BOOST_AUTO_TEST_CASE(PlaceHarbors_PlacesNoHarborsOnIslandsBelowMinimumCoast)
{
    RunTest([](Map& map, TextureMap& textures) {
        MapPoint island(3, 3);

        const int minimumIslandSize = 1;
        const int minimumCoastSize = 200;

        auto water = TexturePair(textures.Find(IsWater));

        map.textures.Resize(map.size, water);

        auto coast = textures.Find(IsCoastTerrain);
        textures.Set(island, coast);

        PlaceHarbors(map, minimumIslandSize, minimumCoastSize, {});

        BOOST_REQUIRE(map.harbors.empty());
    });
}

BOOST_AUTO_TEST_CASE(PlaceHarbors_PlacesHarborOnSuitableIsland)
{
    RunTest([](Map& map, TextureMap& textures) {
        MapPoint island(3, 3);

        const int minimumIslandSize = 1;
        const int minimumCoastSize = 1;

        auto water = TexturePair(textures.Find(IsWater));

        map.textures.Resize(map.size, water);

        auto coast = textures.Find(IsCoastTerrain);
        textures.Set(island, coast);

        PlaceHarbors(map, minimumIslandSize, minimumCoastSize, {});

        BOOST_REQUIRE(!map.harbors.empty());
    });
}

BOOST_AUTO_TEST_CASE(PlaceHarbors_PlacesNoHarborNearRiver)
{
    RunTest([](Map& map, TextureMap& textures) {
        MapPoint island(3, 3);

        const int minimumIslandSize = 1;
        const int minimumCoastSize = 1;

        auto water = TexturePair(textures.Find(IsWater));

        map.textures.Resize(map.size, water);

        auto coast = textures.Find(IsCoastTerrain);
        textures.Set(island, coast);

        River river{island};

        PlaceHarbors(map, minimumIslandSize, minimumCoastSize, {river});

        BOOST_REQUIRE(map.harbors.empty());
    });
}

BOOST_AUTO_TEST_SUITE_END()