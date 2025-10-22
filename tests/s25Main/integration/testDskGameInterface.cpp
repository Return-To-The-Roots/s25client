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
    void Msg_PaintBefore() override {}
    void Msg_PaintAfter() override {}
    using dskGameInterface::actionwindow;
    using dskGameInterface::Msg_KeyDown;
    using dskGameInterface::Msg_WheelDown;
    using dskGameInterface::Msg_WheelUp;
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
    SETTINGS.interface.mouseMode = 0;

    Position startPos(10, 15);
    MouseCoords mouse(startPos);
    mouse.rdown = true;

    // First time with mouse, second time with touch
    for(unsigned i = 0; i < 2; i++)
    {
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
            SETTINGS.interface.mouseMode = 1;
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
            SETTINGS.interface.mouseMode = 0;
        }

        // Natural scrolling
        {
            SETTINGS.interface.mouseMode = 2;
            WINDOWMANAGER.Msg_RightDown(mouse);
            startPos = mouse.pos;
            BOOST_TEST_REQUIRE(WINDOWMANAGER.GetCursor() == Cursor::Scroll);
            const DrawPoint pos = view->GetOffset();
            mouse.pos = startPos + Position(4, 3);
            WINDOWMANAGER.Msg_MouseMove(mouse);
            BOOST_TEST_REQUIRE(WINDOWMANAGER.GetCursor() == Cursor::Scroll);
            BOOST_TEST_REQUIRE(view->GetOffset() == pos - Position(4, 3));
            mouse.rdown = false;
            WINDOWMANAGER.Msg_RightUp(mouse);
            SETTINGS.interface.mouseMode = 0;
        }

        // Reset mouse
        mouse = MouseCoords(startPos);
        mouse.rdown = true;
        // Emulate touch input for second run
        mouse.num_tfingers = 1;
    }

    mouse.num_tfingers = 0;

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
    MouseCoords mouse(startPos);
    mouse.rdown = true;
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
    MouseCoords mouse(startPos);
    mouse.ldown = true;
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

BOOST_FIXTURE_TEST_CASE(Zooming, GameInterfaceFixture)
{
    const KeyEvent zoomInEv('z');
    const KeyEvent zoomOutEv('Z');
    KeyEvent zoomDefaultEv('z');
    zoomDefaultEv.ctrl = true;

    BOOST_TEST(view->GetCurrentTargetZoomFactor() == ZOOM_FACTORS[ZOOM_DEFAULT_INDEX]);
    for(size_t i = ZOOM_DEFAULT_INDEX + 1; i < ZOOM_FACTORS.size(); i++)
    {
        gameDesktop->Msg_KeyDown(zoomInEv);
        BOOST_TEST(view->GetCurrentTargetZoomFactor() == ZOOM_FACTORS[i]);
    }
    // Wrap around
    gameDesktop->Msg_KeyDown(zoomInEv);
    BOOST_TEST(view->GetCurrentTargetZoomFactor() == ZOOM_FACTORS[0]);
    gameDesktop->Msg_KeyDown(zoomInEv);
    BOOST_TEST(view->GetCurrentTargetZoomFactor() == ZOOM_FACTORS[1]);

    gameDesktop->Msg_KeyDown(zoomOutEv);
    BOOST_TEST(view->GetCurrentTargetZoomFactor() == ZOOM_FACTORS[0]);
    gameDesktop->Msg_KeyDown(zoomOutEv);
    BOOST_TEST(view->GetCurrentTargetZoomFactor() == ZOOM_FACTORS.back());

    gameDesktop->Msg_KeyDown(zoomDefaultEv);
    BOOST_TEST(view->GetCurrentTargetZoomFactor() == ZOOM_FACTORS[ZOOM_DEFAULT_INDEX]);

    gameDesktop->Msg_WheelDown({});
    auto curZoom = view->GetCurrentTargetZoomFactor();
    BOOST_TEST(curZoom < ZOOM_FACTORS[ZOOM_DEFAULT_INDEX]);
    BOOST_TEST(curZoom > ZOOM_FACTORS[ZOOM_DEFAULT_INDEX - 1]);
    // Go to prev index
    gameDesktop->Msg_KeyDown(zoomOutEv);
    BOOST_TEST(view->GetCurrentTargetZoomFactor() == ZOOM_FACTORS[ZOOM_DEFAULT_INDEX - 1]);

    gameDesktop->Msg_KeyDown(zoomDefaultEv);
    gameDesktop->Msg_WheelUp({});
    curZoom = view->GetCurrentTargetZoomFactor();
    BOOST_TEST(curZoom > ZOOM_FACTORS[ZOOM_DEFAULT_INDEX]);
    BOOST_TEST(curZoom < ZOOM_FACTORS[ZOOM_DEFAULT_INDEX + 1]);
    // Go to next index
    gameDesktop->Msg_KeyDown(zoomInEv);
    BOOST_TEST(view->GetCurrentTargetZoomFactor() == ZOOM_FACTORS[ZOOM_DEFAULT_INDEX + 1]);

    gameDesktop->Msg_WheelDown({});
    curZoom = view->GetCurrentTargetZoomFactor();
    BOOST_TEST(curZoom > ZOOM_FACTORS[ZOOM_DEFAULT_INDEX]);
    BOOST_TEST(curZoom < ZOOM_FACTORS[ZOOM_DEFAULT_INDEX + 1]);
    // Go to prev index
    gameDesktop->Msg_KeyDown(zoomOutEv);
    BOOST_TEST(view->GetCurrentTargetZoomFactor() == ZOOM_FACTORS[ZOOM_DEFAULT_INDEX]);

    // Zoom in and out gets to 1.0 (default)
    static_assert(ZOOM_FACTORS[ZOOM_DEFAULT_INDEX] == 1.f);
    gameDesktop->Msg_WheelUp({});
    gameDesktop->Msg_WheelDown({});
    BOOST_TEST(view->GetCurrentTargetZoomFactor() == 1.f);
    const auto numZoom = rttr::test::randomValue(2, 10);
    for(int i = 0; i < numZoom; i++)
        gameDesktop->Msg_WheelDown({});
    for(int i = 0; i < numZoom; i++)
        gameDesktop->Msg_WheelUp({});
    BOOST_TEST(view->GetCurrentTargetZoomFactor() == 1.f);
}

BOOST_AUTO_TEST_SUITE_END()
