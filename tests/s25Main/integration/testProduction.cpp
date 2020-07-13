// Copyright (c) 2016 - 2020 Settlers Freaks (sf-team at siedler25.org)
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
    goods.goods[GD_IRON] = 10;
    world.GetSpecObj<nobBaseWarehouse>(hqPos)->AddGoods(goods, true);
    MapPoint bldPos = hqPos + MapPoint(2, 0);
    BuildingFactory::CreateBuilding(world, BLD_METALWORKS, bldPos, curPlayer, NAT_AFRICANS);
    this->BuildRoad(world.GetNeighbour(bldPos, Direction::SOUTHEAST), false, std::vector<Direction>(2, Direction::WEST));
    MapPoint bldPos2 = hqPos - MapPoint(2, 0);
    BuildingFactory::CreateBuilding(world, BLD_METALWORKS, bldPos2, curPlayer, NAT_AFRICANS);
    this->BuildRoad(world.GetNeighbour(bldPos2, Direction::SOUTHEAST), false, std::vector<Direction>(2, Direction::EAST));

    std::array<signed char, NUM_TOOLS> toolOrder;
    ToolSettings toolSettings;
    std::fill(toolOrder.begin(), toolOrder.end(), 0);
    std::fill(toolSettings.begin(), toolSettings.end(), 0);
    this->ChangeTools(toolSettings, toolOrder.data());
    // Get wares and workers in
    RTTR_SKIP_GFS(1000);

    toolOrder[0] = 1;
    toolOrder[1] = 1;
    toolOrder[2] = 1;
    PostBox& postbox = world.GetPostMgr().AddPostBox(0);
    postbox.Clear();
    const Inventory& curInventory = world.GetPlayer(curPlayer).GetInventory();
    Inventory expectedInventory = curInventory;
    expectedInventory.Add(TOOLS[0], toolOrder[0]);
    expectedInventory.Add(TOOLS[1], toolOrder[1]);
    expectedInventory.Add(TOOLS[2], toolOrder[2]);
    // Place order
    this->ChangeTools(toolSettings, toolOrder.data());
    RTTR_REQUIRE_LOG_CONTAINS("Committing an order", true);
    // Wait for completion message
    RTTR_EXEC_TILL(3000, postbox.GetNumMsgs() == 1u);
    BOOST_REQUIRE_EQUAL(postbox.GetMsg(0)->GetCategory(), PostCategory::Economy);
    // Stop it and wait till goods are produced
    this->SetProductionEnabled(bldPos, false);
    this->SetProductionEnabled(bldPos2, false);
    RTTR_EXEC_TILL(2000, curInventory[TOOLS[0]] == expectedInventory[TOOLS[0]] && curInventory[TOOLS[1]] == expectedInventory[TOOLS[1]]
                           && curInventory[TOOLS[2]] == expectedInventory[TOOLS[2]]);
}

BOOST_FIXTURE_TEST_CASE(MetalWorkerOrders, WorldWithGCExecution1P)
{
    Inventory inv;
    inv.Add(GD_BOARDS, 10);
    inv.Add(GD_IRON, 10);
    world.GetSpecObj<nobBaseWarehouse>(hqPos)->AddGoods(inv, true);
    ggs.setSelection(AddonId::METALWORKSBEHAVIORONZERO, 1);
    ggs.setSelection(AddonId::TOOL_ORDERING, 1);
    ToolSettings settings;
    std::fill(settings.begin(), settings.end(), 0);
    this->ChangeTools(settings);
    MapPoint housePos(hqPos.x + 3, hqPos.y);
    const nobUsual* mw = static_cast<nobUsual*>(BuildingFactory::CreateBuilding(world, BLD_METALWORKS, housePos, curPlayer, NAT_ROMANS));
    MapPoint flagPos = world.GetNeighbour(hqPos, Direction::SOUTHEAST);
    this->BuildRoad(flagPos, false, std::vector<Direction>(3, Direction::EAST));
    RTTR_EXEC_TILL(200, mw->HasWorker());
    BOOST_REQUIRE(!mw->is_working);
    // Wait till he has all the wares
    RTTR_EXEC_TILL(3000, mw->GetNumWares(0) == 6);
    RTTR_EXEC_TILL(3000, mw->GetNumWares(1) == 6);
    // No order -> not working
    BOOST_REQUIRE(!mw->is_working);
    std::array<int8_t, NUM_TOOLS> orders;
    std::fill(orders.begin(), orders.end(), 0);
    orders[0] = 1;
    this->ChangeTools(settings, &orders.front());
    RTTR_EXEC_TILL(1300, mw->is_working);
}

BOOST_AUTO_TEST_SUITE_END()
