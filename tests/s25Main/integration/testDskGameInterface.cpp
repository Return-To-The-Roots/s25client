// Copyright (C) 2005 - 2025 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "GamePlayer.h"
#include "PointOutput.h"
#include "Settings.h"
#include "WindowManager.h"
#include "buildings/nobBaseWarehouse.h"
#include "desktops/dskGameInterface.h"
#include "driver/KeyEvent.h"
#include "driver/MouseCoords.h"
#include "mockupDrivers/MockupVideoDriver.h"
#include "uiHelper/uiHelpers.hpp"
#include "worldFixtures/CreateEmptyWorld.h"
#include "worldFixtures/WorldFixture.h"
#include "gameData/GuiConsts.h"
#include "rttr/test/random.hpp"
#include <boost/test/unit_test.hpp>

// LCOV_EXCL_START
static std::ostream& operator<<(std::ostream& os, const Cursor& cursor)
{
    return os << static_cast<unsigned>(cursor);
}
// LCOV_EXCL_STOP

// Test stuff related to building/building quality
BOOST_AUTO_TEST_SUITE(GameInterfaceDesktop)

namespace {

struct dskGameInterfaceMock : public dskGameInterface
{
    dskGameInterfaceMock(std::shared_ptr<Game> game)
        : dskGameInterface(std::move(game), std::shared_ptr<NWFInfo>(), 0, false)
    {}
    void msgPaintBefore() override {}
    void msgPaintAfter() override {}
    using dskGameInterface::actionwindow;
    using dskGameInterface::msgKeyDown;
    using dskGameInterface::msgWheelDown;
    using dskGameInterface::msgWheelUp;
};
struct GameInterfaceFixture : uiHelper::Fixture
{
    WorldFixture<CreateEmptyWorld, 1> worldFixture;
    dskGameInterfaceMock* gameDesktop;
    const GameWorldView* view;
    GameInterfaceFixture()
    {
        gameDesktop = static_cast<dskGameInterfaceMock*>(
          WINDOWMANAGER.switchTo(std::make_unique<dskGameInterfaceMock>(worldFixture.game)));
        WINDOWMANAGER.draw();
        view = &gameDesktop->getView();
    }
};
void checkNotScrolling(const GameWorldView& view, Cursor cursor = Cursor::Hand)
{
    const DrawPoint pos = view.getOffset();
    MouseCoords mouse(Position(40, 11));
    WINDOWMANAGER.msgMouseMove(mouse);
    BOOST_TEST_REQUIRE(WINDOWMANAGER.getCursor() == cursor);
    BOOST_TEST_REQUIRE(view.getOffset() == pos);
    mouse.pos += Position(-20, 30);
    WINDOWMANAGER.msgMouseMove(mouse);
    BOOST_TEST_REQUIRE(WINDOWMANAGER.getCursor() == cursor);
    BOOST_TEST_REQUIRE(view.getOffset() == pos);
}
} // namespace

BOOST_FIXTURE_TEST_CASE(Scrolling, GameInterfaceFixture)
{
    const int acceleration = 2;
    SETTINGS.interface.mapScrollMode = MapScrollMode::ScrollOpposite;

    Position startPos(10, 15);
    MouseCoords mouse(startPos);
    mouse.rdown = true;

    // Regular scrolling: Right down, 2 moves, right up
    {
        WINDOWMANAGER.msgRightDown(mouse);
        BOOST_TEST_REQUIRE(WINDOWMANAGER.getCursor() == Cursor::Scroll);
        DrawPoint pos = view->getOffset();
        mouse.pos = startPos + Position(4, 3);
        WINDOWMANAGER.msgMouseMove(mouse);
        pos += acceleration * Position(4, 3);
        BOOST_TEST_REQUIRE(WINDOWMANAGER.getCursor() == Cursor::Scroll);
        BOOST_TEST_REQUIRE(view->getOffset() == pos);
        mouse.pos = startPos + Position(-6, 7);
        WINDOWMANAGER.msgMouseMove(mouse);
        pos += acceleration * Position(-6, 7);
        BOOST_TEST_REQUIRE(WINDOWMANAGER.getCursor() == Cursor::Scroll);
        BOOST_TEST_REQUIRE(view->getOffset() == pos);
        mouse.rdown = false;
        WINDOWMANAGER.msgRightUp(mouse);
        BOOST_TEST_REQUIRE(WINDOWMANAGER.getCursor() == Cursor::Hand);
        BOOST_TEST_REQUIRE(view->getOffset() == pos);
        checkNotScrolling(*view);
    }

    {
        SETTINGS.interface.mapScrollMode = MapScrollMode::ScrollSame;
        WINDOWMANAGER.msgRightDown(mouse);
        startPos = mouse.pos;
        BOOST_TEST_REQUIRE(WINDOWMANAGER.getCursor() == Cursor::Scroll);
        const DrawPoint pos = view->getOffset();
        mouse.pos = startPos + Position(4, 3);
        WINDOWMANAGER.msgMouseMove(mouse);
        BOOST_TEST_REQUIRE(WINDOWMANAGER.getCursor() == Cursor::Scroll);
        BOOST_TEST_REQUIRE(view->getOffset() == pos - acceleration * Position(4, 3));
        mouse.rdown = false;
        WINDOWMANAGER.msgRightUp(mouse);
        SETTINGS.interface.mapScrollMode = MapScrollMode::ScrollOpposite;
    }

    {
        SETTINGS.interface.mapScrollMode = MapScrollMode::GrabAndDrag;
        WINDOWMANAGER.msgRightDown(mouse);
        startPos = mouse.pos;
        BOOST_TEST_REQUIRE(WINDOWMANAGER.getCursor() == Cursor::Scroll);
        const DrawPoint pos = view->getOffset();
        mouse.pos = startPos + Position(4, 3);
        WINDOWMANAGER.msgMouseMove(mouse);
        BOOST_TEST_REQUIRE(WINDOWMANAGER.getCursor() == Cursor::Scroll);
        BOOST_TEST_REQUIRE(view->getOffset() == pos - Position(4, 3));
        mouse.rdown = false;
        WINDOWMANAGER.msgRightUp(mouse);
        SETTINGS.interface.mapScrollMode = MapScrollMode::ScrollOpposite;
    }

    // Opening a window does not cancel scrolling
    {
        mouse.rdown = true;
        WINDOWMANAGER.msgRightDown(mouse);
        startPos = mouse.pos;
        DrawPoint pos = view->getOffset();
        KeyEvent key;
        key.kt = KeyType::Char;
        key.c = 'm';
        key.ctrl = key.alt = key.shift = false;
        WINDOWMANAGER.msgKeyDown(key);
        BOOST_TEST_REQUIRE(WINDOWMANAGER.getTopMostWindow());
        BOOST_TEST_REQUIRE(WINDOWMANAGER.getCursor() == Cursor::Scroll);
        mouse.pos = startPos + Position(-6, 7);
        WINDOWMANAGER.msgMouseMove(mouse);
        pos += acceleration * Position(-6, 7);
        BOOST_TEST_REQUIRE(WINDOWMANAGER.getCursor() == Cursor::Scroll);
        BOOST_TEST_REQUIRE(view->getOffset() == pos);
        // Closing it doesn't either
        WINDOWMANAGER.msgKeyDown(key);
        WINDOWMANAGER.draw();
        BOOST_TEST_REQUIRE(gameDesktop->isActive());
        BOOST_TEST_REQUIRE(!WINDOWMANAGER.getTopMostWindow());
        BOOST_TEST_REQUIRE(WINDOWMANAGER.getCursor() == Cursor::Scroll);
        mouse.pos = startPos + Position(-6, 7);
        WINDOWMANAGER.msgMouseMove(mouse);
        pos += acceleration * Position(-6, 7);
        BOOST_TEST_REQUIRE(WINDOWMANAGER.getCursor() == Cursor::Scroll);
        BOOST_TEST_REQUIRE(view->getOffset() == pos);
        // Left click does cancel it
        mouse.ldown = true;
        WINDOWMANAGER.msgLeftDown(mouse);
        mouse.ldown = false;
        WINDOWMANAGER.msgLeftUp(mouse);
        BOOST_TEST_REQUIRE(WINDOWMANAGER.getCursor() == Cursor::Hand);
        BOOST_TEST_REQUIRE(view->getOffset() == pos);
        checkNotScrolling(*view);
    }
}

BOOST_FIXTURE_TEST_CASE(ScrollingWhileRoadBuilding, GameInterfaceFixture)
{
    const int acceleration = 2;
    MapPoint hqPos = worldFixture.world.getPlayer(0).getFirstWH()->getFlagPos(); //-V522
    gameDesktop->GI_StartRoadBuilding(hqPos, false);
    BOOST_TEST_REQUIRE(WINDOWMANAGER.getCursor() == Cursor::remove);
    Position startPos(10, 15);
    MouseCoords mouse(startPos);
    mouse.rdown = true;
    // Regular scrolling
    WINDOWMANAGER.msgRightDown(mouse);
    BOOST_TEST_REQUIRE(WINDOWMANAGER.getCursor() == Cursor::Scroll);
    DrawPoint pos = view->getOffset();
    mouse.pos = startPos + Position(4, 3);
    WINDOWMANAGER.msgMouseMove(mouse);
    pos += acceleration * Position(4, 3);
    BOOST_TEST_REQUIRE(WINDOWMANAGER.getCursor() == Cursor::Scroll);
    BOOST_TEST_REQUIRE(view->getOffset() == pos);
    mouse.rdown = false;
    WINDOWMANAGER.msgRightUp(mouse);
    BOOST_TEST_REQUIRE(WINDOWMANAGER.getCursor() == Cursor::remove);
    BOOST_TEST_REQUIRE(view->getOffset() == pos);
    checkNotScrolling(*view, Cursor::remove);

    // left click also stops scrolling
    mouse.rdown = true;
    WINDOWMANAGER.msgRightDown(mouse);
    startPos = mouse.pos;
    BOOST_TEST_REQUIRE(WINDOWMANAGER.getCursor() == Cursor::Scroll);
    mouse.pos = startPos + Position(-6, 7);
    WINDOWMANAGER.msgMouseMove(mouse);
    pos += acceleration * Position(-6, 7);
    BOOST_TEST_REQUIRE(WINDOWMANAGER.getCursor() == Cursor::Scroll);
    BOOST_TEST_REQUIRE(view->getOffset() == pos);
    mouse.ldown = true;
    WINDOWMANAGER.msgLeftDown(mouse);
    mouse.ldown = false;
    WINDOWMANAGER.msgLeftUp(mouse);
    BOOST_TEST_REQUIRE(WINDOWMANAGER.getCursor() == Cursor::remove);
    BOOST_TEST_REQUIRE(view->getOffset() == pos);
    checkNotScrolling(*view, Cursor::remove);
}

BOOST_FIXTURE_TEST_CASE(ScrollingWithCtrl, GameInterfaceFixture)
{
    const int acceleration = 2;
    Position startPos(10, 15);
    MouseCoords mouse(startPos);
    mouse.ldown = true;
    uiHelper::getVideoDriver()->modKeyState_.ctrl = true;
    WINDOWMANAGER.msgLeftDown(mouse);
    BOOST_TEST_REQUIRE(WINDOWMANAGER.getCursor() == Cursor::Scroll);
    DrawPoint pos = view->getOffset();
    mouse.pos = startPos + Position(4, 3);
    WINDOWMANAGER.msgMouseMove(mouse);
    pos += acceleration * Position(4, 3);
    BOOST_TEST_REQUIRE(WINDOWMANAGER.getCursor() == Cursor::Scroll);
    BOOST_TEST_REQUIRE(view->getOffset() == pos);
    mouse.ldown = false;
    WINDOWMANAGER.msgLeftUp(mouse);
    BOOST_TEST_REQUIRE(WINDOWMANAGER.getCursor() == Cursor::Hand);
    BOOST_TEST_REQUIRE(view->getOffset() == pos);
    checkNotScrolling(*view);
}

BOOST_FIXTURE_TEST_CASE(IwActionClose, GameInterfaceFixture)
{
    gameDesktop->showActionWindow(iwAction::Tabs{}, MapPoint(0, 1), DrawPoint(42, 37), false);
    WINDOWMANAGER.draw();
    BOOST_TEST_REQUIRE(WINDOWMANAGER.getTopMostWindow());
    BOOST_TEST_REQUIRE(WINDOWMANAGER.getTopMostWindow() == gameDesktop->actionwindow);
    WINDOWMANAGER.getTopMostWindow()->close();
    WINDOWMANAGER.draw();
    BOOST_TEST_REQUIRE(WINDOWMANAGER.getTopMostWindow() == nullptr);
    BOOST_TEST_REQUIRE(gameDesktop->actionwindow == nullptr);
}

BOOST_FIXTURE_TEST_CASE(Zooming, GameInterfaceFixture)
{
    const KeyEvent zoomInEv('z');
    const KeyEvent zoomOutEv('Z');
    KeyEvent zoomDefaultEv('z');
    zoomDefaultEv.ctrl = true;

    BOOST_TEST(view->getCurrentTargetZoomFactor() == ZOOM_FACTORS[ZOOM_DEFAULT_INDEX]);
    for(size_t i = ZOOM_DEFAULT_INDEX + 1; i < ZOOM_FACTORS.size(); i++)
    {
        gameDesktop->msgKeyDown(zoomInEv);
        BOOST_TEST(view->getCurrentTargetZoomFactor() == ZOOM_FACTORS[i]);
    }
    // Wrap around
    gameDesktop->msgKeyDown(zoomInEv);
    BOOST_TEST(view->getCurrentTargetZoomFactor() == ZOOM_FACTORS[0]);
    gameDesktop->msgKeyDown(zoomInEv);
    BOOST_TEST(view->getCurrentTargetZoomFactor() == ZOOM_FACTORS[1]);

    gameDesktop->msgKeyDown(zoomOutEv);
    BOOST_TEST(view->getCurrentTargetZoomFactor() == ZOOM_FACTORS[0]);
    gameDesktop->msgKeyDown(zoomOutEv);
    BOOST_TEST(view->getCurrentTargetZoomFactor() == ZOOM_FACTORS.back());

    gameDesktop->msgKeyDown(zoomDefaultEv);
    BOOST_TEST(view->getCurrentTargetZoomFactor() == ZOOM_FACTORS[ZOOM_DEFAULT_INDEX]);

    gameDesktop->msgWheelDown({});
    auto curZoom = view->getCurrentTargetZoomFactor();
    BOOST_TEST(curZoom < ZOOM_FACTORS[ZOOM_DEFAULT_INDEX]);
    BOOST_TEST(curZoom > ZOOM_FACTORS[ZOOM_DEFAULT_INDEX - 1]);
    // Go to prev index
    gameDesktop->msgKeyDown(zoomOutEv);
    BOOST_TEST(view->getCurrentTargetZoomFactor() == ZOOM_FACTORS[ZOOM_DEFAULT_INDEX - 1]);

    gameDesktop->msgKeyDown(zoomDefaultEv);
    gameDesktop->msgWheelUp({});
    curZoom = view->getCurrentTargetZoomFactor();
    BOOST_TEST(curZoom > ZOOM_FACTORS[ZOOM_DEFAULT_INDEX]);
    BOOST_TEST(curZoom < ZOOM_FACTORS[ZOOM_DEFAULT_INDEX + 1]);
    // Go to next index
    gameDesktop->msgKeyDown(zoomInEv);
    BOOST_TEST(view->getCurrentTargetZoomFactor() == ZOOM_FACTORS[ZOOM_DEFAULT_INDEX + 1]);

    gameDesktop->msgWheelDown({});
    curZoom = view->getCurrentTargetZoomFactor();
    BOOST_TEST(curZoom > ZOOM_FACTORS[ZOOM_DEFAULT_INDEX]);
    BOOST_TEST(curZoom < ZOOM_FACTORS[ZOOM_DEFAULT_INDEX + 1]);
    // Go to prev index
    gameDesktop->msgKeyDown(zoomOutEv);
    BOOST_TEST(view->getCurrentTargetZoomFactor() == ZOOM_FACTORS[ZOOM_DEFAULT_INDEX]);

    // Zoom in and out gets to 1.0 (default)
    static_assert(ZOOM_FACTORS[ZOOM_DEFAULT_INDEX] == 1.f);
    gameDesktop->msgWheelUp({});
    gameDesktop->msgWheelDown({});
    BOOST_TEST(view->getCurrentTargetZoomFactor() == 1.f);
    const auto numZoom = rttr::test::randomValue(2, 10);
    for(int i = 0; i < numZoom; i++)
        gameDesktop->msgWheelDown({});
    for(int i = 0; i < numZoom; i++)
        gameDesktop->msgWheelUp({});
    BOOST_TEST(view->getCurrentTargetZoomFactor() == 1.f);
}

BOOST_AUTO_TEST_SUITE_END()
