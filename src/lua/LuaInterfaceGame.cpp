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

#include "defines.h" // IWYU pragma: keep
#include "LuaInterfaceGame.h"
#include "EventManager.h"
#include "GameClient.h"
#include "GlobalVars.h"
#include "WindowManager.h"
#include "ingameWindows/iwMissionStatement.h"
#include "lua/LuaPlayer.h"
#include "lua/LuaWorld.h"
#include "postSystem/PostMsg.h"
#include "world/GameWorldGame.h"
#include "gameTypes/Resource.h"
#include "libutil/Log.h"
#include "libutil/Serializer.h"
#include <boost/nowide/fstream.hpp>

LuaInterfaceGame::LuaInterfaceGame(GameWorldGame& gw) : gw(gw)
{
#pragma region ConstDefs
#define ADD_LUA_CONST(name) lua[#name] = name

    ADD_LUA_CONST(BLD_HEADQUARTERS);
    ADD_LUA_CONST(BLD_BARRACKS);
    ADD_LUA_CONST(BLD_GUARDHOUSE);
    ADD_LUA_CONST(BLD_WATCHTOWER);
    ADD_LUA_CONST(BLD_FORTRESS);
    ADD_LUA_CONST(BLD_GRANITEMINE);
    ADD_LUA_CONST(BLD_COALMINE);
    ADD_LUA_CONST(BLD_IRONMINE);
    ADD_LUA_CONST(BLD_GOLDMINE);
    ADD_LUA_CONST(BLD_LOOKOUTTOWER);
    ADD_LUA_CONST(BLD_CATAPULT);
    ADD_LUA_CONST(BLD_WOODCUTTER);
    ADD_LUA_CONST(BLD_FISHERY);
    ADD_LUA_CONST(BLD_QUARRY);
    ADD_LUA_CONST(BLD_FORESTER);
    ADD_LUA_CONST(BLD_SLAUGHTERHOUSE);
    ADD_LUA_CONST(BLD_HUNTER);
    ADD_LUA_CONST(BLD_BREWERY);
    ADD_LUA_CONST(BLD_ARMORY);
    ADD_LUA_CONST(BLD_METALWORKS);
    ADD_LUA_CONST(BLD_IRONSMELTER);
    ADD_LUA_CONST(BLD_CHARBURNER);
    ADD_LUA_CONST(BLD_PIGFARM);
    ADD_LUA_CONST(BLD_STOREHOUSE);
    ADD_LUA_CONST(BLD_MILL);
    ADD_LUA_CONST(BLD_BAKERY);
    ADD_LUA_CONST(BLD_SAWMILL);
    ADD_LUA_CONST(BLD_MINT);
    ADD_LUA_CONST(BLD_WELL);
    ADD_LUA_CONST(BLD_SHIPYARD);
    ADD_LUA_CONST(BLD_FARM);
    ADD_LUA_CONST(BLD_DONKEYBREEDER);
    ADD_LUA_CONST(BLD_HARBORBUILDING);

    ADD_LUA_CONST(JOB_HELPER);
    ADD_LUA_CONST(JOB_WOODCUTTER);
    ADD_LUA_CONST(JOB_FISHER);
    ADD_LUA_CONST(JOB_FORESTER);
    ADD_LUA_CONST(JOB_CARPENTER);
    ADD_LUA_CONST(JOB_STONEMASON);
    ADD_LUA_CONST(JOB_HUNTER);
    ADD_LUA_CONST(JOB_FARMER);
    ADD_LUA_CONST(JOB_MILLER);
    ADD_LUA_CONST(JOB_BAKER);
    ADD_LUA_CONST(JOB_BUTCHER);
    ADD_LUA_CONST(JOB_MINER);
    ADD_LUA_CONST(JOB_BREWER);
    ADD_LUA_CONST(JOB_PIGBREEDER);
    ADD_LUA_CONST(JOB_DONKEYBREEDER);
    ADD_LUA_CONST(JOB_IRONFOUNDER);
    ADD_LUA_CONST(JOB_MINTER);
    ADD_LUA_CONST(JOB_METALWORKER);
    ADD_LUA_CONST(JOB_ARMORER);
    ADD_LUA_CONST(JOB_BUILDER);
    ADD_LUA_CONST(JOB_PLANER);
    ADD_LUA_CONST(JOB_PRIVATE);
    ADD_LUA_CONST(JOB_PRIVATEFIRSTCLASS);
    ADD_LUA_CONST(JOB_SERGEANT);
    ADD_LUA_CONST(JOB_OFFICER);
    ADD_LUA_CONST(JOB_GENERAL);
    ADD_LUA_CONST(JOB_GEOLOGIST);
    ADD_LUA_CONST(JOB_SHIPWRIGHT);
    ADD_LUA_CONST(JOB_SCOUT);
    ADD_LUA_CONST(JOB_PACKDONKEY);
    ADD_LUA_CONST(JOB_CHARBURNER);

    ADD_LUA_CONST(GD_BEER);
    ADD_LUA_CONST(GD_TONGS);
    ADD_LUA_CONST(GD_HAMMER);
    ADD_LUA_CONST(GD_AXE);
    ADD_LUA_CONST(GD_SAW);
    ADD_LUA_CONST(GD_PICKAXE);
    ADD_LUA_CONST(GD_SHOVEL);
    ADD_LUA_CONST(GD_CRUCIBLE);
    ADD_LUA_CONST(GD_RODANDLINE);
    ADD_LUA_CONST(GD_SCYTHE);
    ADD_LUA_CONST(GD_WATER);
    ADD_LUA_CONST(GD_CLEAVER);
    ADD_LUA_CONST(GD_ROLLINGPIN);
    ADD_LUA_CONST(GD_BOW);
    ADD_LUA_CONST(GD_BOAT);
    ADD_LUA_CONST(GD_SWORD);
    ADD_LUA_CONST(GD_IRON);
    ADD_LUA_CONST(GD_FLOUR);
    ADD_LUA_CONST(GD_FISH);
    ADD_LUA_CONST(GD_BREAD);
    lua["GD_SHIELD"] = GD_SHIELDROMANS;
    ADD_LUA_CONST(GD_WOOD);
    ADD_LUA_CONST(GD_BOARDS);
    ADD_LUA_CONST(GD_STONES);
    ADD_LUA_CONST(GD_GRAIN);
    ADD_LUA_CONST(GD_COINS);
    ADD_LUA_CONST(GD_GOLD);
    ADD_LUA_CONST(GD_IRONORE);
    ADD_LUA_CONST(GD_COAL);
    ADD_LUA_CONST(GD_MEAT);
    ADD_LUA_CONST(GD_HAM);

    lua["RES_IRON"] = Resource::Iron;
    lua["RES_GOLD"] = Resource::Gold;
    lua["RES_COAL"] = Resource::Coal;
    lua["RES_GRANITE"] = Resource::Granite;
    lua["RES_WATER"] = Resource::Water;

#undef ADD_LUA_CONST
#define ADD_LUA_CONST(name) lua[#name] = iwMissionStatement::name
    ADD_LUA_CONST(IM_NONE);
    ADD_LUA_CONST(IM_SWORDSMAN);
    ADD_LUA_CONST(IM_READER);
    ADD_LUA_CONST(IM_RIDER);
    ADD_LUA_CONST(IM_AVATAR1);
    ADD_LUA_CONST(IM_AVATAR2);
    ADD_LUA_CONST(IM_AVATAR3);
    ADD_LUA_CONST(IM_AVATAR4);
    ADD_LUA_CONST(IM_AVATAR5);
    ADD_LUA_CONST(IM_AVATAR6);
    ADD_LUA_CONST(IM_AVATAR7);
    ADD_LUA_CONST(IM_AVATAR8);
    ADD_LUA_CONST(IM_AVATAR9);
    ADD_LUA_CONST(IM_AVATAR10);
    ADD_LUA_CONST(IM_AVATAR11);
    ADD_LUA_CONST(IM_AVATAR12);

#undef ADD_LUA_CONST
#pragma endregion ConstDefs

    Register(lua);
    LuaPlayer::Register(lua);
    LuaWorld::Register(lua);

    lua["rttr"] = this;
}

LuaInterfaceGame::~LuaInterfaceGame() {}

KAGUYA_MEMBER_FUNCTION_OVERLOADS(SetMissionGoalWrapper, LuaInterfaceGame, SetMissionGoal, 1, 2)

void LuaInterfaceGame::Register(kaguya::State& state)
{
    state["RTTRGame"].setClass(kaguya::UserdataMetatable<LuaInterfaceGame, LuaInterfaceBase>()
                                 .addFunction("ClearResources", &LuaInterfaceGame::ClearResources)
                                 .addFunction("GetGF", &LuaInterfaceGame::GetGF)
                                 .addFunction("GetGameFrame", &LuaInterfaceGame::GetGF)
                                 .addFunction("GetPlayerCount", &LuaInterfaceGame::GetPlayerCount)
                                 .addFunction("Chat", &LuaInterfaceGame::Chat)
                                 .addOverloadedFunctions("MissionStatement", &LuaInterfaceGame::MissionStatement,
                                                         &LuaInterfaceGame::MissionStatement2, &LuaInterfaceGame::MissionStatement3)
                                 .addFunction("SetMissionGoal", SetMissionGoalWrapper())
                                 .addFunction("PostMessage", &LuaInterfaceGame::PostMessageLua)
                                 .addFunction("PostMessageWithLocation", &LuaInterfaceGame::PostMessageWithLocation)
                                 .addFunction("GetPlayer", &LuaInterfaceGame::GetPlayer)
                                 .addFunction("GetWorld", &LuaInterfaceGame::GetWorld));
    state["RTTR_Serializer"].setClass(kaguya::UserdataMetatable<Serializer>()
                                        .addFunction("PushInt", &Serializer::PushSignedInt)
                                        .addFunction("PopInt", &Serializer::PopSignedInt)
                                        .addFunction("PushBool", &Serializer::PushBool)
                                        .addFunction("PopBool", &Serializer::PopBool)
                                        .addFunction("PushString", &Serializer::PushString)
                                        .addFunction("PopString", &Serializer::PopString));
    state.setErrorHandler(ErrorHandler);
}

Serializer LuaInterfaceGame::Serialize()
{
    kaguya::LuaRef save = lua["onSave"];
    if(save.type() == LUA_TFUNCTION)
    {
        Serializer luaSaveState;
        lua.setErrorHandler(ErrorHandlerThrow);
        try
        {
            if(!save.call<bool>(kaguya::standard::ref(luaSaveState)))
            {
                LOG.write("Lua state could not be saved!");
                luaSaveState.Clear();
            }
            lua.setErrorHandler(ErrorHandler);
            return luaSaveState;
        } catch(std::exception& e)
        {
            lua.setErrorHandler(ErrorHandler);
            LOG.write("Error during saving: %s\n") % e.what();
            if(GLOBALVARS.isTest)
                throw std::runtime_error("Error during lua call");
        }
    }
    return Serializer();
}

bool LuaInterfaceGame::Deserialize(Serializer& luaSaveState)
{
    kaguya::LuaRef load = lua["onLoad"];
    if(load.type() == LUA_TFUNCTION)
    {
        lua.setErrorHandler(ErrorHandlerThrow);
        try
        {
            bool result = load.call<bool>(kaguya::standard::ref(luaSaveState));
            lua.setErrorHandler(ErrorHandler);
            if(result)
                return true;
            else
            {
                LOG.write("Lua state was not loaded correctly!");
                return false;
            }
        } catch(std::exception& e)
        {
            lua.setErrorHandler(ErrorHandler);
            LOG.write("Error during loading: %s\n") % e.what();
            if(GLOBALVARS.isTest)
                throw std::runtime_error("Error during lua call");
            return false;
        }
    } else
        return true;
}

void LuaInterfaceGame::ClearResources()
{
    for(unsigned p = 0; p < gw.GetPlayerCount(); p++)
        GetPlayer(p).ClearResources();
}

unsigned LuaInterfaceGame::GetGF()
{
    return gw.GetEvMgr().GetCurrentGF();
}

unsigned LuaInterfaceGame::GetPlayerCount()
{
    return gw.GetPlayerCount();
}

void LuaInterfaceGame::Chat(int playerIdx, const std::string& msg)
{
    if(playerIdx >= 0 && GAMECLIENT.GetPlayerId() != unsigned(playerIdx))
        return;

    GAMECLIENT.SystemChat(msg);
}

void LuaInterfaceGame::MissionStatement(int playerIdx, const std::string& title, const std::string& msg)
{
    MissionStatement2(playerIdx, title, msg, iwMissionStatement::IM_SWORDSMAN);
}

void LuaInterfaceGame::MissionStatement2(int playerIdx, const std::string& title, const std::string& msg, unsigned imgIdx)
{
    MissionStatement3(playerIdx, title, msg, imgIdx, true);
}

void LuaInterfaceGame::MissionStatement3(int playerIdx, const std::string& title, const std::string& msg, unsigned imgIdx, bool pause)
{
    if(playerIdx >= 0 && GAMECLIENT.GetPlayerId() != unsigned(playerIdx))
        return;

    WINDOWMANAGER.Show(new iwMissionStatement(_(title), msg, gw.IsSinglePlayer() && pause, iwMissionStatement::HelpImage(imgIdx)));
}

void LuaInterfaceGame::SetMissionGoal(int playerIdx, const std::string& newGoal)
{
    gw.GetPostMgr().SetMissionGoal(playerIdx, newGoal);
}

// Must not be PostMessage as this is a windows define :(
void LuaInterfaceGame::PostMessageLua(unsigned playerIdx, const std::string& msg)
{
    gw.GetPostMgr().SendMsg(playerIdx, new PostMsg(gw.GetEvMgr().GetCurrentGF(), msg, PostCategory::General));
}

void LuaInterfaceGame::PostMessageWithLocation(unsigned playerIdx, const std::string& msg, int x, int y)
{
    gw.GetPostMgr().SendMsg(playerIdx,
                            new PostMsg(gw.GetEvMgr().GetCurrentGF(), msg, PostCategory::General, gw.MakeMapPoint(Point<int>(x, y))));
}

LuaPlayer LuaInterfaceGame::GetPlayer(unsigned playerIdx)
{
    if(playerIdx >= gw.GetPlayerCount())
        throw std::runtime_error("Invalid player idx");
    return LuaPlayer(gw.GetPlayer(playerIdx));
}

LuaWorld LuaInterfaceGame::GetWorld()
{
    return LuaWorld(gw);
}

void LuaInterfaceGame::EventExplored(unsigned player, const MapPoint pt, unsigned char owner)
{
    kaguya::LuaRef onExplored = lua["onExplored"];
    if(onExplored.type() == LUA_TFUNCTION)
    {
        if(owner == 0)
        {
            // No owner? Pass nil value to Lua.
            onExplored.call<void>(player, pt.x, pt.y, kaguya::NilValue());
        } else
        {
            // Adapt owner to be comparable with the player index
            onExplored.call<void>(player, pt.x, pt.y, owner - 1);
        }
    }
}

void LuaInterfaceGame::EventOccupied(unsigned player, const MapPoint pt)
{
    kaguya::LuaRef onOccupied = lua["onOccupied"];
    if(onOccupied.type() == LUA_TFUNCTION)
        onOccupied.call<void>(player, pt.x, pt.y);
}

void LuaInterfaceGame::EventStart(bool isFirstStart)
{
    kaguya::LuaRef onStart = lua["onStart"];
    if(onStart.type() == LUA_TFUNCTION)
        onStart.call<void>(isFirstStart);
}

void LuaInterfaceGame::EventGameFrame(unsigned nr)
{
    kaguya::LuaRef onGameFrame = lua["onGameFrame"];
    if(onGameFrame.type() == LUA_TFUNCTION)
        onGameFrame.call<void>(nr);
}

void LuaInterfaceGame::EventResourceFound(unsigned char player, const MapPoint pt, unsigned char type, unsigned char quantity)
{
    kaguya::LuaRef onResourceFound = lua["onResourceFound"];
    if(onResourceFound.type() == LUA_TFUNCTION)
        onResourceFound.call<void>(player, pt.x, pt.y, type, quantity);
}
