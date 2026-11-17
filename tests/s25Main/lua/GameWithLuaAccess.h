// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
#include "world/GameWorld.h"
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
    GameWithLuaAccess() : Game(GlobalGameSettings(), 0u, createPlayers()) {}

    void executeAICommands()
    {
        AIPlayer* ai = getAIPlayer(1);
        std::vector<gc::GameCommandPtr> aiGcs = ai->fetchGameCommands();
        for(unsigned i = 0; i < 5; i++)
        {
            em_->executeNextGF();
            ai->runGF(em_->getCurrentGF(), i == 0);
        }
        for(gc::GameCommandPtr& gc : aiGcs)
        {
            gc->execute(world_, 1);
        }
    }

    static std::vector<PlayerInfo> createPlayers()
    {
        std::vector<PlayerInfo> players(3);
        players[0].ps = PlayerState::Occupied;
        players[0].name = "Player1";
        players[0].nation = Nation::Vikings;
        players[0].color = PLAYER_COLORS[5];
        players[0].team = Team::Team1;
        players[0].isHost = true;

        players[1].ps = PlayerState::AI;
        players[1].name = "PlayerAI";
        players[1].nation = Nation::Romans;
        players[1].color = 0xFFFF0000;
        players[1].team = Team::Team2;
        players[1].isHost = false;

        players[2].ps = PlayerState::Locked;
        return players;
    }
};

struct LuaTestsFixture : public rttr::test::LogAccessor, public LuaBaseFixture, GCExecutor
{
public:
    GameWithLuaAccess game;
    GameWorld& world;
    MockLocalGameState localGameState;
    std::vector<MapPoint> hqPositions;

    LuaTestsFixture() : world(game.world_)
    {
        game.setLua(std::make_unique<LuaInterfaceGame>(game, localGameState));
        setLua(&world.getLua());
    }

    void initWorld()
    {
        // For consistent results
        initGameRNG(0);

        loadGameData(world.getDescriptionWriteable());
        world.init(MapExtent(24, 32));
        hqPositions.push_back(MapPoint(0, 1));
        hqPositions.push_back(MapPoint(world.getSize() / 2u + hqPositions[0]));
        std::vector<Nation> playerNations;
        playerNations.push_back(world.getPlayer(0).nation);
        playerNations.push_back(world.getPlayer(1).nation);
        BOOST_TEST_REQUIRE(MapLoader::placeHQs(world, hqPositions, false));

        for(unsigned id = 0; id < world.getNumPlayers(); id++)
        {
            GamePlayer& player = world.getPlayer(id);
            if(!player.isHuman() && player.isUsed())
                game.addAIPlayer(AIFactory::create(world.getPlayer(id).aiInfo, id, world));
        }
    }

    GameWorld& getWorld() override { return world; }
};
