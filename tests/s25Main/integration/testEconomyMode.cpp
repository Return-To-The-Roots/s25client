// Copyright (c) 2016 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#include "EconomyModeHandler.h"
#include "EventManager.h"
#include "GamePlayer.h"
#include "Savegame.h"
#include "SerializedGameData.h"
#include "buildings/nobBaseWarehouse.h"
#include "worldFixtures/MockLocalGameState.h"
#include "worldFixtures/WorldFixture.h"
#include "worldFixtures/WorldWithGCExecution.h"
#include "world/GameWorld.h"
#include "gameTypes/GO_Type.h"
#include "s25util/tmpFile.h"
#include <boost/range/combine.hpp>
#include <boost/test/unit_test.hpp>
#include <helpers/make_array.h>

BOOST_AUTO_TEST_SUITE(EconomyModeTestSuite)

namespace {

BOOST_FIXTURE_TEST_CASE(EconomyMode3Players, WorldWithGCExecution3P)
{
    constexpr auto amountsToAdd = helpers::make_array(63, 100, 85);

    RTTR_Assert(world.GetNumPlayers() == 3);
    world.GetPlayer(0).team = TM_TEAM2;
    world.GetPlayer(1).team = TM_TEAM1;
    world.GetPlayer(2).team = TM_TEAM2;
    std::array<MapPoint, 3> hqPos;
    for(unsigned playerIdx = 0; playerIdx < 3; ++playerIdx)
    {
        world.GetPlayer(playerIdx).MakeStartPacts();
        hqPos[playerIdx] = world.GetPlayer(playerIdx).GetHQPos();
    }
    this->ggs.objective = GameObjective::EconomyMode;

    world.econHandler = std::make_unique<EconomyModeHandler>(4);
    const auto goodsToCollect = world.econHandler->GetGoodTypesToCollect();

    BOOST_REQUIRE(!goodsToCollect.empty());
    BOOST_REQUIRE_EQUAL(world.econHandler->GetEndFrame(), 4u);
    BOOST_REQUIRE(!world.econHandler->isOver());
    BOOST_REQUIRE(!world.econHandler->isInfinite());
    BOOST_REQUIRE(world.GetEvMgr().ObjectHasEvents(*world.econHandler));

    world.GetEvMgr().ExecuteNextGF();
    world.econHandler->UpdateAmounts();

    // Individual amount counting
    unsigned initialAmount = world.econHandler->GetAmount(0, 0);
    BOOST_REQUIRE_EQUAL(initialAmount, world.econHandler->GetAmount(0, 1));
    BOOST_REQUIRE_EQUAL(initialAmount, world.econHandler->GetAmount(0, 2));

    for(unsigned playerIdx = 0; playerIdx < 3; ++playerIdx)
    {
        Inventory inv;
        inv.Add(goodsToCollect[0], amountsToAdd[playerIdx]);
        world.GetSpecObj<nobBaseWarehouse>(hqPos[playerIdx])->AddGoods(inv, true);
    }
    world.GetEvMgr().ExecuteNextGF();
    world.econHandler->UpdateAmounts();
    for(unsigned playerIdx = 0; playerIdx < 3; ++playerIdx)
    {
        BOOST_REQUIRE_EQUAL(world.econHandler->GetAmount(0, playerIdx), initialAmount + amountsToAdd[playerIdx]);
    }

    // Teams behaviour
    const auto econTeams = world.econHandler->GetTeams();

    // People get assigned to the correct teams?
    BOOST_REQUIRE_EQUAL(econTeams.size(), 2u);
    unsigned smallTeam = econTeams[0].playersInTeam.count() - 1;
    unsigned bigTeam = 2 - econTeams[0].playersInTeam.count();
    BOOST_REQUIRE_EQUAL(econTeams[smallTeam].playersInTeam.count(), 1u);
    BOOST_REQUIRE_EQUAL(econTeams[bigTeam].playersInTeam.count(), 2u);
    BOOST_REQUIRE(econTeams[smallTeam].containsPlayer(1));
    BOOST_REQUIRE(econTeams[bigTeam].containsPlayer(0));
    BOOST_REQUIRE(econTeams[bigTeam].containsPlayer(2));

    // Team amounts get calculated correctly?
    BOOST_REQUIRE_EQUAL(econTeams[smallTeam].amountsTheTeamCollected[0], initialAmount + amountsToAdd[1]);
    BOOST_REQUIRE_EQUAL(econTeams[bigTeam].amountsTheTeamCollected[0],
                        2 * initialAmount + amountsToAdd[0] + amountsToAdd[2]);
    BOOST_REQUIRE(econTeams[smallTeam].goodTypeWins < econTeams[bigTeam].goodTypeWins);
    BOOST_REQUIRE_EQUAL(world.econHandler->GetMaxTeamAmount(0), econTeams[bigTeam].amountsTheTeamCollected[0]);

    // Serialization/Deserialization
    Savegame save;
    for(unsigned i = 0; i < world.GetNumPlayers(); i++)
        save.AddPlayer(world.GetPlayer(i));
    save.ggs = ggs;
    save.start_gf = em.GetCurrentGF();
    save.sgd.MakeSnapshot(game);
    TmpFile tmpFile;
    BOOST_TEST_REQUIRE(save.Save(tmpFile.filePath, "MapTitle"));
    Savegame loadSave;
    BOOST_TEST_REQUIRE(loadSave.Load(tmpFile.filePath, SaveGameDataToLoad::All));
    BOOST_TEST_REQUIRE(loadSave.GetNumPlayers() == 3u);
    std::vector<PlayerInfo> players;
    for(unsigned j = 0; j < 3; j++)
        players.push_back(PlayerInfo(loadSave.GetPlayer(j)));
    auto sharedGame = std::make_shared<Game>(save.ggs, loadSave.start_gf, players);
    GameWorld& newWorld = sharedGame->world_;
    MockLocalGameState localGameState;
    save.sgd.ReadSnapshot(sharedGame, localGameState);
    BOOST_REQUIRE(newWorld.econHandler);
    const auto& goodsToCollectAfter = newWorld.econHandler->GetGoodTypesToCollect();
    const auto& econTeamsAfter = newWorld.econHandler->GetTeams();
    BOOST_REQUIRE(goodsToCollect == goodsToCollectAfter);
    BOOST_REQUIRE_EQUAL(econTeams.size(), econTeamsAfter.size());
    newWorld.GetEvMgr().ExecuteNextGF();
    newWorld.econHandler->UpdateAmounts();
    for(const auto& teamPair : boost::combine(econTeams, econTeamsAfter))
    {
        const EconomyModeHandler::EconTeam& before = boost::get<0>(teamPair);
        const EconomyModeHandler::EconTeam& after = boost::get<1>(teamPair);
        BOOST_REQUIRE_EQUAL(before.playersInTeam, after.playersInTeam);
        BOOST_REQUIRE(before.amountsTheTeamCollected == after.amountsTheTeamCollected);
        BOOST_REQUIRE_EQUAL(before.goodTypeWins, after.goodTypeWins);
    }
    BOOST_REQUIRE(newWorld.GetEvMgr().ObjectHasEvents(*newWorld.econHandler));
}

} // namespace

BOOST_AUTO_TEST_SUITE_END()
