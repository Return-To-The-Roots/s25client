// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "DrawPoint.h"
#include "Loader.h"
#include "Point.h"
#include "PointOutput.h"
#include "Settings.h"
#include "WindowManager.h"
#include "controls/ctrlButton.h"
#include "controls/ctrlCheck.h"
#include "controls/ctrlComboBox.h"
#include "controls/ctrlEdit.h"
#include "controls/ctrlOptionGroup.h"
#include "controls/ctrlPercent.h"
#include "controls/ctrlProgress.h"
#include "desktops/dskGameLobby.h"
#include "drivers/VideoDriverWrapper.h"
#include "helpers/format.hpp"
#include "ingameWindows/IngameWindow.h"
#include "ingameWindows/iwConnecting.h"
#include "ingameWindows/iwDirectIPConnect.h"
#include "ingameWindows/iwHelp.h"
#include "ingameWindows/iwMapGenerator.h"
#include "ingameWindows/iwMsgbox.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "uiHelper/uiHelpers.hpp"
#include "gameTypes/GameTypesOutput.h"
#include "gameData/const_gui_ids.h"
#include "rttr/test/random.hpp"
#include "s25util/StringConversion.h"
#include <turtle/mock.hpp>
#include <boost/test/unit_test.hpp>
#include <functional>

// LCOV_EXCL_START
BOOST_TEST_DONT_PRINT_LOG_VALUE(rttr::mapGenerator::MapStyle)
BOOST_TEST_DONT_PRINT_LOG_VALUE(rttr::mapGenerator::MountainDistance)
BOOST_TEST_DONT_PRINT_LOG_VALUE(rttr::mapGenerator::IslandAmount)
// LCOV_EXCL_STOP

BOOST_FIXTURE_TEST_SUITE(IngameWindows, uiHelper::Fixture)

static MouseCoords makeLeftDown(const Position pos)
{
    MouseCoords mc(pos);
    mc.ldown = true;
    return mc;
}

static MouseCoords makeLeftDblClick(const Position pos)
{
    MouseCoords mc(pos);
    mc.dbl_click = true;
    return mc;
}

BOOST_AUTO_TEST_CASE(MinimizeWindow)
{
    iwHelp wnd("Foo barFoo barFoo barFoo bar\n\n\n\nFoo\nFoo\nFoo\nFoo\nFoo\nFoo\nFoo\nFoo\nFoo\nFoo\nFoo\nFoo\n");
    const Extent oldSize = wnd.getSize();
    BOOST_TEST_REQUIRE(oldSize.x > 50u);
    BOOST_TEST_REQUIRE(oldSize.y > 50u);
    // Window should reduce height (only)
    wnd.setMinimized(true);
    BOOST_TEST_REQUIRE(wnd.getSize().x == oldSize.x); //-V807
    BOOST_TEST_REQUIRE(wnd.getSize().y > 0u);
    BOOST_TEST_REQUIRE(wnd.getSize().y < oldSize.y);
    // And fully expand to old size
    wnd.setMinimized(false);
    BOOST_TEST_REQUIRE(wnd.getSize() == oldSize);
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
    const auto expectedMountains = rttr::test::randomValue(0u, 100u);
    const auto expectedDesert = rttr::test::randomValue(0u, 100u);
    const auto expectedSwamp = rttr::test::randomValue(0u, 100u);
    const auto expectedStonePiles = rttr::test::randomValue(0u, 100u);
    const bool expectedSpecialObjects = rttr::test::randomValue(0u, 1u) == 1u;

    rttr::mapGenerator::MapSettings settings;
    iwMapGenerator wnd(settings);
    // UI values are set according to current values. Check that and adjust to random value to test apply-button
    BOOST_TEST(wnd.getCtrl<ctrlComboBox>(iwMapGenerator::ID_cbNumPlayers)->getSelection().value()
               == settings.numPlayers - 2u);
    wnd.getCtrl<ctrlComboBox>(iwMapGenerator::ID_cbNumPlayers)->setSelection(expectedNumPlayers - 2);
    BOOST_TEST(wnd.getCtrl<ctrlComboBox>(iwMapGenerator::ID_cbMapType)->getSelection().value() == settings.type.value);
    wnd.getCtrl<ctrlComboBox>(iwMapGenerator::ID_cbMapType)->setSelection(expectedMapType);
    BOOST_TEST(wnd.getCtrl<ctrlComboBox>(iwMapGenerator::ID_cbMapStyle)->getSelection().value()
               == static_cast<unsigned>(settings.style));
    wnd.getCtrl<ctrlComboBox>(iwMapGenerator::ID_cbMapStyle)->setSelection(1); // MapStyle::Land
    BOOST_TEST(wnd.getCtrl<ctrlComboBox>(iwMapGenerator::ID_cbMapSizeX)->getSelectedText().value()
               == s25util::toStringClassic(settings.size.x));
    BOOST_TEST(wnd.getCtrl<ctrlComboBox>(iwMapGenerator::ID_cbMapSizeY)->getSelectedText().value()
               == s25util::toStringClassic(settings.size.y));
    wnd.getCtrl<ctrlComboBox>(iwMapGenerator::ID_cbMapSizeX)->setSelection(7);                              // 256
    wnd.getCtrl<ctrlComboBox>(iwMapGenerator::ID_cbMapSizeY)->setSelection(3);                              // 128
    BOOST_TEST(wnd.getCtrl<ctrlComboBox>(iwMapGenerator::ID_cbMountainDist)->getSelection().value() == 1u); // Normal
    wnd.getCtrl<ctrlComboBox>(iwMapGenerator::ID_cbMountainDist)->setSelection(3);                          // VeryFar
    BOOST_TEST(wnd.getCtrl<ctrlComboBox>(iwMapGenerator::ID_cbIslands)->getSelection().value() == 0u);      // Few
    wnd.getCtrl<ctrlComboBox>(iwMapGenerator::ID_cbIslands)->setSelection(2); // IslandAmount::Many
    BOOST_TEST(wnd.getCtrl<ctrlProgress>(iwMapGenerator::ID_pgGoldRatio)->getPosition() == settings.ratioGold);
    wnd.getCtrl<ctrlProgress>(iwMapGenerator::ID_pgGoldRatio)->setPosition(expectedGoldRatio);
    BOOST_TEST(wnd.getCtrl<ctrlProgress>(iwMapGenerator::ID_pgIronRatio)->getPosition() == settings.ratioIron);
    wnd.getCtrl<ctrlProgress>(iwMapGenerator::ID_pgIronRatio)->setPosition(expectedIronRatio);
    BOOST_TEST(wnd.getCtrl<ctrlProgress>(iwMapGenerator::ID_pgCoalRatio)->getPosition() == settings.ratioCoal);
    wnd.getCtrl<ctrlProgress>(iwMapGenerator::ID_pgCoalRatio)->setPosition(expectedCoalRatio);
    BOOST_TEST(wnd.getCtrl<ctrlProgress>(iwMapGenerator::ID_pgGraniteRatio)->getPosition() == settings.ratioGranite);
    wnd.getCtrl<ctrlProgress>(iwMapGenerator::ID_pgGraniteRatio)->setPosition(expectedGraniteRatio);
    BOOST_TEST(wnd.getCtrl<ctrlProgress>(iwMapGenerator::ID_pgRivers)->getPosition() == settings.rivers);
    wnd.getCtrl<ctrlProgress>(iwMapGenerator::ID_pgRivers)->setPosition(expectedRivers);
    BOOST_TEST(wnd.getCtrl<ctrlProgress>(iwMapGenerator::ID_pgTrees)->getPosition() == settings.trees);
    wnd.getCtrl<ctrlProgress>(iwMapGenerator::ID_pgTrees)->setPosition(expectedTrees);
    BOOST_TEST(wnd.getCtrl<ctrlProgress>(iwMapGenerator::ID_pgMountains)->getPosition() == settings.mountains);
    wnd.getCtrl<ctrlProgress>(iwMapGenerator::ID_pgMountains)->setPosition(expectedMountains);
    BOOST_TEST(wnd.getCtrl<ctrlProgress>(iwMapGenerator::ID_pgDesert)->getPosition() == settings.desert);
    wnd.getCtrl<ctrlProgress>(iwMapGenerator::ID_pgDesert)->setPosition(expectedDesert);
    BOOST_TEST(wnd.getCtrl<ctrlProgress>(iwMapGenerator::ID_pgSwamp)->getPosition() == settings.swamp);
    wnd.getCtrl<ctrlProgress>(iwMapGenerator::ID_pgSwamp)->setPosition(expectedSwamp);
    BOOST_TEST(wnd.getCtrl<ctrlProgress>(iwMapGenerator::ID_pgStonePiles)->getPosition() == settings.stonePiles);
    wnd.getCtrl<ctrlProgress>(iwMapGenerator::ID_pgStonePiles)->setPosition(expectedStonePiles);
    BOOST_TEST(wnd.getCtrl<ctrlCheck>(iwMapGenerator::ID_cbSpecialObjects)->isChecked() == settings.specialObjects);
    wnd.getCtrl<ctrlCheck>(iwMapGenerator::ID_cbSpecialObjects)->setChecked(expectedSpecialObjects);
    wnd.msgButtonClick(iwMapGenerator::ID_btApply);
    BOOST_TEST(wnd.shouldBeClosed());

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
    BOOST_TEST(settings.mountains == expectedMountains);
    BOOST_TEST(settings.desert == expectedDesert);
    BOOST_TEST(settings.swamp == expectedSwamp);
    BOOST_TEST(settings.stonePiles == expectedStonePiles);
    BOOST_TEST(settings.specialObjects == expectedSpecialObjects);
}

BOOST_AUTO_TEST_CASE(ConnectWindow)
{
    iwDirectIPConnect wnd(ServerType::Local);
    const auto edts = wnd.getCtrls<ctrlEdit>();
    // Should have (at least) 3 fields: IP, port, password (in this order)
    const auto& edtIp = *edts.at(0);
    const auto& edtPort = *edts.at(1);
    const auto& edtPw = *edts.at(2);
    // And an option group to choose IPv6 vs IPv4
    auto& ipGrp = *wnd.getCtrls<ctrlOptionGroup>().at(0);
    const auto testHost = rttr::test::randString(10);
    const auto testPort = rttr::test::randomValue(10, 10000);
    const auto testIsIpv6 = rttr::test::randomBool();
    wnd.connect(testHost, testPort, testIsIpv6,
                true); // When the server has a password, the window shouldn't initiate the connection process
    BOOST_TEST_REQUIRE(!wnd.shouldBeClosed()); // No error or anything, window still open
    // Fields filled and password field has focus
    BOOST_TEST(edtIp.getText() == testHost);
    BOOST_TEST(edtPort.getText() == s25util::toStringClassic(testPort));
    BOOST_TEST(edtPw.getText().empty());
    BOOST_TEST(edtPw.hasFocus());
    BOOST_TEST(ipGrp.getSelection() == static_cast<unsigned>(testIsIpv6));
}

BOOST_AUTO_TEST_CASE(ConnectingWindow)
{
    MOCK_FUNCTOR(onError, void(ClientError));
    {
        iwConnecting wnd(ServerType::Local, nullptr);
        ClientInterface& ci = wnd;
        boost::signals2::scoped_connection _ = wnd.onError.connect(onError);
        // Modal and doesn't react on right-click
        BOOST_TEST(wnd.isModal());
        BOOST_TEST(wnd.getCloseBehavior() == CloseBehavior::Custom);
        ctrlPercent& progressBar = *wnd.getCtrls<ctrlPercent>().at(0);
        BOOST_TEST(!progressBar.isVisible()); // Initially hidden
        for(auto st :
            {ConnectState::Initiated, ConnectState::VerifyServer, ConnectState::QueryPw, ConnectState::QueryMapInfo})
        {
            ci.CI_NextConnectState(st);
            BOOST_TEST(!progressBar.isVisible()); // Still hidden
        }
        ci.CI_NextConnectState(ConnectState::ReceiveMap);
        BOOST_TEST(progressBar.isVisible()); // Shown
        BOOST_TEST(progressBar.getPercentage() == 0u);
        ci.CI_MapPartReceived(499, 1000);
        BOOST_TEST(progressBar.isVisible());
        BOOST_TEST(progressBar.getPercentage() == 50u);
        ci.CI_MapPartReceived(1000, 1000);
        BOOST_TEST(progressBar.isVisible());
        BOOST_TEST(progressBar.getPercentage() == 100u);
        for(auto st : {ConnectState::VerifyMap, ConnectState::QueryServerName, ConnectState::QueryPlayerList,
                       ConnectState::QuerySettings})
        {
            ci.CI_NextConnectState(st);
            BOOST_TEST(!progressBar.isVisible()); // Hidden again
        }
        BOOST_TEST(!wnd.shouldBeClosed());
        ci.CI_NextConnectState(ConnectState::Finished);
        BOOST_TEST(wnd.shouldBeClosed());
        WINDOWMANAGER.draw();
        BOOST_TEST((dynamic_cast<dskGameLobby*>(WINDOWMANAGER.getCurrentDesktop()) != nullptr));
    }
    {
        iwConnecting wnd(ServerType::Local, nullptr);
        ClientInterface& ci = wnd;
        boost::signals2::scoped_connection _ = wnd.onError.connect(onError);
        // CI_ERROR sends the onError signal and closes the window
        MOCK_EXPECT(onError).once().with(ClientError::ServerFull);
        ci.CI_Error(ClientError::ServerFull);
        BOOST_TEST(wnd.shouldBeClosed());
    }
    {
        iwConnecting wnd(ServerType::Local, nullptr);
        // Clicking the button sends the onError signal (or if not set opens an error window) and closes the window
        wnd.msgButtonClick(wnd.getCtrls<ctrlButton>().at(0)->getID());
        BOOST_TEST(wnd.shouldBeClosed());
        BOOST_TEST((dynamic_cast<iwMsgbox*>(WINDOWMANAGER.getTopMostWindow())));
    }
}

BOOST_AUTO_TEST_CASE(SaveAndRestoreMinimized)
{
    constexpr auto id = CGI_MINIMAP;
    auto it = SETTINGS.windows.persistentSettings.find(id);
    BOOST_REQUIRE(it != SETTINGS.windows.persistentSettings.end());
    auto& settings = it->second;

    {
        settings.isMinimized = false;

        IngameWindow wnd(id, IngameWindow::posLastOrCenter, Extent(100, 100), "Test Window", nullptr);
        BOOST_TEST(!wnd.isMinimized());
        BOOST_TEST(wnd.getSize() == Extent(100, 100));

        wnd.setMinimized(true);
        BOOST_TEST(settings.isMinimized);
    }

    {
        settings.isMinimized = true;

        IngameWindow wnd(id, IngameWindow::posLastOrCenter, Extent(100, 100), "Test Window", nullptr);
        BOOST_TEST(wnd.isMinimized());
        BOOST_TEST(wnd.getSize() != Extent(100, 100));

        wnd.setMinimized(false);
        BOOST_TEST(!settings.isMinimized);
    }
}

BOOST_AUTO_TEST_CASE(SaveAndRestorePinned)
{
    constexpr auto id = CGI_MINIMAP;
    auto it = SETTINGS.windows.persistentSettings.find(id);
    BOOST_REQUIRE(it != SETTINGS.windows.persistentSettings.end());
    auto& settings = it->second;

    {
        settings.isPinned = false;

        IngameWindow wnd(id, IngameWindow::posLastOrCenter, Extent(100, 100), "Test Window", nullptr);
        BOOST_TEST(!wnd.isPinned());
        BOOST_TEST(wnd.getSize() == Extent(100, 100));

        wnd.setPinned();
        BOOST_TEST(settings.isPinned);
    }

    {
        settings.isPinned = true;

        IngameWindow wnd(id, IngameWindow::posLastOrCenter, Extent(100, 100), "Test Window", nullptr);
        BOOST_TEST(wnd.isPinned());

        wnd.setPinned(false);
        BOOST_TEST(!settings.isPinned);
    }
}

BOOST_AUTO_TEST_CASE(TitleBarButtons)
{
    constexpr auto id = CGI_MINIMAP;
    auto it = SETTINGS.windows.persistentSettings.find(id);
    BOOST_REQUIRE(it != SETTINGS.windows.persistentSettings.end());
    auto& settings = it->second;

    BOOST_TEST_CONTEXT("Window pinning disabled")
    {
        SETTINGS.interface.enableWindowPinning = false;
        settings.isMinimized = false;
        settings.isPinned = false;
        BOOST_TEST_CONTEXT("Left title bar button closes window")
        {
            IngameWindow wnd(id, IngameWindow::posLastOrCenter, Extent(100, 100), "Test Window", nullptr);
            const MouseCoords evLDown = makeLeftDown(wnd.getPos() + DrawPoint(4, 4));
            const MouseCoords evLDownInsideWindow = makeLeftDown(wnd.getPos() + wnd.getSize() / 2);
            const MouseCoords evLUp = MouseCoords(evLDown.pos);

            BOOST_TEST(!wnd.shouldBeClosed());
            BOOST_TEST(!wnd.isMinimized());
            BOOST_TEST(!wnd.isPinned());

            // Only react when clicking the button not when moving the mouse to the button
            // while holding left mouse button
            wnd.msgLeftDown(evLDownInsideWindow);
            wnd.msgMouseMove(evLDown);
            wnd.msgLeftUp(evLUp);
            BOOST_TEST(!wnd.shouldBeClosed());
            BOOST_TEST(!wnd.isMinimized());
            BOOST_TEST(!wnd.isPinned());
            // Similar when moving out of the button
            wnd.msgLeftDown(evLDown);
            wnd.msgMouseMove(evLDownInsideWindow);
            wnd.msgMouseMove(evLDown);
            wnd.msgLeftUp(evLUp);
            BOOST_TEST(!wnd.shouldBeClosed());
            BOOST_TEST(!wnd.isMinimized());
            BOOST_TEST(!wnd.isPinned());

            wnd.msgLeftDown(evLDown);
            wnd.msgLeftUp(evLUp);
            BOOST_TEST(wnd.shouldBeClosed());
            BOOST_TEST(!wnd.isMinimized());
            BOOST_TEST(!wnd.isPinned());
            BOOST_TEST(!settings.isOpen);
            BOOST_TEST(!settings.isMinimized);
            BOOST_TEST(!settings.isPinned);
        }

        BOOST_TEST_CONTEXT("Double-click on the window title has no effect")
        {
            IngameWindow wnd(id, IngameWindow::posLastOrCenter, Extent(100, 100), "Test Window", nullptr);
            const MouseCoords evLDown = makeLeftDown(wnd.getPos() + DrawPoint(wnd.getSize().x / 2, 4));
            const MouseCoords evLDblClick = makeLeftDblClick(evLDown.pos);

            BOOST_TEST(!wnd.shouldBeClosed());
            BOOST_TEST(!wnd.isMinimized());
            BOOST_TEST(!wnd.isPinned());
            wnd.msgLeftDown(evLDown);
            wnd.msgLeftUp(evLDblClick);
            BOOST_TEST(!wnd.shouldBeClosed());
            BOOST_TEST(!wnd.isMinimized());
            BOOST_TEST(!wnd.isPinned());
            BOOST_TEST(settings.isOpen);
            BOOST_TEST(!settings.isMinimized);
            BOOST_TEST(!settings.isPinned);
        }

        settings.isMinimized = false;
        BOOST_TEST_CONTEXT("Right title bar button minimizes window")
        {
            IngameWindow wnd(id, IngameWindow::posLastOrCenter, Extent(100, 100), "Test Window", nullptr);
            const MouseCoords evLDown = makeLeftDown(wnd.getPos() + DrawPoint(wnd.getSize().x, 0) + DrawPoint(-4, 4));
            const MouseCoords evLUp = MouseCoords(evLDown.pos);

            BOOST_TEST(!wnd.shouldBeClosed());
            BOOST_TEST(!wnd.isMinimized());
            BOOST_TEST(!wnd.isPinned());
            wnd.msgLeftDown(evLDown);
            wnd.msgLeftUp(evLUp);
            BOOST_TEST(!wnd.shouldBeClosed());
            BOOST_TEST(wnd.isMinimized());
            BOOST_TEST(!wnd.isPinned());
            BOOST_TEST(settings.isOpen);
            BOOST_TEST(settings.isMinimized);
            BOOST_TEST(!settings.isPinned);
        }
    }

    BOOST_TEST_CONTEXT("Window pinning enabled")
    {
        SETTINGS.interface.enableWindowPinning = true;
        settings.isMinimized = false;
        settings.isPinned = false;
        BOOST_TEST_CONTEXT("Left title bar button closes window")
        {
            IngameWindow wnd(id, IngameWindow::posLastOrCenter, Extent(100, 100), "Test Window", nullptr);
            const MouseCoords evLDown = makeLeftDown(wnd.getPos() + DrawPoint(4, 4));
            const MouseCoords evLUp = MouseCoords(evLDown.pos);

            BOOST_TEST(!wnd.shouldBeClosed());
            BOOST_TEST(!wnd.isMinimized());
            BOOST_TEST(!wnd.isPinned());
            wnd.msgLeftDown(evLDown);
            wnd.msgLeftUp(evLUp);
            BOOST_TEST(wnd.shouldBeClosed());
            BOOST_TEST(!wnd.isMinimized());
            BOOST_TEST(!wnd.isPinned());
            BOOST_TEST(!settings.isOpen);
            BOOST_TEST(!settings.isMinimized);
            BOOST_TEST(!settings.isPinned);
        }

        BOOST_TEST_CONTEXT("Double-click on the window title minimizes")
        {
            IngameWindow wnd(id, IngameWindow::posLastOrCenter, Extent(100, 100), "Test Window", nullptr);
            const MouseCoords evLDown = makeLeftDown(wnd.getPos() + DrawPoint(wnd.getSize().x / 2, 4));
            const MouseCoords evLDblClick = makeLeftDblClick(evLDown.pos);

            BOOST_TEST(!wnd.shouldBeClosed());
            BOOST_TEST(!wnd.isMinimized());
            BOOST_TEST(!wnd.isPinned());
            wnd.msgLeftDown(evLDown);
            wnd.msgLeftUp(evLDblClick);
            BOOST_TEST(!wnd.shouldBeClosed());
            BOOST_TEST(wnd.isMinimized());
            BOOST_TEST(!wnd.isPinned());
            BOOST_TEST(settings.isOpen);
            BOOST_TEST(settings.isMinimized);
            BOOST_TEST(!settings.isPinned);
        }

        settings.isMinimized = false;
        settings.isPinned = false;
        BOOST_TEST_CONTEXT("Right title bar button pins window")
        {
            IngameWindow wnd(id, IngameWindow::posLastOrCenter, Extent(100, 100), "Test Window", nullptr);
            const MouseCoords evLDown = makeLeftDown(wnd.getPos() + DrawPoint(wnd.getSize().x, 0) + DrawPoint(-4, 4));
            const MouseCoords evLUp = MouseCoords(evLDown.pos);

            BOOST_TEST(!wnd.shouldBeClosed());
            BOOST_TEST(!wnd.isMinimized());
            BOOST_TEST(!wnd.isPinned());
            wnd.msgLeftDown(evLDown);
            wnd.msgLeftUp(evLUp);
            BOOST_TEST(!wnd.shouldBeClosed());
            BOOST_TEST(!wnd.isMinimized());
            BOOST_TEST(wnd.isPinned());
            BOOST_TEST(settings.isOpen);
            BOOST_TEST(!settings.isMinimized);
            BOOST_TEST(settings.isPinned);
        }
    }
}

namespace {
void windowPositioningTestOne(IngameWindow& wnd, const char* context, const std::function<void()>& checkNormal,
                              const std::function<void()>& checkMinimized)
{
    BOOST_TEST_CONTEXT(context)
    {
        BOOST_TEST_CONTEXT("Before minimize/un-minimize") checkNormal();

        wnd.setMinimized(true);

        BOOST_TEST_CONTEXT("Minimized") checkMinimized();

        wnd.setMinimized(false);

        BOOST_TEST_CONTEXT("After minimize/un-minimize") checkNormal();
    }
}
} // namespace

BOOST_AUTO_TEST_CASE(WindowPositioning)
{
    VIDEODRIVER.resizeScreen(VideoMode(800, 600), DisplayMode::Windowed);

    const auto renderSize = VIDEODRIVER.getRenderSize();

    constexpr auto idPersisted = CGI_MINIMAP;
    constexpr auto idNonPersisted = CGI_OBSERVATION;
    constexpr auto wndSizeS = Extent(50, 50);
    constexpr auto wndSizeM = Extent(90, 90);
    constexpr auto wndSizeL = Extent(200, 200);
    constexpr auto offset = DrawPoint(100, 100);

    auto it = SETTINGS.windows.persistentSettings.find(idNonPersisted);
    BOOST_REQUIRE(it == SETTINGS.windows.persistentSettings.end());

    it = SETTINGS.windows.persistentSettings.find(idPersisted);
    BOOST_REQUIRE(it != SETTINGS.windows.persistentSettings.end());
    auto& settings = it->second;

    // Calculate minimized height
    const auto minHeight = LOADER.getImageN("resource", 42)->getHeight()    // title bar
                           + LOADER.getImageN("resource", 40)->getHeight(); // bottom bar

    BOOST_TEST_CONTEXT("Persisted window, fresh settings, posLastOrCenter")
    {
        settings = PersistentWindowSettings{};

        IngameWindow wnd(idPersisted, IngameWindow::posLastOrCenter, wndSizeM, "Test Window", nullptr);

        windowPositioningTestOne(
          wnd, "Window should be centered",
          [&]() {
              BOOST_TEST(wnd.getPos() == (renderSize / 2 - wndSizeM / 2));
              BOOST_TEST(wnd.getPos() == settings.lastPos);
              BOOST_TEST(wnd.getPos() == settings.restorePos);
              BOOST_TEST(wnd.getSize() == wndSizeM);
          },
          [&]() {
              BOOST_TEST(wnd.getPos() == settings.lastPos);
              BOOST_TEST(wnd.getPos() == settings.restorePos);
              BOOST_TEST(wnd.getPos() == (renderSize / 2 - wndSizeM / 2));
              BOOST_TEST(wnd.getSize() == Extent(wndSizeM.x, minHeight));
          });

        const auto restorePos = renderSize - offset; // new position is also the restorePos
        windowPositioningTestOne(
          wnd, "Move window into bottom right corner, not connecting with the screen edges",
          [&]() {
              wnd.setPos(restorePos);
              BOOST_TEST(wnd.getPos() == restorePos);
              BOOST_TEST(wnd.getPos() == settings.lastPos);
              BOOST_TEST(wnd.getPos() == settings.restorePos);
              BOOST_TEST(wnd.getSize() == wndSizeM);
          },
          [&]() {
              BOOST_TEST(wnd.getPos() == restorePos);
              BOOST_TEST(wnd.getPos() == settings.lastPos);
              BOOST_TEST(wnd.getPos() == settings.restorePos);
              BOOST_TEST(wnd.getSize() == Extent(wndSizeM.x, minHeight));
          });

        windowPositioningTestOne(
          wnd, "Increase size, window connects with screen edges and should move",
          [&]() {
              wnd.resize(wndSizeL);
              BOOST_TEST(wnd.getPos() == DrawPoint(renderSize - wndSizeL));
              BOOST_TEST(wnd.getPos() == settings.lastPos);
              BOOST_TEST(restorePos == settings.restorePos);
              BOOST_TEST(wnd.getSize() == wndSizeL);
          },
          [&]() {
              BOOST_TEST(wnd.getPos() == renderSize - DrawPoint(wndSizeL.x, minHeight));
              BOOST_TEST(wnd.getPos() == settings.lastPos);
              BOOST_TEST(restorePos == settings.restorePos);
              BOOST_TEST(wnd.getSize() == Extent(wndSizeL.x, minHeight));
          });

        windowPositioningTestOne(
          wnd, "Decrease size, window no longer connects with screen edges and should move to restorePos",
          [&]() {
              wnd.resize(wndSizeS);
              BOOST_TEST(wnd.getPos() == restorePos);
              BOOST_TEST(wnd.getPos() == settings.lastPos);
              BOOST_TEST(restorePos == settings.restorePos);
              BOOST_TEST(wnd.getSize() == wndSizeS);
          },
          [&]() {
              BOOST_TEST(wnd.getPos() == restorePos);
              BOOST_TEST(wnd.getPos() == settings.lastPos);
              BOOST_TEST(restorePos == settings.restorePos);
              BOOST_TEST(wnd.getSize() == Extent(wndSizeS.x, minHeight));
          });
    }

    BOOST_TEST_CONTEXT("Non-persisted window, posAtMouse")
    {
        // the offset subtracted from the window position for posAtMouse
        constexpr auto cursorOffset = DrawPoint(-20, wndSizeM.y / 2);
        const auto restorePos = renderSize - offset; // initial window position is also the restorePos

        VIDEODRIVER.setMousePos(restorePos + cursorOffset);
        BOOST_REQUIRE(VIDEODRIVER.getMousePos() == (restorePos + cursorOffset));

        IngameWindow wnd(idNonPersisted, IngameWindow::posAtMouse, wndSizeM, "Test Window", nullptr);

        windowPositioningTestOne(
          wnd, "Window should be at cursor",
          [&]() {
              BOOST_TEST(wnd.getPos() == restorePos);
              BOOST_TEST(wnd.getSize() == wndSizeM);
          },
          [&]() {
              BOOST_TEST(wnd.getPos() == restorePos);
              BOOST_TEST(wnd.getSize() == Extent(wndSizeM.x, minHeight));
          });

        windowPositioningTestOne(
          wnd, "Increase size, window connects with screen edges and should move",
          [&]() {
              wnd.resize(wndSizeL);
              BOOST_TEST(wnd.getPos() == DrawPoint(renderSize - wndSizeL));
              BOOST_TEST(wnd.getSize() == wndSizeL);
          },
          [&]() {
              BOOST_TEST(wnd.getPos() == renderSize - DrawPoint(wndSizeL.x, minHeight));
              BOOST_TEST(wnd.getSize() == Extent(wndSizeL.x, minHeight));
          });

        windowPositioningTestOne(
          wnd, "Decrease size, window no longer connects with screen edges and should move to restorePos",
          [&]() {
              wnd.resize(wndSizeS);
              BOOST_TEST(wnd.getPos() == restorePos);
              BOOST_TEST(wnd.getSize() == wndSizeS);
          },
          [&]() {
              BOOST_TEST(wnd.getPos() == restorePos);
              BOOST_TEST(wnd.getSize() == Extent(wndSizeS.x, minHeight));
          });
    }
}

BOOST_AUTO_TEST_CASE(WindowSnapping)
{
    SETTINGS.interface.windowSnapDistance = 10;

    VIDEODRIVER.resizeScreen(VideoMode(800, 600), DisplayMode::Windowed);

    auto& wnd1 = static_cast<IngameWindow&>(WINDOWMANAGER.show(
      std::make_unique<IngameWindow>(0, DrawPoint(0, 500), Extent(100, 100), "Test Window 1", nullptr)));
    auto& wnd2 = static_cast<IngameWindow&>(WINDOWMANAGER.show(
      std::make_unique<IngameWindow>(0, DrawPoint(200, 480), Extent(100, 100), "Test Window 2", nullptr)));

    {
        const DrawPoint mousePosRel = DrawPoint(wnd2.getSize().x / 2, 4);
        const MouseCoords evLDown = makeLeftDown(wnd2.getPos() + mousePosRel);
        wnd2.msgLeftDown(evLDown);

        {
            const auto mc = makeLeftDown(evLDown.pos + DrawPoint(-80, 0));
            VIDEODRIVER.setMousePos(mc.pos);
            wnd2.msgMouseMove(mc);
            BOOST_TEST(wnd2.getPos() == DrawPoint(120, 480)); // Not in snap range yet
            BOOST_TEST(VIDEODRIVER.getMousePos() == (wnd2.getPos() + mousePosRel));
        }

        {
            const auto mc = makeLeftDown(evLDown.pos + DrawPoint(-90, 0));
            VIDEODRIVER.setMousePos(mc.pos);
            wnd2.msgMouseMove(mc);
            BOOST_TEST(wnd2.getPos() == DrawPoint(100, 480)); // In snap range
            BOOST_TEST(VIDEODRIVER.getMousePos() == mc.pos);
            // Cursor position is off by snap offset
            BOOST_TEST((VIDEODRIVER.getMousePos() - (wnd2.getPos() + mousePosRel)) == DrawPoint(10, 0));
        }

        {
            const auto mc = makeLeftDown(evLDown.pos + DrawPoint(-90, 30));
            // Reset mouse position to ensure it's properly updated in msgMouseMove()
            VIDEODRIVER.setMousePos(Position(0, 0));
            wnd2.msgMouseMove(mc);
            BOOST_TEST(wnd2.getPos() == DrawPoint(100, 500)); // Still in snap range
            BOOST_TEST(VIDEODRIVER.getMousePos() == Position(mc.pos.x, wnd2.getPos().y + mousePosRel.y));
            // Cursor position is off by snap offset
            BOOST_TEST((VIDEODRIVER.getMousePos() - (wnd2.getPos() + mousePosRel)) == DrawPoint(10, 0));
        }
    }

    BOOST_TEST_CONTEXT("Snap to the closest window")
    {
        auto& wnd3 = static_cast<IngameWindow&>(WINDOWMANAGER.show(
          std::make_unique<IngameWindow>(0, DrawPoint(200, 0), Extent(100, 100), "Test Window 3", nullptr)));

        const DrawPoint mousePosRel = DrawPoint(wnd3.getSize().x / 2, 4);
        const MouseCoords evLDown = makeLeftDown(wnd3.getPos() + mousePosRel);
        wnd3.msgLeftDown(evLDown);

        {
            wnd1.setPos(DrawPoint(0, 0));
            wnd2.setPos(DrawPoint(5, 0));
            const auto mc = makeLeftDown(evLDown.pos + DrawPoint(-90, 0));
            VIDEODRIVER.setMousePos(mc.pos);
            wnd3.msgMouseMove(mc);
            // In snap range of wnd1 and wnd2, but closest to wnd2
            BOOST_TEST(wnd3.getPos() == DrawPoint(wnd2.getPos().x + wnd2.getSize().x, 0));
            BOOST_TEST(VIDEODRIVER.getMousePos() == mc.pos);
            // Cursor position is off by snap offset
            BOOST_TEST((VIDEODRIVER.getMousePos() - (wnd3.getPos() + mousePosRel)) == DrawPoint(5, 0));
        }

        {
            wnd1.setPos(DrawPoint(5, 0));
            wnd2.setPos(DrawPoint(0, 0));
            const auto mc = makeLeftDown(evLDown.pos + DrawPoint(-90, 0));
            VIDEODRIVER.setMousePos(mc.pos);
            wnd3.msgMouseMove(mc);
            // In snap range of wnd1 and wnd2, but closest to wnd1
            BOOST_TEST(wnd3.getPos() == DrawPoint(wnd1.getPos().x + wnd1.getSize().x, 0));
            BOOST_TEST(VIDEODRIVER.getMousePos() == mc.pos);
            // Cursor position is off by snap offset
            BOOST_TEST((VIDEODRIVER.getMousePos() - (wnd3.getPos() + mousePosRel)) == DrawPoint(5, 0));
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
