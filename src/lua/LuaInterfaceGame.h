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

#ifndef LuaInterfaceGame_h__
#define LuaInterfaceGame_h__

#include "LuaInterfaceBase.h"
#include "gameTypes/MapCoordinates.h"
#include "gameTypes/PactTypes.h"
#include <string>

class GameWorldGame;
class LuaPlayer;
class LuaWorld;
class Serializer;

class LuaInterfaceGame : public LuaInterfaceBase
{
public:
    LuaInterfaceGame(GameWorldGame& gw);
    virtual ~LuaInterfaceGame();

    static void Register(kaguya::State& state);

    Serializer Serialize();
    bool Deserialize(Serializer& luaState);

    void EventExplored(unsigned player, const MapPoint pt, unsigned char owner);
    void EventOccupied(unsigned player, const MapPoint pt);
    void EventStart(bool isFirstStart);
    void EventGameFrame(unsigned number);
    void EventResourceFound(unsigned char player, const MapPoint pt, unsigned char type, unsigned char quantity);
    // Called if player wants to cancel a pact
    bool EventCancelPactRequest(PactType pt, unsigned char canceledByPlayerId, unsigned char targetPlayerId);
    // Called if player suggests a pact
    void EventSuggestPact(const PactType pt, unsigned char suggestedByPlayerId, unsigned char targetPlayerId, const unsigned duration);
    // called if pact was canceled
    void EventPactCanceled(const PactType pt, unsigned char canceledByPlayerId, unsigned char targetPlayerId);
    // called if pact was created
    void EventPactCreated(const PactType pt, unsigned char suggestedByPlayerId, unsigned char targetPlayerId, const unsigned duration);
    // Callable from Lua
    void ClearResources();
    unsigned GetGF() const;
    unsigned GetNumPlayers() const;
    void Chat(int playerIdx, const std::string& msg);
    void MissionStatement(int playerIdx, const std::string& title, const std::string& msg);
    void MissionStatement2(int playerIdx, const std::string& title, const std::string& msg, unsigned imgIdx);
    void MissionStatement3(int playerIdx, const std::string& title, const std::string& msg, unsigned imgIdx, bool pause);
    void SetMissionGoal(int playerIdx, const std::string& newGoal = "");
    void PostMessageLua(unsigned playerIdx, const std::string& msg);
    void PostMessageWithLocation(unsigned playerIdx, const std::string& msg, int x, int y);

private:
    GameWorldGame& gw;

    LuaPlayer GetPlayer(unsigned playerIdx);
    LuaWorld GetWorld();
};

#endif // LuaInterfaceGame_h__
