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
#include "gameTypes/TeamTypes.h"
#include <boost/test/unit_test.hpp>

static unsigned getMaxTeamSizeDifference(std::vector<JoinPlayerInfo>& playerInfos, unsigned nTeams)
{
    unsigned nPlayers[4] = {0, 0, 0, 0};
    for(auto& playerInfo : playerInfos)
    {
        switch(playerInfo.team)
        {
            case TM_RANDOMTEAM:
            case TM_TEAM1: ++nPlayers[0]; break;
            case TM_RANDOMTEAM2:
            case TM_TEAM2: ++nPlayers[1]; break;
            case TM_RANDOMTEAM3:
            case TM_TEAM3: ++nPlayers[2]; break;
            case TM_RANDOMTEAM4:
            case TM_TEAM4: ++nPlayers[3]; break;
            default: return -1;
        };
    }

    std::sort(&nPlayers[0], &nPlayers[nTeams]);
    BOOST_TEST_REQUIRE(nPlayers[0] <= nPlayers[nTeams - 1]);
    return nPlayers[nTeams - 1] - nPlayers[0];
}

BOOST_AUTO_TEST_CASE(JoinPlayerAssignment)
{
    BasePlayerInfo BPI1, BPI2, BPI3, BPI4, BPI1_2, BPI1_3, BPI1_4, BPIRT1, BPIRT2, BPIRT3, BPIRT4;
    BPI1.team = TM_TEAM1;
    BPI2.team = TM_TEAM2;
    BPI3.team = TM_TEAM3;
    BPI4.team = TM_TEAM4;
    BPIRT1.team = TM_RANDOMTEAM;
    BPIRT2.team = TM_RANDOMTEAM2;
    BPIRT3.team = TM_RANDOMTEAM3;
    BPIRT4.team = TM_RANDOMTEAM4;
    BPI1_2.team = TM_TEAM_1_TO_2;
    BPI1_3.team = TM_TEAM_1_TO_3;
    BPI1_4.team = TM_TEAM_1_TO_4;

    std::vector<JoinPlayerInfo> playerInfos;

    playerInfos.push_back(JoinPlayerInfo(BPI1_4));
    playerInfos.push_back(JoinPlayerInfo(BPI1));
    playerInfos.push_back(JoinPlayerInfo(BPIRT2));
    playerInfos.push_back(JoinPlayerInfo(BPI1_3));
    BOOST_REQUIRE(GameServer::assignPlayersOfRandomTeams(playerInfos));
    BOOST_TEST(2u >= getMaxTeamSizeDifference(playerInfos, 4));

    playerInfos.clear();
    playerInfos.push_back(JoinPlayerInfo(BPI1_4));
    playerInfos.push_back(JoinPlayerInfo(BPI1));
    playerInfos.push_back(JoinPlayerInfo(BPIRT2));
    playerInfos.push_back(JoinPlayerInfo(BPI1_4));
    BOOST_REQUIRE(GameServer::assignPlayersOfRandomTeams(playerInfos));
    BOOST_TEST(0u == getMaxTeamSizeDifference(playerInfos, 4));

    playerInfos.clear();
    playerInfos.push_back(JoinPlayerInfo(BPI1_3));
    playerInfos.push_back(JoinPlayerInfo(BPI1));
    playerInfos.push_back(JoinPlayerInfo(BPIRT2));
    playerInfos.push_back(JoinPlayerInfo(BPI1_3));
    BOOST_REQUIRE(GameServer::assignPlayersOfRandomTeams(playerInfos));
    BOOST_TEST(1u == getMaxTeamSizeDifference(playerInfos, 3));

    playerInfos.clear();
    playerInfos.push_back(JoinPlayerInfo(BPI1_4));
    playerInfos.push_back(JoinPlayerInfo(BPI1_4));
    playerInfos.push_back(JoinPlayerInfo(BPI1_4));
    playerInfos.push_back(JoinPlayerInfo(BPI1_4));
    playerInfos.push_back(JoinPlayerInfo(BPI1_4));
    playerInfos.push_back(JoinPlayerInfo(BPI1_4));
    playerInfos.push_back(JoinPlayerInfo(BPI1_4));
    playerInfos.push_back(JoinPlayerInfo(BPI1_4));
    BOOST_REQUIRE(GameServer::assignPlayersOfRandomTeams(playerInfos));
    BOOST_TEST(0u == getMaxTeamSizeDifference(playerInfos, 4));

    playerInfos.clear();
    playerInfos.push_back(JoinPlayerInfo(BPI1_4));
    playerInfos.push_back(JoinPlayerInfo(BPI1_4));
    playerInfos.push_back(JoinPlayerInfo(BPI1_4));
    playerInfos.push_back(JoinPlayerInfo(BPI1_3));
    playerInfos.push_back(JoinPlayerInfo(BPI1_3));
    playerInfos.push_back(JoinPlayerInfo(BPI1_3));
    playerInfos.push_back(JoinPlayerInfo(BPI1_3));
    playerInfos.push_back(JoinPlayerInfo(BPI1_3));
    BOOST_REQUIRE(GameServer::assignPlayersOfRandomTeams(playerInfos));
    BOOST_TEST(2u >= getMaxTeamSizeDifference(playerInfos, 4));

    playerInfos.clear();
    playerInfos.push_back(JoinPlayerInfo(BPI3));
    playerInfos.push_back(JoinPlayerInfo(BPI3));
    playerInfos.push_back(JoinPlayerInfo(BPI1_2));
    playerInfos.push_back(JoinPlayerInfo(BPI1_2));
    playerInfos.push_back(JoinPlayerInfo(BPIRT1));
    playerInfos.push_back(JoinPlayerInfo(BPIRT1));
    BOOST_REQUIRE(GameServer::assignPlayersOfRandomTeams(playerInfos));
    BOOST_TEST(0u == getMaxTeamSizeDifference(playerInfos, 3));

    playerInfos.clear();
    playerInfos.push_back(JoinPlayerInfo(BPI3));
    playerInfos.push_back(JoinPlayerInfo(BPI3));
    playerInfos.push_back(JoinPlayerInfo(BPIRT1));
    playerInfos.push_back(JoinPlayerInfo(BPIRT1));
    BOOST_REQUIRE(!GameServer::assignPlayersOfRandomTeams(playerInfos));
    BOOST_TEST(2u == getMaxTeamSizeDifference(playerInfos, 3));
}
