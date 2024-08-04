// Copyright (C) 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "CampaignSaveData.h"
#include "Settings.h"
#include "lua/CampaignDataLoader.h"
#include "gameData/CampaignDescription.h"
#include "gameData/CampaignSaveCodes.h"
#include "rttr/test/TmpFolder.hpp"
#include <boost/nowide/fstream.hpp>
#include <boost/test/unit_test.hpp>

namespace {
constexpr auto uid = "uniqueID";
struct CampaignSaveDataFixture
{
    CampaignSaveDataFixture()
    {
        desc.uid = uid;
        saveData.clear();
    }

    CampaignDescription desc;
    std::map<std::string, std::string>& saveData = SETTINGS.campaigns.saveData;
};
} // namespace

BOOST_FIXTURE_TEST_CASE(isChapterEnabled_ReturnsFalse_WhenChapterIsDisabled, CampaignSaveDataFixture)
{
    BOOST_TEST_REQUIRE(isChapterEnabled(desc, 0) == false);
    BOOST_TEST_REQUIRE(isChapterEnabled(desc, 1) == false);
    BOOST_TEST_REQUIRE(isChapterEnabled(desc, 2) == false);
    BOOST_TEST_REQUIRE(isChapterEnabled(desc, 9) == false);
}

BOOST_FIXTURE_TEST_CASE(isChapterEnabled_ReturnsTrue_WhenChapterIsEnabled, CampaignSaveDataFixture)
{
    saveData[uid] = "011";
    BOOST_TEST_REQUIRE(isChapterEnabled(desc, 0) == false);
    BOOST_TEST_REQUIRE(isChapterEnabled(desc, 1) == true);
    BOOST_TEST_REQUIRE(isChapterEnabled(desc, 2) == true);
    BOOST_TEST_REQUIRE(isChapterEnabled(desc, 9) == false);
}

BOOST_FIXTURE_TEST_CASE(Chapters0and1AreEnabledByDefault, CampaignSaveDataFixture)
{
    saveData[uid] = CampaignSaveCodes::defaultChaptersEnabled;
    BOOST_TEST_REQUIRE(isChapterEnabled(desc, 0) == true);
    BOOST_TEST_REQUIRE(isChapterEnabled(desc, 1) == true);
    BOOST_TEST_REQUIRE(isChapterEnabled(desc, 2) == false);
    BOOST_TEST_REQUIRE(isChapterEnabled(desc, 9) == false);
}

BOOST_FIXTURE_TEST_CASE(DefaultChaptersCanBeSetPerCampaign, CampaignSaveDataFixture)
{
    desc.defaultChaptersEnabled = "100";
    BOOST_TEST_REQUIRE(isChapterEnabled(desc, 0) == true);
    BOOST_TEST_REQUIRE(isChapterEnabled(desc, 1) == false);
    BOOST_TEST_REQUIRE(isChapterEnabled(desc, 2) == false);
    BOOST_TEST_REQUIRE(isChapterEnabled(desc, 9) == false);
}

BOOST_FIXTURE_TEST_CASE(
  getMissionsStatus_ReturnsPlayableForChaptersWhichAreEnabled_AndConqueredForChaptersWhichAreCompleted,
  CampaignSaveDataFixture)
{
    rttr::test::TmpFolder tmp;
    {
        boost::nowide::ofstream file(tmp / "campaign.lua");

        file << "campaign ={\
            version = \"1\",\
            uid = \"roman\",\
            author = \"Max Meier\",\
            name = \"Meine Kampagne\",\
            shortDescription = \"Sehr kurze Beschreibung\",\
            longDescription = \"Das ist die lange Beschreibung\",\
            image = \"<RTTR_GAME>/GFX/PICS/WORLD.LBM\",\
            maxHumanPlayers = 1,\
            difficulty = \"easy\",\
            mapFolder = \"<RTTR_GAME>/DATA/MAPS\",\
            luaFolder = \"<RTTR_GAME>/CAMPAIGNS/ROMAN\",\
            maps = { \"dessert0.WLD\", \"dessert1.WLD\", \"dessert2.WLD\"}\
        }";

        file << "function getRequiredLuaVersion() return 1 end";
    }

    CampaignDescription dsc;
    CampaignDataLoader loader{dsc, tmp};
    BOOST_TEST_REQUIRE(loader.Load());

    saveData["roman"] = "210";
    const auto& ms = getMissionsStatus(dsc);
    BOOST_TEST_REQUIRE(ms[0].playable == true);
    BOOST_TEST_REQUIRE(ms[0].conquered == true);
    BOOST_TEST_REQUIRE(ms[1].playable == true);
    BOOST_TEST_REQUIRE(ms[1].conquered == false);
    BOOST_TEST_REQUIRE(ms[2].playable == false);
    BOOST_TEST_REQUIRE(ms[2].conquered == false);
}
