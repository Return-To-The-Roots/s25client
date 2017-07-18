// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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

#ifndef GameWorldWithLuaAccess_h__
#define GameWorldWithLuaAccess_h__

#include "GlobalGameSettings.h"
#include "EventManager.h"
#include "GlobalVars.h"
#include "lua/LuaInterfaceGame.h"
#include "GamePlayer.h"
#include "world/GameWorldGame.h"
#include "world/MapLoader.h"
#include "test/initTestHelpers.h"
#include "libutil/src/Log.h"
#include "libutil/src/colors.h"
#include "libutil/src/StringStreamWriter.h"
#include <boost/test/unit_test.hpp>
#include <vector>

class GameWorldWithLuaAccess: public GameWorldGame
{
public:
    GlobalGameSettings ggs;
    EventManager em;

    GameWorldWithLuaAccess(): GameWorldGame(CreatePlayers(), ggs, em), em(0)
    {
        createLua();
    }

    void createLua()
    {
        lua.reset(new LuaInterfaceGame(*this));
    }

    void executeLua(const std::string& luaCode)
    {
        lua->LoadScriptString(luaCode);
    }

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

struct LuaTestsFixture{
    StringStreamWriter* logWriter;
    GameWorldWithLuaAccess world;
    std::vector<MapPoint> hqPositions;

    LuaTestsFixture()
    {
        GLOBALVARS.isTest = true;
        logWriter = dynamic_cast<StringStreamWriter*>(LOG.getFileWriter());
        BOOST_REQUIRE(logWriter);
        clearLog();
        GameObject::SetPointers(&world);
    }

    ~LuaTestsFixture()
    {
        GLOBALVARS.isTest = false;
        GameObject::SetPointers(NULL);
    }

    void clearLog()
    {
        logWriter->getStream().str("");
    }

    std::string getLog(bool clear = true)
    {
        std::string result = logWriter->getText();
        if(clear)
            clearLog();
        return result;
    }

    void executeLua(const std::string& luaCode)
    {
        world.executeLua(luaCode);
    }

    void executeLua(const boost::format& luaCode)
    {
        executeLua(luaCode.str());
    }

    boost::test_tools::predicate_result isLuaEqual(const std::string& luaVal, const std::string& expectedValue)
    {
        try
        {
            executeLua(std::string("assert(") + luaVal + "==" + expectedValue + ", 'xxx=' .. tostring(" + luaVal + "))");
        } catch(std::runtime_error& e)
        {
            boost::test_tools::predicate_result result(false);
            std::string msg = e.what();
            size_t xPos = msg.rfind("xxx=");
            if(xPos != std::string::npos)
                result.message() << "Value = " << msg.substr(xPos + 4);
            else
                result.message() << e.what();
            return result;
        }
        return true;
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

#endif // GameWorldWithLuaAccess_h__
