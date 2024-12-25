// Copyright (C) 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "CampaignSettings.h"
#include "Settings.h"
#include "lua/CampaignDataLoader.h"
#include "gameData/CampaignDescription.h"
#include "rttr/test/TmpFolder.hpp"
#include <boost/nowide/fstream.hpp>
#include <boost/test/unit_test.hpp>

namespace {
struct CampaignSettingsFixture
{
    CampaignDescription desc;
    CampaignSettings sut;
};
} // namespace

BOOST_FIXTURE_TEST_CASE(CampaignSettingsIntegrationTest, CampaignSettingsFixture)
{
    constexpr auto cmpgn1 = "roman";
    constexpr auto cmpgn2 = "world";
    rttr::test::TmpFolder tmp;
    {
        boost::nowide::ofstream file(tmp / "campaign.lua");
        // read first campaign - chaptersEnabled not specified
        file << R"(campaign = {
                version = "1",
                uid = "roman",
                author = "Max Meier",
                name = "My campaign",
                shortDescription = "Very short description",
                longDescription = "This is the long description",
                image = "<RTTR_GAME>/GFX/PICS/WORLD.LBM",
                maxHumanPlayers = 1,
                difficulty = "easy",
                mapFolder = "<RTTR_GAME>/DATA/MAPS",
                luaFolder = "<RTTR_GAME>/CAMPAIGNS/ROMAN",
                maps = { "dessert0.WLD", "dessert1.WLD", "dessert2.WLD"}
            }
            )";
        file << "function getRequiredLuaVersion() return 1 end";
    }
    BOOST_TEST_REQUIRE((CampaignDataLoader{desc, tmp}.Load()));

    // chapters 1 and 2 are playable by default
    BOOST_TEST_REQUIRE(sut.isChapterPlayable(desc, 0) == true);
    BOOST_TEST_REQUIRE(sut.isChapterPlayable(desc, 1) == true);
    BOOST_TEST_REQUIRE(sut.isChapterPlayable(desc, 2) == false);
    BOOST_TEST_REQUIRE(sut.isChapterPlayable(desc, 9) == false);

    // complete ch2 and enable ch3
    sut.setChapterCompleted(cmpgn1, 1);
    sut.enableChapter(cmpgn1, 2);
    auto ms = sut.getMissionsStatus(desc);
    BOOST_TEST_REQUIRE(ms[0].playable == true);
    BOOST_TEST_REQUIRE(ms[0].conquered == false);
    BOOST_TEST_REQUIRE(ms[1].playable == true);
    BOOST_TEST_REQUIRE(ms[1].conquered == true);
    BOOST_TEST_REQUIRE(ms[2].playable == true);
    BOOST_TEST_REQUIRE(ms[2].conquered == false);

    // read save data - ch1 completed, ch2 enabled, ch3 disabled
    sut.readSaveData(cmpgn1, "210");
    ms = sut.getMissionsStatus(desc);
    BOOST_TEST_REQUIRE(ms[0].playable == true);
    BOOST_TEST_REQUIRE(ms[0].conquered == true);
    BOOST_TEST_REQUIRE(ms[1].playable == true);
    BOOST_TEST_REQUIRE(ms[1].conquered == false);
    BOOST_TEST_REQUIRE(ms[2].playable == false);
    BOOST_TEST_REQUIRE(ms[2].conquered == false);

    // enable ch3
    sut.enableChapter(cmpgn1, 2);
    // save code should now be 211
    decltype(sut.createSaveData()) expectedSaveData;
    expectedSaveData[cmpgn1] = "211";
    BOOST_TEST_REQUIRE(sut.createSaveData() == expectedSaveData);

    // read and save data for campaign 2
    sut.readSaveData(cmpgn2, "21010");
    expectedSaveData[cmpgn2] = "21010";
    BOOST_TEST_REQUIRE(sut.createSaveData() == expectedSaveData);

    // read second campaign - chaptersEnabled specified
    {
        boost::nowide::ofstream file(tmp / "campaign.lua");
        file.clear();
        file << R"(campaign = {
                version = "1",
                uid = "world",
                author = "Max Meier",
                name = "My campaign",
                shortDescription = "Very short description",
                longDescription = "This is the long description",
                image = "<RTTR_GAME>/GFX/PICS/WORLD.LBM",
                maxHumanPlayers = 1,
                difficulty = "easy",
                mapFolder = "<RTTR_GAME>/DATA/MAPS",
                luaFolder = "<RTTR_GAME>/CAMPAIGNS/ROMAN",
                maps = { "dessert0.WLD", "dessert1.WLD", "dessert2.WLD"},
                chaptersEnabled = {1, 2, 4}
            }
            )";
        file << "function getRequiredLuaVersion() return 1 end";
    }
    BOOST_TEST_REQUIRE((CampaignDataLoader{desc, tmp}.Load()));

    BOOST_TEST_REQUIRE(sut.isChapterPlayable(desc, 0) == true);
    BOOST_TEST_REQUIRE(sut.isChapterPlayable(desc, 1) == true);
    BOOST_TEST_REQUIRE(sut.isChapterPlayable(desc, 2) == false);
    BOOST_TEST_REQUIRE(sut.isChapterPlayable(desc, 3) == true);

    sut.enableChapter(cmpgn2, 2);
    BOOST_TEST_REQUIRE(sut.isChapterPlayable(desc, 2) == true);
}
