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
    auto land = TexturePair(map.textureMap.find(isBuildableLand));

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

    auto coast = TexturePair(map.textureMap.find(isCoastTerrain));
    auto buildable = map.textureMap.find(isBuildableCoast);

    map.getTextures().resize(map.size, coast);

    MapPoint position(3, 4);

    placeHarborPosition(map, position);

    auto triangles = getTriangles(position, map.size);
    const auto& textureTriangles = map.getTextures();
    for(const rttr::mapGenerator::Triangle& triangle : triangles)
    {
        if(triangle.rsu)
            BOOST_TEST(textureTriangles[triangle.position].rsu == buildable);
        else
            BOOST_TEST(textureTriangles[triangle.position].lsd == buildable);
    }
}

BOOST_AUTO_TEST_CASE(FindCoastlines_ignores_swamps_made_of_water)
{
    // Some landscapes use water textures for swamps which can't be used by ships
    Map map = createMap(MapExtent(32, 32));
    map.getTextures().resize(map.size, TexturePair(map.textureMap.find(isBuildableLand)));
    const auto water = map.textureMap.find(isWater);
    const MapPoint lake(8, 16);
    const MapPoint swamp(24, 16);
    const unsigned radius = 4;

    for(const MapPoint& center : {lake, swamp})
    {
        for(const MapPoint& pt : map.z.getPointsInRadiusWithCenter(center, radius))
            map.textureMap.set(pt, water);
    }
    const auto swampNodes = map.z.getPointsInRadiusWithCenter(swamp, radius);
    map.barren.insert(swampNodes.begin(), swampNodes.end());

    const auto coasts = findCoastlines(map, {});

    BOOST_TEST_REQUIRE(coasts.size() == 1u);
    BOOST_TEST_REQUIRE(!coasts.front().empty());
    for(const MapPoint& pt : coasts.front())
    {
        BOOST_TEST(map.z.calcDistance(pt, lake) == radius + 1);
    }
}

BOOST_AUTO_TEST_CASE(PlaceHarbors_places_no_harbors_on_coast_below_minimum_size)
{
    Map map = createMap(MapExtent(6, 8));
    MapPoint island(3, 3);

    auto water = TexturePair(map.textureMap.find(isWater));

    map.getTextures().resize(map.size, water);

    auto coast = map.textureMap.find(isCoastTerrain);
    map.textureMap.set(island, coast);

    placeHarbors(map, {}, 100);

    BOOST_TEST(map.harbors.empty());
}

BOOST_AUTO_TEST_CASE(PlaceHarbors_places_harbor_on_suitable_island)
{
    Map map = createMap(MapExtent(6, 8));
    MapPoint island(3, 3);

    auto water = TexturePair(map.textureMap.find(isWater));

    map.getTextures().resize(map.size, water);

    auto coast = map.textureMap.find(isCoastTerrain);
    map.textureMap.set(island, coast);

    placeHarbors(map, {}, 5);

    BOOST_TEST(!map.harbors.empty());
}

BOOST_AUTO_TEST_CASE(PlaceHarbors_places_no_harbor_near_river)
{
    Map map = createMap(MapExtent(6, 8));
    MapPoint island(3, 3);

    auto water = TexturePair(map.textureMap.find(isWater));

    map.getTextures().resize(map.size, water);

    auto coast = map.textureMap.find(isCoastTerrain);
    map.textureMap.set(island, coast);

    River river{island};

    placeHarbors(map, {river}, 5);

    BOOST_TEST(map.harbors.empty());
}

BOOST_AUTO_TEST_SUITE_END()
