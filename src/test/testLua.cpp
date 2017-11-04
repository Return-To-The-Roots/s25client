// Copyright (c) 2016 - 2017 Settlers Freaks (sf-team at siedler25.org)
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
#include "ClientInterface.h"
#include "GameClient.h"
#include "GameMessages.h"
#include "PointOutput.h"
#include "buildings/noBuildingSite.h"
#include "buildings/nobHQ.h"
#include "helpers/Deleter.h"
#include "helpers/converters.h"
#include "notifications/BuildingNote.h"
#include "postSystem/PostBox.h"
#include "postSystem/PostMsg.h"
#include "nodeObjs/noAnimal.h"
#include "nodeObjs/noEnvObject.h"
#include "nodeObjs/noStaticObject.h"
#include "gameTypes/Resource.h"
#include "test/GameWorldWithLuaAccess.h"
#include "test/initTestHelpers.h"
#include "libutil/tmpFile.h"
#include <boost/assign/std/vector.hpp>
#include <boost/format.hpp>
#include <boost/interprocess/smart_ptr/unique_ptr.hpp>
#include <boost/test/unit_test.hpp>

using namespace boost::assign;

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
    luaFile.getStream() << script;
    luaFile.close();
    BOOST_REQUIRE(lua.LoadScript(luaFile.filePath));
    BOOST_REQUIRE_EQUAL(lua.GetScript(), script);

    // Test failing load
    GLOBALVARS.isTest = false;
    // Invalid code
    script = "assertTypo(rue)";
    BOOST_REQUIRE(!lua.LoadScriptString(script));
    BOOST_REQUIRE_EQUAL(lua.GetScript(), "");
    TmpFile luaFile2(".lua");
    luaFile2.getStream() << script;
    luaFile2.close();
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
    executeLua(boost::format("function getRequiredLuaVersion()\n return %1%\n end") % (lua.GetVersion() + 1));
    BOOST_REQUIRE(!lua.CheckScriptVersion());
    // Correct version
    executeLua(boost::format("function getRequiredLuaVersion()\n return %1%\n end") % lua.GetVersion());
    BOOST_REQUIRE(lua.CheckScriptVersion());

    BOOST_CHECK(isLuaEqual("rttr:GetFeatureLevel()", helpers::toString(lua.GetFeatureLevel())));

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

struct LocaleResetter
{
    const char* oldLoc;
    LocaleResetter(const char* newLoc) : oldLoc(mysetlocale(LC_ALL, newLoc)) {}
    ~LocaleResetter() { mysetlocale(LC_ALL, oldLoc); }
};

BOOST_AUTO_TEST_CASE(Translations)
{
    // Return same id if nothing set
    executeLua("rttr:Log(_('Foo'))");
    BOOST_REQUIRE_EQUAL(getLog(), "Foo\n");
    // Return translation for default locale
    executeLua("rttr:RegisterTranslations({ en_GB = { Foo = 'Eng', Bar = 'Eng2' } })");
    executeLua("rttr:Log(_('Foo'))");
    BOOST_REQUIRE_EQUAL(getLog(), "Eng\n");
    // Return translation for language or default
    std::string localSetting =
      "rttr:RegisterTranslations({ en_GB = { Foo = 'Eng', Bar = 'Eng2' }, pt = { Foo = 'Port', Bar = 'Port2' }, pt_BR = { Foo = 'PortBR', "
      "Bar = 'PortBR2' } })";
    // With region
    {
        LocaleResetter loc("pt_BR");
        executeLua(localSetting);
        executeLua("rttr:Log(_('Foo'))");
        BOOST_REQUIRE_EQUAL(getLog(), "PortBR\n");
    }
    // Other region
    {
        LocaleResetter loc("pt_PT");
        executeLua(localSetting);
        executeLua("rttr:Log(_('Foo'))");
        BOOST_REQUIRE_EQUAL(getLog(), "Port\n");
    }
    // Non-Translated lang
    {
        LocaleResetter loc("de");
        executeLua(localSetting);
        executeLua("rttr:Log(_('Foo'))");
        BOOST_REQUIRE_EQUAL(getLog(), "Eng\n");
    }
}

namespace {
struct StoreChat : public ClientInterface
{
    unsigned lastPlayerId;
    ChatDestination lastCD;
    std::string lastMsg;

    StoreChat()
    {
        Clear();
        GAMECLIENT.SetInterface(this);
    }

    ~StoreChat() { GAMECLIENT.RemoveInterface(this); }

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
} // namespace

BOOST_AUTO_TEST_CASE(GameFunctions)
{
    initWorld();
    boost::array<nobHQ*, 2> hqs;
    hqs[0] = world.GetSpecObj<nobHQ>(world.GetPlayer(0).GetHQPos());
    hqs[1] = world.GetSpecObj<nobHQ>(world.GetPlayer(1).GetHQPos());

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

    for(unsigned i = 0; i < 2; i++)
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
    BOOST_REQUIRE_EQUAL(postBox.GetMsg(1)->GetPos(), MapPoint(35 - world.GetWidth(), -5 + world.GetHeight()));

    executeLua("assert(rttr:GetPlayer(0))");
    executeLua("assert(rttr:GetPlayer(1))");
    executeLua("assert(rttr:GetPlayer(2))");
    // Invalid player
    BOOST_REQUIRE_THROW(executeLua("assert(rttr:GetPlayer(3))"), std::runtime_error);
    BOOST_REQUIRE_THROW(executeLua("assert(rttr:GetPlayer(-1))"), std::runtime_error);

    executeLua("assert(rttr:GetWorld())");
}

BOOST_AUTO_TEST_CASE(MissionGoal)
{
    initWorld();

    const PostBox& postBox = *world.GetPostMgr().AddPostBox(1);
    BOOST_REQUIRE(postBox.GetCurrentMissionGoal().empty());

    // Set goal for non-existing or other player
    executeLua("rttr:SetMissionGoal(99, 'Goal')");
    BOOST_REQUIRE(postBox.GetCurrentMissionGoal().empty());
    executeLua("rttr:SetMissionGoal(0, 'Goal')");
    BOOST_REQUIRE(postBox.GetCurrentMissionGoal().empty());

    // Set correctly
    executeLua("rttr:SetMissionGoal(1, 'Goal')");
    BOOST_REQUIRE_EQUAL(postBox.GetCurrentMissionGoal(), "Goal");

    // Delete current goal
    executeLua("rttr:SetMissionGoal(1, '')");
    BOOST_REQUIRE(postBox.GetCurrentMissionGoal().empty());
    // Set and delete with default arg -> clear
    executeLua("rttr:SetMissionGoal(1, 'Goal')");
    executeLua("rttr:SetMissionGoal(1)");
    BOOST_REQUIRE(postBox.GetCurrentMissionGoal().empty());
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

namespace {
struct CatchConstructionNote
{
    boost::interprocess::unique_ptr<BuildingNote, Deleter<BuildingNote> > note_;
    Subscribtion sub;

    CatchConstructionNote(GameWorldGame& world) : sub(world.GetNotifications().subscribe<BuildingNote>(boost::ref(*this))) {}

    void operator()(const BuildingNote& note) { note_.reset(new BuildingNote(note)); }
};
} // namespace

BOOST_AUTO_TEST_CASE(IngamePlayer)
{
    initWorld();

    const GamePlayer& player = world.GetPlayer(1);
    const nobHQ* hq = world.GetSpecObj<nobHQ>(player.GetHQPos());
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

    executeLua("player:ClearResources()");
    for(unsigned gd = 0; gd < WARE_TYPES_COUNT; gd++)
    {
        BOOST_REQUIRE_EQUAL(hq->GetRealWaresCount(GoodType(gd)), 0u);
        BOOST_REQUIRE_EQUAL(hq->GetVisualWaresCount(GoodType(gd)), 0u);
    }
    for(unsigned job = 0; job < JOB_TYPES_COUNT; job++)
    {
        BOOST_REQUIRE_EQUAL(hq->GetRealFiguresCount(Job(job)), 0u);
        BOOST_REQUIRE_EQUAL(hq->GetVisualFiguresCount(Job(job)), 0u);
    }

    executeLua("wares = {[GD_HAMMER]=8,[GD_AXE]=6,[GD_SAW]=3}\n"
               "people = {[JOB_HELPER] = 30,[JOB_WOODCUTTER] = 6,[JOB_FISHER] = 0,[JOB_FORESTER] = 2}");
    executeLua("assert(player:AddWares(wares))");
    BOOST_REQUIRE_EQUAL(player.GetInventory().goods[GD_HAMMER], 8u);
    BOOST_REQUIRE_EQUAL(player.GetInventory().goods[GD_AXE], 6u);
    BOOST_REQUIRE_EQUAL(player.GetInventory().goods[GD_SAW], 3u);
    BOOST_CHECK_EQUAL(hq->GetRealWaresCount(GD_HAMMER), 8u);
    BOOST_CHECK_EQUAL(hq->GetRealWaresCount(GD_AXE), 6u);
    BOOST_CHECK_EQUAL(hq->GetRealWaresCount(GD_SAW), 3u);
    executeLua("assert(player:AddPeople(people))");
    BOOST_REQUIRE_EQUAL(player.GetInventory().people[JOB_HELPER], 30u);
    BOOST_REQUIRE_EQUAL(player.GetInventory().people[JOB_WOODCUTTER], 6u);
    BOOST_REQUIRE_EQUAL(player.GetInventory().people[JOB_FISHER], 0u);
    BOOST_REQUIRE_EQUAL(player.GetInventory().people[JOB_FORESTER], 2u);
    BOOST_CHECK_EQUAL(hq->GetRealFiguresCount(JOB_HELPER), 30u);
    BOOST_CHECK_EQUAL(hq->GetRealFiguresCount(JOB_WOODCUTTER), 6u);
    BOOST_CHECK_EQUAL(hq->GetRealFiguresCount(JOB_FISHER), 0u);
    BOOST_CHECK_EQUAL(hq->GetRealFiguresCount(JOB_FORESTER), 2u);

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
    world.SetNO(hq->GetPos() + MapPoint(4, 0), new noBuildingSite(BLD_WOODCUTTER, hq->GetPos() + MapPoint(4, 0), 1));
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
    BOOST_REQUIRE(hq->IsTent());
    executeLua("player:ModifyHQ(false)");
    BOOST_REQUIRE(!hq->IsTent());

    executeLua("hqX, hqY = player:GetHQPos()");
    BOOST_CHECK(isLuaEqual("hqX", helpers::toString(hq->GetPos().x)));
    BOOST_CHECK(isLuaEqual("hqY", helpers::toString(hq->GetPos().y)));

    // Destroy players HQ
    world.DestroyNO(hq->GetPos());
    // HQ-Pos is invalid
    executeLua("hqX, hqY = player:GetHQPos()");
    BOOST_CHECK(isLuaEqual("hqX", helpers::toString(MapPoint::Invalid().x)));
    BOOST_CHECK(isLuaEqual("hqY", helpers::toString(MapPoint::Invalid().y)));
    // Adding wares/people returns false
    executeLua("assert(player:AddWares(wares) == false)");
    executeLua("assert(player:AddPeople(people) == false)");
    // ModifyHQ does not crash
    executeLua("player:ModifyHQ(true)");

    const GamePlayer& player0 = world.GetPlayer(0);
    executeLua("player = rttr:GetPlayer(0)");
    MapPoint hqPos = player0.GetHQPos();
    BOOST_REQUIRE(hqPos.isValid() && world.GetSpecObj<nobHQ>(hqPos));
    BOOST_REQUIRE(!player0.IsDefeated());
    BOOST_CHECK(isLuaEqual("player:IsDefeated()", "false"));
    executeLua("player:Surrender(false)");
    BOOST_REQUIRE(player0.IsDefeated());
    BOOST_CHECK(isLuaEqual("player:IsDefeated()", "true"));
    // HQ should still be there
    BOOST_REQUIRE(player0.GetHQPos().isValid() && world.GetSpecObj<nobHQ>(hqPos));
    // Destroy everything
    executeLua("player:Surrender(true)");
    BOOST_REQUIRE(!player0.GetHQPos().isValid() && !world.GetSpecObj<nobHQ>(hqPos));
}

BOOST_AUTO_TEST_CASE(RestrictedArea)
{
    initWorld();

    const GamePlayer& player = world.GetPlayer(1);
    executeLua("player = rttr:GetPlayer(1)\nassert(player)");

    // Just a single polygon
    executeLua("player:SetRestrictedArea(5,7, 5,12, 15,12)");
    std::vector<MapPoint> expectedRestrictedArea;
    expectedRestrictedArea += MapPoint(5, 7), MapPoint(5, 12), MapPoint(15, 12);
    RTTR_REQUIRE_EQUAL_COLLECTIONS(player.GetRestrictedArea(), expectedRestrictedArea);

    // Polygon with hole
    executeLua("player:SetRestrictedArea(0,0, 1,1, 10,1, 10,10, 1,10, 1,1, 0,0, 5,5, 7,5, 7,7, 5,5, 0,0)");
    expectedRestrictedArea.clear();
    expectedRestrictedArea += MapPoint(0, 0), MapPoint(1, 1), MapPoint(10, 1), MapPoint(10, 10), MapPoint(1, 10), MapPoint(1, 1);
    expectedRestrictedArea += MapPoint(0, 0), MapPoint(5, 5), MapPoint(7, 5), MapPoint(7, 7), MapPoint(5, 5), MapPoint(0, 0);
    RTTR_REQUIRE_EQUAL_COLLECTIONS(player.GetRestrictedArea(), expectedRestrictedArea);
    // Some prefer using nils
    executeLua("player:SetRestrictedArea(nil,nil, 1,1, 10,1, 10,10, 1,10, 1,1, nil,nil, 5,5, 7,5, 7,7, 5,5, nil,nil)");
    RTTR_REQUIRE_EQUAL_COLLECTIONS(player.GetRestrictedArea(), expectedRestrictedArea);

    // New API: Single nil and no double point
    executeLua("player:SetRestrictedArea(1,1, 10,1, 10,10, 1,10, nil, 5,5, 7,5, 7,7)");
    RTTR_REQUIRE_EQUAL_COLLECTIONS(player.GetRestrictedArea(), expectedRestrictedArea);
    // Although you could use nil at the end...
    executeLua("player:SetRestrictedArea(1,1, 10,1, 10,10, 1,10, nil, 5,5, 7,5, 7,7, nil)");
    RTTR_REQUIRE_EQUAL_COLLECTIONS(player.GetRestrictedArea(), expectedRestrictedArea);
    // ...or beginning
    executeLua("player:SetRestrictedArea(nil,1,1, 10,1, 10,10, 1,10, nil, 5,5, 7,5, 7,7)");
    RTTR_REQUIRE_EQUAL_COLLECTIONS(player.GetRestrictedArea(), expectedRestrictedArea);

    // Also for single polygon
    executeLua("player:SetRestrictedArea(5,7, 5,12, 15,12, nil)");
    expectedRestrictedArea.clear();
    expectedRestrictedArea += MapPoint(5, 7), MapPoint(5, 12), MapPoint(15, 12);
    RTTR_REQUIRE_EQUAL_COLLECTIONS(player.GetRestrictedArea(), expectedRestrictedArea);

    executeLua("player:SetRestrictedArea()");
    BOOST_REQUIRE_EQUAL(player.GetRestrictedArea().size(), 0u);

    // Numbers must be pairs
    BOOST_REQUIRE_THROW(executeLua("player:SetRestrictedArea(1,2, 3)"), std::runtime_error);
    // And non negative
    BOOST_REQUIRE_THROW(executeLua("player:SetRestrictedArea(1,2, -3,4)"), std::runtime_error);
    BOOST_REQUIRE_THROW(executeLua("player:SetRestrictedArea(1,2, 3,-4)"), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(World)
{
    initWorld();
    executeLua("world = rttr:GetWorld()");

    const MapPoint envPt(15, 12);
    const MapPoint hqPos(world.GetPlayer(1).GetHQPos());
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
    BOOST_REQUIRE_EQUAL(obj2->GetSize(), 1u);
    // ID and File (replace env obj)
    executeLua(boost::format("world:AddStaticObject(%1%, %2%, 5, 3)") % envPt2.x % envPt2.y);
    obj2 = world.GetSpecObj<noStaticObject>(envPt);
    BOOST_REQUIRE(obj2);
    BOOST_REQUIRE_EQUAL(obj2->GetGOT(), GOT_STATICOBJECT);
    BOOST_REQUIRE_EQUAL(obj2->GetItemID(), 5u);
    BOOST_REQUIRE_EQUAL(obj2->GetItemFile(), 3u);
    BOOST_REQUIRE_EQUAL(obj2->GetSize(), 1u);
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

    MapPoint animalPos(20, 12);
    const std::list<noBase*>& figs = world.GetFigures(animalPos);
    BOOST_REQUIRE(figs.empty());
    executeLua(boost::format("world:AddAnimal(%1%, %2%, SPEC_DEER)") % animalPos.x % animalPos.y);
    BOOST_REQUIRE_EQUAL(figs.size(), 1u);
    const noAnimal* animal = dynamic_cast<noAnimal*>(figs.front());
    BOOST_REQUIRE(animal);
    BOOST_REQUIRE_EQUAL(animal->GetSpecies(), SPEC_DEER);
    executeLua(boost::format("world:AddAnimal(%1%, %2%, SPEC_FOX)") % animalPos.x % animalPos.y);
    BOOST_REQUIRE_EQUAL(figs.size(), 2u);
    animal = dynamic_cast<noAnimal*>(figs.back());
    BOOST_REQUIRE(animal);
    BOOST_REQUIRE_EQUAL(animal->GetSpecies(), SPEC_FOX);
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
    lua.EventResourceFound(1, pt3, Resource::Gold, 1);
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

    executeLua(
      "function onExplored(player_id, x, y, owner)\n  rttr:Log('explored: '..player_id..'('..x..', '..y..')'..tostring(owner))\nend");
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

    executeLua("function onResourceFound(player_id, x, y, type, quantity)\n  rttr:Log('resFound: '..player_id..'('..x..', "
               "'..y..')'..getResName(type)..':'..quantity)\nend");
    boost::format resFmt("resFound: %1%%2%%3%:%4%\n");
    lua.EventResourceFound(2, pt3, Resource::Iron, 1);
    BOOST_REQUIRE_EQUAL(getLog(), (resFmt % 2 % pt3 % "Iron" % 1).str());
    lua.EventResourceFound(2, pt3, Resource::Gold, 2);
    BOOST_REQUIRE_EQUAL(getLog(), (resFmt % 2 % pt3 % "Gold" % 2).str());
    lua.EventResourceFound(2, pt3, Resource::Coal, 3);
    BOOST_REQUIRE_EQUAL(getLog(), (resFmt % 2 % pt3 % "Coal" % 3).str());
    lua.EventResourceFound(2, pt3, Resource::Granite, 6);
    BOOST_REQUIRE_EQUAL(getLog(), (resFmt % 2 % pt3 % "Granite" % 6).str());
    lua.EventResourceFound(2, pt3, Resource::Water, 5);
    BOOST_REQUIRE_EQUAL(getLog(), (resFmt % 2 % pt3 % "Water" % 5).str());
}

BOOST_AUTO_TEST_SUITE_END()
