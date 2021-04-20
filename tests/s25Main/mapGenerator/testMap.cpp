// Copyright (c) 2017 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
