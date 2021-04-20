// Copyright (C) 2016 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
            default: BOOST_TEST_FAIL("Invalid team"); // LCOV_EXCL_LINE
        };
    }

    const auto itMinMax = std::minmax_element(nPlayers.begin(), nPlayers.begin() + nTeams);
    BOOST_TEST_REQUIRE(*itMinMax.first <= *itMinMax.second);
    return *itMinMax.second - *itMinMax.first;
}

BOOST_AUTO_TEST_CASE(JoinPlayerAssignment)
{
    JoinPlayerInfo BPI1, BPI2, BPI3, BPI4, BPI1_2, BPI1_3, BPI1_4, BPI_Rand, BPI_None;
    BPI1.team = Team::Team1;
    BPI2.team = Team::Team2;
    BPI3.team = Team::Team3;
    BPI4.team = Team::Team4;
    BPI_Rand.team = Team::Random;
    BPI1_2.team = Team::Random1To2;
    BPI1_3.team = Team::Random1To3;
    BPI1_4.team = Team::Random1To4;
    BPI_None.team = Team::None;

    std::vector<JoinPlayerInfo> playerInfos;

    // Sanity check for getMaxTeamSizeDifference: team 2 is empty, 1 & 3 have 2 players
    playerInfos = {BPI3, BPI3, BPI1, BPI1};
    BOOST_TEST(getMaxTeamSizeDifference(playerInfos, 1) == 0u);
    BOOST_TEST(getMaxTeamSizeDifference(playerInfos, 3) == 2u);
    BOOST_TEST(getMaxTeamSizeDifference(playerInfos, 4) == 2u);

    // No change
    playerInfos = {BPI1, BPI2, BPI3, BPI4, BPI_None};
    BOOST_REQUIRE(!GameServer::assignPlayersOfRandomTeams(playerInfos));
    BOOST_TEST(playerInfos[0].team == Team::Team1);
    BOOST_TEST(playerInfos[1].team == Team::Team2);
    BOOST_TEST(playerInfos[2].team == Team::Team3);
    BOOST_TEST(playerInfos[3].team == Team::Team4);
    BOOST_TEST(playerInfos[4].team == Team::None);

    // (only) random team gets changed
    playerInfos.push_back(BPI_Rand);
    BOOST_REQUIRE(GameServer::assignPlayersOfRandomTeams(playerInfos));
    BOOST_TEST(playerInfos[0].team == Team::Team1);
    BOOST_TEST(playerInfos[1].team == Team::Team2);
    BOOST_TEST(playerInfos[2].team == Team::Team3);
    BOOST_TEST(playerInfos[3].team == Team::Team4);
    BOOST_TEST(playerInfos[4].team == Team::None);
    BOOST_TEST(isTeam(playerInfos[5].team));

    // Assigned teams are as selected, teams are even
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
    BOOST_TEST(getMaxTeamSizeDifference(playerInfos, 4) == 0u);

    // 1 player each to team 3 and 4
    playerInfos = {BPI1_4, BPI1, BPI2, BPI1_4};
    BOOST_REQUIRE(GameServer::assignPlayersOfRandomTeams(playerInfos));
    BOOST_TEST(getMaxTeamSizeDifference(playerInfos, 4) == 0u);

    // 1 player to team 3 the other to any of 1-3
    playerInfos = {BPI1_3, BPI1, BPI2, BPI1_3};
    BOOST_REQUIRE(GameServer::assignPlayersOfRandomTeams(playerInfos));
    BOOST_TEST(getMaxTeamSizeDifference(playerInfos, 3) == 1u);

    // BPI1_3 has to go into team 3, and then BPI1_4 into team 4
    playerInfos = {BPI1_4, BPI1, BPI2, BPI1_3};
    BOOST_REQUIRE(GameServer::assignPlayersOfRandomTeams(playerInfos));
    BOOST_TEST(playerInfos[0].team == Team::Team4);
    BOOST_TEST(playerInfos[1].team == Team::Team1);
    BOOST_TEST(playerInfos[2].team == Team::Team2);
    BOOST_TEST(playerInfos[3].team == Team::Team3);
    BOOST_TEST(getMaxTeamSizeDifference(playerInfos, 4) == 0u);

    // Randomly distribute all evenly
    playerInfos = {BPI1_4, BPI1_4, BPI1_4, BPI1_4, BPI1_4, BPI1_4, BPI1_4, BPI1_4};
    BOOST_REQUIRE(GameServer::assignPlayersOfRandomTeams(playerInfos));
    BOOST_TEST(getMaxTeamSizeDifference(playerInfos, 4) == 0u);

    // First the teams 1-3 are filled evenly, then the other 2 are put into team 4
    playerInfos = {BPI1_4, BPI1_4, BPI1_3, BPI1_3, BPI1_3, BPI1_3, BPI1_3, BPI1_3};
    BOOST_REQUIRE(GameServer::assignPlayersOfRandomTeams(playerInfos));
    BOOST_TEST(playerInfos[0].team == Team::Team4);
    BOOST_TEST(playerInfos[1].team == Team::Team4);
    BOOST_TEST(getMaxTeamSizeDifference(playerInfos, 4) == 0u);

    // Both randoms will be in team 2
    playerInfos = {BPI3, BPI3, BPI1_2, BPI1_2, BPI1, BPI1};
    BOOST_REQUIRE(GameServer::assignPlayersOfRandomTeams(playerInfos));
    BOOST_TEST(playerInfos[0].team == Team::Team3);
    BOOST_TEST(playerInfos[1].team == Team::Team3);
    BOOST_TEST(playerInfos[2].team == Team::Team2);
    BOOST_TEST(playerInfos[3].team == Team::Team2);
    BOOST_TEST(playerInfos[4].team == Team::Team1);
    BOOST_TEST(playerInfos[5].team == Team::Team1);
    BOOST_TEST(getMaxTeamSizeDifference(playerInfos, 3) == 0u);

    // Uneven distribution: Team 3 doesn't has any player yet, but team 1-2 have
    // --> First BPI1_2 goes into team 2, the second to 1 or 2 making it 3 players, while the others has 2. Team 3 will
    // only get exactly 1
    playerInfos = {BPI1_3, BPI2, BPI1_2, BPI1_2, BPI1, BPI1};
    BOOST_REQUIRE(GameServer::assignPlayersOfRandomTeams(playerInfos));
    BOOST_TEST(playerInfos[0].team == Team::Team3);
    BOOST_TEST(playerInfos[1].team == Team::Team2);
    BOOST_TEST(playerInfos[4].team == Team::Team1);
    BOOST_TEST(playerInfos[5].team == Team::Team1);
    BOOST_TEST(getMaxTeamSizeDifference(playerInfos, 3) == 2u);

    {
        std::array<helpers::EnumArray<bool, Team>, 5> playerWasAssignedTeam{};
        const auto playerWasInTeams = [&playerWasAssignedTeam](unsigned player, std::initializer_list<Team> teams) {
            for(const Team t : teams)
            {
                if(!playerWasAssignedTeam[player][t])
                    return false;
            }
            return true;
        };
        for(int i = 0; i < 100000; i++)
        { // Any large bound to avoid infinite loop
            // More difficult case: First selected player will go in team 2, the next 2 to a random on (in its bounds)
            // But all random players have a chance to be in any team
            playerInfos = {BPI1, BPI3, BPI1_2, BPI1_3, BPI1_3};
            BOOST_REQUIRE(GameServer::assignPlayersOfRandomTeams(playerInfos));
            for(unsigned j = 2; j < playerInfos.size(); j++)
                playerWasAssignedTeam[j][playerInfos[j].team] = true;
            if(playerWasInTeams(2, {Team::Team1, Team::Team2})
               && playerWasInTeams(3, {Team::Team1, Team::Team2, Team::Team3})
               && playerWasInTeams(4, {Team::Team1, Team::Team2, Team::Team3}))
            {
                break;
            }
            BOOST_TEST(getMaxTeamSizeDifference(playerInfos, 3) == 1u);
        }
        BOOST_TEST(playerWasInTeams(2, {Team::Team1, Team::Team2}));
        BOOST_TEST(playerWasInTeams(3, {Team::Team1, Team::Team2, Team::Team3}));
        BOOST_TEST(playerWasInTeams(4, {Team::Team1, Team::Team2, Team::Team3}));
    }

    {
        std::array<helpers::EnumArray<bool, Team>, 6> playerWasAssignedTeam{};
        const auto playerWasInTeams = [&playerWasAssignedTeam](unsigned player, std::initializer_list<Team> teams) {
            for(const Team t : teams)
            {
                if(!playerWasAssignedTeam[player][t])
                    return false;
            }
            return true;
        };
        for(int i = 0; i < 100000; i++)
        { // Any large bound to avoid infinite loop
            // Similar to above but it should result in even teams
            playerInfos = {BPI1, BPI3, BPI1_2, BPI1_3, BPI1_3, BPI1_3};
            GameServer::assignPlayersOfRandomTeams(playerInfos);
            for(unsigned j = 2; j < playerInfos.size(); j++)
                playerWasAssignedTeam[j][playerInfos[j].team] = true;
            if(playerWasInTeams(2, {Team::Team1, Team::Team2})
               && playerWasInTeams(3, {Team::Team1, Team::Team2, Team::Team3})
               && playerWasInTeams(4, {Team::Team1, Team::Team2, Team::Team3})
               && playerWasInTeams(5, {Team::Team1, Team::Team2, Team::Team3}))
            {
                break;
            }
            BOOST_TEST(getMaxTeamSizeDifference(playerInfos, 3) == 0u);
        }
        BOOST_TEST(playerWasInTeams(2, {Team::Team1, Team::Team2}));
        BOOST_TEST(playerWasInTeams(3, {Team::Team1, Team::Team2, Team::Team3}));
        BOOST_TEST(playerWasInTeams(4, {Team::Team1, Team::Team2, Team::Team3}));
        BOOST_TEST(playerWasInTeams(5, {Team::Team1, Team::Team2, Team::Team3}));
    }
}
