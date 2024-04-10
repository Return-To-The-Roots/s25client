// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "GamePlayer.h"
#include "buildings/nobBaseWarehouse.h"
#include "buildings/nobMilitary.h"
#include "buildings/nobUsual.h"
#include "factories/BuildingFactory.h"
#include "figures/nofPassiveSoldier.h"
#include "ingameWindows/iwBuildingProductivities.h"
#include "worldFixtures/CreateEmptyWorld.h"
#include "worldFixtures/WorldFixture.h"
#include "gameData/BuildingProperties.h"
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

using WorldFixtureEmpty1P = WorldFixture<CreateEmptyWorld, 1, 2 * helpers::MaxEnumValue_v<BuildingType>, 4>;
BOOST_FIXTURE_TEST_CASE(ProductivityStats, WorldFixtureEmpty1P)
{
    using boost::test_tools::per_element;
    const auto& buildingRegister = world.GetPlayer(0).GetBuildingRegister();
    helpers::EnumArray<unsigned short, BuildingType> expectedProductivity{};
    BOOST_TEST(buildingRegister.CalcProductivities() == expectedProductivity, per_element());
    BOOST_TEST(buildingRegister.CalcAverageProductivity() == 0u);

    std::vector<BuildingType> buildingTypes;
    const auto buildingTypesEnum = helpers::EnumRange<BuildingType>{};

    for(auto it = buildingTypesEnum.begin(); it != buildingTypesEnum.end(); ++it)
    {
        buildingTypes.push_back(*it);
    }

    // Sort buildings so military buildings are created first 
    // and no buildings are destroyed when borders are recalculated
    std::sort(buildingTypes.begin(), buildingTypes.end(), [](BuildingType a, BuildingType b) {
        return !BuildingProperties::IsUsual(a) > !BuildingProperties::IsUsual(b);
    });

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
        if(helpers::contains(iwBuildingProductivities::icons, bldType))
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
                       / iwBuildingProductivities::icons.size();
    BOOST_TEST(buildingRegister.CalcAverageProductivity() == avgProd);

    // Average productivity over multiple buildings of same type
    avgProd = 0;
    for(const BuildingType bldType : iwBuildingProductivities::icons)
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
    avgProd /= iwBuildingProductivities::icons.size() * 2;
    BOOST_TEST(buildingRegister.CalcProductivities() == expectedProductivity, per_element());
    BOOST_TEST(buildingRegister.CalcAverageProductivity() == avgProd);
}
