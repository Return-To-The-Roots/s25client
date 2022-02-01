// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "PointOutput.h"
#include "WindowManager.h"
#include "controls/ctrlButton.h"
#include "controls/ctrlComboBox.h"
#include "controls/ctrlPercent.h"
#include "controls/ctrlProgress.h"
#include "desktops/dskGameLobby.h"
#include "ingameWindows/iwConnecting.h"
#include "ingameWindows/iwHelp.h"
#include "ingameWindows/iwMapGenerator.h"
#include "ingameWindows/iwMsgbox.h"
#include "uiHelper/uiHelpers.hpp"
#include "gameTypes/GameTypesOutput.h"
#include "rttr/test/random.hpp"
#include <turtle/mock.hpp>
#include <boost/test/unit_test.hpp>

// LCOV_EXCL_START
BOOST_TEST_DONT_PRINT_LOG_VALUE(rttr::mapGenerator::MapStyle)
BOOST_TEST_DONT_PRINT_LOG_VALUE(rttr::mapGenerator::MountainDistance)
BOOST_TEST_DONT_PRINT_LOG_VALUE(rttr::mapGenerator::IslandAmount)
// LCOV_EXCL_STOP

BOOST_FIXTURE_TEST_SUITE(IngameWindows, uiHelper::Fixture)

BOOST_AUTO_TEST_CASE(MinimizeWindow)
{
    iwHelp wnd("Foo barFoo barFoo barFoo bar\n\n\n\nFoo\nFoo\nFoo\nFoo\nFoo\nFoo\nFoo\nFoo\nFoo\nFoo\nFoo\nFoo\n");
    const Extent oldSize = wnd.GetSize();
    BOOST_TEST_REQUIRE(oldSize.x > 50u);
    BOOST_TEST_REQUIRE(oldSize.y > 50u);
    // Window should reduce height (only)
    wnd.SetMinimized(true);
    BOOST_TEST_REQUIRE(wnd.GetSize().x == oldSize.x); //-V807
    BOOST_TEST_REQUIRE(wnd.GetSize().y > 0u);
    BOOST_TEST_REQUIRE(wnd.GetSize().y < oldSize.y);
    // And fully expand to old size
    wnd.SetMinimized(false);
    BOOST_TEST_REQUIRE(wnd.GetSize() == oldSize);
}

BOOST_AUTO_TEST_CASE(IwMapGenerator)
{
    const auto expectedNumPlayers = rttr::test::randomValue(2u, 7u);
    const auto expectedMapType = rttr::test::randomValue<uint8_t>(0, 2);
    const auto expectedGoldRatio = rttr::test::randomValue<unsigned short>(0, 100);
    const auto expectedIronRatio = rttr::test::randomValue<unsigned short>(0, 100);
    const auto expectedCoalRatio = rttr::test::randomValue<unsigned short>(0, 100);
    const auto expectedGraniteRatio = rttr::test::randomValue<unsigned short>(0, 100);
    const auto expectedRivers = rttr::test::randomValue<unsigned short>(0, 100);
    const auto expectedTrees = rttr::test::randomValue<unsigned short>(0, 100);
    const auto expectedStonePiles = rttr::test::randomValue<unsigned short>(0, 100);

    rttr::mapGenerator::MapSettings settings;
    iwMapGenerator wnd(settings);
    wnd.GetCtrl<ctrlComboBox>(iwMapGenerator::CTRL_PLAYER_NUMBER)->SetSelection(expectedNumPlayers - 2);
    wnd.GetCtrl<ctrlComboBox>(iwMapGenerator::CTRL_MAP_TYPE)->SetSelection(expectedMapType);
    wnd.GetCtrl<ctrlComboBox>(iwMapGenerator::CTRL_MAP_STYLE)->SetSelection(1);     // MapStyle::Land
    wnd.GetCtrl<ctrlComboBox>(iwMapGenerator::CTRL_MAP_SIZE)->SetSelection(4);      // 1024x1024
    wnd.GetCtrl<ctrlComboBox>(iwMapGenerator::CTRL_MOUNTAIN_DIST)->SetSelection(3); // VeryFar
    wnd.GetCtrl<ctrlComboBox>(iwMapGenerator::CTRL_ISLANDS)->SetSelection(2);       // IslandAmount::Many
    wnd.GetCtrl<ctrlProgress>(iwMapGenerator::CTRL_RATIO_GOLD)->SetPosition(expectedGoldRatio);
    wnd.GetCtrl<ctrlProgress>(iwMapGenerator::CTRL_RATIO_IRON)->SetPosition(expectedIronRatio);
    wnd.GetCtrl<ctrlProgress>(iwMapGenerator::CTRL_RATIO_COAL)->SetPosition(expectedCoalRatio);
    wnd.GetCtrl<ctrlProgress>(iwMapGenerator::CTRL_RATIO_GRANITE)->SetPosition(expectedGraniteRatio);
    wnd.GetCtrl<ctrlProgress>(iwMapGenerator::CTRL_RIVERS)->SetPosition(expectedRivers);
    wnd.GetCtrl<ctrlProgress>(iwMapGenerator::CTRL_TREES)->SetPosition(expectedTrees);
    wnd.GetCtrl<ctrlProgress>(iwMapGenerator::CTRL_STONE_PILES)->SetPosition(expectedStonePiles);
    wnd.Msg_ButtonClick(iwMapGenerator::CTRL_BTN_APPLY);

    BOOST_TEST(settings.numPlayers == expectedNumPlayers);
    BOOST_TEST(settings.type == DescIdx<LandscapeDesc>(expectedMapType));
    BOOST_TEST(settings.size == MapExtent(1024, 1024));
    BOOST_TEST(settings.style == rttr::mapGenerator::MapStyle::Land);
    BOOST_TEST(settings.mountainDistance == rttr::mapGenerator::MountainDistance::VeryFar);
    BOOST_TEST(settings.islands == rttr::mapGenerator::IslandAmount::Many);
    BOOST_TEST(settings.ratioGold == expectedGoldRatio);
    BOOST_TEST(settings.ratioIron == expectedIronRatio);
    BOOST_TEST(settings.ratioCoal == expectedCoalRatio);
    BOOST_TEST(settings.ratioGranite == expectedGraniteRatio);
    BOOST_TEST(settings.rivers == expectedRivers);
    BOOST_TEST(settings.trees == expectedTrees);
    BOOST_TEST(settings.stonePiles == expectedStonePiles);
}

BOOST_AUTO_TEST_CASE(ConnectingWindow)
{
    MOCK_FUNCTOR(onError, void(ClientError));
    {
        iwConnecting wnd(ServerType::Local, nullptr);
        ClientInterface& ci = wnd;
        boost::signals2::scoped_connection _ = wnd.onError.connect(onError);
        // Modal and doesn't react on right-click
        BOOST_TEST(wnd.IsModal());
        BOOST_TEST(!wnd.isUserClosable());
        ctrlPercent& progressBar = *wnd.GetCtrls<ctrlPercent>().at(0);
        BOOST_TEST(!progressBar.IsVisible()); // Initially hidden
        for(auto st :
            {ConnectState::Initiated, ConnectState::VerifyServer, ConnectState::QueryPw, ConnectState::QueryMapInfo})
        {
            ci.CI_NextConnectState(st);
            BOOST_TEST(!progressBar.IsVisible()); // Still hidden
        }
        ci.CI_NextConnectState(ConnectState::ReceiveMap);
        BOOST_TEST(progressBar.IsVisible()); // Shown
        BOOST_TEST(progressBar.getPercentage() == 0u);
        ci.CI_MapPartReceived(499, 1000);
        BOOST_TEST(progressBar.IsVisible());
        BOOST_TEST(progressBar.getPercentage() == 50u);
        ci.CI_MapPartReceived(1000, 1000);
        BOOST_TEST(progressBar.IsVisible());
        BOOST_TEST(progressBar.getPercentage() == 100u);
        for(auto st : {ConnectState::VerifyMap, ConnectState::QueryServerName, ConnectState::QueryPlayerList,
                       ConnectState::QuerySettings})
        {
            ci.CI_NextConnectState(st);
            BOOST_TEST(!progressBar.IsVisible()); // Hidden again
        }
        BOOST_TEST(!wnd.ShouldBeClosed());
        ci.CI_NextConnectState(ConnectState::Finished);
        BOOST_TEST(wnd.ShouldBeClosed());
        WINDOWMANAGER.Draw();
        BOOST_TEST((dynamic_cast<dskGameLobby*>(WINDOWMANAGER.GetCurrentDesktop()) != nullptr));
    }
    {
        iwConnecting wnd(ServerType::Local, nullptr);
        ClientInterface& ci = wnd;
        boost::signals2::scoped_connection _ = wnd.onError.connect(onError);
        // CI_ERROR sends the onError signal and closes the window
        MOCK_EXPECT(onError).once().with(ClientError::ServerFull);
        ci.CI_Error(ClientError::ServerFull);
        BOOST_TEST(wnd.ShouldBeClosed());
    }
    {
        iwConnecting wnd(ServerType::Local, nullptr);
        // Clicking the button sends the onError signal (or if not set opens an error window) and closes the window
        wnd.Msg_ButtonClick(wnd.GetCtrls<ctrlButton>().at(0)->GetID());
        BOOST_TEST(wnd.ShouldBeClosed());
        BOOST_TEST((dynamic_cast<iwMsgbox*>(WINDOWMANAGER.GetTopMostWindow())));
    }
}

BOOST_AUTO_TEST_SUITE_END()
