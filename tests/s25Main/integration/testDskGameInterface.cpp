// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
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
    void Msg_PaintBefore() override {}
    void Msg_PaintAfter() override {}
    using dskGameInterface::actionwindow;
};
struct GameInterfaceFixture : uiHelper::Fixture
{
    WorldFixture<CreateEmptyWorld, 1> worldFixture;
    dskGameInterfaceMock* gameDesktop;
    const GameWorldView* view;
    GameInterfaceFixture()
    {
        gameDesktop = static_cast<dskGameInterfaceMock*>(
          WINDOWMANAGER.Switch(std::make_unique<dskGameInterfaceMock>(worldFixture.game)));
        WINDOWMANAGER.Draw();
        view = &gameDesktop->GetView();
    }
};
void checkNotScrolling(const GameWorldView& view, Cursor cursor = Cursor::Hand)
{
    const DrawPoint pos = view.GetOffset();
    MouseCoords mouse(Position(40, 11));
    WINDOWMANAGER.Msg_MouseMove(mouse);
    BOOST_TEST_REQUIRE(WINDOWMANAGER.GetCursor() == cursor);
    BOOST_TEST_REQUIRE(view.GetOffset() == pos);
    mouse.pos += Position(-20, 30);
    WINDOWMANAGER.Msg_MouseMove(mouse);
    BOOST_TEST_REQUIRE(WINDOWMANAGER.GetCursor() == cursor);
    BOOST_TEST_REQUIRE(view.GetOffset() == pos);
}
} // namespace

BOOST_FIXTURE_TEST_CASE(Scrolling, GameInterfaceFixture)
{
    const int acceleration = 2;
    SETTINGS.interface.invertMouse = false;

    Position startPos(10, 15);
    MouseCoords mouse(startPos, false, true);
    // Regular scrolling: Right down, 2 moves, right up
    {
        WINDOWMANAGER.Msg_RightDown(mouse);
        BOOST_TEST_REQUIRE(WINDOWMANAGER.GetCursor() == Cursor::Scroll);
        DrawPoint pos = view->GetOffset();
        mouse.pos = startPos + Position(4, 3);
        WINDOWMANAGER.Msg_MouseMove(mouse);
        pos += acceleration * Position(4, 3);
        BOOST_TEST_REQUIRE(WINDOWMANAGER.GetCursor() == Cursor::Scroll);
        BOOST_TEST_REQUIRE(view->GetOffset() == pos);
        mouse.pos = startPos + Position(-6, 7);
        WINDOWMANAGER.Msg_MouseMove(mouse);
        pos += acceleration * Position(-6, 7);
        BOOST_TEST_REQUIRE(WINDOWMANAGER.GetCursor() == Cursor::Scroll);
        BOOST_TEST_REQUIRE(view->GetOffset() == pos);
        mouse.rdown = false;
        WINDOWMANAGER.Msg_RightUp(mouse);
        BOOST_TEST_REQUIRE(WINDOWMANAGER.GetCursor() == Cursor::Hand);
        BOOST_TEST_REQUIRE(view->GetOffset() == pos);
        checkNotScrolling(*view);
    }

    // Inverted scrolling
    {
        SETTINGS.interface.invertMouse = true;
        WINDOWMANAGER.Msg_RightDown(mouse);
        startPos = mouse.pos;
        BOOST_TEST_REQUIRE(WINDOWMANAGER.GetCursor() == Cursor::Scroll);
        const DrawPoint pos = view->GetOffset();
        mouse.pos = startPos + Position(4, 3);
        WINDOWMANAGER.Msg_MouseMove(mouse);
        BOOST_TEST_REQUIRE(WINDOWMANAGER.GetCursor() == Cursor::Scroll);
        BOOST_TEST_REQUIRE(view->GetOffset() == pos - acceleration * Position(4, 3));
        mouse.rdown = false;
        WINDOWMANAGER.Msg_RightUp(mouse);
        SETTINGS.interface.invertMouse = false;
    }

    // Opening a window does not cancel scrolling
    {
        mouse.rdown = true;
        WINDOWMANAGER.Msg_RightDown(mouse);
        startPos = mouse.pos;
        DrawPoint pos = view->GetOffset();
        KeyEvent key;
        key.kt = KeyType::Char;
        key.c = 'm';
        key.ctrl = key.alt = key.shift = false;
        WINDOWMANAGER.Msg_KeyDown(key);
        BOOST_TEST_REQUIRE(WINDOWMANAGER.GetTopMostWindow());
        BOOST_TEST_REQUIRE(WINDOWMANAGER.GetCursor() == Cursor::Scroll);
        mouse.pos = startPos + Position(-6, 7);
        WINDOWMANAGER.Msg_MouseMove(mouse);
        pos += acceleration * Position(-6, 7);
        BOOST_TEST_REQUIRE(WINDOWMANAGER.GetCursor() == Cursor::Scroll);
        BOOST_TEST_REQUIRE(view->GetOffset() == pos);
        // Closing it doesn't either
        WINDOWMANAGER.Msg_KeyDown(key);
        WINDOWMANAGER.Draw();
        BOOST_TEST_REQUIRE(gameDesktop->IsActive());
        BOOST_TEST_REQUIRE(!WINDOWMANAGER.GetTopMostWindow());
        BOOST_TEST_REQUIRE(WINDOWMANAGER.GetCursor() == Cursor::Scroll);
        mouse.pos = startPos + Position(-6, 7);
        WINDOWMANAGER.Msg_MouseMove(mouse);
        pos += acceleration * Position(-6, 7);
        BOOST_TEST_REQUIRE(WINDOWMANAGER.GetCursor() == Cursor::Scroll);
        BOOST_TEST_REQUIRE(view->GetOffset() == pos);
        // Left click does cancel it
        mouse.ldown = true;
        WINDOWMANAGER.Msg_LeftDown(mouse);
        mouse.ldown = false;
        WINDOWMANAGER.Msg_LeftUp(mouse);
        BOOST_TEST_REQUIRE(WINDOWMANAGER.GetCursor() == Cursor::Hand);
        BOOST_TEST_REQUIRE(view->GetOffset() == pos);
        checkNotScrolling(*view);
    }
}

BOOST_FIXTURE_TEST_CASE(ScrollingWhileRoadBuilding, GameInterfaceFixture)
{
    const int acceleration = 2;
    MapPoint hqPos = worldFixture.world.GetPlayer(0).GetFirstWH()->GetFlagPos(); //-V522
    gameDesktop->GI_StartRoadBuilding(hqPos, false);
    BOOST_TEST_REQUIRE(WINDOWMANAGER.GetCursor() == Cursor::Remove);
    Position startPos(10, 15);
    MouseCoords mouse(startPos, false, true);
    // Regular scrolling
    WINDOWMANAGER.Msg_RightDown(mouse);
    BOOST_TEST_REQUIRE(WINDOWMANAGER.GetCursor() == Cursor::Scroll);
    DrawPoint pos = view->GetOffset();
    mouse.pos = startPos + Position(4, 3);
    WINDOWMANAGER.Msg_MouseMove(mouse);
    pos += acceleration * Position(4, 3);
    BOOST_TEST_REQUIRE(WINDOWMANAGER.GetCursor() == Cursor::Scroll);
    BOOST_TEST_REQUIRE(view->GetOffset() == pos);
    mouse.rdown = false;
    WINDOWMANAGER.Msg_RightUp(mouse);
    BOOST_TEST_REQUIRE(WINDOWMANAGER.GetCursor() == Cursor::Remove);
    BOOST_TEST_REQUIRE(view->GetOffset() == pos);
    checkNotScrolling(*view, Cursor::Remove);

    // left click also stops scrolling
    mouse.rdown = true;
    WINDOWMANAGER.Msg_RightDown(mouse);
    startPos = mouse.pos;
    BOOST_TEST_REQUIRE(WINDOWMANAGER.GetCursor() == Cursor::Scroll);
    mouse.pos = startPos + Position(-6, 7);
    WINDOWMANAGER.Msg_MouseMove(mouse);
    pos += acceleration * Position(-6, 7);
    BOOST_TEST_REQUIRE(WINDOWMANAGER.GetCursor() == Cursor::Scroll);
    BOOST_TEST_REQUIRE(view->GetOffset() == pos);
    mouse.ldown = true;
    WINDOWMANAGER.Msg_LeftDown(mouse);
    mouse.ldown = false;
    WINDOWMANAGER.Msg_LeftUp(mouse);
    BOOST_TEST_REQUIRE(WINDOWMANAGER.GetCursor() == Cursor::Remove);
    BOOST_TEST_REQUIRE(view->GetOffset() == pos);
    checkNotScrolling(*view, Cursor::Remove);
}

BOOST_FIXTURE_TEST_CASE(ScrollingWithCtrl, GameInterfaceFixture)
{
    const int acceleration = 2;
    Position startPos(10, 15);
    MouseCoords mouse(startPos, true);
    uiHelper::GetVideoDriver()->modKeyState_.ctrl = true;
    WINDOWMANAGER.Msg_LeftDown(mouse);
    BOOST_TEST_REQUIRE(WINDOWMANAGER.GetCursor() == Cursor::Scroll);
    DrawPoint pos = view->GetOffset();
    mouse.pos = startPos + Position(4, 3);
    WINDOWMANAGER.Msg_MouseMove(mouse);
    pos += acceleration * Position(4, 3);
    BOOST_TEST_REQUIRE(WINDOWMANAGER.GetCursor() == Cursor::Scroll);
    BOOST_TEST_REQUIRE(view->GetOffset() == pos);
    mouse.ldown = false;
    WINDOWMANAGER.Msg_LeftUp(mouse);
    BOOST_TEST_REQUIRE(WINDOWMANAGER.GetCursor() == Cursor::Hand);
    BOOST_TEST_REQUIRE(view->GetOffset() == pos);
    checkNotScrolling(*view);
}

BOOST_FIXTURE_TEST_CASE(IwActionClose, GameInterfaceFixture)
{
    gameDesktop->ShowActionWindow(iwAction::Tabs{}, MapPoint(0, 1), DrawPoint(42, 37), false);
    WINDOWMANAGER.Draw();
    BOOST_TEST_REQUIRE(WINDOWMANAGER.GetTopMostWindow());
    BOOST_TEST_REQUIRE(WINDOWMANAGER.GetTopMostWindow() == gameDesktop->actionwindow);
    WINDOWMANAGER.GetTopMostWindow()->Close();
    WINDOWMANAGER.Draw();
    BOOST_TEST_REQUIRE(WINDOWMANAGER.GetTopMostWindow() == nullptr);
    BOOST_TEST_REQUIRE(gameDesktop->actionwindow == nullptr);
}

BOOST_AUTO_TEST_SUITE_END()
