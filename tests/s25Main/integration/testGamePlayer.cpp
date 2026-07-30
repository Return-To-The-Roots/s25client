// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "GamePlayer.h"
#include "buildings/nobBaseWarehouse.h"
#include "buildings/nobHQ.h"
#include "buildings/nobMilitary.h"
#include "buildings/nobUsual.h"
#include "factories/BuildingFactory.h"
#include "figures/nofPassiveSoldier.h"
#include "ingameWindows/iwBuildingProductivities.h"
#include "worldFixtures/CreateEmptyWorld.h"
#include "worldFixtures/WorldFixture.h"
#include "gameTypes/MineResourceBehavior.h"
#include "gameTypes/Resource.h"
#include "gameData/BuildingProperties.h"
#include "gameData/GameConsts.h"
#include "rttr/test/random.hpp"
#include "s25util/warningSuppression.h"
#include <boost/test/unit_test.hpp>
#include <numeric>

using WorldFixtureEmpty2P = WorldFixture<CreateEmptyWorld, 2>;

BOOST_FIXTURE_TEST_CASE(Defeat, WorldFixtureEmpty2P)
{
    BOOST_TEST_REQUIRE(!world.GetPlayer(0).IsDefeated());
    BOOST_TEST_REQUIRE(!world.GetPlayer(1).IsDefeated());
    // Destroy HQ -> defeated
    world.DestroyNO(world.GetPlayer(1).GetHQPos()); //-V522
    BOOST_TEST_REQUIRE(!world.GetPlayer(0).IsDefeated());
    BOOST_TEST_REQUIRE(world.GetPlayer(1).IsDefeated());
    // Destroy HQ but leave a military bld
    MapPoint milBldPos = world.MakeMapPoint(world.GetPlayer(0).GetFirstWH()->GetPos() + Position(4, 0)); //-V522
    auto* milBld = dynamic_cast<nobMilitary*>(
      BuildingFactory::CreateBuilding(world, BuildingType::Watchtower, milBldPos, 0, Nation::Babylonians));
    auto& sld = world.AddFigure(milBldPos, std::make_unique<nofPassiveSoldier>(milBldPos, 0, milBld, milBld, 0));
    milBld->GotWorker(Job::Private, sld);
    sld.WalkToGoal();
    world.DestroyNO(world.GetPlayer(0).GetHQPos());
    BOOST_TEST_REQUIRE(!world.GetPlayer(0).IsDefeated());
    // Destroy this -> defeated
    world.DestroyNO(milBldPos);
    BOOST_TEST_REQUIRE(world.GetPlayer(0).IsDefeated());
}

namespace {
// Hack to access protected member for testing
struct SetProductivity : nobUsual
{
    using nobUsual::productivity;
};
RTTR_ATTRIBUTE_NO_UBSAN(vptr) void setProductivity(nobUsual* bld, unsigned short newProd)
{
    static_cast<SetProductivity*>(bld)->productivity = newProd;
}
} // namespace

using WorldFixtureEmpty1P = WorldFixture<CreateEmptyWorld, 1, 2 * helpers::MaxEnumValue_v<BuildingType> + 14, 4>;
using WorldFixtureMineRadius1P = WorldFixture<CreateEmptyWorld, 1, 20, 12>;

namespace {
// S4-like mine productivity reaches the mine's base productivity once this many matching resources remain in the
// mine radius, degrading linearly below it (see GetS4LikeMineProductionChance).
constexpr unsigned S4LIKE_FULL_PRODUCTIVITY_AMOUNT = 20;

// Places a coal mine and drives its S4-like productivity purely through the resources in its radius.
// The 20x12 map is larger than 2*MINER_RADIUS in each dimension, so the mine radius never wraps onto itself.
struct MineProductivityFixture : WorldFixtureMineRadius1P
{
    nobUsual* coalMine;
    MapPoint minePos;

    MineProductivityFixture()
    {
        // BuildingFactory::CreateBuilding ignores the building quality, so any node works; offset from the HQ keeps
        // the mine radius clear of the HQ. The empty world has no resources, but clear the radius to be explicit.
        minePos = world.MakeMapPoint(world.GetPlayer(0).GetHQPos() + Position(4, 0));
        coalMine = static_cast<nobUsual*>(
          BuildingFactory::CreateBuilding(world, BuildingType::CoalMine, minePos, 0, Nation::Romans));
        for(const MapPoint pt : world.GetPointsInRadiusWithCenter(minePos, MINER_RADIUS))
            world.SetResource(pt, Resource());
    }

    // Puts coal on the mine node and its eastern neighbor (0 == none), leaving the rest of the radius empty.
    void setCoalAmounts(const unsigned atMine, const unsigned atNeighbor)
    {
        world.SetResource(minePos, atMine ? Resource(ResourceType::Coal, atMine) : Resource());
        world.SetResource(world.GetNeighbour(minePos, Direction::East),
                          atNeighbor ? Resource(ResourceType::Coal, atNeighbor) : Resource());
    }

    // Spreads the given total coal amount as evenly as possible over every node in the radius (rest set to none).
    // Only the summed amount in range matters, so the exact distribution is irrelevant.
    void spreadCoalInRadius(const unsigned total)
    {
        const std::vector<MapPoint> pts = world.GetPointsInRadiusWithCenter(minePos, MINER_RADIUS);
        unsigned remaining = total;
        for(unsigned i = 0; i < pts.size(); ++i)
        {
            const unsigned here = remaining / (static_cast<unsigned>(pts.size()) - i);
            world.SetResource(pts[i], here ? Resource(ResourceType::Coal, here) : Resource());
            remaining -= here;
        }
    }
};
} // namespace

BOOST_FIXTURE_TEST_CASE(ProductivityStats, WorldFixtureEmpty1P)
{
    using boost::test_tools::per_element;
    const auto& buildingRegister = world.GetPlayer(0).GetBuildingRegister();
    helpers::EnumArray<unsigned short, BuildingType> expectedProductivity{};
    BOOST_TEST(buildingRegister.CalcProductivities() == expectedProductivity, per_element());
    BOOST_TEST(buildingRegister.CalcAverageProductivity() == 0u);

    const auto buildingTypesEnum = helpers::EnumRange<BuildingType>{};
    std::vector<BuildingType> buildingTypes(buildingTypesEnum.begin(), buildingTypesEnum.end());

    // Sort buildings so military buildings are created first
    // and no buildings are destroyed when borders are recalculated
    std::partition(buildingTypes.begin(), buildingTypes.end(),
                   [](BuildingType bld) { return !BuildingProperties::IsUsual(bld); });

    MapPoint curPos(0, 0);
    for(const auto bldType : buildingTypes)
    {
        if(!BuildingProperties::IsValid(bldType))
            continue;

        noBuilding* bld;
        if(bldType == BuildingType::Headquarters)
            bld = world.GetPlayer(0).GetFirstWH();
        else
        {
            // Size checks (in x) only for safety. Should never fail due to construction of map size
            while(world.GetNode(curPos).bq != BuildingQuality::Castle)
                BOOST_TEST_REQUIRE((++curPos.x) < world.GetSize().x);
            bld = BuildingFactory::CreateBuilding(world, bldType, curPos, 0, Nation::Babylonians);
            BOOST_TEST_REQUIRE((curPos.x += 2) < world.GetSize().x);
        }
        // Test productivity calculation for all buildings shown in the productivity window
        if(helpers::contains(iwBuildingProductivities::allIcons, bldType))
        {
            auto* productionBld = dynamic_cast<nobUsual*>(bld);
            BOOST_TEST_REQUIRE(productionBld);
            const auto productivity = rttr::test::randomValue(1, 100);
            setProductivity(productionBld, productivity);
            expectedProductivity[bldType] = productivity;
        }
    }
    BOOST_TEST(buildingRegister.CalcProductivities() == expectedProductivity, per_element());
    unsigned avgProd = std::accumulate(expectedProductivity.begin(), expectedProductivity.end(), 0u)
                       / iwBuildingProductivities::allIcons.size();
    BOOST_TEST(buildingRegister.CalcAverageProductivity() == avgProd);

    // Average productivity over multiple buildings of same type
    avgProd = 0;
    for(const BuildingType bldType : iwBuildingProductivities::allIcons)
    {
        auto* bld =
          static_cast<nobUsual*>(BuildingFactory::CreateBuilding(world, bldType, curPos, 0, Nation::Babylonians));
        if((curPos.x += 2) >= world.GetSize().x)
            curPos = MapPoint(0, curPos.y + 2);
        const auto productivity = rttr::test::randomValue(1, 100);
        setProductivity(bld, productivity);
        avgProd += productivity + expectedProductivity[bldType];
        expectedProductivity[bldType] = (productivity + expectedProductivity[bldType]) / 2;
    }
    avgProd /= iwBuildingProductivities::allIcons.size() * 2;
    BOOST_TEST(buildingRegister.CalcProductivities() == expectedProductivity, per_element());
    BOOST_TEST(buildingRegister.CalcAverageProductivity() == avgProd);
}

BOOST_FIXTURE_TEST_CASE(MineProductivityAccountsForS4LikeResourceChance, MineProductivityFixture)
{
    setProductivity(coalMine, 100);

    // Without the S4-like behavior the base productivity is reported unchanged, regardless of the resources left.
    setCoalAmounts(1, 0);
    BOOST_TEST(coalMine->GetProductivity() == 100u);

    ggs.setSelection(AddonId::COALMINE_RESOURCE_BEHAVIOR,
                     static_cast<unsigned>(MineResourceBehavior::S4LikeExhaustion));

    // S4-like scales productivity with the resources left, reaching the full base value at 20 (full productivity).
    setCoalAmounts(15, 5); // == S4LIKE_FULL_PRODUCTIVITY_AMOUNT
    BOOST_TEST(coalMine->GetProductivity() == 100u);
    // Halving the resources halves the reported productivity.
    setCoalAmounts(5, 5);
    BOOST_TEST(coalMine->GetProductivity() == 50u);
    // Lowering the base productivity scales the result by the same factor: 80% of the 50% chance -> 40%.
    setProductivity(coalMine, 80);
    BOOST_TEST(coalMine->GetProductivity() == 40u);
    BOOST_TEST(world.GetPlayer(0).GetBuildingRegister().CalcProductivities()[BuildingType::CoalMine] == 40u);
    // No resources left -> no production.
    setProductivity(coalMine, 100);
    setCoalAmounts(0, 0);
    BOOST_TEST(coalMine->GetProductivity() == 0u);

    // Inexhaustible mines always report their base productivity again, ignoring the resources.
    ggs.setSelection(AddonId::COALMINE_RESOURCE_BEHAVIOR, static_cast<unsigned>(MineResourceBehavior::Inexhaustible));
    BOOST_TEST(coalMine->GetProductivity() == 100u);
}

BOOST_FIXTURE_TEST_CASE(MineProductivityUsesAllMatchingResourcesWithinMineRadius, MineProductivityFixture)
{
    // Base productivity 100 so GetProductivity() directly mirrors the resource-based production chance.
    setProductivity(coalMine, 100);
    ggs.setSelection(AddonId::COALMINE_RESOURCE_BEHAVIOR,
                     static_cast<unsigned>(MineResourceBehavior::S4LikeExhaustion));

    // A different resource type in range and matching coal just outside the radius must not count as coal.
    world.SetResource(world.GetNeighbour(minePos, Direction::NorthWest), Resource(ResourceType::Iron, 15));
    const MapPoint outOfRangePt = world.GetNeighbour(
      world.GetNeighbour(world.GetNeighbour(minePos, Direction::East), Direction::East), Direction::East);
    const auto inRangePts = world.GetPointsInRadiusWithCenter(minePos, MINER_RADIUS);
    BOOST_TEST_REQUIRE(std::find(inRangePts.begin(), inRangePts.end(), outOfRangePt) == inRangePts.end());
    world.SetResource(outOfRangePt, Resource(ResourceType::Coal, 15));
    BOOST_TEST(GetRemainingMineResources(world, minePos, ResourceType::Coal) == 0u);
    BOOST_TEST(coalMine->GetProductivity() == 0u);

    // Half of the full amount, but spread across every node of the radius: productivity depends only on the sum in
    // range, not on how it is distributed. This overwrites the in-range iron, which no longer matters here.
    spreadCoalInRadius(S4LIKE_FULL_PRODUCTIVITY_AMOUNT / 2);
    BOOST_TEST(GetRemainingMineResources(world, minePos, ResourceType::Coal) == 10u);
    BOOST_TEST(coalMine->GetProductivity() == 50u);
    BOOST_TEST(world.GetPlayer(0).GetBuildingRegister().CalcProductivities()[BuildingType::CoalMine] == 50u);
}

BOOST_FIXTURE_TEST_CASE(IsHQTent_ReturnsFalse_IfPrimaryHQIsNotTent, WorldFixtureEmpty1P)
{
    GamePlayer& p1 = world.GetPlayer(0);

    // place another HQ that is a tent
    MapPoint newHqPos = p1.GetHQPos();
    newHqPos.x += 3;
    static_cast<nobHQ*>(
      BuildingFactory::CreateBuilding(world, BuildingType::Headquarters, newHqPos, 0, Nation::Babylonians))
      ->SetIsTent(true);

    BOOST_TEST_REQUIRE(p1.IsHQTent() == false);
}

BOOST_FIXTURE_TEST_CASE(IsHQTent_ReturnsTrue_IfPrimaryHQIsTent, WorldFixtureEmpty1P)
{
    GamePlayer& p1 = world.GetPlayer(0);
    p1.SetHQIsTent(true);

    // place another HQ that is not a tent
    MapPoint newHqPos = p1.GetHQPos();
    newHqPos.x += 3;
    static_cast<nobHQ*>(
      BuildingFactory::CreateBuilding(world, BuildingType::Headquarters, newHqPos, 0, Nation::Babylonians))
      ->SetIsTent(false);

    BOOST_TEST_REQUIRE(p1.IsHQTent() == true);
}
