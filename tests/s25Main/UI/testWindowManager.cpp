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
#include "mockupDrivers/MockupVideoDriver.h"
#include "uiHelper/uiHelpers.hpp"
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

BOOST_AUTO_TEST_SUITE_END()
