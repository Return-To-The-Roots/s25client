// Copyright (c) 2016 -2017 Settlers Freaks (sf-team at siedler25.org)
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

#include "rttrDefines.h" // IWYU pragma: keep
#include "PointOutput.h"
#include "controls/ctrlPreviewMinimap.h"
#include "libsiedler2/ArchivItem_Map_Header.h"
#include "libsiedler2/ArchivItem_Raw.h"
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(Controls)

void resizeMap(glArchivItem_Map& glMap, const Extent& size)
{
    libsiedler2::ArchivItem_Map map;
    libsiedler2::ArchivItem_Map_Header* header = new libsiedler2::ArchivItem_Map_Header;
    header->setWidth(size.x);
    header->setHeight(size.y);
    header->setNumPlayers(2);
    map.push(header);
    for(int i = 0; i <= MAP_TYPE; i++)
        map.push(new libsiedler2::ArchivItem_Raw(std::vector<uint8_t>(prodOfComponents(size))));
    glMap.load(map);
}

BOOST_AUTO_TEST_CASE(PreviewMinimap)
{
    DrawPoint pos(5, 12);
    Extent size(20, 10);
    ctrlPreviewMinimap mm(NULL, 1, pos, size, NULL);
    BOOST_REQUIRE_EQUAL(mm.GetCurMapSize(), Extent::all(0));
    BOOST_REQUIRE_EQUAL(mm.GetBoundaryRect().getSize(), Extent::all(4)); // Padding
    // Remove padding
    mm.SetPadding(Extent::all(0));
    BOOST_REQUIRE_EQUAL(mm.GetBoundaryRect().getSize(), Extent::all(0));
    glArchivItem_Map map;
    resizeMap(map, size);
    mm.SetMap(&map);
    BOOST_REQUIRE_LE(mm.GetBoundaryRect().getSize().x, size.x);
    BOOST_REQUIRE_LE(mm.GetBoundaryRect().getSize().y, size.y);
    const Extent origBoundary = mm.GetBoundaryRect().getSize();
    // Width smaller -> Don't go over width
    resizeMap(map, Extent(size.x / 2, size.y));
    mm.SetMap(&map);
    BOOST_REQUIRE_LE(mm.GetBoundaryRect().getSize().x, size.x);
    BOOST_REQUIRE_EQUAL(mm.GetBoundaryRect().getSize().y, size.y);
    // Height smaller -> Don't go over height
    resizeMap(map, Extent(size.x, size.y / 2));
    mm.SetMap(&map);
    BOOST_REQUIRE_EQUAL(mm.GetBoundaryRect().getSize().x, size.x);
    BOOST_REQUIRE_LE(mm.GetBoundaryRect().getSize().y, size.y);
    // Both smaller -> Stretch to original height
    resizeMap(map, size / 2u);
    mm.SetMap(&map);
    BOOST_REQUIRE_EQUAL(mm.GetBoundaryRect().getSize(), origBoundary);
    // Width bigger -> Narrow map
    resizeMap(map, Extent(size.x * 2, size.y));
    mm.SetMap(&map);
    BOOST_REQUIRE_EQUAL(mm.GetBoundaryRect().getSize().x, size.x);
    BOOST_REQUIRE_LE(mm.GetBoundaryRect().getSize().y, size.y);
    // Height bigger -> Narrow map in x
    resizeMap(map, Extent(size.x, size.y * 2));
    mm.SetMap(&map);
    BOOST_REQUIRE_LE(mm.GetBoundaryRect().getSize().x, size.x);
    BOOST_REQUIRE_EQUAL(mm.GetBoundaryRect().getSize().y, size.y);
    // Both bigger -> Stretch to original height
    resizeMap(map, size * 2u);
    mm.SetMap(&map);
    BOOST_REQUIRE_EQUAL(mm.GetBoundaryRect().getSize(), origBoundary);
}

BOOST_AUTO_TEST_SUITE_END()
