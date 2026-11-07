// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "GamePlayer.h"
#include "GlobalGameSettings.h"
#include "MockGameInterface.h"
#include "PointOutput.h"
#include "WindowManager.h"
#include "controls/ctrlBuildingIcon.h"
#include "controls/ctrlGroup.h"
#include "controls/ctrlTab.h"
#include "factories/BuildingFactory.h"
#include "figures/nofFarmhand.h"
#include "ingameWindows/iwAction.h"
#include "uiHelper/uiHelpers.hpp"
#include "worldFixtures/CreateEmptyWorld.h"
#include "worldFixtures/WorldFixture.h"
#include "world/GameWorldView.h"
#include "world/GameWorldViewer.h"
#include "gameTypes/GameTypesOutput.h"
#include "gameData/JobConsts.h"
#include "gameData/MilitaryConsts.h"
#include "rttr/test/random.hpp"
#include <rttr/test/testHelpers.hpp>
#include <boost/test/output/compiler_log_formatter.hpp>
#include <boost/test/unit_test.hpp>

// LCOV_EXCL_START
BOOST_TEST_DONT_PRINT_LOG_VALUE(Window::ButtonState)
// LCOV_EXCL_STOP

BOOST_AUTO_TEST_SUITE(BuildingRadiusTests)

namespace {
using EmptyWorldFixture1P = WorldFixture<CreateEmptyWorld, 1, 4, 4>;

struct GameWorldViewFixture : EmptyWorldFixture1P, uiHelper::Fixture // Required for GameWorldView
{
    // Ensure addon is enabled when gwv is constructed
    int dummy = (ggs.setSelection(AddonId::BUILDING_RADIUS, 1), 0);
    GameWorldViewer gwv;
    GameWorldView view;
    GameWorldViewFixture() : gwv(0, world), view(gwv, Position(0, 0), Extent(100, 100)) {}
};
} // namespace

BOOST_AUTO_TEST_CASE(FarmHandGetWorkRadius)
{
    GlobalGameSettings ggs;
    // Fixed radius
    BOOST_TEST(nofFarmhand::GetWorkRadius(ggs, Job::Farmer) > 0u);
    BOOST_TEST(nofFarmhand::GetWorkRadius(ggs, Job::Fisher) > 0u);

    // Addon dependent
    const auto defaultValue = nofFarmhand::GetWorkRadius(ggs, Job::Woodcutter);
    BOOST_TEST(defaultValue > 0);
    ggs.setSelection(AddonId::WOODCUTTER_REACH_RADIUS, rttr::test::randomValue(1, 3));
    BOOST_TEST(nofFarmhand::GetWorkRadius(ggs, Job::Woodcutter) > defaultValue);

    // Non farmhand jobs throw
    BOOST_CHECK_THROW(nofFarmhand::GetWorkRadius(ggs, Job::Helper), std::logic_error);
    BOOST_CHECK_THROW(nofFarmhand::GetWorkRadius(ggs, Job::Metalworker), std::logic_error);
}

BOOST_FIXTURE_TEST_CASE(GetBuildingRadius, GameWorldViewFixture)
{
    // Military buildings (territory influence radius)
    BOOST_TEST(view.GetBuildingRadius(BuildingType::Barracks) == MILITARY_RADIUS[0]);
    BOOST_TEST(view.GetBuildingRadius(BuildingType::Guardhouse) == MILITARY_RADIUS[1]);
    BOOST_TEST(view.GetBuildingRadius(BuildingType::Watchtower) == MILITARY_RADIUS[2]);
    BOOST_TEST(view.GetBuildingRadius(BuildingType::Fortress) == MILITARY_RADIUS[3]);
    BOOST_TEST(view.GetBuildingRadius(BuildingType::Headquarters) == HQ_RADIUS);
    BOOST_TEST(view.GetBuildingRadius(BuildingType::HarborBuilding) == HARBOR_RADIUS);

    BOOST_TEST(view.GetBuildingRadius(BuildingType::LookoutTower) == VISUALRANGE_LOOKOUTTOWER);
    BOOST_TEST(view.GetBuildingRadius(BuildingType::Catapult) == CATAPULT_MAX_TARGET_RANGE);
    BOOST_TEST(view.GetBuildingRadius(BuildingType::Hunter) == ANIMAL_RADIUS);
    BOOST_TEST(view.GetBuildingRadius(BuildingType::Skinner) == ANIMAL_RADIUS);

    for(const auto mine : {BuildingType::GraniteMine, BuildingType::CoalMine, BuildingType::IronMine,
                           BuildingType::GoldMine, BuildingType::Well})
    {
        BOOST_TEST(view.GetBuildingRadius(mine) == MINER_RADIUS);
    }
    for(const auto bt : {BuildingType::Woodcutter, BuildingType::Forester, BuildingType::Fishery, BuildingType::Quarry,
                         BuildingType::Farm, BuildingType::Vineyard, BuildingType::Charburner})
    {
        BOOST_TEST(view.GetBuildingRadius(bt) > 0u);
    }

    // Remaining building types have no relevant radius
    for(const auto bt : {BuildingType::Armory, BuildingType::Bakery, BuildingType::Brewery, BuildingType::DonkeyBreeder,
                         BuildingType::Ironsmelter, BuildingType::LeatherWorks, BuildingType::Metalworks,
                         BuildingType::Mill, BuildingType::Mint, BuildingType::Nothing9, BuildingType::PigFarm,
                         BuildingType::Sawmill, BuildingType::Shipyard, BuildingType::Slaughterhouse,
                         BuildingType::Storehouse, BuildingType::Tannery, BuildingType::Temple, BuildingType::Winery})
    {
        BOOST_TEST(view.GetBuildingRadius(bt) == 0u);
    }
}

BOOST_FIXTURE_TEST_CASE(GetBuildingRadiusRespectsAddons, GameWorldViewFixture)
{
    // Smoke tests only: Increasing the addon setting increases reach w/o assuming specific number of settings or values
    ggs.setSelection(AddonId::WOODCUTTER_REACH_RADIUS, 0);
    auto defaultRadius = view.GetBuildingRadius(BuildingType::Woodcutter);
    ggs.setSelection(AddonId::WOODCUTTER_REACH_RADIUS, 1);
    BOOST_TEST(view.GetBuildingRadius(BuildingType::Woodcutter) > defaultRadius);

    ggs.setSelection(AddonId::FORESTER_REACH_RADIUS, 0);
    defaultRadius = view.GetBuildingRadius(BuildingType::Forester);
    ggs.setSelection(AddonId::FORESTER_REACH_RADIUS, 1);
    BOOST_TEST(view.GetBuildingRadius(BuildingType::Forester) > defaultRadius);

    ggs.setSelection(AddonId::STONEMASON_REACH_RADIUS, 0);
    defaultRadius = view.GetBuildingRadius(BuildingType::Quarry);
    ggs.setSelection(AddonId::STONEMASON_REACH_RADIUS, 1);
    BOOST_TEST(view.GetBuildingRadius(BuildingType::Quarry) > defaultRadius);
}

BOOST_AUTO_TEST_CASE(SnapToNearestCopy)
{
    const Extent mapSize(122, 84);
    const DrawPoint pt(37, 17);
    const DrawPoint ref(pt + mapSize / 5); // Close but not the same

    // Same point and reference -> no change
    BOOST_TEST(GameWorldView::SnapToNearestCopy(pt, pt, mapSize) == pt);
    // Fixed ref is close enough -> closest is already this point
    BOOST_TEST(GameWorldView::SnapToNearestCopy(pt, ref, mapSize) == pt);
    for(const auto offset : {// 1 map size right,left,top,bottom -> same offset for point
                             Offset(mapSize.x, 0), -Offset(mapSize.x, 0), -Offset(0, mapSize.y), Offset(0, mapSize.y),
                             // diagonal
                             Offset(mapSize), -Offset(mapSize)})
    {
        BOOST_TEST(GameWorldView::SnapToNearestCopy(pt, ref + offset, mapSize) == pt + offset);
        const Offset offset2 = offset + mapSize * 3; // Multiple map sizes
        BOOST_TEST(GameWorldView::SnapToNearestCopy(pt, ref + offset2, mapSize) == pt + offset2);
    }

    // Reference is exactly half a map size away -> point wraps to the other side
    const Extent halfMapSize = mapSize / 2u;
    BOOST_TEST(GameWorldView::SnapToNearestCopy(pt, pt - Offset(halfMapSize.x, 0), mapSize)
               == pt - Offset(mapSize.x, 0));
    BOOST_TEST(GameWorldView::SnapToNearestCopy(pt, pt + Offset(halfMapSize.x, 0), mapSize)
               == pt + Offset(mapSize.x, 0));
    BOOST_TEST(GameWorldView::SnapToNearestCopy(pt, pt - Offset(0, halfMapSize.y), mapSize)
               == pt - Offset(0, mapSize.y));
    BOOST_TEST(GameWorldView::SnapToNearestCopy(pt, pt + Offset(0, halfMapSize.y), mapSize)
               == pt + Offset(0, mapSize.y));

    // Reference at least half a map size to the left -> point wraps to the left
    const Offset offsetX = Offset(rttr::test::randomValue(mapSize.x / 2, mapSize.x), 0);
    BOOST_TEST(GameWorldView::SnapToNearestCopy(pt, pt - offsetX, mapSize) == pt - Offset(mapSize.x, 0));
    // same for right
    BOOST_TEST(GameWorldView::SnapToNearestCopy(pt, pt + offsetX, mapSize) == pt + Offset(mapSize.x, 0));
    const Offset offsetY = Offset(0, rttr::test::randomValue(mapSize.y / 2, mapSize.y));
    // And top/bottom
    BOOST_TEST(GameWorldView::SnapToNearestCopy(pt, pt - offsetY, mapSize) == pt - Offset(0, mapSize.y));
    BOOST_TEST(GameWorldView::SnapToNearestCopy(pt, pt + offsetY, mapSize) == pt + Offset(0, mapSize.y));
}

BOOST_FIXTURE_TEST_CASE(TestUpdateRadiusPreview, GameWorldViewFixture)
{
    // Open iwAction in build mode
    MockGameInterface gi;
    iwAction::Tabs tabs;
    tabs.build = true;
    tabs.build_tabs = iwAction::BuildTab::House;
    const auto selPt = MapPoint(2, 3);

    auto getIcons = [](iwAction& wnd) {
        auto i = wnd.GetCtrls<ctrlTab>();                    // main tab
        auto i2 = i.at(0)->GetCtrls<ctrlGroup>();            // build tab group
        auto i3 = i2.at(0)->GetCtrls<ctrlTab>();             // build tab
        i3.at(0)->SetSelection(1);                           // Select medium size tab
        auto i4 = i3.at(0)->GetCtrls<ctrlGroup>();           // bld sizes
        return i4.at(1)->GetCtrls<const ctrlBuildingIcon>(); // medium buildings
    };
    std::vector<const ctrlBuildingIcon*> icons;
    const auto& findIcon = [&](BuildingType type) -> const ctrlBuildingIcon& {
        const auto it =
          helpers::find_if(icons, [type](const ctrlBuildingIcon* icon) { return icon->GetType() == type; });
        BOOST_TEST_REQUIRE((it != icons.end()), "Icon for " << type << " not found");
        return ensureNonNull(*it);
    };

    // No radius shown in tooltip when addon is disabled
    ggs.setSelection(AddonId::BUILDING_RADIUS, 0);
    {
        auto& action = WINDOWMANAGER.Show(
          std::make_unique<iwAction>(gi, view, tabs, selPt, DrawPoint(0, 0), iwAction::FlagType::Normal, true));
        icons = getIcons(action);
        const auto& findIcon = [&](BuildingType type) -> const ctrlBuildingIcon& {
            const auto it =
              helpers::find_if(icons, [type](const ctrlBuildingIcon* icon) { return icon->GetType() == type; });
            BOOST_TEST_REQUIRE((it != icons.end()));
            return ensureNonNull(*it);
        };
        BOOST_TEST(findIcon(BuildingType::Watchtower).GetTooltip().find(_("Range:")) == std::string::npos);
    }

    // Enable addon and re-open iwAction
    ggs.setSelection(AddonId::BUILDING_RADIUS, 1);
    auto& action = WINDOWMANAGER.Show(
      std::make_unique<iwAction>(gi, view, tabs, selPt, DrawPoint(0, 0), iwAction::FlagType::Normal, true));
    icons = getIcons(action);
    BOOST_TEST(findIcon(BuildingType::Watchtower).GetTooltip().find(_("Range:")) != std::string::npos);
    BOOST_TEST(findIcon(BuildingType::Catapult).GetTooltip().find(_("Range:")) != std::string::npos);
    BOOST_TEST(findIcon(BuildingType::Mill).GetTooltip().find(_("Range:")) == std::string::npos);

    // Moving mouse over icons shows radius preview
    for(const auto bt : {BuildingType::Watchtower, BuildingType::Mill, BuildingType::Catapult})
    {
        const auto& icon = findIcon(bt);
        WINDOWMANAGER.Msg_MouseMove(MouseCoords(icon.GetDrawPos() + DrawPoint(1, 1)));
        BOOST_TEST_REQUIRE(icon.getState() == Window::ButtonState::Hover);
        const auto& radius = view.getRadiusPreview();
        // Mill has no radius, so preview is hidden
        if(bt == BuildingType::Mill)
            BOOST_TEST(!radius.has_value());
        else
        {
            BOOST_TEST(radius.value().center == selPt);
            BOOST_TEST(radius.value().radius == view.GetBuildingRadius(bt));
        }
    }
    // Moving mouse away from icons hides radius preview
    BOOST_TEST_REQUIRE(view.getRadiusPreview().has_value());
    WINDOWMANAGER.Msg_MouseMove(MouseCoords(action.GetDrawPos()));
    BOOST_TEST(!view.getRadiusPreview());

    // Closing the action window hides radius preview
    const auto& icon = findIcon(BuildingType::Watchtower);
    WINDOWMANAGER.Msg_MouseMove(MouseCoords(icon.GetDrawPos() + DrawPoint(1, 1)));
    BOOST_TEST_REQUIRE(view.getRadiusPreview().has_value());
    WINDOWMANAGER.CloseNow(&action);
    BOOST_TEST(!view.getRadiusPreview());
}

BOOST_AUTO_TEST_SUITE_END()
