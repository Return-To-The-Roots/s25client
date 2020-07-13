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
#include "ingameWindows/iwHelp.h"
#include "uiHelper/uiHelpers.hpp"
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(IngameWnd)
{
    uiHelper::initGUITests();
    iwHelp wnd("Foo barFoo barFoo barFoo bar\n\n\n\nFoo\nFoo\nFoo\nFoo\nFoo\nFoo\nFoo\nFoo\nFoo\nFoo\nFoo\nFoo\n");
    const Extent oldSize = wnd.GetSize();
    BOOST_REQUIRE_GT(oldSize.x, 50u);
    BOOST_REQUIRE_GT(oldSize.y, 50u);
    // Window should reduce height (only)
    wnd.SetMinimized(true);
    BOOST_REQUIRE_EQUAL(wnd.GetSize().x, oldSize.x); //-V807
    BOOST_REQUIRE_GT(wnd.GetSize().y, 0u);
    BOOST_REQUIRE_LT(wnd.GetSize().y, oldSize.y);
    // And fully expand to old size
    wnd.SetMinimized(false);
    BOOST_REQUIRE_EQUAL(wnd.GetSize(), oldSize);
}
