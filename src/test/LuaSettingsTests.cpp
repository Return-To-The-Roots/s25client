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
#include "addons/Addon.h"
#include "lua/LuaInterfaceSettings.h"
#include "GameServerInterface.h"
#include "JoinPlayerInfo.h"
#include "GlobalVars.h"
#include "libutil/src/Log.h"
#include "libutil/src/StringStreamWriter.h"
#include <boost/test/unit_test.hpp>
#include <boost/format.hpp>
#include <boost/foreach.hpp>
#include <vector>
#include "colors.h"

namespace{

    struct LuaSettingsTestsFixture: public GameServerInterface
    {
        std::vector<JoinPlayerInfo> players;
        GlobalGameSettings ggs;
        LuaInterfaceSettings lua;
        StringStreamWriter& logWriter;

        bool IsRunning() const override { return true; }
        unsigned GetMaxPlayerCount() const override { return players.size(); }
        JoinPlayerInfo& GetJoinPlayer(unsigned playerIdx) override { return players.at(playerIdx); }
        void KickPlayer(unsigned playerIdx) override { GetJoinPlayer(playerIdx).ps = PS_FREE; }

        void CheckAndSetColor(unsigned playerIdx, unsigned newColor) override
        {
            while(true)
            {
                bool found = false;
                unsigned curId = 0;
                BOOST_FOREACH(JoinPlayerInfo& player, players)
                {
                    if(curId != playerIdx && player.isUsed() && newColor == player.color)
                    {
                        found = true;
                        break;
                    }
                    ++curId;
                }
                if(!found)
                {
                    GetJoinPlayer(playerIdx).color = newColor;
                    return;
                }
                newColor = rand() & (0xFF << 24);
            }
        }

        void AnnounceStatusChange() override {}
        const GlobalGameSettings& GetGGS() const override { return ggs; }
        void ChangeGlobalGameSettings(const GlobalGameSettings& ggs) override { this->ggs = ggs; }
        void SendToAll(const GameMessage& msg) override {}

        LuaSettingsTestsFixture(): lua(*this), logWriter(dynamic_cast<StringStreamWriter&>(*LOG.getFileWriter()))
        {
            GLOBALVARS.isTest = true;
            clearLog();
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

        ~LuaSettingsTestsFixture()
        {
            GLOBALVARS.isTest = false;
        }

        void clearLog()
        {
            logWriter.getStream().str("");
        }

        std::string getLog(bool clear = true)
        {
            std::string result = logWriter.getText();
            if(clear)
                clearLog();
            return result;
        }

        void executeLua(const std::string& luaCode)
        {
            lua.LoadScriptString(luaCode);
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

}

BOOST_FIXTURE_TEST_SUITE(LuaTestSuiteSettings, LuaSettingsTestsFixture)

BOOST_AUTO_TEST_CASE(AssertionThrows)
{
    BOOST_REQUIRE_THROW(executeLua("assert(false)"), std::runtime_error);
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
    executeLua("function onSettingsInit(isSinglePlayer, isSavegame)\n  rttr:Log('init: '..tostring(isSinglePlayer)..' '..tostring(isSavegame))\nend");
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
    BOOST_REQUIRE(!getLog().empty());
    executeLua("function getAllowedAddons()\n  return {9999}\nend");
    allowedAddons = lua.GetAllowedAddons();
    BOOST_REQUIRE(allowedAddons.empty());
    BOOST_REQUIRE(!getLog().empty());
}

BOOST_AUTO_TEST_CASE(SettingsFunctions)
{
    executeLua("assert(rttr:GetPlayer(0))");
    executeLua("assert(rttr:GetPlayer(1))");
    executeLua("assert(rttr:GetPlayer(2))");
    // Invalid player
    BOOST_REQUIRE_THROW(executeLua("assert(rttr:GetPlayer(3))"), std::runtime_error);
    BOOST_REQUIRE_THROW(executeLua("assert(rttr:GetPlayer(-1))"), std::runtime_error);

    executeLua("rttr:SetAddon(ADDON_LIMIT_CATAPULTS, 4)\n  rttr:SetAddon(ADDON_TRADE, true)\n");
    BOOST_REQUIRE_EQUAL(ggs.getSelection(AddonId::LIMIT_CATAPULTS), 4u);
    BOOST_REQUIRE(ggs.isEnabled(AddonId::TRADE));
    executeLua("rttr:SetAddon(ADDON_LIMIT_CATAPULTS, 1)\n  rttr:SetAddon(ADDON_TRADE, false)\n");
    BOOST_REQUIRE_EQUAL(ggs.getSelection(AddonId::LIMIT_CATAPULTS), 1u);
    BOOST_REQUIRE(!ggs.isEnabled(AddonId::TRADE));
    // Set some random options
    for(unsigned i = 0; i < ggs.getNumAddons(); i++)
        ggs.setSelection(ggs.getAddon(i)->getId(), rand() % ggs.getAddon(i)->getNumOptions());
    executeLua("rttr:ResetAddons()\n");
    for(unsigned i = 0; i < ggs.getNumAddons(); i++)
    {
        BOOST_REQUIRE_EQUAL(ggs.getSelection(ggs.getAddon(i)->getId()), ggs.getAddon(i)->getDefaultStatus());
        BOOST_REQUIRE(!ggs.isEnabled(ggs.getAddon(i)->getId()));
    }

    GlobalGameSettings shouldSettings = ggs;

#define SET_AND_CHECK(setting, value) executeLua("rttr:SetGameSettings({" #setting "=" #value "})"); \
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
        BOOST_REQUIRE_EQUAL(ggs.getSelection(ggs.getAddon(i)->getId()), ggs.getAddon(i)->getDefaultStatus());
        BOOST_REQUIRE(!ggs.isEnabled(ggs.getAddon(i)->getId()));
    }
}

BOOST_AUTO_TEST_CASE(PlayerSettings)
{
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
    BOOST_REQUIRE_EQUAL(players[0].aiInfo.type, AI::DUMMY);
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
}

BOOST_AUTO_TEST_SUITE_END()

