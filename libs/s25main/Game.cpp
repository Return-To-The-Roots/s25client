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

#include "rttrDefines.h" // IWYU pragma: keep
#include "Game.h"
#include "EventManager.h"
#include "GameInterface.h"
#include "GamePlayer.h"
#include "ai/AIPlayer.h"
#include "lua/LuaInterfaceGame.h"

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
        StatisticStep();
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

    // check winning condition
    unsigned maxPoints = 0, totalPoints = 0, bestPlayer = 0xFFFF, maxTeamPoints = 0, bestTeam = 0xFFFF;

    // Find out best player. Since at least 3/4 of the populated land is needed to win, we don't care about ties.
    for(unsigned i = 0; i < world_.GetNumPlayers(); ++i)
    {
        GamePlayer& player = world_.GetPlayer(i);
        if(player.IsDefeated())
            continue;
        if(ggs_.lockedTeams) // in games with locked team settings check for team victory
        {
            unsigned curTeam = 0;
            unsigned teamPoints = 0;
            // Add points of all players in this players team (including himself)
            for(unsigned j = 0; j < world_.GetNumPlayers(); ++j)
            {
                if(!player.IsAlly(j))
                    continue;
                GamePlayer& teamPlayer = world_.GetPlayer(j);
                if(!teamPlayer.IsDefeated())
                {
                    curTeam = curTeam | (1 << j);
                    teamPoints += teamPlayer.GetStatisticCurrentValue(STAT_COUNTRY);
                }
            }
            if(teamPoints > maxTeamPoints)
            {
                maxTeamPoints = teamPoints;
                bestTeam = curTeam;
            }
        }
        unsigned points = player.GetStatisticCurrentValue(STAT_COUNTRY);
        if(points > maxPoints)
        {
            maxPoints = points;
            bestPlayer = i;
        }

        totalPoints += points;
    }
    // No one has land -> All lost?
    if(totalPoints == 0u)
        return;

    switch(ggs_.objective)
    {
        case GO_CONQUER3_4: // at least 3/4 of the land
            if(maxTeamPoints * 4 >= totalPoints * 3)
                finished_ = true;
            else if(maxPoints * 4 >= totalPoints * 3)
                finished_ = true;
            break;

        case GO_TOTALDOMINATION: // whole populated land
            if(maxTeamPoints == totalPoints)
                finished_ = true;
            else if(maxPoints == totalPoints)
                finished_ = true;
            break;
        default: break;
    }

    // We have a winner!
    if(finished_)
    {
        if(maxPoints >= maxTeamPoints)
            world_.GetGameInterface()->GI_Winner(bestPlayer);
        else
            world_.GetGameInterface()->GI_TeamWinner(bestTeam);
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
