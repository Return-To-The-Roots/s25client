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

#include "defines.h" // IWYU pragma: keep
#include "LuaInterfaceGame.h"
#include "lua/LuaPlayer.h"
#include "lua/LuaWorld.h"
#include "GameClient.h"
#include "ingameWindows/iwMissionStatement.h"
#include "buildings/nobBaseWarehouse.h"
#include "WindowManager.h"
#include "GlobalVars.h"
#include "PostMsg.h"
#include "libutil/src/Serializer.h"
#include "libutil/src/Log.h"
#include <fstream>

// Include last!
#include "DebugNew.h" // IWYU pragma: keep

LuaInterfaceGame::LuaInterfaceGame(GameWorldGame& gw): gw(gw)
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

#undef ADD_LUA_CONST
#pragma endregion ConstDefs

    Register(lua);
    LuaPlayer::Register(lua);
    LuaWorld::Register(lua);

    lua["rttr"] = this;
}

LuaInterfaceGame::~LuaInterfaceGame()
{}

void LuaInterfaceGame::Register(kaguya::State& state)
{
    state["RTTRGame"].setClass(kaguya::ClassMetatable<LuaInterfaceGame, LuaInterfaceBase>()
        .addMemberFunction("ClearResources", &LuaInterfaceGame::ClearResources)
        .addMemberFunction("GetGF", &LuaInterfaceGame::GetGF)
        .addMemberFunction("GetGameFrame", &LuaInterfaceGame::GetGF)
        .addMemberFunction("GetPlayerCount", &LuaInterfaceGame::GetPlayerCount)
        .addMemberFunction("Chat", &LuaInterfaceGame::Chat)
        .addMemberFunction("MissionStatement", &LuaInterfaceGame::MissionStatement)
        .addMemberFunction("PostMessage", &LuaInterfaceGame::PostMessageLua)
        .addMemberFunction("PostMessageWithLocation", &LuaInterfaceGame::PostMessageWithLocation)
        .addMemberFunction("GetPlayer", &LuaInterfaceGame::GetPlayer)
        .addMemberFunction("GetWorld", &LuaInterfaceGame::GetWorld)
        );
    state["RTTR_Serializer"].setClass(kaguya::ClassMetatable<Serializer>()
        .addMemberFunction("PushInt", &Serializer::PushSignedInt)
        .addMemberFunction("PopInt", &Serializer::PopSignedInt)
        .addMemberFunction("PushBool", &Serializer::PushBool)
        .addMemberFunction("PopBool", &Serializer::PopBool)
        .addMemberFunction("PushString", &Serializer::PushString)
        .addMemberFunction("PopString", &Serializer::PopString)
        );
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
            if(!save.call<bool>(luaSaveState))
            {
                LOG.lprintf("Lua state could not be saved!");
                luaSaveState.Clear();
            }
            lua.setErrorHandler(ErrorHandler);
            return luaSaveState;
        } catch(std::exception& e)
        {
            lua.setErrorHandler(ErrorHandler);
            LOG.lprintf("Error during saving: %s\n", e.what());
            if(GLOBALVARS.isTest)
                throw std::runtime_error("Error during lua call");
        }
    }
    return Serializer();
}

void LuaInterfaceGame::Deserialize(Serializer& luaSaveState)
{
    kaguya::LuaRef load = lua["onLoad"];
    if(load.type() == LUA_TFUNCTION)
    {
        lua.setErrorHandler(ErrorHandlerThrow);
        try
        {
            if(!load.call<bool>(luaSaveState))
                LOG.lprintf("Lua state was not loaded correctly!");
            lua.setErrorHandler(ErrorHandler);
        } catch(std::exception& e)
        {
            lua.setErrorHandler(ErrorHandler);
            LOG.lprintf("Error during loading: %s\n", e.what());
            if(GLOBALVARS.isTest)
                throw std::runtime_error("Error during lua call");
        }
    }
}

void LuaInterfaceGame::ClearResources()
{
    for(unsigned p = 0; p < GAMECLIENT.GetPlayerCount(); p++)
    {
        const std::list<nobBaseWarehouse*> warehouses = GAMECLIENT.GetPlayer(p).GetStorehouses();
        for(std::list<nobBaseWarehouse*>::const_iterator wh = warehouses.begin(); wh != warehouses.end(); ++wh)
            (*wh)->Clear();
    }
}

unsigned LuaInterfaceGame::GetGF()
{
    return GAMECLIENT.GetGFNumber();
}

unsigned LuaInterfaceGame::GetPlayerCount()
{
    return GAMECLIENT.GetPlayerCount();
}

void LuaInterfaceGame::Chat(int playerIdx, const std::string& msg)
{
    if(playerIdx >= 0 && GAMECLIENT.GetPlayerID() != unsigned(playerIdx))
        return;

    GAMECLIENT.SystemChat(msg);
}

void LuaInterfaceGame::MissionStatement(int playerIdx, const std::string& title, const std::string& msg)
{
    if(playerIdx >= 0 && GAMECLIENT.GetPlayerID() != unsigned(playerIdx))
        return;

    WINDOWMANAGER.Show(new iwMissionStatement(title, msg));
}

// Must not be PostMessage as this is a windows define :(
void LuaInterfaceGame::PostMessageLua(unsigned playerIdx, const std::string& msg)
{
    if(GAMECLIENT.GetPlayerID() != playerIdx)
        return;

    GAMECLIENT.SendPostMessage(new PostMsg(msg, PMC_OTHER));
}

void LuaInterfaceGame::PostMessageWithLocation(unsigned playerIdx, const std::string& msg, int x, int y)
{
    if(GAMECLIENT.GetPlayerID() != playerIdx)
        return;

    GAMECLIENT.SendPostMessage(new PostMsgWithLocation(msg, PMC_OTHER, gw.MakeMapPoint(Point<int>(x, y))));
}

LuaPlayer LuaInterfaceGame::GetPlayer(unsigned playerIdx)
{
    if(playerIdx >= GAMECLIENT.GetPlayerCount())
        throw std::runtime_error("Invalid player idx");
    return LuaPlayer(GAMECLIENT.GetPlayer(playerIdx));
}

LuaWorld LuaInterfaceGame::GetWorld()
{
    return LuaWorld(gw);
}

void LuaInterfaceGame::EventExplored(unsigned player, const MapPoint pt)
{
    kaguya::LuaRef onExplored = lua["onExplored"];
    if(onExplored.type() == LUA_TFUNCTION)
        onExplored.call<void>(player, pt.x, pt.y);
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

void LuaInterfaceGame::EventResourceFound(const unsigned char player, const MapPoint pt, const unsigned char type, const unsigned char quantity)
{
    kaguya::LuaRef onResourceFound = lua["onResourceFound"];
    if(onResourceFound.type() == LUA_TFUNCTION)
        onResourceFound.call<void>(player, pt.x, pt.y, type, quantity);
}
