// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "helpers/containerUtils.h"
#include "helpers/reverse.h"
#include "s25util/warningSuppression.h"
#include <rttr/test/random.hpp>
#include <boost/test/unit_test.hpp>
#include <set>
#include <vector>

BOOST_AUTO_TEST_SUITE(ContainerUtils)

constexpr boost::test_tools::per_element per_element;

BOOST_AUTO_TEST_CASE(MakeUniqueStable)
{
    // Empty vector -> Not modified
    std::vector<int> vec;
    helpers::makeUniqueStable(vec);
    BOOST_TEST_REQUIRE(vec.empty());
    // 1 el -> Not modified
    vec.push_back(1);
    helpers::makeUniqueStable(vec);
    BOOST_TEST_REQUIRE(vec.at(0) == 1);
    // 2 same els -> Only 1 remains
    vec.push_back(1);
    helpers::makeUniqueStable(vec);
    BOOST_TEST_REQUIRE(vec.size() == 1u);
    BOOST_TEST_REQUIRE(vec[0] == 1);
    // 2 different els -> Both remain
    vec.push_back(-1);
    helpers::makeUniqueStable(vec);
    BOOST_TEST_REQUIRE(vec.size() == 2u);
    BOOST_TEST_REQUIRE(vec[0] == 1);
    BOOST_TEST_REQUIRE(vec[1] == -1);
    // More mixed elements
    vec = {5, 6, 5, 5, 2, 6, 1, 5, 7, 7, 3, -1, 3};
    std::vector<int> expectedVec = {5, 6, 2, 1, 7, 3, -1};
    helpers::makeUniqueStable(vec);
    BOOST_TEST(vec == expectedVec, per_element);
}

BOOST_AUTO_TEST_CASE(MakeUnique)
{
    // Empty vector -> Not modified
    std::vector<int> vec;
    helpers::makeUnique(vec);
    BOOST_TEST_REQUIRE(vec.empty());
    // 1 el -> Not modified
    vec.push_back(1);
    helpers::makeUnique(vec);
    BOOST_TEST_REQUIRE(vec.at(0) == 1);
    // 2 same els -> Only 1 remains
    vec.push_back(1);
    helpers::makeUnique(vec);
    BOOST_TEST_REQUIRE(vec.size() == 1u);
    BOOST_TEST_REQUIRE(vec[0] == 1);
    // 2 different els -> Both remain
    vec.push_back(-1);
    helpers::makeUnique(vec);
    BOOST_TEST_REQUIRE(vec.size() == 2u);
    BOOST_TEST(vec[0] == -1);
    BOOST_TEST(vec[1] == 1);
    // More mixed elements
    vec = {5, 6, 5, 5, 2, 6, 1, 5, 7, 7, 3, -1, 3};
    std::vector<int> expectedVec = {-1, 1, 2, 3, 5, 6, 7};
    helpers::makeUnique(vec);
    BOOST_TEST(vec == expectedVec, per_element);
}

BOOST_AUTO_TEST_CASE(IndexOf)
{
    std::vector<int> vec;
    // Empty vector
    BOOST_TEST(helpers::indexOf(vec, 1) == -1);
    // 1 el
    vec.push_back(1);
    BOOST_TEST(helpers::indexOf(vec, 1) == 0);
    BOOST_TEST(helpers::indexOf(vec, 2) == -1);
    // 2 els
    vec.push_back(0);
    BOOST_TEST(helpers::indexOf(vec, 1) == 0);
    BOOST_TEST(helpers::indexOf(vec, 0) == 1);
    BOOST_TEST(helpers::indexOf(vec, 2) == -1);

    // Pointer vector
    std::vector<int*> ptrVec;
    BOOST_TEST(helpers::indexOf(ptrVec, (int*)1337) == -1);       //-V566
    BOOST_TEST(helpers::indexOf(ptrVec, (const int*)1337) == -1); //-V566
    ptrVec.push_back((int*)1336);                                 //-V566
    ptrVec.push_back((int*)1337);                                 //-V566
    ptrVec.push_back((int*)1338);                                 //-V566
    BOOST_TEST(helpers::indexOf(ptrVec, (int*)1337) == 1);        //-V566
    BOOST_TEST(helpers::indexOf(ptrVec, (const int*)1337) == 1);  //-V566

    vec = {1, 3, 5, 6};
    BOOST_TEST(helpers::indexOf_if(vec, [](int el) { return el % 2 == 0; }) == 3);
    BOOST_TEST(helpers::indexOf_if(vec, [](int el) { return el > 2; }) == 1);
    BOOST_TEST(helpers::indexOf_if(vec, [](int el) { return el > 6; }) == -1);
}

BOOST_AUTO_TEST_CASE(Reverse)
{
    std::vector<int> vecIn, vecOut;
    for(int i : helpers::reverse(vecIn))
    {
        RTTR_UNUSED(i);                                             // LCOV_EXCL_LINE
        BOOST_TEST_FAIL("Reverse of empty vector should be empty"); // LCOV_EXCL_LINE
    }

    vecIn.resize(rttr::test::randomValue(1, 100));
    for(int& i : vecIn)
        i = rttr::test::randomValue<int>();

    for(int i : helpers::reverse(vecIn))
        vecOut.push_back(i);
    std::reverse(vecIn.begin(), vecIn.end());
    BOOST_TEST(vecIn == vecOut, per_element);
}

BOOST_AUTO_TEST_CASE(Erase)
{
    std::vector<int> vecIn = {1, 2, 3, 4, 5}, vecExp;
    helpers::erase(vecIn, 42);
    BOOST_TEST(vecIn == vecIn, per_element);
    helpers::erase(vecIn, 2);
    vecExp = {1, 3, 4, 5};
    BOOST_TEST(vecIn == vecExp, per_element);
    helpers::erase(vecIn, 1);
    vecExp = {3, 4, 5};
    BOOST_TEST(vecIn == vecExp, per_element);
    helpers::erase(vecIn, 5);
    vecExp = {3, 4};
    BOOST_TEST(vecIn == vecExp, per_element);
    helpers::erase(vecIn, 4);
    vecExp = {3};
    BOOST_TEST(vecIn == vecExp, per_element);
    helpers::erase(vecIn, 3);
    vecExp = {};
    BOOST_TEST(vecIn == vecExp, per_element);
}

BOOST_AUTO_TEST_CASE(PopFront)
{
    std::vector<int> vec{1, 2, 3};
    helpers::pop_front(vec);
    BOOST_TEST(vec == (std::vector<int>{2, 3}));

    std::set<int> set{1, 2, 3};
    helpers::pop_front(set);
    BOOST_TEST(set == (std::set<int>{2, 3}));
}

BOOST_AUTO_TEST_CASE(Find)
{
    std::vector<int> vec{1, 2, 3};
    BOOST_TEST((helpers::find(vec, 1) == vec.begin()));
    BOOST_TEST((helpers::find(vec, 2) == vec.begin() + 1));
    BOOST_TEST((helpers::find(vec, 4) == vec.end()));

    std::set<int> set{1, 2, 3};
    BOOST_TEST((helpers::find(set, 1) == set.begin()));
    BOOST_TEST((helpers::find(set, 2) == set.find(2)));
    BOOST_TEST((helpers::find(set, 4) == set.end()));
}

BOOST_AUTO_TEST_CASE(RemoveIf)
{
    std::vector<int> vecIn = {1, 2, 3, 4, 5, 6}, vecExp = {1, 3, 5};
    helpers::erase_if(vecIn, [](int i) { return i % 2 == 0; });
    BOOST_TEST(vecIn == vecExp, per_element);

    vecIn = {1, 2, 3, 4, 5, 6}, vecExp = {2, 4, 6};
    helpers::erase_if(vecIn, [](int i) { return i % 2 != 0; });
    BOOST_TEST(vecIn == vecExp, per_element);
}

BOOST_AUTO_TEST_CASE(Count)
{
    std::vector<int> values = {1, 2, 2, 4, 4, 4};
    BOOST_TEST(helpers::count(values, 1) == 1u);
    BOOST_TEST(helpers::count(values, 2) == 2u);
    BOOST_TEST(helpers::count(values, 3) == 0u);
    BOOST_TEST(helpers::count(values, 4) == 3u);
}

BOOST_AUTO_TEST_CASE(CountIf)
{
    std::vector<int> values = {1, 2, 3, 4, 5};
    const auto isEven = [](int i) { return i % 2 == 0; };
    BOOST_TEST(helpers::count_if(values, isEven) == 2u);
}

BOOST_AUTO_TEST_SUITE_END()
