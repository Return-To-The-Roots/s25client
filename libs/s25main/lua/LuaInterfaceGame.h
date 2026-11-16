// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "LuaInterfaceGameBase.h"
#include "gameTypes/MapCoordinates.h"
#include "gameTypes/PactTypes.h"
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

    static void registerClasses(kaguya::State& state);

    bool serialize(Serializer& luaSaveState);
    bool deserialize(Serializer& luaSaveState);

    void eventExplored(unsigned player, MapPoint pt, unsigned char owner);
    void eventOccupied(unsigned player, MapPoint pt);
    void eventAttack(unsigned char attackerPlayerId, unsigned char defenderPlayerId, unsigned attackerCount);
    void eventStart(bool isFirstStart);
    void eventGameFrame(unsigned nr);
    void eventResourceFound(unsigned char player, MapPoint pt, ResourceType type, unsigned char quantity);
    // Called if player wants to cancel a pact
    bool eventCancelPactRequest(PactType pt, unsigned char canceledByPlayerId, unsigned char targetPlayerId);
    // Called if player suggests a pact
    void eventSuggestPact(PactType pt, unsigned char suggestedByPlayerId, unsigned char targetPlayerId,
                          unsigned duration);
    // called if pact was canceled
    void eventPactCanceled(PactType pt, unsigned char canceledByPlayerId, unsigned char targetPlayerId);
    // called if pact was created
    void eventPactCreated(PactType pt, unsigned char suggestedByPlayerId, unsigned char targetPlayerId,
                          unsigned duration);
    // Callable from Lua
    void clearResources();
    unsigned getGF() const;
    std::string formatNumGFs(unsigned numGFs) const;
    unsigned getNumPlayers() const;
    void chat(int playerIdx, const std::string& msg);
    void missionStatement(int playerIdx, const std::string& title, const std::string& msg);
    void missionStatement2(int playerIdx, const std::string& title, const std::string& msg, unsigned imgIdx);
    void missionStatement3(int playerIdx, const std::string& title, const std::string& msg, unsigned imgIdx,
                           bool pause);
    void setMissionGoal(int playerIdx, const std::string& newGoal = "");
    void postMessageLua(int playerIdx, const std::string& msg);
    void postMessageWithLocation(int playerIdx, const std::string& msg, int x, int y);

private:
    ILocalGameState& localGameState;
    GameWorld& gw;
    Game& game;
    LuaPlayer getPlayer(int playerIdx);
    LuaWorld getWorld();
};
