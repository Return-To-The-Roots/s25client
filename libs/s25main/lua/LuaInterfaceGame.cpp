// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
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

#include "LuaInterfaceGame.h"
#include "EventManager.h"
#include "Game.h"
#include "WindowManager.h"
#include "ai/AIInterface.h"
#include "ai/AIPlayer.h"
#include "ingameWindows/iwMissionStatement.h"
#include "lua/LuaHelpers.h"
#include "lua/LuaPlayer.h"
#include "lua/LuaWorld.h"
#include "postSystem/PostMsg.h"
#include "world/GameWorldGame.h"
#include "gameTypes/Resource.h"
#include "s25util/Serializer.h"

LuaInterfaceGame::LuaInterfaceGame(const std::weak_ptr<Game>& gameInstance, ILocalGameState& localGameState)
    : LuaInterfaceGameBase(localGameState), localGameState(localGameState), gw(gameInstance.lock()->world_), game(gameInstance)
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

    ADD_LUA_CONST(NON_AGGRESSION_PACT);
    ADD_LUA_CONST(TREATY_OF_ALLIANCE);
    // infinite pact duration, see GamePlayer::GetRemainingPactTime
    ADD_LUA_CONST(DURATION_INFINITE);

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

LuaInterfaceGame::~LuaInterfaceGame() = default;

KAGUYA_MEMBER_FUNCTION_OVERLOADS(SetMissionGoalWrapper, LuaInterfaceGame, SetMissionGoal, 1, 2)

void LuaInterfaceGame::Register(kaguya::State& state)
{
    state["RTTRGame"].setClass(kaguya::UserdataMetatable<LuaInterfaceGame, LuaInterfaceGameBase>()
                                 .addFunction("ClearResources", &LuaInterfaceGame::ClearResources)
                                 .addFunction("GetGF", &LuaInterfaceGame::GetGF)
                                 .addFunction("FormatNumGFs", &LuaInterfaceGame::FormatNumGFs)
                                 .addFunction("GetGameFrame", &LuaInterfaceGame::GetGF)
                                 .addFunction("GetNumPlayers", &LuaInterfaceGame::GetNumPlayers)
                                 .addFunction("Chat", &LuaInterfaceGame::Chat)
                                 .addOverloadedFunctions("MissionStatement", &LuaInterfaceGame::MissionStatement,
                                                         &LuaInterfaceGame::MissionStatement2, &LuaInterfaceGame::MissionStatement3)
                                 .addFunction("SetMissionGoal", SetMissionGoalWrapper())
                                 .addFunction("PostMessage", &LuaInterfaceGame::PostMessageLua)
                                 .addFunction("PostMessageWithLocation", &LuaInterfaceGame::PostMessageWithLocation)
                                 .addFunction("GetPlayer", &LuaInterfaceGame::GetPlayer)
                                 .addFunction("GetWorld", &LuaInterfaceGame::GetWorld)
                                 // Old name
                                 .addFunction("GetPlayerCount", &LuaInterfaceGame::GetNumPlayers));
    state["RTTR_Serializer"].setClass(kaguya::UserdataMetatable<Serializer>()
                                        .addFunction("PushInt", &Serializer::PushSignedInt)
                                        .addFunction("PopInt", &Serializer::PopSignedInt)
                                        .addFunction("PushBool", &Serializer::PushBool)
                                        .addFunction("PopBool", &Serializer::PopBool)
                                        .addFunction("PushString", &Serializer::PushString)
                                        .addFunction("PopString", &Serializer::PopString));
}

bool LuaInterfaceGame::Serialize(Serializer& luaSaveState)
{
    kaguya::LuaRef save = lua["onSave"];
    if(save.type() == LUA_TFUNCTION)
    {
        clearErrorOccured();
        if(save.call<bool>(kaguya::standard::ref(luaSaveState)) && !hasErrorOccurred())
            return true;
        else
        {
            luaSaveState.Clear();
            return false;
        }
    } else
        return true;
}

bool LuaInterfaceGame::Deserialize(Serializer& luaSaveState)
{
    kaguya::LuaRef load = lua["onLoad"];
    if(load.type() == LUA_TFUNCTION)
    {
        clearErrorOccured();
        return load.call<bool>(kaguya::standard::ref(luaSaveState)) && !hasErrorOccurred();
    } else
        return true;
}

void LuaInterfaceGame::ClearResources()
{
    for(unsigned p = 0; p < gw.GetNumPlayers(); p++)
        GetPlayer(p).ClearResources();
}

unsigned LuaInterfaceGame::GetGF() const
{
    return gw.GetEvMgr().GetCurrentGF();
}

std::string LuaInterfaceGame::FormatNumGFs(unsigned numGFs) const
{
    return localGameState.FormatGFTime(numGFs);
}

unsigned LuaInterfaceGame::GetNumPlayers() const
{
    return gw.GetNumPlayers();
}

void LuaInterfaceGame::Chat(int playerIdx, const std::string& msg)
{
    if(playerIdx >= 0 && localGameState.GetPlayerId() != unsigned(playerIdx))
        return;

    localGameState.SystemChat(msg);
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
    if(playerIdx >= 0 && localGameState.GetPlayerId() != unsigned(playerIdx))
        return;

    WINDOWMANAGER.Show(
      std::make_unique<iwMissionStatement>(_(title), msg, gw.IsSinglePlayer() && pause, iwMissionStatement::HelpImage(imgIdx)));
}

void LuaInterfaceGame::SetMissionGoal(int playerIdx, const std::string& newGoal)
{
    gw.GetPostMgr().SetMissionGoal(playerIdx, newGoal);
}

// Must not be PostMessage as this is a windows define :(
void LuaInterfaceGame::PostMessageLua(int playerIdx, const std::string& msg)
{
    lua::assertTrue(playerIdx >= 0, "Invalid player idx");
    gw.GetPostMgr().SendMsg(static_cast<unsigned>(playerIdx),
                            std::make_unique<PostMsg>(gw.GetEvMgr().GetCurrentGF(), msg, PostCategory::General));
}

void LuaInterfaceGame::PostMessageWithLocation(int playerIdx, const std::string& msg, int x, int y)
{
    lua::assertTrue(playerIdx >= 0, "Invalid player idx");
    gw.GetPostMgr().SendMsg(
      static_cast<unsigned>(playerIdx),
      std::make_unique<PostMsg>(gw.GetEvMgr().GetCurrentGF(), msg, PostCategory::General, gw.MakeMapPoint(Position(x, y))));
}

LuaPlayer LuaInterfaceGame::GetPlayer(int playerIdx)
{
    lua::assertTrue(playerIdx >= 0 && static_cast<unsigned>(playerIdx) < gw.GetNumPlayers(), "Invalid player idx");
    return LuaPlayer(game, gw.GetPlayer(static_cast<unsigned>(playerIdx)));
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

bool LuaInterfaceGame::EventCancelPactRequest(PactType pt, unsigned char canceledByPlayerId, unsigned char targetPlayerId)
{
    kaguya::LuaRef onPactCancel = lua["onCancelPactRequest"];
    if(onPactCancel.type() == LUA_TFUNCTION)
        return onPactCancel.call<bool>(pt, canceledByPlayerId, targetPlayerId);
    return true; // always accept pact cancel if there is no handler
}

void LuaInterfaceGame::EventSuggestPact(const PactType pt, unsigned char suggestedByPlayerId, unsigned char targetPlayerId,
                                        const unsigned duration)
{
    auto gameInst = game.lock();
    if(!gameInst)
        return;
    AIPlayer* ai = gameInst->GetAIPlayer(targetPlayerId);
    if(ai != nullptr)
    {
        kaguya::LuaRef onPactCancel = lua["onSuggestPact"];
        if(onPactCancel.type() == LUA_TFUNCTION)
        {
            AIInterface& aii = ai->getAIInterface();
            auto luaResult = onPactCancel.call<bool>(pt, suggestedByPlayerId, targetPlayerId, duration);
            if(luaResult)
                aii.AcceptPact(gw.GetEvMgr().GetCurrentGF(), pt, suggestedByPlayerId);
            else
                aii.CancelPact(pt, suggestedByPlayerId);
        }
    }
}

void LuaInterfaceGame::EventPactCanceled(const PactType pt, unsigned char canceledByPlayerId, unsigned char targetPlayerId)
{
    kaguya::LuaRef onPactCanceled = lua["onPactCanceled"];
    if(onPactCanceled.type() == LUA_TFUNCTION)
    {
        onPactCanceled.call<void>(pt, canceledByPlayerId, targetPlayerId);
    }
}

void LuaInterfaceGame::EventPactCreated(const PactType pt, unsigned char suggestedByPlayerId, unsigned char targetPlayerId,
                                        const unsigned duration)
{
    kaguya::LuaRef onPactCreated = lua["onPactCreated"];
    if(onPactCreated.type() == LUA_TFUNCTION)
    {
        onPactCreated.call<void>(pt, suggestedByPlayerId, targetPlayerId, duration);
    }
}
