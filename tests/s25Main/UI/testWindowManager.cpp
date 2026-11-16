// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "DrawPoint.h"
#include "PointOutput.h"
#include "Settings.h"
#include "WindowManager.h"
#include "desktops/Desktop.h"
#include "driver/MouseCoords.h"
#include "helpers/containerUtils.h"
#include "ingameWindows/IngameWindow.h"
#include "ingameWindows/TransmitSettingsIgwAdapter.h"
#include "mockupDrivers/MockupVideoDriver.h"
#include "uiHelper/uiHelpers.hpp"
#include "gameData/const_gui_ids.h"
#include <turtle/mock.hpp>
#include <boost/test/unit_test.hpp>
#include <sstream>

//-V:MOCK_METHOD:813
//-V:MOCK_EXPECT:807

inline bool operator==(const MouseCoords& lhs, const MouseCoords& rhs)
{
    return lhs.pos == rhs.pos && lhs.ldown == rhs.ldown && lhs.rdown == rhs.rdown && lhs.dbl_click == rhs.dbl_click;
}

inline std::ostream& operator<<(std::ostream& s, const MouseCoords& mc)
{
    return s << "<" << mc.pos << "," << mc.ldown << "," << mc.rdown << "," << mc.dbl_click << ">";
}

namespace {
MOCK_BASE_CLASS(TestDesktop, Desktop)
{
    TestDesktop() : Desktop(nullptr) {}
    MOCK_METHOD(msgLeftDown, 1)
    MOCK_METHOD(msgLeftUp, 1)
    MOCK_METHOD(msgMouseMove, 1)
};

struct WMFixture : mock::cleanup
{
    MockupVideoDriver* video;
    TestDesktop* dsk;
    WMFixture() : video(uiHelper::getVideoDriver())
    {
        dsk = static_cast<TestDesktop*>(WINDOWMANAGER.Switch(std::make_unique<TestDesktop>()));
        MOCK_EXPECT(dsk->msgMouseMove).once().returns(true);
        MOCK_EXPECT(dsk->msgLeftDown).once().returns(true);
        MOCK_EXPECT(dsk->msgLeftUp).once().returns(true);
        WINDOWMANAGER.draw();

        // Ensure next click in test won't be registered as double-click
        // by doing a fake click outside of any relevant space and time
        MouseCoords mc1(-100, -100);
        mc1.ldown = true;
        MouseCoords mc1_u(mc1.pos);
        video->tickCount_ = DOUBLE_CLICK_INTERVAL * 100;
        WINDOWMANAGER.msgLeftDown(mc1);
        WINDOWMANAGER.msgLeftUp(mc1);
        video->tickCount_ = 0;

        mock::verify(*dsk);
        mock::reset(*dsk);
    }
};
} // namespace

BOOST_AUTO_TEST_SUITE(WindowManagerSuite)

BOOST_AUTO_TEST_CASE(MouseCoordsOutput)
{
    std::stringstream s;
    MouseCoords mc(Position(2, 3));
    mc.ldown = true;
    mc.dbl_click = true;
    s << mc;
    BOOST_TEST(s.str() == "<(2, 3),1,0,1>");
}

BOOST_FIXTURE_TEST_CASE(LeftClick, WMFixture)
{
    video->tickCount_ = 0;
    mock::sequence s;
    MouseCoords mc1(5, 2);
    mc1.ldown = true;
    MouseCoords mc1_u(mc1.pos);
    MouseCoords mc2(10, 7);
    mc2.ldown = true;
    MouseCoords mc2_u(mc2.pos);
    MOCK_EXPECT(dsk->msgLeftDown).once().with(mc1).in(s).returns(true);
    MOCK_EXPECT(dsk->msgLeftUp).once().with(mc1_u).in(s).returns(true);
    MOCK_EXPECT(dsk->msgLeftDown).once().with(mc2).in(s).returns(true);
    MOCK_EXPECT(dsk->msgLeftUp).once().with(mc2_u).in(s).returns(true);
    MOCK_EXPECT(dsk->msgLeftDown).once().with(mc1).in(s).returns(true);
    MOCK_EXPECT(dsk->msgLeftUp).once().with(mc2_u).in(s).returns(true);
    WINDOWMANAGER.msgLeftDown(mc1);
    video->tickCount_ += 50;
    WINDOWMANAGER.msgLeftUp(mc1_u);
    video->tickCount_ += 2000;
    WINDOWMANAGER.msgLeftDown(mc2);
    video->tickCount_ += 50;
    WINDOWMANAGER.msgLeftUp(mc2_u);
    video->tickCount_ += 3000;
    WINDOWMANAGER.msgLeftDown(mc1);
    WINDOWMANAGER.msgLeftUp(mc2_u);
    mock::verify();
}

BOOST_FIXTURE_TEST_CASE(DblClick, WMFixture)
{
    video->tickCount_ = 0;
    mock::sequence s;
    MouseCoords mc1(5, 2);
    mc1.ldown = true;
    MouseCoords mc1_u(mc1.pos);
    MouseCoords mc2(6, 1);
    mc2.ldown = true;
    MouseCoords mc2_u(mc2.pos);
    // Click with time > DOUBLE_CLICK_INTERVAL is no dbl click
    MOCK_EXPECT(dsk->msgLeftDown).once().with(mc1).in(s).returns(true);
    MOCK_EXPECT(dsk->msgLeftUp).once().with(mc1_u).in(s).returns(true);
    MOCK_EXPECT(dsk->msgLeftDown).once().with(mc1).in(s).returns(true);
    MOCK_EXPECT(dsk->msgLeftUp).once().with(mc1_u).in(s).returns(true);
    // Click on different positions is no dbl click
    MOCK_EXPECT(dsk->msgLeftDown).once().with(mc2).in(s).returns(true);
    MOCK_EXPECT(dsk->msgLeftUp).once().with(mc2_u).in(s).returns(true);
    // Click on same pos with time < DOUBLE_CLICK_INTERVAL is dbl click
    MOCK_EXPECT(dsk->msgLeftDown).once().with(mc2).in(s).returns(true);
    MouseCoords dbl_mc2(mc2.pos);
    dbl_mc2.dbl_click = true;
    MOCK_EXPECT(dsk->msgLeftUp).once().with(dbl_mc2).in(s).returns(true);
    // No triple click
    MOCK_EXPECT(dsk->msgLeftDown).once().with(mc2).in(s).returns(true);
    MOCK_EXPECT(dsk->msgLeftUp).once().with(mc2_u).in(s).returns(true);

    WINDOWMANAGER.msgLeftDown(mc1);
    video->tickCount_ += 1;
    WINDOWMANAGER.msgLeftUp(mc1_u);
    // time > DOUBLE_CLICK_INTERVAL
    video->tickCount_ += DOUBLE_CLICK_INTERVAL;
    WINDOWMANAGER.msgLeftDown(mc1);
    video->tickCount_ += 1;
    WINDOWMANAGER.msgLeftUp(mc1_u);
    // Different position
    WINDOWMANAGER.msgLeftDown(mc2);
    WINDOWMANAGER.msgLeftUp(mc2_u);
    // time < DOUBLE_CLICK_INTERVAL
    video->tickCount_ += DOUBLE_CLICK_INTERVAL - 1;
    WINDOWMANAGER.msgLeftDown(mc2);
    WINDOWMANAGER.msgLeftUp(mc2_u);
    // Triple?
    video->tickCount_ += DOUBLE_CLICK_INTERVAL - 1;
    WINDOWMANAGER.msgLeftDown(mc2);
    WINDOWMANAGER.msgLeftUp(mc2_u);
    mock::verify();
}

BOOST_FIXTURE_TEST_CASE(DblClickTouch, WMFixture)
{
    video->tickCount_ = 0;
    video->numTfinger_ = 1;
    mock::sequence s;

    MouseCoords mc1(5, 2);
    mc1.ldown = true;
    mc1.num_tfingers = 1;
    MouseCoords mc1_u(mc1.pos);
    mc1_u.num_tfingers = 1;
    // Test click on distance > TOUCH_MAX_DOUBLE_CLICK_DISTANCE
    MouseCoords mc2(mc1.pos.x + TOUCH_MAX_DOUBLE_CLICK_DISTANCE + 1, mc1.pos.y);
    mc2.ldown = true;
    mc2.num_tfingers = 1;
    MouseCoords mc2_u(mc2.pos);
    mc2_u.num_tfingers = 1;
    // Test click on distance < TOUCH_MAX_DOUBLE_CLICK_DISTANCE
    MouseCoords mc3(mc2.pos.x + TOUCH_MAX_DOUBLE_CLICK_DISTANCE - 1, mc2.pos.y);
    mc3.ldown = true;
    mc3.num_tfingers = 1;
    MouseCoords mc3_u(mc3.pos);
    mc3_u.num_tfingers = 1;

    // Set last click position on mc1
    MOCK_EXPECT(dsk->msgLeftDown).once().with(mc1).in(s).returns(true);
    MOCK_EXPECT(dsk->msgLeftUp).once().with(mc1_u).in(s).returns(true);

    // Touch position mc2 -> Too far away fom previous point -> Set last click position on mc2
    MOCK_EXPECT(dsk->msgLeftDown).once().with(mc2).in(s).returns(true);
    MOCK_EXPECT(dsk->msgLeftUp).once().with(mc2_u).in(s).returns(true);

    // Touch position mc3 -> In range of mc -> dblclick
    MOCK_EXPECT(dsk->msgLeftDown).once().with(mc3).in(s).returns(true);
    MouseCoords range_mc3(mc3.pos);
    range_mc3.dbl_click = true;
    range_mc3.num_tfingers = 1;
    MOCK_EXPECT(dsk->msgLeftUp).once().with(range_mc3).in(s).returns(true);

    // Try to touch 3rd time -> no dblclick -> Set last click position to mc3
    MOCK_EXPECT(dsk->msgLeftDown).once().with(mc3).in(s).returns(true);
    MOCK_EXPECT(dsk->msgLeftUp).once().with(mc3_u).in(s).returns(true);

    // Touch position mc3 again -> In range of mc but duration > TOUCH_DOUBLE_CLICK_INTERVAL no dblclick
    MOCK_EXPECT(dsk->msgLeftDown).once().with(mc3).in(s).returns(true);
    MOCK_EXPECT(dsk->msgLeftUp).once().with(mc3_u).in(s).returns(true);

    WINDOWMANAGER.msgLeftDown(mc1);
    video->tickCount_ += 1;
    WINDOWMANAGER.msgLeftUp(mc1_u);

    WINDOWMANAGER.msgLeftDown(mc2);
    video->tickCount_ += 1;
    WINDOWMANAGER.msgLeftUp(mc2_u);

    video->tickCount_ += TOUCH_DOUBLE_CLICK_INTERVAL - 1;
    WINDOWMANAGER.msgLeftDown(mc3);
    WINDOWMANAGER.msgLeftUp(mc3_u);

    video->tickCount_ += TOUCH_DOUBLE_CLICK_INTERVAL - 1;
    WINDOWMANAGER.msgLeftDown(mc3);
    WINDOWMANAGER.msgLeftUp(mc3_u);

    video->tickCount_ += TOUCH_DOUBLE_CLICK_INTERVAL;
    WINDOWMANAGER.msgLeftDown(mc3);
    WINDOWMANAGER.msgLeftUp(mc3_u);
    mock::verify();
}

namespace {
MOCK_BASE_CLASS(TestIngameWnd, IngameWindow)
{
    explicit TestIngameWnd(unsigned id, bool isModal = false, CloseBehavior closeBehavior = CloseBehavior::Regular)
        : IngameWindow(id, DrawPoint(0, 0), Extent(100, 100), "", nullptr, isModal, closeBehavior)
    {
        closed.erase(std::remove(closed.begin(), closed.end(), this), closed.end());
    }
    ~TestIngameWnd() override { closed.push_back(this); }
    MOCK_METHOD(drawContent, 0, void())
    MOCK_METHOD(msgKeyDown, 1)
    MOCK_METHOD(msgLeftDown, 1)
    MOCK_METHOD(msgMiddleDown, 1)
    static std::vector<TestIngameWnd*> closed;
};
std::vector<TestIngameWnd*> TestIngameWnd::closed;

#define REQUIRE_WINDOW_ALIVE(wnd) BOOST_TEST_REQUIRE(!helpers::contains(TestIngameWnd::closed, wnd))
#define REQUIRE_WINDOW_ACTIVE(wnd) \
    REQUIRE_WINDOW_ALIVE(wnd);     \
    BOOST_TEST_REQUIRE((wnd)->IsActive())
#define REQUIRE_WINDOW_DESTROYED(wnd) BOOST_TEST_REQUIRE(helpers::contains(TestIngameWnd::closed, wnd))
} // namespace

// Note for all tests: We check the state after a Draw call as this is when a user notices it

BOOST_FIXTURE_TEST_CASE(ShowIngameWnd, uiHelper::Fixture)
{
    auto* wnd = &WINDOWMANAGER.show(std::make_unique<TestIngameWnd>(CGI_HELP));
    BOOST_TEST_REQUIRE(wnd);
    MOCK_EXPECT(wnd->drawContent).once();
    WINDOWMANAGER.draw();
    REQUIRE_WINDOW_ACTIVE(wnd);
    BOOST_TEST(!WINDOWMANAGER.getCurrentDesktop()->IsActive());
    // Closing the window removes it and calls delete
    wnd->close();
    WINDOWMANAGER.draw();
    REQUIRE_WINDOW_DESTROYED(wnd);
    // Desktop active again
    BOOST_TEST(WINDOWMANAGER.getCurrentDesktop()->IsActive());

    // Opening a window with the same ID works. The last gets the focus
    wnd = &WINDOWMANAGER.show(std::make_unique<TestIngameWnd>(CGI_HELP));
    auto* wnd2 = &WINDOWMANAGER.show(std::make_unique<TestIngameWnd>(CGI_HELP));
    BOOST_TEST_REQUIRE((wnd && wnd2));
    MOCK_EXPECT(wnd->drawContent).once();
    MOCK_EXPECT(wnd2->drawContent).once();
    WINDOWMANAGER.draw();
    REQUIRE_WINDOW_ALIVE(wnd);
    REQUIRE_WINDOW_ACTIVE(wnd2);
    // Desktop inactive again
    BOOST_TEST(!WINDOWMANAGER.getCurrentDesktop()->IsActive());

    // Closing a window and immediately reopening works
    wnd->close();
    wnd2->close();
    wnd2 = &WINDOWMANAGER.show(std::make_unique<TestIngameWnd>(wnd->getID()));
    BOOST_TEST_REQUIRE(wnd2);
    MOCK_EXPECT(wnd2->drawContent).once();
    WINDOWMANAGER.draw();
    REQUIRE_WINDOW_DESTROYED(wnd);
    REQUIRE_WINDOW_ACTIVE(wnd2);
    BOOST_TEST(!WINDOWMANAGER.getCurrentDesktop()->IsActive());

    // Close by ID
    WINDOWMANAGER.close(wnd2->getID());
    WINDOWMANAGER.draw();
    REQUIRE_WINDOW_DESTROYED(wnd2);
    BOOST_TEST(WINDOWMANAGER.getCurrentDesktop()->IsActive());
    mock::verify();
}

BOOST_FIXTURE_TEST_CASE(ToggleIngameWnd, uiHelper::Fixture)
{
    // When no window with the ID is open, then this is just Show
    auto* wnd = WINDOWMANAGER.toggleWindow(std::make_unique<TestIngameWnd>(CGI_HELP));
    BOOST_TEST_REQUIRE(wnd);
    MOCK_EXPECT(wnd->drawContent).once();
    WINDOWMANAGER.draw();
    REQUIRE_WINDOW_ACTIVE(wnd);
    BOOST_TEST(!WINDOWMANAGER.getCurrentDesktop()->IsActive());

    // When window is about to be closed we can open a new one
    wnd->close();
    auto* wnd2 = WINDOWMANAGER.toggleWindow(std::make_unique<TestIngameWnd>(CGI_HELP));
    BOOST_TEST_REQUIRE(wnd2);
    MOCK_EXPECT(wnd2->drawContent).once();
    WINDOWMANAGER.draw();
    REQUIRE_WINDOW_ACTIVE(wnd2);
    REQUIRE_WINDOW_DESTROYED(wnd);
    BOOST_TEST(!WINDOWMANAGER.getCurrentDesktop()->IsActive());

    // Opening a window with the same ID closes and frees both
    wnd = wnd2;
    wnd2 = new TestIngameWnd(wnd->getID());
    BOOST_TEST((WINDOWMANAGER.toggleWindow(std::unique_ptr<TestIngameWnd>(wnd2)) == nullptr));
    WINDOWMANAGER.draw();
    REQUIRE_WINDOW_DESTROYED(wnd);
    REQUIRE_WINDOW_DESTROYED(wnd2);
    // Desktop active again
    BOOST_TEST(WINDOWMANAGER.getCurrentDesktop()->IsActive());

    // Windows with different IDs are fine
    wnd = WINDOWMANAGER.toggleWindow(std::make_unique<TestIngameWnd>(CGI_HELP));
    wnd2 = WINDOWMANAGER.toggleWindow(std::make_unique<TestIngameWnd>(CGI_SETTINGS));
    BOOST_TEST_REQUIRE((wnd && wnd2));
    MOCK_EXPECT(wnd->drawContent).once();
    MOCK_EXPECT(wnd2->drawContent).once();
    WINDOWMANAGER.draw();
    REQUIRE_WINDOW_ALIVE(wnd);
    REQUIRE_WINDOW_ACTIVE(wnd2);
    wnd->close();
    wnd2->close();
    mock::verify();
}

BOOST_FIXTURE_TEST_CASE(ReplaceIngameWnd, uiHelper::Fixture)
{
    // When no window with the ID is open, then this is just Show
    auto* wnd = &WINDOWMANAGER.replaceWindow(std::make_unique<TestIngameWnd>(CGI_HELP));
    MOCK_EXPECT(wnd->drawContent).once();
    WINDOWMANAGER.draw();
    REQUIRE_WINDOW_ACTIVE(wnd);
    BOOST_TEST(!WINDOWMANAGER.getCurrentDesktop()->IsActive());

    // When window is about to be closed we can open a new one
    wnd->close();
    auto* wnd2 = &WINDOWMANAGER.replaceWindow(std::make_unique<TestIngameWnd>(CGI_HELP));
    BOOST_TEST_REQUIRE(wnd2);
    MOCK_EXPECT(wnd2->drawContent).once();
    WINDOWMANAGER.draw();
    REQUIRE_WINDOW_ACTIVE(wnd2);
    REQUIRE_WINDOW_DESTROYED(wnd);
    BOOST_TEST(!WINDOWMANAGER.getCurrentDesktop()->IsActive());

    // Opening a window with the same ID closes and frees the first
    wnd = wnd2;
    wnd2 = &WINDOWMANAGER.replaceWindow(std::make_unique<TestIngameWnd>(CGI_HELP));
    BOOST_TEST_REQUIRE(wnd2);
    MOCK_EXPECT(wnd2->drawContent).once();
    WINDOWMANAGER.draw();
    REQUIRE_WINDOW_DESTROYED(wnd);
    REQUIRE_WINDOW_ACTIVE(wnd2);
    BOOST_TEST(!WINDOWMANAGER.getCurrentDesktop()->IsActive());

    // Windows with different IDs are fine
    wnd = wnd2;
    wnd2 = &WINDOWMANAGER.replaceWindow(std::make_unique<TestIngameWnd>(CGI_SETTINGS));
    BOOST_TEST_REQUIRE(wnd2);
    MOCK_EXPECT(wnd->drawContent).once();
    MOCK_EXPECT(wnd2->drawContent).once();
    WINDOWMANAGER.draw();
    REQUIRE_WINDOW_ALIVE(wnd);
    REQUIRE_WINDOW_ACTIVE(wnd2);
    wnd->close();
    wnd2->close();

    // Modal windows are not replaced but placed on top of existing ones
    wnd = &WINDOWMANAGER.replaceWindow(std::make_unique<TestIngameWnd>(CGI_SETTINGS, true));
    wnd2 = &WINDOWMANAGER.replaceWindow(std::make_unique<TestIngameWnd>(CGI_SETTINGS, true));
    BOOST_TEST_REQUIRE((wnd && wnd2));
    MOCK_EXPECT(wnd->drawContent).once();
    MOCK_EXPECT(wnd2->drawContent).once();
    WINDOWMANAGER.draw();
    REQUIRE_WINDOW_ACTIVE(wnd2);
    REQUIRE_WINDOW_ALIVE(wnd);
    wnd->close();
    wnd2->close();
    mock::verify();
}

BOOST_FIXTURE_TEST_CASE(ModalWindowPlacement, uiHelper::Fixture)
{
    // new modal windows get placed on top of older ones, non-modal ones behind all modal ones
    auto& wnd = WINDOWMANAGER.replaceWindow(std::make_unique<TestIngameWnd>(CGI_MSGBOX, true));
    MOCK_EXPECT(wnd.drawContent).once();
    WINDOWMANAGER.draw();
    REQUIRE_WINDOW_ACTIVE(&wnd);
    BOOST_TEST_REQUIRE(WINDOWMANAGER.getTopMostWindow() == &wnd);
    auto& wnd2 = WINDOWMANAGER.replaceWindow(std::make_unique<TestIngameWnd>(CGI_MSGBOX, true));
    MOCK_EXPECT(wnd.drawContent).once();
    MOCK_EXPECT(wnd2.drawContent).once();
    WINDOWMANAGER.draw();
    REQUIRE_WINDOW_ACTIVE(&wnd2);
    BOOST_TEST_REQUIRE(WINDOWMANAGER.getTopMostWindow() == &wnd2);
    auto& wnd3 = WINDOWMANAGER.replaceWindow(std::make_unique<TestIngameWnd>(CGI_MISSION_STATEMENT, true));
    MOCK_EXPECT(wnd.drawContent).once();
    MOCK_EXPECT(wnd2.drawContent).once();
    MOCK_EXPECT(wnd3.drawContent).once();
    WINDOWMANAGER.draw();
    REQUIRE_WINDOW_ACTIVE(&wnd3);
    BOOST_TEST_REQUIRE(WINDOWMANAGER.getTopMostWindow() == &wnd3);
    auto& wnd4 = WINDOWMANAGER.replaceWindow(std::make_unique<TestIngameWnd>(CGI_MSGBOX));
    MOCK_EXPECT(wnd.drawContent).once();
    MOCK_EXPECT(wnd2.drawContent).once();
    MOCK_EXPECT(wnd3.drawContent).once();
    MOCK_EXPECT(wnd4.drawContent).once();
    WINDOWMANAGER.draw();
    REQUIRE_WINDOW_ACTIVE(&wnd3);
    BOOST_TEST_REQUIRE(WINDOWMANAGER.getTopMostWindow() == &wnd3);
    auto& wnd5 = WINDOWMANAGER.replaceWindow(std::make_unique<TestIngameWnd>(CGI_HELP, true));
    MOCK_EXPECT(wnd.drawContent).once();
    MOCK_EXPECT(wnd2.drawContent).once();
    MOCK_EXPECT(wnd3.drawContent).once();
    MOCK_EXPECT(wnd4.drawContent).once();
    MOCK_EXPECT(wnd5.drawContent).once();
    WINDOWMANAGER.draw();
    REQUIRE_WINDOW_ACTIVE(&wnd5);
    BOOST_TEST_REQUIRE(WINDOWMANAGER.getTopMostWindow() == &wnd5);
    auto& wnd6 = WINDOWMANAGER.replaceWindow(std::make_unique<TestIngameWnd>(CGI_SETTINGS));
    MOCK_EXPECT(wnd.drawContent).once();
    MOCK_EXPECT(wnd2.drawContent).once();
    MOCK_EXPECT(wnd3.drawContent).once();
    MOCK_EXPECT(wnd4.drawContent).once();
    MOCK_EXPECT(wnd5.drawContent).once();
    MOCK_EXPECT(wnd6.drawContent).once();
    WINDOWMANAGER.draw();
    REQUIRE_WINDOW_ACTIVE(&wnd5);
    BOOST_TEST_REQUIRE(WINDOWMANAGER.getTopMostWindow() == &wnd5);
    // Now we have the following order
    std::vector<TestIngameWnd*> expectedOrder = {&wnd5, &wnd3, &wnd2, &wnd, &wnd6, &wnd4};
    // Only way to check the order is to simulate a key event, expect the top most one to handle it and close it, then
    // proceed
    mock::sequence s;
    for(TestIngameWnd* curWnd : expectedOrder)
    {
        MOCK_EXPECT(curWnd->msgKeyDown).once().in(s).returns(true);
        MOCK_EXPECT(curWnd->drawContent); // Ignore all draw calls
    }
    // Way outside any window, should still be handled
    const KeyEvent ke('a');
    for(TestIngameWnd* curWnd : expectedOrder)
    {
        REQUIRE_WINDOW_ACTIVE(curWnd);
        WINDOWMANAGER.msgKeyDown(ke);
        curWnd->close();
        WINDOWMANAGER.draw();
    }
    mock::verify();
}

BOOST_FIXTURE_TEST_CASE(ModalBlocksMouseActivation, uiHelper::Fixture)
{
    auto* wndA = &WINDOWMANAGER.show(std::make_unique<TestIngameWnd>(CGI_HELP));
    auto* wndB = &WINDOWMANAGER.show(std::make_unique<TestIngameWnd>(CGI_SETTINGS, true));
    wndB->setPos(DrawPoint(200, 200));
    MOCK_EXPECT(wndA->drawContent).once();
    MOCK_EXPECT(wndB->drawContent).once();
    WINDOWMANAGER.draw();
    REQUIRE_WINDOW_ACTIVE(wndB);
    BOOST_TEST_REQUIRE(WINDOWMANAGER.getTopMostWindow() == wndB);

    // Click is over non-modal A, not modal B; B must still receive it instead of the window under the cursor.
    const Position posInA = wndA->getDrawPos() + Position(10, 10);

    MOCK_EXPECT(wndB->msgLeftDown).once().returns(true);
    MouseCoords mcDown(posInA);
    mcDown.ldown = true;
    WINDOWMANAGER.msgLeftDown(mcDown);
    WINDOWMANAGER.msgLeftUp(MouseCoords(posInA));
    BOOST_TEST_REQUIRE(WINDOWMANAGER.getTopMostWindow() == wndB);
    REQUIRE_WINDOW_ACTIVE(wndB);
    BOOST_TEST(!wndA->IsActive());
    mock::verify();

    MOCK_EXPECT(wndB->msgMiddleDown).once().returns(true);
    MouseCoords mcMiddle(posInA);
    mcMiddle.mdown = true;
    WINDOWMANAGER.msgMiddleDown(mcMiddle);
    BOOST_TEST_REQUIRE(WINDOWMANAGER.getTopMostWindow() == wndB);
    BOOST_TEST(!wndA->IsActive());
    mock::verify();

    // Wheel events must respect modal blocking too; the stack/active state must stay unchanged regardless.
    WINDOWMANAGER.msgWheelUp(MouseCoords(posInA));
    WINDOWMANAGER.msgWheelDown(MouseCoords(posInA));
    BOOST_TEST_REQUIRE(WINDOWMANAGER.getTopMostWindow() == wndB);
    REQUIRE_WINDOW_ACTIVE(wndB);
    BOOST_TEST(!wndA->IsActive());
}

BOOST_FIXTURE_TEST_CASE(BuriedModalClosedByGame, uiHelper::Fixture)
{
    auto* wndA = &WINDOWMANAGER.show(std::make_unique<TestIngameWnd>(CGI_HELP, true));
    auto* wndB = &WINDOWMANAGER.show(std::make_unique<TestIngameWnd>(CGI_SETTINGS, true));
    auto* wndC = &WINDOWMANAGER.show(std::make_unique<TestIngameWnd>(CGI_MISSION_STATEMENT, true));
    MOCK_EXPECT(wndA->drawContent).once();
    MOCK_EXPECT(wndB->drawContent).once();
    MOCK_EXPECT(wndC->drawContent).once();
    WINDOWMANAGER.draw();
    REQUIRE_WINDOW_ACTIVE(wndC);
    BOOST_TEST_REQUIRE(WINDOWMANAGER.getTopMostWindow() == wndC);

    // Topmost C must remain topmost after non-topmost B is closed
    WINDOWMANAGER.close(wndB->getID());
    MOCK_EXPECT(wndA->drawContent).once();
    MOCK_EXPECT(wndC->drawContent).once();
    WINDOWMANAGER.draw();

    REQUIRE_WINDOW_DESTROYED(wndB);
    BOOST_TEST_REQUIRE(WINDOWMANAGER.getTopMostWindow() == wndC);
    REQUIRE_WINDOW_ACTIVE(wndC);
    REQUIRE_WINDOW_ALIVE(wndA);
    mock::verify();
}

BOOST_FIXTURE_TEST_CASE(ClickActivatesBackgroundNonModal, uiHelper::Fixture)
{
    auto* wndA = &WINDOWMANAGER.show(std::make_unique<TestIngameWnd>(CGI_HELP));
    auto* wndB = &WINDOWMANAGER.show(std::make_unique<TestIngameWnd>(CGI_SETTINGS));
    wndB->setPos(DrawPoint(200, 200));
    MOCK_EXPECT(wndA->drawContent).once();
    MOCK_EXPECT(wndB->drawContent).once();
    WINDOWMANAGER.draw();
    BOOST_TEST_REQUIRE(WINDOWMANAGER.getTopMostWindow() == wndB);
    REQUIRE_WINDOW_ACTIVE(wndB);
    BOOST_TEST(!wndA->IsActive());

    // B is topmost, click is over A which must become topmost/active
    MOCK_EXPECT(wndA->msgLeftDown).once().returns(true);
    MouseCoords mcDownA(wndA->getDrawPos() + Position(10, 10));
    mcDownA.ldown = true;
    WINDOWMANAGER.msgLeftDown(mcDownA);
    WINDOWMANAGER.msgLeftUp(MouseCoords(mcDownA.pos));
    BOOST_TEST_REQUIRE(WINDOWMANAGER.getTopMostWindow() == wndA);
    REQUIRE_WINDOW_ACTIVE(wndA);
    BOOST_TEST(!wndB->IsActive());
    mock::verify();

    // A is topmost, click is over B which must become topmost/active again
    MOCK_EXPECT(wndB->msgLeftDown).once().returns(true);
    MouseCoords mcDownB(wndB->getDrawPos() + Position(10, 10));
    mcDownB.ldown = true;
    WINDOWMANAGER.msgLeftDown(mcDownB);
    WINDOWMANAGER.msgLeftUp(MouseCoords(mcDownB.pos));
    BOOST_TEST_REQUIRE(WINDOWMANAGER.getTopMostWindow() == wndB);
    REQUIRE_WINDOW_ACTIVE(wndB);
    BOOST_TEST(!wndA->IsActive());
    mock::verify();
}

BOOST_FIXTURE_TEST_CASE(EscClosesWindow, uiHelper::Fixture)
{
    auto* wnd = &WINDOWMANAGER.show(std::make_unique<TestIngameWnd>(CGI_HELP));
    BOOST_TEST_REQUIRE(WINDOWMANAGER.getTopMostWindow() == wnd);
    const KeyEvent evEsc(KeyType::Escape);
    WINDOWMANAGER.msgKeyDown(evEsc);
    WINDOWMANAGER.draw();
    BOOST_TEST_REQUIRE(WINDOWMANAGER.getTopMostWindow() == nullptr);
    REQUIRE_WINDOW_DESTROYED(wnd);

    // Multiple escapes close multiple windows, even modal ones
    auto* wnd1 = &WINDOWMANAGER.show(std::make_unique<TestIngameWnd>(CGI_HELP));
    auto* wnd2 = &WINDOWMANAGER.show(std::make_unique<TestIngameWnd>(CGI_HELP));
    auto* wnd3 = &WINDOWMANAGER.show(std::make_unique<TestIngameWnd>(CGI_HELP, true));
    WINDOWMANAGER.msgKeyDown(evEsc);
    WINDOWMANAGER.msgKeyDown(evEsc);
    MOCK_EXPECT(wnd1->drawContent).once();
    WINDOWMANAGER.draw();
    BOOST_TEST_REQUIRE(WINDOWMANAGER.getTopMostWindow() == wnd1);
    REQUIRE_WINDOW_DESTROYED(wnd2);
    REQUIRE_WINDOW_DESTROYED(wnd3);
    WINDOWMANAGER.msgKeyDown(evEsc);
    WINDOWMANAGER.draw();
    REQUIRE_WINDOW_DESTROYED(wnd1);

    // ESC does not close non-user-closable windows
    wnd1 = &WINDOWMANAGER.show(std::make_unique<TestIngameWnd>(CGI_HELP, false, CloseBehavior::Custom));
    BOOST_TEST_REQUIRE(WINDOWMANAGER.getTopMostWindow() == wnd1);
    WINDOWMANAGER.msgKeyDown(evEsc);
    REQUIRE_WINDOW_ALIVE(wnd1);
    MOCK_EXPECT(wnd1->drawContent).once();
    WINDOWMANAGER.draw();
    BOOST_TEST_REQUIRE(WINDOWMANAGER.getTopMostWindow() == wnd1);

    wnd2 = &WINDOWMANAGER.show(std::make_unique<TestIngameWnd>(CGI_HELP, true, CloseBehavior::Custom));
    BOOST_TEST_REQUIRE(WINDOWMANAGER.getTopMostWindow() == wnd2);
    WINDOWMANAGER.msgKeyDown(evEsc);
    REQUIRE_WINDOW_ALIVE(wnd1);
    REQUIRE_WINDOW_ALIVE(wnd2);
    MOCK_EXPECT(wnd1->drawContent).once();
    MOCK_EXPECT(wnd2->drawContent).once();
    WINDOWMANAGER.draw();
    BOOST_TEST_REQUIRE(WINDOWMANAGER.getTopMostWindow() == wnd2);
}

BOOST_FIXTURE_TEST_CASE(RightclickClosesWindow, uiHelper::Fixture)
{
    auto* wnd = &WINDOWMANAGER.show(std::make_unique<TestIngameWnd>(CGI_HELP));
    BOOST_TEST_REQUIRE(WINDOWMANAGER.getTopMostWindow() == wnd);
    MouseCoords evRDown(wnd->getDrawPos() + Position(10, 10));
    evRDown.rdown = true;
    WINDOWMANAGER.msgRightDown(evRDown);
    WINDOWMANAGER.draw();
    BOOST_TEST_REQUIRE(WINDOWMANAGER.getTopMostWindow() == nullptr);
    REQUIRE_WINDOW_DESTROYED(wnd);

    // Only close top most window
    auto* wnd1 = &WINDOWMANAGER.show(std::make_unique<TestIngameWnd>(CGI_HELP));
    auto* wnd2 = &WINDOWMANAGER.show(std::make_unique<TestIngameWnd>(CGI_HELP));
    WINDOWMANAGER.msgRightDown(evRDown);
    MOCK_EXPECT(wnd1->drawContent).once();
    WINDOWMANAGER.draw();
    BOOST_TEST_REQUIRE(WINDOWMANAGER.getTopMostWindow() == wnd1);
    REQUIRE_WINDOW_DESTROYED(wnd2);
    // Also modal windows, even when not opened last
    wnd2 = &WINDOWMANAGER.show(std::make_unique<TestIngameWnd>(CGI_HELP, true));
    auto* wnd3 = &WINDOWMANAGER.show(std::make_unique<TestIngameWnd>(CGI_HELP));
    WINDOWMANAGER.msgRightDown(evRDown);
    REQUIRE_WINDOW_ALIVE(wnd1);
    REQUIRE_WINDOW_ALIVE(wnd3);
    MOCK_EXPECT(wnd1->drawContent).once();
    MOCK_EXPECT(wnd3->drawContent).once();
    WINDOWMANAGER.draw();
    BOOST_TEST_REQUIRE(WINDOWMANAGER.getTopMostWindow() == wnd3);
    REQUIRE_WINDOW_DESTROYED(wnd2);
    WINDOWMANAGER.msgRightDown(evRDown);
    MOCK_EXPECT(wnd1->drawContent).once();
    WINDOWMANAGER.draw();
    WINDOWMANAGER.msgRightDown(evRDown);
    WINDOWMANAGER.draw();
    REQUIRE_WINDOW_DESTROYED(wnd1);
    REQUIRE_WINDOW_DESTROYED(wnd3);

    // Don't close non-user-closable windows
    wnd1 = &WINDOWMANAGER.show(std::make_unique<TestIngameWnd>(CGI_HELP, false, CloseBehavior::Custom));
    BOOST_TEST_REQUIRE(WINDOWMANAGER.getTopMostWindow() == wnd1);
    WINDOWMANAGER.msgRightDown(evRDown);
    MOCK_EXPECT(wnd1->drawContent).once();
    WINDOWMANAGER.draw();
    BOOST_TEST_REQUIRE(WINDOWMANAGER.getTopMostWindow() == wnd1);

    wnd2 = &WINDOWMANAGER.show(std::make_unique<TestIngameWnd>(CGI_HELP, true, CloseBehavior::NoRightClick));
    BOOST_TEST_REQUIRE(WINDOWMANAGER.getTopMostWindow() == wnd2);
    WINDOWMANAGER.msgRightDown(evRDown);
    MOCK_EXPECT(wnd1->drawContent).once();
    MOCK_EXPECT(wnd2->drawContent).once();
    WINDOWMANAGER.draw();
    BOOST_TEST_REQUIRE(WINDOWMANAGER.getTopMostWindow() == wnd2);
}

BOOST_FIXTURE_TEST_CASE(PinnedWindows, uiHelper::Fixture)
{
    constexpr KeyEvent evEsc(KeyType::Escape);
    KeyEvent evAltW('w');
    evAltW.alt = true;

    BOOST_TEST_CONTEXT("Pinned windows ignore escape key")
    {
        auto* wnd = &WINDOWMANAGER.show(std::make_unique<TestIngameWnd>(CGI_HELP));

        wnd->setPinned();
        BOOST_TEST_CONTEXT("Pinned")
        {
            BOOST_TEST_REQUIRE(WINDOWMANAGER.getTopMostWindow() == wnd);
            WINDOWMANAGER.msgKeyDown(evEsc);
            MOCK_EXPECT(wnd->drawContent).once();
            WINDOWMANAGER.draw();
            BOOST_TEST_REQUIRE(WINDOWMANAGER.getTopMostWindow() == wnd);
        }

        wnd->setPinned(false);
        BOOST_TEST_CONTEXT("Un-pinned")
        {
            WINDOWMANAGER.msgKeyDown(evEsc);
            WINDOWMANAGER.draw();
            REQUIRE_WINDOW_DESTROYED(wnd);
        }
    }

    BOOST_TEST_CONTEXT("Pinned windows close on Alt+W")
    {
        auto* wnd = &WINDOWMANAGER.show(std::make_unique<TestIngameWnd>(CGI_HELP));

        wnd->setPinned();
        BOOST_TEST_CONTEXT("Pinned")
        {
            BOOST_TEST_REQUIRE(WINDOWMANAGER.getTopMostWindow() == wnd);
            WINDOWMANAGER.msgKeyDown(evAltW);
            WINDOWMANAGER.draw();
            REQUIRE_WINDOW_DESTROYED(wnd);
        }
    }

    BOOST_TEST_CONTEXT("Pinned windows ignore right click")
    {
        auto* wnd = &WINDOWMANAGER.show(std::make_unique<TestIngameWnd>(CGI_HELP));
        BOOST_TEST_REQUIRE(WINDOWMANAGER.getTopMostWindow() == wnd);
        MouseCoords evRDown(wnd->getDrawPos() + Position(10, 10));
        evRDown.rdown = true;

        wnd->setPinned();
        BOOST_TEST_CONTEXT("Pinned")
        {
            WINDOWMANAGER.msgRightDown(evRDown);
            MOCK_EXPECT(wnd->drawContent).once();
            WINDOWMANAGER.draw();
            BOOST_TEST_REQUIRE(WINDOWMANAGER.getTopMostWindow() == wnd);
        }

        wnd->setPinned(false);
        BOOST_TEST_CONTEXT("Un-pinned")
        {
            WINDOWMANAGER.msgRightDown(evRDown);
            WINDOWMANAGER.draw();
            REQUIRE_WINDOW_DESTROYED(wnd);
        }
    }

    mock::verify();
}

BOOST_FIXTURE_TEST_CASE(SnapWindow, uiHelper::Fixture)
{
    SETTINGS.interface.windowSnapDistance = 10;

    WINDOWMANAGER.show(std::make_unique<IngameWindow>(0, DrawPoint(0, 0), Extent(100, 100), "", nullptr));
    auto* wnd =
      &WINDOWMANAGER.show(std::make_unique<IngameWindow>(0, DrawPoint(100, 0), Extent(100, 100), "", nullptr));

    BOOST_TEST(WINDOWMANAGER.snapWindow(wnd, wnd->getBoundaryRect()) == SnapOffset(0, 0));

    wnd->setPos(DrawPoint(111, 5));
    BOOST_TEST(WINDOWMANAGER.snapWindow(wnd, wnd->getBoundaryRect()) == SnapOffset(0, 0));

    wnd->setPos(DrawPoint(110, 5));
    BOOST_TEST(WINDOWMANAGER.snapWindow(wnd, wnd->getBoundaryRect()) == SnapOffset(-10, -5));
}

MOCK_BASE_CLASS(MockSettingsWnd, TransmitSettingsIgwAdapter)
{
    static int activeWnds;
    MockSettingsWnd(unsigned id)
        : TransmitSettingsIgwAdapter(id, IngameWindow::posCenter, Extent(256, 256), "", nullptr)
    {
        activeWnds++;
    }
    ~MockSettingsWnd() { activeWnds--; }
    MOCK_NON_CONST_METHOD(updateSettings, 0); // LCOV_EXCL_LINE
    MOCK_NON_CONST_METHOD(transmitSettings, 0);
};

int MockSettingsWnd::activeWnds = 0;

BOOST_FIXTURE_TEST_CASE(TestTransmitSettingsAdapter, uiHelper::Fixture)
{
    auto* wnd = &WINDOWMANAGER.show(std::make_unique<MockSettingsWnd>(CGI_TOOLS));
    BOOST_TEST_REQUIRE(wnd);
    {
        // Save settings on close via window method
        MOCK_EXPECT(wnd->transmitSettings).once();
        wnd->close();
        WINDOWMANAGER.draw();
        BOOST_TEST(MockSettingsWnd::activeWnds == 0);
    }
    {
        // Save settings on close via ID
        wnd = &WINDOWMANAGER.show(std::make_unique<MockSettingsWnd>(CGI_TOOLS));
        MOCK_EXPECT(wnd->transmitSettings).once();
        WINDOWMANAGER.close(CGI_TOOLS);
        WINDOWMANAGER.draw();
        BOOST_TEST(MockSettingsWnd::activeWnds == 0);
    }
    {
        // Save settings on closeNow
        wnd = &WINDOWMANAGER.show(std::make_unique<MockSettingsWnd>(CGI_TOOLS));
        MOCK_EXPECT(wnd->transmitSettings).once();
        WINDOWMANAGER.closeNow(wnd);
        BOOST_TEST(MockSettingsWnd::activeWnds == 0);
    }
    {
        // Save settings on ESC
        wnd = &WINDOWMANAGER.show(std::make_unique<MockSettingsWnd>(CGI_TOOLS));
        BOOST_TEST_REQUIRE(WINDOWMANAGER.getTopMostWindow() == wnd);
        MOCK_EXPECT(wnd->transmitSettings).once();
        WINDOWMANAGER.msgKeyDown(KeyEvent(KeyType::Escape));
        WINDOWMANAGER.draw();
        BOOST_TEST(MockSettingsWnd::activeWnds == 0);
        BOOST_TEST(WINDOWMANAGER.getTopMostWindow() == nullptr);
    }
    {
        // Save settings on ALT+W
        wnd = &WINDOWMANAGER.show(std::make_unique<MockSettingsWnd>(CGI_TOOLS));
        BOOST_TEST_REQUIRE(WINDOWMANAGER.getTopMostWindow() == wnd);
        MOCK_EXPECT(wnd->transmitSettings).once();
        KeyEvent ke('w');
        ke.alt = true;
        WINDOWMANAGER.msgKeyDown(ke);
        WINDOWMANAGER.draw();
        BOOST_TEST(MockSettingsWnd::activeWnds == 0);
        BOOST_TEST(WINDOWMANAGER.getTopMostWindow() == nullptr);
    }
    mock::verify();
}

BOOST_AUTO_TEST_SUITE_END()
