// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "buildings/nobBaseWarehouse.h"
#include "buildings/nobUsual.h"
#include "factories/BuildingFactory.h"
#include "postSystem/PostBox.h"
#include "postSystem/PostMsg.h"
#include "random/Random.h"
#include "worldFixtures/WorldWithGCExecution.h"
#include "gameTypes/MineNoOutputFallback.h"
#include "gameTypes/MineResourceBehavior.h"
#include "gameData/ToolConsts.h"
#include <rttr/test/LogAccessor.hpp>
#include <boost/test/data/monomorphic.hpp>
#include <boost/test/data/test_case.hpp>
#include <boost/test/unit_test.hpp>
#include <algorithm>
#include <array>
#include <ostream>

namespace dataset = boost::unit_test::data;

// LCOV_EXCL_START
static std::ostream& operator<<(std::ostream& os, const PostCategory& cat)
{
    return os << static_cast<unsigned>(cat);
}
// LCOV_EXCL_STOP

BOOST_AUTO_TEST_SUITE(Production)

namespace {
// Provides a single, connected, staffed mine so tests only differ in the mine type, its resource spot and the
// addon settings under test. The miner supplies are added in the ctor (comment: setup belongs in the fixture);
// the mine itself is created per test because its type/resource is what varies.
struct MineProductionFixture : WorldWithGCExecution1P
{
    // Enough GFs for several miner production cycles. Reused so timing-based tests stay comparable.
    static constexpr unsigned maxProductionGFs = 5000;
    // Seed + window shared by the two S4-like exhaustion tests so they form a direct comparison: fed the SAME random
    // sequence, a near-exhausted mine (1 resource -> ~5% chance) produces nothing within the window, while a full
    // mine (many resources -> high chance) reliably completes a depleting cycle. Only the resource amount differs.
    static constexpr unsigned s4LikeComparisonSeed = 2;
    static constexpr unsigned s4LikeComparisonGFs = 5000;

    MineProductionFixture()
    {
        GoodsAndPeopleCounts inv;
        inv[GoodType::Fish] = 40;
        inv[GoodType::PickAxe] = 1;
        inv[Job::Miner] = 1;
        world.GetSpecObj<nobBaseWarehouse>(hqPos)->AddToInventory(inv, true);
    }

    // Places a mine of the given type next to the HQ, optionally seeds its resource spot, connects it by road and
    // waits until the miner has moved in. Returns the mine position.
    MapPoint CreateMine(const BuildingType mineType, const Resource initialResource = Resource())
    {
        const MapPoint minePos = hqPos + MapPoint(2, 0);
        const auto* mine =
          static_cast<nobUsual*>(BuildingFactory::CreateBuilding(world, mineType, minePos, curPlayer, Nation::Romans));
        if(initialResource.getType() != ResourceType::Nothing)
            world.GetNodeWriteable(minePos).resources = initialResource;
        BuildRoad(world.GetNeighbour(minePos, Direction::SouthEast), false, std::vector<Direction>(2, Direction::West));
        RTTR_EXEC_TILL(500, mine->HasWorker());
        return minePos;
    }

    static void SeedProductionRng(const unsigned seed) { RANDOM.Init(seed); }
};

// One S4-like "no output" fallback scenario: a nearly exhausted mine that keeps working but usually mines nothing,
// so the configured fallback ware is produced instead of the primary ware.
struct NoOutputFallbackCase
{
    AddonId mineBehaviorAddon;
    BuildingType mineType;
    ResourceType mineResource;
    GoodType primaryGood; // must stay unchanged (deposit too small to actually mine)
    MineNoOutputFallback fallback;
    GoodType fallbackGood; // must be produced instead
    unsigned seed;         // chosen so the (probabilistic) fallback fires within maxProductionGFs
    friend std::ostream& operator<<(std::ostream& os, const NoOutputFallbackCase& c)
    {
        return os << "fallback=" << static_cast<unsigned>(c.fallback);
    }
};
} // namespace

BOOST_FIXTURE_TEST_CASE(MetalWorkerStopped, WorldWithGCExecution1P)
{
    addStartResources();
    rttr::test::LogAccessor logAcc;
    ggs.setSelection(AddonId::TOOL_ORDERING, 1);
    ggs.setSelection(AddonId::METALWORKSBEHAVIORONZERO, 1);
    world.GetSpecObj<nobBaseWarehouse>(hqPos)->AddToInventory(GoodCounts::make(GoodType::Iron, 10), true);
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
    GoodsAndPeopleCounts inv;
    inv[GoodType::Boards] = 10;
    inv[GoodType::Iron] = 10;
    inv[Job::Metalworker] = 1;
    world.GetSpecObj<nobBaseWarehouse>(hqPos)->AddToInventory(inv, true);
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

// Without any deposit under the mine, only the WorkEverywhere behavior lets it produce; Default and Inexhaustible
// both keep needing an actual resource spot. Depletion/production here is deterministic, so no RNG seed is needed.
BOOST_FIXTURE_TEST_CASE(GraniteMineWithoutResourcesNeedsAddon, MineProductionFixture)
{
    CreateMine(BuildingType::GraniteMine);
    const Inventory& curInventory = world.GetPlayer(curPlayer).GetInventory();
    const unsigned initialStones = curInventory[GoodType::Stones];

    RTTR_SKIP_GFS(maxProductionGFs);

    BOOST_TEST(curInventory[GoodType::Stones] == initialStones);
}

BOOST_FIXTURE_TEST_CASE(InexhaustibleGraniteMineStillNeedsResourceSpot, MineProductionFixture)
{
    ggs.setSelection(AddonId::GRANITEMINE_RESOURCE_BEHAVIOR,
                     static_cast<unsigned>(MineResourceBehavior::Inexhaustible));
    CreateMine(BuildingType::GraniteMine);
    const Inventory& curInventory = world.GetPlayer(curPlayer).GetInventory();
    const unsigned initialStones = curInventory[GoodType::Stones];

    RTTR_SKIP_GFS(maxProductionGFs);

    BOOST_TEST(curInventory[GoodType::Stones] == initialStones);
}

BOOST_FIXTURE_TEST_CASE(GraniteMineWorkEverywhereProducesWithoutCreatingResource, MineProductionFixture)
{
    ggs.setSelection(AddonId::GRANITEMINE_RESOURCE_BEHAVIOR,
                     static_cast<unsigned>(MineResourceBehavior::WorkEverywhere));
    const MapPoint minePos = CreateMine(BuildingType::GraniteMine);
    const Inventory& curInventory = world.GetPlayer(curPlayer).GetInventory();
    const unsigned initialStones = curInventory[GoodType::Stones];

    RTTR_EXEC_TILL(maxProductionGFs, curInventory[GoodType::Stones] > initialStones);
    // WorkEverywhere must not conjure a deposit into the ground
    BOOST_TEST(static_cast<unsigned>(world.GetNode(minePos).resources.getType())
               == static_cast<unsigned>(ResourceType::Nothing));
    BOOST_TEST(world.GetNode(minePos).resources.getAmount() == 0u);
}

BOOST_FIXTURE_TEST_CASE(GraniteMineWorkEverywhereIgnoresExistingResource, MineProductionFixture)
{
    ggs.setSelection(AddonId::GRANITEMINE_RESOURCE_BEHAVIOR,
                     static_cast<unsigned>(MineResourceBehavior::WorkEverywhere));
    // A granite mine on a foreign (coal) deposit still just makes stones and leaves the deposit untouched.
    const Resource foreignDeposit(ResourceType::Coal, 4);
    const MapPoint minePos = CreateMine(BuildingType::GraniteMine, foreignDeposit);
    const Inventory& curInventory = world.GetPlayer(curPlayer).GetInventory();
    const unsigned initialStones = curInventory[GoodType::Stones];

    RTTR_EXEC_TILL(maxProductionGFs, curInventory[GoodType::Stones] > initialStones);
    BOOST_TEST(world.GetNode(minePos).resources.has(foreignDeposit.getType()));
    BOOST_TEST(world.GetNode(minePos).resources.getAmount() == foreignDeposit.getAmount());
}

BOOST_FIXTURE_TEST_CASE(CoalMineInexhaustibleBehaviorDoesNotDepleteResource, MineProductionFixture)
{
    ggs.setSelection(AddonId::COALMINE_RESOURCE_BEHAVIOR, static_cast<unsigned>(MineResourceBehavior::Inexhaustible));
    const Resource initCoal(ResourceType::Coal, 4);
    const MapPoint minePos = CreateMine(BuildingType::CoalMine, initCoal);
    const Inventory& curInventory = world.GetPlayer(curPlayer).GetInventory();
    const unsigned initialCoal = curInventory[GoodType::Coal];

    RTTR_EXEC_TILL(maxProductionGFs, curInventory[GoodType::Coal] > initialCoal);

    // Inexhaustible mines produce without ever reducing the deposit
    BOOST_TEST(world.GetNode(minePos).resources.has(initCoal.getType()));
    BOOST_TEST(world.GetNode(minePos).resources.getAmount() == initCoal.getAmount());
}

// Comparison A: an almost exhausted S4-like mine mostly mines nothing, so the coal count stays put (see the
// s4LikeComparison* constants). Comparison B is CoalMineS4LikeExhaustionReducesResourceOnSuccessfulCycle below.
BOOST_FIXTURE_TEST_CASE(CoalMineS4LikeExhaustionCanProduceNothing, MineProductionFixture)
{
    ggs.setSelection(AddonId::COALMINE_RESOURCE_BEHAVIOR,
                     static_cast<unsigned>(MineResourceBehavior::S4LikeExhaustion));
    const MapPoint minePos = CreateMine(BuildingType::CoalMine, Resource(ResourceType::Coal, 1));
    const Inventory& curInventory = world.GetPlayer(curPlayer).GetInventory();
    const unsigned initialCoal = curInventory[GoodType::Coal];

    SeedProductionRng(s4LikeComparisonSeed);
    RTTR_SKIP_GFS(s4LikeComparisonGFs);

    BOOST_TEST(curInventory[GoodType::Coal] == initialCoal);
    BOOST_TEST(world.GetNode(minePos).resources.getAmount() == 1u);
}

// Same scenario for every configured no-output fallback: an S4-like mine sitting on a single-unit deposit keeps
// working but (almost) never mines it, so the fallback ware appears while the primary ware and the deposit are
// untouched. The seeds are per-case because the fallback chance is probabilistic (25%/50% need a matching roll).
BOOST_DATA_TEST_CASE_F(
  MineProductionFixture, S4LikeNoOutputFallbackProducesFallbackWare,
  dataset::make(std::array{
    NoOutputFallbackCase{AddonId::COALMINE_RESOURCE_BEHAVIOR, BuildingType::CoalMine, ResourceType::Coal,
                         GoodType::Coal, MineNoOutputFallback::ProduceGranite25, GoodType::Stones, 2},
    NoOutputFallbackCase{AddonId::COALMINE_RESOURCE_BEHAVIOR, BuildingType::CoalMine, ResourceType::Coal,
                         GoodType::Coal, MineNoOutputFallback::ProduceGranite50, GoodType::Stones, 7},
    NoOutputFallbackCase{AddonId::COALMINE_RESOURCE_BEHAVIOR, BuildingType::CoalMine, ResourceType::Coal,
                         GoodType::Coal, MineNoOutputFallback::ProduceGranite100, GoodType::Stones, 2},
    NoOutputFallbackCase{AddonId::GOLDMINE_RESOURCE_BEHAVIOR, BuildingType::GoldMine, ResourceType::Gold,
                         GoodType::Gold, MineNoOutputFallback::ProduceLowerGradeResource, GoodType::IronOre, 2}}))
{
    ggs.setSelection(sample.mineBehaviorAddon, static_cast<unsigned>(MineResourceBehavior::S4LikeExhaustion));
    ggs.setSelection(AddonId::MINE_NO_OUTPUT_FALLBACK, static_cast<unsigned>(sample.fallback));
    const MapPoint minePos = CreateMine(sample.mineType, Resource(sample.mineResource, 1));
    const Inventory& curInventory = world.GetPlayer(curPlayer).GetInventory();
    const unsigned initialPrimary = curInventory[sample.primaryGood];
    const unsigned initialFallback = curInventory[sample.fallbackGood];

    SeedProductionRng(sample.seed);
    RTTR_EXEC_TILL(maxProductionGFs, curInventory[sample.fallbackGood] > initialFallback);

    BOOST_TEST(curInventory[sample.primaryGood] == initialPrimary);
    BOOST_TEST(world.GetNode(minePos).resources.getAmount() == 1u);
}

// Comparison B (see CoalMineS4LikeExhaustionCanProduceNothing): same seed and window, but a full deposit reliably
// completes a producing cycle, which reduces the deposit by one (down to, but never below, the minimum of 1).
BOOST_FIXTURE_TEST_CASE(CoalMineS4LikeExhaustionReducesResourceOnSuccessfulCycle, MineProductionFixture)
{
    ggs.setSelection(AddonId::COALMINE_RESOURCE_BEHAVIOR,
                     static_cast<unsigned>(MineResourceBehavior::S4LikeExhaustion));
    const MapPoint minePos = CreateMine(BuildingType::CoalMine, Resource(ResourceType::Coal, 15));

    SeedProductionRng(s4LikeComparisonSeed);
    RTTR_EXEC_TILL(s4LikeComparisonGFs, world.GetNode(minePos).resources.getAmount() == 14u);

    BOOST_TEST(world.GetNode(minePos).resources.getAmount() == 14u);
}

// A mine that actually produces its primary ware ignores the no-output fallback entirely (no stones), both for the
// S4-like and the default behavior. The two are structured identically and only differ in how the deposit is used.
BOOST_FIXTURE_TEST_CASE(CoalMineS4LikeSuccessfulCycleIgnoresNoOutputFallback, MineProductionFixture)
{
    ggs.setSelection(AddonId::COALMINE_RESOURCE_BEHAVIOR,
                     static_cast<unsigned>(MineResourceBehavior::S4LikeExhaustion));
    ggs.setSelection(AddonId::MINE_NO_OUTPUT_FALLBACK, static_cast<unsigned>(MineNoOutputFallback::ProduceGranite100));
    const MapPoint minePos = CreateMine(BuildingType::CoalMine, Resource(ResourceType::Coal, 15));
    const Inventory& curInventory = world.GetPlayer(curPlayer).GetInventory();
    const unsigned initialCoal = curInventory[GoodType::Coal];
    const unsigned initialStones = curInventory[GoodType::Stones];

    // Seed chosen so the very first cycle succeeds: otherwise a failed cycle would emit a fallback stone first.
    SeedProductionRng(21);
    RTTR_EXEC_TILL(maxProductionGFs, curInventory[GoodType::Coal] > initialCoal);

    BOOST_TEST(curInventory[GoodType::Stones] == initialStones);
    // S4-like consumes exactly one unit per successful cycle
    BOOST_TEST(world.GetNode(minePos).resources.getAmount() == 14u);
}

BOOST_FIXTURE_TEST_CASE(CoalMineDefaultProductionIgnoresNoOutputFallback, MineProductionFixture)
{
    ggs.setSelection(AddonId::MINE_NO_OUTPUT_FALLBACK, static_cast<unsigned>(MineNoOutputFallback::ProduceGranite100));
    const MapPoint minePos = CreateMine(BuildingType::CoalMine, Resource(ResourceType::Coal, 3));
    const Inventory& curInventory = world.GetPlayer(curPlayer).GetInventory();
    const unsigned initialCoal = curInventory[GoodType::Coal];
    const unsigned initialStones = curInventory[GoodType::Stones];

    // Default production is deterministic (always mines when a deposit is present), so no seed is needed.
    RTTR_EXEC_TILL(maxProductionGFs, curInventory[GoodType::Coal] > initialCoal);

    BOOST_TEST(curInventory[GoodType::Stones] == initialStones);
    // The default behavior depletes the deposit on every production
    BOOST_TEST(world.GetNode(minePos).resources.getAmount() < 3u);
}

BOOST_FIXTURE_TEST_CASE(CoalMineWorkEverywhereBehaviorProducesWithoutCreatingResource, MineProductionFixture)
{
    ggs.setSelection(AddonId::COALMINE_RESOURCE_BEHAVIOR, static_cast<unsigned>(MineResourceBehavior::WorkEverywhere));
    const MapPoint minePos = CreateMine(BuildingType::CoalMine);
    const Inventory& curInventory = world.GetPlayer(curPlayer).GetInventory();
    const unsigned initialCoal = curInventory[GoodType::Coal];

    RTTR_EXEC_TILL(maxProductionGFs, curInventory[GoodType::Coal] > initialCoal);
    BOOST_TEST(static_cast<unsigned>(world.GetNode(minePos).resources.getType())
               == static_cast<unsigned>(ResourceType::Nothing));
    BOOST_TEST(world.GetNode(minePos).resources.getAmount() == 0u);
}

// Regression test: the out-of-resources notification was moved from nofWorkman::FindPointWithResource into its
// callers, so every caller (not only mines) must still report it.
BOOST_FIXTURE_TEST_CASE(WellWithoutWaterReportsOutOfResources, WorldWithGCExecution1P)
{
    PostBox& postbox = world.GetPostMgr().AddPostBox(curPlayer);
    GoodsAndPeopleCounts inv;
    inv[GoodType::Fish] = 40;
    inv[Job::Helper] = 2;
    world.GetSpecObj<nobBaseWarehouse>(hqPos)->AddToInventory(inv, true);

    const MapPoint wellPos = hqPos + MapPoint(2, 0);
    const auto* well = static_cast<nobUsual*>(
      BuildingFactory::CreateBuilding(world, BuildingType::Well, wellPos, curPlayer, Nation::Romans));
    BuildRoad(world.GetNeighbour(wellPos, Direction::SouthEast), false, std::vector<Direction>(2, Direction::West));
    RTTR_EXEC_TILL(500, well->HasWorker());

    // The empty test world has no water resources at all, so the well must report that it dried out
    RTTR_EXEC_TILL(2000, postbox.GetNumMsgs() > 0u);
    BOOST_TEST(well->GetProductivity() == 0u);
}

BOOST_AUTO_TEST_SUITE_END()
