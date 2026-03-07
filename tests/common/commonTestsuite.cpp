// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#define BOOST_TEST_MODULE RTTR_Test
#include <boost/test/unit_test.hpp>

#if RTTR_HAS_VLD
#    include <vld.h>
#endif

#include <helpers/StrongId.h>

BOOST_AUTO_TEST_SUITE(Helpers)

BOOST_AUTO_TEST_CASE(CheckStrongId)
{
    using MyId = helpers::StrongId<unsigned, struct MyIdTag>;
    MyId id;
    BOOST_TEST(!id.isValid());
    BOOST_TEST(!id);
    BOOST_TEST(static_cast<unsigned>(id) == 0);

    MyId id2 = MyId(5);
    BOOST_TEST(id2.isValid());
    BOOST_TEST(id2);
    BOOST_TEST(static_cast<unsigned>(id2) == 5);
    BOOST_TEST(id2 != id);
    id = id2;
    BOOST_TEST(id2 == id);

    BOOST_TEST(id2.next().value() == id2.value() + 1u);

    id.reset();
    BOOST_TEST(!id.isValid());
}

BOOST_AUTO_TEST_SUITE_END()