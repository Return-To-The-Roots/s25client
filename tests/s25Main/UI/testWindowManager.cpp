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

#include "PointOutput.h"
#include "WindowManager.h"
#include "desktops/Desktop.h"
#include "helpers/containerUtils.h"
#include "ingameWindows/IngameWindow.h"
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
    MOCK_METHOD(Msg_LeftUp, 1)
    MOCK_METHOD(Msg_MouseMove, 1)
};
/* clang-format on */

struct WMFixture
{
    MockupVideoDriver* video;
    TestDesktop* dsk;
    WMFixture() : video(uiHelper::GetVideoDriver())
    {
        dsk = static_cast<TestDesktop*>(WINDOWMANAGER.Switch(std::make_unique<TestDesktop>()));
        MOCK_EXPECT(dsk->Msg_MouseMove).once().returns(true);
        WINDOWMANAGER.Draw();
        mock::verify(*dsk);
        mock::reset(*dsk);
    }
};
} // namespace

BOOST_AUTO_TEST_SUITE(WindowManagerSuite)

BOOST_AUTO_TEST_CASE(MouseCoordsOutput)
{
    std::stringstream s;
    s << MouseCoords(Position(2, 3), true, false, true);
    BOOST_TEST(s.str() == "<(2, 3),1,0,1>");
}

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
    explicit TestIngameWnd(unsigned id, bool isModal = false): IngameWindow(id, DrawPoint(0,0), Extent(100, 100), "", nullptr, isModal){
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
    BOOST_TEST_REQUIRE((wnd)->IsActive())
#define REQUIRE_WINDOW_DESTROYED(wnd) BOOST_TEST_REQUIRE(helpers::contains(TestIngameWnd::closed, wnd))
} // namespace

// Note for all tests: We check the state after a Draw call as this is when a user notices it

BOOST_FIXTURE_TEST_CASE(ShowIngameWnd, uiHelper::Fixture)
{
    auto* wnd = &WINDOWMANAGER.Show(std::make_unique<TestIngameWnd>(CGI_HELP));
    BOOST_TEST_REQUIRE(wnd);
    MOCK_EXPECT(wnd->Draw_).once();
    WINDOWMANAGER.Draw();
    REQUIRE_WINDOW_ACTIVE(wnd);
    BOOST_TEST(!WINDOWMANAGER.GetCurrentDesktop()->IsActive());
    // Closing the window removes it and calls delete
    wnd->Close();
    WINDOWMANAGER.Draw();
    REQUIRE_WINDOW_DESTROYED(wnd);
    // Desktop active again
    BOOST_TEST(WINDOWMANAGER.GetCurrentDesktop()->IsActive());

    // Opening a window with the same ID works. The last gets the focus
    wnd = &WINDOWMANAGER.Show(std::make_unique<TestIngameWnd>(CGI_HELP));
    auto* wnd2 = &WINDOWMANAGER.Show(std::make_unique<TestIngameWnd>(CGI_HELP));
    BOOST_TEST_REQUIRE((wnd && wnd2));
    MOCK_EXPECT(wnd->Draw_).once();
    MOCK_EXPECT(wnd2->Draw_).once();
    WINDOWMANAGER.Draw();
    REQUIRE_WINDOW_ALIVE(wnd);
    REQUIRE_WINDOW_ACTIVE(wnd2);
    // Desktop inactive again
    BOOST_TEST(!WINDOWMANAGER.GetCurrentDesktop()->IsActive());

    // Closing a window and immediately reopening works
    wnd->Close();
    wnd2->Close();
    wnd2 = &WINDOWMANAGER.Show(std::make_unique<TestIngameWnd>(wnd->GetID()));
    BOOST_TEST_REQUIRE(wnd2);
    MOCK_EXPECT(wnd2->Draw_).once();
    WINDOWMANAGER.Draw();
    REQUIRE_WINDOW_DESTROYED(wnd);
    REQUIRE_WINDOW_ACTIVE(wnd2);
    BOOST_TEST(!WINDOWMANAGER.GetCurrentDesktop()->IsActive());

    // Close by ID
    WINDOWMANAGER.Close(wnd2->GetID());
    REQUIRE_WINDOW_DESTROYED(wnd2);
    BOOST_TEST(WINDOWMANAGER.GetCurrentDesktop()->IsActive());
}

BOOST_FIXTURE_TEST_CASE(ToggleIngameWnd, uiHelper::Fixture)
{
    // When no window with the ID is open, then this is just Show
    auto* wnd = WINDOWMANAGER.ToggleWindow(std::make_unique<TestIngameWnd>(CGI_HELP));
    BOOST_TEST_REQUIRE(wnd);
    MOCK_EXPECT(wnd->Draw_).once();
    WINDOWMANAGER.Draw();
    REQUIRE_WINDOW_ACTIVE(wnd);
    BOOST_TEST(!WINDOWMANAGER.GetCurrentDesktop()->IsActive());

    // When window is about to be closed we can open a new one
    wnd->Close();
    auto* wnd2 = WINDOWMANAGER.ToggleWindow(std::make_unique<TestIngameWnd>(CGI_HELP));
    BOOST_TEST_REQUIRE(wnd2);
    MOCK_EXPECT(wnd2->Draw_).once();
    WINDOWMANAGER.Draw();
    REQUIRE_WINDOW_ACTIVE(wnd2);
    REQUIRE_WINDOW_DESTROYED(wnd);
    BOOST_TEST(!WINDOWMANAGER.GetCurrentDesktop()->IsActive());

    // Opening a window with the same ID closes and frees both
    wnd = wnd2;
    wnd2 = new TestIngameWnd(wnd->GetID());
    BOOST_TEST((WINDOWMANAGER.ToggleWindow(std::unique_ptr<TestIngameWnd>(wnd2)) == nullptr));
    WINDOWMANAGER.Draw();
    REQUIRE_WINDOW_DESTROYED(wnd);
    REQUIRE_WINDOW_DESTROYED(wnd2);
    // Desktop active again
    BOOST_TEST(WINDOWMANAGER.GetCurrentDesktop()->IsActive());

    // Windows with different IDs are fine
    wnd = WINDOWMANAGER.ToggleWindow(std::make_unique<TestIngameWnd>(CGI_HELP));
    wnd2 = WINDOWMANAGER.ToggleWindow(std::make_unique<TestIngameWnd>(CGI_SETTINGS));
    BOOST_TEST_REQUIRE((wnd && wnd2));
    MOCK_EXPECT(wnd->Draw_).once();
    MOCK_EXPECT(wnd2->Draw_).once();
    WINDOWMANAGER.Draw();
    REQUIRE_WINDOW_ALIVE(wnd);
    REQUIRE_WINDOW_ACTIVE(wnd2);
    wnd->Close();
    wnd2->Close();
}

BOOST_FIXTURE_TEST_CASE(ReplaceIngameWnd, uiHelper::Fixture)
{
    // When no window with the ID is open, then this is just Show
    auto* wnd = &WINDOWMANAGER.ReplaceWindow(std::make_unique<TestIngameWnd>(CGI_HELP));
    MOCK_EXPECT(wnd->Draw_).once();
    WINDOWMANAGER.Draw();
    REQUIRE_WINDOW_ACTIVE(wnd);
    BOOST_TEST(!WINDOWMANAGER.GetCurrentDesktop()->IsActive());

    // When window is about to be closed we can open a new one
    wnd->Close();
    auto* wnd2 = &WINDOWMANAGER.ReplaceWindow(std::make_unique<TestIngameWnd>(CGI_HELP));
    BOOST_TEST_REQUIRE(wnd2);
    MOCK_EXPECT(wnd2->Draw_).once();
    WINDOWMANAGER.Draw();
    REQUIRE_WINDOW_ACTIVE(wnd2);
    REQUIRE_WINDOW_DESTROYED(wnd);
    BOOST_TEST(!WINDOWMANAGER.GetCurrentDesktop()->IsActive());

    // Opening a window with the same ID closes and frees the first
    wnd = wnd2;
    wnd2 = &WINDOWMANAGER.ReplaceWindow(std::make_unique<TestIngameWnd>(CGI_HELP));
    BOOST_TEST_REQUIRE(wnd2);
    MOCK_EXPECT(wnd2->Draw_).once();
    WINDOWMANAGER.Draw();
    REQUIRE_WINDOW_DESTROYED(wnd);
    REQUIRE_WINDOW_ACTIVE(wnd2);
    BOOST_TEST(!WINDOWMANAGER.GetCurrentDesktop()->IsActive());

    // Windows with different IDs are fine
    wnd = wnd2;
    wnd2 = &WINDOWMANAGER.ReplaceWindow(std::make_unique<TestIngameWnd>(CGI_SETTINGS));
    BOOST_TEST_REQUIRE(wnd2);
    MOCK_EXPECT(wnd->Draw_).once();
    MOCK_EXPECT(wnd2->Draw_).once();
    WINDOWMANAGER.Draw();
    REQUIRE_WINDOW_ALIVE(wnd);
    REQUIRE_WINDOW_ACTIVE(wnd2);
    wnd->Close();
    wnd2->Close();

    // Modal windows are not replaced but placed behind existing ones
    wnd = &WINDOWMANAGER.ReplaceWindow(std::make_unique<TestIngameWnd>(CGI_SETTINGS, true));
    wnd2 = &WINDOWMANAGER.ReplaceWindow(std::make_unique<TestIngameWnd>(CGI_SETTINGS, true));
    BOOST_TEST_REQUIRE((wnd && wnd2));
    MOCK_EXPECT(wnd->Draw_).once();
    MOCK_EXPECT(wnd2->Draw_).once();
    WINDOWMANAGER.Draw();
    REQUIRE_WINDOW_ACTIVE(wnd);
    REQUIRE_WINDOW_ALIVE(wnd2);
    wnd->Close();
    wnd2->Close();
}

BOOST_FIXTURE_TEST_CASE(ModalWindowPlacement, uiHelper::Fixture)
{
    // new modal windows get placed before older ones
    auto& wnd = WINDOWMANAGER.ReplaceWindow(std::make_unique<TestIngameWnd>(CGI_MSGBOX, true));
    MOCK_EXPECT(wnd.Draw_).once();
    WINDOWMANAGER.Draw();
    REQUIRE_WINDOW_ACTIVE(&wnd);
    auto& wnd2 = WINDOWMANAGER.ReplaceWindow(std::make_unique<TestIngameWnd>(CGI_MSGBOX, true));
    MOCK_EXPECT(wnd.Draw_).once();
    MOCK_EXPECT(wnd2.Draw_).once();
    WINDOWMANAGER.Draw();
    REQUIRE_WINDOW_ACTIVE(&wnd);
    auto& wnd3 = WINDOWMANAGER.ReplaceWindow(std::make_unique<TestIngameWnd>(CGI_MISSION_STATEMENT, true));
    MOCK_EXPECT(wnd.Draw_).once();
    MOCK_EXPECT(wnd2.Draw_).once();
    MOCK_EXPECT(wnd3.Draw_).once();
    WINDOWMANAGER.Draw();
    REQUIRE_WINDOW_ACTIVE(&wnd);
    auto& wnd4 = WINDOWMANAGER.ReplaceWindow(std::make_unique<TestIngameWnd>(CGI_MSGBOX));
    MOCK_EXPECT(wnd.Draw_).once();
    MOCK_EXPECT(wnd2.Draw_).once();
    MOCK_EXPECT(wnd3.Draw_).once();
    MOCK_EXPECT(wnd4.Draw_).once();
    WINDOWMANAGER.Draw();
    REQUIRE_WINDOW_ACTIVE(&wnd);
    auto& wnd5 = WINDOWMANAGER.ReplaceWindow(std::make_unique<TestIngameWnd>(CGI_HELP, true));
    MOCK_EXPECT(wnd.Draw_).once();
    MOCK_EXPECT(wnd2.Draw_).once();
    MOCK_EXPECT(wnd3.Draw_).once();
    MOCK_EXPECT(wnd4.Draw_).once();
    MOCK_EXPECT(wnd5.Draw_).once();
    WINDOWMANAGER.Draw();
    REQUIRE_WINDOW_ACTIVE(&wnd);
    auto& wnd6 = WINDOWMANAGER.ReplaceWindow(std::make_unique<TestIngameWnd>(CGI_SETTINGS));
    MOCK_EXPECT(wnd.Draw_).once();
    MOCK_EXPECT(wnd2.Draw_).once();
    MOCK_EXPECT(wnd3.Draw_).once();
    MOCK_EXPECT(wnd4.Draw_).once();
    MOCK_EXPECT(wnd5.Draw_).once();
    MOCK_EXPECT(wnd6.Draw_).once();
    WINDOWMANAGER.Draw();
    REQUIRE_WINDOW_ACTIVE(&wnd);
    // Now we have the following order
    std::vector<TestIngameWnd*> expectedOrder = {&wnd, &wnd2, &wnd3, &wnd5, &wnd6, &wnd4};
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
