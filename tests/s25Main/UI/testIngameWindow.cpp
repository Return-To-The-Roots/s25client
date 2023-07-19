// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "DrawPoint.h"
#include "Point.h"
#include "PointOutput.h"
#include "Settings.h"
#include "WindowManager.h"
#include "controls/ctrlButton.h"
#include "controls/ctrlComboBox.h"
#include "controls/ctrlEdit.h"
#include "controls/ctrlOptionGroup.h"
#include "controls/ctrlPercent.h"
#include "controls/ctrlProgress.h"
#include "desktops/dskGameLobby.h"
#include "helpers/format.hpp"
#include "ingameWindows/IngameWindow.h"
#include "ingameWindows/iwConnecting.h"
#include "ingameWindows/iwDirectIPConnect.h"
#include "ingameWindows/iwHelp.h"
#include "ingameWindows/iwMapGenerator.h"
#include "ingameWindows/iwMsgbox.h"
#include "uiHelper/uiHelpers.hpp"
#include "gameTypes/GameTypesOutput.h"
#include "gameData/const_gui_ids.h"
#include "rttr/test/random.hpp"
#include "s25util/StringConversion.h"
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
    const auto expectedGoldRatio = rttr::test::randomValue(0u, 100u);
    const auto expectedIronRatio = rttr::test::randomValue(0u, 100u);
    const auto expectedCoalRatio = rttr::test::randomValue(0u, 100u);
    const auto expectedGraniteRatio = rttr::test::randomValue(0u, 100u);
    const auto expectedRivers = rttr::test::randomValue(0u, 100u);
    const auto expectedTrees = rttr::test::randomValue(0u, 100u);
    const auto expectedStonePiles = rttr::test::randomValue(0u, 100u);

    rttr::mapGenerator::MapSettings settings;
    iwMapGenerator wnd(settings);
    // UI values are set according to current values. Check that and adjust to random value to test apply-button
    BOOST_TEST(wnd.GetCtrl<ctrlComboBox>(iwMapGenerator::ID_cbNumPlayers)->GetSelection().value()
               == settings.numPlayers - 2u);
    wnd.GetCtrl<ctrlComboBox>(iwMapGenerator::ID_cbNumPlayers)->SetSelection(expectedNumPlayers - 2);
    BOOST_TEST(wnd.GetCtrl<ctrlComboBox>(iwMapGenerator::ID_cbMapType)->GetSelection().value() == settings.type.value);
    wnd.GetCtrl<ctrlComboBox>(iwMapGenerator::ID_cbMapType)->SetSelection(expectedMapType);
    BOOST_TEST(wnd.GetCtrl<ctrlComboBox>(iwMapGenerator::ID_cbMapStyle)->GetSelection().value()
               == static_cast<unsigned>(settings.style));
    wnd.GetCtrl<ctrlComboBox>(iwMapGenerator::ID_cbMapStyle)->SetSelection(1); // MapStyle::Land
    BOOST_TEST(wnd.GetCtrl<ctrlComboBox>(iwMapGenerator::ID_cbMapSizeX)->GetSelectedText().value()
               == s25util::toStringClassic(settings.size.x));
    BOOST_TEST(wnd.GetCtrl<ctrlComboBox>(iwMapGenerator::ID_cbMapSizeY)->GetSelectedText().value()
               == s25util::toStringClassic(settings.size.y));
    wnd.GetCtrl<ctrlComboBox>(iwMapGenerator::ID_cbMapSizeX)->SetSelection(7);                              // 256
    wnd.GetCtrl<ctrlComboBox>(iwMapGenerator::ID_cbMapSizeY)->SetSelection(3);                              // 128
    BOOST_TEST(wnd.GetCtrl<ctrlComboBox>(iwMapGenerator::ID_cbMountainDist)->GetSelection().value() == 1u); // Normal
    wnd.GetCtrl<ctrlComboBox>(iwMapGenerator::ID_cbMountainDist)->SetSelection(3);                          // VeryFar
    BOOST_TEST(wnd.GetCtrl<ctrlComboBox>(iwMapGenerator::ID_cbIslands)->GetSelection().value() == 0u);      // Few
    wnd.GetCtrl<ctrlComboBox>(iwMapGenerator::ID_cbIslands)->SetSelection(2); // IslandAmount::Many
    BOOST_TEST(wnd.GetCtrl<ctrlProgress>(iwMapGenerator::ID_pgGoldRatio)->GetPosition() == settings.ratioGold);
    wnd.GetCtrl<ctrlProgress>(iwMapGenerator::ID_pgGoldRatio)->SetPosition(expectedGoldRatio);
    BOOST_TEST(wnd.GetCtrl<ctrlProgress>(iwMapGenerator::ID_pgIronRatio)->GetPosition() == settings.ratioIron);
    wnd.GetCtrl<ctrlProgress>(iwMapGenerator::ID_pgIronRatio)->SetPosition(expectedIronRatio);
    BOOST_TEST(wnd.GetCtrl<ctrlProgress>(iwMapGenerator::ID_pgCoalRatio)->GetPosition() == settings.ratioCoal);
    wnd.GetCtrl<ctrlProgress>(iwMapGenerator::ID_pgCoalRatio)->SetPosition(expectedCoalRatio);
    BOOST_TEST(wnd.GetCtrl<ctrlProgress>(iwMapGenerator::ID_pgGraniteRatio)->GetPosition() == settings.ratioGranite);
    wnd.GetCtrl<ctrlProgress>(iwMapGenerator::ID_pgGraniteRatio)->SetPosition(expectedGraniteRatio);
    BOOST_TEST(wnd.GetCtrl<ctrlProgress>(iwMapGenerator::ID_pgRivers)->GetPosition() == settings.rivers);
    wnd.GetCtrl<ctrlProgress>(iwMapGenerator::ID_pgRivers)->SetPosition(expectedRivers);
    BOOST_TEST(wnd.GetCtrl<ctrlProgress>(iwMapGenerator::ID_pgTrees)->GetPosition() == settings.trees);
    wnd.GetCtrl<ctrlProgress>(iwMapGenerator::ID_pgTrees)->SetPosition(expectedTrees);
    BOOST_TEST(wnd.GetCtrl<ctrlProgress>(iwMapGenerator::ID_pgStonePiles)->GetPosition() == settings.stonePiles);
    wnd.GetCtrl<ctrlProgress>(iwMapGenerator::ID_pgStonePiles)->SetPosition(expectedStonePiles);
    wnd.Msg_ButtonClick(iwMapGenerator::ID_btApply);
    BOOST_TEST(wnd.ShouldBeClosed());

    BOOST_TEST(settings.numPlayers == expectedNumPlayers);
    BOOST_TEST(settings.type == DescIdx<LandscapeDesc>(expectedMapType));
    BOOST_TEST(settings.size == MapExtent(256, 128));
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

BOOST_AUTO_TEST_CASE(ConnectWindow)
{
    iwDirectIPConnect wnd(ServerType::Local);
    const auto edts = wnd.GetCtrls<ctrlEdit>();
    // Should have (at least) 3 fields: IP, port, password (in this order)
    const auto& edtIp = *edts.at(0);
    const auto& edtPort = *edts.at(1);
    const auto& edtPw = *edts.at(2);
    // And an option group to choose IPv6 vs IPv4
    auto& ipGrp = *wnd.GetCtrls<ctrlOptionGroup>().at(0);
    const auto testHost = rttr::test::randString(10);
    const auto testPort = rttr::test::randomValue(10, 10000);
    const auto testIsIpv6 = rttr::test::randomBool();
    wnd.Connect(testHost, testPort, testIsIpv6,
                true); // When the server has a password, the window shouldn't initiate the connection process
    BOOST_TEST_REQUIRE(!wnd.ShouldBeClosed()); // No error or anything, window still open
    // Fields filled and password field has focus
    BOOST_TEST(edtIp.GetText() == testHost);
    BOOST_TEST(edtPort.GetText() == s25util::toStringClassic(testPort));
    BOOST_TEST(edtPw.GetText().empty());
    BOOST_TEST(edtPw.HasFocus());
    BOOST_TEST(ipGrp.GetSelection() == static_cast<unsigned>(testIsIpv6));
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
        BOOST_TEST(wnd.getCloseBehavior() == CloseBehavior::Custom);
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

BOOST_AUTO_TEST_CASE(SaveAndRestoreMinimized)
{
    constexpr auto id = CGI_MINIMAP;
    auto it = SETTINGS.windows.persistentSettings.find(id);
    BOOST_REQUIRE(it != SETTINGS.windows.persistentSettings.end());

    {
        it->second.isMinimized = false;

        IngameWindow wnd(id, IngameWindow::posLastOrCenter, Extent(100, 100), "Test Window", nullptr);
        BOOST_TEST(!wnd.IsMinimized());
        BOOST_TEST(wnd.GetSize() == Extent(100, 100));

        wnd.SetMinimized(true);
        BOOST_TEST(it->second.isMinimized);
    }

    {
        it->second.isMinimized = true;

        IngameWindow wnd(id, IngameWindow::posLastOrCenter, Extent(100, 100), "Test Window", nullptr);
        BOOST_TEST(wnd.IsMinimized());
        BOOST_TEST(wnd.GetSize() != Extent(100, 100));

        wnd.SetMinimized(false);
        BOOST_TEST(!it->second.isMinimized);
    }
}

BOOST_AUTO_TEST_SUITE_END()
