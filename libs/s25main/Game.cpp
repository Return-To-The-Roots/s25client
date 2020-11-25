// Copyright (c) 2017 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#include "Game.h"
#include "EventManager.h"
#include "GameInterface.h"
#include "GamePlayer.h"
#include "addons/AddonGameLength.h"
#include "addons/const_addons.h"
#include "ai/AIPlayer.h"
#include "lua/LuaInterfaceGame.h"
#include <boost/optional.hpp>

Game::Game(const GlobalGameSettings& settings, unsigned startGF, const std::vector<PlayerInfo>& players)
    : Game(settings, std::make_unique<EventManager>(startGF), players)
{}

Game::Game(const GlobalGameSettings& settings, std::unique_ptr<EventManager> em, const std::vector<PlayerInfo>& players)
    : ggs_(settings), em_(std::move(em)), world_(players, ggs_, *em_), started_(false), finished_(false)
{}

Game::~Game() = default;

void Game::Start(bool startFromSave)
{
    if(started_)
        return;
    started_ = true;
    if(startFromSave)
        CheckObjective();
    else
    {
        if(ggs_.objective == GO_ECONOMYMODE)
        {
            unsigned int selection = ggs_.getSelection(AddonId::GAME_LENGTH);
            world_.econHandler =
              new EconomyModeHandler(AddonGameLengthList[selection] / 50); // 50 is the assumed game frame length
        }
        StatisticStep();
    }
    if(world_.HasLua())
        world_.GetLua().EventStart(!startFromSave);
}

void Game::AddAIPlayer(std::unique_ptr<AIPlayer> newAI)
{
    aiPlayers_.push_back(newAI.release());
}

namespace {
unsigned getNumAlivePlayers(const GameWorldBase& world)
{
    unsigned numPlayersAlive = 0;
    for(unsigned i = 0; i < world.GetNumPlayers(); ++i)
    {
        if(!world.GetPlayer(i).IsDefeated())
            ++numPlayersAlive;
    }
    return numPlayersAlive;
}
} // namespace

void Game::RunGF()
{
    unsigned numPlayersAlive = getNumAlivePlayers(world_);
    //  EventManager Bescheid sagen
    em_->ExecuteNextGF();
    // Notfallprogramm durchlaufen lassen
    for(unsigned i = 0; i < world_.GetNumPlayers(); ++i)
    {
        GamePlayer& player = world_.GetPlayer(i);
        if(player.isUsed())
        {
            // Auf Notfall testen (Wenige Bretter/Steine und keine Holzindustrie)
            player.TestForEmergencyProgramm();
            player.TestPacts();
        }
    }

    if(world_.HasLua())
        world_.GetLua().EventGameFrame(em_->GetCurrentGF());
    // Update statistic every 750 GFs (30 seconds on 'fast')
    if(em_->GetCurrentGF() % 750 == 0)
        StatisticStep();
    // If some players got defeated check objective
    if(getNumAlivePlayers(world_) < numPlayersAlive)
        CheckObjective();
}

void Game::StatisticStep()
{
    for(unsigned i = 0; i < world_.GetNumPlayers(); ++i)
        world_.GetPlayer(i).StatisticStep();

    CheckObjective();
}

void Game::CheckObjective()
{
    // Check objective if there is one
    if(finished_ || (ggs_.objective != GO_CONQUER3_4 && ggs_.objective != GO_TOTALDOMINATION))
        return;

    unsigned maxPoints = 0, maxTeamPoints = 0, totalPoints = 0, bestPlayer = 0;
    boost::optional<unsigned> bestTeam;

    const auto getPlayerMask = [](unsigned playerId) { return 1u << playerId; };

    // Find out best player. Since at least 3/4 of the populated land is needed to win, we don't care about ties.
    for(unsigned i = 0; i < world_.GetNumPlayers(); ++i)
    {
        const GamePlayer& player = world_.GetPlayer(i);
        if(player.IsDefeated())
            continue;
        const unsigned points = player.GetStatisticCurrentValue(STAT_COUNTRY);
        if(points > maxPoints)
        {
            maxPoints = points;
            bestPlayer = i;
        }
        totalPoints += points;
        if(ggs_.lockedTeams) // in games with locked team settings check for team victory
        {
            unsigned curTeam = getPlayerMask(i);
            unsigned teamPoints = points;
            // Add points of all players in this players team (excluding himself)
            for(unsigned j = 0; j < world_.GetNumPlayers(); ++j)
            {
                if(i == j || !player.IsAlly(j))
                    continue;
                curTeam = curTeam | getPlayerMask(j);
                const GamePlayer& teamPlayer = world_.GetPlayer(j);
                if(!teamPlayer.IsDefeated())
                    teamPoints += teamPlayer.GetStatisticCurrentValue(STAT_COUNTRY);
            }
            if(teamPoints > maxTeamPoints)
            {
                maxTeamPoints = teamPoints;
                bestTeam = curTeam;
            }
        }
    }
    // No one has land -> All lost?
    if(totalPoints == 0u)
        return;

    switch(ggs_.objective)
    {
        case GO_CONQUER3_4: // at least 3/4 of the land
            if(maxTeamPoints * 4u >= totalPoints * 3u || maxPoints * 4u >= totalPoints * 3u)
                finished_ = true;
            break;

        case GO_TOTALDOMINATION: // whole populated land
            if(maxTeamPoints == totalPoints || maxPoints == totalPoints)
                finished_ = true;
            break;
        default: break;
    }

    // We have a winner!
    if(finished_)
    {
        // If there is a team that is best and it does not only consist of the best player
        // then it is a team victory, else a single players victory
        if(bestTeam && *bestTeam != getPlayerMask(bestPlayer))
            world_.GetGameInterface()->GI_TeamWinner(*bestTeam);
        else
            world_.GetGameInterface()->GI_Winner(bestPlayer);
    }
}

AIPlayer* Game::GetAIPlayer(unsigned id)
{
    for(AIPlayer& ai : aiPlayers_)
    {
        if(ai.GetPlayerId() == id)
            return &ai;
    }
    return nullptr;
}
