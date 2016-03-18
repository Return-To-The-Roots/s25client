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

#ifndef LuaInterface_h__
#define LuaInterface_h__

#include "gameTypes/MapTypes.h"
#include <kaguya/kaguya.hpp>
#include <string>

class GameWorldGame;
class LuaPlayer;
class LuaWorld;
class Serializer;

class LuaInterface{
public:

    LuaInterface(GameWorldGame& gw);
    virtual ~LuaInterface();

    static void Register(kaguya::State& state);

    bool LoadScript(const std::string& scriptPath);
    bool LoadScriptString(const std::string& script);
    const std::string& GetScript() const { return script_; }

    Serializer Serialize();
    void Deserialize(Serializer& luaState);

    void EventExplored(unsigned player, const MapPoint pt);
    void EventOccupied(unsigned player, const MapPoint pt);
    void EventStart(bool isFirstStart);
    void EventGameFrame(unsigned number);
    void EventResourceFound(unsigned char player, const MapPoint pt, const unsigned char type, const unsigned char quantity);

private:
    GameWorldGame& gw;
    kaguya::State lua;
    std::string script_;

    void ClearResources();
    unsigned GetGF();
    unsigned GetPlayerCount();
    void Log(const std::string& msg);
    void Chat(int playerIdx, const std::string& msg);
    void MissionStatement(int playerIdx, const std::string& title, const std::string& msg);
    void PostMessageLua(unsigned playerIdx, const std::string& msg);
    void PostMessageWithLocation(unsigned playerIdx, const std::string& msg, int x, int y);
    LuaPlayer GetPlayer(unsigned playerIdx);
    LuaWorld GetWorld();

    static void ErrorHandler(int status, const char* message);
    static void ErrorHandlerThrow(int status, const char* message);
};

#endif // LuaInterface_h__