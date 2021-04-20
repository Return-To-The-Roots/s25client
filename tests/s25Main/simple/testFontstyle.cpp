// Copyright (c) 2019 - 2019 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ogl/FontStyle.h"
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(TestFontStyle)

BOOST_AUTO_TEST_CASE(DefaultConstructed)
{
    FontStyle style;
    BOOST_TEST(style.is(FontStyle::LEFT));
    BOOST_TEST(!style.is(FontStyle::CENTER));
    BOOST_TEST(!style.is(FontStyle::RIGHT));

    BOOST_TEST(style.is(FontStyle::TOP));
    BOOST_TEST(!style.is(FontStyle::VCENTER));
    BOOST_TEST(!style.is(FontStyle::BOTTOM));

    BOOST_TEST(style.is(FontStyle::OUTLINE));
    BOOST_TEST(!style.is(FontStyle::NO_OUTLINE));
}

BOOST_AUTO_TEST_CASE(ValueConstructed)
{
    {
        FontStyle style = FontStyle::CENTER;
        BOOST_TEST(style.is(FontStyle::CENTER));
        BOOST_TEST(style.is(FontStyle::TOP));
        BOOST_TEST(style.is(FontStyle::OUTLINE));
    }
    {
        FontStyle style = FontStyle::VCENTER;
        BOOST_TEST(style.is(FontStyle::LEFT));
        BOOST_TEST(style.is(FontStyle::VCENTER));
        BOOST_TEST(style.is(FontStyle::OUTLINE));
    }
    {
        FontStyle style = FontStyle::NO_OUTLINE;
        BOOST_TEST(style.is(FontStyle::LEFT));
        BOOST_TEST(style.is(FontStyle::TOP));
        BOOST_TEST(style.is(FontStyle::NO_OUTLINE));
    }
}

BOOST_AUTO_TEST_CASE(Combineable)
{
    {
        FontStyle style = FontStyle::CENTER | FontStyle::TOP | FontStyle::NO_OUTLINE;
        BOOST_TEST(!style.is(FontStyle::LEFT));
        BOOST_TEST(style.is(FontStyle::CENTER));
        BOOST_TEST(!style.is(FontStyle::RIGHT));

        BOOST_TEST(style.is(FontStyle::TOP));
        BOOST_TEST(!style.is(FontStyle::VCENTER));
        BOOST_TEST(!style.is(FontStyle::BOTTOM));

        BOOST_TEST(!style.is(FontStyle::OUTLINE));
        BOOST_TEST(style.is(FontStyle::NO_OUTLINE));
    }
    {
        FontStyle style = FontStyle::CENTER | FontStyle::TOP;
        style = style | FontStyle::RIGHT; // Replace
        BOOST_TEST(!style.is(FontStyle::LEFT));
        BOOST_TEST(!style.is(FontStyle::CENTER));
        BOOST_TEST(style.is(FontStyle::RIGHT));

        BOOST_TEST(style.is(FontStyle::TOP));
        BOOST_TEST(!style.is(FontStyle::VCENTER));
        BOOST_TEST(!style.is(FontStyle::BOTTOM));

        BOOST_TEST(style.is(FontStyle::OUTLINE));
        BOOST_TEST(!style.is(FontStyle::NO_OUTLINE));
    }
}

BOOST_AUTO_TEST_CASE(ConstexprUseable)
{
    constexpr FontStyle style = FontStyle::RIGHT | FontStyle::BOTTOM;
    static_assert(style.is(FontStyle::RIGHT), "!");
    static_assert(style.is(FontStyle::BOTTOM), "!");
    BOOST_TEST(style.is(FontStyle::OUTLINE));
}

BOOST_AUTO_TEST_SUITE_END()
