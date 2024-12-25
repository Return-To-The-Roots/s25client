// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "LuaInterfaceGameBase.h"
#include "gameTypes/MapCoordinates.h"
#include "gameTypes/PactTypes.h"
#include "gameData/CampaignTypes.h"
#include <memory>
#include <string>

class GameWorld;
class LuaPlayer;
class LuaWorld;
class Serializer;
class Game;
enum class ResourceType : uint8_t;

class LuaInterfaceGame : public LuaInterfaceGameBase
{
public:
    // Passing Game by reference here relies on LuaInterfaceGame being part of Game
    LuaInterfaceGame(Game& gameInstance, ILocalGameState& localGameState);
    virtual ~LuaInterfaceGame();

    static void Register(kaguya::State& state);

    bool Serialize(Serializer& luaSaveState);
    bool Deserialize(Serializer& luaSaveState);

    void EventExplored(unsigned player, MapPoint pt, unsigned char owner);
    void EventOccupied(unsigned player, MapPoint pt);
    void EventAttack(unsigned char attackerPlayerId, unsigned char defenderPlayerId, unsigned attackerCount);
    void EventStart(bool isFirstStart);
    void EventHumanWinner();
    void EventGameFrame(unsigned nr);
    void EventResourceFound(unsigned char player, MapPoint pt, ResourceType type, unsigned char quantity);
    // Called if player wants to cancel a pact
    bool EventCancelPactRequest(PactType pt, unsigned char canceledByPlayerId, unsigned char targetPlayerId);
    // Called if player suggests a pact
    void EventSuggestPact(PactType pt, unsigned char suggestedByPlayerId, unsigned char targetPlayerId,
                          unsigned duration);
    // called if pact was canceled
    void EventPactCanceled(PactType pt, unsigned char canceledByPlayerId, unsigned char targetPlayerId);
    // called if pact was created
    void EventPactCreated(PactType pt, unsigned char suggestedByPlayerId, unsigned char targetPlayerId,
                          unsigned duration);

    // Callable from Lua
    void ClearResources();
    unsigned GetGF() const;
    std::string FormatNumGFs(unsigned numGFs) const;
    unsigned GetNumPlayers() const;
    void Chat(int playerIdx, const std::string& msg);
    void MissionStatement(int playerIdx, const std::string& title, const std::string& msg);
    void MissionStatement2(int playerIdx, const std::string& title, const std::string& msg, unsigned imgIdx);
    void MissionStatement3(int playerIdx, const std::string& title, const std::string& msg, unsigned imgIdx,
                           bool pause);
    void SetMissionGoal(int playerIdx, const std::string& newGoal = "");
    void PostMessageLua(int playerIdx, const std::string& msg);
    void PostMessageWithLocation(int playerIdx, const std::string& msg, int x, int y);

    void EnableCampaignChapter(const CampaignID& campaignUid, ChapterID chapter);
    void SetCampaignChapterCompleted(const CampaignID& campaignUid, ChapterID chapter);
    void SetCampaignCompleted(const CampaignID& campaignUid);

private:
    ILocalGameState& localGameState;
    GameWorld& gw;
    Game& game;
    LuaPlayer GetPlayer(int playerIdx);
    LuaWorld GetWorld();
};
