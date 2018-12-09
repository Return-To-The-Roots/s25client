// Copyright (c) 2016 -2017 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Return To The Roots.
//
// Return To The Roots is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Return To The Roots is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Return To The Roots. If not, see <http://www.gnu.org/licenses/>.

#include "rttrDefines.h" // IWYU pragma: keep
#include "CreateEmptyWorld.h"
#include "GameManager.h"
#include "GamePlayer.h"
#include "PointOutput.h"
#include "WindowManager.h"
#include "WorldFixture.h"
#include "buildings/nobBaseWarehouse.h"
#include "desktops/dskGameInterface.h"
#include "driver/KeyEvent.h"
#include "driver/MouseCoords.h"
#include "initTestHelpers.h"
#include "mockupDrivers/MockupVideoDriver.h"
#include <boost/test/unit_test.hpp>

// Test stuff related to building/building quality
BOOST_AUTO_TEST_SUITE(GameInterfaceDesktop)

namespace {
struct dskGameInterfaceMock : public dskGameInterface
{
    dskGameInterfaceMock(boost::shared_ptr<Game> game) : dskGameInterface(game, boost::shared_ptr<NWFInfo>(), 0, false) {}
    void Msg_PaintBefore() override {}
    void Msg_PaintAfter() override {}
};
struct GameInterfaceFixture
{
    WorldFixture<CreateEmptyWorld, 1> worldFixture;
    dskGameInterface* gameDesktop;
    const GameWorldView* view;
    GameInterfaceFixture()
    {
        initGUITests();
        gameDesktop = new dskGameInterfaceMock(worldFixture.game);
        WINDOWMANAGER.Switch(gameDesktop);
        WINDOWMANAGER.Draw();
        view = &gameDesktop->GetView();
    }
};
void checkNotScrolling(const GameWorldView& view, CursorType cursor = CURSOR_HAND)
{
    const DrawPoint pos = view.GetOffset();
    MouseCoords mouse(Position(40, 11));
    WINDOWMANAGER.Msg_MouseMove(mouse);
    BOOST_REQUIRE_EQUAL(GAMEMANAGER.GetCursor(), cursor);
    BOOST_REQUIRE_EQUAL(view.GetOffset(), pos);
    mouse.pos += Position(-20, 30);
    WINDOWMANAGER.Msg_MouseMove(mouse);
    BOOST_REQUIRE_EQUAL(GAMEMANAGER.GetCursor(), cursor);
    BOOST_REQUIRE_EQUAL(view.GetOffset(), pos);
}
} // namespace

BOOST_FIXTURE_TEST_CASE(Scrolling, GameInterfaceFixture)
{
    const int acceleration = 2;
    Position startPos(10, 15);
    MouseCoords mouse(startPos, false, true);
    // Regular scrolling: Right down, 2 moves, right up
    WINDOWMANAGER.Msg_RightDown(mouse);
    BOOST_REQUIRE_EQUAL(GAMEMANAGER.GetCursor(), CURSOR_SCROLL);
    DrawPoint pos = view->GetOffset();
    mouse.pos = startPos + Position(4, 3);
    WINDOWMANAGER.Msg_MouseMove(mouse);
    pos += acceleration * Position(4, 3);
    BOOST_REQUIRE_EQUAL(GAMEMANAGER.GetCursor(), CURSOR_SCROLL);
    BOOST_REQUIRE_EQUAL(view->GetOffset(), pos);
    mouse.pos = startPos + Position(-6, 7);
    WINDOWMANAGER.Msg_MouseMove(mouse);
    pos += acceleration * Position(-6, 7);
    BOOST_REQUIRE_EQUAL(GAMEMANAGER.GetCursor(), CURSOR_SCROLL);
    BOOST_REQUIRE_EQUAL(view->GetOffset(), pos);
    mouse.rdown = false;
    WINDOWMANAGER.Msg_RightUp(mouse);
    BOOST_REQUIRE_EQUAL(GAMEMANAGER.GetCursor(), CURSOR_HAND);
    BOOST_REQUIRE_EQUAL(view->GetOffset(), pos);
    checkNotScrolling(*view);

    // Opening a window does not cancel scrolling
    mouse.rdown = true;
    WINDOWMANAGER.Msg_RightDown(mouse);
    startPos = mouse.pos;
    KeyEvent key;
    key.kt = KT_CHAR;
    key.c = 'm';
    key.ctrl = key.alt = key.shift = false;
    WINDOWMANAGER.Msg_KeyDown(key);
    BOOST_REQUIRE(WINDOWMANAGER.GetTopMostWindow());
    BOOST_REQUIRE_EQUAL(GAMEMANAGER.GetCursor(), CURSOR_SCROLL);
    mouse.pos = startPos + Position(-6, 7);
    WINDOWMANAGER.Msg_MouseMove(mouse);
    pos += acceleration * Position(-6, 7);
    BOOST_REQUIRE_EQUAL(GAMEMANAGER.GetCursor(), CURSOR_SCROLL);
    BOOST_REQUIRE_EQUAL(view->GetOffset(), pos);
    // Closing it doesn't either
    WINDOWMANAGER.Msg_KeyDown(key);
    WINDOWMANAGER.Draw();
    BOOST_REQUIRE(gameDesktop->IsActive());
    BOOST_REQUIRE(!WINDOWMANAGER.GetTopMostWindow());
    BOOST_REQUIRE_EQUAL(GAMEMANAGER.GetCursor(), CURSOR_SCROLL);
    mouse.pos = startPos + Position(-6, 7);
    WINDOWMANAGER.Msg_MouseMove(mouse);
    pos += acceleration * Position(-6, 7);
    BOOST_REQUIRE_EQUAL(GAMEMANAGER.GetCursor(), CURSOR_SCROLL);
    BOOST_REQUIRE_EQUAL(view->GetOffset(), pos);
    // Left click does cancel it
    mouse.ldown = true;
    WINDOWMANAGER.Msg_LeftDown(mouse);
    mouse.ldown = false;
    WINDOWMANAGER.Msg_LeftUp(mouse);
    BOOST_REQUIRE_EQUAL(GAMEMANAGER.GetCursor(), CURSOR_HAND);
    BOOST_REQUIRE_EQUAL(view->GetOffset(), pos);
    checkNotScrolling(*view);
}

BOOST_FIXTURE_TEST_CASE(ScrollingWhileRoadBuilding, GameInterfaceFixture)
{
    const int acceleration = 2;
    MapPoint hqPos = worldFixture.world.GetPlayer(0).GetFirstWH()->GetFlagPos();
    gameDesktop->GI_StartRoadBuilding(hqPos, false);
    BOOST_REQUIRE_EQUAL(GAMEMANAGER.GetCursor(), CURSOR_RM);
    Position startPos(10, 15);
    MouseCoords mouse(startPos, false, true);
    // Regular scrolling
    WINDOWMANAGER.Msg_RightDown(mouse);
    BOOST_REQUIRE_EQUAL(GAMEMANAGER.GetCursor(), CURSOR_SCROLL);
    DrawPoint pos = view->GetOffset();
    mouse.pos = startPos + Position(4, 3);
    WINDOWMANAGER.Msg_MouseMove(mouse);
    pos += acceleration * Position(4, 3);
    BOOST_REQUIRE_EQUAL(GAMEMANAGER.GetCursor(), CURSOR_SCROLL);
    BOOST_REQUIRE_EQUAL(view->GetOffset(), pos);
    mouse.rdown = false;
    WINDOWMANAGER.Msg_RightUp(mouse);
    BOOST_REQUIRE_EQUAL(GAMEMANAGER.GetCursor(), CURSOR_RM);
    BOOST_REQUIRE_EQUAL(view->GetOffset(), pos);
    checkNotScrolling(*view, CURSOR_RM);

    // left click also stops scrolling
    mouse.rdown = true;
    WINDOWMANAGER.Msg_RightDown(mouse);
    startPos = mouse.pos;
    BOOST_REQUIRE_EQUAL(GAMEMANAGER.GetCursor(), CURSOR_SCROLL);
    mouse.pos = startPos + Position(-6, 7);
    WINDOWMANAGER.Msg_MouseMove(mouse);
    pos += acceleration * Position(-6, 7);
    BOOST_REQUIRE_EQUAL(GAMEMANAGER.GetCursor(), CURSOR_SCROLL);
    BOOST_REQUIRE_EQUAL(view->GetOffset(), pos);
    mouse.ldown = true;
    WINDOWMANAGER.Msg_LeftDown(mouse);
    mouse.ldown = false;
    WINDOWMANAGER.Msg_LeftUp(mouse);
    BOOST_REQUIRE_EQUAL(GAMEMANAGER.GetCursor(), CURSOR_RM);
    BOOST_REQUIRE_EQUAL(view->GetOffset(), pos);
    checkNotScrolling(*view, CURSOR_RM);
}

BOOST_FIXTURE_TEST_CASE(ScrollingWithCtrl, GameInterfaceFixture)
{
    const int acceleration = 2;
    Position startPos(10, 15);
    MouseCoords mouse(startPos, true);
    GetVideoDriver()->modKeyState_.ctrl = true;
    WINDOWMANAGER.Msg_LeftDown(mouse);
    BOOST_REQUIRE_EQUAL(GAMEMANAGER.GetCursor(), CURSOR_SCROLL);
    DrawPoint pos = view->GetOffset();
    mouse.pos = startPos + Position(4, 3);
    WINDOWMANAGER.Msg_MouseMove(mouse);
    pos += acceleration * Position(4, 3);
    BOOST_REQUIRE_EQUAL(GAMEMANAGER.GetCursor(), CURSOR_SCROLL);
    BOOST_REQUIRE_EQUAL(view->GetOffset(), pos);
    mouse.ldown = false;
    WINDOWMANAGER.Msg_LeftUp(mouse);
    BOOST_REQUIRE_EQUAL(GAMEMANAGER.GetCursor(), CURSOR_HAND);
    BOOST_REQUIRE_EQUAL(view->GetOffset(), pos);
    checkNotScrolling(*view);
}

BOOST_AUTO_TEST_SUITE_END()
