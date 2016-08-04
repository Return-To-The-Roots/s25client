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
#include "buildings/noBuildingSite.h"
#include "buildings/nobHQ.h"
#include "nodeObjs/noEnvObject.h"
#include "nodeObjs/noStaticObject.h"
#include "notifications/BuildingNote.h"
#include "postSystem/PostMsg.h"
#include "gameTypes/Resource.h"
#include "world/MapLoader.h"
#include "world/GameWorldGame.h"
#include "helpers/converters.h"
#include "helpers/Deleter.h"
#include "test/PointOutput.h"
#include "test/testHelpers.h"
#include "libutil/src/colors.h"
#include "libutil/src/Log.h"
#include "libutil/src/StringStreamWriter.h"
#include "libutil/src/tmpFile.h"
#include <boost/test/unit_test.hpp>
#include <boost/interprocess/smart_ptr/unique_ptr.hpp>
#include <boost/format.hpp>
#include <vector>

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

BOOST_AUTO_TEST_CASE(ScriptLoading)
{
    std::string script = "assert(true)";
    LuaInterfaceGame& lua = world.GetLua();
    BOOST_REQUIRE(lua.LoadScriptString(script));
    BOOST_REQUIRE_EQUAL(lua.GetScript(), script);
    // Load from file
    TmpFile luaFile(".lua");
    script += "assert(42==42)";
    luaFile.GetStream() << script;
    luaFile.GetStream().close();
    BOOST_REQUIRE(lua.LoadScript(luaFile.filePath));
    BOOST_REQUIRE_EQUAL(lua.GetScript(), script);

    // Test failing load
    GLOBALVARS.isTest = false;
    // Invalid code
    script = "assertTypo(rue)";
    BOOST_REQUIRE(!lua.LoadScriptString(script));
    BOOST_REQUIRE_EQUAL(lua.GetScript(), "");
    TmpFile luaFile2(".lua");
    luaFile2.GetStream() << script;
    luaFile2.GetStream().close();
    BOOST_REQUIRE(!lua.LoadScript(luaFile2.filePath));
    BOOST_REQUIRE_EQUAL(lua.GetScript(), "");

    GLOBALVARS.isTest = true;
    BOOST_REQUIRE_THROW(lua.LoadScriptString(script), std::runtime_error);
    BOOST_REQUIRE_THROW(lua.LoadScript(luaFile2.filePath), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(BaseFunctions)
{
    executeLua("rttr:Log('Hello World')");
    BOOST_REQUIRE_EQUAL(getLog(), "Hello World\n");

    // No getRequiredLuaVersion
    LuaInterfaceBase& lua = world.GetLua();
    BOOST_REQUIRE(!lua.CheckScriptVersion());
    // Wrong version
    executeLua("function getRequiredLuaVersion()\n return 0\n end");
    BOOST_REQUIRE(!lua.CheckScriptVersion());
    executeLua(boost::format("function getRequiredLuaVersion()\n return %1%\n end") % (lua.GetVersion().first + 1));
    BOOST_REQUIRE(!lua.CheckScriptVersion());
    // Correct version
    executeLua(boost::format("function getRequiredLuaVersion()\n return %1%\n end") % lua.GetVersion().first);
    BOOST_REQUIRE(lua.CheckScriptVersion());

    executeLua("vMajor, vMinor = rttr:GetVersion()");
    BOOST_CHECK(isLuaEqual("vMajor", helpers::toString(lua.GetVersion().first)));
    BOOST_CHECK(isLuaEqual("vMinor", helpers::toString(lua.GetVersion().second)));

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
    BOOST_CHECK(isLuaEqual("player:IsHuman()", "true"));
    BOOST_CHECK(isLuaEqual("player:IsAI()", "false"));
    BOOST_CHECK(isLuaEqual("player:IsClosed()", "false"));
    BOOST_CHECK(isLuaEqual("player:IsFree()", "false"));
    BOOST_CHECK(isLuaEqual("player:GetAILevel()", "-1"));

    executeLua("player = rttr:GetPlayer(1)");
    BasePlayerInfo& player = world.GetPlayer(1);
    BOOST_CHECK(isLuaEqual("player:GetName()", "'PlayerAI'"));
    BOOST_CHECK(isLuaEqual("player:GetNation()", "NAT_ROMANS"));
    BOOST_CHECK(isLuaEqual("player:GetTeam()", "TM_TEAM2"));
    BOOST_CHECK(isLuaEqual("player:GetColor()", helpers::toString(0xFFFF0000)));
    BOOST_CHECK(isLuaEqual("player:IsHuman()", "false"));
    BOOST_CHECK(isLuaEqual("player:IsAI()", "true"));
    BOOST_CHECK(isLuaEqual("player:IsClosed()", "false"));
    BOOST_CHECK(isLuaEqual("player:IsFree()", "false"));
    player.aiInfo = AI::Info(AI::DUMMY, AI::MEDIUM);
    BOOST_CHECK(isLuaEqual("player:GetAILevel()", "0"));
    player.aiInfo = AI::Info(AI::DEFAULT, AI::EASY);
    BOOST_CHECK(isLuaEqual("player:GetAILevel()", "1"));
    player.aiInfo = AI::Info(AI::DEFAULT, AI::MEDIUM);
    BOOST_CHECK(isLuaEqual("player:GetAILevel()", "2"));
    player.aiInfo = AI::Info(AI::DEFAULT, AI::HARD);
    BOOST_CHECK(isLuaEqual("player:GetAILevel()", "3"));

    executeLua("player = rttr:GetPlayer(2)");
    BOOST_CHECK(isLuaEqual("player:IsHuman()", "false"));
    BOOST_CHECK(isLuaEqual("player:IsAI()", "false"));
    BOOST_CHECK(isLuaEqual("player:IsClosed()", "true"));
    BOOST_CHECK(isLuaEqual("player:IsFree()", "false"));
}

namespace{
    struct CatchConstructionNote
    {
        boost::interprocess::unique_ptr<BuildingNote, Deleter<BuildingNote> > note_;
        Subscribtion sub;

        CatchConstructionNote(GameWorldGame& world): sub(world.GetNotifications().subscribe<BuildingNote>(boost::ref(*this)))
        {}

        void operator()(const BuildingNote& note)
        {
            note_.reset(new BuildingNote(note));
        }
    };
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
    
    // Numbers must be pairs
    BOOST_REQUIRE_THROW(executeLua("player:SetRestrictedArea(1,2, 3)"), std::runtime_error);
    // And non negative
    BOOST_REQUIRE_THROW(executeLua("player:SetRestrictedArea(1,2, -3,4)"), std::runtime_error);
    BOOST_REQUIRE_THROW(executeLua("player:SetRestrictedArea(1,2, 3,-4)"), std::runtime_error);

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
    executeLua("assert(player:AddWares(wares))");
    BOOST_REQUIRE_EQUAL(player.GetInventory().goods[GD_HAMMER], 8u);
    BOOST_REQUIRE_EQUAL(player.GetInventory().goods[GD_AXE], 6u);
    BOOST_REQUIRE_EQUAL(player.GetInventory().goods[GD_SAW], 3u);
    BOOST_CHECK_EQUAL(hqs[1]->GetRealWaresCount(GD_HAMMER), 8u);
    BOOST_CHECK_EQUAL(hqs[1]->GetRealWaresCount(GD_AXE), 6u);
    BOOST_CHECK_EQUAL(hqs[1]->GetRealWaresCount(GD_SAW), 3u);
    executeLua("assert(player:AddPeople(people))");
    BOOST_REQUIRE_EQUAL(player.GetInventory().people[JOB_HELPER], 30u);
    BOOST_REQUIRE_EQUAL(player.GetInventory().people[JOB_WOODCUTTER], 6u);
    BOOST_REQUIRE_EQUAL(player.GetInventory().people[JOB_FISHER], 0u);
    BOOST_REQUIRE_EQUAL(player.GetInventory().people[JOB_FORESTER], 2u);
    BOOST_CHECK_EQUAL(hqs[1]->GetRealFiguresCount(JOB_HELPER), 30u);
    BOOST_CHECK_EQUAL(hqs[1]->GetRealFiguresCount(JOB_WOODCUTTER), 6u);
    BOOST_CHECK_EQUAL(hqs[1]->GetRealFiguresCount(JOB_FISHER), 0u);
    BOOST_CHECK_EQUAL(hqs[1]->GetRealFiguresCount(JOB_FORESTER), 2u);

    BOOST_CHECK(isLuaEqual("player:GetWareCount(GD_HAMMER)", "8"));
    BOOST_CHECK(isLuaEqual("player:GetWareCount(GD_AXE)", "6"));

    BOOST_CHECK(isLuaEqual("player:GetPeopleCount(JOB_HELPER)", "30"));
    BOOST_CHECK(isLuaEqual("player:GetPeopleCount(JOB_FORESTER)", "2"));

    // Invalid ware/player throws
    BOOST_CHECK_THROW(executeLua("player:AddWares({[9999]=8})"), std::runtime_error);
    BOOST_CHECK_THROW(executeLua("player:AddPeople({[9999]=8})"), std::runtime_error);

    BOOST_CHECK(isLuaEqual("player:GetBuildingCount(BLD_WOODCUTTER)", "0"));
    BOOST_CHECK(isLuaEqual("player:GetBuildingCount(BLD_HEADQUARTERS)", "1"));
    BOOST_CHECK(isLuaEqual("player:GetBuildingSitesCount(BLD_HEADQUARTERS)", "0"));
    BOOST_CHECK(isLuaEqual("player:GetBuildingSitesCount(BLD_WOODCUTTER)", "0"));
    world.SetNO(hqs[1]->GetPos() + MapPoint(4, 0), new noBuildingSite(BLD_WOODCUTTER, hqs[1]->GetPos() + MapPoint(4, 0), 1));
    BOOST_CHECK(isLuaEqual("player:GetBuildingCount(BLD_WOODCUTTER)", "0"));
    BOOST_CHECK(isLuaEqual("player:GetBuildingSitesCount(BLD_WOODCUTTER)", "1"));

    CatchConstructionNote note(world);
    // Closed or non-AI player
    BOOST_REQUIRE(isLuaEqual("rttr:GetPlayer(0):AIConstructionOrder(12, 13, BLD_WOODCUTTER)", "false"));
    BOOST_REQUIRE(isLuaEqual("rttr:GetPlayer(2):AIConstructionOrder(12, 13, BLD_WOODCUTTER)", "false"));
    // Wrong coordinate
    BOOST_REQUIRE_THROW(executeLua("player:AIConstructionOrder(9999, 13, BLD_WOODCUTTER)"), std::runtime_error);
    BOOST_REQUIRE_THROW(executeLua("player:AIConstructionOrder(12, 9999, BLD_WOODCUTTER)"), std::runtime_error);
    // Wrong building
    BOOST_REQUIRE_THROW(executeLua("player:AIConstructionOrder(12, 13, 9999)"), std::runtime_error);
    // Correct
    BOOST_REQUIRE(!note.note_);
    BOOST_REQUIRE(isLuaEqual("player:AIConstructionOrder(12, 13, BLD_WOODCUTTER)", "true"));
    BOOST_REQUIRE(note.note_);
    BOOST_CHECK_EQUAL(note.note_->type, BuildingNote::LuaOrder);
    BOOST_CHECK_EQUAL(note.note_->player, 1u);
    BOOST_CHECK_EQUAL(note.note_->bld, BLD_WOODCUTTER);
    BOOST_CHECK_EQUAL(note.note_->pos, MapPoint(12, 13));

    executeLua("player:ModifyHQ(true)");
    BOOST_REQUIRE(hqs[1]->IsTent());
    executeLua("player:ModifyHQ(false)");
    BOOST_REQUIRE(!hqs[1]->IsTent());

    executeLua("hqX, hqY = player:GetHQPos()");
    BOOST_CHECK(isLuaEqual("hqX", helpers::toString(hqs[1]->GetPos().x)));
    BOOST_CHECK(isLuaEqual("hqY", helpers::toString(hqs[1]->GetPos().y)));

    // Destroy players HQ
    world.DestroyNO(hqs[1]->GetPos());
    // HQ-Pos is invalid
    executeLua("hqX, hqY = player:GetHQPos()");
    BOOST_CHECK(isLuaEqual("hqX", helpers::toString(MapPoint::Invalid().x)));
    BOOST_CHECK(isLuaEqual("hqY", helpers::toString(MapPoint::Invalid().y)));
    // Adding wares/people returns false
    executeLua("assert(player:AddWares(wares) == false)");
    executeLua("assert(player:AddPeople(people) == false)");
    // ModifyHQ does not crash
    executeLua("player:ModifyHQ(true)");
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
    // Replace and test wrap around (envPt2==envPt1)
    const Point<int> envPt2(envPt.x + world.GetWidth(), envPt.y - world.GetHeight());
    BOOST_REQUIRE_EQUAL(world.MakeMapPoint(envPt2), envPt);
    executeLua(boost::format("world:AddEnvObject(%1%, %2%, 1, 2)") % envPt2.x % envPt2.y);
    obj = world.GetSpecObj<noEnvObject>(envPt);
    BOOST_REQUIRE(obj);
    BOOST_REQUIRE_EQUAL(obj->GetItemID(), 1u);
    BOOST_REQUIRE_EQUAL(obj->GetItemFile(), 2u);

    // ID only
    const MapPoint envPt3(envPt.x + 5, envPt.y);
    executeLua(boost::format("world:AddStaticObject(%1%, %2%, 501)") % envPt3.x % envPt3.y);
    const noStaticObject* obj2 = world.GetSpecObj<noStaticObject>(envPt3);
    BOOST_REQUIRE(obj2);
    BOOST_REQUIRE_EQUAL(obj2->GetGOT(), GOT_STATICOBJECT);
    BOOST_REQUIRE_EQUAL(obj2->GetItemID(), 501u);
    BOOST_REQUIRE_EQUAL(obj2->GetItemFile(), 0xFFFFu);
    BOOST_REQUIRE_EQUAL(obj2->GetSize(), 0u);
    // ID and File (replace env obj)
    executeLua(boost::format("world:AddStaticObject(%1%, %2%, 5, 3)") % envPt2.x % envPt2.y);
    obj2 = world.GetSpecObj<noStaticObject>(envPt);
    BOOST_REQUIRE(obj2);
    BOOST_REQUIRE_EQUAL(obj2->GetGOT(), GOT_STATICOBJECT);
    BOOST_REQUIRE_EQUAL(obj2->GetItemID(), 5u);
    BOOST_REQUIRE_EQUAL(obj2->GetItemFile(), 3u);
    BOOST_REQUIRE_EQUAL(obj2->GetSize(), 0u);
    // ID, File and Size (replace static obj)
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
    BOOST_REQUIRE(!world.GetSpecObj<noEnvObject>(hqPos));
    executeLua(boost::format("world:AddStaticObject(%1%, %2%, 1, 2)") % hqPos.x % hqPos.y);
    BOOST_REQUIRE(!world.GetSpecObj<noStaticObject>(hqPos));
    // But environment obj can also replace static one
    executeLua(boost::format("world:AddEnvObject(%1%, %2%, 5, 3)") % envPt2.x % envPt2.y);
    obj2 = world.GetSpecObj<noStaticObject>(envPt);
    BOOST_REQUIRE(obj2);
    BOOST_REQUIRE_EQUAL(obj2->GetGOT(), GOT_ENVOBJECT);
}

BOOST_AUTO_TEST_CASE(WorldEvents)
{
    const MapPoint pt1(3, 4), pt2(5, 1), pt3(7, 6);
    // All events need to work w/o having them
    LuaInterfaceGame& lua = world.GetLua();
    lua.EventStart(true);
    Serializer serData1 = lua.Serialize();
    BOOST_REQUIRE_EQUAL(serData1.GetLength(), 0u);
    BOOST_REQUIRE(lua.Deserialize(serData1));
    lua.EventOccupied(1, pt1);
    lua.EventExplored(1, pt2, 0);
    lua.EventGameFrame(0);
    lua.EventResourceFound(1, pt3, RES_GOLD, 1);
    executeLua("function getResName(res)\n  if(res==RES_IRON) then return 'Iron' "
        "elseif(res==RES_GOLD) then return 'Gold' "
        "elseif(res==RES_COAL) then return 'Coal' "
        "elseif(res==RES_GRANITE) then return 'Granite' "
        "elseif(res==RES_WATER) then return 'Water' "
        "else assert(false)\n  end\n  end");
    executeLua("function onStart(isFirstStart)\n  rttr:Log('start: '..tostring(isFirstStart))\nend");
    clearLog();
    lua.EventStart(true);
    BOOST_REQUIRE_EQUAL(getLog(), "start: true\n");
    lua.EventStart(false);
    BOOST_REQUIRE_EQUAL(getLog(), "start: false\n");

    executeLua("function onSave(serializer)\n"
        "serializer:PushInt(42)\n"
        "serializer:PushBool(true)\n"
        "serializer:PushBool(false)\n"
        "serializer:PushString('Hello World!')\n"
        "rttr:Log('saved')\n  return true\nend");
    Serializer serData2 = lua.Serialize();
    BOOST_REQUIRE_EQUAL(getLog(), "saved\n");
    BOOST_REQUIRE_GT(serData2.GetLength(), 0u);

    // Returning false should not save anything but should also print an error
    executeLua("function onSave(serializer)\n"
        "serializer:PushInt(42)\n  return false\nend");
    Serializer serData3 = lua.Serialize();
    BOOST_REQUIRE_EQUAL(serData3.GetLength(), 0u);
    BOOST_REQUIRE_NE(getLog(), "");

    executeLua("function onLoad(serializer)\n"
        "assert(serializer:PopInt() == 42)\n"
        "assert(serializer:PopBool() == true)\n"
        "assert(serializer:PopBool() == false)\n"
        "assert(serializer:PopString() == 'Hello World!')\n"
        "rttr:Log('loaded')\n  return true\nend");
    BOOST_REQUIRE(lua.Deserialize(serData2));
    BOOST_REQUIRE_EQUAL(getLog(), "loaded\n");
    BOOST_REQUIRE_EQUAL(serData2.GetBytesLeft(), 0u);

    // Throwing an error in onSave/onLoad should be caught
    GLOBALVARS.isTest = false;

    executeLua("function onSave(serializer)\n  serializer:PushInt(42)\n  assert(false)\nend");
    Serializer serData4 = lua.Serialize();
    BOOST_REQUIRE_EQUAL(serData3.GetLength(), 0u);
    BOOST_REQUIRE_NE(getLog(), "");
    // And in test mode it throws
    GLOBALVARS.isTest = true;
    BOOST_REQUIRE_THROW(lua.Serialize(), std::runtime_error);
    GLOBALVARS.isTest = false;

    // Error from C++
    BOOST_REQUIRE(!lua.Deserialize(serData2));
    BOOST_REQUIRE_NE(getLog(), "");
    // Error from Lua
    executeLua("function onLoad(serializer)\n  assert(false)\nend");
    BOOST_REQUIRE(!lua.Deserialize(serData4));
    BOOST_REQUIRE_NE(getLog(), "");
    // And in test mode it throws
    GLOBALVARS.isTest = true;
    BOOST_REQUIRE_THROW(lua.Deserialize(serData4), std::runtime_error);
    GLOBALVARS.isTest = false;

    // False returned
    executeLua("function onLoad(serializer)\n  return false\nend");
    BOOST_REQUIRE(!lua.Deserialize(serData4));
    BOOST_REQUIRE_NE(getLog(), "");
    // Re-enable
    GLOBALVARS.isTest = true;

    executeLua("function onOccupied(player_id, x, y)\n  rttr:Log('occupied: '..player_id..'('..x..', '..y..')')\nend");
    lua.EventOccupied(1, pt1);
    BOOST_REQUIRE_EQUAL(getLog(), (boost::format("occupied: %1%%2%\n") % 1 % pt1).str());

    executeLua("function onExplored(player_id, x, y, owner)\n  rttr:Log('explored: '..player_id..'('..x..', '..y..')'..tostring(owner))\nend");
    // Owner == 0 -> No owner (nil in lua)
    lua.EventExplored(0, pt2, 0);
    BOOST_REQUIRE_EQUAL(getLog(), (boost::format("explored: %1%%2%%3%\n") % 0 % pt2 % "nil").str());
    // Else owner-1 = playerIdx
    lua.EventExplored(0, pt2, 2);
    BOOST_REQUIRE_EQUAL(getLog(), (boost::format("explored: %1%%2%%3%\n") % 0 % pt2 % 1).str());

    executeLua("function onGameFrame(gameframe_number)\n  rttr:Log('gf: '..gameframe_number)\nend");
    lua.EventGameFrame(0);
    lua.EventGameFrame(42);
    BOOST_REQUIRE_EQUAL(getLog(), "gf: 0\ngf: 42\n");

    executeLua("function onResourceFound(player_id, x, y, type, quantity)\n  rttr:Log('resFound: '..player_id..'('..x..', '..y..')'..getResName(type)..':'..quantity)\nend");
    boost::format resFmt("resFound: %1%%2%%3%:%4%\n");
    lua.EventResourceFound(2, pt3, RES_IRON_ORE, 1);
    BOOST_REQUIRE_EQUAL(getLog(), (resFmt % 2 % pt3 % "Iron" % 1).str());
    lua.EventResourceFound(2, pt3, RES_GOLD, 2);
    BOOST_REQUIRE_EQUAL(getLog(), (resFmt % 2 % pt3 % "Gold" % 2).str());
    lua.EventResourceFound(2, pt3, RES_COAL, 3);
    BOOST_REQUIRE_EQUAL(getLog(), (resFmt % 2 % pt3 % "Coal" % 3).str());
    lua.EventResourceFound(2, pt3, RES_GRANITE, 6);
    BOOST_REQUIRE_EQUAL(getLog(), (resFmt % 2 % pt3 % "Granite" % 6).str());
    lua.EventResourceFound(2, pt3, RES_WATER, 5);
    BOOST_REQUIRE_EQUAL(getLog(), (resFmt % 2 % pt3 % "Water" % 5).str());
}

BOOST_AUTO_TEST_SUITE_END()

