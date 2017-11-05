// Copyright (c) 2016 - 2017 Settlers Freaks (sf-team at siedler25.org)
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
#include "helpers/containerUtils.h"
#include "test/initTestHelpers.h"
#include <boost/assign/std/vector.hpp>
#include <boost/test/unit_test.hpp>
#include <vector>

BOOST_AUTO_TEST_SUITE(ContainerUtils)

BOOST_AUTO_TEST_CASE(MakeUnique)
{
    // Empty vector -> Not modified
    std::vector<int> vec;
    helpers::makeUnique(vec);
    BOOST_REQUIRE(vec.empty());
    // 1 el -> Not modified
    vec.push_back(1);
    helpers::makeUnique(vec);
    BOOST_REQUIRE_EQUAL(vec.at(0), 1);
    // 2 same els -> Only 1 remains
    vec.push_back(1);
    helpers::makeUnique(vec);
    BOOST_REQUIRE_EQUAL(vec.size(), 1u);
    BOOST_REQUIRE_EQUAL(vec[0], 1);
    // 2 different els -> Both remain
    vec.push_back(-1);
    helpers::makeUnique(vec);
    BOOST_REQUIRE_EQUAL(vec.size(), 2u);
    BOOST_REQUIRE_EQUAL(vec[0], 1);
    BOOST_REQUIRE_EQUAL(vec[1], -1);
    // More mixed elements
    vec.clear();
    using namespace boost::assign;
    vec += 5, 6, 5, 5, 2, 6, 1, 5, 7, 7, 3, -1, 3;
    std::vector<int> expectedVec;
    expectedVec += 5, 6, 2, 1, 7, 3, -1;
    helpers::makeUnique(vec);
    RTTR_REQUIRE_EQUAL_COLLECTIONS(vec, expectedVec);
}

BOOST_AUTO_TEST_CASE(IndexOf)
{
    std::vector<int> vec;
    // Empty vector
    BOOST_REQUIRE_EQUAL(helpers::indexOf(vec, 1), -1);
    // 1 el
    vec.push_back(1);
    BOOST_REQUIRE_EQUAL(helpers::indexOf(vec, 1), 0);
    BOOST_REQUIRE_EQUAL(helpers::indexOf(vec, 2), -1);
    // 2 els
    vec.push_back(0);
    BOOST_REQUIRE_EQUAL(helpers::indexOf(vec, 1), 0);
    BOOST_REQUIRE_EQUAL(helpers::indexOf(vec, 0), 1);
    BOOST_REQUIRE_EQUAL(helpers::indexOf(vec, 2), -1);

    // Pointer vector
    std::vector<int*> ptrVec;
    BOOST_REQUIRE_EQUAL(helpers::indexOf(ptrVec, (int*)1337), -1);
    BOOST_REQUIRE_EQUAL(helpers::indexOf(ptrVec, (const int*)1337), -1);
    ptrVec.push_back((int*)1336);
    ptrVec.push_back((int*)1337);
    ptrVec.push_back((int*)1338);
    BOOST_REQUIRE_EQUAL(helpers::indexOf(ptrVec, (int*)1337), 1);
    BOOST_REQUIRE_EQUAL(helpers::indexOf(ptrVec, (const int*)1337), 1);
}
BOOST_AUTO_TEST_SUITE_END()
