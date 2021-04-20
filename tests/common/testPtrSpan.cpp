// Copyright (C) 2016 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "helpers/PtrSpan.h"
#include "rttr/test/random.hpp"
#include <boost/test/unit_test.hpp>
#include <memory>

namespace {
struct TestClass
{
    int value;
};
} // namespace

BOOST_AUTO_TEST_CASE(PlaintPtrSpan)
{
    std::vector<TestClass> values(10);
    std::vector<TestClass*> ptrs;
    const auto initialSpan = helpers::nonNullPtrSpan(ptrs);
    BOOST_TEST(initialSpan.empty());
    for(TestClass& i : values)
    {
        i.value = rttr::test::randomValue<int>();
        ptrs.push_back(&i);
    }
    BOOST_TEST(!helpers::nonNullPtrSpan(ptrs).empty());
    BOOST_TEST(!initialSpan.empty());

    auto it = values.begin();
    const auto span = helpers::nonNullPtrSpan(ptrs);
    BOOST_TEST_REQUIRE(span.size() == values.size());
    BOOST_TEST_REQUIRE(initialSpan.size() == values.size());
    for(auto it2 = span.begin(); it2 != span.end(); ++it2, ++it)
    {
        static_assert(std::is_same<decltype(*it2), TestClass&>::value, "!");
        BOOST_TEST((*it2).value == it->value);
        BOOST_TEST(it2->value == it->value);
        BOOST_TEST(&*it2 == &*it);
    }

    it = values.begin();
    for(const TestClass& i : helpers::nonNullPtrSpan(ptrs))
    {
        BOOST_TEST(i.value == it->value);
        BOOST_TEST(&i == &*it);
        ++it;
    }

    it = values.begin();
    for(const TestClass& i : initialSpan)
    {
        BOOST_TEST(i.value == it->value);
        BOOST_TEST(&i == &*it);
        ++it;
    }
}

BOOST_AUTO_TEST_CASE(UniquePtrSpan)
{
    std::vector<std::unique_ptr<TestClass>> ptrs;
    const auto initialSpan = helpers::nonNullPtrSpan(ptrs);
    BOOST_TEST(initialSpan.empty());
    for(int i = 0; i < 10; i++)
        ptrs.push_back(std::make_unique<TestClass>(TestClass{rttr::test::randomValue<int>()}));
    BOOST_TEST(!helpers::nonNullPtrSpan(ptrs).empty());
    BOOST_TEST(!initialSpan.empty());

    auto it = ptrs.begin();
    const auto span = helpers::nonNullPtrSpan(ptrs);
    BOOST_TEST_REQUIRE(span.size() == ptrs.size());
    BOOST_TEST_REQUIRE(initialSpan.size() == ptrs.size());
    for(auto it2 = span.begin(); it2 != span.end(); ++it2, ++it)
    {
        static_assert(std::is_same<decltype(*it2), TestClass&>::value, "!");
        BOOST_TEST((*it2).value == (*it)->value);
        BOOST_TEST(it2->value == (*it)->value);
    }

    it = ptrs.begin();
    for(const TestClass& i : helpers::nonNullPtrSpan(ptrs))
    {
        BOOST_TEST(i.value == (*it)->value);
        BOOST_TEST(&i == it->get());
        ++it;
    }

    it = ptrs.begin();
    for(const TestClass& i : initialSpan)
    {
        BOOST_TEST(i.value == (*it)->value);
        BOOST_TEST(&i == it->get());
        ++it;
    }
}
