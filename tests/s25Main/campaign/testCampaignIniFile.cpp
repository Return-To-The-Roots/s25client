// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "CampaingSettings.h"
#include <boost/filesystem.hpp>
#include <boost/test/unit_test.hpp>

namespace bfs = boost::filesystem;

BOOST_AUTO_TEST_SUITE(CampaignIni)

struct CampaignIniFixture
{
    CampaignIniFixture()
    {
        tmpPath = bfs::absolute(bfs::unique_path());
        BOOST_TEST_CHECKPOINT("Creating campaign ini file");
        bfs::create_directories(tmpPath);
        campaignIniFileName = tmpPath.string() + "/campaign.ini";
        fixtureCampaignSettings = std::make_unique<CampaignSettings>(campaignIniFileName);

        // campaign description
        fixtureCampaignSettings->campaignDescription.author = "Max Müller";
        fixtureCampaignSettings->campaignDescription.name = "The journey";
        fixtureCampaignSettings->campaignDescription.shortDescription = "We are aground in the dessert";
        fixtureCampaignSettings->campaignDescription.longDescription =
          "We are in a large dessert with low amount of water and few space"
          " for farming. We need to hunt for wild and use our ore mines to produce gold and "
          "exchange it for goods we can not produce.";
        fixtureCampaignSettings->campaignDescription.maxPlayers = 1;

        // maps
        fixtureCampaignSettings->missions.mapNames.push_back("dessert0.WLD");
        fixtureCampaignSettings->missions.mapNames.push_back("dessert1.WLD");
        fixtureCampaignSettings->missions.mapNames.push_back("dessert2.WLD");
    }
    virtual ~CampaignIniFixture() { bfs::remove_all(tmpPath); }
    std::unique_ptr<CampaignSettings> fixtureCampaignSettings;
    std::string campaignIniFileName;
    bfs::path tmpPath;
};

CampaignIniFixture campaignIniFixture;
BOOST_TEST_GLOBAL_FIXTURE(CampaignIniFixture);

BOOST_AUTO_TEST_CASE(TestCampaingIniFileSave)
{
    campaignIniFixture.fixtureCampaignSettings->Save();
    BOOST_TEST_REQUIRE(bfs::exists(campaignIniFixture.campaignIniFileName));
}

BOOST_AUTO_TEST_CASE(TestCampaingIniFileLoad)
{
    CampaignSettings campaignSettings(campaignIniFixture.campaignIniFileName);
    BOOST_TEST_REQUIRE(campaignSettings.Load());

    // campaign description
    BOOST_TEST_REQUIRE(campaignSettings.campaignDescription.author
                       == campaignIniFixture.fixtureCampaignSettings->campaignDescription.author);
    BOOST_TEST_REQUIRE(campaignSettings.campaignDescription.name
                       == campaignIniFixture.fixtureCampaignSettings->campaignDescription.name);
    BOOST_TEST_REQUIRE(campaignSettings.campaignDescription.shortDescription
                       == campaignIniFixture.fixtureCampaignSettings->campaignDescription.shortDescription);
    BOOST_TEST_REQUIRE(campaignSettings.campaignDescription.longDescription
                       == campaignIniFixture.fixtureCampaignSettings->campaignDescription.longDescription);
    BOOST_TEST_REQUIRE(campaignSettings.campaignDescription.maxPlayers
                       == campaignIniFixture.fixtureCampaignSettings->campaignDescription.maxPlayers);

    // maps
    BOOST_TEST_REQUIRE(campaignSettings.missions.mapNames
                       == campaignIniFixture.fixtureCampaignSettings->missions.mapNames);
}

BOOST_AUTO_TEST_SUITE_END()
