// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "lua/CampaignDataLoader.h"
#include "gameData/CampaignDescription.h"
#include "rttr/test/LocaleResetter.hpp"
#include "rttr/test/TmpFolder.hpp"
#include <s25util/utf8.h>
#include <boost/filesystem.hpp>
#include <boost/nowide/fstream.hpp>
#include <boost/test/unit_test.hpp>

namespace bnw = boost::nowide;

BOOST_AUTO_TEST_SUITE(CampaignLuaFile)

BOOST_AUTO_TEST_CASE(LoadCampaignDescriptionWithoutTranslation)
{
    rttr::test::TmpFolder tmp;
    {
        bnw::ofstream file(tmp.get() / "campaign.lua");

        file << "rttr:AddCampaign{\
            version = \"1\",\
            author = \"Max Meier\",\
            name = \"Meine Kampagne\",\
            shortDescription = \"Sehr kurze Beschreibung\",\
            longDescription = \"Das ist die lange Beschreibung\",\
            maxHumanPlayers = 1,\
            mapFolder = \"<RTTR_GAME>/DATA/MAPS\",\
            luaFolder = \"<RTTR_GAME>/CAMPAINGS/ROMAN\",\
            maps = { \"dessert0.WLD\", \"dessert1.WLD\", \"dessert2.WLD\"}\
        }";
    }

    CampaignDescription desc;
    CampaignDataLoader loader(desc, tmp.get());
    BOOST_TEST_REQUIRE(loader.Load());

    // campaign description
    BOOST_TEST(desc.version == "1");
    BOOST_TEST(desc.author == "Max Meier");
    BOOST_TEST(desc.name == "Meine Kampagne");
    BOOST_TEST(desc.shortDescription == "Sehr kurze Beschreibung");
    BOOST_TEST(desc.longDescription == "Das ist die lange Beschreibung");
    BOOST_TEST(desc.maxHumanPlayers == 1);
    BOOST_TEST(desc.mapFolder == "<RTTR_GAME>/DATA/MAPS");
    BOOST_TEST(desc.luaFolder == "<RTTR_GAME>/CAMPAINGS/ROMAN");

    // maps
    BOOST_TEST(desc.mapNames.size() == static_cast<size_t>(3));
    BOOST_TEST(desc.mapNames[0] == "dessert0.WLD");
    BOOST_TEST(desc.mapNames[1] == "dessert1.WLD");
    BOOST_TEST(desc.mapNames[2] == "dessert2.WLD");
}

BOOST_AUTO_TEST_CASE(CampaignDescriptionLoadWithTranslation)
{
    rttr::test::TmpFolder tmp;
    {
        bnw::ofstream file(tmp.get() / "campaign.lua");

        file << "rttr:RegisterTranslations(\
        {\
            en =\
            {\
                name = 'My campaign',\
                shortDescription = 'Very short description',\
                longDescription = 'That is the long description'\
            },\
            de =\
            {\
                name = 'Meine Kampagne',\
                shortDescription = 'Sehr kurze Beschreibung',\
                longDescription = 'Das ist die lange Beschreibung'\
            }\
        })";

        file << "rttr:AddCampaign{\
            version = \"1\",\
            author = \"Max Meier\",\
            name = _\"name\",\
            shortDescription = _\"shortDescription\",\
            longDescription = _\"longDescription\",\
            maxHumanPlayers = 1,\
            mapFolder = \"<RTTR_GAME>/DATA/MAPS\",\
            luaFolder = \"<RTTR_GAME>/CAMPAINGS/ROMAN\",\
            maps = { \"dessert0.WLD\", \"dessert1.WLD\", \"dessert2.WLD\"}\
        }";
    }

    rttr::test::LocaleResetter loc("de");

    CampaignDescription desc;
    CampaignDataLoader loader(desc, tmp.get());
    BOOST_TEST_REQUIRE(loader.Load());

    // campaign description
    BOOST_TEST(desc.version == "1");
    BOOST_TEST(desc.author == "Max Meier");
    BOOST_TEST(desc.name == "Meine Kampagne");
    BOOST_TEST(desc.shortDescription == "Sehr kurze Beschreibung");
    BOOST_TEST(desc.longDescription == "Das ist die lange Beschreibung");
    BOOST_TEST(desc.maxHumanPlayers == 1);
    BOOST_TEST(desc.mapFolder == "<RTTR_GAME>/DATA/MAPS");
    BOOST_TEST(desc.luaFolder == "<RTTR_GAME>/CAMPAINGS/ROMAN");

    // maps
    BOOST_TEST(desc.mapNames.size() == static_cast<size_t>(3));
    BOOST_TEST(desc.mapNames[0] == "dessert0.WLD");
    BOOST_TEST(desc.mapNames[1] == "dessert1.WLD");
    BOOST_TEST(desc.mapNames[2] == "dessert2.WLD");
}

BOOST_AUTO_TEST_SUITE_END()
