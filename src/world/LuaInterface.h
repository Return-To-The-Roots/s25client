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
#include <string>
struct lua_State;

class LuaInterface{
public:

    LuaInterface();
    virtual ~LuaInterface();

    bool LoadScript(const std::string& scriptPath);

    void EventExplored(unsigned player, const MapPoint pt);
    void EventOccupied(unsigned player, const MapPoint pt);
    void EventStart();
    void EventGF(unsigned number);
    void EventResourceFound(unsigned char player, const MapPoint pt, const unsigned char type, const unsigned char quantity);

private:
    lua_State* lua;

    static int DisableBuilding(lua_State* L);
    static int EnableBuilding(lua_State* L);
    static int SetRestrictedArea(lua_State* L);
    static int ClearResources(lua_State *L);
    static int AddWares(lua_State* L);
    static int AddPeople(lua_State* L);
    static int GetGF(lua_State *L);
    static int Log(lua_State *L);
    static int Chat(lua_State *L);
    static int MissionStatement(lua_State *L);
    static int PostMessageLua(lua_State *L);
    static int PostMessageWithLocation(lua_State *L);
    static int GetPlayerCount(lua_State *L);
    static int GetBuildingCount(lua_State *L);
    static int GetWareCount(lua_State *L);
    static int GetPeopleCount(lua_State *L);
    static int AddEnvObject(lua_State *L);
    static int AIConstructionOrder(lua_State *L);
    static int AddStaticObject(lua_State *L);
    static int PostNewBuildings(lua_State *L);
    static int ModifyPlayerHQ(lua_State* L);
};

#endif // LuaInterface_h__