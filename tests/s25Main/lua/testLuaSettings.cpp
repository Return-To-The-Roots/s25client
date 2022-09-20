// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "GlobalGameSettings.h"
#include "JoinPlayerInfo.h"
#include "LuaBaseFixture.h"
#include "addons/Addon.h"
#include "enum_cast.hpp"
#include "lua/LuaInterfaceSettings.h"
#include "network/IGameLobbyController.h"
#include "worldFixtures/MockLocalGameState.h"
#include "gameTypes/GameTypesOutput.h"
#include "s25util/colors.h"
#include <rttr/test/LogAccessor.hpp>
#include <boost/test/unit_test.hpp>
#include <vector>

// LCOV_EXCL_START
static std::ostream& operator<<(std::ostream& os, AddonId id)
{
    return os << rttrEnum::toString(id);
}
// LCOV_EXCL_STOP

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
    void CloseSlot(unsigned playerIdx) override { GetJoinPlayer(playerIdx).ps = PlayerState::Locked; }
    void SetPlayerState(unsigned playerIdx, PlayerState state, const AI::Info& aiInfo) override
    {
        GetJoinPlayer(playerIdx).ps = state;
        GetJoinPlayer(playerIdx).aiInfo = aiInfo;
    }
    void SetName(unsigned playerIdx, const std::string& name) override { GetJoinPlayer(playerIdx).name = name; }
    void SetPortrait(unsigned playerIdx, unsigned int portraitIndex) override
    {
        GetJoinPlayer(playerIdx).portraitIndex = portraitIndex;
    }
    void SetColor(unsigned playerIdx, unsigned newColor) override { GetJoinPlayer(playerIdx).color = newColor; }
    void SetTeam(unsigned playerIdx, Team newTeam) override { GetJoinPlayer(playerIdx).team = newTeam; }
    void SetNation(unsigned playerIdx, Nation newNation) override { GetJoinPlayer(playerIdx).nation = newNation; }

    LuaSettingsTestsFixture() : lua(*this, localGameState)
    {
        setLua(&lua);

        players.resize(3);
        players[0].ps = PlayerState::Occupied;
        players[0].name = "Player1";
        players[0].nation = Nation::Vikings;
        players[0].color = 0xFF00FF00;
        players[0].team = Team::Team1;
        players[0].isHost = true;

        players[1].ps = PlayerState::AI;
        players[1].name = "PlayerAI";
        players[1].nation = Nation::Romans;
        players[1].color = 0xFFFF0000;
        players[1].team = Team::Team2;
        players[1].isHost = false;

        players[2].ps = PlayerState::Free;
    }

    void checkSettings(const GlobalGameSettings& shouldVal) const
    {
        BOOST_TEST_REQUIRE(ggs.speed == shouldVal.speed);
        BOOST_TEST_REQUIRE(ggs.objective == shouldVal.objective);
        BOOST_TEST_REQUIRE(ggs.startWares == shouldVal.startWares);
        BOOST_TEST_REQUIRE(ggs.lockedTeams == shouldVal.lockedTeams);
        BOOST_TEST_REQUIRE(ggs.exploration == shouldVal.exploration);
        BOOST_TEST_REQUIRE(ggs.teamView == shouldVal.teamView);
        BOOST_TEST_REQUIRE(ggs.randomStartPosition == shouldVal.randomStartPosition);
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
    BOOST_TEST_REQUIRE(getLog() != "");
}

BOOST_AUTO_TEST_CASE(Events)
{
    // All events need to work w/o having them
    lua.EventSettingsInit(true, false);
    lua.EventSettingsReady();
    lua.EventPlayerJoined(0);
    lua.EventPlayerLeft(0);
    lua.EventPlayerReady(0);
    BOOST_TEST_REQUIRE(lua.IsChangeAllowed("general", true));
    BOOST_TEST_REQUIRE(!lua.IsChangeAllowed("general", false));
    std::vector<AddonId> allowedAddons = lua.GetAllowedAddons();
    BOOST_TEST_REQUIRE(allowedAddons.empty());

    clearLog();
    executeLua("function onSettingsInit(isSinglePlayer, isSavegame)\n  rttr:Log('init: '..tostring(isSinglePlayer)..' "
               "'..tostring(isSavegame))\nend");
    lua.EventSettingsInit(true, false);
    BOOST_TEST_REQUIRE(getLog() == "init: true false\n");
    lua.EventSettingsInit(false, true);
    BOOST_TEST_REQUIRE(getLog() == "init: false true\n");

    executeLua("function onSettingsReady()\n  rttr:Log('ready')\nend");
    lua.EventSettingsReady();
    BOOST_TEST_REQUIRE(getLog() == "ready\n");

    executeLua("function onPlayerJoined(playerIdx)\n  rttr:Log('joined'..playerIdx)\nend");
    lua.EventPlayerJoined(2);
    BOOST_TEST_REQUIRE(getLog() == "joined2\n");

    executeLua("function onPlayerLeft(playerIdx)\n  rttr:Log('left'..playerIdx)\nend");
    lua.EventPlayerLeft(2);
    BOOST_TEST_REQUIRE(getLog() == "left2\n");

    executeLua("function onPlayerReady(playerIdx)\n  rttr:Log('ready'..playerIdx)\nend");
    lua.EventPlayerReady(2);
    BOOST_TEST_REQUIRE(getLog() == "ready2\n");

    executeLua("function getAllowedChanges()\n  return {general=true, swapping=true}\nend");
    BOOST_TEST_REQUIRE(lua.IsChangeAllowed("general", false));
    BOOST_TEST_REQUIRE(lua.IsChangeAllowed("swapping", false));
    BOOST_TEST_REQUIRE(!lua.IsChangeAllowed("addonsAll", false));

    executeLua("function getAllowedAddons()\n  return {ADDON_LIMIT_CATAPULTS, ADDON_CHARBURNER, ADDON_TRADE}\nend");
    allowedAddons = lua.GetAllowedAddons();
    BOOST_TEST_REQUIRE(allowedAddons.size() == 3u);
    BOOST_TEST_REQUIRE(allowedAddons[0] == AddonId::LIMIT_CATAPULTS);
    BOOST_TEST_REQUIRE(allowedAddons[1] == AddonId::CHARBURNER);
    BOOST_TEST_REQUIRE(allowedAddons[2] == AddonId::TRADE);
    // Return invalid type -> ignored, but log output
    clearLog();
    executeLua("function getAllowedAddons()\n  return {'ADDON_FAIL_ME'}\nend");
    allowedAddons = lua.GetAllowedAddons();
    BOOST_TEST_REQUIRE(allowedAddons.empty());
    BOOST_TEST_REQUIRE(getLog() != "");
    executeLua("function getAllowedAddons()\n  return {9999}\nend");
    allowedAddons = lua.GetAllowedAddons();
    BOOST_TEST_REQUIRE(allowedAddons.empty());
    BOOST_TEST_REQUIRE(getLog() != "");
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
    BOOST_TEST_REQUIRE(ggs.getSelection(AddonId::LIMIT_CATAPULTS) == 4u);
    BOOST_TEST_REQUIRE(ggs.isEnabled(AddonId::TRADE));
    executeLua("rttr:SetAddon(ADDON_LIMIT_CATAPULTS, 1)\n  rttr:SetAddon(ADDON_TRADE, false)\n");
    BOOST_TEST_REQUIRE(ggs.getSelection(AddonId::LIMIT_CATAPULTS) == 1u);
    BOOST_TEST_REQUIRE(!ggs.isEnabled(AddonId::TRADE));
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
        BOOST_TEST_REQUIRE(ggs.getSelection(curAddon->getId()) == curAddon->getDefaultStatus());
        BOOST_TEST_REQUIRE(!ggs.isEnabled(curAddon->getId()));
    }

    GlobalGameSettings shouldSettings = ggs;

#define SET_AND_CHECK(setting, luaValue, value)                       \
    executeLua("rttr:SetGameSettings({" #setting "=" #luaValue "})"); \
    shouldSettings.setting = value;                                   \
    checkSettings(shouldSettings);

    SET_AND_CHECK(speed, GS_VERYSLOW, GameSpeed::VerySlow);
    SET_AND_CHECK(speed, GS_FAST, GameSpeed::Fast);
    SET_AND_CHECK(objective, GO_TOTALDOMINATION, GameObjective::TotalDomination);
    SET_AND_CHECK(objective, GO_CONQUER3_4, GameObjective::Conquer3_4);
    SET_AND_CHECK(startWares, SWR_LOW, StartWares::Low);
    SET_AND_CHECK(startWares, SWR_VLOW, StartWares::VLow);

    executeLua("rttr:SetGameSettings({fow=EXP_FOGOFWARE_EXPLORED})"); // Legacy
    shouldSettings.exploration = Exploration::FogOfWarExplored;
    checkSettings(shouldSettings);
    executeLua("rttr:SetGameSettings({fow=EXP_CLASSIC})");
    shouldSettings.exploration = Exploration::Classic;
    checkSettings(shouldSettings);
    executeLua("rttr:SetGameSettings({fow=EXP_FOGOFWAREXPLORED})"); // New value
    shouldSettings.exploration = Exploration::FogOfWarExplored;
    checkSettings(shouldSettings);

#define SET_AND_CHECK2(setting, value) SET_AND_CHECK(setting, value, value)
    SET_AND_CHECK2(lockedTeams, true);
    SET_AND_CHECK2(lockedTeams, false);
    SET_AND_CHECK2(teamView, true);
    SET_AND_CHECK2(teamView, false);
    SET_AND_CHECK2(randomStartPosition, true);
    SET_AND_CHECK2(randomStartPosition, false);

#undef SET_AND_CHECK
#undef SET_AND_CHECK2

    // And multiple settings at once:
    executeLua("rttr:SetGameSettings({fow=EXP_FOGOFWARE_EXPLORED, speed=GS_VERYFAST, lockedTeams=true})");
    shouldSettings.exploration = Exploration::FogOfWarExplored;
    shouldSettings.speed = GameSpeed::VeryFast;
    shouldSettings.lockedTeams = true;
    checkSettings(shouldSettings);

    // Reset all settings
    executeLua("rttr:ResetGameSettings()");
    shouldSettings.speed = GameSpeed::Normal;
    shouldSettings.objective = GameObjective::None;
    shouldSettings.startWares = StartWares::Normal;
    shouldSettings.lockedTeams = false;
    shouldSettings.exploration = Exploration::FogOfWar;
    shouldSettings.teamView = true;
    shouldSettings.randomStartPosition = false;
    checkSettings(shouldSettings);
    BOOST_TEST_REQUIRE(ggs.getNumAddons() == shouldSettings.getNumAddons());
    for(unsigned i = 0; i < ggs.getNumAddons(); i++)
    {
        const Addon* curAddon = ggs.getAddon(i);
        BOOST_TEST_REQUIRE(ggs.getSelection(curAddon->getId()) == curAddon->getDefaultStatus());
        BOOST_TEST_REQUIRE(!ggs.isEnabled(curAddon->getId()));
    }
}

BOOST_AUTO_TEST_CASE(PlayerSettings)
{
    LogAccessor logAcc;
    executeLua("player = rttr:GetPlayer(0)");
    executeLua("player1 = rttr:GetPlayer(1)");
    executeLua("player:SetNation(NAT_ROMANS)");
    BOOST_TEST_REQUIRE(players[0].nation == Nation::Romans);
    executeLua("player1:SetNation(NAT_BABYLONIANS)");
    BOOST_TEST_REQUIRE(players[1].nation == Nation::Babylonians);

    // Check some properties
    BOOST_TEST(isLuaEqual("player:GetNation()", "NAT_ROMANS"));
    BOOST_TEST(isLuaEqual("player:IsHuman()", "true"));
    BOOST_TEST(isLuaEqual("player:IsAI()", "false"));
    BOOST_TEST(isLuaEqual("player:IsClosed()", "false"));
    BOOST_TEST(isLuaEqual("player:IsFree()", "false"));

    executeLua("player:SetTeam(TM_TEAM2)");
    BOOST_TEST_REQUIRE(players[0].team == Team::Team2);

    executeLua("player:SetName(\"Foo\")");
    BOOST_TEST(isLuaEqual("player:GetName()", "'Foo'"));

    executeLua("player:SetPortrait(8)");
    BOOST_TEST(players[0].portraitIndex == 8u);

    executeLua("player:SetColor(2)");
    BOOST_TEST_REQUIRE(players[0].color == PLAYER_COLORS[2]);
    executeLua("player:SetColor(0xFF0000FF)");
    BOOST_TEST_REQUIRE(players[0].color == 0xFF0000FF);
    executeLua("player1:SetColor(2)");
    BOOST_TEST_REQUIRE(players[1].color == PLAYER_COLORS[2]);
    // Duplicate color are allowed
    executeLua("player1:SetColor(0xFF0000FF)");
    BOOST_TEST_REQUIRE(players[0].color == 0xFF0000FF);
    BOOST_TEST_REQUIRE(players[1].color == 0xFF0000FF);

    // Kick AI
    executeLua("player1:Close()");
    BOOST_TEST_REQUIRE(players[1].ps == PlayerState::Locked);
    // Kick Human
    players[1].ps = PlayerState::Occupied;
    executeLua("player1:Close()");
    BOOST_TEST_REQUIRE(players[1].ps == PlayerState::Locked);
    // Kick none
    executeLua("player1:Close()");
    BOOST_TEST_REQUIRE(players[1].ps == PlayerState::Locked);

    executeLua("player:SetAI(0)");
    BOOST_TEST_REQUIRE(players[0].ps == PlayerState::AI);
    BOOST_TEST_REQUIRE(players[0].aiInfo.type == AI::Type::Dummy); //-V807
    executeLua("player:SetAI(1)");
    BOOST_TEST_REQUIRE(players[0].ps == PlayerState::AI);
    BOOST_TEST_REQUIRE(players[0].aiInfo.type == AI::Type::Default);
    BOOST_TEST_REQUIRE(players[0].aiInfo.level == AI::Level::Easy);
    executeLua("player:SetAI(2)");
    BOOST_TEST_REQUIRE(players[0].ps == PlayerState::AI);
    BOOST_TEST_REQUIRE(players[0].aiInfo.type == AI::Type::Default);
    BOOST_TEST_REQUIRE(players[0].aiInfo.level == AI::Level::Medium);
    executeLua("player:SetAI(3)");
    BOOST_TEST_REQUIRE(players[0].ps == PlayerState::AI);
    BOOST_TEST_REQUIRE(players[0].aiInfo.type == AI::Type::Default);
    BOOST_TEST_REQUIRE(players[0].aiInfo.level == AI::Level::Hard);
    // Invalid lvl
    BOOST_REQUIRE_THROW(executeLua("player:SetAI(4)"), std::exception);
    RTTR_REQUIRE_LOG_CONTAINS("Invalid AI", false);
}

BOOST_AUTO_TEST_SUITE_END()
