// Copyright (c) 2016 - 2019 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Loader.h"
#include "ogl/glFont.h"
#include "uiHelper/uiHelpers.hpp"
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(Font)

BOOST_FIXTURE_TEST_CASE(WrapInfoVaryingLen, uiHelper::Fixture)
{
    LOADER.initResourceFolders();
    BOOST_TEST(LOADER.LoadFonts());
    const auto& font = *SmallFont;
    std::string input = "a\naa\naaa\naaaa\naaaaa\naa";
    auto output = std::vector<std::string>{"a", "aa", "aaa", "aaaa", "aaaa", "a", "aa"};
    glFont::WrapInfo wrapInfo = font.GetWrapInfo(input, font.CharWidth('a') * 4, font.CharWidth('a') * 4);
    BOOST_TEST(wrapInfo.lines.size() == output.size());
    BOOST_TEST(wrapInfo.CreateSingleStrings(input) == output, boost::test_tools::per_element{});

    input = "a aa aaa aaaa aaaaa aa";
    output = std::vector<std::string>{"a aa", "aaa", "aaaa", "aaaa", "a aa"};
    wrapInfo = font.GetWrapInfo(input, font.CharWidth('a') * 4, font.CharWidth('a') * 4);
    BOOST_TEST(wrapInfo.lines.size() == output.size());
    BOOST_TEST(wrapInfo.CreateSingleStrings(input) == output, boost::test_tools::per_element{});
}

BOOST_FIXTURE_TEST_CASE(WrapInfoWithTrailingNewlines, uiHelper::Fixture)
{
    const std::string input = "a1\nb234\n\nc4124\n\n\n";
    const auto output = std::vector<std::string>{"a1", "b234", "", "c4124", "", "", ""};
    const glFont::WrapInfo wrapInfo = SmallFont->GetWrapInfo(input, 1000, 1000);
    BOOST_TEST(wrapInfo.lines.size() == output.size());
    BOOST_TEST(wrapInfo.CreateSingleStrings(input) == output, boost::test_tools::per_element{});
}

BOOST_AUTO_TEST_SUITE_END()
