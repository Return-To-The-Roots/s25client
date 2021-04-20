// Copyright (c) 2016 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "EconomyModeHandler.h"
#include "EventManager.h"
#include "GamePlayer.h"
#include "Savegame.h"
#include "SerializedGameData.h"
#include "addons/AddonEconomyModeGameLength.h"
#include "buildings/nobBaseWarehouse.h"
#include "worldFixtures/MockLocalGameState.h"
#include "worldFixtures/WorldFixture.h"
#include "worldFixtures/WorldWithGCExecution.h"
#include "world/GameWorld.h"
#include "gameTypes/GO_Type.h"
#include "s25util/tmpFile.h"
#include <rttr/test/random.hpp>
#include <boost/range/combine.hpp>
#include <boost/test/unit_test.hpp>
#include <helpers/make_array.h>

BOOST_AUTO_TEST_SUITE(EconomyModeTestSuite)

namespace {
class EconModeFixture : WorldFixture<CreateEmptyWorld, 3>
{
    using Parent = WorldFixture<CreateEmptyWorld, 3>;

public:
    EconModeFixture()
    {
        world.GetPlayer(0).team = Team::Team2;
        world.GetPlayer(1).team = Team::Team1;
        world.GetPlayer(2).team = Team::Team2;
        for(unsigned playerIdx = 0; playerIdx < 3; ++playerIdx)
        {
            world.GetPlayer(playerIdx).MakeStartPacts();
            hqPos[playerIdx] = world.GetPlayer(playerIdx).GetHQPos();
        }
        this->ggs.objective = GameObjective::EconomyMode;
    }

    using Parent::game;
    using Parent::ggs;
    using Parent::world;
    std::array<MapPoint, 3> hqPos;
};
} // namespace

BOOST_FIXTURE_TEST_CASE(EconomyMode3Players, EconModeFixture)
{
    game->Start(false);
    BOOST_TEST(world.getEconHandler()); // Just test that we have any

    constexpr auto amountsToAdd = helpers::make_array(63, 100, 85);

    EconomyModeHandler econHandler(4);
    const auto& goodsToCollect = econHandler.GetGoodTypesToCollect();

    BOOST_TEST_REQUIRE(!goodsToCollect.empty());
    BOOST_TEST(econHandler.GetEndFrame() == 4u);
    BOOST_TEST(!econHandler.isOver());
    BOOST_TEST(!econHandler.isInfinite());
    BOOST_TEST(world.GetEvMgr().ObjectHasEvents(econHandler));
    world.GetEvMgr().ExecuteNextGF();
    econHandler.UpdateAmounts();

    // Individual amount counting
    unsigned initialAmount = econHandler.GetAmount(0, 0);
    BOOST_TEST_REQUIRE(initialAmount == econHandler.GetAmount(0, 1));
    BOOST_TEST_REQUIRE(initialAmount == econHandler.GetAmount(0, 2));

    for(unsigned playerIdx = 0; playerIdx < 3; ++playerIdx)
    {
        Inventory inv;
        inv.Add(goodsToCollect[0], amountsToAdd[playerIdx]);
        world.GetSpecObj<nobBaseWarehouse>(hqPos[playerIdx])->AddGoods(inv, true);
    }
    world.GetEvMgr().ExecuteNextGF();
    econHandler.UpdateAmounts();
    for(unsigned playerIdx = 0; playerIdx < 3; ++playerIdx)
    {
        BOOST_TEST_REQUIRE(econHandler.GetAmount(0, playerIdx) == initialAmount + amountsToAdd[playerIdx]);
    }

    // Teams behaviour
    const auto& econTeams = econHandler.GetTeams();

    // People get assigned to the correct teams?
    BOOST_TEST_REQUIRE(econTeams.size() == 2u);
    unsigned smallTeam = econTeams[0].playersInTeam.count() - 1;
    unsigned bigTeam = 2 - econTeams[0].playersInTeam.count();
    BOOST_TEST_REQUIRE(econTeams[smallTeam].playersInTeam.count() == 1u);
    BOOST_TEST_REQUIRE(econTeams[bigTeam].playersInTeam.count() == 2u);
    BOOST_TEST(econTeams[smallTeam].containsPlayer(1));
    BOOST_TEST(econTeams[bigTeam].containsPlayer(0));
    BOOST_TEST(econTeams[bigTeam].containsPlayer(2));

    // Team amounts get calculated correctly?
    BOOST_TEST(econTeams[smallTeam].amountsTheTeamCollected[0] == initialAmount + amountsToAdd[1]);
    BOOST_TEST(econTeams[bigTeam].amountsTheTeamCollected[0] == 2 * initialAmount + amountsToAdd[0] + amountsToAdd[2]);
    BOOST_TEST(econTeams[smallTeam].goodTypeWins < econTeams[bigTeam].goodTypeWins);
    BOOST_TEST(econHandler.GetMaxTeamAmount(0) == econTeams[bigTeam].amountsTheTeamCollected[0]);
}

BOOST_FIXTURE_TEST_CASE(EconomyModeSerialization, EconModeFixture)
{
    ggs.setSelection(AddonId::ECONOMY_MODE_GAME_LENGTH,
                     rttr::test::randomValue<unsigned>(1, AddonEconomyModeGameLengthList.size() - 1));
    game->Start(false);
    const auto& goodsToCollect = world.getEconHandler()->GetGoodTypesToCollect();
    for(unsigned playerIdx = 0; playerIdx < 3; ++playerIdx)
    {
        Inventory inv;
        inv.Add(goodsToCollect[0], rttr::test::randomValue(1u, 50u));
        world.GetSpecObj<nobBaseWarehouse>(hqPos[playerIdx])->AddGoods(inv, true);
    }
    world.GetEvMgr().ExecuteNextGF();
    world.getEconHandler()->UpdateAmounts();

    Savegame save;
    for(unsigned i = 0; i < world.GetNumPlayers(); i++)
        save.AddPlayer(world.GetPlayer(i));
    save.ggs = ggs;
    save.start_gf = game->em_->GetCurrentGF();
    save.sgd.MakeSnapshot(*game);
    TmpFile tmpFile;
    BOOST_TEST_REQUIRE(save.Save(tmpFile.filePath, "MapTitle"));
    Savegame loadSave;
    BOOST_TEST_REQUIRE(loadSave.Load(tmpFile.filePath, SaveGameDataToLoad::All));
    BOOST_TEST_REQUIRE(loadSave.GetNumPlayers() == 3u);
    std::vector<PlayerInfo> players;
    for(unsigned j = 0; j < 3; j++)
        players.push_back(PlayerInfo(loadSave.GetPlayer(j)));
    Game newGame(save.ggs, loadSave.start_gf, players);
    GameWorld& newWorld = newGame.world_;
    MockLocalGameState localGameState;
    save.sgd.ReadSnapshot(newGame, localGameState);

    BOOST_TEST_REQUIRE(newWorld.getEconHandler());
    BOOST_TEST(newWorld.getEconHandler()->GetEndFrame() == world.getEconHandler()->GetEndFrame());
    const auto& goodsToCollectAfter = newWorld.getEconHandler()->GetGoodTypesToCollect();
    BOOST_TEST(goodsToCollect == goodsToCollectAfter);
    const auto& econTeams = world.getEconHandler()->GetTeams();
    const auto& econTeamsAfter = newWorld.getEconHandler()->GetTeams();
    BOOST_TEST_REQUIRE(econTeams.size() == econTeamsAfter.size());
    newWorld.getEconHandler()->UpdateAmounts();
    for(const auto& teamPair : boost::combine(econTeams, econTeamsAfter))
    {
        const EconomyModeHandler::EconTeam& before = boost::get<0>(teamPair);
        const EconomyModeHandler::EconTeam& after = boost::get<1>(teamPair);
        BOOST_TEST_REQUIRE(before.playersInTeam == after.playersInTeam);
        BOOST_TEST_REQUIRE(before.amountsTheTeamCollected == after.amountsTheTeamCollected);
        BOOST_TEST_REQUIRE(before.goodTypeWins == after.goodTypeWins);
    }
    BOOST_TEST(newWorld.GetEvMgr().ObjectHasEvents(*newWorld.getEconHandler()));
}

BOOST_AUTO_TEST_SUITE_END()
