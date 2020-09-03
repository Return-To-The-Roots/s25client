// Copyright (c) 2016 - 2019 Settlers Freaks (sf-team at siedler25.org)
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
