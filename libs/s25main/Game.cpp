// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Game.h"
#include "EconomyModeHandler.h"
#include "EventManager.h"
#include "GameInterface.h"
#include "GamePlayer.h"
#include "addons/AddonEconomyModeGameLength.h"
#include "addons/const_addons.h"
#include "ai/AIPlayer.h"
#include "lua/LuaInterfaceGame.h"
#include "network/GameClient.h"
#include "gameData/GameConsts.h"
#include <optional>

Game::Game(GlobalGameSettings settings, unsigned startGF, const std::vector<PlayerInfo>& players)
    : Game(std::move(settings), std::make_unique<EventManager>(startGF), players)
{}

Game::Game(GlobalGameSettings settings, std::unique_ptr<EventManager> em, const std::vector<PlayerInfo>& players)
    : ggs_(std::move(settings)), em_(std::move(em)), world_(players, ggs_, *em_), started_(false), finished_(false)
{}

Game::~Game() = default;

void Game::start(bool startFromSave)
{
    if(started_)
        return;
    started_ = true;
    if(startFromSave)
        checkObjective();
    else
    {
        if(ggs_.objective == GameObjective::EconomyMode)
        {
            unsigned int selection = ggs_.getSelection(AddonId::ECONOMY_MODE_GAME_LENGTH);
            world_.setEconHandler(
              std::make_unique<EconomyModeHandler>(durationToGfs(AddonEconomyModeGameLengthList[selection])));
        }
        statisticStep();
    }
    if(world_.hasLua())
        world_.getLua().eventStart(!startFromSave);
}

void Game::addAIPlayer(std::unique_ptr<AIPlayer> newAI)
{
    aiPlayers_.push_back(std::move(newAI));
}

void Game::setLua(std::unique_ptr<LuaInterfaceGame> newLua)
{
    lua = std::move(newLua);
    world_.setLua(lua.get());
}

namespace {
unsigned getNumAlivePlayers(const GameWorldBase& world)
{
    unsigned numPlayersAlive = 0;
    for(const auto& player : world.getPlayers())
    {
        if(!player.isDefeated())
            ++numPlayersAlive;
    }
    return numPlayersAlive;
}
} // namespace

void Game::runGF()
{
    unsigned numPlayersAlive = getNumAlivePlayers(world_);
    //  EventManager Bescheid sagen
    em_->executeNextGF();
    // Notfallprogramm durchlaufen lassen
    for(GamePlayer& player : world_.getPlayers())
    {
        if(player.isUsed())
        {
            // Auf Notfall testen (Wenige Bretter/Steine und keine Holzindustrie)
            player.testForEmergencyProgramm();
            player.testPacts();
        }
    }

    if(world_.hasLua())
        world_.getLua().eventGameFrame(em_->getCurrentGF());
    // Update statistic every 30 seconds
    constexpr auto GFsIn30s = static_cast<unsigned>(durationToGfs(30s));
    if(em_->getCurrentGF() % GFsIn30s == 0)
        statisticStep();
    // If some players got defeated check objective
    if(getNumAlivePlayers(world_) < numPlayersAlive)
        checkObjective();
}

void Game::statisticStep()
{
    for(auto& player : world_.getPlayers())
        player.statisticStep();

    checkObjective();
}

void Game::checkObjective()
{
    // Check objective if there is one
    if(finished_ || (ggs_.objective != GameObjective::Conquer3_4 && ggs_.objective != GameObjective::TotalDomination))
        return;

    unsigned maxPoints = 0, maxTeamPoints = 0, totalPoints = 0, bestPlayer = 0;
    std::optional<unsigned> bestTeam;

    const auto getPlayerMask = [](unsigned playerId) { return 1u << playerId; };

    // Find out best player. Since at least 3/4 of the populated land is needed to win, we don't care about ties.
    for(unsigned i = 0; i < world_.getNumPlayers(); ++i)
    {
        const GamePlayer& player = world_.getPlayer(i);
        if(player.isDefeated())
            continue;
        const unsigned points = player.getStatisticCurrentValue(StatisticType::Country);
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
            for(unsigned j = 0; j < world_.getNumPlayers(); ++j)
            {
                if(i == j || !player.isAlly(j))
                    continue;
                curTeam = curTeam | getPlayerMask(j);
                const GamePlayer& teamPlayer = world_.getPlayer(j);
                if(!teamPlayer.isDefeated())
                    teamPoints += teamPlayer.getStatisticCurrentValue(StatisticType::Country);
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
        case GameObjective::Conquer3_4: // at least 3/4 of the land
            if(maxTeamPoints * 4u >= totalPoints * 3u || maxPoints * 4u >= totalPoints * 3u)
                finished_ = true;
            break;

        case GameObjective::TotalDomination: // whole populated land
            if(maxTeamPoints == totalPoints || maxPoints == totalPoints)
                finished_ = true;
            break;
        default: break;
    }

    // We have a winner!
    if(world_.getGameInterface() && finished_)
    {
        // If there is a team that is best and it does not only consist of the best player
        // then it is a team victory, else a single players victory
        if(bestTeam && *bestTeam != getPlayerMask(bestPlayer))
            world_.getGameInterface()->GI_TeamWinner(*bestTeam);
        else
            world_.getGameInterface()->GI_Winner(bestPlayer);
    }
}

AIPlayer* Game::getAIPlayer(unsigned id)
{
    for(AIPlayer& ai : aiPlayers_)
    {
        if(ai.getPlayerId() == id)
            return &ai;
    }
    return nullptr;
}
