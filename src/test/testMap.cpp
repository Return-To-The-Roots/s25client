// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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
#include "world/MapLoader.h"
#include "world/World.h"
#include "GameObject.h"
#include "EventManager.h"
#include "ogl/glArchivItem_Map.h"
#include "libsiedler2/src/ArchivItem_Map_Header.h"
#include <boost/test/unit_test.hpp>
#include <fstream>

class TestWorld: public World
{
    void AltitudeChanged(const MapPoint pt) override {}
};

BOOST_AUTO_TEST_CASE(LoadWorld)
{
    glArchivItem_Map map;
    std::ifstream mapFile("RTTR/MAPS/NEW/Bergruft.swd", std::ios::binary);
    BOOST_REQUIRE_EQUAL(map.load(mapFile, false), 0);
    BOOST_CHECK_EQUAL(map.getHeader().getWidth(), 176);
    BOOST_CHECK_EQUAL(map.getHeader().getHeight(), 80);
    BOOST_CHECK_EQUAL(map.getHeader().getPlayer(), 4);

    /*std::vector<Nation> players(4, NAT_ROMANS);
    TestWorld world;
    EventManager evMgr;
    InitEventMgr::GetEvMgr() = &evMgr;
    MapLoader loader(world, players);
    BOOST_REQUIRE(loader.Load(map, false, EXP_FOGOFWAR));
    BOOST_CHECK_EQUAL(world.GetWidth(), map.getHeader().getWidth());
    BOOST_CHECK_EQUAL(world.GetHeight(), map.getHeader().getHeight());*/
}