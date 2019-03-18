// Copyright (c) 2016 - 2018 Settlers Freaks (sf-team at siedler25.org)
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
#include "PointOutput.h"
#include "WindowManager.h"
#include "desktops/Desktop.h"
#include "helpers/containerUtils.h"
#include "ingameWindows/IngameWindow.h"
#include "mockupDrivers/MockupVideoDriver.h"
#include "uiHelper/uiHelpers.hpp"
#include "gameData/const_gui_ids.h"
#include <boost/test/unit_test.hpp>
#include <turtle/mock.hpp>

//-V:MOCK_METHOD:813
//-V:MOCK_EXPECT:807

inline bool operator==(const MouseCoords& lhs, const MouseCoords& rhs)
{
    return lhs.GetPos() == rhs.GetPos() && lhs.ldown == rhs.ldown && lhs.rdown == rhs.rdown && lhs.dbl_click == rhs.dbl_click;
}

inline std::ostream& operator<<(std::ostream& s, const MouseCoords& mc)
{
    return s << "<" << mc.GetPos() << "," << mc.ldown << "," << mc.rdown << "," << mc.dbl_click << ">";
}

namespace {
/* clang-format off */
MOCK_BASE_CLASS(TestDesktop, Desktop)
{
    TestDesktop(): Desktop(nullptr){}
    MOCK_METHOD(Msg_LeftDown, 1)
    MOCK_METHOD(Msg_RightDown, 1)
    MOCK_METHOD(Msg_LeftUp, 1)
    MOCK_METHOD(Msg_RightUp, 1)
    MOCK_METHOD(Msg_WheelUp, 1)
    MOCK_METHOD(Msg_WheelDown, 1)
    MOCK_METHOD(Msg_MouseMove, 1)
    MOCK_METHOD(Msg_KeyDown, 1)
};
/* clang-format on */

struct WMFixture
{
    MockupVideoDriver* video;
    TestDesktop* dsk;
    WMFixture() : video(uiHelper::GetVideoDriver())
    {
        dsk = new TestDesktop;
        WINDOWMANAGER.Switch(dsk);
        MOCK_EXPECT(dsk->Msg_MouseMove).once().returns(true);
        WINDOWMANAGER.Draw();
        mock::verify(*dsk);
        mock::reset(*dsk);
    }
};
} // namespace

BOOST_AUTO_TEST_SUITE(WindowManagerSuite)

BOOST_FIXTURE_TEST_CASE(LeftClick, WMFixture)
{
    video->tickCount_ = 0;
    mock::sequence s;
    MouseCoords mc1(5, 2, true);
    MouseCoords mc1_u(mc1.GetPos());
    MouseCoords mc2(10, 7, true);
    MouseCoords mc2_u(mc2.GetPos());
    MOCK_EXPECT(dsk->Msg_LeftDown).once().with(mc1).in(s).returns(true);
    MOCK_EXPECT(dsk->Msg_LeftUp).once().with(mc1_u).in(s).returns(true);
    MOCK_EXPECT(dsk->Msg_LeftDown).once().with(mc2).in(s).returns(true);
    MOCK_EXPECT(dsk->Msg_LeftUp).once().with(mc2_u).in(s).returns(true);
    MOCK_EXPECT(dsk->Msg_LeftDown).once().with(mc1).in(s).returns(true);
    MOCK_EXPECT(dsk->Msg_LeftUp).once().with(mc2_u).in(s).returns(true);
    WINDOWMANAGER.Msg_LeftDown(mc1);
    video->tickCount_ += 50;
    WINDOWMANAGER.Msg_LeftUp(mc1_u);
    video->tickCount_ += 2000;
    WINDOWMANAGER.Msg_LeftDown(mc2);
    video->tickCount_ += 50;
    WINDOWMANAGER.Msg_LeftUp(mc2_u);
    video->tickCount_ += 3000;
    WINDOWMANAGER.Msg_LeftDown(mc1);
    WINDOWMANAGER.Msg_LeftUp(mc2_u);
}

BOOST_FIXTURE_TEST_CASE(DblClick, WMFixture)
{
    video->tickCount_ = 0;
    mock::sequence s;
    MouseCoords mc1(5, 2, true);
    MouseCoords mc1_u(mc1.GetPos());
    MouseCoords mc2(6, 1, true);
    MouseCoords mc2_u(mc2.GetPos());
    // Click with time > DOUBLE_CLICK_INTERVAL is no dbl click
    MOCK_EXPECT(dsk->Msg_LeftDown).once().with(mc1).in(s).returns(true);
    MOCK_EXPECT(dsk->Msg_LeftUp).once().with(mc1_u).in(s).returns(true);
    MOCK_EXPECT(dsk->Msg_LeftDown).once().with(mc1).in(s).returns(true);
    MOCK_EXPECT(dsk->Msg_LeftUp).once().with(mc1_u).in(s).returns(true);
    // Click on different positions is no dbl click
    MOCK_EXPECT(dsk->Msg_LeftDown).once().with(mc2).in(s).returns(true);
    MOCK_EXPECT(dsk->Msg_LeftUp).once().with(mc2_u).in(s).returns(true);
    // Click on same pos with time < DOUBLE_CLICK_INTERVAL is dbl click
    MOCK_EXPECT(dsk->Msg_LeftDown).once().with(mc2).in(s).returns(true);
    MOCK_EXPECT(dsk->Msg_LeftUp).once().with(MouseCoords(mc2.GetPos(), false, false, true)).in(s).returns(true);
    // No triple click
    MOCK_EXPECT(dsk->Msg_LeftDown).once().with(mc2).in(s).returns(true);
    MOCK_EXPECT(dsk->Msg_LeftUp).once().with(mc2_u).in(s).returns(true);

    WINDOWMANAGER.Msg_LeftDown(mc1);
    video->tickCount_ += 1;
    WINDOWMANAGER.Msg_LeftUp(mc1_u);
    // time > DOUBLE_CLICK_INTERVAL
    video->tickCount_ += DOUBLE_CLICK_INTERVAL;
    WINDOWMANAGER.Msg_LeftDown(mc1);
    video->tickCount_ += 1;
    WINDOWMANAGER.Msg_LeftUp(mc1_u);
    // Different position
    WINDOWMANAGER.Msg_LeftDown(mc2);
    WINDOWMANAGER.Msg_LeftUp(mc2_u);
    // time < DOUBLE_CLICK_INTERVAL
    video->tickCount_ += DOUBLE_CLICK_INTERVAL - 1;
    WINDOWMANAGER.Msg_LeftDown(mc2);
    WINDOWMANAGER.Msg_LeftUp(mc2_u);
    // Triple?
    video->tickCount_ += DOUBLE_CLICK_INTERVAL - 1;
    WINDOWMANAGER.Msg_LeftDown(mc2);
    WINDOWMANAGER.Msg_LeftUp(mc2_u);
}

namespace {
/* clang-format off */
MOCK_BASE_CLASS(TestIngameWnd, IngameWindow)
{
    TestIngameWnd(unsigned id, bool isModal = false): IngameWindow(id, DrawPoint(0,0), Extent(100, 100), "", nullptr, isModal){
        closed.erase(std::remove(closed.begin(), closed.end(), this), closed.end());
    }
    ~TestIngameWnd() override
    {
        closed.push_back(this);
    }
    MOCK_METHOD(Draw_, 0, void())
    MOCK_METHOD(Msg_KeyDown, 1)
    static std::vector<TestIngameWnd*> closed;
};
std::vector<TestIngameWnd*> TestIngameWnd::closed;
/* clang-format on */

#define REQUIRE_WINDOW_ALIVE(wnd) BOOST_TEST_REQUIRE(!helpers::contains(TestIngameWnd::closed, wnd))
#define REQUIRE_WINDOW_ACTIVE(wnd) \
    REQUIRE_WINDOW_ALIVE(wnd);     \
    BOOST_TEST_REQUIRE(wnd->IsActive())
#define REQUIRE_WINDOW_DESTROYED(wnd) BOOST_TEST_REQUIRE(helpers::contains(TestIngameWnd::closed, wnd))
} // namespace

BOOST_FIXTURE_TEST_CASE(ShowIngameWnd, uiHelper::Fixture)
{
    // Note: We check the state after a Draw call as this is when a user notices it

    auto* wnd = new TestIngameWnd(CGI_NEXT + 42);
    MOCK_EXPECT(wnd->Draw_).once();
    WINDOWMANAGER.Show(wnd);
    WINDOWMANAGER.Draw();
    REQUIRE_WINDOW_ACTIVE(wnd);
    BOOST_TEST(!WINDOWMANAGER.GetCurrentDesktop()->IsActive());
    // Closing the window removes it and calls delete
    wnd->Close();
    WINDOWMANAGER.Draw();
    REQUIRE_WINDOW_DESTROYED(wnd);
    // Desktop active again
    BOOST_TEST(WINDOWMANAGER.GetCurrentDesktop()->IsActive());

    // Opening a window with the same ID closes the old and frees both
    wnd = new TestIngameWnd(CGI_NEXT + 42);
    WINDOWMANAGER.Show(wnd);
    auto* wnd2 = new TestIngameWnd(wnd->GetID());
    WINDOWMANAGER.Show(wnd2);
    WINDOWMANAGER.Draw();
    REQUIRE_WINDOW_DESTROYED(wnd);
    REQUIRE_WINDOW_DESTROYED(wnd2);
    // Desktop active again
    BOOST_TEST(WINDOWMANAGER.GetCurrentDesktop()->IsActive());

    // But closing a window and immediately reopening works
    wnd = new TestIngameWnd(CGI_NEXT + 42);
    WINDOWMANAGER.Show(wnd);
    wnd->Close();
    wnd2 = new TestIngameWnd(wnd->GetID());
    MOCK_EXPECT(wnd2->Draw_).once();
    WINDOWMANAGER.Show(wnd2);
    WINDOWMANAGER.Draw();
    REQUIRE_WINDOW_DESTROYED(wnd);
    REQUIRE_WINDOW_ACTIVE(wnd2);
    BOOST_TEST(!WINDOWMANAGER.GetCurrentDesktop()->IsActive());

    // Opening another window steals the focus
    BOOST_TEST_REQUIRE(wnd2->IsActive()); // Current situation check
    wnd = new TestIngameWnd(wnd2->GetID() + 1);
    MOCK_EXPECT(wnd2->Draw_).once();
    MOCK_EXPECT(wnd->Draw_).once();
    WINDOWMANAGER.Show(wnd);
    WINDOWMANAGER.Draw();
    REQUIRE_WINDOW_ALIVE(wnd2);
    REQUIRE_WINDOW_ACTIVE(wnd);
    BOOST_TEST(!wnd2->IsActive());
    BOOST_TEST(!WINDOWMANAGER.GetCurrentDesktop()->IsActive());
    // Immediate close and free -> next wnd is active
    WINDOWMANAGER.Close(wnd);
    REQUIRE_WINDOW_DESTROYED(wnd);
    REQUIRE_WINDOW_ACTIVE(wnd2);
    BOOST_TEST(!WINDOWMANAGER.GetCurrentDesktop()->IsActive());
    // Close by ID
    WINDOWMANAGER.Close(wnd2->GetID());
    REQUIRE_WINDOW_DESTROYED(wnd2);
    BOOST_TEST(WINDOWMANAGER.GetCurrentDesktop()->IsActive());

    // Help windows replace old help windows
    wnd = new TestIngameWnd(CGI_HELP);
    wnd2 = new TestIngameWnd(CGI_HELP);
    MOCK_EXPECT(wnd->Draw_).once();
    MOCK_EXPECT(wnd2->Draw_).once();
    WINDOWMANAGER.Show(wnd);
    WINDOWMANAGER.Draw();
    WINDOWMANAGER.Show(wnd2);
    WINDOWMANAGER.Draw();
    REQUIRE_WINDOW_DESTROYED(wnd);
    REQUIRE_WINDOW_ACTIVE(wnd2);
    BOOST_TEST(!WINDOWMANAGER.GetCurrentDesktop()->IsActive());
    WINDOWMANAGER.Close(wnd2);

    // new modal windows get placed before older ones
    wnd = new TestIngameWnd(CGI_MSGBOX, true);
    MOCK_EXPECT(wnd->Draw_).once();
    WINDOWMANAGER.Show(wnd);
    WINDOWMANAGER.Draw();
    REQUIRE_WINDOW_ACTIVE(wnd);
    wnd2 = new TestIngameWnd(CGI_MSGBOX, true);
    MOCK_EXPECT(wnd->Draw_).once();
    MOCK_EXPECT(wnd2->Draw_).once();
    WINDOWMANAGER.Show(wnd2);
    WINDOWMANAGER.Draw();
    REQUIRE_WINDOW_ACTIVE(wnd);
    auto* wnd3 = new TestIngameWnd(CGI_MISSION_STATEMENT, true);
    MOCK_EXPECT(wnd->Draw_).once();
    MOCK_EXPECT(wnd2->Draw_).once();
    MOCK_EXPECT(wnd3->Draw_).once();
    WINDOWMANAGER.Show(wnd3);
    WINDOWMANAGER.Draw();
    REQUIRE_WINDOW_ACTIVE(wnd);
    auto* wnd4 = new TestIngameWnd(CGI_MSGBOX);
    MOCK_EXPECT(wnd->Draw_).once();
    MOCK_EXPECT(wnd2->Draw_).once();
    MOCK_EXPECT(wnd3->Draw_).once();
    MOCK_EXPECT(wnd4->Draw_).once();
    WINDOWMANAGER.Show(wnd4);
    WINDOWMANAGER.Draw();
    REQUIRE_WINDOW_ACTIVE(wnd);
    auto* wnd5 = new TestIngameWnd(CGI_NEXT + 42, true);
    MOCK_EXPECT(wnd->Draw_).once();
    MOCK_EXPECT(wnd2->Draw_).once();
    MOCK_EXPECT(wnd3->Draw_).once();
    MOCK_EXPECT(wnd4->Draw_).once();
    MOCK_EXPECT(wnd5->Draw_).once();
    WINDOWMANAGER.Show(wnd5);
    WINDOWMANAGER.Draw();
    REQUIRE_WINDOW_ACTIVE(wnd);
    auto* wnd6 = new TestIngameWnd(CGI_NEXT + 43);
    MOCK_EXPECT(wnd->Draw_).once();
    MOCK_EXPECT(wnd2->Draw_).once();
    MOCK_EXPECT(wnd3->Draw_).once();
    MOCK_EXPECT(wnd4->Draw_).once();
    MOCK_EXPECT(wnd5->Draw_).once();
    MOCK_EXPECT(wnd6->Draw_).once();
    WINDOWMANAGER.Show(wnd6);
    WINDOWMANAGER.Draw();
    REQUIRE_WINDOW_ACTIVE(wnd);
    // Now we have the following order
    std::vector<TestIngameWnd*> expectedOrder;
    expectedOrder.push_back(wnd);
    expectedOrder.push_back(wnd2);
    expectedOrder.push_back(wnd3);
    expectedOrder.push_back(wnd5);
    expectedOrder.push_back(wnd6);
    expectedOrder.push_back(wnd4);
    // Only way to check the order is to simulate a key event, expect the top most one to handle it and close it, then proceed
    mock::sequence s;
    for(TestIngameWnd* curWnd : expectedOrder)
        MOCK_EXPECT(curWnd->Msg_KeyDown).once().in(s).returns(true);
    // Way outside any window, should still be handled
    KeyEvent ke{KT_CHAR, 'a', false, false, false};
    for(TestIngameWnd* curWnd : expectedOrder)
    {
        REQUIRE_WINDOW_ACTIVE(curWnd);
        WINDOWMANAGER.Msg_KeyDown(ke);
        WINDOWMANAGER.Close(curWnd);
    }
}

BOOST_AUTO_TEST_SUITE_END()
