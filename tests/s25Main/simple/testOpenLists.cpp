// Copyright (C) 2021 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "pathfinding/OpenListBinaryHeap.h"
#include <rttr/test/random.hpp>
#include <boost/test/unit_test.hpp>
#include <algorithm>
#include <iterator>
#include <set>
#include <vector>

namespace {
struct ListEl : BinaryHeapPosMarker
{
    unsigned key;
    ListEl(unsigned key) : BinaryHeapPosMarker(), key(key) {}
};

struct ListGetKey
{
    constexpr auto operator()(const ListEl& el) const { return el.key; }
};

using OpenListBase = OpenListBinaryHeap<ListEl, ListGetKey>;

class OpenList : public OpenListBase
{
public:
    using Parent = OpenListBase;
    using Parent::arePositionsValid;
    using Parent::isHeap;
};
auto getSortedVector(unsigned ct, bool ascending)
{
    std::vector<ListEl> elements;
    std::generate_n(std::back_inserter(elements), ct, [i = 0u]() mutable { return ListEl(i++); });
    if(!ascending)
        std::reverse(elements.begin(), elements.end());
    return elements;
}
auto getRandomVector(unsigned ct, unsigned maxVal = 100000)
{
    std::vector<ListEl> elements;
    std::uniform_int_distribution<unsigned> distr(0, maxVal);
    std::generate_n(std::back_inserter(elements), ct, [&]() { return ListEl(distr(rttr::test::getRandState())); });
    return elements;
}
} // namespace

BOOST_AUTO_TEST_SUITE(OpenLists)

BOOST_AUTO_TEST_CASE(PushTopWorks)
{
    OpenList list;
    BOOST_TEST_REQUIRE(list.isHeap());
    BOOST_TEST_REQUIRE(list.arePositionsValid());
    BOOST_TEST_REQUIRE(list.size() == 0u);
    BOOST_TEST_REQUIRE(list.empty());

    std::vector<ListEl> elements = getSortedVector(rttr::test::randomValue(5u, 30u), true);
    for(unsigned i = 0; i < elements.size(); ++i)
    {
        // After pushing an element the list must still be a heap and the first (lowest) element at the top
        list.push(&elements[i]);
        BOOST_TEST_REQUIRE(list.isHeap());
        BOOST_TEST_REQUIRE(list.arePositionsValid());
        BOOST_TEST(list.size() == i + 1u);
        BOOST_TEST(list.top() == &elements.front());
    }

    list.clear();
    BOOST_TEST_REQUIRE(list.empty());

    // Now the same but in reverse
    elements = getSortedVector(rttr::test::randomValue(5u, 30u), false);
    for(unsigned i = 0; i < elements.size(); ++i)
    {
        // After pushing an element the list must still be a heap and the last (lowest) element at the top
        list.push(&elements[i]);
        BOOST_TEST_REQUIRE(list.isHeap());
        BOOST_TEST_REQUIRE(list.arePositionsValid());
        BOOST_TEST(list.size() == i + 1u);
        BOOST_TEST(list.top() == &elements[i]);
    }

    list.clear();
    BOOST_TEST_REQUIRE(list.empty());

    // And random
    elements = getRandomVector(rttr::test::randomValue(5u, 50u));
    for(unsigned i = 0; i < elements.size(); ++i)
    {
        // After pushing an element the list must still be a heap and the lowest element at the top
        list.push(&elements[i]);
        BOOST_TEST_REQUIRE(list.isHeap());
        BOOST_TEST_REQUIRE(list.arePositionsValid());
        BOOST_TEST(list.size() == i + 1u);
        const auto minVal =
          std::min_element(elements.begin(), elements.begin() + i + 1, [](const auto& lhs, const auto& rhs) {
              return lhs.key < rhs.key;
          })->key;
        BOOST_TEST(list.top()->key == minVal);
    }
}

BOOST_AUTO_TEST_CASE(PopRemovesLowestElement)
{
    OpenList list;

    // Random vector with some duplicate elements
    auto elements = getRandomVector(rttr::test::randomValue(30u, 70u), 20);
    std::multiset<unsigned> keys;
    for(unsigned i = 0; i < elements.size(); ++i)
    {
        list.push(&elements[i]);
        keys.insert(elements[i].key);
        BOOST_TEST_REQUIRE(list.isHeap());
        BOOST_TEST_REQUIRE(list.arePositionsValid());
        BOOST_TEST(list.size() == i + 1u);
    }
    // Pop all elements should return lowest elements first (hence the set for comparison)
    for(unsigned i = 0; i < elements.size(); ++i)
    {
        BOOST_TEST_REQUIRE(!list.empty());
        const auto* el = list.top();
        BOOST_TEST(el->key == *keys.begin());
        const auto* elPop = list.pop();
        BOOST_TEST_REQUIRE(list.isHeap());
        BOOST_TEST_REQUIRE(list.arePositionsValid());
        BOOST_TEST(elPop->key == *keys.begin());
        BOOST_TEST(elPop == el);
        keys.erase(keys.begin());
    }
    BOOST_TEST(list.empty());
}

BOOST_AUTO_TEST_CASE(RearrangeMakesTheHeapValidAgain)
{
    OpenList list;

    // Random vector with some duplicate elements
    auto elements = getRandomVector(rttr::test::randomValue(30u, 70u), 20);
    std::multiset<unsigned> keys;
    for(auto& el : elements)
    {
        list.push(&el);
        // Just to be sure, actually already tested
        BOOST_TEST_REQUIRE(list.isHeap());
        BOOST_TEST_REQUIRE(list.arePositionsValid());
    }
    // Change each element once
    for(auto& el : elements)
    {
        list.rearrange(&el); // No-op
        BOOST_TEST_REQUIRE(list.isHeap());
        BOOST_TEST_REQUIRE(list.arePositionsValid());
        // Make a lower value
        if(el.key > 0u)
            el.key = rttr::test::randomValue(0u, el.key - 1u);
        keys.insert(el.key);
        list.rearrange(&el);
        BOOST_TEST_REQUIRE(list.isHeap());
        BOOST_TEST_REQUIRE(list.arePositionsValid());
    }

    // Pop all elements in right order
    for(unsigned i = 0; i < elements.size(); ++i)
    {
        BOOST_TEST_REQUIRE(!list.empty());
        const auto* el = list.top();
        const auto* elPop = list.pop();
        BOOST_TEST_REQUIRE(list.isHeap());
        BOOST_TEST_REQUIRE(list.arePositionsValid());
        BOOST_TEST(elPop == el);
        BOOST_TEST(elPop->key == *keys.begin());
        keys.erase(keys.begin());
    }
    BOOST_TEST(list.empty());
}

BOOST_AUTO_TEST_SUITE_END()
