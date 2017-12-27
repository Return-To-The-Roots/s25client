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
#include "GameInterface.h"
#include "GamePlayer.h"
#include "ai/AIPlayer.h"
#include "lua/LuaInterfaceGame.h"

Game::Game(const GlobalGameSettings& settings, unsigned startGF, const std::vector<PlayerInfo>& players)
    : ggs(settings), em(new EventManager(startGF)), world(players, ggs, *em), started(false), finished(false)
{}

Game::Game(const GlobalGameSettings& settings, EventManager* em, const std::vector<PlayerInfo>& players)
    : ggs(settings), em(em), world(players, ggs, *em), started(false), finished(false)
{}

Game::~Game() {}

void Game::Start(bool startFromSave)
{
    if(started)
        return;
    started = true;
    if(startFromSave)
        CheckObjective();
    else
        StatisticStep();
    if(world.HasLua())
        world.GetLua().EventStart(!startFromSave);
}

void Game::RunGF()
{
    //  EventManager Bescheid sagen
    em->ExecuteNextGF();
    // Notfallprogramm durchlaufen lassen
    for(unsigned i = 0; i < world.GetNumPlayers(); ++i)
    {
        GamePlayer& player = world.GetPlayer(i);
        if(player.isUsed())
        {
            // Auf Notfall testen (Wenige Bretter/Steine und keine Holzindustrie)
            player.TestForEmergencyProgramm();
            player.TestPacts();
        }
    }

    if(world.HasLua())
        world.GetLua().EventGameFrame(em->GetCurrentGF());
    // Update statistic every 750 GFs (30 seconds on 'fast')
    if(em->GetCurrentGF() % 750 == 0)
        StatisticStep();
}

void Game::StatisticStep()
{
    for(unsigned i = 0; i < world.GetNumPlayers(); ++i)
        world.GetPlayer(i).StatisticStep();

    CheckObjective();
}

void Game::CheckObjective()
{
    // Check objective if there is one
    if(finished || (ggs.objective != GO_CONQUER3_4 && ggs.objective != GO_TOTALDOMINATION))
        return;

    // check winning condition
    unsigned max = 0, sum = 0, best = 0xFFFF, maxteam = 0, bestteam = 0xFFFF;

    // Find out best player. Since at least 3/4 of the populated land is needed to win, we don't care about ties.
    for(unsigned i = 0; i < world.GetNumPlayers(); ++i)
    {
        GamePlayer& player = world.GetPlayer(i);
        if(ggs.lockedTeams) // in games with locked team settings check for team victory
        {
            if(player.IsDefeated())
                continue;
            unsigned curteam = 0;
            unsigned teampoints = 0;
            for(unsigned j = 0; j < world.GetNumPlayers(); ++j)
            {
                if(i == j || !player.IsAlly(j))
                    continue;
                GamePlayer& teamPlayer = world.GetPlayer(j);
                if(!teamPlayer.IsDefeated())
                {
                    curteam = curteam | (1 << j);
                    teampoints += teamPlayer.GetStatisticCurrentValue(NUM_STATSRY);
                }
            }
            teampoints += player.GetStatisticCurrentValue(NUM_STATSRY);
            curteam = curteam | (1 << i);
            if(teampoints > maxteam && teampoints > player.GetStatisticCurrentValue(NUM_STATSRY))
            {
                maxteam = teampoints;
                bestteam = curteam;
            }
        }
        unsigned v = player.GetStatisticCurrentValue(NUM_STATSRY);
        if(v > max)
        {
            max = v;
            best = i;
        }

        sum += v;
    }

    switch(ggs.objective)
    {
        case GO_CONQUER3_4: // at least 3/4 of the land
            if((max * 4 >= sum * 3) && (best != 0xFFFF))
                finished = true;
            if((maxteam * 4 >= sum * 3) && (bestteam != 0xFFFF))
                finished = true;
            break;

        case GO_TOTALDOMINATION: // whole populated land
            if((max == sum) && (best != 0xFFFF))
                finished = true;
            if((maxteam == sum) && (bestteam != 0xFFFF))
                finished = true;
            break;
        default: break;
    }

    // We have a winner!
    if(finished)
    {
        if(maxteam <= best)
            world.GetGameInterface()->GI_Winner(best);
        else
            world.GetGameInterface()->GI_TeamWinner(bestteam);
    }
}
