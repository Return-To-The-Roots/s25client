// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "GamePlayer.h"
#include "PointOutput.h"
#include "RttrForeachPt.h"
#include "addons/const_addons.h"
#include "buildings/nobBaseWarehouse.h"
#include "buildings/nobMilitary.h"
#include "buildings/nobUsual.h"
#include "enum_cast.hpp"
#include "factories/BuildingFactory.h"
#include "figures/nofPassiveSoldier.h"
#include "postSystem/PostBox.h"
#include "worldFixtures/WorldWithGCExecution.h"
#include "worldFixtures/initGameRNG.hpp"
#include "nodeObjs/noBase.h"
#include "nodeObjs/noEnvObject.h"
#include "nodeObjs/noFlag.h"
#include "gameTypes/GameTypesOutput.h"
#include "gameTypes/InventorySetting.h"
#include "gameTypes/VisualSettings.h"
#include "gameData/MilitaryConsts.h"
#include "gameData/SettingTypeConv.h"
#include "gameData/ShieldConsts.h"
#include "rttr/test/random.hpp"
#include <boost/test/unit_test.hpp>
#include <iostream>

#if defined(PVS_STUDIO) || defined(__clang_analyzer__)
#    undef BOOST_TEST_REQUIRE
#    define BOOST_TEST_REQUIRE(expr)             \
        do                                       \
        {                                        \
            if(!(expr))                          \
                throw "Silence static analyzer"; \
        } while(false)
#endif

// LCOV_EXCL_START
static std::ostream& operator<<(std::ostream& out, const InventorySetting& setting)
{
    return out << static_cast<unsigned>(static_cast<uint8_t>(setting));
}
static std::ostream& operator<<(std::ostream& out, const NodalObjectType& e)
{
    return out << static_cast<unsigned>(rttr::enum_cast(e));
}
static void dummySuppressUnused(std::ostream& out)
{
    out << InventorySetting{} << NodalObjectType{};
}
// LCOV_EXCL_STOP

BOOST_AUTO_TEST_SUITE(GameCommandSuite)

BOOST_FIXTURE_TEST_CASE(PlaceFlagTest, WorldWithGCExecution2P)
{
    (void)&dummySuppressUnused;

    MapPoint flagPt = hqPos + MapPoint(4, 0);
    // Place flag for other player:
    curPlayer = 1;
    this->SetFlag(flagPt);
    // Wrong terrain
    BOOST_TEST_REQUIRE(!world.GetSpecObj<noRoadNode>(flagPt));

    curPlayer = 0;
    this->SetFlag(flagPt);
    BOOST_TEST_REQUIRE(world.GetNO(flagPt)->GetType() == NodalObjectType::Flag);
    auto* flag = world.GetSpecObj<noRoadNode>(flagPt);
    BOOST_TEST_REQUIRE(flag);
    BOOST_TEST_REQUIRE(flag->GetPos() == flagPt);
    BOOST_TEST_REQUIRE(flag->GetPlayer() == 0);
    // Flag blocked
    BOOST_TEST_REQUIRE(world.GetNeighbourNode(flagPt, Direction::West).bq == BuildingQuality::Nothing);
    BOOST_TEST_REQUIRE(world.GetNeighbourNode(flagPt, Direction::NorthEast).bq == BuildingQuality::Nothing);
    // This flag = house flag
    BOOST_TEST_REQUIRE(world.GetNeighbourNode(flagPt, Direction::NorthWest).bq == BuildingQuality::Castle);
    // Flag blocks castle
    BOOST_TEST_REQUIRE(world.GetNeighbourNode(flagPt, Direction::East).bq == BuildingQuality::House);
    BOOST_TEST_REQUIRE(world.GetNeighbourNode(flagPt, Direction::SouthEast).bq == BuildingQuality::House);
    BOOST_TEST_REQUIRE(world.GetNeighbourNode(flagPt, Direction::SouthWest).bq == BuildingQuality::House);
    // Place flag again
    this->SetFlag(flagPt);
    // Nothing should be changed
    BOOST_TEST_REQUIRE(world.GetSpecObj<noRoadNode>(flagPt) == flag);

    // Place flag at neighbour
    for(const MapPoint curPt : world.GetNeighbours(flagPt))
    {
        this->SetFlag(curPt);
        // Should not work
        BOOST_TEST_REQUIRE(!world.GetSpecObj<noRoadNode>(curPt));
    }

    unsigned objCt = GameObject::GetNumObjs();
    this->DestroyFlag(flagPt);
    // Removed from map
    BOOST_TEST_REQUIRE(world.GetNO(flagPt)->GetType() == NodalObjectType::Nothing);
    // Removed from game
    BOOST_TEST_REQUIRE(GameObject::GetNumObjs() == objCt - 1);
    // And everything clear now
    for(const MapPoint nb : world.GetNeighbours(flagPt))
        BOOST_TEST_REQUIRE(world.GetNode(nb).bq == BuildingQuality::Castle);
}

BOOST_FIXTURE_TEST_CASE(BuildRoadTest, WorldWithGCExecution2P)
{
    MapPoint flagPt = hqPos + MapPoint(-5, 4);
    // 2 flags outside range of HQ
    this->SetFlag(flagPt);
    this->SetFlag(flagPt + MapPoint(4, 0));
    // Build road with 3 segments:
    // a1) invalid start pt -> No road
    this->BuildRoad(flagPt + MapPoint(2, 0), false, std::vector<Direction>(4, Direction::East));
    for(unsigned i = 0; i < 6; i++)
        BOOST_TEST_REQUIRE(world.GetPointRoad(flagPt + MapPoint(i, 0), Direction::East) == PointRoad::None);
    // a2) invalid player
    curPlayer = 1;
    this->BuildRoad(flagPt, false, std::vector<Direction>(4, Direction::East));
    for(unsigned i = 0; i < 6; i++)
        BOOST_TEST_REQUIRE(world.GetPointRoad(flagPt + MapPoint(i, 0), Direction::East) == PointRoad::None);
    curPlayer = 0;

    // b) Flag->Flag ->OK
    this->BuildRoad(flagPt, false, std::vector<Direction>(4, Direction::East));
    for(unsigned i = 0; i < 4; i++)
    {
        BOOST_TEST_REQUIRE(world.GetPointRoad(flagPt + MapPoint(i, 0), Direction::East) == PointRoad::Normal);
        // Same but in opposite direction
        BOOST_TEST_REQUIRE(world.GetPointRoad(flagPt + MapPoint(i + 1, 0), Direction::West) == PointRoad::Normal);
    }
    // End of road
    BOOST_TEST_REQUIRE(world.GetPointRoad(flagPt + MapPoint(4, 0), Direction::East) == PointRoad::None);
    // BQ on road
    BOOST_TEST_REQUIRE(world.GetNode(flagPt + MapPoint(1, 0)).bq == BuildingQuality::Nothing);
    BOOST_TEST_REQUIRE(world.GetNode(flagPt + MapPoint(2, 0)).bq == BuildingQuality::Flag);
    BOOST_TEST_REQUIRE(world.GetNode(flagPt + MapPoint(3, 0)).bq == BuildingQuality::Nothing);
    BOOST_TEST_REQUIRE(world.GetNode(flagPt + MapPoint(4, 0)).bq == BuildingQuality::Nothing);
    // BQ above road
    BOOST_TEST_REQUIRE(world.GetNeighbourNode(flagPt + MapPoint(2, 0), Direction::NorthWest).bq
                       == BuildingQuality::Castle); // Flag could be build
    BOOST_TEST_REQUIRE(world.GetNeighbourNode(flagPt + MapPoint(2, 0), Direction::NorthEast).bq
                       == BuildingQuality::Flag); // only flag possible
    // BQ below road (Castle blocked by road)
    BOOST_TEST_REQUIRE(world.GetNeighbourNode(flagPt + MapPoint(2, 0), Direction::SouthEast).bq
                       == BuildingQuality::House);
    BOOST_TEST_REQUIRE(world.GetNeighbourNode(flagPt + MapPoint(2, 0), Direction::SouthWest).bq
                       == BuildingQuality::House);

    // Set another flag on the road
    // c) to close
    this->SetFlag(flagPt + MapPoint(1, 0));
    BOOST_TEST_REQUIRE(world.GetNO(flagPt + MapPoint(1, 0))->GetType() == NodalObjectType::Nothing);
    // d) middle -> ok
    this->SetFlag(flagPt + MapPoint(2, 0));
    BOOST_TEST_REQUIRE(world.GetNO(flagPt + MapPoint(2, 0))->GetType() == NodalObjectType::Flag); //-V807
    BOOST_TEST_REQUIRE(world.GetNeighbourNode(flagPt + MapPoint(2, 0), Direction::NorthWest).bq
                       == BuildingQuality::Castle); // Flag could be build
    BOOST_TEST_REQUIRE(world.GetNeighbourNode(flagPt + MapPoint(2, 0), Direction::NorthEast).bq
                       == BuildingQuality::Nothing); // no more flag possible
    // f) destroy middle flag -> Road destroyed
    this->DestroyFlag(flagPt + MapPoint(2, 0));
    for(unsigned i = 0; i < 4; i++)
        BOOST_TEST_REQUIRE(world.GetPointRoad(flagPt + MapPoint(i, 0), Direction::East) == PointRoad::None);
    this->DestroyFlag(flagPt + MapPoint(4, 0));

    // g) Road with no existing end flag -> Build road and place flag if possible
    // g1) Other flag to close
    this->SetFlag(flagPt + MapPoint(3, 0));
    this->BuildRoad(flagPt, false, std::vector<Direction>(2, Direction::East));
    BOOST_TEST_REQUIRE(world.GetNO(flagPt + MapPoint(2, 0))->GetType() == NodalObjectType::Nothing);
    for(unsigned i = 0; i < 3; i++)
        BOOST_TEST_REQUIRE(world.GetPointRoad(flagPt + MapPoint(i, 0), Direction::East) == PointRoad::None);
    this->DestroyFlag(flagPt + MapPoint(3, 0));
    // g2) Building to close
    this->SetBuildingSite(flagPt + MapPoint(3, 0), BuildingType::Farm);
    this->BuildRoad(flagPt, false, std::vector<Direction>(2, Direction::East));
    BOOST_TEST_REQUIRE(world.GetNO(flagPt + MapPoint(2, 0))->GetType() != NodalObjectType::Flag);
    for(unsigned i = 0; i < 2; i++)
        BOOST_TEST_REQUIRE(world.GetPointRoad(flagPt + MapPoint(i, 0), Direction::East) == PointRoad::None);
    this->DestroyFlag(world.GetNeighbour(flagPt + MapPoint(3, 0), Direction::SouthEast));
    // g3) Nothing objectionable
    this->BuildRoad(flagPt, false, std::vector<Direction>(2, Direction::East));
    BOOST_TEST_REQUIRE(world.GetNO(flagPt + MapPoint(2, 0))->GetType() == NodalObjectType::Flag);
    for(unsigned i = 0; i < 2; i++)
        BOOST_TEST_REQUIRE(world.GetPointRoad(flagPt + MapPoint(i, 0), Direction::East) == PointRoad::Normal);

    // h) Non-blocking env. object
    world.SetNO(flagPt + MapPoint(3, 0), new noEnvObject(flagPt, 512));
    BOOST_TEST_REQUIRE(world.GetNO(flagPt + MapPoint(3, 0))->GetType() == NodalObjectType::Environment);
    this->BuildRoad(flagPt + MapPoint(2, 0), false, std::vector<Direction>(2, Direction::East));
    for(unsigned i = 2; i < 4; i++)
        BOOST_TEST_REQUIRE(world.GetPointRoad(flagPt + MapPoint(i, 0), Direction::East) == PointRoad::Normal);
    BOOST_TEST_REQUIRE(world.GetNO(flagPt + MapPoint(3, 0))->GetType() == NodalObjectType::Nothing);

    // Remove other flags
    this->DestroyFlag(flagPt + MapPoint(2, 0));

    // i) outside player territory
    // i1) border
    this->BuildRoad(flagPt, false, std::vector<Direction>(HQ_RADIUS - (flagPt.x - hqPos.x), Direction::East));
    for(unsigned i = 0; i <= HQ_RADIUS; i++)
        BOOST_TEST_REQUIRE(world.GetPointRoad(flagPt + MapPoint(i, 0), Direction::East) == PointRoad::None);
    // i2) territory
    this->BuildRoad(flagPt, false, std::vector<Direction>(HQ_RADIUS - (flagPt.x - hqPos.x) + 1, Direction::East));
    for(unsigned i = 0; i <= HQ_RADIUS; i++)
        BOOST_TEST_REQUIRE(world.GetPointRoad(flagPt + MapPoint(i, 0), Direction::East) == PointRoad::None);
}

BOOST_FIXTURE_TEST_CASE(DestroyRoadTest, WorldWithGCExecution2P)
{
    MapPoint flagPt = hqPos + MapPoint(4, 0);
    MapPoint flagPt2 = flagPt + MapPoint(2, 0);
    this->SetFlag(flagPt);
    // Build road with 3 segments:
    this->BuildRoad(flagPt, false, std::vector<Direction>(2, Direction::East));
    this->BuildRoad(flagPt2, false, std::vector<Direction>(2, Direction::East));
    for(unsigned i = 0; i < 4; i++)
        BOOST_TEST_REQUIRE(world.GetPointRoad(flagPt + MapPoint(i, 0), Direction::East) == PointRoad::Normal);
    // Destroy from middle of road -> fail
    this->DestroyRoad(flagPt2 + MapPoint(1, 0), Direction::East);
    for(unsigned i = 0; i < 4; i++)
        BOOST_TEST_REQUIRE(world.GetPointRoad(flagPt + MapPoint(i, 0), Direction::East) == PointRoad::Normal);
    // Wrong player -> Fail
    curPlayer = 1;
    this->DestroyRoad(flagPt2, Direction::East);
    for(unsigned i = 0; i < 4; i++)
        BOOST_TEST_REQUIRE(world.GetPointRoad(flagPt + MapPoint(i, 0), Direction::East) == PointRoad::Normal);
    // Wrong direction -> Fail
    curPlayer = 0;
    this->DestroyRoad(flagPt2, Direction::SouthEast);
    for(unsigned i = 0; i < 4; i++)
        BOOST_TEST_REQUIRE(world.GetPointRoad(flagPt + MapPoint(i, 0), Direction::East) == PointRoad::Normal);
    // Correct -> Succeed
    this->DestroyRoad(flagPt2, Direction::East);
    for(unsigned i = 0; i < 2; i++)
    {
        BOOST_TEST_REQUIRE(world.GetPointRoad(flagPt + MapPoint(i, 0), Direction::East) == PointRoad::Normal);
        BOOST_TEST_REQUIRE(world.GetPointRoad(flagPt + MapPoint(i + 2, 0), Direction::East) == PointRoad::None);
    }
    // Flags must still exist
    BOOST_TEST_REQUIRE(world.GetSpecObj<noFlag>(flagPt));
    BOOST_TEST_REQUIRE(world.GetSpecObj<noFlag>(flagPt2));
    BOOST_TEST_REQUIRE(world.GetSpecObj<noFlag>(flagPt2 + MapPoint(2, 0)));
    // Rebuild
    this->BuildRoad(flagPt2, false, std::vector<Direction>(2, Direction::East));
    for(unsigned i = 0; i < 4; i++)
        BOOST_TEST_REQUIRE(world.GetPointRoad(flagPt + MapPoint(i, 0), Direction::East) == PointRoad::Normal);
    // Opposite dir
    this->DestroyRoad(flagPt2, Direction::West);
    for(unsigned i = 0; i < 2; i++)
    {
        BOOST_TEST_REQUIRE(world.GetPointRoad(flagPt + MapPoint(i, 0), Direction::East) == PointRoad::None);
        BOOST_TEST_REQUIRE(world.GetPointRoad(flagPt + MapPoint(i + 2, 0), Direction::East) == PointRoad::Normal);
    }
    // Both
    this->DestroyRoad(flagPt2, Direction::East);
    for(unsigned i = 0; i < 4; i++)
        BOOST_TEST_REQUIRE(world.GetPointRoad(flagPt + MapPoint(i, 0), Direction::East) == PointRoad::None);
    // Flags must still exist
    BOOST_TEST_REQUIRE(world.GetSpecObj<noFlag>(flagPt));
    BOOST_TEST_REQUIRE(world.GetSpecObj<noFlag>(flagPt2));
    BOOST_TEST_REQUIRE(world.GetSpecObj<noFlag>(flagPt2 + MapPoint(2, 0)));
}

BOOST_FIXTURE_TEST_CASE(UpgradeRoadTest, WorldWithGCExecution2P)
{
    // 3 Roads, so we have a middle flag, and an additional road
    const MapPoint hqFlagPos = world.GetNeighbour(hqPos, Direction::SouthEast);
    this->BuildRoad(hqFlagPos, false, std::vector<Direction>(2, Direction::East));
    this->BuildRoad(hqFlagPos + MapPoint(2, 0), false, std::vector<Direction>(2, Direction::East));
    this->BuildRoad(hqFlagPos + MapPoint(4, 0), false, std::vector<Direction>(2, Direction::East));
    const MapPoint middleFlag = hqFlagPos + MapPoint(2, 0);
    const noFlag* hqFlag = world.GetSpecObj<noFlag>(hqFlagPos);
    const noFlag* flag = world.GetSpecObj<noFlag>(middleFlag);
    const noFlag* flag2 = world.GetSpecObj<noFlag>(middleFlag + MapPoint(2, 0));
    BOOST_TEST_REQUIRE(hqFlag);
    BOOST_TEST_REQUIRE(flag);
    BOOST_TEST_REQUIRE(flag2);
    BOOST_TEST_REQUIRE(flag->GetFlagType() == FlagType::Normal);
    // No flag
    this->UpgradeRoad(middleFlag - MapPoint(1, 0), Direction::East);
    BOOST_TEST_REQUIRE(flag->GetFlagType() == FlagType::Normal);
    // Wrong direction
    this->UpgradeRoad(middleFlag, Direction::SouthEast);
    BOOST_TEST_REQUIRE(flag->GetFlagType() == FlagType::Normal);
    // Upgrading correctly but twice (2nd does nothing)
    for(unsigned i = 0; i < 2; i++)
    {
        // Correct
        this->UpgradeRoad(middleFlag, Direction::East);
        BOOST_TEST(flag->GetFlagType() == FlagType::Large);
        BOOST_TEST(flag2->GetFlagType() == FlagType::Large);
        BOOST_TEST(hqFlag->GetFlagType() == FlagType::Normal);
        BOOST_TEST(world.GetPointRoad(middleFlag, Direction::East) == PointRoad::Donkey);
        BOOST_TEST(world.GetPointRoad(middleFlag + MapPoint(1, 0), Direction::East) == PointRoad::Donkey);
        BOOST_TEST(world.GetPointRoad(middleFlag + MapPoint(2, 0), Direction::East) == PointRoad::Normal);
        BOOST_TEST(world.GetPointRoad(middleFlag, Direction::West) == PointRoad::Normal);
    }
    // Upgrade in other direction
    this->UpgradeRoad(middleFlag, Direction::West);
    BOOST_TEST(flag->GetFlagType() == FlagType::Large);
    BOOST_TEST(flag2->GetFlagType() == FlagType::Large);
    BOOST_TEST(hqFlag->GetFlagType() == FlagType::Large);
    BOOST_TEST(world.GetPointRoad(middleFlag, Direction::East) == PointRoad::Donkey);
    BOOST_TEST(world.GetPointRoad(middleFlag + MapPoint(1, 0), Direction::East) == PointRoad::Donkey);
    BOOST_TEST(world.GetPointRoad(middleFlag + MapPoint(2, 0), Direction::East) == PointRoad::Normal);
    BOOST_TEST(world.GetPointRoad(middleFlag, Direction::West) == PointRoad::Donkey);
}

BOOST_FIXTURE_TEST_CASE(PlayerEconomySettings, WorldWithGCExecution2P)
{
    // Execute for both players to make sure they don't influence each other
    for(; curPlayer < 2; curPlayer++)
    {
        Distributions inDist;
        for(uint8_t& i : inDist)
            i = rand();
        this->ChangeDistribution(inDist);

        bool orderType = rand() % 2 == 0;
        BuildOrders inBuildOrder = GamePlayer::GetStandardBuildOrder();
        std::shuffle(inBuildOrder.begin(), inBuildOrder.end(), rttr::test::getRandState());
        this->ChangeBuildOrder(orderType, inBuildOrder);

        TransportOrders inTransportOrder;
        for(unsigned i = 0; i < inTransportOrder.size(); i++)
            inTransportOrder[i] = i;
        std::shuffle(inTransportOrder.begin(), inTransportOrder.end(), rttr::test::getRandState());
        this->ChangeTransport(inTransportOrder);

        MilitarySettings militarySettings;
        for(unsigned i = 0; i < militarySettings.size(); ++i)
            militarySettings[i] = rand() % (MILITARY_SETTINGS_SCALE[i] + 1);
        this->ChangeMilitary(militarySettings);

        ToolSettings toolPrios;
        for(uint8_t& toolPrio : toolPrios)
            toolPrio = rand() % 11;
        this->ChangeTools(toolPrios);

        const GamePlayer& player = world.GetPlayer(curPlayer);
        // TODO: Use better getters once available
        VisualSettings outSettings;
        player.FillVisualSettings(outSettings);
        for(unsigned i = 0; i < inDist.size(); i++)
            BOOST_TEST_REQUIRE(outSettings.distribution[i] == inDist[i]);
        BOOST_TEST_REQUIRE(outSettings.useCustomBuildOrder == orderType);
        for(unsigned i = 0; i < inBuildOrder.size(); i++)
            BOOST_TEST_REQUIRE(outSettings.build_order[i] == inBuildOrder[i]);
        for(unsigned i = 0; i < inTransportOrder.size(); i++)
            BOOST_TEST_REQUIRE(outSettings.transport_order[i] == inTransportOrder[i]);
        for(const auto i : helpers::enumRange<GoodType>())
            BOOST_TEST_REQUIRE(player.GetTransportPriority(i) == GetTransportPrioFromOrdering(inTransportOrder, i));
        for(unsigned i = 0; i < militarySettings.size(); i++)
            BOOST_TEST_REQUIRE(player.GetMilitarySetting(i) == militarySettings[i]);
        for(const auto i : helpers::enumRange<Tool>())
            BOOST_TEST_REQUIRE(player.GetToolPriority(i) == toolPrios[i]);
    }
}

BOOST_FIXTURE_TEST_CASE(BuildBuilding, WorldWithGCExecution2P)
{
    initGameRNG();

    const MapPoint closePt = hqPos + MapPoint(2, 0);
    const MapPoint farmPt = hqPos + MapPoint(6, 0);
    const MapPoint closeMilPt = hqPos - MapPoint(4, 0);
    const MapPoint okMilPt = hqPos - MapPoint(5, 0);
    // Wrong BQ (blocked by HQ)
    this->SetBuildingSite(closePt, BuildingType::Farm);
    BOOST_TEST_REQUIRE(world.GetNO(closePt)->GetType() == NodalObjectType::Nothing); //-V807
    // OK
    this->SetBuildingSite(closePt, BuildingType::Woodcutter);
    BOOST_TEST_REQUIRE(world.GetNO(closePt)->GetType() == NodalObjectType::Buildingsite);
    BOOST_TEST_REQUIRE(world.GetSpecObj<noBaseBuilding>(closePt)->GetBuildingType() == BuildingType::Woodcutter);
    BOOST_TEST_REQUIRE(world.GetNO(world.GetNeighbour(closePt, Direction::SouthEast))->GetType()
                       == NodalObjectType::Flag);
    // OK
    this->SetBuildingSite(farmPt, BuildingType::Farm);
    BOOST_TEST_REQUIRE(world.GetNO(farmPt)->GetType() == NodalObjectType::Buildingsite);
    BOOST_TEST_REQUIRE(world.GetSpecObj<noBaseBuilding>(farmPt)->GetBuildingType() == BuildingType::Farm);
    BOOST_TEST_REQUIRE(world.GetNO(world.GetNeighbour(farmPt, Direction::SouthEast))->GetType()
                       == NodalObjectType::Flag);
    // Millitary bld to close
    this->SetBuildingSite(closeMilPt, BuildingType::Barracks);
    BOOST_TEST_REQUIRE(world.GetNO(closeMilPt)->GetType() == NodalObjectType::Nothing);
    // Millitary bld ok
    this->SetBuildingSite(okMilPt, BuildingType::Barracks);
    BOOST_TEST_REQUIRE(world.GetNO(okMilPt)->GetType() == NodalObjectType::Buildingsite);
    BOOST_TEST_REQUIRE(world.GetSpecObj<noBaseBuilding>(okMilPt)->GetBuildingType() == BuildingType::Barracks);
    BOOST_TEST_REQUIRE(world.GetNO(world.GetNeighbour(okMilPt, Direction::SouthEast))->GetType()
                       == NodalObjectType::Flag);

    // Remove bld
    this->DestroyBuilding(farmPt);
    BOOST_TEST_REQUIRE(world.GetNO(farmPt)->GetType() == NodalObjectType::Nothing);
    BOOST_TEST_REQUIRE(world.GetNO(world.GetNeighbour(farmPt, Direction::SouthEast))->GetType()
                       == NodalObjectType::Flag);

    // Remove flag -> bld also destroyed
    this->DestroyFlag(world.GetNeighbour(okMilPt, Direction::SouthEast));
    BOOST_TEST_REQUIRE(world.GetNO(okMilPt)->GetType() == NodalObjectType::Nothing);
    BOOST_TEST_REQUIRE(world.GetNO(world.GetNeighbour(okMilPt, Direction::SouthEast))->GetType()
                       == NodalObjectType::Nothing);

    // Check if bld is build
    this->BuildRoad(world.GetNeighbour(hqPos, Direction::SouthEast), false, std::vector<Direction>(2, Direction::East));
    RTTR_EXEC_TILL(1200, world.GetNO(closePt)->GetType() == NodalObjectType::Building);
    BOOST_TEST_REQUIRE(world.GetNO(closePt)->GetGOT() == GO_Type::NobUsual);
    BOOST_TEST_REQUIRE(world.GetSpecObj<noBaseBuilding>(closePt)->GetBuildingType() == BuildingType::Woodcutter);

    // Destroy finished bld -> Fire
    this->DestroyBuilding(closePt);
    BOOST_TEST_REQUIRE(world.GetNO(closePt)->GetType() == NodalObjectType::Fire);
    // Destroy twice -> Nothing happens
    this->DestroyBuilding(closePt);
    BOOST_TEST_REQUIRE(world.GetNO(closePt)->GetType() == NodalObjectType::Fire);
    // Try to build on fire
    this->SetBuildingSite(closePt, BuildingType::Woodcutter);
    BOOST_TEST_REQUIRE(world.GetNO(closePt)->GetType() == NodalObjectType::Fire);
}

BOOST_FIXTURE_TEST_CASE(SendSoldiersHomeTest, WorldWithGCExecution2P)
{
    initGameRNG();

    const MapPoint milPt = hqPos + MapPoint(4, 0);
    // Setup: Give player 3 generals
    GamePlayer& player = world.GetPlayer(curPlayer);
    nobBaseWarehouse* wh = player.GetFirstWH();
    BOOST_TEST_REQUIRE(wh);
    BOOST_TEST_REQUIRE(wh->GetInventory().people[Job::General] == 0u); //-V522
    Inventory goods;
    goods.Add(Job::PrivateFirstClass, 1);
    goods.Add(Job::Sergeant, 1);
    goods.Add(Job::Officer, 1);
    goods.Add(Job::General, 2);
    wh->AddGoods(goods, true);
    // Don't keep any reserve
    for(unsigned i = 0; i <= this->ggs.GetMaxMilitaryRank(); ++i)
        this->ChangeReserve(hqPos, i, 0);
    // Set all military stuff to max
    this->ChangeMilitary(MILITARY_SETTINGS_SCALE);
    // Build a watchtower and connect it
    auto* bld = dynamic_cast<nobMilitary*>(
      BuildingFactory::CreateBuilding(world, BuildingType::Watchtower, milPt, curPlayer, player.nation));
    BOOST_TEST_REQUIRE(bld);
    this->BuildRoad(world.GetNeighbour(hqPos, Direction::SouthEast), false,
                    std::vector<Direction>((milPt.x - hqPos.x), Direction::East));
    // Now run some GFs so the bld is occupied (<=30GFs/per Soldier for leaving HQ, 20GFs per node walked (distance + to
    // and from flag), 30GFs for leaving carrier)
    unsigned numGFtillAllArrive = 30 * 6 + 20 * (milPt.x - hqPos.x + 2) + 30;
    RTTR_SKIP_GFS(numGFtillAllArrive);
    // Now we should have 1 each of ranks 0-3 and 2 rank 4s
    BOOST_TEST_REQUIRE(bld->GetNumTroops() == 6u); //-V522
    auto itTroops = bld->GetTroops().begin();
    for(unsigned i = 0; i < 4; i++, ++itTroops)
        BOOST_TEST_REQUIRE(itTroops->GetRank() == i);
    for(unsigned i = 0; i < 2; i++, ++itTroops)
        BOOST_TEST_REQUIRE(itTroops->GetRank() == 4);
    // End preparation

    unsigned expectedTroopCt = 6u;
    // send each of the higher ranks home
    for(unsigned curRank = 4; curRank > 0; --curRank)
    {
        this->SendSoldiersHome(milPt);
        expectedTroopCt -= (curRank == 4) ? 2 : 1; // 2 generals, 1 of the others
        BOOST_TEST_REQUIRE(bld->GetNumTroops() == expectedTroopCt);
        itTroops = bld->GetTroops().begin();
        for(unsigned i = 0; i < expectedTroopCt; i++, ++itTroops)
            BOOST_TEST_REQUIRE(itTroops->GetRank() == i);
    }
    // One low rank is left
    BOOST_TEST_REQUIRE(bld->GetNumTroops() == 1u);
    this->SendSoldiersHome(milPt);
    // But he must stay
    BOOST_TEST_REQUIRE(bld->GetNumTroops() == 1u);

    // Wait till new soldiers have arrived
    RTTR_SKIP_GFS(numGFtillAllArrive);

    // 6 low ranks
    BOOST_TEST_REQUIRE(bld->GetNumTroops() == 6u);
    itTroops = bld->GetTroops().begin();
    for(unsigned i = 0; i < 6; i++, ++itTroops)
        BOOST_TEST_REQUIRE(itTroops->GetRank() == 0u);

    // Send 5 of them home
    this->SendSoldiersHome(milPt);
    BOOST_TEST_REQUIRE(bld->GetNumTroops() == 1u);

    // Wait till one left so new ones get ordered
    RTTR_SKIP_GFS(40);

    // All higher rank soldiers should have been ordered and hence removed from the real inventory
    for(unsigned i = 1; i < SOLDIER_JOBS.size(); i++)
        BOOST_TEST_REQUIRE(wh->GetNumRealFigures(SOLDIER_JOBS[i]) == 0u);

    // Allow one of them to leave the HQ
    RTTR_SKIP_GFS(40);

    // Now cancel orders for generals and replace with low rank ones
    this->OrderNewSoldiers(milPt);

    // Wait till new soldiers have arrived
    RTTR_SKIP_GFS(numGFtillAllArrive);

    // 3 low ranks and 1 each of other ranks except general
    BOOST_TEST_REQUIRE(bld->GetNumTroops() == 6u);
    itTroops = bld->GetTroops().begin();
    for(unsigned i = 0; i < 3; i++, ++itTroops)
        BOOST_TEST_REQUIRE(itTroops->GetRank() == 0u);
    for(unsigned i = 1; i < 3; i++, ++itTroops)
        BOOST_TEST_REQUIRE(itTroops->GetRank() == i);
}

BOOST_FIXTURE_TEST_CASE(OrderNewSoldiersFailOnMinRank, WorldWithGCExecution2P)
{
    initGameRNG();

    const MapPoint milPt = hqPos + MapPoint(4, 0);
    GamePlayer& player = world.GetPlayer(curPlayer);
    nobBaseWarehouse* wh = player.GetFirstWH();
    BOOST_TEST_REQUIRE(wh);
    // Set all military stuff to max
    this->ChangeMilitary(MILITARY_SETTINGS_SCALE);
    ggs.setSelection(AddonId::MAX_RANK, MAX_MILITARY_RANK);
    // Build a watchtower and connect it
    auto* bld = static_cast<nobMilitary*>(
      BuildingFactory::CreateBuilding(world, BuildingType::Barracks, milPt, curPlayer, player.nation));
    this->BuildRoad(world.GetNeighbour(hqPos, Direction::SouthEast), false,
                    std::vector<Direction>((milPt.x - hqPos.x), Direction::East));
    auto* hq = world.GetSpecObj<nobBaseWarehouse>(hqPos);
    const nofPassiveSoldier* soldier = nullptr;
    for(const noFigure& fig : hq->GetLeavingFigures())
    {
        soldier = dynamic_cast<const nofPassiveSoldier*>(&fig);
        if(soldier)
            break;
    }
    BOOST_TEST_REQUIRE(soldier);
    // Let soldiers out and walk a bit
    MapPoint sldTestPos = world.GetNeighbour(world.GetNeighbour(hqPos, Direction::SouthEast), Direction::East);
    RTTR_EXEC_TILL(30 * 2 + 20 * 2 + 10, soldier->GetPos() == sldTestPos); //-V522
    BOOST_TEST_REQUIRE(soldier->GetGoal() == bld);
    this->OrderNewSoldiers(milPt);
    // Soldier must still have this goal!
    BOOST_TEST_REQUIRE(soldier->GetGoal() == bld);
    // Now run some GFs so the bld is occupied (20GFs per node walked (distance + to and from flag))
    unsigned numGFtillAllArrive = 20 * (milPt.x - hqPos.x + 2);
    RTTR_EXEC_TILL(numGFtillAllArrive, bld->GetNumTroops() == 2u);
    this->OrderNewSoldiers(milPt);
    // No one leaves!
    BOOST_TEST_REQUIRE(bld->GetNumTroops() == 2u);
}

namespace {
void FlagWorkerTest(WorldWithGCExecution2P& worldFixture, Job workerJob, GoodType toolType)
{
    const MapPoint flagPt = worldFixture.world.GetNeighbour(worldFixture.hqPos, Direction::SouthEast) + MapPoint(3, 0);
    GamePlayer& player = worldFixture.world.GetPlayer(worldFixture.curPlayer);
    nobBaseWarehouse* wh = player.GetFirstWH();
    BOOST_TEST_REQUIRE(wh);

    const unsigned startFigureCt = wh->GetNumRealFigures(workerJob); //-V522
    const unsigned startToolsCt = wh->GetNumRealWares(toolType);
    // We need some of them!
    BOOST_TEST_REQUIRE(startFigureCt > 0u);
    BOOST_TEST_REQUIRE(startToolsCt > 0u);

    // No flag -> Nothing happens
    worldFixture.CallSpecialist(flagPt, workerJob);
    BOOST_TEST_REQUIRE(wh->GetNumRealFigures(workerJob) == startFigureCt);
    BOOST_TEST_REQUIRE(wh->GetLeavingFigures().empty()); //-V807

    worldFixture.SetFlag(flagPt);
    // Unconnected flag -> Nothing happens
    worldFixture.CallSpecialist(flagPt, workerJob);
    BOOST_TEST_REQUIRE(wh->GetNumRealFigures(workerJob) == startFigureCt);
    BOOST_TEST_REQUIRE(wh->GetLeavingFigures().empty());

    // Build road and let worker leave
    worldFixture.BuildRoad(flagPt, false, std::vector<Direction>(3, Direction::West));
    for(unsigned i = 0; i < 30; i++)
        worldFixture.em.ExecuteNextGF();
    BOOST_TEST_REQUIRE(wh->GetLeavingFigures().empty());

    // Call one geologist to flag
    worldFixture.CallSpecialist(flagPt, workerJob);
    BOOST_TEST_REQUIRE(wh->GetNumRealFigures(workerJob) + 1 == startFigureCt);
    BOOST_TEST_REQUIRE(wh->GetLeavingFigures().size() == 1u);
    BOOST_TEST_REQUIRE(wh->GetLeavingFigures().front().GetJobType() == workerJob);

    // Call remaining ones
    for(unsigned i = 1; i < startFigureCt; i++)
        worldFixture.CallSpecialist(flagPt, workerJob);
    BOOST_TEST_REQUIRE(wh->GetNumRealFigures(workerJob) == 0u);
    BOOST_TEST_REQUIRE(wh->GetLeavingFigures().size() == startFigureCt);

    // Recruit all possible ones
    BOOST_TEST_REQUIRE(wh->GetNumRealWares(toolType) == startToolsCt);
    for(unsigned i = 0; i < startToolsCt; i++)
        worldFixture.CallSpecialist(flagPt, workerJob);
    BOOST_TEST_REQUIRE(wh->GetNumRealFigures(workerJob) == 0u);
    BOOST_TEST_REQUIRE(wh->GetLeavingFigures().size() == startFigureCt + startToolsCt);

    // And an extra one -> Fail
    worldFixture.CallSpecialist(flagPt, workerJob);
    BOOST_TEST_REQUIRE(wh->GetNumRealFigures(workerJob) == 0u);
    BOOST_TEST_REQUIRE(wh->GetLeavingFigures().size() == startFigureCt + startToolsCt);
}
} // namespace

BOOST_FIXTURE_TEST_CASE(CallGeologist, WorldWithGCExecution2P)
{
    initGameRNG();

    FlagWorkerTest(*this, Job::Geologist, GoodType::Hammer);
}

BOOST_FIXTURE_TEST_CASE(CallScout, WorldWithGCExecution2P)
{
    initGameRNG();

    FlagWorkerTest(*this, Job::Scout, GoodType::Bow);
}

BOOST_FIXTURE_TEST_CASE(ChangeCoinAccept, WorldWithGCExecution2P)
{
    const MapPoint bldPt = hqPos + MapPoint(3, 0);
    auto* bld = dynamic_cast<nobMilitary*>(
      BuildingFactory::CreateBuilding(world, BuildingType::Watchtower, bldPt, curPlayer, Nation::Romans));
    BOOST_TEST_REQUIRE(bld);
    BOOST_TEST_REQUIRE(!bld->IsGoldDisabled()); //-V522

    // Enable (already is)
    this->SetCoinsAllowed(bldPt, true);
    BOOST_TEST_REQUIRE(!bld->IsGoldDisabled());

    // Disable
    this->SetCoinsAllowed(bldPt, false);
    BOOST_TEST_REQUIRE(bld->IsGoldDisabled());

    // Reenable
    this->SetCoinsAllowed(bldPt, true);
    BOOST_TEST_REQUIRE(!bld->IsGoldDisabled());

    // Production should have no effect
    this->SetProductionEnabled(bldPt, true);
    BOOST_TEST_REQUIRE(!bld->IsGoldDisabled());
    this->SetProductionEnabled(bldPt, false);
    BOOST_TEST_REQUIRE(!bld->IsGoldDisabled());
}

BOOST_FIXTURE_TEST_CASE(DisableProduction, WorldWithGCExecution2P)
{
    const MapPoint bldPt = hqPos + MapPoint(3, 0);
    auto* bld = dynamic_cast<nobUsual*>(
      BuildingFactory::CreateBuilding(world, BuildingType::Forester, bldPt, curPlayer, Nation::Romans));
    BOOST_TEST_REQUIRE(bld);
    BOOST_TEST_REQUIRE(!bld->IsProductionDisabled()); //-V522

    // Enable (already is)
    this->SetProductionEnabled(bldPt, true);
    BOOST_TEST_REQUIRE(!bld->IsProductionDisabled());

    // Disable
    this->SetProductionEnabled(bldPt, false);
    BOOST_TEST_REQUIRE(bld->IsProductionDisabled());

    // Reenable
    this->SetProductionEnabled(bldPt, true);
    BOOST_TEST_REQUIRE(!bld->IsProductionDisabled());

    // Coins should have no effect
    this->SetCoinsAllowed(bldPt, true);
    BOOST_TEST_REQUIRE(!bld->IsProductionDisabled());
    this->SetCoinsAllowed(bldPt, false);
    BOOST_TEST_REQUIRE(!bld->IsProductionDisabled());
}

namespace {
void InitPactsAndPost(GameWorldBase& world)
{
    for(unsigned i = 0; i < world.GetNumPlayers(); i++)
    {
        world.GetPlayer(i).MakeStartPacts();
        PostBox& box = *world.GetPostMgr().GetPostBox(i);
        const unsigned numMsgs = box.GetNumMsgs();
        for(unsigned j = 0; j < numMsgs; j++)
            BOOST_TEST_REQUIRE(box.DeleteMsg(0u));
        BOOST_TEST_REQUIRE(box.GetNumMsgs() == 0u);
    }
}
} // namespace

BOOST_FIXTURE_TEST_CASE(NotifyAllies, WorldWithGCExecution3P)
{
    // At first there are no teams
    for(unsigned i = 0; i < world.GetNumPlayers(); i++)
        BOOST_TEST_REQUIRE(world.GetPlayer(i).team == Team::None);
    PostManager& postMgr = world.GetPostMgr();
    // Add postbox for each player
    for(unsigned i = 0; i < world.GetNumPlayers(); i++)
        postMgr.AddPostBox(i);
    // Choose middle player so we can observe side effects and off-by-one errors
    curPlayer = 1;

    // No ally -> no messages
    this->NotifyAlliesOfLocation(hqPos);
    for(unsigned i = 0; i < world.GetNumPlayers(); i++)
        BOOST_TEST_REQUIRE(postMgr.GetPostBox(i)->GetNumMsgs() == 0u); //-V807
    // Still no allies
    world.GetPlayer(1).team = Team::Team1; //-V807
    InitPactsAndPost(world);
    this->NotifyAlliesOfLocation(hqPos);
    for(unsigned i = 0; i < world.GetNumPlayers(); i++)
        BOOST_TEST_REQUIRE(postMgr.GetPostBox(i)->GetNumMsgs() == 0u);

    // First 2 players are allied -> Message received by player 0 only
    world.GetPlayer(0).team = Team::Team1; //-V807
    world.GetPlayer(1).team = Team::Team1;
    InitPactsAndPost(world);
    this->NotifyAlliesOfLocation(hqPos);
    BOOST_TEST_REQUIRE(postMgr.GetPostBox(0u)->GetNumMsgs() == 1u); //-V807
    for(unsigned i = 1; i < world.GetNumPlayers(); i++)
        BOOST_TEST_REQUIRE(postMgr.GetPostBox(i)->GetNumMsgs() == 0u);

    // Same if player 2 is in another team
    world.GetPlayer(0).team = Team::Team1; //-V525
    world.GetPlayer(1).team = Team::Team1;
    world.GetPlayer(2).team = Team::Team2; //-V807
    InitPactsAndPost(world);
    this->NotifyAlliesOfLocation(hqPos);
    BOOST_TEST_REQUIRE(postMgr.GetPostBox(0u)->GetNumMsgs() == 1u);
    for(unsigned i = 1; i < world.GetNumPlayers(); i++)
        BOOST_TEST_REQUIRE(postMgr.GetPostBox(i)->GetNumMsgs() == 0u);

    // player 2 is in same team
    world.GetPlayer(0).team = Team::Team1; //-V525
    world.GetPlayer(1).team = Team::Team2;
    world.GetPlayer(2).team = Team::Team2;
    InitPactsAndPost(world);
    this->NotifyAlliesOfLocation(hqPos);
    BOOST_TEST_REQUIRE(postMgr.GetPostBox(2u)->GetNumMsgs() == 1u);
    for(unsigned i = 0; i < 2; i++)
        BOOST_TEST_REQUIRE(postMgr.GetPostBox(i)->GetNumMsgs() == 0u);

    // All are in same team
    world.GetPlayer(0).team = Team::Team3;
    world.GetPlayer(1).team = Team::Team3;
    world.GetPlayer(2).team = Team::Team3;
    InitPactsAndPost(world);
    this->NotifyAlliesOfLocation(hqPos);
    BOOST_TEST_REQUIRE(postMgr.GetPostBox(0u)->GetNumMsgs() == 1u);
    BOOST_TEST_REQUIRE(postMgr.GetPostBox(1u)->GetNumMsgs() == 0u);
    BOOST_TEST_REQUIRE(postMgr.GetPostBox(2u)->GetNumMsgs() == 1u);
}

BOOST_AUTO_TEST_CASE(InventorySettingType)
{
    InventorySetting setting;
    // Default setting is 0
    BOOST_TEST_REQUIRE(static_cast<uint8_t>(setting) == 0u);

    // Test all 3 single types
    setting = EInventorySetting::Stop;
    BOOST_TEST_REQUIRE(setting.IsSet(EInventorySetting::Stop));
    BOOST_TEST(!setting.IsSet(EInventorySetting::Send));
    BOOST_TEST(!setting.IsSet(EInventorySetting::Collect));

    setting = EInventorySetting::Send;
    BOOST_TEST(!setting.IsSet(EInventorySetting::Stop));
    BOOST_TEST_REQUIRE(setting.IsSet(EInventorySetting::Send));
    BOOST_TEST(!setting.IsSet(EInventorySetting::Collect));

    setting = EInventorySetting::Collect;
    BOOST_TEST(!setting.IsSet(EInventorySetting::Stop));
    BOOST_TEST(!setting.IsSet(EInventorySetting::Send));
    BOOST_TEST_REQUIRE(setting.IsSet(EInventorySetting::Collect));

    // Reset and test toggle
    setting = InventorySetting();
    setting.Toggle(EInventorySetting::Stop);
    BOOST_TEST_REQUIRE(setting == EInventorySetting::Stop);
    setting.Toggle(EInventorySetting::Send);
    // Both set
    BOOST_TEST_REQUIRE(setting.IsSet(EInventorySetting::Stop));
    BOOST_TEST_REQUIRE(setting.IsSet(EInventorySetting::Send));
    BOOST_TEST_REQUIRE(!setting.IsSet(EInventorySetting::Collect));

    // Resets others
    setting.Toggle(EInventorySetting::Collect);
    BOOST_TEST_REQUIRE(setting == EInventorySetting::Collect);
    // Resets collect
    setting.Toggle(EInventorySetting::Stop);
    BOOST_TEST_REQUIRE(setting == EInventorySetting::Stop);

    // Enable send, disable stop
    setting.Toggle(EInventorySetting::Send);
    setting.Toggle(EInventorySetting::Stop);
    BOOST_TEST_REQUIRE(setting == EInventorySetting::Send);
}

template<class T, class U>
auto makeVector(const helpers::EnumArray<U, T>& srcArray)
{
    std::vector<U> result(srcArray.size());
    std::copy(srcArray.begin(), srcArray.end(), result.begin());
    return result;
}

BOOST_FIXTURE_TEST_CASE(SetInventorySettingTest, WorldWithGCExecution2P)
{
    GamePlayer& player = world.GetPlayer(curPlayer);
    const nobBaseWarehouse* wh = player.GetFirstWH();
    BOOST_TEST_REQUIRE(wh);
    InventorySetting expectedSetting;
    BOOST_TEST_REQUIRE(wh->GetInventorySetting(GoodType::Boards) == expectedSetting); //-V522
    BOOST_TEST_REQUIRE(wh->GetInventorySetting(Job::Private) == expectedSetting);
    expectedSetting.Toggle(EInventorySetting::Stop);
    expectedSetting.Toggle(EInventorySetting::Send);

    this->SetInventorySetting(hqPos, GoodType::Boards, expectedSetting);
    BOOST_TEST_REQUIRE(wh->GetInventorySetting(GoodType::Boards) == expectedSetting);
    this->SetInventorySetting(hqPos, Job::Private, expectedSetting);
    BOOST_TEST_REQUIRE(wh->GetInventorySetting(Job::Private) == expectedSetting);

    expectedSetting.Toggle(EInventorySetting::Collect);
    this->SetInventorySetting(hqPos, GoodType::Boards, expectedSetting);
    BOOST_TEST_REQUIRE(wh->GetInventorySetting(GoodType::Boards) == expectedSetting);
    this->SetInventorySetting(hqPos, Job::Private, expectedSetting);
    BOOST_TEST_REQUIRE(wh->GetInventorySetting(Job::Private) == expectedSetting);

    helpers::EnumArray<InventorySetting, Job> jobSettings;
    for(const auto i : helpers::enumRange<Job>())
        jobSettings[i] = InventorySetting(rand() % 5);
    this->SetAllInventorySettings(hqPos, true, makeVector(jobSettings));
    for(const auto i : helpers::enumRange<Job>())
    {
        // Boat carriers are stored as helpers and boats
        if(i == Job::BoatCarrier)
            BOOST_TEST_REQUIRE(wh->GetInventorySetting(i) == jobSettings[Job::Helper]);
        else
            BOOST_TEST_REQUIRE(wh->GetInventorySetting(i) == jobSettings[i]);
    }

    helpers::EnumArray<InventorySetting, GoodType> goodSettings;
    for(const auto i : helpers::enumRange<GoodType>())
        goodSettings[i] = InventorySetting(rand() % 5);
    this->SetAllInventorySettings(hqPos, false, makeVector(goodSettings));
    for(const auto i : helpers::enumRange<GoodType>())
        BOOST_TEST_REQUIRE(wh->GetInventorySetting(i) == goodSettings[ConvertShields(i)]);

    std::fill(goodSettings.begin(), goodSettings.end(), InventorySetting(0));
    this->SetAllInventorySettings(hqPos, false, makeVector(goodSettings));
    std::fill(jobSettings.begin(), jobSettings.end(), InventorySetting(0));
    this->SetAllInventorySettings(hqPos, true, makeVector(jobSettings));

    unsigned numBoards = wh->GetNumRealWares(GoodType::Boards);
    unsigned numWoodcutters = wh->GetNumRealFigures(Job::Woodcutter);
    this->SetInventorySetting(hqPos, GoodType::Boards, EInventorySetting::Send);
    // Nothing should happen
    RTTR_SKIP_GFS(100);
    BOOST_TEST_REQUIRE(wh->GetNumRealWares(GoodType::Boards) == numBoards);
    this->SetInventorySetting(hqPos, GoodType::Boards, InventorySetting());
    this->SetInventorySetting(hqPos, Job::Woodcutter, EInventorySetting::Send);
    // Figure goes out and wanders
    RTTR_SKIP_GFS(100);
    BOOST_TEST_REQUIRE(wh->GetNumRealFigures(Job::Woodcutter) < numWoodcutters);
    this->SetInventorySetting(hqPos, Job::Woodcutter, InventorySetting());

    numWoodcutters = wh->GetNumRealFigures(Job::Woodcutter);
    MapPoint whPos = hqPos + MapPoint(3, 0);
    BuildingFactory::CreateBuilding(world, BuildingType::Storehouse, whPos, curPlayer, Nation::Africans);
    this->BuildRoad(wh->GetFlagPos(), false, std::vector<Direction>(3, Direction::East));
    this->SetInventorySetting(hqPos, GoodType::Boards, EInventorySetting::Send);
    // Send some
    RTTR_SKIP_GFS(100);
    BOOST_TEST_REQUIRE(wh->GetNumRealWares(GoodType::Boards) < numBoards);
    this->SetInventorySetting(hqPos, GoodType::Boards, InventorySetting());
    this->SetInventorySetting(hqPos, Job::Woodcutter, EInventorySetting::Send);
    // Nothing should happen
    RTTR_SKIP_GFS(100);
    BOOST_TEST_REQUIRE(wh->GetNumRealFigures(Job::Woodcutter) < numWoodcutters);
    this->SetInventorySetting(hqPos, Job::Woodcutter, InventorySetting());
}

BOOST_FIXTURE_TEST_CASE(ChangeReserveTest, WorldWithGCExecution2P)
{
    GamePlayer& player = world.GetPlayer(curPlayer);
    nobBaseWarehouse* wh = player.GetFirstWH();
    BOOST_TEST_REQUIRE(wh);
    Inventory goods;

    // Add enough soldiers per rank
    for(auto i : SOLDIER_JOBS)
        goods.Add(i, 50);
    wh->AddGoods(goods, true); //-V522

    // Use more
    for(unsigned i = 0; i < SOLDIER_JOBS.size(); i++)
    {
        // We already have soldier per rank as reserve
        BOOST_TEST_REQUIRE(wh->GetReserveClaimed(i) == 1u);
        BOOST_TEST_REQUIRE(*wh->GetReserveAvailablePointer(i) == 1u);

        unsigned newVal = i * 5 + 2;
        unsigned numSoldiersAv = wh->GetNumVisualFigures(SOLDIER_JOBS[i]);
        BOOST_TEST_REQUIRE(wh->GetNumRealFigures(SOLDIER_JOBS[i]) == numSoldiersAv);
        // Update reserve -> Removed from inventory
        this->ChangeReserve(hqPos, i, newVal);
        BOOST_TEST_REQUIRE(wh->GetReserveClaimed(i) == newVal);
        BOOST_TEST_REQUIRE(*wh->GetReserveAvailablePointer(i) == newVal);
        // Current figure ct should be old figure count minus the new reserve soldiers (currentVal - 1 for old reserve
        // val)
        BOOST_TEST_REQUIRE(wh->GetNumVisualFigures(SOLDIER_JOBS[i]) == numSoldiersAv - (newVal - 1));
        BOOST_TEST_REQUIRE(wh->GetNumRealFigures(SOLDIER_JOBS[i]) == numSoldiersAv - (newVal - 1));
    }
    // Use less
    for(unsigned i = 0; i < SOLDIER_JOBS.size(); i++)
    {
        unsigned newVal = i * 3 + 1;
        unsigned numSoldiersAv = wh->GetNumVisualFigures(SOLDIER_JOBS[i]);
        unsigned numSoldiersReleased = *wh->GetReserveAvailablePointer(i) - newVal;
        // Release some soldiers from reserve -> Added to inventory
        this->ChangeReserve(hqPos, i, newVal);
        BOOST_TEST_REQUIRE(wh->GetReserveClaimed(i) == newVal);
        BOOST_TEST_REQUIRE(*wh->GetReserveAvailablePointer(i) == newVal);
        BOOST_TEST_REQUIRE(wh->GetNumVisualFigures(SOLDIER_JOBS[i]) == numSoldiersAv + numSoldiersReleased);
        BOOST_TEST_REQUIRE(wh->GetNumRealFigures(SOLDIER_JOBS[i]) == numSoldiersAv + numSoldiersReleased);
    }
}

BOOST_FIXTURE_TEST_CASE(Armageddon, WorldWithGCExecution2P)
{
    GamePlayer& player1 = world.GetPlayer(0);
    GamePlayer& player2 = world.GetPlayer(1);
    MapPoint hqPt1 = player1.GetHQPos();
    MapPoint hqPt2 = player2.GetHQPos();
    BOOST_TEST_REQUIRE(hqPt1.isValid());
    BOOST_TEST_REQUIRE(hqPt2.isValid());
    // Destroy everything
    this->CheatArmageddon();
    BOOST_TEST_REQUIRE(!player1.GetHQPos().isValid());
    BOOST_TEST_REQUIRE(!player2.GetHQPos().isValid());
    RTTR_FOREACH_PT(MapPoint, world.GetSize())
    {
        const MapNode& node = world.GetNode(pt);
        BOOST_TEST_REQUIRE(node.owner == 0u);
    }
    BOOST_TEST_REQUIRE(world.GetNO(hqPt1)->GetGOT() != GO_Type::NobHq);
    BOOST_TEST_REQUIRE(world.GetNO(hqPt2)->GetGOT() != GO_Type::NobHq);
    BOOST_TEST_REQUIRE(player1.IsDefeated());
    BOOST_TEST_REQUIRE(player2.IsDefeated());
}

BOOST_FIXTURE_TEST_CASE(DestroyAllTest, WorldWithGCExecution2P)
{
    GamePlayer& player1 = world.GetPlayer(0);
    GamePlayer& player2 = world.GetPlayer(1);
    MapPoint hqPt1 = player1.GetHQPos();
    MapPoint hqPt2 = player2.GetHQPos();
    BOOST_TEST_REQUIRE(hqPt1.isValid());
    BOOST_TEST_REQUIRE(hqPt2.isValid());
    // Destroy only own buildings
    this->DestroyAll();
    BOOST_TEST_REQUIRE(!player1.GetHQPos().isValid());
    BOOST_TEST_REQUIRE(player2.GetHQPos().isValid());
    RTTR_FOREACH_PT(MapPoint, world.GetSize())
    {
        const MapNode& node = world.GetNode(pt);
        BOOST_TEST_REQUIRE(node.owner != 1u);
    }
    BOOST_TEST_REQUIRE(world.GetNO(hqPt1)->GetGOT() != GO_Type::NobHq);
    BOOST_TEST_REQUIRE(world.GetNO(hqPt2)->GetGOT() == GO_Type::NobHq);
    BOOST_TEST_REQUIRE(player1.IsDefeated());
    BOOST_TEST_REQUIRE(!player2.IsDefeated());
}

BOOST_FIXTURE_TEST_CASE(SurrenderTest, WorldWithGCExecution2P)
{
    GamePlayer& player1 = world.GetPlayer(0);
    GamePlayer& player2 = world.GetPlayer(1);
    MapPoint hqPt1 = player1.GetHQPos();
    MapPoint hqPt2 = player2.GetHQPos();
    BOOST_TEST_REQUIRE(hqPt1.isValid());
    BOOST_TEST_REQUIRE(hqPt2.isValid());
    // Only sets defeated flag
    this->Surrender();
    BOOST_TEST_REQUIRE(player1.GetHQPos().isValid());
    BOOST_TEST_REQUIRE(player2.GetHQPos().isValid());
    BOOST_TEST_REQUIRE(world.GetNode(hqPt1).obj != (const noBase*)nullptr);
    BOOST_TEST_REQUIRE(world.GetNode(hqPt2).obj != (const noBase*)nullptr);
    BOOST_TEST_REQUIRE(player1.IsDefeated());
    BOOST_TEST_REQUIRE(!player2.IsDefeated());
}

BOOST_AUTO_TEST_SUITE_END()
