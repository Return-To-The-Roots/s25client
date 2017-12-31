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

#ifndef GameWithLuaAccess_h__
#define GameWithLuaAccess_h__

#include "BufferedWriter.h"
#include "EventManager.h"
#include "GamePlayer.h"
#include "GlobalGameSettings.h"
#include "GlobalVars.h"
#include "LuaBaseFixture.h"
#include "helperFuncs.h"
#include "lua/LuaInterfaceGame.h"
#include "world/GameWorldGame.h"
#include "world/MapLoader.h"
#include "test/initTestHelpers.h"
#include "libutil/AvoidDuplicatesWriter.h"
#include "libutil/Log.h"
#include "libutil/StringStreamWriter.h"
#include "libutil/colors.h"
#include "Game.h"
#include <boost/test/unit_test.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <vector>

class GameWithLuaAccess : public Game
{
public:
    GlobalGameSettings ggs;
    GameWithLuaAccess() : Game(ggs, (unsigned int)0, CreatePlayers()) { }

    static std::vector<PlayerInfo> CreatePlayers()
    {
        std::vector<PlayerInfo> players(3);
        players[0].ps = PS_OCCUPIED;
        players[0].name = "Player1";
        players[0].nation = NAT_VIKINGS;
        players[0].color = PLAYER_COLORS[5];
        players[0].team = TM_TEAM1;
        players[0].isHost = true;

        players[1].ps = PS_AI;
        players[1].name = "PlayerAI";
        players[1].nation = NAT_ROMANS;
        players[1].color = 0xFFFF0000;
        players[1].team = TM_TEAM2;
        players[1].isHost = false;

        players[2].ps = PS_LOCKED;
        return players;
    }
};

struct LuaTestsFixture : public LogAccessor, public LuaBaseFixture
{
public:
    boost::shared_ptr<GameWithLuaAccess> game;
    GameWorld& world;
    std::vector<MapPoint> hqPositions;

    LuaTestsFixture() : game(new GameWithLuaAccess), world(game->world) { 
        game->world.SetLua(new LuaInterfaceGame(game));
        luaBase = &game->world.GetLua(); 
    }

    void initWorld()
    {
        // For consistent results
        doInitGameRNG(0);

        world.Init(MapExtent(24, 32), LT_GREENLAND);
        hqPositions.push_back(MapPoint(0, 1));
        hqPositions.push_back(MapPoint(16, 17));
        std::vector<Nation> playerNations;
        playerNations.push_back(world.GetPlayer(0).nation);
        playerNations.push_back(world.GetPlayer(1).nation);
        BOOST_REQUIRE(MapLoader::PlaceHQs(world, hqPositions, playerNations, false));
    }
};

#endif // GameWithLuaAccess_h__
