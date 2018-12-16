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

#include "rttrDefines.h" // IWYU pragma: keep
#include "controls/ctrlButton.h"
#include "controls/ctrlText.h"
#include "ingameWindows/iwVictory.h"
#include "initTestHelpers.h"
#include <boost/foreach.hpp>
#include <boost/test/unit_test.hpp>
#include <turtle/mock.hpp>
#include "WindowManager.h"
#include "desktops/Desktop.h"
#include "controls/ctrlGroup.h"

namespace {

struct WindowsFixture
{
    WindowsFixture() { initGUITests(); }
};
} // namespace

BOOST_FIXTURE_TEST_SUITE(Windows, WindowsFixture)

BOOST_AUTO_TEST_CASE(Victory)
{
    std::vector<std::string> winnerNames;
    winnerNames.push_back("FooName");
    winnerNames.push_back("BarNameBaz");
    const iwVictory wnd(winnerNames);
    // 2 buttons
    BOOST_REQUIRE_EQUAL(wnd.GetCtrls<ctrlButton>().size(), 2u);
    // Find a text field containing all winner names
    std::vector<const ctrlText*> txts = wnd.GetCtrls<ctrlText>();
    bool found = false;
    BOOST_FOREACH(const ctrlText* txt, txts)
    {
        bool curFound = true;
        BOOST_FOREACH(const std::string& name, winnerNames)
        {
            curFound &= txt->GetText().find(name) != std::string::npos;
        }
        found |= curFound;
    }
    BOOST_REQUIRE(found);
}

namespace
{
    /* clang-format off */
    MOCK_BASE_CLASS(TestWindow, Window)
    {
    public:
        TestWindow(Window* parent, unsigned id, const DrawPoint& position): Window(parent, id, position) {}
        MOCK_METHOD(Msg_PaintBefore, 0)
        MOCK_METHOD(Msg_PaintAfter, 0)
        MOCK_METHOD(Draw_, 0, void())
    };
    /* clang-format on */
}

BOOST_AUTO_TEST_CASE(DrawOrder)
{
    Desktop* dsk = WINDOWMANAGER.GetCurrentDesktop();
    std::vector<TestWindow*> wnds;
    // Top level controls
    for(int i = 0; i < 3; i++)
    {
        wnds.push_back(new TestWindow(dsk, wnds.size(), DrawPoint(0, 0)));
        dsk->AddCtrl(wnds.back());
    }
    // Some groups with own controls
    for(int i = 0; i < 3; i++)
    {
        ctrlGroup* grp = dsk->AddGroup(100 + i);
        for(int i = 0; i < 3; i++)
        {
            wnds.push_back(new TestWindow(dsk, wnds.size(), DrawPoint(0, 0)));
            grp->AddCtrl(wnds.back());
        }
    }
    mock::sequence s;
    // Note: Actually order of calls to controls is undefined but in practice matches the IDs
    BOOST_FOREACH(TestWindow* wnd, wnds)
        MOCK_EXPECT(wnd->Msg_PaintBefore).once().in(s);
    BOOST_FOREACH(TestWindow* wnd, wnds)
        MOCK_EXPECT(wnd->Draw_).once().in(s);
    BOOST_FOREACH(TestWindow* wnd, wnds)
        MOCK_EXPECT(wnd->Msg_PaintAfter).once().in(s);
    WINDOWMANAGER.Draw();
    mock::verify();
}

BOOST_AUTO_TEST_SUITE_END()
