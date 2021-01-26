// Copyright (c) 2016 - 2018 Settlers Freaks (sf-team at siedler25.org)
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

#include "BasePlayerInfo.h"
#include "JoinPlayerInfo.h"
#include "network/GameServer.h"
#include "gameTypes/GameTypesOutput.h"
#include "gameTypes/TeamTypes.h"
#include <boost/test/unit_test.hpp>

static unsigned getMaxTeamSizeDifference(const std::vector<JoinPlayerInfo>& playerInfos, unsigned nTeams)
{
    std::array<unsigned, NUM_TEAMS> nPlayers{};
    for(const auto& playerInfo : playerInfos)
    {
        switch(playerInfo.team)
        {
            case Team::Team1: ++nPlayers[0]; break;
            case Team::Team2: ++nPlayers[1]; break;
            case Team::Team3: ++nPlayers[2]; break;
            case Team::Team4: ++nPlayers[3]; break;
            default: BOOST_TEST_FAIL("Invalid team");
        };
    }

    std::sort(nPlayers.begin(), nPlayers.begin() + nTeams);
    BOOST_TEST_REQUIRE(nPlayers[0] <= nPlayers[nTeams - 1]);
    return nPlayers[nTeams - 1] - nPlayers[0];
}

BOOST_AUTO_TEST_CASE(JoinPlayerAssignment)
{
    JoinPlayerInfo BPI1, BPI2, BPI3, BPI4, BPI1_2, BPI1_3, BPI1_4, BPI_Rand;
    BPI1.team = Team::Team1;
    BPI2.team = Team::Team2;
    BPI3.team = Team::Team3;
    BPI4.team = Team::Team4;
    BPI_Rand.team = Team::Random;
    BPI1_2.team = Team::Random1To2;
    BPI1_3.team = Team::Random1To3;
    BPI1_4.team = Team::Random1To4;

    std::vector<JoinPlayerInfo> playerInfos;

    // No change
    playerInfos = {BPI1, BPI2, BPI3, BPI4};
    BOOST_REQUIRE(!GameServer::assignPlayersOfRandomTeams(playerInfos));
    BOOST_TEST(playerInfos[0].team == Team::Team1);
    BOOST_TEST(playerInfos[1].team == Team::Team2);
    BOOST_TEST(playerInfos[2].team == Team::Team3);
    BOOST_TEST(playerInfos[3].team == Team::Team4);

    // (only) random team gets changed
    playerInfos.push_back(BPI_Rand);
    BOOST_REQUIRE(GameServer::assignPlayersOfRandomTeams(playerInfos));
    BOOST_TEST(playerInfos[0].team == Team::Team1);
    BOOST_TEST(playerInfos[1].team == Team::Team2);
    BOOST_TEST(playerInfos[2].team == Team::Team3);
    BOOST_TEST(playerInfos[3].team == Team::Team4);
    BOOST_TEST(isTeam(playerInfos[4].team));

    // Assigned teams are as selected
    playerInfos = {BPI1, BPI2, BPI3, BPI4, BPI_Rand, BPI1_2, BPI1_3, BPI1_4};
    BOOST_REQUIRE(GameServer::assignPlayersOfRandomTeams(playerInfos));
    BOOST_TEST(playerInfos[0].team == Team::Team1);
    BOOST_TEST(playerInfos[1].team == Team::Team2);
    BOOST_TEST(playerInfos[2].team == Team::Team3);
    BOOST_TEST(playerInfos[3].team == Team::Team4);
    BOOST_TEST(isTeam(playerInfos[4].team));
    BOOST_TEST((playerInfos[5].team == Team::Team1 || playerInfos[5].team == Team::Team2));
    BOOST_TEST(
      (playerInfos[6].team == Team::Team1 || playerInfos[6].team == Team::Team2 || playerInfos[6].team == Team::Team3));
    BOOST_TEST(isTeam(playerInfos[7].team));

    // Sanity check for getMaxTeamSizeDifference: team 2 is empty, 1 & 3 have 2 players
    playerInfos = {BPI3, BPI3, BPI1, BPI1};
    BOOST_REQUIRE(!GameServer::assignPlayersOfRandomTeams(playerInfos));
    BOOST_TEST(getMaxTeamSizeDifference(playerInfos, 3) == 2u);

    // 1 player each to team 3 and 4
    playerInfos = {BPI1_4, BPI1, BPI2, BPI1_4};
    BOOST_REQUIRE(GameServer::assignPlayersOfRandomTeams(playerInfos));
    BOOST_TEST(getMaxTeamSizeDifference(playerInfos, 4) == 0u);

    // 1 player to team 3 the other to any of 1-3
    playerInfos = {BPI1_3, BPI1, BPI2, BPI1_3};
    BOOST_REQUIRE(GameServer::assignPlayersOfRandomTeams(playerInfos));
    BOOST_TEST(getMaxTeamSizeDifference(playerInfos, 3) == 1u);

    // If BPI1_4 gets into team 3 then team 4 will be empty, else all will be even
    playerInfos = {BPI1_4, BPI1, BPI2, BPI1_3};
    BOOST_REQUIRE(GameServer::assignPlayersOfRandomTeams(playerInfos));
    BOOST_TEST(getMaxTeamSizeDifference(playerInfos, 4) <= 2u);

    // Randomly distribute all evenly
    playerInfos = {BPI1_4, BPI1_4, BPI1_4, BPI1_4, BPI1_4, BPI1_4, BPI1_4, BPI1_4};
    BOOST_REQUIRE(GameServer::assignPlayersOfRandomTeams(playerInfos));
    BOOST_TEST(getMaxTeamSizeDifference(playerInfos, 4) == 0u);

    // Worst case: First 3 into team 1-3 so team 4 will be empty
    playerInfos = {BPI1_4, BPI1_4, BPI1_4, BPI1_3, BPI1_3, BPI1_3, BPI1_3, BPI1_3, BPI1_3};
    BOOST_REQUIRE(GameServer::assignPlayersOfRandomTeams(playerInfos));
    BOOST_TEST(getMaxTeamSizeDifference(playerInfos, 4) <= 2u);

    // Both randoms will be in team 2
    playerInfos = {BPI3, BPI3, BPI1_2, BPI1_2, BPI1, BPI1};
    BOOST_REQUIRE(GameServer::assignPlayersOfRandomTeams(playerInfos));
    BOOST_TEST(getMaxTeamSizeDifference(playerInfos, 3) == 0u);
}
