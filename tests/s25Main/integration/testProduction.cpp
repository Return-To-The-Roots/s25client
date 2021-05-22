// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "buildings/nobBaseWarehouse.h"
#include "buildings/nobUsual.h"
#include "factories/BuildingFactory.h"
#include "postSystem/PostBox.h"
#include "postSystem/PostMsg.h"
#include "worldFixtures/WorldWithGCExecution.h"
#include "gameData/ToolConsts.h"
#include <rttr/test/LogAccessor.hpp>
#include <boost/test/unit_test.hpp>
#include <algorithm>
#include <array>

// LCOV_EXCL_START
static std::ostream& operator<<(std::ostream& os, const PostCategory& cat)
{
    return os << static_cast<unsigned>(cat);
}
// LCOV_EXCL_STOP

BOOST_AUTO_TEST_SUITE(Production)

BOOST_FIXTURE_TEST_CASE(MetalWorkerStopped, WorldWithGCExecution1P)
{
    rttr::test::LogAccessor logAcc;
    ggs.setSelection(AddonId::TOOL_ORDERING, 1);
    ggs.setSelection(AddonId::METALWORKSBEHAVIORONZERO, 1);
    Inventory goods;
    goods.goods[GoodType::Iron] = 10;
    world.GetSpecObj<nobBaseWarehouse>(hqPos)->AddGoods(goods, true);
    MapPoint bldPos = hqPos + MapPoint(2, 0);
    BuildingFactory::CreateBuilding(world, BuildingType::Metalworks, bldPos, curPlayer, Nation::Africans);
    this->BuildRoad(world.GetNeighbour(bldPos, Direction::SouthEast), false,
                    std::vector<Direction>(2, Direction::West));
    MapPoint bldPos2 = hqPos - MapPoint(2, 0);
    BuildingFactory::CreateBuilding(world, BuildingType::Metalworks, bldPos2, curPlayer, Nation::Africans);
    this->BuildRoad(world.GetNeighbour(bldPos2, Direction::SouthEast), false,
                    std::vector<Direction>(2, Direction::East));

    helpers::EnumArray<int8_t, Tool> toolOrder;
    ToolSettings toolSettings;
    std::fill(toolOrder.begin(), toolOrder.end(), 0);
    std::fill(toolSettings.begin(), toolSettings.end(), 0);
    this->ChangeTools(toolSettings, toolOrder.data());
    // Get wares and workers in
    RTTR_SKIP_GFS(1000);

    toolOrder[Tool::Tongs] = 1;
    toolOrder[Tool::Cleaver] = 1;
    toolOrder[Tool::Rollingpin] = 1;
    PostBox& postbox = world.GetPostMgr().AddPostBox(0);
    postbox.Clear();
    const Inventory& curInventory = world.GetPlayer(curPlayer).GetInventory();
    Inventory expectedInventory = curInventory;
    expectedInventory.Add(GoodType::Tongs, toolOrder[Tool::Tongs]);
    expectedInventory.Add(GoodType::Cleaver, toolOrder[Tool::Cleaver]);
    expectedInventory.Add(GoodType::Rollingpin, toolOrder[Tool::Rollingpin]);
    // Place order
    this->ChangeTools(toolSettings, toolOrder.data());
    RTTR_REQUIRE_LOG_CONTAINS("Committing an order", true);
    // Wait for completion message
    RTTR_EXEC_TILL(3000, postbox.GetNumMsgs() == 1u);
    BOOST_TEST_REQUIRE(postbox.GetMsg(0)->GetCategory() == PostCategory::Economy);
    // Stop it and wait till goods are produced
    this->SetProductionEnabled(bldPos, false);
    this->SetProductionEnabled(bldPos2, false);
    RTTR_EXEC_TILL(2000, curInventory[GoodType::Tongs] == expectedInventory[GoodType::Tongs]
                           && curInventory[GoodType::Cleaver] == expectedInventory[GoodType::Cleaver]
                           && curInventory[GoodType::Rollingpin] == expectedInventory[GoodType::Rollingpin]);
}

BOOST_FIXTURE_TEST_CASE(MetalWorkerOrders, WorldWithGCExecution1P)
{
    Inventory inv;
    inv.Add(GoodType::Boards, 10);
    inv.Add(GoodType::Iron, 10);
    world.GetSpecObj<nobBaseWarehouse>(hqPos)->AddGoods(inv, true);
    ggs.setSelection(AddonId::METALWORKSBEHAVIORONZERO, 1);
    ggs.setSelection(AddonId::TOOL_ORDERING, 1);
    ToolSettings settings;
    std::fill(settings.begin(), settings.end(), 0);
    this->ChangeTools(settings);
    MapPoint housePos(hqPos.x + 3, hqPos.y);
    const nobUsual* mw = static_cast<nobUsual*>(
      BuildingFactory::CreateBuilding(world, BuildingType::Metalworks, housePos, curPlayer, Nation::Romans));
    MapPoint flagPos = world.GetNeighbour(hqPos, Direction::SouthEast);
    this->BuildRoad(flagPos, false, std::vector<Direction>(3, Direction::East));
    RTTR_EXEC_TILL(200, mw->HasWorker());
    BOOST_TEST_REQUIRE(!mw->is_working);
    // Wait till he has all the wares
    RTTR_EXEC_TILL(3000, mw->GetNumWares(0) == 6);
    RTTR_EXEC_TILL(3000, mw->GetNumWares(1) == 6);
    // No order -> not working
    BOOST_TEST_REQUIRE(!mw->is_working);
    helpers::EnumArray<int8_t, Tool> orders;
    std::fill(orders.begin(), orders.end(), 0);
    orders[Tool::Bow] = 1;
    this->ChangeTools(settings, orders.data());
    RTTR_EXEC_TILL(1300, mw->is_working);
}

BOOST_AUTO_TEST_SUITE_END()
