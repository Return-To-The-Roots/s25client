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

    void LUA_EventExplored(unsigned player, const MapPoint pt);
    void LUA_EventOccupied(unsigned player, const MapPoint pt);
    void LUA_EventStart();
    void LUA_EventGF(unsigned number);
    void LUA_EventResourceFound(unsigned char player, const MapPoint pt, const unsigned char type, const unsigned char quantity);

private:
    lua_State* lua;

    static int LUA_DisableBuilding(lua_State* L);
    static int LUA_EnableBuilding(lua_State* L);
    static int LUA_SetRestrictedArea(lua_State* L);
    static int LUA_ClearResources(lua_State *L);
    static int LUA_AddWares(lua_State* L);
    static int LUA_AddPeople(lua_State* L);
    static int LUA_GetGF(lua_State *L);
    static int LUA_Log(lua_State *L);
    static int LUA_Chat(lua_State *L);
    static int LUA_MissionStatement(lua_State *L);
    static int LUA_PostMessage(lua_State *L);
    static int LUA_PostMessageWithLocation(lua_State *L);
    static int LUA_GetPlayerCount(lua_State *L);
    static int LUA_GetBuildingCount(lua_State *L);
    static int LUA_GetWareCount(lua_State *L);
    static int LUA_GetPeopleCount(lua_State *L);
    static int LUA_AddEnvObject(lua_State *L);
    static int LUA_AIConstructionOrder(lua_State *L);
    static int LUA_AddStaticObject(lua_State *L);
    static int LUA_PostNewBuildings(lua_State *L);
    static int LUA_ModifyPlayerHQ(lua_State* L);
};

#endif // LuaInterface_h__