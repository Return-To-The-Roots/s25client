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

#include "GlobalGameSettings.h"
#include "JoinPlayerInfo.h"
#include "LuaBaseFixture.h"
#include "addons/Addon.h"
#include "lua/LuaInterfaceSettings.h"
#include "network/IGameLobbyController.h"
#include "worldFixtures/MockLocalGameState.h"
#include "s25util/colors.h"
#include <rttr/test/LogAccessor.hpp>
#include <boost/test/unit_test.hpp>
#include <vector>

static std::ostream& operator<<(std::ostream& os, AddonId id)
{
    return os << rttrEnum::toString(id);
}

namespace {

/// Fixture for the settings tests, implements IGameLobbyController
/// Note: Here all settings are applied immediately, in the real thing they are transmitted to the server and then
/// applied, hence with a delay
struct LuaSettingsTestsFixture : public LuaBaseFixture, public IGameLobbyController, public rttr::test::LogAccessor
{
    std::vector<JoinPlayerInfo> players;
    GlobalGameSettings ggs;
    MockLocalGameState localGameState;
    LuaInterfaceSettings lua;

    unsigned GetMaxNumPlayers() const override { return players.size(); }
    JoinPlayerInfo& GetJoinPlayer(unsigned playerIdx) override { return players.at(playerIdx); }
    const GlobalGameSettings& GetGGS() const override { return ggs; }
    void ChangeGlobalGameSettings(const GlobalGameSettings& ggs) override { this->ggs = ggs; }
    void CloseSlot(unsigned playerIdx) override { GetJoinPlayer(playerIdx).ps = PS_LOCKED; }
    void SetPlayerState(unsigned playerIdx, PlayerState state, const AI::Info& aiInfo) override
    {
        GetJoinPlayer(playerIdx).ps = state;
        GetJoinPlayer(playerIdx).aiInfo = aiInfo;
    }
    void SetName(unsigned playerIdx, const std::string& name) override { GetJoinPlayer(playerIdx).name = name; }
    void SetColor(unsigned playerIdx, unsigned newColor) override { GetJoinPlayer(playerIdx).color = newColor; }
    void SetTeam(unsigned playerIdx, Team newTeam) override { GetJoinPlayer(playerIdx).team = newTeam; }
    void SetNation(unsigned playerIdx, Nation newNation) override { GetJoinPlayer(playerIdx).nation = newNation; }

    LuaSettingsTestsFixture() : lua(*this, localGameState)
    {
        setLua(&lua);

        players.resize(3);
        players[0].ps = PS_OCCUPIED;
        players[0].name = "Player1";
        players[0].nation = NAT_VIKINGS;
        players[0].color = 0xFF00FF00;
        players[0].team = TM_TEAM1;
        players[0].isHost = true;

        players[1].ps = PS_AI;
        players[1].name = "PlayerAI";
        players[1].nation = NAT_ROMANS;
        players[1].color = 0xFFFF0000;
        players[1].team = TM_TEAM2;
        players[1].isHost = false;

        players[2].ps = PS_FREE;
    }

    void checkSettings(const GlobalGameSettings& shouldVal)
    {
        BOOST_REQUIRE_EQUAL(ggs.speed, shouldVal.speed);
        BOOST_REQUIRE_EQUAL(ggs.objective, shouldVal.objective);
        BOOST_REQUIRE_EQUAL(ggs.startWares, shouldVal.startWares);
        BOOST_REQUIRE_EQUAL(ggs.lockedTeams, shouldVal.lockedTeams);
        BOOST_REQUIRE_EQUAL(ggs.exploration, shouldVal.exploration);
        BOOST_REQUIRE_EQUAL(ggs.teamView, shouldVal.teamView);
        BOOST_REQUIRE_EQUAL(ggs.randomStartPosition, shouldVal.randomStartPosition);
    }
};

} // namespace

BOOST_FIXTURE_TEST_SUITE(LuaTestSuiteSettings, LuaSettingsTestsFixture)

BOOST_AUTO_TEST_CASE(AddonId_ToString)
{
    std::stringstream s;
    s << AddonId::CHARBURNER;
    BOOST_TEST(s.str() == "CHARBURNER");
}

BOOST_AUTO_TEST_CASE(AssertionThrows)
{
    BOOST_REQUIRE_THROW(executeLua("assert(false)"), LuaExecutionError);
    BOOST_REQUIRE_NE(getLog(), "");
}

BOOST_AUTO_TEST_CASE(Events)
{
    // All events need to work w/o having them
    lua.EventSettingsInit(true, false);
    lua.EventSettingsReady();
    lua.EventPlayerJoined(0);
    lua.EventPlayerLeft(0);
    lua.EventPlayerReady(0);
    BOOST_REQUIRE(lua.IsChangeAllowed("general", true));
    BOOST_REQUIRE(!lua.IsChangeAllowed("general", false));
    std::vector<AddonId> allowedAddons = lua.GetAllowedAddons();
    BOOST_REQUIRE(allowedAddons.empty());

    clearLog();
    executeLua("function onSettingsInit(isSinglePlayer, isSavegame)\n  rttr:Log('init: '..tostring(isSinglePlayer)..' "
               "'..tostring(isSavegame))\nend");
    lua.EventSettingsInit(true, false);
    BOOST_REQUIRE_EQUAL(getLog(), "init: true false\n");
    lua.EventSettingsInit(false, true);
    BOOST_REQUIRE_EQUAL(getLog(), "init: false true\n");

    executeLua("function onSettingsReady()\n  rttr:Log('ready')\nend");
    lua.EventSettingsReady();
    BOOST_REQUIRE_EQUAL(getLog(), "ready\n");

    executeLua("function onPlayerJoined(playerIdx)\n  rttr:Log('joined'..playerIdx)\nend");
    lua.EventPlayerJoined(2);
    BOOST_REQUIRE_EQUAL(getLog(), "joined2\n");

    executeLua("function onPlayerLeft(playerIdx)\n  rttr:Log('left'..playerIdx)\nend");
    lua.EventPlayerLeft(2);
    BOOST_REQUIRE_EQUAL(getLog(), "left2\n");

    executeLua("function onPlayerReady(playerIdx)\n  rttr:Log('ready'..playerIdx)\nend");
    lua.EventPlayerReady(2);
    BOOST_REQUIRE_EQUAL(getLog(), "ready2\n");

    executeLua("function getAllowedChanges()\n  return {general=true, swapping=true}\nend");
    BOOST_REQUIRE(lua.IsChangeAllowed("general", false));
    BOOST_REQUIRE(lua.IsChangeAllowed("swapping", false));
    BOOST_REQUIRE(!lua.IsChangeAllowed("addonsAll", false));

    executeLua("function getAllowedAddons()\n  return {ADDON_LIMIT_CATAPULTS, ADDON_CHARBURNER, ADDON_TRADE}\nend");
    allowedAddons = lua.GetAllowedAddons();
    BOOST_REQUIRE_EQUAL(allowedAddons.size(), 3u);
    BOOST_REQUIRE_EQUAL(allowedAddons[0], AddonId::LIMIT_CATAPULTS);
    BOOST_REQUIRE_EQUAL(allowedAddons[1], AddonId::CHARBURNER);
    BOOST_REQUIRE_EQUAL(allowedAddons[2], AddonId::TRADE);
    // Return invalid type -> ignored, but log output
    clearLog();
    executeLua("function getAllowedAddons()\n  return {'ADDON_FAIL_ME'}\nend");
    allowedAddons = lua.GetAllowedAddons();
    BOOST_REQUIRE(allowedAddons.empty());
    BOOST_REQUIRE_NE(getLog(), "");
    executeLua("function getAllowedAddons()\n  return {9999}\nend");
    allowedAddons = lua.GetAllowedAddons();
    BOOST_REQUIRE(allowedAddons.empty());
    BOOST_REQUIRE_NE(getLog(), "");
}

BOOST_AUTO_TEST_CASE(SettingsFunctions)
{
    LogAccessor logAcc;
    executeLua("assert(rttr:GetPlayer(0))");
    executeLua("assert(rttr:GetPlayer(1))");
    executeLua("assert(rttr:GetPlayer(2))");
    // Invalid player
    BOOST_REQUIRE_THROW(executeLua("assert(rttr:GetPlayer(3))"), std::runtime_error);
    RTTR_REQUIRE_LOG_CONTAINS("Invalid player idx", false);
    BOOST_REQUIRE_THROW(executeLua("assert(rttr:GetPlayer(-1))"), std::runtime_error);
    RTTR_REQUIRE_LOG_CONTAINS("Invalid player idx", false);

    executeLua("rttr:SetAddon(ADDON_LIMIT_CATAPULTS, 4)\n  rttr:SetAddon(ADDON_TRADE, true)\n");
    BOOST_REQUIRE_EQUAL(ggs.getSelection(AddonId::LIMIT_CATAPULTS), 4u);
    BOOST_REQUIRE(ggs.isEnabled(AddonId::TRADE));
    executeLua("rttr:SetAddon(ADDON_LIMIT_CATAPULTS, 1)\n  rttr:SetAddon(ADDON_TRADE, false)\n");
    BOOST_REQUIRE_EQUAL(ggs.getSelection(AddonId::LIMIT_CATAPULTS), 1u);
    BOOST_REQUIRE(!ggs.isEnabled(AddonId::TRADE));
    // Set some random options
    for(unsigned i = 0; i < ggs.getNumAddons(); i++)
    {
        const Addon* curAddon = ggs.getAddon(i);
        ggs.setSelection(curAddon->getId(), rand() % curAddon->getNumOptions());
    }
    executeLua("rttr:ResetAddons()\n");
    for(unsigned i = 0; i < ggs.getNumAddons(); i++)
    {
        const Addon* curAddon = ggs.getAddon(i);
        BOOST_REQUIRE_EQUAL(ggs.getSelection(curAddon->getId()), curAddon->getDefaultStatus());
        BOOST_REQUIRE(!ggs.isEnabled(curAddon->getId()));
    }

    GlobalGameSettings shouldSettings = ggs;

#define SET_AND_CHECK(setting, value)                              \
    executeLua("rttr:SetGameSettings({" #setting "=" #value "})"); \
    shouldSettings.setting = value;                                \
    checkSettings(shouldSettings);

    SET_AND_CHECK(speed, GS_VERYSLOW);
    SET_AND_CHECK(speed, GS_FAST);
    SET_AND_CHECK(objective, GO_TOTALDOMINATION);
    SET_AND_CHECK(objective, GO_CONQUER3_4);
    SET_AND_CHECK(startWares, SWR_LOW);
    SET_AND_CHECK(startWares, SWR_VLOW);

    executeLua("rttr:SetGameSettings({fow=EXP_FOGOFWARE_EXPLORED})");
    shouldSettings.exploration = EXP_FOGOFWARE_EXPLORED;
    checkSettings(shouldSettings);
    executeLua("rttr:SetGameSettings({fow=EXP_CLASSIC})");
    shouldSettings.exploration = EXP_CLASSIC;
    checkSettings(shouldSettings);

    SET_AND_CHECK(lockedTeams, true);
    SET_AND_CHECK(lockedTeams, false);
    SET_AND_CHECK(teamView, true);
    SET_AND_CHECK(teamView, false);
    SET_AND_CHECK(randomStartPosition, true);
    SET_AND_CHECK(randomStartPosition, false);

#undef SET_AND_CHECK

    // And multiple settings at once:
    executeLua("rttr:SetGameSettings({fow=EXP_FOGOFWARE_EXPLORED, speed=GS_VERYFAST, lockedTeams=true})");
    shouldSettings.exploration = EXP_FOGOFWARE_EXPLORED;
    shouldSettings.speed = GS_VERYFAST;
    shouldSettings.lockedTeams = true;
    checkSettings(shouldSettings);

    // Reset all settings
    executeLua("rttr:ResetGameSettings()");
    shouldSettings.speed = GS_NORMAL;
    shouldSettings.objective = GO_NONE;
    shouldSettings.startWares = SWR_NORMAL;
    shouldSettings.lockedTeams = false;
    shouldSettings.exploration = EXP_FOGOFWAR;
    shouldSettings.teamView = true;
    shouldSettings.randomStartPosition = false;
    checkSettings(shouldSettings);
    BOOST_REQUIRE_EQUAL(ggs.getNumAddons(), shouldSettings.getNumAddons());
    for(unsigned i = 0; i < ggs.getNumAddons(); i++)
    {
        const Addon* curAddon = ggs.getAddon(i);
        BOOST_REQUIRE_EQUAL(ggs.getSelection(curAddon->getId()), curAddon->getDefaultStatus());
        BOOST_REQUIRE(!ggs.isEnabled(curAddon->getId()));
    }
}

BOOST_AUTO_TEST_CASE(PlayerSettings)
{
    LogAccessor logAcc;
    executeLua("player = rttr:GetPlayer(0)");
    executeLua("player1 = rttr:GetPlayer(1)");
    executeLua("player:SetNation(NAT_ROMANS)");
    BOOST_REQUIRE_EQUAL(players[0].nation, NAT_ROMANS);
    executeLua("player1:SetNation(NAT_BABYLONIANS)");
    BOOST_REQUIRE_EQUAL(players[1].nation, NAT_BABYLONIANS);

    // Check some properties
    BOOST_CHECK(isLuaEqual("player:GetNation()", "NAT_ROMANS"));
    BOOST_CHECK(isLuaEqual("player:IsHuman()", "true"));
    BOOST_CHECK(isLuaEqual("player:IsAI()", "false"));
    BOOST_CHECK(isLuaEqual("player:IsClosed()", "false"));
    BOOST_CHECK(isLuaEqual("player:IsFree()", "false"));

    executeLua("player:SetTeam(TM_TEAM2)");
    BOOST_REQUIRE_EQUAL(players[0].team, TM_TEAM2);

    executeLua("player:SetName(\"Foo\")");
    BOOST_CHECK(isLuaEqual("player:GetName()", "'Foo'"));

    executeLua("player:SetColor(2)");
    BOOST_REQUIRE_EQUAL(players[0].color, PLAYER_COLORS[2]);
    executeLua("player:SetColor(0xFF0000FF)");
    BOOST_REQUIRE_EQUAL(players[0].color, 0xFF0000FF);
    executeLua("player1:SetColor(2)");
    BOOST_REQUIRE_EQUAL(players[1].color, PLAYER_COLORS[2]);
    // Duplicate color are allowed
    executeLua("player1:SetColor(0xFF0000FF)");
    BOOST_REQUIRE_EQUAL(players[0].color, 0xFF0000FF);
    BOOST_REQUIRE_EQUAL(players[1].color, 0xFF0000FF);

    // Kick AI
    executeLua("player1:Close()");
    BOOST_REQUIRE_EQUAL(players[1].ps, PS_LOCKED);
    // Kick Human
    players[1].ps = PS_OCCUPIED;
    executeLua("player1:Close()");
    BOOST_REQUIRE_EQUAL(players[1].ps, PS_LOCKED);
    // Kick none
    executeLua("player1:Close()");
    BOOST_REQUIRE_EQUAL(players[1].ps, PS_LOCKED);

    executeLua("player:SetAI(0)");
    BOOST_REQUIRE_EQUAL(players[0].ps, PS_AI);
    BOOST_REQUIRE_EQUAL(players[0].aiInfo.type, AI::DUMMY); //-V807
    executeLua("player:SetAI(1)");
    BOOST_REQUIRE_EQUAL(players[0].ps, PS_AI);
    BOOST_REQUIRE_EQUAL(players[0].aiInfo.type, AI::DEFAULT);
    BOOST_REQUIRE_EQUAL(players[0].aiInfo.level, AI::EASY);
    executeLua("player:SetAI(2)");
    BOOST_REQUIRE_EQUAL(players[0].ps, PS_AI);
    BOOST_REQUIRE_EQUAL(players[0].aiInfo.type, AI::DEFAULT);
    BOOST_REQUIRE_EQUAL(players[0].aiInfo.level, AI::MEDIUM);
    executeLua("player:SetAI(3)");
    BOOST_REQUIRE_EQUAL(players[0].ps, PS_AI);
    BOOST_REQUIRE_EQUAL(players[0].aiInfo.type, AI::DEFAULT);
    BOOST_REQUIRE_EQUAL(players[0].aiInfo.level, AI::HARD);
    // Invalid lvl
    BOOST_REQUIRE_THROW(executeLua("player:SetAI(4)"), std::exception);
    RTTR_REQUIRE_LOG_CONTAINS("Invalid AI", false);
}

BOOST_AUTO_TEST_SUITE_END()
