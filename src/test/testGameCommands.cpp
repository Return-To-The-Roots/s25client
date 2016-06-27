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
#include "test/WorldWithGCExecution.h"
#include "GamePlayer.h"
#include "nodeObjs/noBase.h"
#include "nodeObjs/noEnvObject.h"
#include "nodeObjs/noFlag.h"
#include "buildings/nobBaseWarehouse.h"
#include "buildings/nobMilitary.h"
#include "buildings/nobUsual.h"
#include "figures/nofPassiveSoldier.h"
#include "factories/BuildingFactory.h"
#include "postSystem/PostBox.h"
#include "PointOutput.h"
#include "gameTypes/InventorySetting.h"
#include "gameTypes/VisualSettings.h"
#include "gameData/SettingTypeConv.h"
#include "gameData/ShieldConsts.h"
#include <boost/test/unit_test.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>

std::ostream& operator<<(std::ostream &out, const InventorySetting& setting)
{
    return out << setting.ToUnsignedChar();
}

BOOST_AUTO_TEST_SUITE(GameCommandSuite)

BOOST_FIXTURE_TEST_CASE(PlaceFlagTest, WorldWithGCExecution2P)
{
    MapPoint flagPt = hqPos + MapPoint(4, 0);
    // Place flag for other player:
    curPlayer = 1;
    this->SetFlag(flagPt);
    // Wrong terrain
    BOOST_REQUIRE(!world.GetSpecObj<noRoadNode>(flagPt));

    curPlayer = 0;
    this->SetFlag(flagPt);
    BOOST_REQUIRE_EQUAL(world.GetNO(flagPt)->GetType(), NOP_FLAG);
    noRoadNode* flag = world.GetSpecObj<noRoadNode>(flagPt);
    BOOST_REQUIRE(flag);
    BOOST_REQUIRE_EQUAL(flag->GetPos(), flagPt);
    BOOST_REQUIRE_EQUAL(flag->GetPlayer(), 0);
    // Flag blocked
    BOOST_REQUIRE_EQUAL(world.GetNeighbourNode(flagPt, Direction::WEST).bq, BQ_NOTHING);
    BOOST_REQUIRE_EQUAL(world.GetNeighbourNode(flagPt, Direction::NORTHEAST).bq, BQ_NOTHING);
    // This flag = house flag
    BOOST_REQUIRE_EQUAL(world.GetNeighbourNode(flagPt, Direction::NORTHWEST).bq, BQ_CASTLE);
    // Flag blocks castle
    BOOST_REQUIRE_EQUAL(world.GetNeighbourNode(flagPt, Direction::EAST).bq, BQ_HOUSE);
    BOOST_REQUIRE_EQUAL(world.GetNeighbourNode(flagPt, Direction::SOUTHEAST).bq, BQ_HOUSE);
    BOOST_REQUIRE_EQUAL(world.GetNeighbourNode(flagPt, Direction::SOUTHWEST).bq, BQ_HOUSE);
    // Place flag again
    this->SetFlag(flagPt);
    // Nothing should be changed
    BOOST_REQUIRE_EQUAL(world.GetSpecObj<noRoadNode>(flagPt), flag);

    // Place flag at neighbour
    for(int dir = 0; dir < 6; dir++)
    {
        MapPoint curPt = world.GetNeighbour(flagPt, dir);
        this->SetFlag(curPt);
        // Should not work
        BOOST_REQUIRE(!world.GetSpecObj<noRoadNode>(curPt));
    }

    unsigned objCt = GameObject::GetObjCount();
    this->DestroyFlag(flagPt);
    // Removed from map
    BOOST_REQUIRE_EQUAL(world.GetNO(flagPt)->GetType(), NOP_NOTHING);
    // Removed from game
    BOOST_REQUIRE_EQUAL(GameObject::GetObjCount(), objCt - 1);
    // And everything clear now
    for(int dir = 0; dir < 6; dir++)
        BOOST_REQUIRE_EQUAL(world.GetNeighbourNode(flagPt, dir).bq, BQ_CASTLE);
}

BOOST_FIXTURE_TEST_CASE(BuildRoadTest, WorldWithGCExecution2P)
{
    MapPoint flagPt = hqPos + MapPoint(4, 0);
    // 2 flags outside range of HQ
    this->SetFlag(flagPt);
    this->SetFlag(flagPt + MapPoint(4, 0));
    // Build road with 3 segments:
    // a1) invalid start pt -> No road
    this->BuildRoad(flagPt + MapPoint(2, 0), false, std::vector<unsigned char>(4, Direction::EAST));
    for(unsigned i = 0; i < 6; i++)
        BOOST_REQUIRE_EQUAL(world.GetPointRoad(flagPt + MapPoint(i, 0), Direction::EAST), 0);
    // a2) invalid player
    curPlayer = 1;
    this->BuildRoad(flagPt, false, std::vector<unsigned char>(4, Direction::EAST));
    for(unsigned i = 0; i < 6; i++)
        BOOST_REQUIRE_EQUAL(world.GetPointRoad(flagPt + MapPoint(i, 0), Direction::EAST), 0);
    curPlayer = 0;

    // b) Flag->Flag ->OK
    this->BuildRoad(flagPt, false, std::vector<unsigned char>(4, Direction::EAST));
    for(unsigned i = 0; i < 4; i++)
    {
        BOOST_REQUIRE_EQUAL(world.GetPointRoad(flagPt + MapPoint(i, 0), Direction::EAST), 1);
        // Same but in opposite direction
        BOOST_REQUIRE_EQUAL(world.GetPointRoad(flagPt + MapPoint(i + 1, 0), Direction::WEST), 1);
    }
    // End of road
    BOOST_REQUIRE_EQUAL(world.GetPointRoad(flagPt + MapPoint(4, 0), Direction::EAST), 0);
    // BQ on road
    BOOST_REQUIRE_EQUAL(world.GetNode(flagPt + MapPoint(1, 0)).bq, BQ_NOTHING);
    BOOST_REQUIRE_EQUAL(world.GetNode(flagPt + MapPoint(2, 0)).bq, BQ_FLAG);
    BOOST_REQUIRE_EQUAL(world.GetNode(flagPt + MapPoint(3, 0)).bq, BQ_NOTHING);
    BOOST_REQUIRE_EQUAL(world.GetNode(flagPt + MapPoint(4, 0)).bq, BQ_NOTHING);
    // BQ above road
    BOOST_REQUIRE_EQUAL(world.GetNeighbourNode(flagPt + MapPoint(2, 0), Direction::NORTHWEST).bq, BQ_CASTLE);  // Flag could be build
    BOOST_REQUIRE_EQUAL(world.GetNeighbourNode(flagPt + MapPoint(2, 0), Direction::NORTHEAST).bq, BQ_FLAG);    // only flag possible
    // BQ below road (Castle blocked by road)
    BOOST_REQUIRE_EQUAL(world.GetNeighbourNode(flagPt + MapPoint(2, 0), Direction::SOUTHEAST).bq, BQ_HOUSE);
    BOOST_REQUIRE_EQUAL(world.GetNeighbourNode(flagPt + MapPoint(2, 0), Direction::SOUTHWEST).bq, BQ_HOUSE);

    // Set another flag on the road
    // c) to close
    this->SetFlag(flagPt + MapPoint(1, 0));
    BOOST_REQUIRE_EQUAL(world.GetNO(flagPt + MapPoint(1, 0))->GetType(), NOP_NOTHING);
    // d) middle -> ok
    this->SetFlag(flagPt + MapPoint(2, 0));
    BOOST_REQUIRE_EQUAL(world.GetNO(flagPt + MapPoint(2, 0))->GetType(), NOP_FLAG);
    BOOST_REQUIRE_EQUAL(world.GetNeighbourNode(flagPt + MapPoint(2, 0), Direction::NORTHWEST).bq, BQ_CASTLE);  // Flag could be build
    BOOST_REQUIRE_EQUAL(world.GetNeighbourNode(flagPt + MapPoint(2, 0), Direction::NORTHEAST).bq, BQ_NOTHING);    // no more flag possible
    // f) destroy middle flag -> Road destroyed
    this->DestroyFlag(flagPt + MapPoint(2, 0));
    for(unsigned i = 0; i < 4; i++)
        BOOST_REQUIRE_EQUAL(world.GetPointRoad(flagPt + MapPoint(i, 0), Direction::EAST), 0);

    // g) Road with no existing end flag -> Road and flag place
    this->BuildRoad(flagPt, false, std::vector<unsigned char>(2, 3));
    BOOST_REQUIRE_EQUAL(world.GetNO(flagPt + MapPoint(2, 0))->GetType(), NOP_FLAG);
    for(unsigned i = 0; i < 2; i++)
        BOOST_REQUIRE_EQUAL(world.GetPointRoad(flagPt + MapPoint(i, 0), Direction::EAST), 1);

    // h) Non-blocking env. object
    world.SetNO(flagPt + MapPoint(3, 0), new noEnvObject(flagPt, 512));
    BOOST_REQUIRE_EQUAL(world.GetNO(flagPt + MapPoint(3, 0))->GetType(), NOP_ENVIRONMENT);
    this->BuildRoad(flagPt + MapPoint(2, 0), false, std::vector<unsigned char>(2, Direction::EAST));
    for(unsigned i = 2; i < 4; i++)
        BOOST_REQUIRE_EQUAL(world.GetPointRoad(flagPt + MapPoint(i, 0), Direction::EAST), 1);
    BOOST_REQUIRE_EQUAL(world.GetNO(flagPt + MapPoint(3, 0))->GetType(), NOP_NOTHING);

    // Remove other flags
    this->DestroyFlag(flagPt + MapPoint(2, 0));
    this->DestroyFlag(flagPt + MapPoint(4, 0));

    // i) outside player territory
    // i1) border
    this->BuildRoad(flagPt, false, std::vector<unsigned char>(HQ_RADIUS - (flagPt.x - hqPos.x), Direction::EAST));
    for(unsigned i = 0; i <= HQ_RADIUS; i++)
        BOOST_REQUIRE_EQUAL(world.GetPointRoad(flagPt + MapPoint(i, 0), Direction::EAST), 0);
    // i2) territory
    this->BuildRoad(flagPt, false, std::vector<unsigned char>(HQ_RADIUS - (flagPt.x - hqPos.x) + 1, Direction::EAST));
    for(unsigned i = 0; i <= HQ_RADIUS; i++)
        BOOST_REQUIRE_EQUAL(world.GetPointRoad(flagPt + MapPoint(i, 0), Direction::EAST), 0);
}

BOOST_FIXTURE_TEST_CASE(DestroyRoadTest, WorldWithGCExecution2P)
{
    MapPoint flagPt = hqPos + MapPoint(4, 0);
    MapPoint flagPt2 = flagPt + MapPoint(2, 0);
    this->SetFlag(flagPt);
    // Build road with 3 segments:
    this->BuildRoad(flagPt, false, std::vector<unsigned char>(2, Direction::EAST));
    this->BuildRoad(flagPt2, false, std::vector<unsigned char>(2, Direction::EAST));
    for(unsigned i = 0; i < 4; i++)
        BOOST_REQUIRE_EQUAL(world.GetPointRoad(flagPt + MapPoint(i, 0), Direction::EAST), 1);
    // Destroy from middle of road -> fail
    this->DestroyRoad(flagPt2 + MapPoint(1, 0), Direction::EAST);
    for(unsigned i = 0; i < 4; i++)
        BOOST_REQUIRE_EQUAL(world.GetPointRoad(flagPt + MapPoint(i, 0), Direction::EAST), 1);
    // Wrong player -> Fail
    curPlayer = 1;
    this->DestroyRoad(flagPt2, Direction::EAST);
    for(unsigned i = 0; i < 4; i++)
        BOOST_REQUIRE_EQUAL(world.GetPointRoad(flagPt + MapPoint(i, 0), Direction::EAST), 1);
    // Wrong direction -> Fail
    curPlayer = 0;
    this->DestroyRoad(flagPt2, Direction::SOUTHEAST);
    for(unsigned i = 0; i < 4; i++)
        BOOST_REQUIRE_EQUAL(world.GetPointRoad(flagPt + MapPoint(i, 0), Direction::EAST), 1);
    // Correct -> Succeed
    this->DestroyRoad(flagPt2, Direction::EAST);
    for(unsigned i = 0; i < 2; i++)
    {
        BOOST_REQUIRE_EQUAL(world.GetPointRoad(flagPt + MapPoint(i, 0), Direction::EAST), 1);
        BOOST_REQUIRE_EQUAL(world.GetPointRoad(flagPt + MapPoint(i + 2, 0), Direction::EAST), 0);
    }
    // Flags must still exist
    BOOST_REQUIRE(world.GetSpecObj<noFlag>(flagPt));
    BOOST_REQUIRE(world.GetSpecObj<noFlag>(flagPt2));
    BOOST_REQUIRE(world.GetSpecObj<noFlag>(flagPt2 + MapPoint(2, 0)));
    // Rebuild
    this->BuildRoad(flagPt2, false, std::vector<unsigned char>(2, Direction::EAST));
    for(unsigned i = 0; i < 4; i++)
        BOOST_REQUIRE_EQUAL(world.GetPointRoad(flagPt + MapPoint(i, 0), Direction::EAST), 1);
    // Opposite dir
    this->DestroyRoad(flagPt2, Direction::WEST);
    for(unsigned i = 0; i < 2; i++)
    {
        BOOST_REQUIRE_EQUAL(world.GetPointRoad(flagPt + MapPoint(i, 0), Direction::EAST), 0);
        BOOST_REQUIRE_EQUAL(world.GetPointRoad(flagPt + MapPoint(i + 2, 0), Direction::EAST), 1);
    }
    // Both
    this->DestroyRoad(flagPt2, Direction::EAST);
    for(unsigned i = 0; i < 4; i++)
        BOOST_REQUIRE_EQUAL(world.GetPointRoad(flagPt + MapPoint(i, 0), Direction::EAST), 0);
    // Flags must still exist
    BOOST_REQUIRE(world.GetSpecObj<noFlag>(flagPt));
    BOOST_REQUIRE(world.GetSpecObj<noFlag>(flagPt2));
    BOOST_REQUIRE(world.GetSpecObj<noFlag>(flagPt2 + MapPoint(2, 0)));
}

BOOST_FIXTURE_TEST_CASE(UpgradeRoadTest, WorldWithGCExecution2P)
{
    // 3 Roads, so we have a middle flag, and an additional road
    const MapPoint hqFlagPos = world.GetNeighbour(hqPos, Direction::SOUTHEAST);
    this->BuildRoad(hqFlagPos, false, std::vector<unsigned char>(2, Direction::EAST));
    this->BuildRoad(hqFlagPos + MapPoint(2, 0), false, std::vector<unsigned char>(2, Direction::EAST));
    this->BuildRoad(hqFlagPos + MapPoint(4, 0), false, std::vector<unsigned char>(2, Direction::EAST));
    const MapPoint middleFlag = hqFlagPos + MapPoint(2, 0);
    const noFlag* hqFlag = world.GetSpecObj<noFlag>(hqFlagPos);
    const noFlag* flag = world.GetSpecObj<noFlag>(middleFlag);
    const noFlag* flag2 = world.GetSpecObj<noFlag>(middleFlag + MapPoint(2, 0));
    BOOST_REQUIRE(hqFlag);
    BOOST_REQUIRE(flag);
    BOOST_REQUIRE(flag2);
    BOOST_REQUIRE_EQUAL(flag->GetFlagType(), FT_NORMAL);
    // No flag
    this->UpgradeRoad(middleFlag - MapPoint(1, 0), Direction::EAST);
    BOOST_REQUIRE_EQUAL(flag->GetFlagType(), FT_NORMAL);
    // Wrong direction
    this->UpgradeRoad(middleFlag, Direction::SOUTHEAST);
    BOOST_REQUIRE_EQUAL(flag->GetFlagType(), FT_NORMAL);
    // Upgrading correctly but twice (2nd does nothing)
    for(unsigned i=0; i<2; i++)
    {
        // Correct
        this->UpgradeRoad(middleFlag, Direction::EAST);
        BOOST_CHECK_EQUAL(flag->GetFlagType(), FT_LARGE);
        BOOST_CHECK_EQUAL(flag2->GetFlagType(), FT_LARGE);
        BOOST_CHECK_EQUAL(hqFlag->GetFlagType(), FT_NORMAL);
        BOOST_CHECK_EQUAL(world.GetPointRoad(middleFlag, Direction::EAST), RoadSegment::RT_DONKEY + 1);
        BOOST_CHECK_EQUAL(world.GetPointRoad(middleFlag + MapPoint(1, 0), Direction::EAST), RoadSegment::RT_DONKEY + 1);
        BOOST_CHECK_EQUAL(world.GetPointRoad(middleFlag + MapPoint(2, 0), Direction::EAST), RoadSegment::RT_NORMAL + 1);
        BOOST_CHECK_EQUAL(world.GetPointRoad(middleFlag, Direction::WEST), RoadSegment::RT_NORMAL + 1);
    }
    // Upgrade in other direction
    this->UpgradeRoad(middleFlag, Direction::WEST);
    BOOST_CHECK_EQUAL(flag->GetFlagType(), FT_LARGE);
    BOOST_CHECK_EQUAL(flag2->GetFlagType(), FT_LARGE);
    BOOST_CHECK_EQUAL(hqFlag->GetFlagType(), FT_LARGE);
    BOOST_CHECK_EQUAL(world.GetPointRoad(middleFlag, Direction::EAST), RoadSegment::RT_DONKEY + 1);
    BOOST_CHECK_EQUAL(world.GetPointRoad(middleFlag + MapPoint(1, 0), Direction::EAST), RoadSegment::RT_DONKEY + 1);
    BOOST_CHECK_EQUAL(world.GetPointRoad(middleFlag + MapPoint(2, 0), Direction::EAST), RoadSegment::RT_NORMAL + 1);
    BOOST_CHECK_EQUAL(world.GetPointRoad(middleFlag, Direction::WEST), RoadSegment::RT_DONKEY + 1);
}

BOOST_FIXTURE_TEST_CASE(PlayerEconomySettings, WorldWithGCExecution2P)
{
    // Execute for both players to make sure they don't influence each other
    for(; curPlayer < 2; curPlayer++)
    {
        Distributions inDist;
        for(unsigned i = 0; i < inDist.size(); i++)
            inDist[i] = rand();
        this->ChangeDistribution(inDist);

        bool orderType = rand() % 2 == 0;
        BuildOrders inBuildOrder = GamePlayer::GetStandardBuildOrder();
        std::random_shuffle(inBuildOrder.begin(), inBuildOrder.end());
        this->ChangeBuildOrder(orderType, inBuildOrder);

        TransportOrders inTransportOrder;
        for(unsigned i = 0; i < inTransportOrder.size(); i++)
            inTransportOrder[i] = i;
        std::random_shuffle(inTransportOrder.begin(), inTransportOrder.end());
        this->ChangeTransport(inTransportOrder);

        MilitarySettings militarySettings;
        for(unsigned i = 0; i < militarySettings.size(); ++i)
            militarySettings[i] = rand() % (MILITARY_SETTINGS_SCALE[i] + 1);
        this->ChangeMilitary(militarySettings);

        ToolSettings toolPrios;
        for(unsigned i = 0; i < toolPrios.size(); ++i)
            toolPrios[i] = rand() % 11;
        this->ChangeTools(toolPrios);

        const GamePlayer& player = world.GetPlayer(curPlayer);
        // TODO: Use better getters once available
        VisualSettings outSettings;
        player.FillVisualSettings(outSettings);
        for(unsigned i = 0; i < inDist.size(); i++)
            BOOST_REQUIRE_EQUAL(outSettings.distribution[i], inDist[i]);
        BOOST_REQUIRE_EQUAL(outSettings.useCustomBuildOrder, orderType);
        for(unsigned i = 0; i < inBuildOrder.size(); i++)
            BOOST_REQUIRE_EQUAL(outSettings.build_order[i], inBuildOrder[i]);
        for(unsigned i = 0; i < inTransportOrder.size(); i++)
            BOOST_REQUIRE_EQUAL(outSettings.transport_order[i], inTransportOrder[i]);
        for(unsigned i = 0; i < WARE_TYPES_COUNT; i++)
            BOOST_REQUIRE_EQUAL(player.GetTransportPriority(GoodType(i)), GetTransportPrioFromOrdering(inTransportOrder, GoodType(i)));
        for(unsigned i = 0; i < militarySettings.size(); i++)
            BOOST_REQUIRE_EQUAL(player.GetMilitarySetting(i), militarySettings[i]);
        for(unsigned i = 0; i < toolPrios.size(); i++)
            BOOST_REQUIRE_EQUAL(player.GetToolPriority(i), toolPrios[i]);
    }
}

BOOST_FIXTURE_TEST_CASE(BuildBuilding, WorldWithGCExecution2P)
{
    const MapPoint closePt = hqPos + MapPoint(2, 0);
    const MapPoint farmPt = hqPos + MapPoint(6, 0);
    const MapPoint closeMilPt = hqPos - MapPoint(4, 0);
    const MapPoint okMilPt = hqPos - MapPoint(5, 0);
    // Wrong BQ (blocked by HQ)
    this->SetBuildingSite(closePt, BLD_FARM);
    BOOST_REQUIRE_EQUAL(world.GetNO(closePt)->GetType(), NOP_NOTHING);
    // OK
    this->SetBuildingSite(closePt, BLD_WOODCUTTER);
    BOOST_REQUIRE_EQUAL(world.GetNO(closePt)->GetType(), NOP_BUILDINGSITE);
    BOOST_REQUIRE_EQUAL(world.GetSpecObj<noBaseBuilding>(closePt)->GetBuildingType(), BLD_WOODCUTTER);
    BOOST_REQUIRE_EQUAL(world.GetNO(world.GetNeighbour(closePt, Direction::SOUTHEAST))->GetType(), NOP_FLAG);
    // OK
    this->SetBuildingSite(farmPt, BLD_FARM);
    BOOST_REQUIRE_EQUAL(world.GetNO(farmPt)->GetType(), NOP_BUILDINGSITE);
    BOOST_REQUIRE_EQUAL(world.GetSpecObj<noBaseBuilding>(farmPt)->GetBuildingType(), BLD_FARM);
    BOOST_REQUIRE_EQUAL(world.GetNO(world.GetNeighbour(farmPt, Direction::SOUTHEAST))->GetType(), NOP_FLAG);
    // Millitary bld to close
    this->SetBuildingSite(closeMilPt, BLD_BARRACKS);
    BOOST_REQUIRE_EQUAL(world.GetNO(closeMilPt)->GetType(), NOP_NOTHING);
    // Millitary bld ok
    this->SetBuildingSite(okMilPt, BLD_BARRACKS);
    BOOST_REQUIRE_EQUAL(world.GetNO(okMilPt)->GetType(), NOP_BUILDINGSITE);
    BOOST_REQUIRE_EQUAL(world.GetSpecObj<noBaseBuilding>(okMilPt)->GetBuildingType(), BLD_BARRACKS);
    BOOST_REQUIRE_EQUAL(world.GetNO(world.GetNeighbour(okMilPt, Direction::SOUTHEAST))->GetType(), NOP_FLAG);

    // Remove bld
    this->DestroyBuilding(farmPt);
    BOOST_REQUIRE_EQUAL(world.GetNO(farmPt)->GetType(), NOP_NOTHING);
    BOOST_REQUIRE_EQUAL(world.GetNO(world.GetNeighbour(farmPt, Direction::SOUTHEAST))->GetType(), NOP_FLAG);

    // Remove flag -> bld also destroyed
    this->DestroyFlag(world.GetNeighbour(okMilPt, Direction::SOUTHEAST));
    BOOST_REQUIRE_EQUAL(world.GetNO(okMilPt)->GetType(), NOP_NOTHING);
    BOOST_REQUIRE_EQUAL(world.GetNO(world.GetNeighbour(okMilPt, Direction::SOUTHEAST))->GetType(), NOP_NOTHING);

    // Check if bld is build
    this->BuildRoad(world.GetNeighbour(hqPos, Direction::SOUTHEAST), false, std::vector<unsigned char>(2, Direction::EAST));
    for(unsigned i = 0; i < 1200; i++)
        this->em.ExecuteNextGF();
    BOOST_REQUIRE_EQUAL(world.GetNO(closePt)->GetType(), NOP_BUILDING);
    BOOST_REQUIRE_EQUAL(world.GetNO(closePt)->GetGOT(), GOT_NOB_USUAL);
    BOOST_REQUIRE_EQUAL(world.GetSpecObj<noBaseBuilding>(closePt)->GetBuildingType(), BLD_WOODCUTTER);

    // Destroy finished bld -> Fire
    this->DestroyBuilding(closePt);
    BOOST_REQUIRE_EQUAL(world.GetNO(closePt)->GetType(), NOP_FIRE);
    // Destroy twice -> Nothing happens
    this->DestroyBuilding(closePt);
    BOOST_REQUIRE_EQUAL(world.GetNO(closePt)->GetType(), NOP_FIRE);
    // Try to build on fire
    this->SetBuildingSite(closePt, BLD_WOODCUTTER);
    BOOST_REQUIRE_EQUAL(world.GetNO(closePt)->GetType(), NOP_FIRE);
}

BOOST_FIXTURE_TEST_CASE(SendSoldiersHomeTest, WorldWithGCExecution2P)
{
    const MapPoint milPt = hqPos + MapPoint(6, 0);
    // Setup: Give player 3 generals
    GamePlayer& player = world.GetPlayer(curPlayer);
    nobBaseWarehouse* wh = player.GetFirstWH();
    BOOST_REQUIRE(wh);
    BOOST_REQUIRE_EQUAL(wh->GetInventory().people[JOB_GENERAL], 0u);
    Inventory goods;
    goods.Add(JOB_PRIVATEFIRSTCLASS, 1);
    goods.Add(JOB_SERGEANT, 1);
    goods.Add(JOB_OFFICER, 1);
    goods.Add(JOB_GENERAL, 2);
    wh->AddGoods(goods, true);
    // Set all military stuff to max
    this->ChangeMilitary(MILITARY_SETTINGS_SCALE);
    // Build a watchtower and connect it
    nobMilitary* bld = dynamic_cast<nobMilitary*>(BuildingFactory::CreateBuilding(&world, BLD_WATCHTOWER, milPt, curPlayer, player.nation));
    BOOST_REQUIRE(bld);
    this->BuildRoad(world.GetNeighbour(hqPos, Direction::SOUTHEAST), false, std::vector<unsigned char>((milPt.x - hqPos.x), Direction::EAST));
    // Now run some GFs so the bld is occupied (<=30GFs/per Soldier for leaving HQ, 20GFs per node walked, 50GFs safety)
    unsigned numGFtillAllArrive = 30 * 6 + 20 * (milPt.x - hqPos.x) + 50;
    for(unsigned i = 0; i < numGFtillAllArrive; i++)
        this->em.ExecuteNextGF();
    // Now we should have 1 each of ranks 0-3 and 2 rank 4s
    BOOST_REQUIRE_EQUAL(bld->GetTroopsCount(), 6u);
    SortedTroops::const_iterator itTroops = bld->GetTroops().begin();
    for(unsigned i = 0; i < 4; i++, ++itTroops)
        BOOST_REQUIRE_EQUAL((*itTroops)->GetRank(), i);
    for(unsigned i = 0; i < 2; i++, ++itTroops)
        BOOST_REQUIRE_EQUAL((*itTroops)->GetRank(), 4);
    // End preparation

    unsigned expectedTroopCt = 6u;
    // send each of the higher ranks home
    for(unsigned curRank = 4; curRank > 0; --curRank)
    {
        this->SendSoldiersHome(milPt);
        expectedTroopCt -= (curRank == 4) ? 2 : 1; // 2 generals, 1 of the others
        BOOST_REQUIRE_EQUAL(bld->GetTroopsCount(), expectedTroopCt);
        itTroops = bld->GetTroops().begin();
        for(unsigned i = 0; i < expectedTroopCt; i++, ++itTroops)
            BOOST_REQUIRE_EQUAL((*itTroops)->GetRank(), i);
    }
    // One low rank is left
    BOOST_REQUIRE_EQUAL(bld->GetTroopsCount(), 1u);
    this->SendSoldiersHome(milPt);
    // But he must stay
    BOOST_REQUIRE_EQUAL(bld->GetTroopsCount(), 1u);

    // Wait till new soldiers have arrived
    for(unsigned i = 0; i < numGFtillAllArrive; i++)
        this->em.ExecuteNextGF();

    // 6 low ranks
    BOOST_REQUIRE_EQUAL(bld->GetTroopsCount(), 6u);
    itTroops = bld->GetTroops().begin();
    for(unsigned i = 0; i < 6; i++, ++itTroops)
        BOOST_REQUIRE_EQUAL((*itTroops)->GetRank(), 0u);

    // Send 5 of them home
    this->SendSoldiersHome(milPt);
    BOOST_REQUIRE_EQUAL(bld->GetTroopsCount(), 1u);

    // Wait till one left so new ones get ordered
    for(unsigned i = 0; i < 40; i++)
        this->em.ExecuteNextGF();

    // All higher rank soldiers should have been ordered and hence removed from the real inventory
    for(unsigned i = 1; i < SOLDIER_JOBS.size(); i++)
        BOOST_REQUIRE_EQUAL(wh->GetRealFiguresCount(SOLDIER_JOBS[i]), 0u);

    // Allow one of them to leave the HQ
    for(unsigned i = 0; i < 40; i++)
        this->em.ExecuteNextGF();

    // Now cancel orders for generals and replace with low rank ones
    this->OrderNewSoldiers(milPt);

    // Wait till new soldiers have arrived
    for(unsigned i = 0; i < numGFtillAllArrive; i++)
        this->em.ExecuteNextGF();

    // 3 low ranks and 1 each of other ranks except general
    BOOST_REQUIRE_EQUAL(bld->GetTroopsCount(), 6u);
    itTroops = bld->GetTroops().begin();
    for(unsigned i = 0; i < 3; i++, ++itTroops)
        BOOST_REQUIRE_EQUAL((*itTroops)->GetRank(), 0u);
    for(unsigned i = 1; i < 3; i++, ++itTroops)
        BOOST_REQUIRE_EQUAL((*itTroops)->GetRank(), i);
}

namespace{
    template<typename T_CallWorker>
    void FlagWorkerTest(WorldWithGCExecution2P& worldFixture, Job workerJob, GoodType toolType, T_CallWorker callWorker)
    {
        const MapPoint flagPt = worldFixture.world.GetNeighbour(worldFixture.hqPos, Direction::SOUTHEAST) + MapPoint(3, 0);
        GamePlayer& player = worldFixture.world.GetPlayer(worldFixture.curPlayer);
        nobBaseWarehouse* wh = player.GetFirstWH();
        BOOST_REQUIRE(wh);

        const unsigned startFigureCt = wh->GetRealFiguresCount(workerJob);
        const unsigned startToolsCt = wh->GetRealWaresCount(toolType);
        // We need some of them!
        BOOST_REQUIRE_GT(startFigureCt, 0u);
        BOOST_REQUIRE_GT(startToolsCt, 0u);

        // No flag -> Nothing happens
        callWorker(flagPt);
        BOOST_REQUIRE_EQUAL(wh->GetRealFiguresCount(workerJob), startFigureCt);
        BOOST_REQUIRE_EQUAL(wh->GetLeavingFigures().size(), 0u);

        worldFixture.SetFlag(flagPt);
        // Unconnected flag -> Nothing happens
        callWorker(flagPt);
        BOOST_REQUIRE_EQUAL(wh->GetRealFiguresCount(workerJob), startFigureCt);
        BOOST_REQUIRE_EQUAL(wh->GetLeavingFigures().size(), 0u);

        // Build road and let worker leave
        worldFixture.BuildRoad(flagPt, false, std::vector<unsigned char>(3, Direction::WEST));
        for(unsigned i = 0; i < 30; i++)
            worldFixture.em.ExecuteNextGF();
        BOOST_REQUIRE_EQUAL(wh->GetLeavingFigures().size(), 0u);

        // Call one geologist to flag
        callWorker(flagPt);
        BOOST_REQUIRE_EQUAL(wh->GetRealFiguresCount(workerJob) + 1, startFigureCt);
        BOOST_REQUIRE_EQUAL(wh->GetLeavingFigures().size(), 1u);
        BOOST_REQUIRE_EQUAL(wh->GetLeavingFigures().front()->GetJobType(), workerJob);

        // Call remaining ones
        for(unsigned i = 1; i < startFigureCt; i++)
            callWorker(flagPt);
        BOOST_REQUIRE_EQUAL(wh->GetRealFiguresCount(workerJob), 0u);
        BOOST_REQUIRE_EQUAL(wh->GetLeavingFigures().size(), startFigureCt);

        // Recruit all possible ones
        BOOST_REQUIRE_EQUAL(wh->GetRealWaresCount(toolType), startToolsCt);
        for(unsigned i = 0; i < startToolsCt; i++)
            callWorker(flagPt);
        BOOST_REQUIRE_EQUAL(wh->GetRealFiguresCount(workerJob), 0u);
        BOOST_REQUIRE_EQUAL(wh->GetLeavingFigures().size(), startFigureCt + startToolsCt);

        // And an extra one -> Fail
        callWorker(flagPt);
        BOOST_REQUIRE_EQUAL(wh->GetRealFiguresCount(workerJob), 0u);
        BOOST_REQUIRE_EQUAL(wh->GetLeavingFigures().size(), startFigureCt + startToolsCt);
    }
}

BOOST_FIXTURE_TEST_CASE(CallGeologist, WorldWithGCExecution2P)
{
    FlagWorkerTest(*this, JOB_GEOLOGIST, GD_HAMMER, boost::lambda::bind(&GameCommandFactory::CallGeologist, this, boost::lambda::_1));
}

BOOST_FIXTURE_TEST_CASE(CallScout, WorldWithGCExecution2P)
{
    FlagWorkerTest(*this, JOB_SCOUT, GD_BOW, boost::lambda::bind(&GameCommandFactory::CallScout, this, boost::lambda::_1));
}

BOOST_FIXTURE_TEST_CASE(ChangeCoinAccept, WorldWithGCExecution2P)
{
    const MapPoint bldPt = hqPos + MapPoint(3, 0);
    nobMilitary* bld = dynamic_cast<nobMilitary*>(BuildingFactory::CreateBuilding(&world, BLD_WATCHTOWER, bldPt, curPlayer, NAT_ROMANS));
    BOOST_REQUIRE(bld);
    BOOST_REQUIRE(!bld->IsGoldDisabled());

    // Enable (already is)
    this->SetCoinsAllowed(bldPt, true);
    BOOST_REQUIRE(!bld->IsGoldDisabled());

    // Disable
    this->SetCoinsAllowed(bldPt, false);
    BOOST_REQUIRE(bld->IsGoldDisabled());

    // Reenable
    this->SetCoinsAllowed(bldPt, true);
    BOOST_REQUIRE(!bld->IsGoldDisabled());

    // Production should have no effect
    this->SetProductionEnabled(bldPt, true);
    BOOST_REQUIRE(!bld->IsGoldDisabled());
    this->SetProductionEnabled(bldPt, false);
    BOOST_REQUIRE(!bld->IsGoldDisabled());
}

BOOST_FIXTURE_TEST_CASE(DisableProduction, WorldWithGCExecution2P)
{
    const MapPoint bldPt = hqPos + MapPoint(3, 0);
    nobUsual* bld = dynamic_cast<nobUsual*>(BuildingFactory::CreateBuilding(&world, BLD_FORESTER, bldPt, curPlayer, NAT_ROMANS));
    BOOST_REQUIRE(bld);
    BOOST_REQUIRE(!bld->IsProductionDisabled());

    // Enable (already is)
    this->SetProductionEnabled(bldPt, true);
    BOOST_REQUIRE(!bld->IsProductionDisabled());

    // Disable
    this->SetProductionEnabled(bldPt, false);
    BOOST_REQUIRE(bld->IsProductionDisabled());

    // Reenable
    this->SetProductionEnabled(bldPt, true);
    BOOST_REQUIRE(!bld->IsProductionDisabled());

    // Coins should have no effect
    this->SetCoinsAllowed(bldPt, true);
    BOOST_REQUIRE(!bld->IsProductionDisabled());
    this->SetCoinsAllowed(bldPt, false);
    BOOST_REQUIRE(!bld->IsProductionDisabled());
}

namespace{
    void InitPactsAndPost(GameWorldBase& world)
    {
        for(unsigned i = 0; i < world.GetPlayerCount(); i++)
        {
            world.GetPlayer(i).MakeStartPacts();
            PostBox& box = *world.GetPostMgr().GetPostBox(i);
            const unsigned numMsgs = box.GetNumMsgs();
            for(unsigned j = 0; j < numMsgs; j++)
                BOOST_REQUIRE(box.DeleteMsg(0u));
            BOOST_REQUIRE_EQUAL(box.GetNumMsgs(), 0u);
        }
    }
}

BOOST_FIXTURE_TEST_CASE(NotifyAllies, WorldWithGCExecution3P)
{
    // At first there are no teams
    for(unsigned i = 0; i < world.GetPlayerCount(); i++)
        BOOST_REQUIRE_EQUAL(world.GetPlayer(i).team, TM_NOTEAM);
    // Add postbox for each player
    for(unsigned i = 0; i < world.GetPlayerCount(); i++)
        world.GetPostMgr().AddPostBox(i);
    // Choose middle player so we can observe side effects and off-by-one errors
    curPlayer = 1;

    // No ally -> no messages
    this->NotifyAlliesOfLocation(hqPos);
    for(unsigned i = 0; i < world.GetPlayerCount(); i++)
        BOOST_REQUIRE_EQUAL(world.GetPostMgr().GetPostBox(i)->GetNumMsgs(), 0u);
    // Still no allies
    world.GetPlayer(1).team = TM_TEAM1;
    InitPactsAndPost(world);
    this->NotifyAlliesOfLocation(hqPos);
    for(unsigned i = 0; i < world.GetPlayerCount(); i++)
        BOOST_REQUIRE_EQUAL(world.GetPostMgr().GetPostBox(i)->GetNumMsgs(), 0u);

    // First 2 players are allied -> Message received by player 0 only
    world.GetPlayer(0).team = TM_TEAM1;
    world.GetPlayer(1).team = TM_TEAM1;
    InitPactsAndPost(world);
    this->NotifyAlliesOfLocation(hqPos);
    BOOST_REQUIRE_EQUAL(world.GetPostMgr().GetPostBox(0u)->GetNumMsgs(), 1u);
    for(unsigned i = 1; i < world.GetPlayerCount(); i++)
        BOOST_REQUIRE_EQUAL(world.GetPostMgr().GetPostBox(i)->GetNumMsgs(), 0u);

    // Same if player 2 is in another team
    world.GetPlayer(0).team = TM_TEAM1;
    world.GetPlayer(1).team = TM_TEAM1;
    world.GetPlayer(2).team = TM_TEAM2;
    InitPactsAndPost(world);
    this->NotifyAlliesOfLocation(hqPos);
    BOOST_REQUIRE_EQUAL(world.GetPostMgr().GetPostBox(0u)->GetNumMsgs(), 1u);
    for(unsigned i = 1; i < world.GetPlayerCount(); i++)
        BOOST_REQUIRE_EQUAL(world.GetPostMgr().GetPostBox(i)->GetNumMsgs(), 0u);

    // player 2 is in same team
    world.GetPlayer(0).team = TM_TEAM1;
    world.GetPlayer(1).team = TM_TEAM2;
    world.GetPlayer(2).team = TM_TEAM2;
    InitPactsAndPost(world);
    this->NotifyAlliesOfLocation(hqPos);
    BOOST_REQUIRE_EQUAL(world.GetPostMgr().GetPostBox(2u)->GetNumMsgs(), 1u);
    for(unsigned i = 0; i < 2; i++)
        BOOST_REQUIRE_EQUAL(world.GetPostMgr().GetPostBox(i)->GetNumMsgs(), 0u);

    // All are in same team
    world.GetPlayer(0).team = TM_TEAM3;
    world.GetPlayer(1).team = TM_TEAM3;
    world.GetPlayer(2).team = TM_TEAM3;
    InitPactsAndPost(world);
    this->NotifyAlliesOfLocation(hqPos);
    BOOST_REQUIRE_EQUAL(world.GetPostMgr().GetPostBox(0u)->GetNumMsgs(), 1u);
    BOOST_REQUIRE_EQUAL(world.GetPostMgr().GetPostBox(1u)->GetNumMsgs(), 0u);
    BOOST_REQUIRE_EQUAL(world.GetPostMgr().GetPostBox(2u)->GetNumMsgs(), 1u);
}

BOOST_AUTO_TEST_CASE(InventorySettingType)
{
    InventorySetting setting;
    // Default setting is 0
    BOOST_REQUIRE_EQUAL(setting.ToUnsignedChar(), 0u);

    // Test all 3 single types
    setting = EInventorySetting::STOP;
    BOOST_REQUIRE(setting.IsSet(EInventorySetting::STOP));
    BOOST_CHECK(!setting.IsSet(EInventorySetting::SEND));
    BOOST_CHECK(!setting.IsSet(EInventorySetting::COLLECT));

    setting = EInventorySetting::SEND;
    BOOST_CHECK(!setting.IsSet(EInventorySetting::STOP));
    BOOST_REQUIRE(setting.IsSet(EInventorySetting::SEND));
    BOOST_CHECK(!setting.IsSet(EInventorySetting::COLLECT));

    setting = EInventorySetting::COLLECT;
    BOOST_CHECK(!setting.IsSet(EInventorySetting::STOP));
    BOOST_CHECK(!setting.IsSet(EInventorySetting::SEND));
    BOOST_REQUIRE(setting.IsSet(EInventorySetting::COLLECT));

    // Reset and test toggle
    setting = InventorySetting();
    setting.Toggle(EInventorySetting::STOP);
    BOOST_REQUIRE_EQUAL(setting, EInventorySetting::STOP);
    setting.Toggle(EInventorySetting::SEND);
    // Both set
    BOOST_REQUIRE(setting.IsSet(EInventorySetting::STOP));
    BOOST_REQUIRE(setting.IsSet(EInventorySetting::SEND));
    BOOST_REQUIRE(!setting.IsSet(EInventorySetting::COLLECT));

    // Resets others
    setting.Toggle(EInventorySetting::COLLECT);
    BOOST_REQUIRE_EQUAL(setting, EInventorySetting::COLLECT);
    // Resets collect
    setting.Toggle(EInventorySetting::STOP);
    BOOST_REQUIRE_EQUAL(setting, EInventorySetting::STOP);

    // Enable send, disable stop
    setting.Toggle(EInventorySetting::SEND);
    setting.Toggle(EInventorySetting::STOP);
    BOOST_REQUIRE_EQUAL(setting, EInventorySetting::SEND);
}

BOOST_FIXTURE_TEST_CASE(SetInventorySettingTest, WorldWithGCExecution2P)
{
    GamePlayer& player = world.GetPlayer(curPlayer);
    nobBaseWarehouse* wh = player.GetFirstWH();
    BOOST_REQUIRE(wh);
    InventorySetting expectedSetting;
    BOOST_REQUIRE_EQUAL(wh->GetInventorySetting(GD_BOARDS), expectedSetting);
    BOOST_REQUIRE_EQUAL(wh->GetInventorySetting(JOB_PRIVATE), expectedSetting);
    expectedSetting.Toggle(EInventorySetting::STOP);
    expectedSetting.Toggle(EInventorySetting::SEND);

    this->SetInventorySetting(hqPos, GD_BOARDS, expectedSetting);
    BOOST_REQUIRE_EQUAL(wh->GetInventorySetting(GD_BOARDS), expectedSetting);
    this->SetInventorySetting(hqPos, JOB_PRIVATE, expectedSetting);
    BOOST_REQUIRE_EQUAL(wh->GetInventorySetting(JOB_PRIVATE), expectedSetting);

    expectedSetting.Toggle(EInventorySetting::COLLECT);
    this->SetInventorySetting(hqPos, GD_BOARDS, expectedSetting);
    BOOST_REQUIRE_EQUAL(wh->GetInventorySetting(GD_BOARDS), expectedSetting);
    this->SetInventorySetting(hqPos, JOB_PRIVATE, expectedSetting);
    BOOST_REQUIRE_EQUAL(wh->GetInventorySetting(JOB_PRIVATE), expectedSetting);

    std::vector<InventorySetting> settings(JOB_TYPES_COUNT);
    for(unsigned i = 0; i < JOB_TYPES_COUNT; i++)
        settings[i] = InventorySetting(rand() % 5);
    this->SetAllInventorySettings(hqPos, true, settings);
    for(unsigned i = 0; i < JOB_TYPES_COUNT; i++)
    {
        // Boat carriers are stored as helpers and boats
        if(i == JOB_BOATCARRIER)
            BOOST_REQUIRE_EQUAL(wh->GetInventorySetting(Job(i)), settings[JOB_HELPER]);
        else
            BOOST_REQUIRE_EQUAL(wh->GetInventorySetting(Job(i)), settings[i]);
    }

    settings.resize(WARE_TYPES_COUNT);
    for(unsigned i = 0; i < WARE_TYPES_COUNT; i++)
        settings[i] = InventorySetting(rand() % 5);
    this->SetAllInventorySettings(hqPos, false, settings);
    for(unsigned i = 0; i < JOB_TYPES_COUNT; i++)
        BOOST_REQUIRE_EQUAL(wh->GetInventorySetting(GoodType(i)), settings[ConvertShields(GoodType(i))]);
}

BOOST_FIXTURE_TEST_CASE(ChangeReserveTest, WorldWithGCExecution2P)
{
    GamePlayer& player = world.GetPlayer(curPlayer);
    nobBaseWarehouse* wh = player.GetFirstWH();
    BOOST_REQUIRE(wh);
    Inventory goods;

    // Add enough soldiers per rank
    for(unsigned i = 0; i < SOLDIER_JOBS.size(); i++)
    {
        goods.Add(SOLDIER_JOBS[i], 50);
        BOOST_REQUIRE_EQUAL(*wh->GetReservePointerAvailable(i), 0u);
    }
    wh->AddGoods(goods, true);

    // Use more
    for(unsigned i = 0; i < SOLDIER_JOBS.size(); i++)
    {
        unsigned newVal = i * 5 + 2;
        unsigned numSoldiersAv = wh->GetVisualFiguresCount(SOLDIER_JOBS[i]);
        BOOST_REQUIRE_EQUAL(wh->GetRealFiguresCount(SOLDIER_JOBS[i]), numSoldiersAv);
        // Update reserve -> Removed from inventory
        this->ChangeReserve(hqPos, i, newVal);
        BOOST_REQUIRE_EQUAL(*wh->GetReservePointerAvailable(i), newVal);
        BOOST_REQUIRE_EQUAL(wh->GetVisualFiguresCount(SOLDIER_JOBS[i]), numSoldiersAv - newVal);
        BOOST_REQUIRE_EQUAL(wh->GetRealFiguresCount(SOLDIER_JOBS[i]), numSoldiersAv - newVal);
    }
    // Use less
    for(unsigned i = 0; i < SOLDIER_JOBS.size(); i++)
    {
        unsigned newVal = i * 3 + 1;
        unsigned numSoldiersAv = wh->GetVisualFiguresCount(SOLDIER_JOBS[i]);
        unsigned numSoldiersReleased = *wh->GetReservePointerAvailable(i) - newVal;
        // Release some soldiers from reserve -> Added to inventory
        this->ChangeReserve(hqPos, i, newVal);
        BOOST_REQUIRE_EQUAL(*wh->GetReservePointerAvailable(i), newVal);
        BOOST_REQUIRE_EQUAL(wh->GetVisualFiguresCount(SOLDIER_JOBS[i]), numSoldiersAv + numSoldiersReleased);
        BOOST_REQUIRE_EQUAL(wh->GetRealFiguresCount(SOLDIER_JOBS[i]), numSoldiersAv + numSoldiersReleased);
    }
}

BOOST_FIXTURE_TEST_CASE(Armageddon, WorldWithGCExecution2P)
{
    GamePlayer& player1 = world.GetPlayer(0);
    GamePlayer& player2 = world.GetPlayer(1);
    MapPoint hqPt1 = player1.GetHQPos();
    MapPoint hqPt2 = player2.GetHQPos();
    BOOST_REQUIRE(hqPt1.isValid());
    BOOST_REQUIRE(hqPt2.isValid());
    // Destroy everything
    this->CheatArmageddon();
    BOOST_REQUIRE(!player1.GetHQPos().isValid());
    BOOST_REQUIRE(!player2.GetHQPos().isValid());
    RTTR_FOREACH_PT(MapPoint, world.GetWidth(), world.GetHeight())
    {
        const MapNode& node = world.GetNode(pt);
        BOOST_REQUIRE_EQUAL(node.owner, 0u);
    }
    BOOST_REQUIRE_NE(world.GetNO(hqPt1)->GetGOT(), GOT_NOB_HQ);
    BOOST_REQUIRE_NE(world.GetNO(hqPt2)->GetGOT(), GOT_NOB_HQ);
    BOOST_REQUIRE(player1.IsDefeated());
    BOOST_REQUIRE(player2.IsDefeated());
}

BOOST_FIXTURE_TEST_CASE(DestroyAllTest, WorldWithGCExecution2P)
{
    GamePlayer& player1 = world.GetPlayer(0);
    GamePlayer& player2 = world.GetPlayer(1);
    MapPoint hqPt1 = player1.GetHQPos();
    MapPoint hqPt2 = player2.GetHQPos();
    BOOST_REQUIRE(hqPt1.isValid());
    BOOST_REQUIRE(hqPt2.isValid());
    // Destroy only own buildings
    this->DestroyAll();
    BOOST_REQUIRE(!player1.GetHQPos().isValid());
    BOOST_REQUIRE(player2.GetHQPos().isValid());
    RTTR_FOREACH_PT(MapPoint, world.GetWidth(), world.GetHeight())
    {
        const MapNode& node = world.GetNode(pt);
        BOOST_REQUIRE_NE(node.owner, 1u);
    }
    BOOST_REQUIRE_NE(world.GetNO(hqPt1)->GetGOT(), GOT_NOB_HQ);
    BOOST_REQUIRE_EQUAL(world.GetNO(hqPt2)->GetGOT(), GOT_NOB_HQ);
    BOOST_REQUIRE(player1.IsDefeated());
    BOOST_REQUIRE(!player2.IsDefeated());
}

BOOST_FIXTURE_TEST_CASE(SurrenderTest, WorldWithGCExecution2P)
{
    GamePlayer& player1 = world.GetPlayer(0);
    GamePlayer& player2 = world.GetPlayer(1);
    MapPoint hqPt1 = player1.GetHQPos();
    MapPoint hqPt2 = player2.GetHQPos();
    BOOST_REQUIRE(hqPt1.isValid());
    BOOST_REQUIRE(hqPt2.isValid());
    // Only sets defeated flag
    this->Surrender();
    BOOST_REQUIRE(player1.GetHQPos().isValid());
    BOOST_REQUIRE(player2.GetHQPos().isValid());
    BOOST_REQUIRE_NE(world.GetNode(hqPt1).obj, (const noBase*) NULL);
    BOOST_REQUIRE_NE(world.GetNode(hqPt2).obj, (const noBase*) NULL);
    BOOST_REQUIRE(player1.IsDefeated());
    BOOST_REQUIRE(!player2.IsDefeated());
}

BOOST_AUTO_TEST_SUITE_END()
