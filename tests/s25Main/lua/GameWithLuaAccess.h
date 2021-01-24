// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#pragma once

#include "EventManager.h"
#include "Game.h"
#include "GamePlayer.h"
#include "GlobalGameSettings.h"
#include "GlobalVars.h"
#include "LuaBaseFixture.h"
#include "ai/AIPlayer.h"
#include "factories/AIFactory.h"
#include "lua/GameDataLoader.h"
#include "lua/LuaInterfaceGame.h"
#include "worldFixtures/GCExecutor.h"
#include "worldFixtures/MockLocalGameState.h"
#include "worldFixtures/initGameRNG.hpp"
#include "world/GameWorldGame.h"
#include "world/MapLoader.h"
#include "s25util/AvoidDuplicatesWriter.h"
#include "s25util/Log.h"
#include "s25util/StringStreamWriter.h"
#include "s25util/colors.h"
#include <rttr/test/LogAccessor.hpp>
#include <boost/test/unit_test.hpp>
#include <memory>
#include <vector>

class GameWithLuaAccess : public Game
{
public:
    GameWithLuaAccess() : Game(GlobalGameSettings(), 0u, CreatePlayers()) {}

    void executeAICommands()
    {
        AIPlayer* ai = GetAIPlayer(1);
        std::vector<gc::GameCommandPtr> aiGcs = ai->FetchGameCommands();
        for(unsigned i = 0; i < 5; i++)
        {
            em_->ExecuteNextGF();
            ai->RunGF(em_->GetCurrentGF(), i == 0);
        }
        for(gc::GameCommandPtr& gc : aiGcs)
        {
            gc->Execute(world_, 1);
        }
    }

    static std::vector<PlayerInfo> CreatePlayers()
    {
        std::vector<PlayerInfo> players(3);
        players[0].ps = PlayerState::Occupied;
        players[0].name = "Player1";
        players[0].nation = Nation::Vikings;
        players[0].color = PLAYER_COLORS[5];
        players[0].team = TM_TEAM1;
        players[0].isHost = true;

        players[1].ps = PlayerState::AI;
        players[1].name = "PlayerAI";
        players[1].nation = Nation::Romans;
        players[1].color = 0xFFFF0000;
        players[1].team = TM_TEAM2;
        players[1].isHost = false;

        players[2].ps = PlayerState::Locked;
        return players;
    }
};

struct LuaTestsFixture : public rttr::test::LogAccessor, public LuaBaseFixture, GCExecutor
{
public:
    std::shared_ptr<GameWithLuaAccess> game;
    GameWorld& world;
    MockLocalGameState localGameState;
    std::vector<MapPoint> hqPositions;

    LuaTestsFixture() : game(std::make_shared<GameWithLuaAccess>()), world(game->world_)
    {
        game->world_.SetLua(std::make_unique<LuaInterfaceGame>(game, localGameState));
        setLua(&game->world_.GetLua());
    }

    void initWorld()
    {
        // For consistent results
        initGameRNG(0);

        loadGameData(world.GetDescriptionWriteable());
        world.Init(MapExtent(24, 32));
        hqPositions.push_back(MapPoint(0, 1));
        hqPositions.push_back(MapPoint(world.GetSize() / 2u + hqPositions[0]));
        std::vector<Nation> playerNations;
        playerNations.push_back(world.GetPlayer(0).nation);
        playerNations.push_back(world.GetPlayer(1).nation);
        BOOST_TEST_REQUIRE(MapLoader::PlaceHQs(world, hqPositions, false));

        for(unsigned id = 0; id < world.GetNumPlayers(); id++)
        {
            GamePlayer& player = world.GetPlayer(id);
            if(!player.isHuman() && player.isUsed())
                game->AddAIPlayer(AIFactory::Create(world.GetPlayer(id).aiInfo, id, world));
        }
    }

    virtual GameWorldGame& GetWorld() override { return world; }
};
