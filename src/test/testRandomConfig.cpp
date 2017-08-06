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

#include "defines.h" // IWYU pragma: keep
#include "mapGenerator/RandomConfig.h"
#include <boost/array.hpp>
#include <boost/foreach.hpp>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(RandomConfigTest)

/**
 * Tests the RandomConfig constructor to ensure the maximum height of an area does
 * not exceed the number of textures.
 */
BOOST_AUTO_TEST_CASE(MaxHeightBelowTextureCount)
{
    boost::array<MapStyle, 7> mapStyles = {{MapStyle::Greenland, MapStyle::Riverland, MapStyle::Ringland, MapStyle::Migration,
                                            MapStyle::Islands, MapStyle::Continent, MapStyle::Random}};
    BOOST_FOREACH(MapStyle mapStyle, mapStyles)
    {
        RandomConfig config(mapStyle, 0x1337);
        BOOST_FOREACH(const AreaDesc& area, config.areas)
        {
            BOOST_REQUIRE_LT(area.maxElevation, config.textures.size());
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
