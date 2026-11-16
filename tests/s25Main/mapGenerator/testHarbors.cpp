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
    auto land = TexturePair(map.textureMap.Find(isBuildableLand));

    map.getTextures().resize(map.size, land);

    MapPoint position(3, 4);

    map.z.resize(map.size, map.height.maximum);

    const auto minHeight = map.height.minimum;
    const auto neighbors = map.z.getNeighbours(position);

    for(unsigned i = 0; i < neighbors.size(); i++)
    {
        map.z[neighbors[Direction(i)]] = minHeight + i;
    }

    // run actual test
    placeHarborPosition(map, position);

    BOOST_TEST(map.z[position] == minHeight);
    for(auto neighbor : neighbors)
    {
        BOOST_TEST(map.z[neighbor] == minHeight);
    }
}

BOOST_AUTO_TEST_CASE(PlaceHarborPosition_applies_buildable_terrain_around_position)
{
    Map map = createMap(MapExtent(6, 8));

    auto coast = TexturePair(map.textureMap.Find(isCoastTerrain));
    auto buildable = map.textureMap.Find(isBuildableCoast);

    map.getTextures().resize(map.size, coast);

    MapPoint position(3, 4);

    placeHarborPosition(map, position);

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

    map.getTextures().resize(map.size, water);

    auto coast = map.textureMap.Find(isCoastTerrain);
    map.textureMap.set(island, coast);

    placeHarbors(map, {}, 100);

    BOOST_TEST(map.harbors.empty());
}

BOOST_AUTO_TEST_CASE(PlaceHarbors_places_harbor_on_suitable_island)
{
    Map map = createMap(MapExtent(6, 8));
    MapPoint island(3, 3);

    auto water = TexturePair(map.textureMap.Find(IsWater));

    map.getTextures().resize(map.size, water);

    auto coast = map.textureMap.Find(isCoastTerrain);
    map.textureMap.set(island, coast);

    placeHarbors(map, {}, 5);

    BOOST_TEST(!map.harbors.empty());
}

BOOST_AUTO_TEST_CASE(PlaceHarbors_places_no_harbor_near_river)
{
    Map map = createMap(MapExtent(6, 8));
    MapPoint island(3, 3);

    auto water = TexturePair(map.textureMap.Find(IsWater));

    map.getTextures().resize(map.size, water);

    auto coast = map.textureMap.Find(isCoastTerrain);
    map.textureMap.set(island, coast);

    River river{island};

    placeHarbors(map, {river}, 5);

    BOOST_TEST(map.harbors.empty());
}

BOOST_AUTO_TEST_SUITE_END()
