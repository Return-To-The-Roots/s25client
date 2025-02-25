// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "DrawPoint.h"
#include "PointOutput.h"
#include "Settings.h"
#include "WindowManager.h"
#include "desktops/Desktop.h"
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
    return lhs.GetPos() == rhs.GetPos() && lhs.ldown == rhs.ldown && lhs.rdown == rhs.rdown
           && lhs.dbl_click == rhs.dbl_click;
}

inline std::ostream& operator<<(std::ostream& s, const MouseCoords& mc)
{
    return s << "<" << mc.GetPos() << "," << mc.ldown << "," << mc.rdown << "," << mc.dbl_click << ">";
}

namespace {
MOCK_BASE_CLASS(TestDesktop, Desktop)
{
    TestDesktop() : Desktop(nullptr) {}
    MOCK_METHOD(Msg_LeftDown, 1)
    MOCK_METHOD(Msg_LeftUp, 1)
    MOCK_METHOD(Msg_MouseMove, 1)
};

struct WMFixture : mock::cleanup
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
    mock::verify();
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
    MOCK_METHOD(DrawContent, 0, void())
    MOCK_METHOD(Msg_KeyDown, 1)
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
    auto* wnd = &WINDOWMANAGER.Show(std::make_unique<TestIngameWnd>(CGI_HELP));
    BOOST_TEST_REQUIRE(wnd);
    MOCK_EXPECT(wnd->DrawContent).once();
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
    MOCK_EXPECT(wnd->DrawContent).once();
    MOCK_EXPECT(wnd2->DrawContent).once();
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
    MOCK_EXPECT(wnd2->DrawContent).once();
    WINDOWMANAGER.Draw();
    REQUIRE_WINDOW_DESTROYED(wnd);
    REQUIRE_WINDOW_ACTIVE(wnd2);
    BOOST_TEST(!WINDOWMANAGER.GetCurrentDesktop()->IsActive());

    // Close by ID
    WINDOWMANAGER.Close(wnd2->GetID());
    WINDOWMANAGER.Draw();
    REQUIRE_WINDOW_DESTROYED(wnd2);
    BOOST_TEST(WINDOWMANAGER.GetCurrentDesktop()->IsActive());
    mock::verify();
}

BOOST_FIXTURE_TEST_CASE(ToggleIngameWnd, uiHelper::Fixture)
{
    // When no window with the ID is open, then this is just Show
    auto* wnd = WINDOWMANAGER.ToggleWindow(std::make_unique<TestIngameWnd>(CGI_HELP));
    BOOST_TEST_REQUIRE(wnd);
    MOCK_EXPECT(wnd->DrawContent).once();
    WINDOWMANAGER.Draw();
    REQUIRE_WINDOW_ACTIVE(wnd);
    BOOST_TEST(!WINDOWMANAGER.GetCurrentDesktop()->IsActive());

    // When window is about to be closed we can open a new one
    wnd->Close();
    auto* wnd2 = WINDOWMANAGER.ToggleWindow(std::make_unique<TestIngameWnd>(CGI_HELP));
    BOOST_TEST_REQUIRE(wnd2);
    MOCK_EXPECT(wnd2->DrawContent).once();
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
    MOCK_EXPECT(wnd->DrawContent).once();
    MOCK_EXPECT(wnd2->DrawContent).once();
    WINDOWMANAGER.Draw();
    REQUIRE_WINDOW_ALIVE(wnd);
    REQUIRE_WINDOW_ACTIVE(wnd2);
    wnd->Close();
    wnd2->Close();
    mock::verify();
}

BOOST_FIXTURE_TEST_CASE(ReplaceIngameWnd, uiHelper::Fixture)
{
    // When no window with the ID is open, then this is just Show
    auto* wnd = &WINDOWMANAGER.ReplaceWindow(std::make_unique<TestIngameWnd>(CGI_HELP));
    MOCK_EXPECT(wnd->DrawContent).once();
    WINDOWMANAGER.Draw();
    REQUIRE_WINDOW_ACTIVE(wnd);
    BOOST_TEST(!WINDOWMANAGER.GetCurrentDesktop()->IsActive());

    // When window is about to be closed we can open a new one
    wnd->Close();
    auto* wnd2 = &WINDOWMANAGER.ReplaceWindow(std::make_unique<TestIngameWnd>(CGI_HELP));
    BOOST_TEST_REQUIRE(wnd2);
    MOCK_EXPECT(wnd2->DrawContent).once();
    WINDOWMANAGER.Draw();
    REQUIRE_WINDOW_ACTIVE(wnd2);
    REQUIRE_WINDOW_DESTROYED(wnd);
    BOOST_TEST(!WINDOWMANAGER.GetCurrentDesktop()->IsActive());

    // Opening a window with the same ID closes and frees the first
    wnd = wnd2;
    wnd2 = &WINDOWMANAGER.ReplaceWindow(std::make_unique<TestIngameWnd>(CGI_HELP));
    BOOST_TEST_REQUIRE(wnd2);
    MOCK_EXPECT(wnd2->DrawContent).once();
    WINDOWMANAGER.Draw();
    REQUIRE_WINDOW_DESTROYED(wnd);
    REQUIRE_WINDOW_ACTIVE(wnd2);
    BOOST_TEST(!WINDOWMANAGER.GetCurrentDesktop()->IsActive());

    // Windows with different IDs are fine
    wnd = wnd2;
    wnd2 = &WINDOWMANAGER.ReplaceWindow(std::make_unique<TestIngameWnd>(CGI_SETTINGS));
    BOOST_TEST_REQUIRE(wnd2);
    MOCK_EXPECT(wnd->DrawContent).once();
    MOCK_EXPECT(wnd2->DrawContent).once();
    WINDOWMANAGER.Draw();
    REQUIRE_WINDOW_ALIVE(wnd);
    REQUIRE_WINDOW_ACTIVE(wnd2);
    wnd->Close();
    wnd2->Close();

    // Modal windows are not replaced but placed behind existing ones
    wnd = &WINDOWMANAGER.ReplaceWindow(std::make_unique<TestIngameWnd>(CGI_SETTINGS, true));
    wnd2 = &WINDOWMANAGER.ReplaceWindow(std::make_unique<TestIngameWnd>(CGI_SETTINGS, true));
    BOOST_TEST_REQUIRE((wnd && wnd2));
    MOCK_EXPECT(wnd->DrawContent).once();
    MOCK_EXPECT(wnd2->DrawContent).once();
    WINDOWMANAGER.Draw();
    REQUIRE_WINDOW_ACTIVE(wnd);
    REQUIRE_WINDOW_ALIVE(wnd2);
    wnd->Close();
    wnd2->Close();
    mock::verify();
}

BOOST_FIXTURE_TEST_CASE(ModalWindowPlacement, uiHelper::Fixture)
{
    // new modal windows get placed before older ones
    auto& wnd = WINDOWMANAGER.ReplaceWindow(std::make_unique<TestIngameWnd>(CGI_MSGBOX, true));
    MOCK_EXPECT(wnd.DrawContent).once();
    WINDOWMANAGER.Draw();
    REQUIRE_WINDOW_ACTIVE(&wnd);
    auto& wnd2 = WINDOWMANAGER.ReplaceWindow(std::make_unique<TestIngameWnd>(CGI_MSGBOX, true));
    MOCK_EXPECT(wnd.DrawContent).once();
    MOCK_EXPECT(wnd2.DrawContent).once();
    WINDOWMANAGER.Draw();
    REQUIRE_WINDOW_ACTIVE(&wnd);
    auto& wnd3 = WINDOWMANAGER.ReplaceWindow(std::make_unique<TestIngameWnd>(CGI_MISSION_STATEMENT, true));
    MOCK_EXPECT(wnd.DrawContent).once();
    MOCK_EXPECT(wnd2.DrawContent).once();
    MOCK_EXPECT(wnd3.DrawContent).once();
    WINDOWMANAGER.Draw();
    REQUIRE_WINDOW_ACTIVE(&wnd);
    auto& wnd4 = WINDOWMANAGER.ReplaceWindow(std::make_unique<TestIngameWnd>(CGI_MSGBOX));
    MOCK_EXPECT(wnd.DrawContent).once();
    MOCK_EXPECT(wnd2.DrawContent).once();
    MOCK_EXPECT(wnd3.DrawContent).once();
    MOCK_EXPECT(wnd4.DrawContent).once();
    WINDOWMANAGER.Draw();
    REQUIRE_WINDOW_ACTIVE(&wnd);
    auto& wnd5 = WINDOWMANAGER.ReplaceWindow(std::make_unique<TestIngameWnd>(CGI_HELP, true));
    MOCK_EXPECT(wnd.DrawContent).once();
    MOCK_EXPECT(wnd2.DrawContent).once();
    MOCK_EXPECT(wnd3.DrawContent).once();
    MOCK_EXPECT(wnd4.DrawContent).once();
    MOCK_EXPECT(wnd5.DrawContent).once();
    WINDOWMANAGER.Draw();
    REQUIRE_WINDOW_ACTIVE(&wnd);
    auto& wnd6 = WINDOWMANAGER.ReplaceWindow(std::make_unique<TestIngameWnd>(CGI_SETTINGS));
    MOCK_EXPECT(wnd.DrawContent).once();
    MOCK_EXPECT(wnd2.DrawContent).once();
    MOCK_EXPECT(wnd3.DrawContent).once();
    MOCK_EXPECT(wnd4.DrawContent).once();
    MOCK_EXPECT(wnd5.DrawContent).once();
    MOCK_EXPECT(wnd6.DrawContent).once();
    WINDOWMANAGER.Draw();
    REQUIRE_WINDOW_ACTIVE(&wnd);
    // Now we have the following order
    std::vector<TestIngameWnd*> expectedOrder = {&wnd, &wnd2, &wnd3, &wnd5, &wnd6, &wnd4};
    // Only way to check the order is to simulate a key event, expect the top most one to handle it and close it, then
    // proceed
    mock::sequence s;
    for(TestIngameWnd* curWnd : expectedOrder)
    {
        MOCK_EXPECT(curWnd->Msg_KeyDown).once().in(s).returns(true);
        MOCK_EXPECT(curWnd->DrawContent); // Ignore all draw calls
    }
    // Way outside any window, should still be handled
    KeyEvent ke{KeyType::Char, 'a', false, false, false};
    for(TestIngameWnd* curWnd : expectedOrder)
    {
        REQUIRE_WINDOW_ACTIVE(curWnd);
        WINDOWMANAGER.Msg_KeyDown(ke);
        curWnd->Close();
        WINDOWMANAGER.Draw();
    }
    mock::verify();
}

BOOST_FIXTURE_TEST_CASE(EscClosesWindow, uiHelper::Fixture)
{
    auto* wnd = &WINDOWMANAGER.Show(std::make_unique<TestIngameWnd>(CGI_HELP));
    BOOST_TEST_REQUIRE(WINDOWMANAGER.GetTopMostWindow() == wnd);
    KeyEvent evEsc{KeyType::Escape, 0, false, false, false};
    WINDOWMANAGER.Msg_KeyDown(evEsc);
    WINDOWMANAGER.Draw();
    BOOST_TEST_REQUIRE(WINDOWMANAGER.GetTopMostWindow() == nullptr);
    REQUIRE_WINDOW_DESTROYED(wnd);

    // Multiple escapes close multiple windows, even modal ones
    auto* wnd1 = &WINDOWMANAGER.Show(std::make_unique<TestIngameWnd>(CGI_HELP));
    auto* wnd2 = &WINDOWMANAGER.Show(std::make_unique<TestIngameWnd>(CGI_HELP));
    auto* wnd3 = &WINDOWMANAGER.Show(std::make_unique<TestIngameWnd>(CGI_HELP, true));
    WINDOWMANAGER.Msg_KeyDown(evEsc);
    WINDOWMANAGER.Msg_KeyDown(evEsc);
    MOCK_EXPECT(wnd1->DrawContent).once();
    WINDOWMANAGER.Draw();
    BOOST_TEST_REQUIRE(WINDOWMANAGER.GetTopMostWindow() == wnd1);
    REQUIRE_WINDOW_DESTROYED(wnd2);
    REQUIRE_WINDOW_DESTROYED(wnd3);
    WINDOWMANAGER.Msg_KeyDown(evEsc);
    WINDOWMANAGER.Draw();
    REQUIRE_WINDOW_DESTROYED(wnd1);

    // ESC does not close non-user-closable windows
    wnd1 = &WINDOWMANAGER.Show(std::make_unique<TestIngameWnd>(CGI_HELP, false, CloseBehavior::Custom));
    BOOST_TEST_REQUIRE(WINDOWMANAGER.GetTopMostWindow() == wnd1);
    WINDOWMANAGER.Msg_KeyDown(evEsc);
    REQUIRE_WINDOW_ALIVE(wnd1);
    MOCK_EXPECT(wnd1->DrawContent).once();
    WINDOWMANAGER.Draw();
    BOOST_TEST_REQUIRE(WINDOWMANAGER.GetTopMostWindow() == wnd1);

    wnd2 = &WINDOWMANAGER.Show(std::make_unique<TestIngameWnd>(CGI_HELP, true, CloseBehavior::Custom));
    BOOST_TEST_REQUIRE(WINDOWMANAGER.GetTopMostWindow() == wnd2);
    WINDOWMANAGER.Msg_KeyDown(evEsc);
    REQUIRE_WINDOW_ALIVE(wnd1);
    REQUIRE_WINDOW_ALIVE(wnd2);
    MOCK_EXPECT(wnd1->DrawContent).once();
    MOCK_EXPECT(wnd2->DrawContent).once();
    WINDOWMANAGER.Draw();
    BOOST_TEST_REQUIRE(WINDOWMANAGER.GetTopMostWindow() == wnd2);
}

BOOST_FIXTURE_TEST_CASE(RightclickClosesWindow, uiHelper::Fixture)
{
    auto* wnd = &WINDOWMANAGER.Show(std::make_unique<TestIngameWnd>(CGI_HELP));
    BOOST_TEST_REQUIRE(WINDOWMANAGER.GetTopMostWindow() == wnd);
    const MouseCoords evRDown(wnd->GetDrawPos() + Position(10, 10), false, true);
    WINDOWMANAGER.Msg_RightDown(evRDown);
    WINDOWMANAGER.Draw();
    BOOST_TEST_REQUIRE(WINDOWMANAGER.GetTopMostWindow() == nullptr);
    REQUIRE_WINDOW_DESTROYED(wnd);

    // Only close top most window
    auto* wnd1 = &WINDOWMANAGER.Show(std::make_unique<TestIngameWnd>(CGI_HELP));
    auto* wnd2 = &WINDOWMANAGER.Show(std::make_unique<TestIngameWnd>(CGI_HELP));
    WINDOWMANAGER.Msg_RightDown(evRDown);
    MOCK_EXPECT(wnd1->DrawContent).once();
    WINDOWMANAGER.Draw();
    BOOST_TEST_REQUIRE(WINDOWMANAGER.GetTopMostWindow() == wnd1);
    REQUIRE_WINDOW_DESTROYED(wnd2);
    // Also modal windows, even when not opened last
    wnd2 = &WINDOWMANAGER.Show(std::make_unique<TestIngameWnd>(CGI_HELP, true));
    auto* wnd3 = &WINDOWMANAGER.Show(std::make_unique<TestIngameWnd>(CGI_HELP));
    WINDOWMANAGER.Msg_RightDown(evRDown);
    REQUIRE_WINDOW_ALIVE(wnd1);
    REQUIRE_WINDOW_ALIVE(wnd3);
    MOCK_EXPECT(wnd1->DrawContent).once();
    MOCK_EXPECT(wnd3->DrawContent).once();
    WINDOWMANAGER.Draw();
    BOOST_TEST_REQUIRE(WINDOWMANAGER.GetTopMostWindow() == wnd3);
    REQUIRE_WINDOW_DESTROYED(wnd2);
    WINDOWMANAGER.Msg_RightDown(evRDown);
    MOCK_EXPECT(wnd1->DrawContent).once();
    WINDOWMANAGER.Draw();
    WINDOWMANAGER.Msg_RightDown(evRDown);
    WINDOWMANAGER.Draw();
    REQUIRE_WINDOW_DESTROYED(wnd1);
    REQUIRE_WINDOW_DESTROYED(wnd3);

    // Don't close non-user-closable windows
    wnd1 = &WINDOWMANAGER.Show(std::make_unique<TestIngameWnd>(CGI_HELP, false, CloseBehavior::Custom));
    BOOST_TEST_REQUIRE(WINDOWMANAGER.GetTopMostWindow() == wnd1);
    WINDOWMANAGER.Msg_RightDown(evRDown);
    MOCK_EXPECT(wnd1->DrawContent).once();
    WINDOWMANAGER.Draw();
    BOOST_TEST_REQUIRE(WINDOWMANAGER.GetTopMostWindow() == wnd1);

    wnd2 = &WINDOWMANAGER.Show(std::make_unique<TestIngameWnd>(CGI_HELP, true, CloseBehavior::NoRightClick));
    BOOST_TEST_REQUIRE(WINDOWMANAGER.GetTopMostWindow() == wnd2);
    WINDOWMANAGER.Msg_RightDown(evRDown);
    MOCK_EXPECT(wnd1->DrawContent).once();
    MOCK_EXPECT(wnd2->DrawContent).once();
    WINDOWMANAGER.Draw();
    BOOST_TEST_REQUIRE(WINDOWMANAGER.GetTopMostWindow() == wnd2);
}

BOOST_FIXTURE_TEST_CASE(PinnedWindows, uiHelper::Fixture)
{
    constexpr KeyEvent evEsc{KeyType::Escape, 0, false, false, false};
    constexpr KeyEvent evAltW{KeyType::Char, 'w', false, false, true};

    BOOST_TEST_CONTEXT("Pinned windows ignore escape key")
    {
        auto* wnd = &WINDOWMANAGER.Show(std::make_unique<TestIngameWnd>(CGI_HELP));

        wnd->SetPinned();
        BOOST_TEST_CONTEXT("Pinned")
        {
            BOOST_TEST_REQUIRE(WINDOWMANAGER.GetTopMostWindow() == wnd);
            WINDOWMANAGER.Msg_KeyDown(evEsc);
            MOCK_EXPECT(wnd->DrawContent).once();
            WINDOWMANAGER.Draw();
            BOOST_TEST_REQUIRE(WINDOWMANAGER.GetTopMostWindow() == wnd);
        }

        wnd->SetPinned(false);
        BOOST_TEST_CONTEXT("Un-pinned")
        {
            WINDOWMANAGER.Msg_KeyDown(evEsc);
            WINDOWMANAGER.Draw();
            REQUIRE_WINDOW_DESTROYED(wnd);
        }
    }

    BOOST_TEST_CONTEXT("Pinned windows close on Alt+W")
    {
        auto* wnd = &WINDOWMANAGER.Show(std::make_unique<TestIngameWnd>(CGI_HELP));

        wnd->SetPinned();
        BOOST_TEST_CONTEXT("Pinned")
        {
            BOOST_TEST_REQUIRE(WINDOWMANAGER.GetTopMostWindow() == wnd);
            WINDOWMANAGER.Msg_KeyDown(evAltW);
            WINDOWMANAGER.Draw();
            REQUIRE_WINDOW_DESTROYED(wnd);
        }
    }

    BOOST_TEST_CONTEXT("Pinned windows ignore right click")
    {
        auto* wnd = &WINDOWMANAGER.Show(std::make_unique<TestIngameWnd>(CGI_HELP));
        BOOST_TEST_REQUIRE(WINDOWMANAGER.GetTopMostWindow() == wnd);
        const MouseCoords evRDown(wnd->GetDrawPos() + Position(10, 10), false, true);

        wnd->SetPinned();
        BOOST_TEST_CONTEXT("Pinned")
        {
            WINDOWMANAGER.Msg_RightDown(evRDown);
            MOCK_EXPECT(wnd->DrawContent).once();
            WINDOWMANAGER.Draw();
            BOOST_TEST_REQUIRE(WINDOWMANAGER.GetTopMostWindow() == wnd);
        }

        wnd->SetPinned(false);
        BOOST_TEST_CONTEXT("Un-pinned")
        {
            WINDOWMANAGER.Msg_RightDown(evRDown);
            WINDOWMANAGER.Draw();
            REQUIRE_WINDOW_DESTROYED(wnd);
        }
    }

    mock::verify();
}

BOOST_FIXTURE_TEST_CASE(SnapWindow, uiHelper::Fixture)
{
    SETTINGS.interface.windowSnapDistance = 10;

    WINDOWMANAGER.Show(std::make_unique<IngameWindow>(0, DrawPoint(0, 0), Extent(100, 100), "", nullptr));
    auto* wnd =
      &WINDOWMANAGER.Show(std::make_unique<IngameWindow>(0, DrawPoint(100, 0), Extent(100, 100), "", nullptr));

    BOOST_TEST(WINDOWMANAGER.snapWindow(wnd, wnd->GetBoundaryRect()) == SnapOffset(0, 0));

    wnd->SetPos(DrawPoint(111, 5));
    BOOST_TEST(WINDOWMANAGER.snapWindow(wnd, wnd->GetBoundaryRect()) == SnapOffset(0, 0));

    wnd->SetPos(DrawPoint(110, 5));
    BOOST_TEST(WINDOWMANAGER.snapWindow(wnd, wnd->GetBoundaryRect()) == SnapOffset(-10, -5));
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
    MOCK_NON_CONST_METHOD(UpdateSettings, 0); // LCOV_EXCL_LINE
    MOCK_NON_CONST_METHOD(TransmitSettings, 0);
};

int MockSettingsWnd::activeWnds = 0;

BOOST_FIXTURE_TEST_CASE(TestTransmitSettingsAdapter, uiHelper::Fixture)
{
    auto* wnd = &WINDOWMANAGER.Show(std::make_unique<MockSettingsWnd>(CGI_TOOLS));
    BOOST_TEST_REQUIRE(wnd);
    {
        // Save settings on close via window method
        MOCK_EXPECT(wnd->TransmitSettings).once();
        wnd->Close();
        WINDOWMANAGER.Draw();
        BOOST_TEST(MockSettingsWnd::activeWnds == 0);
    }
    {
        // Save settings on close via ID
        wnd = &WINDOWMANAGER.Show(std::make_unique<MockSettingsWnd>(CGI_TOOLS));
        MOCK_EXPECT(wnd->TransmitSettings).once();
        WINDOWMANAGER.Close(CGI_TOOLS);
        WINDOWMANAGER.Draw();
        BOOST_TEST(MockSettingsWnd::activeWnds == 0);
    }
    {
        // Save settings on closeNow
        wnd = &WINDOWMANAGER.Show(std::make_unique<MockSettingsWnd>(CGI_TOOLS));
        MOCK_EXPECT(wnd->TransmitSettings).once();
        WINDOWMANAGER.CloseNow(wnd);
        BOOST_TEST(MockSettingsWnd::activeWnds == 0);
    }
    {
        // Save settings on ESC
        wnd = &WINDOWMANAGER.Show(std::make_unique<MockSettingsWnd>(CGI_TOOLS));
        BOOST_TEST_REQUIRE(WINDOWMANAGER.GetTopMostWindow() == wnd);
        MOCK_EXPECT(wnd->TransmitSettings).once();
        KeyEvent ev{KeyType::Escape, 0, false, false, false};
        WINDOWMANAGER.Msg_KeyDown(ev);
        WINDOWMANAGER.Draw();
        BOOST_TEST(MockSettingsWnd::activeWnds == 0);
        BOOST_TEST(WINDOWMANAGER.GetTopMostWindow() == nullptr);
    }
    {
        // Save settings on ALT+W
        wnd = &WINDOWMANAGER.Show(std::make_unique<MockSettingsWnd>(CGI_TOOLS));
        BOOST_TEST_REQUIRE(WINDOWMANAGER.GetTopMostWindow() == wnd);
        MOCK_EXPECT(wnd->TransmitSettings).once();
        KeyEvent ev{KeyType::Escape, 'w', false, false, true};
        WINDOWMANAGER.Msg_KeyDown(ev);
        WINDOWMANAGER.Draw();
        BOOST_TEST(MockSettingsWnd::activeWnds == 0);
        BOOST_TEST(WINDOWMANAGER.GetTopMostWindow() == nullptr);
    }
    mock::verify();
}

BOOST_AUTO_TEST_SUITE_END()
