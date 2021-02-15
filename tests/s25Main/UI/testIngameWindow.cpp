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
#include "controls/ctrlComboBox.h"
#include "controls/ctrlProgress.h"
#include "ingameWindows/iwHelp.h"
#include "ingameWindows/iwMapGenerator.h"
#include "mapGenerator/MapSettings.h"
#include "uiHelper/uiHelpers.hpp"
#include "rttr/test/random.hpp"
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(IngameWnd)
{
    uiHelper::initGUITests();
    iwHelp wnd("Foo barFoo barFoo barFoo bar\n\n\n\nFoo\nFoo\nFoo\nFoo\nFoo\nFoo\nFoo\nFoo\nFoo\nFoo\nFoo\nFoo\n");
    const Extent oldSize = wnd.GetSize();
    BOOST_TEST_REQUIRE(oldSize.x > 50u);
    BOOST_TEST_REQUIRE(oldSize.y > 50u);
    // Window should reduce height (only)
    wnd.SetMinimized(true);
    BOOST_TEST_REQUIRE(wnd.GetSize().x == oldSize.x); //-V807
    BOOST_TEST_REQUIRE(wnd.GetSize().y > 0u);
    BOOST_TEST_REQUIRE(wnd.GetSize().y < oldSize.y);
    // And fully expand to old size
    wnd.SetMinimized(false);
    BOOST_TEST_REQUIRE(wnd.GetSize() == oldSize);
}

BOOST_AUTO_TEST_CASE(IwMapGenerator)
{
    enum
    {
        CTRL_BTN_BACK = 0,
        CTRL_BTN_APPLY,
        CTRL_TXT_LANDSCAPE,
        CTRL_TXT_GOAL,
        CTRL_TXT_IRON,
        CTRL_TXT_COAL,
        CTRL_TXT_GRANITE,
        CTRL_TXT_RIVERS,
        CTRL_TXT_MOUNTAIN_DIST,
        CTRL_TXT_TREES,
        CTRL_TXT_STONE_PILES,
        CTRL_TXT_ISLANDS,
        CTRL_PLAYER_NUMBER,
        CTRL_MAP_STYLE,
        CTRL_MAP_SIZE,
        CTRL_MAP_TYPE,
        CTRL_RATIO_GOLD,
        CTRL_RATIO_IRON,
        CTRL_RATIO_COAL,
        CTRL_RATIO_GRANITE,
        CTRL_RIVERS,
        CTRL_MOUNTAIN_DIST,
        CTRL_TREES,
        CTRL_STONE_PILES,
        CTRL_ISLANDS
    };
    const auto expectedNumPlayers = rttr::test::randomValue(2u, 7u);
    const auto expectedMapType = rttr::test::randomValue(0, 2);
    const auto expectedGoldRatio = rttr::test::randomValue(0u, 100u);
    const auto expectedIronRatio = rttr::test::randomValue(0u, 100u);
    const auto expectedCoalRatio = rttr::test::randomValue(0u, 100u);
    const auto expectedGraniteRatio = rttr::test::randomValue(0u, 100u);
    const auto expectedRivers = rttr::test::randomValue(0u, 100u);
    const auto expectedTrees = rttr::test::randomValue(0u, 100u);
    const auto expectedStonePiles = rttr::test::randomValue(0u, 100u);

    uiHelper::initGUITests();
    rttr::mapGenerator::MapSettings settings;
    iwMapGenerator wnd(settings);
    wnd.GetCtrl<ctrlComboBox>(CTRL_PLAYER_NUMBER)->SetSelection(expectedNumPlayers - 2);
    wnd.GetCtrl<ctrlComboBox>(CTRL_MAP_TYPE)->SetSelection(expectedMapType);
    wnd.GetCtrl<ctrlComboBox>(CTRL_MAP_STYLE)->SetSelection(1);     // MapStyle::Land
    wnd.GetCtrl<ctrlComboBox>(CTRL_MAP_SIZE)->SetSelection(4);      // 1024x1024
    wnd.GetCtrl<ctrlComboBox>(CTRL_MOUNTAIN_DIST)->SetSelection(3); // MountainDistance::VeryFar
    wnd.GetCtrl<ctrlComboBox>(CTRL_ISLANDS)->SetSelection(2);       // IslandAmount::Many
    wnd.GetCtrl<ctrlProgress>(CTRL_RATIO_GOLD)->SetPosition(expectedGoldRatio);
    wnd.GetCtrl<ctrlProgress>(CTRL_RATIO_IRON)->SetPosition(expectedIronRatio);
    wnd.GetCtrl<ctrlProgress>(CTRL_RATIO_COAL)->SetPosition(expectedCoalRatio);
    wnd.GetCtrl<ctrlProgress>(CTRL_RATIO_GRANITE)->SetPosition(expectedGraniteRatio);
    wnd.GetCtrl<ctrlProgress>(CTRL_RIVERS)->SetPosition(expectedRivers);
    wnd.GetCtrl<ctrlProgress>(CTRL_TREES)->SetPosition(expectedTrees);
    wnd.GetCtrl<ctrlProgress>(CTRL_STONE_PILES)->SetPosition(expectedStonePiles);
    wnd.Msg_ButtonClick(CTRL_BTN_APPLY);

    BOOST_TEST(settings.numPlayers == expectedNumPlayers);
    BOOST_TEST(settings.type == DescIdx<LandscapeDesc>(expectedMapType));
    BOOST_TEST(settings.size == MapExtent(1024, 1024));
    BOOST_TEST(settings.style == rttr::mapGenerator::MapStyle::Land);
    BOOST_TEST(settings.mountainDistance == rttr::mapGenerator::MountainDistance::VeryFar);
    BOOST_TEST(settings.islands == rttr::mapGenerator::IslandAmount::Many);
    BOOST_TEST(settings.ratioGold == expectedGoldRatio);
    BOOST_TEST(settings.ratioIron == expectedIronRatio);
    BOOST_TEST(settings.ratioCoal == expectedCoalRatio);
    BOOST_TEST(settings.ratioGranite == expectedGraniteRatio);
    BOOST_TEST(settings.rivers == expectedRivers);
    BOOST_TEST(settings.trees == expectedTrees);
    BOOST_TEST(settings.stonePiles == expectedStonePiles);
}
