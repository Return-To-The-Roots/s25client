// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "LuaInterfaceGame.h"
#include "EventManager.h"
#include "Game.h"
#include "Settings.h"
#include "WindowManager.h"
#include "ai/AIInterface.h"
#include "ai/AIPlayer.h"
#include "ingameWindows/iwMissionStatement.h"
#include "lua/LuaHelpers.h"
#include "lua/LuaPlayer.h"
#include "lua/LuaWorld.h"
#include "network/GameClient.h"
#include "postSystem/PostMsg.h"
#include "world/GameWorld.h"
#include "gameTypes/Resource.h"
#include "gameData/CampaignSaveCodes.h"
#include "s25util/Serializer.h"
#include "s25util/strAlgos.h"

LuaInterfaceGame::LuaInterfaceGame(Game& gameInstance, ILocalGameState& localGameState)
    : LuaInterfaceGameBase(localGameState), localGameState(localGameState), gw(gameInstance.world_), game(gameInstance)
{
#pragma region ConstDefs
#define ADD_LUA_CONST(name) lua["BLD_" + s25util::toUpper(#name)] = BuildingType::name
    ADD_LUA_CONST(Headquarters);
    ADD_LUA_CONST(Barracks);
    ADD_LUA_CONST(Guardhouse);
    ADD_LUA_CONST(Watchtower);
    ADD_LUA_CONST(Fortress);
    ADD_LUA_CONST(GraniteMine);
    ADD_LUA_CONST(CoalMine);
    ADD_LUA_CONST(IronMine);
    ADD_LUA_CONST(GoldMine);
    ADD_LUA_CONST(LookoutTower);
    ADD_LUA_CONST(Catapult);
    ADD_LUA_CONST(Woodcutter);
    ADD_LUA_CONST(Fishery);
    ADD_LUA_CONST(Quarry);
    ADD_LUA_CONST(Forester);
    ADD_LUA_CONST(Slaughterhouse);
    ADD_LUA_CONST(Hunter);
    ADD_LUA_CONST(Brewery);
    ADD_LUA_CONST(Armory);
    ADD_LUA_CONST(Metalworks);
    ADD_LUA_CONST(Ironsmelter);
    ADD_LUA_CONST(Charburner);
    ADD_LUA_CONST(PigFarm);
    ADD_LUA_CONST(Storehouse);
    ADD_LUA_CONST(Mill);
    ADD_LUA_CONST(Bakery);
    ADD_LUA_CONST(Sawmill);
    ADD_LUA_CONST(Mint);
    ADD_LUA_CONST(Well);
    ADD_LUA_CONST(Shipyard);
    ADD_LUA_CONST(Farm);
    ADD_LUA_CONST(DonkeyBreeder);
    ADD_LUA_CONST(HarborBuilding);
    ADD_LUA_CONST(Vineyard);
    ADD_LUA_CONST(Winery);
    ADD_LUA_CONST(Temple);
#undef ADD_LUA_CONST

#define ADD_LUA_CONST(name) lua["JOB_" + s25util::toUpper(#name)] = Job::name
    ADD_LUA_CONST(Helper);
    ADD_LUA_CONST(Woodcutter);
    ADD_LUA_CONST(Fisher);
    ADD_LUA_CONST(Forester);
    ADD_LUA_CONST(Carpenter);
    ADD_LUA_CONST(Stonemason);
    ADD_LUA_CONST(Hunter);
    ADD_LUA_CONST(Farmer);
    ADD_LUA_CONST(Miller);
    ADD_LUA_CONST(Baker);
    ADD_LUA_CONST(Butcher);
    ADD_LUA_CONST(Miner);
    ADD_LUA_CONST(Brewer);
    ADD_LUA_CONST(PigBreeder);
    ADD_LUA_CONST(DonkeyBreeder);
    ADD_LUA_CONST(IronFounder);
    ADD_LUA_CONST(Minter);
    ADD_LUA_CONST(Metalworker);
    ADD_LUA_CONST(Armorer);
    ADD_LUA_CONST(Builder);
    ADD_LUA_CONST(Planer);
    ADD_LUA_CONST(Private);
    ADD_LUA_CONST(PrivateFirstClass);
    ADD_LUA_CONST(Sergeant);
    ADD_LUA_CONST(Officer);
    ADD_LUA_CONST(General);
    ADD_LUA_CONST(Geologist);
    ADD_LUA_CONST(Shipwright);
    ADD_LUA_CONST(Scout);
    ADD_LUA_CONST(PackDonkey);
    ADD_LUA_CONST(BoatCarrier);
    ADD_LUA_CONST(CharBurner);
    ADD_LUA_CONST(Winegrower);
    ADD_LUA_CONST(Vintner);
    ADD_LUA_CONST(TempleServant);
#undef ADD_LUA_CONST

#define ADD_LUA_CONST(name) lua["STAT_" + s25util::toUpper(#name)] = StatisticType::name
    ADD_LUA_CONST(Country);
    ADD_LUA_CONST(Buildings);
    ADD_LUA_CONST(Inhabitants);
    ADD_LUA_CONST(Merchandise);
    ADD_LUA_CONST(Military);
    ADD_LUA_CONST(Gold);
    ADD_LUA_CONST(Productivity);
    ADD_LUA_CONST(Vanquished);
    ADD_LUA_CONST(Tournament);
#undef ADD_LUA_CONST

#define ADD_LUA_CONST(name) lua["GD_" + s25util::toUpper(#name)] = GoodType::name
    ADD_LUA_CONST(Beer);
    ADD_LUA_CONST(Tongs);
    ADD_LUA_CONST(Hammer);
    ADD_LUA_CONST(Axe);
    ADD_LUA_CONST(Saw);
    ADD_LUA_CONST(PickAxe);
    ADD_LUA_CONST(Shovel);
    ADD_LUA_CONST(Crucible);
    ADD_LUA_CONST(RodAndLine);
    ADD_LUA_CONST(Scythe);
    ADD_LUA_CONST(Water);
    ADD_LUA_CONST(Cleaver);
    ADD_LUA_CONST(Rollingpin);
    ADD_LUA_CONST(Bow);
    ADD_LUA_CONST(Boat);
    ADD_LUA_CONST(Sword);
    ADD_LUA_CONST(Iron);
    ADD_LUA_CONST(Flour);
    ADD_LUA_CONST(Fish);
    ADD_LUA_CONST(Bread);
    lua["GD_SHIELD"] = GoodType::ShieldRomans;
    ADD_LUA_CONST(Wood);
    ADD_LUA_CONST(Boards);
    ADD_LUA_CONST(Stones);
    ADD_LUA_CONST(Grain);
    ADD_LUA_CONST(Coins);
    ADD_LUA_CONST(Gold);
    ADD_LUA_CONST(IronOre);
    ADD_LUA_CONST(Coal);
    ADD_LUA_CONST(Meat);
    ADD_LUA_CONST(Ham);
    ADD_LUA_CONST(Grapes);
    ADD_LUA_CONST(Wine);
#undef ADD_LUA_CONST

#define ADD_LUA_CONST(name) lua["RES_" + s25util::toUpper(#name)] = ResourceType::name
    ADD_LUA_CONST(Iron);
    ADD_LUA_CONST(Gold);
    ADD_LUA_CONST(Coal);
    ADD_LUA_CONST(Granite);
    ADD_LUA_CONST(Water);
#undef ADD_LUA_CONST

#define ADD_LUA_CONST(name) lua[#name] = name
    lua["NON_AGGRESSION_PACT"] = PactType::NonAgressionPact;
    lua["TREATY_OF_ALLIANCE"] = PactType::TreatyOfAlliance;
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
    state["RTTRGame"].setClass(
      kaguya::UserdataMetatable<LuaInterfaceGame, LuaInterfaceGameBase>()
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
        .addFunction("EnableCampaignChapter", &LuaInterfaceGame::EnableCampaignChapter)
        .addFunction("SetCampaignChapterCompleted", &LuaInterfaceGame::SetCampaignChapterCompleted)
        .addFunction("SetCampaignCompleted", &LuaInterfaceGame::SetCampaignCompleted)
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

void LuaInterfaceGame::MissionStatement2(int playerIdx, const std::string& title, const std::string& msg,
                                         unsigned imgIdx)
{
    MissionStatement3(playerIdx, title, msg, imgIdx, true);
}

void LuaInterfaceGame::MissionStatement3(int playerIdx, const std::string& title, const std::string& msg,
                                         unsigned imgIdx, bool pause)
{
    if(playerIdx >= 0 && localGameState.GetPlayerId() != unsigned(playerIdx))
        return;

    WINDOWMANAGER.Show(std::make_unique<iwMissionStatement>(_(title), msg, gw.IsSinglePlayer() && pause,
                                                            iwMissionStatement::HelpImage(imgIdx)));
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
    gw.GetPostMgr().SendMsg(static_cast<unsigned>(playerIdx),
                            std::make_unique<PostMsg>(gw.GetEvMgr().GetCurrentGF(), msg, PostCategory::General,
                                                      gw.MakeMapPoint(Position(x, y))));
}

namespace {
void MarkCampaignChapter(const std::string& campaignUid, unsigned char chapter, char code)
{
    if(chapter < 1)
        return;

    auto& saveData = SETTINGS.campaigns.saveData[campaignUid];

    if(saveData.length() < chapter)
        saveData.resize(chapter);

    auto& chapterSaveData = saveData[chapter - 1];

    if(chapterSaveData == CampaignSaveCodes::chapterCompleted)
        return;

    chapterSaveData = code;
}
} // namespace

void LuaInterfaceGame::EnableCampaignChapter(const std::string& campaignUid, unsigned char chapter)
{
    MarkCampaignChapter(campaignUid, chapter, CampaignSaveCodes::chapterEnabled);
}

void LuaInterfaceGame::SetCampaignChapterCompleted(const std::string& campaignUid, unsigned char chapter)
{
    MarkCampaignChapter(campaignUid, chapter, CampaignSaveCodes::chapterCompleted);
    GAMECLIENT.SetCampaignChapterCompleted(chapter);
}

void LuaInterfaceGame::SetCampaignCompleted(const std::string& /* campaignUid */)
{
    GAMECLIENT.SetCampaignCompleted(true);
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

void LuaInterfaceGame::EventAttack(unsigned char attackerPlayerId, unsigned char defenderPlayerId,
                                   unsigned attackerCount)
{
    kaguya::LuaRef onAttack = lua["onAttack"];
    if(onAttack.type() == LUA_TFUNCTION)
        onAttack.call<void>(attackerPlayerId, defenderPlayerId, attackerCount);
}

void LuaInterfaceGame::EventStart(bool isFirstStart)
{
    kaguya::LuaRef onStart = lua["onStart"];
    if(onStart.type() == LUA_TFUNCTION)
        onStart.call<void>(isFirstStart);
}

void LuaInterfaceGame::EventHumanWinner()
{
    kaguya::LuaRef onStart = lua["onHumanWinner"];
    if(onStart.type() == LUA_TFUNCTION)
        onStart.call<void>();
}

void LuaInterfaceGame::EventGameFrame(unsigned nr)
{
    kaguya::LuaRef onGameFrame = lua["onGameFrame"];
    if(onGameFrame.type() == LUA_TFUNCTION)
        onGameFrame.call<void>(nr);
}

void LuaInterfaceGame::EventResourceFound(unsigned char player, const MapPoint pt, ResourceType type,
                                          unsigned char quantity)
{
    kaguya::LuaRef onResourceFound = lua["onResourceFound"];
    if(onResourceFound.type() == LUA_TFUNCTION)
        onResourceFound.call<void>(player, pt.x, pt.y, type, quantity);
}

bool LuaInterfaceGame::EventCancelPactRequest(PactType pt, unsigned char canceledByPlayerId,
                                              unsigned char targetPlayerId)
{
    kaguya::LuaRef onPactCancel = lua["onCancelPactRequest"];
    if(onPactCancel.type() == LUA_TFUNCTION)
        return onPactCancel.call<bool>(pt, canceledByPlayerId, targetPlayerId);
    return true; // always accept pact cancel if there is no handler
}

void LuaInterfaceGame::EventSuggestPact(const PactType pt, unsigned char suggestedByPlayerId,
                                        unsigned char targetPlayerId, const unsigned duration)
{
    AIPlayer* ai = game.GetAIPlayer(targetPlayerId);
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

void LuaInterfaceGame::EventPactCanceled(const PactType pt, unsigned char canceledByPlayerId,
                                         unsigned char targetPlayerId)
{
    kaguya::LuaRef onPactCanceled = lua["onPactCanceled"];
    if(onPactCanceled.type() == LUA_TFUNCTION)
    {
        onPactCanceled.call<void>(pt, canceledByPlayerId, targetPlayerId);
    }
}

void LuaInterfaceGame::EventPactCreated(const PactType pt, unsigned char suggestedByPlayerId,
                                        unsigned char targetPlayerId, const unsigned duration)
{
    kaguya::LuaRef onPactCreated = lua["onPactCreated"];
    if(onPactCreated.type() == LUA_TFUNCTION)
    {
        onPactCreated.call<void>(pt, suggestedByPlayerId, targetPlayerId, duration);
    }
}
