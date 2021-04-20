// Copyright (c) 2017 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "mapGenFixtures.h"
#include "mapGenerator/Rivers.h"
#include "mapGenerator/TextureHelper.h"
#include <boost/test/unit_test.hpp>

using namespace rttr::mapGenerator;

class RiverFixture : public MapGenFixture
{
public:
    Map map;
    RandomUtility rnd;
    RiverFixture() : map(createMap(MapExtent(8, 10))), rnd(0) { map.z.Resize(map.size, map.height.maximum); }
};

BOOST_FIXTURE_TEST_SUITE(RiversTest, RiverFixture)

BOOST_AUTO_TEST_CASE(CreateStream_returns_river_of_expected_size)
{
    const MapPoint source(4, 1);
    const int length = 6;

    for(const auto d : helpers::enumRange<Direction>())
    {
        auto river = CreateStream(rnd, map, source, d, length);

        // each point represents one triangle while length is given
        // in tiles (2 triangles) - and one more for the source (which
        // is not counted in length)

        const unsigned expectedNodes = (length + 1) * 2;

        BOOST_TEST_REQUIRE(static_cast<unsigned>(river.size()) == expectedNodes);
    }
}

BOOST_AUTO_TEST_CASE(CreateStream_returns_only_connected_nodes)
{
    const MapPoint source(3, 2);
    const int length = 7;

    for(const auto d : helpers::enumRange<Direction>())
    {
        auto river = CreateStream(rnd, map, source, d, length);

        auto containedByRiver = [&river](const MapPoint& pt) { return helpers::contains(river, pt); };

        for(const MapPoint& pt : river)
        {
            BOOST_TEST_REQUIRE(helpers::contains_if(map.z.GetNeighbours(pt), containedByRiver));
        }
    }
}

BOOST_AUTO_TEST_CASE(CreateStream_returns_only_nodes_covered_by_water)
{
    auto land = map.textureMap.Find(IsBuildableLand);
    map.getTextures().Resize(map.size, land);
    const MapPoint source(3, 2);
    const int length = 7;

    for(const auto d : helpers::enumRange<Direction>())
    {
        auto river = CreateStream(rnd, map, source, d, length);

        for(const MapPoint& pt : river)
        {
            BOOST_TEST_REQUIRE(map.textureMap.Any(pt, IsWater));
        }
    }
}

BOOST_AUTO_TEST_CASE(CreateStream_reduces_height_of_river_nodes)
{
    NodeMapBase<uint8_t> originalZ;
    originalZ.Resize(map.size);
    RTTR_FOREACH_PT(MapPoint, map.size)
    {
        originalZ[pt] = map.z[pt];
    }

    const MapPoint source(4, 1);
    const int length = 6;

    for(const auto d : helpers::enumRange<Direction>())
    {
        auto river = CreateStream(rnd, map, source, d, length);

        for(const MapPoint& pt : river)
        {
            BOOST_TEST_REQUIRE(map.z[pt] < originalZ[pt]);
        }
    }
}

BOOST_AUTO_TEST_CASE(CreateStream_which_ends_at_minimum_height)
{
    MapPoint source(3, 3);
    map.z.Resize(map.size, map.height.minimum);
    map.z[source] = map.height.maximum;

    const auto river = CreateStream(rnd, map, source, Direction::East, 20);
    const auto expectedRange = map.z.GetPointsInRadiusWithCenter(source, 2);

    for(const MapPoint& pt : river)
    {
        BOOST_TEST_REQUIRE(helpers::contains(expectedRange, pt));
    }
}

BOOST_AUTO_TEST_SUITE_END()
