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
#include "mapGenerator/Rivers.h"
#include "mapGenerator/TextureHelper.h"
#include <boost/test/unit_test.hpp>

using namespace rttr::mapGenerator;

BOOST_AUTO_TEST_SUITE(RiversTests)

template<class T_Test>
void RunTest(T_Test test);

template<class T_Test>
void RunTest(T_Test test)
{
    MapExtent size(8, 8);
    RandomUtility rnd(0);
    WorldDescription worldDesc;
    loadGameData(worldDesc);
    DescIdx<LandscapeDesc> landscape(0);
    Map map(size, 0x1, worldDesc, landscape);

    test(rnd, map);
}

BOOST_AUTO_TEST_CASE(CreateStream_returns_river_of_expected_size)
{
    RunTest([](RandomUtility& rnd, Map& map) {
        const MapPoint source(4, 1);
        const int length = 6;

        for(const auto d : helpers::enumRange<Direction>())
        {
            auto river = CreateStream(rnd, map, source, d, length);

            // each point represents one triangle while length is given
            // in tiles (2 triangles) - and one more for the source (which
            // is not counted in length)

            const unsigned expectedNodes = (length + 1) * 2;

            BOOST_REQUIRE_EQUAL(static_cast<unsigned>(river.size()), expectedNodes);
        }
    });
}

BOOST_AUTO_TEST_CASE(CreateStream_returns_only_connected_nodes)
{
    RunTest([](RandomUtility& rnd, Map& map) {
        const MapPoint source(3, 2);
        const int length = 7;

        for(const auto d : helpers::enumRange<Direction>())
        {
            auto river = CreateStream(rnd, map, source, d, length);

            auto containedByRiver = [&river](const MapPoint& pt) { return helpers::contains(river, pt); };

            for(const MapPoint& pt : river)
            {
                BOOST_REQUIRE(helpers::contains_if(map.z.GetNeighbours(pt), containedByRiver));
            }
        }
    });
}

BOOST_AUTO_TEST_CASE(CreateStream_returns_only_nodes_covered_by_water)
{
    RunTest([](RandomUtility& rnd, Map& map) {
        auto land = map.textureMap.Find(IsBuildableLand);
        map.textures.Resize(map.size, land);
        const MapPoint source(3, 2);
        const int length = 7;

        for(const auto d : helpers::enumRange<Direction>())
        {
            auto river = CreateStream(rnd, map, source, d, length);

            for(const MapPoint& pt : river)
            {
                BOOST_REQUIRE(map.textureMap.Any(pt, IsWater));
            }
        }
    });
}

BOOST_AUTO_TEST_CASE(CreateStream_reduces_height_of_river_nodes)
{
    RunTest([](RandomUtility& rnd, Map& map) {
        map.z.Resize(map.size, 4);
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
                BOOST_REQUIRE(map.z[pt] < originalZ[pt]);
            }
        }
    });
}

BOOST_AUTO_TEST_SUITE_END()
