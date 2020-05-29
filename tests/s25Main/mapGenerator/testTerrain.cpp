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

#include "rttrDefines.h"
#include "lua/GameDataLoader.h"
#include "mapGenerator/Terrain.h"
#include "mapGenerator/TextureHelper.h"
#include <boost/test/unit_test.hpp>

using namespace rttr::mapGenerator;

BOOST_AUTO_TEST_SUITE(TerrainTests)

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

    test(map);
}

BOOST_AUTO_TEST_CASE(Restructure_ForFocusArea_KeepsMinimumAndMaximumHeight)
{
    RunTest([](Map& map) {
        map.z.Resize(map.size, 5); // default height

        std::vector<MapPoint> focusArea = {MapPoint(4, 4)}; // center

        Restructure(map, focusArea);

        RTTR_FOREACH_PT(MapPoint, map.size)
        {
            BOOST_REQUIRE(map.z[pt] <= map.height.maximum);
            BOOST_REQUIRE(map.z[pt] >= map.height.minimum);
        }
    });
}

BOOST_AUTO_TEST_CASE(Restructure_ForFocusArea_ElevatesFocusArea)
{
    RunTest([](Map& map) {
        MapPoint focus(4, 4);

        const uint8_t heightBefore = 5;
        map.z.Resize(map.size, heightBefore);

        Restructure(map, {focus});

        const uint8_t heightAfter = map.z[focus];

        BOOST_REQUIRE(heightAfter > heightBefore);
    });
}

BOOST_AUTO_TEST_CASE(Restructure_ForPointFurtherFromFocus_ElevatesPointLessThanFocus)
{
    RunTest([](Map& map) {
        MapPoint focus(4, 4);
        MapPoint nonFocus(0, 3);

        const uint8_t heightBefore = 5;
        map.z.Resize(map.size, heightBefore);

        Restructure(map, {focus});

        const int diffFocus = map.z[focus] - heightBefore;
        const int diffNonFocus = map.z[nonFocus] - heightBefore;

        BOOST_REQUIRE(diffFocus > diffNonFocus);
    });
}

BOOST_AUTO_TEST_SUITE_END()
