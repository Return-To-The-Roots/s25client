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

#include "rttrDefines.h" // IWYU pragma: keep
#include "randomMaps/algorithm/BrushSize.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(BrushSizeTests)

BOOST_AUTO_TEST_CASE(SwappedTinyRsuSizeResultsInTinyLsdSize)
{
    auto tinyRsu = BrushSize::Tiny(true);
    auto swapped = BrushSize::Swap(tinyRsu);
    
    BOOST_REQUIRE(!BrushSize::IsRsu(swapped));
}

BOOST_AUTO_TEST_CASE(SwappedTinyLsdSizeResultsInTinyRsuSize)
{
    auto tinyLsd = BrushSize::Tiny(false);
    auto swapped = BrushSize::Swap(tinyLsd);
    
    BOOST_REQUIRE(BrushSize::IsRsu(swapped));
}

BOOST_AUTO_TEST_CASE(TinySizeIsConsideredTiny)
{
    BOOST_REQUIRE(BrushSize::IsTiny(BrushSize::Tiny(true)));
    BOOST_REQUIRE(BrushSize::IsTiny(BrushSize::Tiny(false)));
}

BOOST_AUTO_TEST_CASE(TinyRsuSizeIsConsideredRsu)
{
    BOOST_REQUIRE(BrushSize::IsRsu(BrushSize::Tiny(true)));
}

BOOST_AUTO_TEST_CASE(TinyLsdSizeIsNotConsideredRsu)
{
    BOOST_REQUIRE(!BrushSize::IsRsu(BrushSize::Tiny(false)));
}

BOOST_AUTO_TEST_CASE(NoneTinySizesAreNotConsideredTiny)
{
    BOOST_REQUIRE(!BrushSize::IsTiny(BrushSize::Small()));
    BOOST_REQUIRE(!BrushSize::IsTiny(BrushSize::Medium()));
    BOOST_REQUIRE(!BrushSize::IsTiny(BrushSize::Large()));
    BOOST_REQUIRE(!BrushSize::IsTiny(BrushSize::Huge()));
}

BOOST_AUTO_TEST_SUITE_END()
