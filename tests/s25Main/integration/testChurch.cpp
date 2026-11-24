// Copyright (C) 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "AddonHelperFunctions.h"
#include "ChurchLoader.h"
#include "GamePlayer.h"
#include "GlobalGameSettings.h"
#include "PointOutput.h"
#include "Savegame.h"
#include "SerializedGameData.h"
#include "addons/const_addons.h"
#include "buildings/noBuildingSite.h"
#include "buildings/nobBaseWarehouse.h"
#include "buildings/nobChurch.h"
#include "buildings/nobMilitary.h"
#include "factories/BuildingFactory.h"
#include "figures/nofPassiveSoldier.h"
#include "worldFixtures/MockLocalGameState.h"
#include "worldFixtures/WorldWithGCExecution.h"
#include "worldFixtures/initGameRNG.hpp"
#include "gameTypes/GameTypesOutput.h"
#include "gameData/BuildingConsts.h"
#include "gameData/MilitaryConsts.h"
#include "gameData/SettingTypeConv.h"
#include "s25util/tmpFile.h"
#include <boost/test/unit_test.hpp>
#include <numeric>

// LCOV_EXCL_START
BOOST_TEST_DONT_PRINT_LOG_VALUE(BuildingCost)
BOOST_TEST_DONT_PRINT_LOG_VALUE(NodalObjectType)
BOOST_TEST_DONT_PRINT_LOG_VALUE(BuildingQuality)
BOOST_TEST_DONT_PRINT_LOG_VALUE(nobChurch::ServiceState)
// LCOV_EXCL_STOP

BOOST_AUTO_TEST_SUITE(ChurchSuite)

namespace {

using ChurchWorld1P = WorldWithGCExecution<1, 36, 36>;
using ChurchWorld2P = WorldWithGCExecution<2, 36, 36>;

unsigned countHealedSoldiers(const Inventory& inv)
{
    unsigned total = 0;
    for(const auto h : helpers::enumRange<HealedSoldier>())
        total += inv[h];
    return total;
}

unsigned countHealedInMilitary(const nobMilitary& mil)
{
    unsigned count = 0;
    for(const auto& troop : mil.getTroops())
    {
        if(troop.hasHealing())
            ++count;
    }
    return count;
}

void addMilitaryTroops(GameWorld& world, nobMilitary& mil, Job job, unsigned count, bool healed = false)
{
    const auto rank = getSoldierRank(job);
    for(unsigned i = 0; i < count; ++i)
    {
        auto& soldier = world.addFigure(
          mil.getPos(), std::make_unique<nofPassiveSoldier>(mil.getPos(), mil.getPlayer(), &mil, &mil, rank));
        world.getPlayer(mil.getPlayer()).increaseInventoryJob(soldier.getJobType(), 1);
        if(healed)
        {
            soldier.grantHealing();
            world.getPlayer(mil.getPlayer()).increaseInventoryJob(jobEnumToHealedSoldierEnum(soldier.getJobType()), 1);
        }
        soldier.walkToGoal();
    }
}

} // namespace

BOOST_AUTO_TEST_CASE(AddonDependenciesAndBuildingCosts)
{
    GlobalGameSettings ggs;
    BOOST_TEST(!ggs.isEnabled(AddonId::CHURCH));
    BOOST_TEST(!ggs.isEnabled(AddonId::WINE));

    // Default: Church, Wine economy, and Temple are unused
    auto unusedBldOff = makeIsUnusedBuilding(ggs);
    auto unusedWareOff = makeIsUnusedWare(ggs);
    auto unusedJobOff = makeIsUnusedJob(ggs);
    BOOST_TEST(unusedBldOff(BuildingType::Church));
    BOOST_TEST(unusedBldOff(BuildingType::Vineyard));
    BOOST_TEST(unusedBldOff(BuildingType::Winery));
    BOOST_TEST(unusedBldOff(BuildingType::Temple));
    BOOST_TEST(unusedWareOff(GoodType::Grapes));
    BOOST_TEST(unusedWareOff(GoodType::Wine));
    BOOST_TEST(unusedJobOff(Job::Winegrower));
    BOOST_TEST(unusedJobOff(Job::Vintner));
    BOOST_TEST(unusedJobOff(Job::TempleServant));

    // Enabling Church alone does NOT activate Church or Wine economy (Wine addon must also be active)
    ggs.setSelection(AddonId::CHURCH, 1);
    BOOST_TEST(!churchaddon::isAddonActive(ggs));
    auto unusedBldChurchOnly = makeIsUnusedBuilding(ggs);
    auto unusedWareChurchOnly = makeIsUnusedWare(ggs);
    auto unusedJobChurchOnly = makeIsUnusedJob(ggs);
    BOOST_TEST(unusedBldChurchOnly(BuildingType::Church));
    BOOST_TEST(unusedBldChurchOnly(BuildingType::Vineyard));
    BOOST_TEST(unusedBldChurchOnly(BuildingType::Winery));
    BOOST_TEST(unusedBldChurchOnly(BuildingType::Temple));
    BOOST_TEST(unusedWareChurchOnly(GoodType::Grapes));
    BOOST_TEST(unusedWareChurchOnly(GoodType::Wine));
    BOOST_TEST(unusedJobChurchOnly(Job::Winegrower));
    BOOST_TEST(unusedJobChurchOnly(Job::Vintner));
    BOOST_TEST(unusedJobChurchOnly(Job::TempleServant));

    // Enabling Wine alone activates Wine economy & Temple, but NOT Church
    ggs.resetAddons();
    ggs.setSelection(AddonId::WINE, 1);
    BOOST_TEST(!churchaddon::isAddonActive(ggs));
    auto unusedBldWineOnly = makeIsUnusedBuilding(ggs);
    auto unusedWareWineOnly = makeIsUnusedWare(ggs);
    auto unusedJobWineOnly = makeIsUnusedJob(ggs);
    BOOST_TEST(unusedBldWineOnly(BuildingType::Church));
    BOOST_TEST(!unusedBldWineOnly(BuildingType::Vineyard));
    BOOST_TEST(!unusedBldWineOnly(BuildingType::Winery));
    BOOST_TEST(!unusedBldWineOnly(BuildingType::Temple));
    BOOST_TEST(!unusedWareWineOnly(GoodType::Grapes));
    BOOST_TEST(!unusedWareWineOnly(GoodType::Wine));
    BOOST_TEST(!unusedJobWineOnly(Job::Winegrower));
    BOOST_TEST(!unusedJobWineOnly(Job::Vintner));
    BOOST_TEST(!unusedJobWineOnly(Job::TempleServant));

    // Enabling BOTH Church and Wine economy activates Church as well
    ggs.setSelection(AddonId::CHURCH, 1);
    BOOST_TEST(churchaddon::isAddonActive(ggs));
    auto unusedBldBoth = makeIsUnusedBuilding(ggs);
    BOOST_TEST(!unusedBldBoth(BuildingType::Church));
    BOOST_TEST(!unusedBldBoth(BuildingType::Vineyard));
    BOOST_TEST(!unusedBldBoth(BuildingType::Winery));
    BOOST_TEST(!unusedBldBoth(BuildingType::Temple));

    // Building size & double Fortress cost (8 boards, 14 stones)
    BOOST_TEST(BUILDING_SIZE[BuildingType::Church] == BuildingQuality::Castle);
    BOOST_TEST(BUILDING_COSTS[BuildingType::Church].boards == BUILDING_COSTS[BuildingType::Fortress].boards * 2);
    BOOST_TEST(BUILDING_COSTS[BuildingType::Church].stones == BUILDING_COSTS[BuildingType::Fortress].stones * 2);
    BOOST_TEST(BUILDING_COSTS[BuildingType::Church].boards == 8u);
    BOOST_TEST(BUILDING_COSTS[BuildingType::Church].stones == 14u);
    BOOST_TEST(churchaddon::CHURCH_FIRST_SERVICE_DELAY == 600u);
    BOOST_TEST(churchaddon::CHURCH_SERVICE_INTERVAL == 6000u);
    BOOST_TEST(churchaddon::CHURCH_SERVICE_DURATION == 100u);

    // +50% hitpoints rounded up for each soldier rank
    BOOST_TEST(getMaxSoldierHitpoints(0, false) == 3);
    BOOST_TEST(getMaxSoldierHitpoints(0, true) == 5);
    BOOST_TEST(getMaxSoldierHitpoints(1, false) == 4);
    BOOST_TEST(getMaxSoldierHitpoints(1, true) == 6);
    BOOST_TEST(getMaxSoldierHitpoints(2, false) == 5);
    BOOST_TEST(getMaxSoldierHitpoints(2, true) == 8);
    BOOST_TEST(getMaxSoldierHitpoints(3, false) == 6);
    BOOST_TEST(getMaxSoldierHitpoints(3, true) == 9);
    BOOST_TEST(getMaxSoldierHitpoints(4, false) == 7);
    BOOST_TEST(getMaxSoldierHitpoints(4, true) == 11);

    // Nation-specific church sprite mapping & unique dove hover height per nation
    unsigned expectedBase = 0;
    for(const Nation nation : helpers::enumRange<Nation>())
    {
        BOOST_TEST(churchaddon::getNationSpriteId(nation, churchaddon::NationSprite::Building) == expectedBase + 0);
        BOOST_TEST(churchaddon::getNationSpriteId(nation, churchaddon::NationSprite::BuildingShadow)
                   == expectedBase + 1);
        BOOST_TEST(churchaddon::getNationSpriteId(nation, churchaddon::NationSprite::Skeleton) == expectedBase + 2);
        BOOST_TEST(churchaddon::getNationSpriteId(nation, churchaddon::NationSprite::SkeletonShadow)
                   == expectedBase + 3);
        BOOST_TEST(churchaddon::getNationSpriteId(nation, churchaddon::NationSprite::Door) == expectedBase + 4);
        BOOST_TEST(churchaddon::getNationSpriteId(nation, churchaddon::NationSprite::Icon) == expectedBase + 5);
        BOOST_TEST(churchaddon::getDoveOffsetY(nation) <= -80);
        expectedBase += churchaddon::NUM_SPRITES_PER_NATION;
    }
}

BOOST_FIXTURE_TEST_CASE(PlacementRestrictedToWarehouseRadius, ChurchWorld1P)
{
    const MapPoint closePt = hqPos + MapPoint(4, 0);
    BOOST_TEST_REQUIRE(world.calcDistance(hqPos, closePt) <= churchaddon::CHURCH_PLACEMENT_RADIUS);

    // Campaign / default addons (rttr:ResetAddons): neither Church nor Wine active -> rejected
    ggs.resetAddons();
    BOOST_TEST(!world.canBuildChurchAt(closePt, curPlayer));
    this->SetBuildingSite(closePt, BuildingType::Church);
    BOOST_TEST(world.getNO(closePt)->getType() == NodalObjectType::Nothing);

    // Church addon ON without Wine addon: still rejected
    ggs.setSelection(AddonId::CHURCH, 1);
    BOOST_TEST(!world.canBuildChurchAt(closePt, curPlayer));
    this->SetBuildingSite(closePt, BuildingType::Church);
    BOOST_TEST(world.getNO(closePt)->getType() == NodalObjectType::Nothing);

    // Wine addon ON without Church addon: still rejected
    ggs.setSelection(AddonId::CHURCH, 0);
    ggs.setSelection(AddonId::WINE, 1);
    BOOST_TEST(!world.canBuildChurchAt(closePt, curPlayer));
    this->SetBuildingSite(closePt, BuildingType::Church);
    BOOST_TEST(world.getNO(closePt)->getType() == NodalObjectType::Nothing);

    // Both addons ON within 9 tiles of HQ: allowed
    ggs.setSelection(AddonId::CHURCH, 1);
    BOOST_TEST(world.canBuildChurchAt(closePt, curPlayer));
    this->SetBuildingSite(closePt, BuildingType::Church);
    BOOST_TEST_REQUIRE(world.getNO(closePt)->getType() == NodalObjectType::Buildingsite);
    BOOST_TEST(world.getSpecObj<noBaseBuilding>(closePt)->getBuildingType() == BuildingType::Church);

    // Expand territory out to x+20 using a Barracks (military radius 8)
    const MapPoint milPos = hqPos + MapPoint(15, 0);
    auto* mil = static_cast<nobMilitary*>(
      BuildingFactory::createBuilding(world, BuildingType::Barracks, milPos, curPlayer, Nation::Romans));
    addMilitaryTroops(world, *mil, Job::Private, 1);
    world.recalcTerritory(*mil, TerritoryChangeReason::Build);

    // Outside 9 tiles of HQ, even though terrain is owned and Castle-sized BQ: rejected
    const MapPoint farPt = hqPos + MapPoint(11, 0);
    BOOST_TEST_REQUIRE(world.calcDistance(hqPos, farPt) == 11u);
    BOOST_TEST_REQUIRE(world.getBQ(farPt, curPlayer) == BuildingQuality::Castle);
    BOOST_TEST(!world.canBuildChurchAt(farPt, curPlayer));
    this->SetBuildingSite(farPt, BuildingType::Church);
    BOOST_TEST(world.getNO(farPt)->getType() == NodalObjectType::Nothing);

    // Near a HarborBuilding only (NOT HQ/Storehouse): still rejected!
    const MapPoint harborPos = hqPos + MapPoint(18, 4);
    BuildingFactory::createBuilding(world, BuildingType::HarborBuilding, harborPos, curPlayer, Nation::Romans);
    const MapPoint nearHarborPt = hqPos + MapPoint(14, 4);
    BOOST_TEST_REQUIRE(world.calcDistance(harborPos, nearHarborPt) <= 5u);
    BOOST_TEST_REQUIRE(world.calcDistance(hqPos, nearHarborPt) > churchaddon::CHURCH_PLACEMENT_RADIUS);
    BOOST_TEST(!world.canBuildChurchAt(nearHarborPt, curPlayer));
    this->SetBuildingSite(nearHarborPt, BuildingType::Church);
    BOOST_TEST(world.getNO(nearHarborPt)->getType() == NodalObjectType::Nothing);

    // Near a Storehouse: allowed!
    const MapPoint storePos = hqPos + MapPoint(18, -4);
    BuildingFactory::createBuilding(world, BuildingType::Storehouse, storePos, curPlayer, Nation::Romans);
    const MapPoint nearStorePt = hqPos + MapPoint(14, -4);
    BOOST_TEST_REQUIRE(world.calcDistance(storePos, nearStorePt) <= churchaddon::CHURCH_PLACEMENT_RADIUS);
    BOOST_TEST(world.canBuildChurchAt(nearStorePt, curPlayer));
    this->SetBuildingSite(nearStorePt, BuildingType::Church);
    BOOST_TEST_REQUIRE(world.getNO(nearStorePt)->getType() == NodalObjectType::Buildingsite);
    BOOST_TEST(world.getSpecObj<noBaseBuilding>(nearStorePt)->getBuildingType() == BuildingType::Church);
}

BOOST_FIXTURE_TEST_CASE(BlessingProductionAndCapAt10, ChurchWorld1P)
{
    ggs.setSelection(AddonId::CHURCH, 1);
    ggs.setSelection(AddonId::WINE, 1);
    initGameRNG();

    auto* hq = world.getSpecObj<nobBaseWarehouse>(hqPos);
    BOOST_TEST_REQUIRE(hq);
    GoodsAndPeopleCounts supply;
    supply[GoodType::Fish] = 15;
    supply[GoodType::Wine] = 15;
    supply[GoodType::Bread] = 15;
    supply[Job::Helper] = 5;
    hq->addToInventory(supply, true);

    const MapPoint churchPos = hqPos + MapPoint(4, 0);
    auto* church = static_cast<nobChurch*>(
      BuildingFactory::createBuilding(world, BuildingType::Church, churchPos, curPlayer, Nation::Romans));
    this->BuildRoad(world.getNeighbour(hqPos, Direction::SouthEast), false,
                    std::vector<Direction>(4, Direction::East));

    RTTR_EXEC_TILL(12000, church->getNumBlessings() == churchaddon::MAX_CHURCH_BLESSINGS);
    BOOST_TEST(church->isBlessingStorageFull());

    // Continue running to confirm cap stays at 10
    RTTR_SKIP_GFS(1500);
    BOOST_TEST(church->getNumBlessings() == churchaddon::MAX_CHURCH_BLESSINGS);
}

BOOST_FIXTURE_TEST_CASE(ServiceAttendanceAndSoldierOnlyBlessing, ChurchWorld1P)
{
    ggs.setSelection(AddonId::CHURCH, 1);
    ggs.setSelection(AddonId::WINE, 1);
    initGameRNG();
    this->ChangeMilitary(MILITARY_SETTINGS_SCALE);

    auto* hq = world.getSpecObj<nobBaseWarehouse>(hqPos);
    BOOST_TEST_REQUIRE(hq);
    // Keep exact ware counts so the church produces 2 blessings before next service (~6600 GFs)
    hq->clear();
    for(unsigned r = 0; r < NUM_SOLDIER_RANKS; ++r)
    {
        hq->setRealReserve(r, 0);
        hq->setReserveVisual(r, 0);
    }
    GoodsAndPeopleCounts clearAndSupply;
    clearAndSupply[GoodType::Fish] = 2;
    clearAndSupply[GoodType::Wine] = 2;
    clearAndSupply[GoodType::Bread] = 2;
    clearAndSupply[Job::Helper] = 10;
    clearAndSupply[Job::Private] = 1;
    hq->addToInventory(clearAndSupply, true);

    const MapPoint churchPos = hqPos + MapPoint(4, 0);
    auto* church = static_cast<nobChurch*>(
      BuildingFactory::createBuilding(world, BuildingType::Church, churchPos, curPlayer, Nation::Romans));
    this->BuildRoad(world.getNeighbour(hqPos, Direction::SouthEast), false,
                    std::vector<Direction>(4, Direction::East));

    // Watchtower with 6 soldiers: 20% = 2 attend (leaving 4 inside)
    const MapPoint watchPos = hqPos + MapPoint(8, 0);
    auto* watchtower = static_cast<nobMilitary*>(
      BuildingFactory::createBuilding(world, BuildingType::Watchtower, watchPos, curPlayer, Nation::Romans));
    addMilitaryTroops(world, *watchtower, Job::Private, 5);
    addMilitaryTroops(world, *watchtower, Job::General, 1);
    this->BuildRoad(world.getNeighbour(churchPos, Direction::SouthEast), false,
                    std::vector<Direction>(4, Direction::East));

    // Barracks with 1 soldier (limit 1 so it does not recruit HQ's Private): must keep >= 1 inside during service
    const MapPoint barracksPos = hqPos + MapPoint(-4, 0);
    auto* barracks = static_cast<nobMilitary*>(
      BuildingFactory::createBuilding(world, BuildingType::Barracks, barracksPos, curPlayer, Nation::Romans));
    addMilitaryTroops(world, *barracks, Job::Private, 1);
    barracks->SetTroopLimit(0, 1);
    this->BuildRoad(world.getNeighbour(barracksPos, Direction::SouthEast), false,
                    std::vector<Direction>(4, Direction::East));

    // Wait until 2 blessings are crafted; productivity stays 0% until a service blesses soldiers
    RTTR_EXEC_TILL(2500, church->getNumBlessings() == 2u);
    BOOST_TEST(church->getProductivity() == 0u);

    // Wait for next church service gathering phase (interval = 6000 GFs)
    RTTR_EXEC_TILL(7500, church->getServiceState() == nobChurch::ServiceState::Gathering
                           && church->getNumActiveVisitors() > 0);
    BOOST_TEST(barracks->getNumTroops() == 1u);
    BOOST_TEST(watchtower->getNumTroops() == 4u);
    BOOST_TEST(church->getNumSoldiersInside() == 0u);
    BOOST_TEST(church->getNumSoldiersEnRoute() > 0u);

    // Wait until EVERY invited visitor is inside -> only then does the 100-GF InService phase start!
    RTTR_EXEC_TILL(1500, church->getServiceState() == nobChurch::ServiceState::InService);
    const unsigned soldiersInside = church->getNumSoldiersInside();
    BOOST_TEST(soldiersInside == 3u); // 2 from Watchtower + 1 Private from HQ
    BOOST_TEST(church->getNumSoldiersEnRoute() == 0u);

    // Wait 95 GFs inside the 100-GF service: everyone must STILL be inside and NOT yet blessed/exited
    RTTR_SKIP_GFS(95);
    BOOST_TEST(church->getServiceState() == nobChurch::ServiceState::InService);
    BOOST_TEST(church->getNumSoldiersInside() == soldiersInside);
    BOOST_TEST(church->getNumBlessings() == 2u);

    // Wait remaining frames for the 100-GF service to end and everyone to exit together
    RTTR_EXEC_TILL(20, church->getServiceState() == nobChurch::ServiceState::Idle);
    BOOST_TEST(church->getNumActiveVisitors() == 0u);
    BOOST_TEST(church->getNumSoldiersInside() == 0u);
    // Only 2 Blessings existed -> 2 soldiers blessed at service end, Helpers did not consume blessings
    BOOST_TEST(church->getNumBlessings() == 0u);
    // Productivity = 10% per blessed soldier in the last service (2 soldiers -> 20%)
    BOOST_TEST(church->getProductivity() == 20u);

    // Wait for all visitors to return home
    RTTR_EXEC_TILL(1500, watchtower->getNumTroops() == 6u && hq->getLeavingFigures().empty());
    // Wait until HQ Private has also walked back into HQ
    RTTR_EXEC_TILL(1500, hq->getNumRealFigures(Job::Private) == 1u);

    const unsigned totalHealed = countHealedSoldiers(world.getPlayer(curPlayer).getInventory());
    BOOST_TEST(totalHealed == 2u);
    const unsigned watchHealed = countHealedInMilitary(*watchtower);
    const unsigned hqHealed = countHealedSoldiers(hq->getInventory());
    BOOST_TEST(watchHealed + hqHealed == 2u);

    for(const auto& troop : watchtower->getTroops())
    {
        if(troop.hasHealing())
            BOOST_TEST(troop.getMaxHitpoints() == getMaxSoldierHitpoints(troop.getRank(), true));
        else
            BOOST_TEST(troop.getMaxHitpoints() == getMaxSoldierHitpoints(troop.getRank(), false));
    }

    // Verify next service does NOT happen prematurely during the first 3000 GFs (doubled to 6000 GFs)
    RTTR_SKIP_GFS(3500);
    BOOST_TEST(church->getServiceState() == nobChurch::ServiceState::Idle);
    RTTR_EXEC_TILL(3000, church->isServiceActive());
}

BOOST_FIXTURE_TEST_CASE(ConvalescenceToBoostedMaxAndDestructionSafety, ChurchWorld1P)
{
    ggs.setSelection(AddonId::CHURCH, 1);
    ggs.setSelection(AddonId::WINE, 1);
    initGameRNG();
    this->ChangeMilitary(MILITARY_SETTINGS_SCALE);

    auto* hq = world.getSpecObj<nobBaseWarehouse>(hqPos);
    BOOST_TEST_REQUIRE(hq);
    for(unsigned r = 0; r < NUM_SOLDIER_RANKS; ++r)
    {
        hq->setRealReserve(r, 0);
        hq->setReserveVisual(r, 0);
    }

    const unsigned initialHqPrivates = hq->getNumRealFigures(Job::Private);
    const unsigned initialTotalPrivates = world.getPlayer(curPlayer).getInventory()[Job::Private];

    const MapPoint churchPos = hqPos + MapPoint(4, 0);
    auto* church = static_cast<nobChurch*>(
      BuildingFactory::createBuilding(world, BuildingType::Church, churchPos, curPlayer, Nation::Romans));

    const MapPoint milPos = hqPos + MapPoint(8, 0);
    auto* mil = static_cast<nobMilitary*>(
      BuildingFactory::createBuilding(world, BuildingType::Watchtower, milPos, curPlayer, Nation::Romans));
    addMilitaryTroops(world, *mil, Job::Private, 2, true);

    // Wound a healed Private (5 max HP) down to 2 HP and verify convalescence heals back to 5, not 3
    auto* soldier = const_cast<nofPassiveSoldier*>(&mil->getTroops().front());
    BOOST_TEST_REQUIRE(soldier->hasHealing());
    soldier->setHitpoints(2);
    soldier->heal();
    RTTR_EXEC_TILL(4000, soldier->getHitpoints() == 5u);
    BOOST_TEST(soldier->getMaxHitpoints() == 5u);

    // Now connect roads, wait for service to begin (soldiers inside), and destroy church mid-service
    this->BuildRoad(world.getNeighbour(hqPos, Direction::SouthEast), false,
                    std::vector<Direction>(4, Direction::East));
    this->BuildRoad(world.getNeighbour(churchPos, Direction::SouthEast), false,
                    std::vector<Direction>(4, Direction::East));

    RTTR_EXEC_TILL(7500, church->getServiceState() == nobChurch::ServiceState::InService
                           && church->getNumSoldiersInside() > 0);
    this->DestroyBuilding(churchPos);
    BOOST_TEST(world.getNO(churchPos)->getType() == NodalObjectType::Fire);

    // All displaced visitors return cleanly to warehouses or military building
    RTTR_EXEC_TILL(1500, mil->getNumTroops() + hq->getNumRealFigures(Job::Private) == initialHqPrivates + 2u);
    BOOST_TEST(world.getPlayer(curPlayer).getInventory()[Job::Private] == initialTotalPrivates + 2u);
}

BOOST_FIXTURE_TEST_CASE(BlessingHpCombatPersistenceAndSaveLoad, ChurchWorld2P)
{
    ggs.setSelection(AddonId::CHURCH, 1);
    ggs.setSelection(AddonId::WINE, 1);
    initGameRNG();
    this->ChangeMilitary(MILITARY_SETTINGS_SCALE);

    auto* hq = world.getSpecObj<nobBaseWarehouse>(hqPos);
    BOOST_TEST_REQUIRE(hq);
    hq->clear();
    for(unsigned r = 0; r < NUM_SOLDIER_RANKS; ++r)
    {
        hq->setRealReserve(r, 0);
        hq->setReserveVisual(r, 0);
    }
    GoodsAndPeopleCounts troopsSupply;
    troopsSupply[Job::Private] = 50; // 20% = 10 Privates invited to service
    hq->addToInventory(troopsSupply, true);

    const MapPoint churchPos = hqPos + MapPoint(4, 0);
    auto* church = static_cast<nobChurch*>(
      BuildingFactory::createBuilding(world, BuildingType::Church, churchPos, curPlayer, Nation::Romans));
    for(unsigned i = 0; i < churchaddon::MAX_CHURCH_BLESSINGS; ++i)
        church->addBlessing();
    BOOST_TEST(church->getNumBlessings() == 10u);
    BOOST_TEST(church->getProductivity() == 0u);
    this->BuildRoad(world.getNeighbour(hqPos, Direction::SouthEast), false, std::vector<Direction>(4, Direction::East));

    // Wait for the 600-GF service to gather all 10 soldiers, wait 100 GFs inside, bless all 10, and finish
    RTTR_EXEC_TILL(4000, church->getServiceState() == nobChurch::ServiceState::Idle && em.getCurrentGF() > 700u
                           && hq->getLeavingFigures().empty() && hq->getNumRealFigures(Job::Private) == 50u);
    BOOST_TEST(church->getNumBlessings() == 0u);
    BOOST_TEST(church->getProductivity() == 100u);
    BOOST_TEST(countHealedSoldiers(hq->getInventory()) == 10u);

    // Re-add 2 stored blessings and verify permanent +50% HP buff & productivity survive save/load
    church->addBlessing();
    church->addBlessing();
    const MapPoint milPos = hqPos + MapPoint(8, 0);
    auto* mil = static_cast<nobMilitary*>(
      BuildingFactory::createBuilding(world, BuildingType::Fortress, milPos, curPlayer, Nation::Romans));
    addMilitaryTroops(world, *mil, Job::General, 1, true);
    addMilitaryTroops(world, *mil, Job::Private, 1, true);

    BOOST_TEST(mil->getSoldiersStrength() == 11u + 5u);

    Savegame save;
    for(unsigned i = 0; i < world.getNumPlayers(); i++)
        save.addPlayer(world.getPlayer(i));
    save.ggs = ggs;
    save.start_gf = em.getCurrentGF();
    save.sgd.makeSnapshot(*game);

    TmpFile tmp;
    tmp.close();
    BOOST_TEST_REQUIRE(save.save(tmp.filePath, "ChurchTest"));

    Savegame loadedSave;
    BOOST_TEST_REQUIRE(loadedSave.load(tmp.filePath, SaveGameDataToLoad::All));

    std::vector<PlayerInfo> players;
    for(unsigned i = 0; i < loadedSave.getNumPlayers(); ++i)
        players.emplace_back(loadedSave.getPlayer(i));
    auto loadedGame = std::make_shared<Game>(loadedSave.ggs, loadedSave.start_gf, players);
    MockLocalGameState localState;
    loadedSave.sgd.readSnapshot(*loadedGame, localState);
    loadedGame->world_.initAfterLoad();

    const auto* loadedChurch = loadedGame->world_.getSpecObj<nobChurch>(churchPos);
    const auto* loadedMil = loadedGame->world_.getSpecObj<nobMilitary>(milPos);
    BOOST_TEST_REQUIRE(loadedChurch);
    BOOST_TEST_REQUIRE(loadedMil);
    BOOST_TEST(loadedChurch->getNumBlessings() == 2u);
    BOOST_TEST(loadedChurch->getProductivity() == 100u);
    BOOST_TEST(countHealedInMilitary(*loadedMil) == 2u);
    BOOST_TEST(loadedMil->getSoldiersStrength() == 16u);
    BOOST_TEST(countHealedSoldiers(loadedGame->world_.getPlayer(0).getInventory()) == 12u);
}

BOOST_AUTO_TEST_SUITE_END()
