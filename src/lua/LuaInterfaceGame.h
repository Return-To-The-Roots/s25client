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

#ifndef LuaInterfaceGame_h__
#define LuaInterfaceGame_h__

#include "LuaInterfaceBase.h"
#include "gameTypes/MapTypes.h"
#include <string>

class GameWorldGame;
class LuaPlayer;
class LuaWorld;
class Serializer;

class LuaInterfaceGame: public LuaInterfaceBase{
public:

    LuaInterfaceGame(GameWorldGame& gw);
    virtual ~LuaInterfaceGame();

    static void Register(kaguya::State& state);

    Serializer Serialize();
    void Deserialize(Serializer& luaState);

    void EventExplored(unsigned player, const MapPoint pt);
    void EventOccupied(unsigned player, const MapPoint pt);
    void EventStart(bool isFirstStart);
    void EventGameFrame(unsigned number);
    void EventResourceFound(unsigned char player, const MapPoint pt, const unsigned char type, const unsigned char quantity);

private:
    GameWorldGame& gw;

    void ClearResources();
    unsigned GetGF();
    unsigned GetPlayerCount();
    void Chat(int playerIdx, const std::string& msg);
    void MissionStatement(int playerIdx, const std::string& title, const std::string& msg);
    void PostMessageLua(unsigned playerIdx, const std::string& msg);
    void PostMessageWithLocation(unsigned playerIdx, const std::string& msg, int x, int y);
    LuaPlayer GetPlayer(unsigned playerIdx);
    LuaWorld GetWorld();
};

#endif // LuaInterfaceGame_h__
