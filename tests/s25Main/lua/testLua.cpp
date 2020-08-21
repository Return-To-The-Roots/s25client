// Copyright (c) 2016 - 2020 Settlers Freaks (sf-team at siedler25.org)
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

#include "GameWithLuaAccess.h"
#include "PointOutput.h"
#include "RttrForeachPt.h"
#include "buildings/noBuildingSite.h"
#include "buildings/nobHQ.h"
#include "enum_cast.hpp"
#include "helpers/EnumRange.h"
#include "lua/LuaTraits.h" // IWYU pragma: keep
#include "notifications/BuildingNote.h"
#include "postSystem/DiplomacyPostQuestion.h"
#include "postSystem/PostBox.h"
#include "postSystem/PostMsg.h"
#include "nodeObjs/noAnimal.h"
#include "nodeObjs/noEnvObject.h"
#include "nodeObjs/noStaticObject.h"
#include "gameTypes/Resource.h"
#include "s25util/Serializer.h"
#include "s25util/StringConversion.h"
#include "s25util/tmpFile.h"
#include <rttr/test/LocaleResetter.hpp>
#include <rttr/test/testHelpers.hpp>
#include <turtle/mock.hpp>
#include <boost/test/unit_test.hpp>
#include <map>
#include <memory>
#include <utility>
#include <vector>

static std::ostream& operator<<(std::ostream& os, Species spec)
{
    return os << unsigned(rttr::enum_cast(spec));
}

BOOST_FIXTURE_TEST_SUITE(LuaTestSuite, LuaTestsFixture)

BOOST_AUTO_TEST_CASE(LuaEqual_IsCorrect)
{
    executeLua("foo=42");
    BOOST_TEST(isLuaEqual("foo", "42"));
    BOOST_TEST(isLuaEqual("foo", "1337").message().str() == "Value = 42");
    BOOST_TEST(isLuaEqual("nonexisting.method()", "1337").message().str().find("nonexisting") != std::string::npos);
    clearLog();
}

BOOST_AUTO_TEST_CASE(AssertionThrows)
{
    BOOST_REQUIRE_THROW(executeLua("assert(false)"), LuaExecutionError);
    BOOST_REQUIRE_NE(getLog(), "");
}

BOOST_AUTO_TEST_CASE(ScriptLoading)
{
    LogAccessor logAcc;
    std::string script = "assert(true)";
    LuaInterfaceGame& lua = world.GetLua();
    BOOST_REQUIRE(lua.loadScriptString(script));
    BOOST_REQUIRE_EQUAL(lua.getScript(), script);
    // Load from file
    TmpFile luaFile(".lua");
    script += "assert(42==42)";
    luaFile.getStream() << script;
    luaFile.close();
    BOOST_REQUIRE(lua.loadScript(luaFile.filePath));
    BOOST_REQUIRE_EQUAL(lua.getScript(), script);

    // Test failing load
    // Invalid code
    script = "assertTypo(rue)";
    BOOST_REQUIRE(!lua.loadScriptString(script));
    BOOST_REQUIRE_EQUAL(lua.getScript(), "");
    RTTR_REQUIRE_LOG_CONTAINS("assertTypo", false);
    TmpFile luaFile2(".lua");
    luaFile2.getStream() << script;
    luaFile2.close();
    BOOST_REQUIRE(!lua.loadScript(luaFile2.filePath));
    BOOST_REQUIRE_EQUAL(lua.getScript(), "");
    RTTR_REQUIRE_LOG_CONTAINS("assertTypo", false);

    script = "msg1='foo'\nmsg2='bar'\nmsg3='\xF1'\nmsg4='ok'";
    BOOST_REQUIRE(!lua.loadScriptString(script));
    RTTR_REQUIRE_LOG_CONTAINS("invalid UTF8 char at line 3", false);
}

BOOST_AUTO_TEST_CASE(BaseFunctions)
{
    executeLua("rttr:Log('Hello World')");
    BOOST_REQUIRE_EQUAL(getLog(), "Hello World\n");

    // No getRequiredLuaVersion
    LuaInterfaceGameBase& lua = world.GetLua();
    BOOST_REQUIRE(!lua.CheckScriptVersion());
    BOOST_REQUIRE_NE(getLog(), "");
    // Wrong version
    executeLua("function getRequiredLuaVersion()\n return 0\n end");
    BOOST_REQUIRE(!lua.CheckScriptVersion());
    BOOST_REQUIRE_NE(getLog(), "");
    executeLua(boost::format("function getRequiredLuaVersion()\n return %1%\n end")
               % (LuaInterfaceGameBase::GetVersion() + 1));
    BOOST_REQUIRE(!lua.CheckScriptVersion());
    BOOST_REQUIRE_NE(getLog(), "");
    // Correct version
    executeLua(boost::format("function getRequiredLuaVersion()\n return %1%\n end")
               % LuaInterfaceGameBase::GetVersion());
    BOOST_REQUIRE(lua.CheckScriptVersion());

    BOOST_CHECK(isLuaEqual("rttr:GetFeatureLevel()", s25util::toStringClassic(lua.GetFeatureLevel())));

    MOCK_EXPECT(localGameState.IsHost).once().returns(true);
    BOOST_CHECK(isLuaEqual("rttr:IsHost()", "true"));
    MOCK_EXPECT(localGameState.IsHost).once().returns(false);
    BOOST_CHECK(isLuaEqual("rttr:IsHost()", "false"));
    BOOST_CHECK(isLuaEqual("rttr:GetNumPlayers()", "3"));

    MOCK_EXPECT(localGameState.GetPlayerId).once().returns(1);
    BOOST_CHECK(isLuaEqual("rttr:GetLocalPlayerIdx()", "1"));
}

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
    std::string localSetting = "rttr:RegisterTranslations({ en_GB = { Foo = 'Eng', Bar = 'Eng2' }, pt = { Foo = "
                               "'Port', Bar = 'Port2' }, pt_BR = { Foo = 'PortBR', "
                               "Bar = 'PortBR2' } })";
    // With region
    {
        rttr::test::LocaleResetter loc("pt_BR");
        executeLua(localSetting);
        executeLua("rttr:Log(_('Foo'))");
        BOOST_REQUIRE_EQUAL(getLog(), "PortBR\n");
    }
    // Other region
    {
        rttr::test::LocaleResetter loc("pt_PT");
        executeLua(localSetting);
        executeLua("rttr:Log(_('Foo'))");
        BOOST_REQUIRE_EQUAL(getLog(), "Port\n");
    }
    // Non-Translated lang
    {
        rttr::test::LocaleResetter loc("de");
        executeLua(localSetting);
        const auto logStr = getLog();
        BOOST_TEST(logStr.find("Did not found translation for language 'de' in LUA file. Available translations:")
                   != std::string::npos);
        // All 3 mentioned
        BOOST_TEST(logStr.find("pt") != std::string::npos);
        BOOST_TEST(logStr.find("en_GB") != std::string::npos);
        BOOST_TEST(logStr.find("pt_BR") != std::string::npos);
        executeLua("rttr:Log(_('Foo'))");
        BOOST_REQUIRE_EQUAL(getLog(), "Eng\n");
    }
}

BOOST_AUTO_TEST_CASE(GameFunctions)
{
    initWorld();
    std::array<nobHQ*, 2> hqs;
    hqs[0] = world.GetSpecObj<nobHQ>(world.GetPlayer(0).GetHQPos());
    hqs[1] = world.GetSpecObj<nobHQ>(world.GetPlayer(1).GetHQPos());

    BOOST_REQUIRE_GT(hqs[0]->GetNumRealWares(GD_BOARDS), 0u);

    executeLua("rttr:ClearResources()");
    for(auto& hq : hqs)
    {
        for(unsigned gd = 0; gd < NUM_WARE_TYPES; gd++)
        {
            BOOST_REQUIRE_EQUAL(hq->GetNumRealWares(GoodType(gd)), 0u);
            BOOST_REQUIRE_EQUAL(hq->GetNumVisualWares(GoodType(gd)), 0u);
        }
        for(unsigned job = 0; job < NUM_JOB_TYPES; job++)
        {
            BOOST_REQUIRE_EQUAL(hq->GetNumRealFigures(Job(job)), 0u);
            BOOST_REQUIRE_EQUAL(hq->GetNumVisualFigures(Job(job)), 0u);
        }
    }

    for(unsigned i = 0; i < 2; i++)
    {
        BOOST_CHECK(isLuaEqual("rttr:GetGF()", s25util::toStringClassic(world.GetEvMgr().GetCurrentGF())));
        world.GetEvMgr().ExecuteNextGF();
    }

    // Set player id
    MOCK_EXPECT(localGameState.GetPlayerId).returns(1);
    // Send to other player
    MOCK_EXPECT(localGameState.SystemChat).never();
    executeLua("rttr:Chat(0, 'Hello World')");
    MOCK_VERIFY(localGameState.SystemChat);
    MOCK_RESET(localGameState.SystemChat);
    // Send to this player and all
    MOCK_EXPECT(localGameState.SystemChat).once().with("Hello World");
    MOCK_EXPECT(localGameState.SystemChat).once().with("Hello All");
    executeLua("rttr:Chat(1, 'Hello World')");
    executeLua("rttr:Chat(-1, 'Hello All')");
    MOCK_VERIFY(localGameState.SystemChat);

    world.GetPostMgr().AddPostBox(1);
    const PostBox& postBox = *world.GetPostMgr().GetPostBox(1);
    // Send to other player or invalid
    executeLua("rttr:PostMessage(0, 'Hello World')");
    BOOST_REQUIRE_THROW(executeLua("rttr:PostMessage(-1, 'Hello World')"), LuaExecutionError);
    BOOST_REQUIRE_NE(getLog(), "");
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
    BOOST_REQUIRE_EQUAL(getLog(), "");
    BOOST_REQUIRE_THROW(executeLua("assert(rttr:GetPlayer(3))"), LuaExecutionError);
    BOOST_REQUIRE_NE(getLog(), "");
    BOOST_REQUIRE_THROW(executeLua("assert(rttr:GetPlayer(-1))"), LuaExecutionError);
    BOOST_REQUIRE_NE(getLog(), "");

    executeLua("assert(rttr:GetWorld())");

    MOCK_EXPECT(localGameState.FormatGFTime).once().with(1023u).returns("01:13:17");
    BOOST_TEST(isLuaEqual("rttr:FormatNumGFs(1023)", "'01:13:17'"));
}

BOOST_AUTO_TEST_CASE(MissionGoal)
{
    initWorld();

    const PostBox& postBox = world.GetPostMgr().AddPostBox(1);
    BOOST_REQUIRE(postBox.GetCurrentMissionGoal().empty()); //-V807

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
    BOOST_CHECK(isLuaEqual("player:GetColor()", s25util::toStringClassic(0xFFFF0000)));
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
    std::unique_ptr<BuildingNote> note_;
    Subscription sub;

    CatchConstructionNote(GameWorldGame& world) : sub(world.GetNotifications().subscribe<BuildingNote>(std::ref(*this)))
    {}

    void operator()(const BuildingNote& note) { note_ = std::make_unique<BuildingNote>(note); }
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
    for(unsigned bld = 0; bld < NUM_BUILDING_TYPES; bld++)
        BOOST_REQUIRE(!player.IsBuildingEnabled(BuildingType(bld)));

    executeLua("player:EnableAllBuildings()");
    for(unsigned bld = 0; bld < NUM_BUILDING_TYPES; bld++)
        BOOST_REQUIRE(player.IsBuildingEnabled(BuildingType(bld)));

    std::array<unsigned, 5> numReserveClaimedBefore, numReserveVisualClaimedBefore;
    for(unsigned rank = 0; rank < SOLDIER_JOBS.size(); rank++)
    {
        numReserveClaimedBefore[rank] = hq->GetReserveClaimed(rank);
        numReserveVisualClaimedBefore[rank] = *hq->GetReserveClaimedVisualPointer(rank);
    }

    executeLua("player:ClearResources()");
    const Inventory& playerInv = player.GetInventory();
    for(const GoodType gd : helpers::EnumRange<GoodType>{})
        BOOST_TEST_CONTEXT("Good: " << gd)
        {
            BOOST_TEST(hq->GetNumRealWares(gd) == 0u);
            BOOST_TEST(hq->GetNumVisualWares(gd) == 0u);
            BOOST_TEST(playerInv[gd] == 0u);
        }
    for(const Job job : helpers::EnumRange<Job>{})
        BOOST_TEST_CONTEXT("Job: " << job)
        {
            BOOST_TEST(hq->GetNumRealFigures(job) == 0u);
            BOOST_TEST(hq->GetNumVisualFigures(job) == 0u);
            BOOST_TEST(playerInv[job] == 0u);
        }
    for(unsigned rank = 0; rank < SOLDIER_JOBS.size(); rank++)
        BOOST_TEST_CONTEXT("Rank: " << rank)
        {
            // Claimed is not touched
            BOOST_TEST(hq->GetReserveClaimed(rank) == numReserveClaimedBefore[rank]);
            BOOST_TEST(*hq->GetReserveClaimedVisualPointer(rank) == numReserveVisualClaimedBefore[rank]);
            // But available/used ones are reset
            BOOST_TEST(*hq->GetReserveAvailablePointer(rank) == 0u);
        }

    executeLua("wares = {[GD_HAMMER]=8,[GD_AXE]=6,[GD_SAW]=3}\n"
               "people = {[JOB_HELPER] = 30,[JOB_WOODCUTTER] = 6,[JOB_FISHER] = 0,[JOB_FORESTER] = 2}");
    executeLua("assert(player:AddWares(wares))");
    BOOST_REQUIRE_EQUAL(playerInv.goods[GD_HAMMER], 8u);
    BOOST_REQUIRE_EQUAL(playerInv.goods[GD_AXE], 6u);
    BOOST_REQUIRE_EQUAL(playerInv.goods[GD_SAW], 3u);
    BOOST_CHECK_EQUAL(hq->GetNumRealWares(GD_HAMMER), 8u);
    BOOST_CHECK_EQUAL(hq->GetNumRealWares(GD_AXE), 6u);
    BOOST_CHECK_EQUAL(hq->GetNumRealWares(GD_SAW), 3u);
    executeLua("assert(player:AddPeople(people))");
    BOOST_REQUIRE_EQUAL(playerInv.people[JOB_HELPER], 30u);
    BOOST_REQUIRE_EQUAL(playerInv.people[JOB_WOODCUTTER], 6u);
    BOOST_REQUIRE_EQUAL(playerInv.people[JOB_FISHER], 0u);
    BOOST_REQUIRE_EQUAL(playerInv.people[JOB_FORESTER], 2u);
    BOOST_CHECK_EQUAL(hq->GetNumRealFigures(JOB_HELPER), 30u);
    BOOST_CHECK_EQUAL(hq->GetNumRealFigures(JOB_WOODCUTTER), 6u);
    BOOST_CHECK_EQUAL(hq->GetNumRealFigures(JOB_FISHER), 0u);
    BOOST_CHECK_EQUAL(hq->GetNumRealFigures(JOB_FORESTER), 2u);

    BOOST_REQUIRE_EQUAL(getLog(), "");
    // Invalid ware/player throws
    BOOST_CHECK_THROW(executeLua("player:AddWares({[9999]=8})"), std::runtime_error);
    BOOST_REQUIRE_NE(getLog(), "");
    BOOST_CHECK_THROW(executeLua("player:AddPeople({[9999]=8})"), std::runtime_error);
    BOOST_REQUIRE_NE(getLog(), "");

    BOOST_CHECK(isLuaEqual("player:GetNumWares(GD_HAMMER)", "8"));
    BOOST_CHECK(isLuaEqual("player:GetNumWares(GD_AXE)", "6"));

    BOOST_CHECK(isLuaEqual("player:GetNumPeople(JOB_HELPER)", "30"));
    BOOST_CHECK(isLuaEqual("player:GetNumPeople(JOB_FORESTER)", "2"));

    BOOST_CHECK(isLuaEqual("player:GetNumBuildings(BLD_WOODCUTTER)", "0"));
    BOOST_CHECK(isLuaEqual("player:GetNumBuildings(BLD_HEADQUARTERS)", "1"));
    BOOST_CHECK(isLuaEqual("player:GetNumBuildingSites(BLD_HEADQUARTERS)", "0"));
    BOOST_CHECK(isLuaEqual("player:GetNumBuildingSites(BLD_WOODCUTTER)", "0"));
    world.SetNO(hq->GetPos() + MapPoint(4, 0), new noBuildingSite(BLD_WOODCUTTER, hq->GetPos() + MapPoint(4, 0), 1));
    BOOST_CHECK(isLuaEqual("player:GetNumBuildings(BLD_WOODCUTTER)", "0"));
    BOOST_CHECK(isLuaEqual("player:GetNumBuildingSites(BLD_WOODCUTTER)", "1"));

    CatchConstructionNote note(world);
    // Closed or non-AI player
    BOOST_REQUIRE(isLuaEqual("rttr:GetPlayer(0):AIConstructionOrder(12, 13, BLD_WOODCUTTER)", "false"));
    BOOST_REQUIRE(isLuaEqual("rttr:GetPlayer(2):AIConstructionOrder(12, 13, BLD_WOODCUTTER)", "false"));
    BOOST_REQUIRE_EQUAL(getLog(), "");
    // Wrong coordinate
    BOOST_REQUIRE_THROW(executeLua("player:AIConstructionOrder(9999, 13, BLD_WOODCUTTER)"), std::runtime_error);
    BOOST_REQUIRE_NE(getLog(), "");
    BOOST_REQUIRE_THROW(executeLua("player:AIConstructionOrder(12, 9999, BLD_WOODCUTTER)"), std::runtime_error);
    BOOST_REQUIRE_NE(getLog(), "");
    // Wrong building
    BOOST_REQUIRE_THROW(executeLua("player:AIConstructionOrder(12, 13, 9999)"), std::runtime_error);
    BOOST_REQUIRE_NE(getLog(), "");
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
    BOOST_CHECK(isLuaEqual("hqX", s25util::toStringClassic(hq->GetPos().x)));
    BOOST_CHECK(isLuaEqual("hqY", s25util::toStringClassic(hq->GetPos().y)));

    // Destroy players HQ
    world.DestroyNO(hq->GetPos());
    // HQ-Pos is invalid
    executeLua("hqX, hqY = player:GetHQPos()");
    BOOST_CHECK(isLuaEqual("hqX", s25util::toStringClassic(MapPoint::Invalid().x)));
    BOOST_CHECK(isLuaEqual("hqY", s25util::toStringClassic(MapPoint::Invalid().y)));
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
    LogAccessor logAcc;
    initWorld();

    const GamePlayer& player = world.GetPlayer(1);
    executeLua("player = rttr:GetPlayer(1)\nassert(player)");

    // Just a single polygon
    executeLua("player:SetRestrictedArea(5,7, 5,12, 15,12)");
    std::vector<MapPoint> expectedRestrictedArea;
    expectedRestrictedArea = {MapPoint(5, 7), MapPoint(5, 12), MapPoint(15, 12)};
    BOOST_TEST_REQUIRE(player.GetRestrictedArea() == expectedRestrictedArea, boost::test_tools::per_element()); //-V807
    executeLua("assert(not player:IsInRestrictedArea(1, 2))");
    executeLua("assert(not player:IsInRestrictedArea(0, 0))");
    executeLua("assert(player:IsInRestrictedArea(6, 8))");
    executeLua("assert(player:IsInRestrictedArea(11, 11))");

    // Polygon with hole
    executeLua("player:SetRestrictedArea(0,0, 1,1, 10,1, 10,10, 1,10, 1,1, 0,0, 5,5, 7,5, 7,7, 5,5, 0,0)");
    expectedRestrictedArea = {MapPoint(0, 0),  MapPoint(1, 1), MapPoint(10, 1), MapPoint(10, 10),
                              MapPoint(1, 10), MapPoint(1, 1), MapPoint(0, 0),  MapPoint(5, 5),
                              MapPoint(7, 5),  MapPoint(7, 7), MapPoint(5, 5),  MapPoint(0, 0)};
    BOOST_TEST_REQUIRE(player.GetRestrictedArea() == expectedRestrictedArea, boost::test_tools::per_element());
    BOOST_REQUIRE_EQUAL(getLog(), "");
    // Some prefer using nils
    executeLua("player:SetRestrictedArea(nil,nil, 1,1, 10,1, 10,10, 1,10, 1,1, nil,nil, 5,5, 7,5, 7,7, 5,5, nil,nil)");
    BOOST_TEST_REQUIRE(player.GetRestrictedArea() == expectedRestrictedArea, boost::test_tools::per_element());
    RTTR_REQUIRE_LOG_CONTAINS("don't need leading nils", false);

    // New API: Single nil and no double point
    executeLua("player:SetRestrictedArea(1,1, 10,1, 10,10, 1,10, nil, 5,5, 7,5, 7,7)");
    BOOST_TEST_REQUIRE(player.GetRestrictedArea() == expectedRestrictedArea, boost::test_tools::per_element());
    // Although you could use nil at the end...
    executeLua("player:SetRestrictedArea(1,1, 10,1, 10,10, 1,10, nil, 5,5, 7,5, 7,7, nil)");
    BOOST_TEST_REQUIRE(player.GetRestrictedArea() == expectedRestrictedArea, boost::test_tools::per_element());
    // ...or beginning
    executeLua("player:SetRestrictedArea(nil,1,1, 10,1, 10,10, 1,10, nil, 5,5, 7,5, 7,7)");
    BOOST_TEST_REQUIRE(player.GetRestrictedArea() == expectedRestrictedArea, boost::test_tools::per_element());
    RTTR_REQUIRE_LOG_CONTAINS("don't need leading nils", false);

    // Also for single polygon
    executeLua("player:SetRestrictedArea(5,7, 5,12, 15,12, nil)");
    expectedRestrictedArea = {MapPoint(5, 7), MapPoint(5, 12), MapPoint(15, 12)};
    BOOST_TEST_REQUIRE(player.GetRestrictedArea() == expectedRestrictedArea, boost::test_tools::per_element());

    executeLua("player:SetRestrictedArea()");
    BOOST_REQUIRE_EQUAL(player.GetRestrictedArea().size(), 0u);

    // Numbers must be pairs
    BOOST_REQUIRE_THROW(executeLua("player:SetRestrictedArea(1,2, 3)"), std::runtime_error);
    RTTR_REQUIRE_LOG_CONTAINS("Argument mismatch", false);
    // And non negative
    BOOST_REQUIRE_THROW(executeLua("player:SetRestrictedArea(1,2, -3,4)"), std::runtime_error);
    RTTR_REQUIRE_LOG_CONTAINS("must be positive", false);
    BOOST_REQUIRE_THROW(executeLua("player:SetRestrictedArea(1,2, 3,-4)"), std::runtime_error);
    RTTR_REQUIRE_LOG_CONTAINS("must be positive", false);
}

BOOST_AUTO_TEST_CASE(World)
{
    LogAccessor logAcc;
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
    const Position envPt2(envPt.x + world.GetWidth(), envPt.y - world.GetHeight());
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
    BOOST_REQUIRE_THROW(executeLua(boost::format("world:AddStaticObject(%1%, %2%, 5, 3, 3)") % envPt2.x % envPt2.y),
                        std::runtime_error);
    RTTR_REQUIRE_LOG_CONTAINS("Invalid size", false);

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
    BOOST_REQUIRE_EQUAL(animal->GetSpecies(), Species::Deer); //-V522
    executeLua(boost::format("world:AddAnimal(%1%, %2%, SPEC_FOX)") % animalPos.x % animalPos.y);
    BOOST_REQUIRE_EQUAL(figs.size(), 2u);
    animal = dynamic_cast<noAnimal*>(figs.back());
    BOOST_REQUIRE(animal);
    BOOST_REQUIRE_EQUAL(animal->GetSpecies(), Species::Fox);
}

BOOST_AUTO_TEST_CASE(WorldEvents)
{
    const MapPoint pt1(3, 4), pt2(5, 1), pt3(7, 6);
    // All events need to work w/o having them
    LuaInterfaceGame& lua = world.GetLua();
    lua.EventStart(true);
    Serializer serData;
    BOOST_REQUIRE(lua.Serialize(serData));
    BOOST_REQUIRE_EQUAL(serData.GetLength(), 0u);
    BOOST_REQUIRE(lua.Deserialize(serData));
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

    // Returning false should not save anything
    executeLua("function onSave(serializer)\n"
               "serializer:PushInt(42)\n  return false\nend");
    serData.Clear();
    BOOST_REQUIRE(!lua.Serialize(serData));
    BOOST_REQUIRE_EQUAL(serData.GetLength(), 0u);

    executeLua("function onSave(serializer)\n"
               "serializer:PushInt(42)\n"
               "serializer:PushBool(true)\n"
               "serializer:PushBool(false)\n"
               "serializer:PushString('Hello World!')\n"
               "rttr:Log('saved')\n  return true\nend");
    serData.Clear();
    BOOST_REQUIRE(lua.Serialize(serData));
    BOOST_REQUIRE_EQUAL(getLog(), "saved\n");
    BOOST_REQUIRE_GT(serData.GetLength(), 0u);

    executeLua("function onLoad(serializer)\n"
               "assert(serializer:PopInt() == 42)\n"
               "assert(serializer:PopBool() == true)\n"
               "assert(serializer:PopBool() == false)\n"
               "assert(serializer:PopString() == 'Hello World!')\n"
               "rttr:Log('loaded')\n  return true\nend");
    BOOST_REQUIRE(lua.Deserialize(serData));
    BOOST_REQUIRE_EQUAL(getLog(), "loaded\n");
    BOOST_REQUIRE_EQUAL(serData.GetBytesLeft(), 0u);

    // Errors in onSave/onLoad should result in false returned even with throw disabled
    executeLua("function onSave(serializer)\n  serializer:PushInt(42)\n  assert(false)\nend");
    serData.Clear();
    BOOST_REQUIRE_THROW(lua.Serialize(serData), LuaExecutionError);
    BOOST_REQUIRE_NE(getLog(), "");

    lua.setThrowOnError(false);

    executeLua("function onSave(serializer)\n  serializer:PushInt(42)\n  assert(false)\nend");
    serData.Clear();
    BOOST_REQUIRE(!lua.Serialize(serData));
    BOOST_REQUIRE_EQUAL(serData.GetLength(), 0u);
    BOOST_REQUIRE_NE(getLog(), "");

    // Error from C++
    BOOST_REQUIRE(!lua.Deserialize(serData));
    BOOST_REQUIRE_NE(getLog(), "");

    // False returned
    executeLua("function onLoad(serializer)\n  return false\nend");
    BOOST_REQUIRE(!lua.Deserialize(serData));
    // Re-enable
    lua.setThrowOnError(true);

    executeLua("function onExplored(player_id, x, y, owner)\n  rttr:Log('explored: '..player_id..'('..x..', "
               "'..y..')'..tostring(owner))\nend");
    // Owner == 0 -> No owner (nil in lua)
    lua.EventExplored(0, pt2, 0);
    BOOST_REQUIRE_EQUAL(getLog(), (boost::format("explored: %1%%2%%3%\n") % 0 % pt2 % "nil").str());
    // Else owner-1 = playerIdx
    lua.EventExplored(0, pt2, 2);
    BOOST_REQUIRE_EQUAL(getLog(), (boost::format("explored: %1%%2%%3%\n") % 0 % pt2 % 1).str());

    executeLua("function onGameFrame(gameframe_number)\n  rttr:Log('gf: '..gameframe_number)\nend");
    lua.EventGameFrame(0);
    BOOST_REQUIRE_EQUAL(getLog(), "gf: 0\n");
    lua.EventGameFrame(42);
    BOOST_REQUIRE_EQUAL(getLog(), "gf: 42\n");

    executeLua(
      "function onResourceFound(player_id, x, y, type, quantity)\n  rttr:Log('resFound: '..player_id..'('..x..', "
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

BOOST_AUTO_TEST_CASE(onOccupied)
{
    executeLua("occupied = {}\n\
    function onOccupied(player_id, x, y)\n\
        points = occupied[player_id] or {}\n\
        table.insert(points, {x, y})\n\
        occupied[player_id] = points\n\
    end");
    initWorld();
    using Points = std::vector<std::pair<int, int>>;
    std::map<int, Points> gamePtsPerPlayer;
    RTTR_FOREACH_PT(MapPoint, world.GetSize())
    {
        uint8_t owner = world.GetNode(pt).owner;
        if(owner)
            gamePtsPerPlayer[owner - 1].push_back(std::pair<int, int>(pt.x, pt.y));
    }
    std::map<int, Points> luaPtsPerPlayer = getLuaState()["occupied"];
    BOOST_REQUIRE_EQUAL(luaPtsPerPlayer.size(), gamePtsPerPlayer.size());
    for(unsigned i = 0; i < world.GetNumPlayers(); i++)
    {
        Points& gamePts = gamePtsPerPlayer[i];
        Points& luaPts = luaPtsPerPlayer[i];
        std::sort(gamePts.begin(), gamePts.end());
        std::sort(luaPts.begin(), luaPts.end());
        BOOST_TEST_REQUIRE(luaPts == gamePts, boost::test_tools::per_element());
    }
}

BOOST_AUTO_TEST_CASE(onExplored)
{
    executeLua("explored = {}\n\
    function onExplored(player_id, x, y)\n\
        points = explored[player_id] or {}\n\
        table.insert(points, {x, y})\n\
        explored[player_id] = points\n\
    end");
    initWorld();
    using Points = std::vector<std::pair<int, int>>;
    std::map<int, Points> gamePtsPerPlayer;
    RTTR_FOREACH_PT(MapPoint, world.GetSize())
    {
        const MapNode& node = world.GetNode(pt);
        for(unsigned i = 0; i < world.GetNumPlayers(); i++)
        {
            if(node.fow[i].visibility == Visibility::Visible)
                gamePtsPerPlayer[i].push_back(std::pair<int, int>(pt.x, pt.y));
        }
    }
    std::map<int, Points> luaPtsPerPlayer = getLuaState()["explored"];
    BOOST_REQUIRE_EQUAL(luaPtsPerPlayer.size(), gamePtsPerPlayer.size());
    for(unsigned i = 0; i < world.GetNumPlayers(); i++)
    {
        Points& gamePts = gamePtsPerPlayer[i];
        Points& luaPts = luaPtsPerPlayer[i];
        std::sort(gamePts.begin(), gamePts.end());
        std::sort(luaPts.begin(), luaPts.end());
        BOOST_TEST_REQUIRE(luaPts == gamePts, boost::test_tools::per_element());
    }
}

BOOST_AUTO_TEST_CASE(LuaPacts)
{
    initWorld();
    GamePlayer& player = world.GetPlayer(0);

    executeLua("player = rttr:GetPlayer(1)");
    // accept every pact from player
    executeLua("function onSuggestPact(pactType, suggestedBy, target, duration) return suggestedBy == 0 end");
    // log created pacts to test callback
    executeLua(
      "function onPactCreated(pt, suggestedByPlayerId, targetPlayerId, duration) rttr:Log('Pact created') end");
    // log canceled pacts to test callback
    executeLua("function onPactCanceled(pt, canceledByPlayerId, targetPlayerId) rttr:Log('Pact canceled') end");

    // create alliance and check players state
    player.SuggestPact(1, TREATY_OF_ALLIANCE, DURATION_INFINITE);
    game->executeAICommands();
    BOOST_REQUIRE(player.IsAlly(1));
    executeLua("assert(player:IsAlly(0))");
    BOOST_REQUIRE(!player.IsAttackable(1));
    executeLua("assert(not player:IsAttackable(0))");

    // check if callback onPactCreated was executed
    BOOST_REQUIRE_EQUAL(getLog(), "Pact created\n");

    // cancel pact by lua request
    executeLua("player:CancelPact(TREATY_OF_ALLIANCE, 0)");
    game->executeAICommands();
    player.CancelPact(TREATY_OF_ALLIANCE, 1);
    BOOST_REQUIRE(!player.IsAlly(1));
    executeLua("assert(not player:IsAlly(0))");
    BOOST_REQUIRE(player.IsAttackable(1));
    executeLua("assert(player:IsAttackable(0))");

    // check if callback onPactCanceled was executed
    BOOST_REQUIRE_EQUAL(getLog(), "Pact canceled\n");

    // accept cancel-request via lua callback
    executeLua("function onCancelPactRequest(pt, player, ai) return true end");
    player.SuggestPact(1, NON_AGGRESSION_PACT, DURATION_INFINITE);
    game->executeAICommands();
    // non aggression was created
    BOOST_REQUIRE(!player.IsAttackable(1));
    BOOST_REQUIRE_EQUAL(getLog(), "Pact created\n");

    // cancel pact by player and check state
    player.CancelPact(NON_AGGRESSION_PACT, 1);
    BOOST_REQUIRE(player.IsAttackable(1));
    BOOST_REQUIRE_EQUAL(getLog(), "Pact canceled\n");

    const PostBox& postbox = world.GetPostMgr().AddPostBox(0);
    // Suggest Pact from Lua
    executeLua("player:SuggestPact(0, TREATY_OF_ALLIANCE, DURATION_INFINITE)");
    game->executeAICommands();
    const auto* msg = dynamic_cast<const DiplomacyPostQuestion*>(postbox.GetMsg(0));
    this->AcceptPact(msg->GetPactId(), TREATY_OF_ALLIANCE, 1);
    BOOST_REQUIRE(!player.IsAttackable(1));
    executeLua("assert(not player:IsAttackable(0))");

    BOOST_REQUIRE_EQUAL(getLog(), "Pact created\n");
}

BOOST_AUTO_TEST_SUITE_END()
