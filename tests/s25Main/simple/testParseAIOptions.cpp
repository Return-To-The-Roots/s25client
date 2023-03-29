// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "QuickStartGame.h"
#include "enum_cast.hpp"
#include <boost/test/unit_test.hpp>

// LCOV_EXCL_START
namespace boost { namespace test_tools { namespace tt_detail {
    template<>
    struct print_log_value<AI::Type>
    {
        void operator()(std::ostream& os, const AI::Type type) { os << static_cast<unsigned>(rttr::enum_cast(type)); }
    };
}}} // namespace boost::test_tools::tt_detail
// LCOV_EXCL_STOP

BOOST_AUTO_TEST_SUITE(ParseAIOptionsTests)

BOOST_AUTO_TEST_CASE(EmptyOptions)
{
    std::vector<std::string> options;
    std::vector<AI::Info> aiInfos = ParseAIOptions(options);
    BOOST_TEST(aiInfos.empty());
}

BOOST_AUTO_TEST_CASE(ParsingAIJH)
{
    std::vector<std::string> options = {"aijh", "AIJH", "AiJh"};
    std::vector<AI::Info> aiInfos = ParseAIOptions(options);
    BOOST_TEST_REQUIRE(aiInfos.size() == 3u);
    BOOST_TEST(aiInfos[0].type == AI::Type::Default);
    BOOST_TEST(aiInfos[1].type == AI::Type::Default);
    BOOST_TEST(aiInfos[2].type == AI::Type::Default);
}

BOOST_AUTO_TEST_CASE(ParsingDummy)
{
    std::vector<std::string> options = {"dummy", "Dummy", "DUMMY"};
    std::vector<AI::Info> aiInfos = ParseAIOptions(options);
    BOOST_TEST_REQUIRE(aiInfos.size() == 3u);
    BOOST_TEST(aiInfos[0].type == AI::Type::Dummy);
    BOOST_TEST(aiInfos[1].type == AI::Type::Dummy);
    BOOST_TEST(aiInfos[2].type == AI::Type::Dummy);
}

BOOST_AUTO_TEST_CASE(ParsingMultiple)
{
    std::vector<std::string> options = {"dummy", "aijh", "dummy", "aijh"};
    std::vector<AI::Info> aiInfos = ParseAIOptions(options);
    BOOST_TEST_REQUIRE(aiInfos.size() == 4u);
    BOOST_TEST(aiInfos[0].type == AI::Type::Dummy);
    BOOST_TEST(aiInfos[1].type == AI::Type::Default);
    BOOST_TEST(aiInfos[2].type == AI::Type::Dummy);
    BOOST_TEST(aiInfos[3].type == AI::Type::Default);
}

BOOST_AUTO_TEST_CASE(ParsingFailed)
{
    std::vector<std::string> options = {"invalid"};
    BOOST_CHECK_THROW(ParseAIOptions(options), std::invalid_argument);
}

BOOST_AUTO_TEST_SUITE_END()
