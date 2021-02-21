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

#pragma once

#include "LuaInterfaceGameBase.h"
#include "gameTypes/MapCoordinates.h"
#include "gameTypes/PactTypes.h"
#include <memory>
#include <string>

class GameWorldGame;
class LuaPlayer;
class LuaWorld;
class Serializer;
class Game;
enum class ResourceType : uint8_t;

class LuaInterfaceGame : public LuaInterfaceGameBase
{
public:
    LuaInterfaceGame(std::weak_ptr<Game> gameInstance, ILocalGameState& localGameState);
    virtual ~LuaInterfaceGame();

    static void Register(kaguya::State& state);

    bool Serialize(Serializer& luaSaveState);
    bool Deserialize(Serializer& luaSaveState);

    void EventExplored(unsigned player, MapPoint pt, unsigned char owner);
    void EventOccupied(unsigned player, MapPoint pt);
    void EventStart(bool isFirstStart);
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

private:
    ILocalGameState& localGameState;
    GameWorldGame& gw;
    std::weak_ptr<Game> game;
    LuaPlayer GetPlayer(int playerIdx);
    LuaWorld GetWorld();
};
