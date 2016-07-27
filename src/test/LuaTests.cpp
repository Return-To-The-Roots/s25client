// Copyright (c) 2016 Settlers Freaks (sf-team at siedler25.org)
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

#include "defines.h" // IWYU pragma: keep
#include "GlobalGameSettings.h"
#include "EventManager.h"
#include "lua/LuaInterfaceGame.h"
#include "GlobalVars.h"
#include "PlayerInfo.h"
#include "GameClient.h"
#include "ClientInterface.h"
#include "GameMessages.h"
#include "GamePlayer.h"
#include "buildings/nobHQ.h"
#include "postSystem/PostMsg.h"
#include "world/MapLoader.h"
#include "world/GameWorldGame.h"
#include "helpers/converters.h"
#include "test/PointOutput.h"
#include "libutil/src/colors.h"
#include "libutil/src/Log.h"
#include "libutil/src/StringStreamWriter.h"
#include <boost/test/unit_test.hpp>
#include <boost/format.hpp>
#include <vector>
#include "nodeObjs/noEnvObject.h"

namespace{
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
        boost::array<const nobHQ*, 2> hqs;

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
            GameObject::SetPointers(NULL);
        }

        void clearLog()
        {
            logWriter->getStream().str("");
        }

        std::string getLog()
        {
            return logWriter->getText();
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
            world.Init(32, 32, LT_GREENLAND);
            hqPositions.push_back(MapPoint(0, 1));
            hqPositions.push_back(MapPoint(16, 17));
            std::vector<Nation> playerNations;
            playerNations.push_back(world.GetPlayer(0).nation);
            playerNations.push_back(world.GetPlayer(1).nation);
            BOOST_REQUIRE(MapLoader::PlaceHQs(world, hqPositions, playerNations, false));
            for(unsigned i = 0; i < hqs.size(); i++)
            {
                hqs[i] = world.GetSpecObj<nobHQ>(world.GetPlayer(i).GetHQPos());
                BOOST_REQUIRE(hqs[i]);
            }
        }
    };

}

BOOST_FIXTURE_TEST_SUITE(LuaTestSuite, LuaTestsFixture)

BOOST_AUTO_TEST_CASE(AssertionThrows)
{
    BOOST_REQUIRE_THROW(executeLua("assert(false)"), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(BaseFunctions)
{
    executeLua("rttr:Log('Hello World')");
    BOOST_REQUIRE_EQUAL(getLog(), "Hello World\n");
    // (Invalid) connect to set params
    BOOST_REQUIRE(!GAMECLIENT.Connect("localhost", "", ServerType::LOCAL, 0, true, false));
    BOOST_CHECK(isLuaEqual("rttr:IsHost()", "true"));
    BOOST_REQUIRE(!GAMECLIENT.Connect("localhost", "", ServerType::LOCAL, 0, false, false));
    BOOST_CHECK(isLuaEqual("rttr:IsHost()", "false"));
    BOOST_CHECK(isLuaEqual("rttr:GetPlayerCount()", "3"));
    // Set Player ID
    static_cast<GameMessageInterface&>(GAMECLIENT).OnGameMessage(GameMessage_Player_Id(1));
    BOOST_CHECK(isLuaEqual("rttr:GetLocalPlayerIdx()", "1"));

    // TODO: Add test for message box
}

namespace{
    struct StoreChat: public ClientInterface
    {
        unsigned lastPlayerId;
        ChatDestination lastCD;
        std::string lastMsg;

        StoreChat()
        {
            Clear();
            GAMECLIENT.SetInterface(this);
        }

        ~StoreChat()
        {
            GAMECLIENT.SetInterface(NULL);
        }

        void Clear()
        {
            lastPlayerId = 1337;
            lastCD = CD_ALL;
            lastMsg.clear();
        }

        void CI_Chat(const unsigned playerId, const ChatDestination cd, const std::string& msg) override
        {
            lastPlayerId = playerId;
            lastCD = cd;
            lastMsg = msg;
        }

    };
}

BOOST_AUTO_TEST_CASE(GameFunctions)
{
    initWorld();
    BOOST_REQUIRE_GT(hqs[0]->GetRealWaresCount(GD_BOARDS), 0u);

    executeLua("rttr:ClearResources()");
    for(unsigned i = 0; i < hqs.size(); i++)
    {
        for(unsigned gd = 0; gd < WARE_TYPES_COUNT; gd++)
        {
            BOOST_REQUIRE_EQUAL(hqs[i]->GetRealWaresCount(GoodType(gd)), 0u);
            BOOST_REQUIRE_EQUAL(hqs[i]->GetVisualWaresCount(GoodType(gd)), 0u);
        }
        for(unsigned job = 0; job < JOB_TYPES_COUNT; job++)
        {
            BOOST_REQUIRE_EQUAL(hqs[i]->GetRealFiguresCount(Job(job)), 0u);
            BOOST_REQUIRE_EQUAL(hqs[i]->GetVisualFiguresCount(Job(job)), 0u);
        }
    }

    for(unsigned i = 0; i<2; i++)
    {
        BOOST_CHECK(isLuaEqual("rttr:GetGF()", helpers::toString(world.em.GetCurrentGF())));
        world.em.ExecuteNextGF();
    }

    StoreChat storeChat;
    // Set player id
    static_cast<GameMessageInterface&>(GAMECLIENT).OnGameMessage(GameMessage_Player_Id(1));
    BOOST_REQUIRE_EQUAL(GAMECLIENT.GetPlayerId(), 1u);
    // Send to other player
    executeLua("rttr:Chat(0, 'Hello World')");
    BOOST_REQUIRE_EQUAL(storeChat.lastMsg, "");
    // Send to this player
    executeLua("rttr:Chat(1, 'Hello World')");
    BOOST_REQUIRE_EQUAL(storeChat.lastMsg, "Hello World");
    BOOST_REQUIRE_EQUAL(storeChat.lastCD, CD_SYSTEM);
    // Send to all
    executeLua("rttr:Chat(-1, 'Hello All')");
    BOOST_REQUIRE_EQUAL(storeChat.lastMsg, "Hello All");
    BOOST_REQUIRE_EQUAL(storeChat.lastCD, CD_SYSTEM);

    // TODO: Test MissionStatement(player, title, message)
    
    world.GetPostMgr().AddPostBox(1);
    const PostBox& postBox = *world.GetPostMgr().GetPostBox(1);
    // Send to other player or invalid
    executeLua("rttr:PostMessage(0, 'Hello World')");
    executeLua("rttr:PostMessage(-1, 'Hello World')");
    BOOST_REQUIRE_EQUAL(postBox.GetNumMsgs(), 0u);
    // Send to this player
    executeLua("rttr:PostMessage(1, 'Hello World')");
    BOOST_REQUIRE_EQUAL(postBox.GetNumMsgs(), 1u);
    BOOST_REQUIRE_EQUAL(postBox.GetMsg(0)->GetText(), "Hello World");
    BOOST_REQUIRE(!postBox.GetMsg(0)->GetPos().isValid());
    // Location wraps to real world coordinates
    executeLua("rttr:PostMessageWithLocation(1, 'Hello Pos', 35, -5)");
    BOOST_REQUIRE_EQUAL(postBox.GetNumMsgs(), 2u);
    BOOST_REQUIRE_EQUAL(postBox.GetMsg(1)->GetText(), "Hello Pos");
    BOOST_REQUIRE_EQUAL(postBox.GetMsg(1)->GetPos(), MapPoint(35 - 32, -5 + 32));

    executeLua("assert(rttr:GetPlayer(0))");
    executeLua("assert(rttr:GetPlayer(1))");
    executeLua("assert(rttr:GetPlayer(2))");
    // Invalid player
    BOOST_REQUIRE_THROW(executeLua("assert(rttr:GetPlayer(3))"), std::runtime_error);
    BOOST_REQUIRE_THROW(executeLua("assert(rttr:GetPlayer(-1))"), std::runtime_error);

    executeLua("assert(rttr:GetWorld())");
}

BOOST_AUTO_TEST_CASE(AccessPlayerProperties)
{
    executeLua("player = rttr:GetPlayer(0)");
    BOOST_CHECK(isLuaEqual("player:GetName()", "'Player1'"));
    BOOST_CHECK(isLuaEqual("player:GetNation()", "NAT_VIKINGS"));
    BOOST_CHECK(isLuaEqual("player:GetTeam()", "TM_TEAM1"));
    BOOST_CHECK(isLuaEqual("player:GetColor()", "5"));
    BOOST_CHECK(isLuaEqual("player:IsAI()", "false"));
    BOOST_CHECK(isLuaEqual("player:IsClosed()", "false"));
    BOOST_CHECK(isLuaEqual("player:IsFree()", "false"));

    executeLua("player = rttr:GetPlayer(1)");
    BOOST_CHECK(isLuaEqual("player:GetName()", "'PlayerAI'"));
    BOOST_CHECK(isLuaEqual("player:GetNation()", "NAT_ROMANS"));
    BOOST_CHECK(isLuaEqual("player:GetTeam()", "TM_TEAM2"));
    BOOST_CHECK(isLuaEqual("player:GetColor()", helpers::toString(0xFFFF0000)));
    BOOST_CHECK(isLuaEqual("player:IsAI()", "true"));
    BOOST_CHECK(isLuaEqual("player:IsClosed()", "false"));
    BOOST_CHECK(isLuaEqual("player:IsFree()", "false"));

    executeLua("player = rttr:GetPlayer(2)");
    BOOST_CHECK(isLuaEqual("player:IsClosed()", "true"));
    BOOST_CHECK(isLuaEqual("player:IsFree()", "false"));
}

BOOST_AUTO_TEST_CASE(IngamePlayer)
{
    initWorld();

    const GamePlayer& player = world.GetPlayer(1);
    executeLua("player = rttr:GetPlayer(1)\nassert(player)");

    BOOST_REQUIRE(player.IsBuildingEnabled(BLD_WOODCUTTER));
    BOOST_REQUIRE(player.IsBuildingEnabled(BLD_FORTRESS));
    executeLua("player:DisableBuilding(BLD_WOODCUTTER)\nplayer:DisableBuilding(BLD_FORTRESS)");
    BOOST_REQUIRE(!player.IsBuildingEnabled(BLD_WOODCUTTER));
    BOOST_REQUIRE(!player.IsBuildingEnabled(BLD_FORTRESS));

    world.GetPostMgr().AddPostBox(1);
    const PostBox& postBox = *world.GetPostMgr().GetPostBox(1);
    // Enable without notification
    executeLua("player:EnableBuilding(BLD_WOODCUTTER)");
    BOOST_REQUIRE(player.IsBuildingEnabled(BLD_WOODCUTTER));
    BOOST_REQUIRE(!player.IsBuildingEnabled(BLD_FORTRESS));
    BOOST_REQUIRE_EQUAL(postBox.GetNumMsgs(), 0u);
    executeLua("player:DisableBuilding(BLD_WOODCUTTER)\nplayer:EnableBuilding(BLD_WOODCUTTER, false)");
    BOOST_REQUIRE(player.IsBuildingEnabled(BLD_WOODCUTTER));
    BOOST_REQUIRE_EQUAL(postBox.GetNumMsgs(), 0u);

    // Enable with notification
    executeLua("player:EnableBuilding(BLD_WOODCUTTER, true)");
    BOOST_REQUIRE(player.IsBuildingEnabled(BLD_WOODCUTTER));
    BOOST_REQUIRE_EQUAL(postBox.GetNumMsgs(), 1u);

    executeLua("player:DisableAllBuildings()");
    for(unsigned bld = 0; bld < BUILDING_TYPES_COUNT; bld++)
        BOOST_REQUIRE(!player.IsBuildingEnabled(BuildingType(bld)));

    executeLua("player:EnableAllBuildings()");
    for(unsigned bld = 0; bld < BUILDING_TYPES_COUNT; bld++)
        BOOST_REQUIRE(player.IsBuildingEnabled(BuildingType(bld)));

    executeLua("player:SetRestrictedArea(5,7, 5,12, 15,12)");
    BOOST_REQUIRE_EQUAL(player.GetRestrictedArea().size(), 3u);
    BOOST_REQUIRE_EQUAL(player.GetRestrictedArea()[0], MapPoint(5, 7));
    BOOST_REQUIRE_EQUAL(player.GetRestrictedArea()[1], MapPoint(5, 12));
    BOOST_REQUIRE_EQUAL(player.GetRestrictedArea()[2], MapPoint(15, 12));

    executeLua("player:SetRestrictedArea()");
    BOOST_REQUIRE_EQUAL(player.GetRestrictedArea().size(), 0u);

    executeLua("player:ClearResources()");
    for(unsigned gd = 0; gd < WARE_TYPES_COUNT; gd++)
    {
        BOOST_REQUIRE_EQUAL(hqs[1]->GetRealWaresCount(GoodType(gd)), 0u);
        BOOST_REQUIRE_EQUAL(hqs[1]->GetVisualWaresCount(GoodType(gd)), 0u);
    }
    for(unsigned job = 0; job < JOB_TYPES_COUNT; job++)
    {
        BOOST_REQUIRE_EQUAL(hqs[1]->GetRealFiguresCount(Job(job)), 0u);
        BOOST_REQUIRE_EQUAL(hqs[1]->GetVisualFiguresCount(Job(job)), 0u);
    }

    executeLua("wares = {[GD_HAMMER]=8,[GD_AXE]=6,[GD_SAW]=3}\n"
               "people = {[JOB_HELPER] = 30,[JOB_WOODCUTTER] = 6,[JOB_FISHER] = 0,[JOB_FORESTER] = 2}");
    executeLua("player:AddWares(wares)");
    BOOST_REQUIRE_EQUAL(player.GetInventory().goods[GD_HAMMER], 8u);
    BOOST_REQUIRE_EQUAL(player.GetInventory().goods[GD_AXE], 6u);
    BOOST_REQUIRE_EQUAL(player.GetInventory().goods[GD_SAW], 3u);
    BOOST_REQUIRE_EQUAL(hqs[1]->GetRealWaresCount(GD_HAMMER), 8u);
    BOOST_REQUIRE_EQUAL(hqs[1]->GetRealWaresCount(GD_AXE), 6u);
    BOOST_REQUIRE_EQUAL(hqs[1]->GetRealWaresCount(GD_SAW), 3u);
    executeLua("player:AddPeople(people)");
    BOOST_REQUIRE_EQUAL(player.GetInventory().people[JOB_HELPER], 30u);
    BOOST_REQUIRE_EQUAL(player.GetInventory().people[JOB_WOODCUTTER], 6u);
    BOOST_REQUIRE_EQUAL(player.GetInventory().people[JOB_FISHER], 0u);
    BOOST_REQUIRE_EQUAL(player.GetInventory().people[JOB_FORESTER], 2u);
    BOOST_REQUIRE_EQUAL(hqs[1]->GetRealFiguresCount(JOB_HELPER), 30u);
    BOOST_REQUIRE_EQUAL(hqs[1]->GetRealFiguresCount(JOB_WOODCUTTER), 6u);
    BOOST_REQUIRE_EQUAL(hqs[1]->GetRealFiguresCount(JOB_FISHER), 0u);
    BOOST_REQUIRE_EQUAL(hqs[1]->GetRealFiguresCount(JOB_FORESTER), 2u);

    BOOST_CHECK(isLuaEqual("player:GetWareCount(GD_HAMMER)", "8"));
    BOOST_CHECK(isLuaEqual("player:GetWareCount(GD_AXE)", "6"));

    BOOST_CHECK(isLuaEqual("player:GetPeopleCount(JOB_HELPER)", "30"));
    BOOST_CHECK(isLuaEqual("player:GetPeopleCount(JOB_FORESTER)", "2"));

    BOOST_CHECK(isLuaEqual("player:GetBuildingCount(BLD_WOODCUTTER)", "0"));
    BOOST_CHECK(isLuaEqual("player:GetBuildingCount(BLD_HEADQUARTERS)", "1"));

    // TODO: Test AIConstructionOrder(x, y, buildingtype)
    
    executeLua("player:ModifyHQ(true)");
    BOOST_REQUIRE(hqs[1]->IsTent());
    executeLua("player:ModifyHQ(false)");
    BOOST_REQUIRE(!hqs[1]->IsTent());

    executeLua("hqX, hqY = player:GetHQPos()");
    BOOST_CHECK(isLuaEqual("hqX", helpers::toString(hqs[1]->GetPos().x)));
    BOOST_CHECK(isLuaEqual("hqY", helpers::toString(hqs[1]->GetPos().y)));
}

BOOST_AUTO_TEST_CASE(World)
{
    initWorld();
    executeLua("world = rttr:GetWorld()");
    
    const MapPoint envPt(15, 12);
    const MapPoint hqPos(hqs[1]->GetPos());
    executeLua(boost::format("world:AddEnvObject(%1%, %2%, 500)") % envPt.x % envPt.y);
    const noEnvObject* obj = world.GetSpecObj<noEnvObject>(envPt);
    BOOST_REQUIRE(obj);
    BOOST_REQUIRE_EQUAL(obj->GetItemID(), 500u);
    BOOST_REQUIRE_EQUAL(obj->GetItemFile(), 0xFFFFu);
    // Replace and test wrap around
    const MapPoint envPt2(envPt.x + world.GetWidth(), envPt.y - world.GetHeight());
    executeLua(boost::format("world:AddEnvObject(%1%, %2%, 1, 2)") % envPt2.x % envPt2.y);
    obj = world.GetSpecObj<noEnvObject>(envPt);
    BOOST_REQUIRE(obj);
    BOOST_REQUIRE_EQUAL(obj->GetItemID(), 1u);
    BOOST_REQUIRE_EQUAL(obj->GetItemFile(), 2u);
    // Can't replace buildings
    executeLua(boost::format("world:AddEnvObject(%1%, %2%, 1, 2)") % hqPos.x % hqPos.y);
    BOOST_REQUIRE(!world.GetSpecObj<noEnvObject>(hqPos));

    executeLua(boost::format("world:AddStaticObject(%1%, %2%, 501)") % envPt2.x % envPt2.y);
    const noStaticObject* obj2 = world.GetSpecObj<noStaticObject>(envPt);
    BOOST_REQUIRE(obj2);
    BOOST_REQUIRE_EQUAL(obj2->GetGOT(), GOT_STATICOBJECT);
    BOOST_REQUIRE_EQUAL(obj2->GetItemID(), 501u);
    BOOST_REQUIRE_EQUAL(obj2->GetItemFile(), 0xFFFFu);
    BOOST_REQUIRE_EQUAL(obj2->GetSize(), 0u);
    executeLua(boost::format("world:AddStaticObject(%1%, %2%, 5, 3, 2)") % envPt2.x % envPt2.y);
    obj2 = world.GetSpecObj<noStaticObject>(envPt);
    BOOST_REQUIRE(obj2);
    BOOST_REQUIRE_EQUAL(obj2->GetGOT(), GOT_STATICOBJECT);
    BOOST_REQUIRE_EQUAL(obj2->GetItemID(), 5u);
    BOOST_REQUIRE_EQUAL(obj2->GetItemFile(), 3u);
    BOOST_REQUIRE_EQUAL(obj2->GetSize(), 2u);
    // Invalid Size
    BOOST_REQUIRE_THROW(executeLua(boost::format("world:AddStaticObject(%1%, %2%, 5, 3, 3)") % envPt2.x % envPt2.y), std::runtime_error);
    // Can't replace buildings
    executeLua(boost::format("world:AddEnvObject(%1%, %2%, 1, 2)") % hqPos.x % hqPos.y);
    BOOST_REQUIRE(!world.GetSpecObj<noStaticObject>(hqPos));
}

BOOST_AUTO_TEST_SUITE_END()