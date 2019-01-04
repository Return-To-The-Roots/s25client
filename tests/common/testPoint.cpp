// Copyright (c) 2016 - 2018 Settlers Freaks (sf-team at siedler25.org)
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

#include "commonDefines.h" // IWYU pragma: keep
#include "PointOutput.h"
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(ProdOfComponents)
{
    Point<uint16_t> pt(256, 256);
    BOOST_REQUIRE_EQUAL(prodOfComponents(pt), 256u * 256u);
    Point<int16_t> ptI(256, 256);
    BOOST_REQUIRE_EQUAL(prodOfComponents(ptI), 256 * 256);
    Point<float> ptF(256.5, 256.5);
    BOOST_REQUIRE_EQUAL(prodOfComponents(ptF), 256.5f * 256.5f);
}
