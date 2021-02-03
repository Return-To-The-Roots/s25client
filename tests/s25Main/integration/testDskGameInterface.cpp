// Copyright (c) 2016 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#include "GamePlayer.h"
#include "PointOutput.h"
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
    dskGameInterfaceMock(const std::shared_ptr<Game>& game)
        : dskGameInterface(game, std::shared_ptr<NWFInfo>(), 0, false)
    {}
    void Msg_PaintBefore() override {}
    void Msg_PaintAfter() override {}
};
struct GameInterfaceFixture : uiHelper::Fixture
{
    WorldFixture<CreateEmptyWorld, 1> worldFixture;
    dskGameInterface* gameDesktop;
    const GameWorldView* view;
    GameInterfaceFixture()
    {
        gameDesktop = static_cast<dskGameInterface*>(
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
    Position startPos(10, 15);
    MouseCoords mouse(startPos, false, true);
    // Regular scrolling: Right down, 2 moves, right up
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

    // Opening a window does not cancel scrolling
    mouse.rdown = true;
    WINDOWMANAGER.Msg_RightDown(mouse);
    startPos = mouse.pos;
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

BOOST_AUTO_TEST_SUITE_END()
