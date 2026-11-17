// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
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
        world.getPlayer(0).team = Team::Team2;
        world.getPlayer(1).team = Team::Team1;
        world.getPlayer(2).team = Team::Team2;
        for(unsigned playerIdx = 0; playerIdx < 3; ++playerIdx)
        {
            world.getPlayer(playerIdx).makeStartPacts();
            hqPos[playerIdx] = world.getPlayer(playerIdx).getHQPos();
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
    game->start(false);
    BOOST_TEST(world.getEconHandler()); // Just test that we have any

    constexpr auto amountsToAdd = helpers::make_array(63, 100, 85);

    EconomyModeHandler econHandler(4);
    const auto& goodsToCollect = econHandler.getGoodTypesToCollect();

    BOOST_TEST_REQUIRE(!goodsToCollect.empty());
    BOOST_TEST(econHandler.getEndFrame() == 4u);
    BOOST_TEST(!econHandler.isOver());
    BOOST_TEST(!econHandler.isInfinite());
    BOOST_TEST(world.getEvMgr().objectHasEvents(econHandler));
    world.getEvMgr().executeNextGF();
    econHandler.updateAmounts();

    // Individual amount counting
    unsigned initialAmount = econHandler.getAmount(0, 0);
    BOOST_TEST_REQUIRE(initialAmount == econHandler.getAmount(0, 1));
    BOOST_TEST_REQUIRE(initialAmount == econHandler.getAmount(0, 2));

    for(unsigned playerIdx = 0; playerIdx < 3; ++playerIdx)
    {
        world.getSpecObj<nobBaseWarehouse>(hqPos[playerIdx])
          ->addToInventory(GoodCounts::make(goodsToCollect[0], amountsToAdd[playerIdx]), true);
    }
    world.getEvMgr().executeNextGF();
    econHandler.updateAmounts();
    for(unsigned playerIdx = 0; playerIdx < 3; ++playerIdx)
    {
        BOOST_TEST_REQUIRE(econHandler.getAmount(0, playerIdx) == initialAmount + amountsToAdd[playerIdx]);
    }

    // Teams behaviour
    const auto& econTeams = econHandler.getTeams();

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
    BOOST_TEST(econHandler.getMaxTeamAmount(0) == econTeams[bigTeam].amountsTheTeamCollected[0]);
}

BOOST_FIXTURE_TEST_CASE(EconomyModeSerialization, EconModeFixture)
{
    ggs.setSelection(AddonId::ECONOMY_MODE_GAME_LENGTH,
                     rttr::test::randomValue<unsigned>(1, AddonEconomyModeGameLengthList.size() - 1));
    game->start(false);
    const auto& goodsToCollect = world.getEconHandler()->getGoodTypesToCollect();
    for(unsigned playerIdx = 0; playerIdx < 3; ++playerIdx)
    {
        world.getSpecObj<nobBaseWarehouse>(hqPos[playerIdx])
          ->addToInventory(GoodCounts::make(goodsToCollect[0], rttr::test::randomValue(1u, 50u)), true);
    }
    world.getEvMgr().executeNextGF();
    world.getEconHandler()->updateAmounts();

    Savegame save;
    for(const auto& player : world.getPlayers())
        save.addPlayer(player);
    save.ggs = ggs;
    save.start_gf = game->em_->getCurrentGF();
    save.sgd.makeSnapshot(*game);
    TmpFile tmpFile;
    BOOST_TEST_REQUIRE(save.save(tmpFile.filePath, "MapTitle"));
    Savegame loadSave;
    BOOST_TEST_REQUIRE(loadSave.Load(tmpFile.filePath, SaveGameDataToLoad::All));
    BOOST_TEST_REQUIRE(loadSave.getNumPlayers() == 3u);
    std::vector<PlayerInfo> players;
    for(unsigned j = 0; j < 3; j++)
        players.push_back(PlayerInfo(loadSave.getPlayer(j)));
    Game newGame(save.ggs, loadSave.start_gf, players);
    GameWorld& newWorld = newGame.world_;
    MockLocalGameState localGameState;
    save.sgd.readSnapshot(newGame, localGameState);

    BOOST_TEST_REQUIRE(newWorld.getEconHandler());
    BOOST_TEST(newWorld.getEconHandler()->getEndFrame() == world.getEconHandler()->getEndFrame());
    const auto& goodsToCollectAfter = newWorld.getEconHandler()->getGoodTypesToCollect();
    BOOST_TEST(goodsToCollect == goodsToCollectAfter);
    const auto& econTeams = world.getEconHandler()->getTeams();
    const auto& econTeamsAfter = newWorld.getEconHandler()->getTeams();
    BOOST_TEST_REQUIRE(econTeams.size() == econTeamsAfter.size());
    newWorld.getEconHandler()->updateAmounts();
    for(const auto& teamPair : boost::combine(econTeams, econTeamsAfter))
    {
        const EconomyModeHandler::EconTeam& before = get<0>(teamPair);
        const EconomyModeHandler::EconTeam& after = get<1>(teamPair);
        BOOST_TEST_REQUIRE(before.playersInTeam == after.playersInTeam);
        BOOST_TEST_REQUIRE(before.amountsTheTeamCollected == after.amountsTheTeamCollected);
        BOOST_TEST_REQUIRE(before.goodTypeWins == after.goodTypeWins);
    }
    BOOST_TEST(newWorld.getEvMgr().objectHasEvents(*newWorld.getEconHandler()));
}

BOOST_AUTO_TEST_SUITE_END()
