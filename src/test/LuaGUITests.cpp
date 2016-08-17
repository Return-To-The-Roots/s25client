// Copyright (c) 2016 Settlers Freaks (sf-team at siedler25.org)
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

#include "defines.h" // IWYU pragma: keep
#include "test/GameWorldWithLuaAccess.h"
#include "WindowManager.h"
#include "desktops/Desktop.h"
#include "controls/ctrlImage.h"
#include "ingameWindows/iwMissionStatement.h"
#include "GameMessages.h"
#include "GameClient.h"
#include "Loader.h"
#include "drivers/VideoDriverWrapper.h"
#include "test/DummyVideoDriver.h"

BOOST_FIXTURE_TEST_SUITE(LuaGUITestSuite, LuaTestsFixture)

class DummyDesktop: public Desktop
{
public: DummyDesktop(): Desktop(NULL){}
};

BOOST_AUTO_TEST_CASE(MissionStatement)
{
    VIDEODRIVER.LoadDriver(new DummyVideoDriver());
    LOADER.LoadDummyGUIFiles();
    initWorld();
    WINDOWMANAGER.Switch(new DummyDesktop);
    WINDOWMANAGER.Draw();

    // Set player
    static_cast<GameMessageInterface&>(GAMECLIENT).OnGameMessage(GameMessage_Player_Id(1));

    BOOST_REQUIRE(!WINDOWMANAGER.GetTopMostWindow());

    // Show mission to non-existing player
    executeLua("rttr:MissionStatement(99, 'Title', 'Text')");
    BOOST_REQUIRE(!WINDOWMANAGER.GetTopMostWindow());

    // Show mission to other player
    executeLua("rttr:MissionStatement(0, 'Title', 'Text')");
    BOOST_REQUIRE(!WINDOWMANAGER.GetTopMostWindow());

    // Show it to us
    executeLua("rttr:MissionStatement(1, 'Title', 'Text')");
    const iwMissionStatement* wnd = dynamic_cast<const iwMissionStatement*>(WINDOWMANAGER.GetTopMostWindow());
    BOOST_REQUIRE(wnd);
    BOOST_REQUIRE(wnd->IsActive());
    BOOST_REQUIRE_EQUAL(wnd->GetTitle(), "Title");

    // double windows stack
    executeLua("rttr:MissionStatement(1, 'Title2', 'Text')");
    const iwMissionStatement* wnd2 = dynamic_cast<const iwMissionStatement*>(WINDOWMANAGER.GetTopMostWindow());
    BOOST_REQUIRE(wnd2);
    // Other window still on top
    BOOST_REQUIRE_EQUAL(wnd2, wnd);
    // Close first wnd
    WINDOWMANAGER.Close(wnd);
    // 2nd shows
    wnd2 = dynamic_cast<const iwMissionStatement*>(WINDOWMANAGER.GetTopMostWindow());
    BOOST_REQUIRE(wnd2);
    BOOST_REQUIRE_NE(wnd2, wnd);
    BOOST_REQUIRE_EQUAL(wnd2->GetTitle(), "Title2");
    // Close wnd
    WINDOWMANAGER.Close(wnd2);
    BOOST_REQUIRE(!WINDOWMANAGER.GetTopMostWindow());
    
    // No image
    executeLua("rttr:MissionStatement(1, 'Title', 'Text', IM_NONE)");
    wnd = dynamic_cast<const iwMissionStatement*>(WINDOWMANAGER.GetTopMostWindow());
    BOOST_REQUIRE(wnd);
    BOOST_REQUIRE(wnd->IsActive());
    BOOST_REQUIRE_EQUAL(wnd->GetTitle(), "Title");
    BOOST_REQUIRE(wnd->GetCtrls<ctrlImage>().empty());
    WINDOWMANAGER.Close(wnd);
    // Non-default image
    executeLua("rttr:MissionStatement(1, 'Title', 'Text', IM_AVATAR10)");
    wnd = dynamic_cast<const iwMissionStatement*>(WINDOWMANAGER.GetTopMostWindow());
    BOOST_REQUIRE(wnd);
    BOOST_REQUIRE(wnd->IsActive());
    BOOST_REQUIRE_EQUAL(wnd->GetTitle(), "Title");
    BOOST_REQUIRE(!wnd->GetCtrls<ctrlImage>().empty());
    WINDOWMANAGER.Close(wnd);
    // Invalid image
    executeLua("rttr:MissionStatement(1, 'Title', 'Text', 999999)");
    wnd = dynamic_cast<const iwMissionStatement*>(WINDOWMANAGER.GetTopMostWindow());
    BOOST_REQUIRE(wnd);
    BOOST_REQUIRE(wnd->IsActive());
    BOOST_REQUIRE_EQUAL(wnd->GetTitle(), "Title");
    BOOST_REQUIRE(wnd->GetCtrls<ctrlImage>().empty());
    WINDOWMANAGER.Close(wnd);
}

BOOST_AUTO_TEST_SUITE_END()
