// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "mapGenFixtures.h"
#include "mapGenerator/Harbors.h"
#include "mapGenerator/Terrain.h"
#include "mapGenerator/TextureHelper.h"
#include "gameTypes/GameTypesOutput.h"
#include <boost/test/unit_test.hpp>

using namespace rttr::mapGenerator;

BOOST_FIXTURE_TEST_SUITE(HarborsTests, MapGenFixture)

BOOST_AUTO_TEST_CASE(PlaceHarborPosition_flattens_ground_around_harbor_position)
{
    Map map = createMap(MapExtent(6, 8));
    auto land = TexturePair(map.textureMap.Find(IsBuildableLand));

    map.getTextures().Resize(map.size, land);

    MapPoint position(3, 4);

    map.z.Resize(map.size, map.height.maximum);

    const auto minHeight = map.height.minimum;
    const auto neighbors = map.z.GetNeighbours(position);

    for(unsigned i = 0; i < neighbors.size(); i++)
    {
        map.z[neighbors[Direction(i)]] = minHeight + i;
    }

    // run actual test
    PlaceHarborPosition(map, position);

    BOOST_TEST(map.z[position] == minHeight);
    for(auto neighbor : neighbors)
    {
        BOOST_TEST(map.z[neighbor] == minHeight);
    }
}

BOOST_AUTO_TEST_CASE(PlaceHarborPosition_applies_buildable_terrain_around_position)
{
    Map map = createMap(MapExtent(6, 8));

    auto coast = TexturePair(map.textureMap.Find(IsCoastTerrain));
    auto buildable = map.textureMap.Find(IsBuildableCoast);

    map.getTextures().Resize(map.size, coast);

    MapPoint position(3, 4);

    PlaceHarborPosition(map, position);

    auto triangles = GetTriangles(position, map.size);
    const auto& textureTriangles = map.getTextures();
    for(const rttr::mapGenerator::Triangle& triangle : triangles)
    {
        if(triangle.rsu)
            BOOST_TEST(textureTriangles[triangle.position].rsu == buildable);
        else
            BOOST_TEST(textureTriangles[triangle.position].lsd == buildable);
    }
}

BOOST_AUTO_TEST_CASE(PlaceHarbors_places_no_harbors_on_coast_below_minimum_size)
{
    Map map = createMap(MapExtent(6, 8));
    MapPoint island(3, 3);

    auto water = TexturePair(map.textureMap.Find(IsWater));

    map.getTextures().Resize(map.size, water);

    auto coast = map.textureMap.Find(IsCoastTerrain);
    map.textureMap.Set(island, coast);

    PlaceHarbors(map, {}, 100);

    BOOST_TEST(map.harbors.empty());
}

BOOST_AUTO_TEST_CASE(PlaceHarbors_places_harbor_on_suitable_island)
{
    Map map = createMap(MapExtent(6, 8));
    MapPoint island(3, 3);

    auto water = TexturePair(map.textureMap.Find(IsWater));

    map.getTextures().Resize(map.size, water);

    auto coast = map.textureMap.Find(IsCoastTerrain);
    map.textureMap.Set(island, coast);

    PlaceHarbors(map, {}, 5);

    BOOST_TEST(!map.harbors.empty());
}

BOOST_AUTO_TEST_CASE(PlaceHarbors_places_no_harbor_near_river)
{
    Map map = createMap(MapExtent(6, 8));
    MapPoint island(3, 3);

    auto water = TexturePair(map.textureMap.Find(IsWater));

    map.getTextures().Resize(map.size, water);

    auto coast = map.textureMap.Find(IsCoastTerrain);
    map.textureMap.Set(island, coast);

    River river{island};

    PlaceHarbors(map, {river}, 5);

    BOOST_TEST(map.harbors.empty());
}

BOOST_AUTO_TEST_SUITE_END()
