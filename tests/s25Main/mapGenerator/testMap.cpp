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

#include "PointOutput.h"
#include "mapGenFixtures.h"
#include "mapGenerator/Map.h"
#include <boost/test/unit_test.hpp>

using namespace rttr::mapGenerator;

BOOST_FIXTURE_TEST_SUITE(MapTests, MapGenFixture)

BOOST_AUTO_TEST_CASE(Constructor_resizes_all_maps)
{
    const Map map = createMap(MapExtent(14, 6));
    BOOST_TEST(map.z.GetSize() == map.size);
    BOOST_TEST(map.getTextures().GetSize() == map.size);
    BOOST_TEST(map.objectTypes.GetSize() == map.size);
    BOOST_TEST(map.objectInfos.GetSize() == map.size);
    BOOST_TEST(map.resources.GetSize() == map.size);
    BOOST_TEST(map.animals.GetSize() == map.size);
}

BOOST_AUTO_TEST_SUITE_END()
