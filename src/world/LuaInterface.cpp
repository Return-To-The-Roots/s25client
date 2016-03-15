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

#include "defines.h" // IWYU pragma: keep
#include "LuaInterface.h"
#include "GameClient.h"
#include "ingameWindows/iwMissionStatement.h"
#include "ai/AIEvents.h"
#include "buildings/nobHQ.h"
#include "nodeObjs/noEnvObject.h"
#include "nodeObjs/noStaticObject.h"
#include "WindowManager.h"
#include "PostMsg.h"
#include "libutil/src/Log.h"
#include "luaIncludes.h"

// Include last!
#include "DebugNew.h" // IWYU pragma: keep

LuaInterface::LuaInterface(): lua(luaL_newstate())
{
    luaopen_base(lua);
    luaopen_package(lua);
    luaopen_string(lua);
    luaopen_table(lua);
    luaopen_math(lua);

    static const luaL_Reg meta[] =
    {
        { "EnableBuilding", LUA_EnableBuilding },
        { "DisableBuilding", LUA_DisableBuilding },
        { "SetRestrictedArea", LUA_SetRestrictedArea },
        { "ClearResources", LUA_ClearResources },
        { "AddWares", LUA_AddWares },
        { "AddPeople", LUA_AddPeople },
        { "GetGF", LUA_GetGF },
        { "GetPlayerCount", LUA_GetPlayerCount },
        { "GetPeopleCount", LUA_GetPeopleCount },
        { "GetWareCount", LUA_GetWareCount },
        { "GetBuildingCount", LUA_GetBuildingCount },
        { "Log", LUA_Log },
        { "Chat", LUA_Chat },
        { "MissionStatement", LUA_MissionStatement },
        { "PostMessage", LUA_PostMessage },
        { "PostMessageWithLocation", LUA_PostMessageWithLocation },
        { "PostNewBuildings", LUA_PostNewBuildings },
        { "AddStaticObject", LUA_AddStaticObject },
        { "AddEnvObject", LUA_AddEnvObject },
        { "AIConstructionOrder", LUA_AIConstructionOrder },
        { "ModifyPlayerHQ", LUA_ModifyPlayerHQ },
        { NULL, NULL }
    };

    luaL_newlibtable(lua, meta);

    lua_setglobal(lua, "rttr");
    lua_getglobal(lua, "rttr");

    lua_pushlightuserdata(lua, this);

    luaL_setfuncs(lua, meta, 1);

#define ADD_LUA_CONST(name) lua_pushnumber(lua, name); lua_setglobal(lua, #name);

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
    ADD_LUA_CONST(JOB_BOATCARRIER);
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
    ADD_LUA_CONST(GD_WATEREMPTY);
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
    ADD_LUA_CONST(GD_SHIELDROMANS);
    ADD_LUA_CONST(GD_WOOD);
    ADD_LUA_CONST(GD_BOARDS);
    ADD_LUA_CONST(GD_STONES);
    ADD_LUA_CONST(GD_SHIELDVIKINGS);
    ADD_LUA_CONST(GD_SHIELDAFRICANS);
    ADD_LUA_CONST(GD_GRAIN);
    ADD_LUA_CONST(GD_COINS);
    ADD_LUA_CONST(GD_GOLD);
    ADD_LUA_CONST(GD_IRONORE);
    ADD_LUA_CONST(GD_COAL);
    ADD_LUA_CONST(GD_MEAT);
    ADD_LUA_CONST(GD_HAM);
    ADD_LUA_CONST(GD_SHIELDJAPANESE);

#undef ADD_LUA_CONST

    lua_settop(lua, 0);
}

LuaInterface::~LuaInterface()
{
    lua_close(lua);
}

bool LuaInterface::LoadScript(const std::string& scriptPath)
{
    if(luaL_dofile(lua, scriptPath.c_str()))
    {
        fprintf(stderr, "LUA ERROR: '%s'!\n", lua_tostring(lua, -1));
        lua_pop(lua, 1);
        return false;
    } else
        return true;
}

int LuaInterface::LUA_ModifyPlayerHQ(lua_State* L)
{
    int argc = lua_gettop(L);

    if(argc != 2)
    {
        lua_pushstring(L, "too few or too many arguments!");
        lua_error(L);
        return(0);
    }

    unsigned playerIdx = (unsigned)luaL_checknumber(L, 1);
    unsigned isTent = (unsigned)luaL_checknumber(L, 2);

    if(playerIdx >= GAMECLIENT.GetPlayerCount())
    {
        lua_pushstring(L, "player number invalid!");
        lua_error(L);
        return(0);
    }

    if(isTent != 0 && isTent != 1)
    {
        lua_pushstring(L, "IsTent must be 0 or 1!");
        lua_error(L);
        return(0);
    }

    const MapPoint hqPos = GAMECLIENT.GetPlayer(playerIdx).hqPos;
    if(hqPos.isValid())
    {
        GameWorldGame *gwg = dynamic_cast<GameWorldGame*>((World*)lua_touserdata(L, lua_upvalueindex(1)));
        nobHQ* hq = gwg->GetSpecObj<nobHQ>(hqPos);
        if(hq)
            hq->SetIsTent(isTent != 0);
    }


    return(0);
}

int LuaInterface::LUA_EnableBuilding(lua_State* L)
{
    //  GameWorldBase *gw = static_cast<GameWorldBase*>(lua_touserdata(L, lua_upvalueindex(1)));
    int argc = lua_gettop(L);

    if(argc < 1)
    {
        lua_pushstring(L, "too few or too many arguments!");
        lua_error(L);
        return(0);
    }

    // player
    unsigned pnr = (unsigned)luaL_checknumber(L, 1);

    if(pnr >= GAMECLIENT.GetPlayerCount())
    {
        lua_pushstring(L, "player number invalid!");
        lua_error(L);
        return(0);
    }

    GameClientPlayer& player = GAMECLIENT.GetPlayer(pnr);

    if(argc == 1)
    {
        for(unsigned building_type = 0; building_type < BUILDING_TYPES_COUNT; building_type++)
        {
            player.EnableBuilding(BuildingType(building_type));
        }

        return(0);
    }

    int cnt = 2;
    while(cnt <= argc)
    {
        // building type
        unsigned building_type = (unsigned)luaL_checknumber(L, cnt++);

        if(building_type < BUILDING_TYPES_COUNT)
        {
            player.EnableBuilding(BuildingType(building_type));
        } else
        {
            lua_pushstring(L, "building type invalid!");
            lua_error(L);
        }
    }

    return(0);
}

int LuaInterface::LUA_DisableBuilding(lua_State* L)
{
    //  GameWorldBase *gw = static_cast<GameWorldBase*>(lua_touserdata(L, lua_upvalueindex(1)));
    int argc = lua_gettop(L);

    if(argc < 1)
    {
        lua_pushstring(L, "too few or too many arguments!");
        lua_error(L);
        return(0);
    }

    // player
    unsigned pnr = (unsigned)luaL_checknumber(L, 1);

    if(pnr >= GAMECLIENT.GetPlayerCount())
    {
        lua_pushstring(L, "player number invalid!");
        lua_error(L);
        return(0);
    }

    GameClientPlayer& player = GAMECLIENT.GetPlayer(pnr);

    if(argc == 1)
    {
        for(unsigned building_type = 0; building_type < BUILDING_TYPES_COUNT; building_type++)
        {
            player.DisableBuilding(BuildingType(building_type));
        }

        return(0);
    }

    int cnt = 2;
    while(cnt <= argc)
    {
        // building type
        unsigned building_type = (unsigned)luaL_checknumber(L, cnt++);

        if(building_type < BUILDING_TYPES_COUNT)
        {
            player.DisableBuilding(BuildingType(building_type));
        } else
        {
            lua_pushstring(L, "building type invalid!");
            lua_error(L);
        }
    }

    return(0);
}


int LuaInterface::LUA_SetRestrictedArea(lua_State* L)
{
    //  GameWorldBase *gw = static_cast<GameWorldBase*>(lua_touserdata(L, lua_upvalueindex(1)));
    int argc = lua_gettop(L) - 1;

    if((argc < 0) || (argc % 2 == 1))
    {
        lua_pushstring(L, "wrong arguments: player, x1, y1, x2, y2, ...");
        lua_error(L);
        return(0);
    }

    // player
    unsigned pnr = (unsigned)luaL_checknumber(L, 1);

    if(pnr >= GAMECLIENT.GetPlayerCount())
    {
        lua_pushstring(L, "player number invalid!");
        lua_error(L);
        return(0);
    }

    GameClientPlayer& player = GAMECLIENT.GetPlayer(pnr);

    std::vector< MapPoint > &restricted_area = player.GetRestrictedArea();

    restricted_area.clear();

    unsigned cnt = 2;
    for(argc >>= 1; argc > 0; --argc)
    {
        MapCoord x = (MapCoord)luaL_checknumber(L, cnt++);
        MapCoord y = (MapCoord)luaL_checknumber(L, cnt++);
        //        fprintf(stderr, "RESTRICTED AREA - %u, %u\n", x, y);

        restricted_area.push_back(MapPoint(x, y));
    }

    return(0);
}

int LuaInterface::LUA_ClearResources(lua_State *L)
{
    if(lua_gettop(L) > 0)
    {
        unsigned p = (unsigned)luaL_checknumber(L, 1);

        if(p >= GAMECLIENT.GetPlayerCount())
        {
            lua_pushstring(L, "player number invalid!");
            lua_error(L);
            return(0);
        }

        const std::list<nobBaseWarehouse*> warehouses = GAMECLIENT.GetPlayer(p).GetStorehouses();

        for(std::list<nobBaseWarehouse*>::const_iterator wh = warehouses.begin(); wh != warehouses.end(); ++wh)
        {
            (*wh)->Clear();
        }
    } else
    {
        for(unsigned p = 0; p < GAMECLIENT.GetPlayerCount(); p++)
        {
            const std::list<nobBaseWarehouse*> warehouses = GAMECLIENT.GetPlayer(p).GetStorehouses();

            for(std::list<nobBaseWarehouse*>::const_iterator wh = warehouses.begin(); wh != warehouses.end(); ++wh)
            {
                (*wh)->Clear();
            }
        }
    }

    return(0);
}

int LuaInterface::LUA_AddWares(lua_State* L)
{
    //  GameWorldBase *gw = static_cast<GameWorldBase*>(lua_touserdata(L, lua_upvalueindex(1)));
    int argc = lua_gettop(L) - 1;

    if((argc < 0) || (argc % 2 == 1))
    {
        lua_pushstring(L, "wrong arguments: player, ware1, count1, ware2, count2, ...");
        lua_error(L);
        return(0);
    }

    // player
    unsigned pnr = (unsigned)luaL_checknumber(L, 1);

    if(pnr >= GAMECLIENT.GetPlayerCount())
    {
        lua_pushstring(L, "player number invalid!");
        lua_error(L);
        return(0);
    }

    GameClientPlayer& player = GAMECLIENT.GetPlayer(pnr);

    nobBaseWarehouse* warehouse = player.GetFirstWH();

    if(!warehouse)
    {
        lua_pushnumber(L, 0);
        return(1);
    }

    Inventory goods;

    unsigned cnt = 2;
    for(argc >>= 1; argc > 0; --argc)
    {
        unsigned type = (unsigned)luaL_checknumber(L, cnt++);
        unsigned count = (unsigned)luaL_checknumber(L, cnt++);

        if(type < WARE_TYPES_COUNT)
        {
            goods.Add(GoodType(type), count);
            player.IncreaseInventoryWare(GoodType(type), count);
        }
    }

    warehouse->AddGoods(goods);

    lua_pushnumber(L, 1);
    return(1);
}

int LuaInterface::LUA_AddPeople(lua_State* L)
{
    //  GameWorldBase *gw = static_cast<GameWorldBase*>(lua_touserdata(L, lua_upvalueindex(1)));
    int argc = lua_gettop(L) - 1;

    if((argc < 0) || (argc % 2 == 1))
    {
        lua_pushstring(L, "wrong arguments: player, ware1, count1, ware2, count2, ...");
        lua_error(L);
        return(0);
    }

    // player
    unsigned pnr = (unsigned)luaL_checknumber(L, 1);

    if(pnr >= GAMECLIENT.GetPlayerCount())
    {
        lua_pushstring(L, "player number invalid!");
        lua_error(L);
        return(0);
    }

    GameClientPlayer& player = GAMECLIENT.GetPlayer(pnr);

    nobBaseWarehouse* warehouse = player.GetFirstWH();

    if(!warehouse)
    {
        lua_pushnumber(L, 0);
        return(1);
    }

    Inventory goods;

    unsigned cnt = 2;
    for(argc >>= 1; argc > 0; --argc)
    {
        unsigned type = (unsigned)luaL_checknumber(L, cnt++);
        unsigned count = (unsigned)luaL_checknumber(L, cnt++);

        if(type < JOB_TYPES_COUNT)
        {
            goods.Add(Job(type), count);
            player.IncreaseInventoryJob(Job(type), count);
        }
    }

    warehouse->AddGoods(goods);

    lua_pushnumber(L, 1);

    return(1);
}

int LuaInterface::LUA_GetGF(lua_State *L)
{
    lua_pushnumber(L, GAMECLIENT.GetGFNumber());
    return(1);
}

int LuaInterface::LUA_GetPlayerCount(lua_State *L)
{
    lua_pushnumber(L, GAMECLIENT.GetPlayerCount());
    return(1);
}

int LuaInterface::LUA_GetBuildingCount(lua_State *L)
{
    if(lua_gettop(L) < 2)
    {
        lua_pushstring(L, "need player number and building type!");
        lua_error(L);
        return(0);
    }

    unsigned pnr = (unsigned)luaL_checknumber(L, 1);

    if(pnr >= GAMECLIENT.GetPlayerCount())
    {
        lua_pushstring(L, "player number invalid!");
        lua_error(L);
        return(0);
    }

    unsigned building_type = (unsigned)luaL_checknumber(L, 2);

    if(building_type >= BUILDING_TYPES_COUNT)
    {
        lua_pushstring(L, "invalid building type!");
        lua_error(L);
        return(0);
    }

    BuildingCount bc;

    GAMECLIENT.GetPlayer(pnr).GetBuildingCount(bc);

    lua_pushnumber(L, bc.building_counts[building_type]);

    return(1);
}

int LuaInterface::LUA_GetWareCount(lua_State *L)
{
    if(lua_gettop(L) < 2)
    {
        lua_pushstring(L, "need player number and ware type!");
        lua_error(L);
        return(0);
    }

    unsigned pnr = (unsigned)luaL_checknumber(L, 1);

    if(pnr >= GAMECLIENT.GetPlayerCount())
    {
        lua_pushstring(L, "player number invalid!");
        lua_error(L);
        return(0);
    }

    unsigned type = (unsigned)luaL_checknumber(L, 2);

    if(type >= WARE_TYPES_COUNT)
    {
        lua_pushstring(L, "invalid ware type!");
        lua_error(L);
        return(0);
    }

    const Inventory& goods = GAMECLIENT.GetPlayer(pnr).GetInventory();

    lua_pushnumber(L, goods.goods[type]);

    return(1);
}

int LuaInterface::LUA_GetPeopleCount(lua_State *L)
{
    if(lua_gettop(L) < 2)
    {
        lua_pushstring(L, "need player number and job type!");
        lua_error(L);
        return(0);
    }

    unsigned pnr = (unsigned)luaL_checknumber(L, 1);

    if(pnr >= GAMECLIENT.GetPlayerCount())
    {
        lua_pushstring(L, "player number invalid!");
        lua_error(L);
        return(0);
    }

    unsigned type = (unsigned)luaL_checknumber(L, 2);

    if(type >= JOB_TYPES_COUNT)
    {
        lua_pushstring(L, "invalid job type!");
        lua_error(L);
        return(0);
    }

    const Inventory& goods = GAMECLIENT.GetPlayer(pnr).GetInventory();

    lua_pushnumber(L, goods.people[type]);

    return(1);
}

int LuaInterface::LUA_Log(lua_State *L)
{
    int argc = lua_gettop(L);

    std::string message;

    for(int n = 1; n <= argc; n++)
    {
        message.append(luaL_checklstring(L, n, NULL));
    }

    LOG.lprintf("%s\n", message.c_str());

    return(0);
}

int LuaInterface::LUA_Chat(lua_State *L)
{
    int argc = lua_gettop(L);

    if(argc < 2)
    {
        lua_pushstring(L, "Too few arguments!");
        lua_error(L);
        return(0);
    }

    unsigned player = (unsigned)luaL_checknumber(L, 1);

    if((player != 0xFFFFFFFF) && (unsigned)GAMECLIENT.GetPlayerID() != player)
    {
        return(0);
    }

    std::string message;

    for(int n = 2; n <= argc; n++)
    {
        message.append(luaL_checklstring(L, n, NULL));
    }

    GAMECLIENT.SystemChat(message);

    return(0);
}

int LuaInterface::LUA_MissionStatement(lua_State *L)
{
    int argc = lua_gettop(L);

    if(argc < 3)
    {
        lua_pushstring(L, "Too few arguments!");
        lua_error(L);
        return(0);
    }

    unsigned player = (unsigned)luaL_checknumber(L, 1);

    if((player != 0xFFFFFFFF) && (unsigned)GAMECLIENT.GetPlayerID() != player)
    {
        return(0);
    }

    std::string message;

    for(int n = 3; n <= argc; n++)
    {
        message.append(luaL_checklstring(L, n, NULL));
    }

    WINDOWMANAGER.Show(new iwMissionStatement(luaL_checklstring(L, 2, NULL), message));

    return(0);
}

int LuaInterface::LUA_PostMessage(lua_State *L)
{
    int argc = lua_gettop(L);

    if(argc < 2)
    {
        lua_pushstring(L, "Too few arguments!");
        lua_error(L);
        return(0);
    }

    if((unsigned)GAMECLIENT.GetPlayerID() != (unsigned)luaL_checknumber(L, 1))
    {
        return(0);
    }

    std::string message;

    for(int n = 2; n <= argc; n++)
    {
        message.append(luaL_checklstring(L, n, NULL));
    }

    GAMECLIENT.SendPostMessage(new PostMsg(message, PMC_OTHER));

    return(0);
}

int LuaInterface::LUA_PostMessageWithLocation(lua_State *L)
{
    int argc = lua_gettop(L);

    if(argc < 4)
    {
        lua_pushstring(L, "Too few arguments!");
        lua_error(L);
        return(0);
    }

    if((unsigned)GAMECLIENT.GetPlayerID() != (unsigned)luaL_checknumber(L, 1))
    {
        return(0);
    }

    MapCoord x = (MapCoord)luaL_checknumber(L, 2);
    MapCoord y = (MapCoord)luaL_checknumber(L, 3);

    std::string message;

    for(int n = 4; n <= argc; n++)
    {
        message.append(luaL_checklstring(L, n, NULL));
    }

    GAMECLIENT.SendPostMessage(new PostMsgWithLocation(message, PMC_OTHER, MapPoint(x, y)));

    return(0);
}

int LuaInterface::LUA_PostNewBuildings(lua_State *L)
{
    int argc = lua_gettop(L);

    if(argc < 2)
    {
        lua_pushstring(L, "Too few arguments!");
        lua_error(L);
        return(0);
    }

    if((unsigned)GAMECLIENT.GetPlayerID() != (unsigned)luaL_checknumber(L, 1))
    {
        return(0);
    }

    unsigned pnr = (unsigned)luaL_checknumber(L, 1);

    for(int n = 2; n <= argc; n++)
    {
        unsigned building_type = (unsigned)luaL_checknumber(L, n);

        if(building_type < BUILDING_TYPES_COUNT)
        {
            GAMECLIENT.SendPostMessage(new ImagePostMsgWithLocation(_(BUILDING_NAMES[building_type]), PMC_GENERAL, GAMECLIENT.GetPlayer(pnr).hqPos, (BuildingType)building_type, (Nation)GAMECLIENT.GetPlayer(pnr).nation));
        }
    }

    return(0);
}

int LuaInterface::LUA_AddStaticObject(lua_State *L)
{
    GameWorldGame *gwg = dynamic_cast<GameWorldGame*>((GameWorldBase*)lua_touserdata(L, lua_upvalueindex(1)));

    if(!gwg)
    {
        return(0);
    }

    int argc = lua_gettop(L);

    if(argc < 3)
    {
        lua_pushstring(L, "Too few arguments!");
        lua_error(L);
        return(0);
    }

    MapCoord x = (MapCoord)luaL_checknumber(L, 1);
    MapCoord y = (MapCoord)luaL_checknumber(L, 2);
    unsigned id = (unsigned)luaL_checknumber(L, 3);
    MapPoint pt(x, y);

    unsigned file = 0xFFFF;
    unsigned size = 0;

    if(argc > 3)
    {
        file = (unsigned)luaL_checknumber(L, 4);

        if(argc > 4)
        {
            size = (unsigned)luaL_checknumber(L, 5);

            if(size > 2)
            {
                lua_pushstring(L, "Invalid size!");
                lua_error(L);
                return(0);
            }
        }
    }

    const MapNode& node = gwg->GetNode(pt);
    if(node.obj && (node.obj->GetGOT() != GOT_NOTHING) && (node.obj->GetGOT() != GOT_STATICOBJECT) && (node.obj->GetGOT() != GOT_ENVOBJECT))
    {
        lua_pushnumber(L, 0);
        return(1);
    }

    gwg->DestroyNO(pt, false);
    gwg->SetNO(pt, new noStaticObject(pt, id, file, size));
    gwg->RecalcBQAroundPoint(pt);

    lua_pushnumber(L, 1);
    return(1);
}

int LuaInterface::LUA_AddEnvObject(lua_State *L)
{
    GameWorldGame *gwg = dynamic_cast<GameWorldGame*>((GameWorldBase*)lua_touserdata(L, lua_upvalueindex(1)));

    if(!gwg)
    {
        return(0);
    }

    int argc = lua_gettop(L);

    if(argc < 3)
    {
        lua_pushstring(L, "Too few arguments!");
        lua_error(L);
        return(0);
    }

    MapCoord x = (MapCoord)luaL_checknumber(L, 1);
    MapCoord y = (MapCoord)luaL_checknumber(L, 2);
    unsigned id = (unsigned)luaL_checknumber(L, 3);
    MapPoint pt(x, y);

    unsigned file = 0xFFFF;

    if(argc > 3)
    {
        file = (unsigned)luaL_checknumber(L, 4);
    }

    const MapNode& node = gwg->GetNode(pt);
    if(node.obj && (node.obj->GetGOT() != GOT_NOTHING) && (node.obj->GetGOT() != GOT_STATICOBJECT) && (node.obj->GetGOT() != GOT_ENVOBJECT))
    {
        lua_pushnumber(L, 0);
        return(1);
    }

    gwg->DestroyNO(pt, false);
    gwg->SetNO(pt, new noEnvObject(pt, id, file));
    gwg->RecalcBQAroundPoint(pt);

    lua_pushnumber(L, 1);
    return(1);
}

int LuaInterface::LUA_AIConstructionOrder(lua_State *L)
{
    GameWorldGame *gwg = dynamic_cast<GameWorldGame*>((GameWorldBase*)lua_touserdata(L, lua_upvalueindex(1)));

    if(!gwg)
    {
        return(0);
    }

    int argc = lua_gettop(L);

    if(argc < 4)//player, x, y, buildingtype
    {
        lua_pushstring(L, "Too few arguments!");
        lua_error(L);
        return(0);
    }

    unsigned pn = (unsigned)luaL_checknumber(L, 1);
    MapCoord x = (MapCoord)luaL_checknumber(L, 2);
    MapCoord y = (MapCoord)luaL_checknumber(L, 3);
    unsigned id = (unsigned)luaL_checknumber(L, 4);
    BuildingType bt = static_cast<BuildingType>(id);

    if(!GAMECLIENT.SendAIEvent(new AIEvent::Building(AIEvent::LuaConstructionOrder, MapPoint(x, y), bt), pn))
        LOG.lprintf("Sending AIConstructionOrder to player %u failed", pn);

    lua_pushnumber(L, 1);
    return(1);
}

void LuaInterface::LUA_EventExplored(unsigned player, const MapPoint pt)
{
    lua_getglobal(lua, "onExplored");

    if(lua_isfunction(lua, -1))
    {
        lua_pushnumber(lua, player);
        lua_pushnumber(lua, pt.x);
        lua_pushnumber(lua, pt.y);

        // 3 arguments, 0 return values, no error handler
        if(lua_pcall(lua, 3, 0, 0))
        {
            fprintf(stderr, "ERROR: '%s'!\n", lua_tostring(lua, -1));
            lua_pop(lua, 1);
        }
    } else
    {
        lua_pop(lua, 1);
    }
}

void LuaInterface::LUA_EventOccupied(unsigned player, const MapPoint pt)
{
    lua_getglobal(lua, "onOccupied");

    if(lua_isfunction(lua, -1))
    {
        lua_pushnumber(lua, player);
        lua_pushnumber(lua, pt.x);
        lua_pushnumber(lua, pt.y);

        // 3 arguments, 0 return values, no error handler
        if(lua_pcall(lua, 3, 0, 0))
        {
            fprintf(stderr, "ERROR: '%s'!\n", lua_tostring(lua, -1));
            lua_pop(lua, 1);
        }
    } else
    {
        lua_pop(lua, 1);
    }
}

void LuaInterface::LUA_EventStart()
{
    lua_getglobal(lua, "onStart");

    if(lua_isfunction(lua, -1))
    {
        // 0 arguments, 0 return values, no error handler
        if(lua_pcall(lua, 0, 0, 0))
        {
            fprintf(stderr, "ERROR: '%s'!\n", lua_tostring(lua, -1));
            lua_pop(lua, 1);
        }
    } else
    {
        lua_pop(lua, 1);
    }
}

void LuaInterface::LUA_EventGF(unsigned nr)
{
    lua_getglobal(lua, "onGameFrame");

    if(lua_isfunction(lua, -1))
    {
        lua_pushnumber(lua, nr);

        // 1 argument, 0 return values, no error handler
        if(lua_pcall(lua, 1, 0, 0))
        {
            fprintf(stderr, "ERROR: '%s'!\n", lua_tostring(lua, -1));
            lua_pop(lua, 1);
        }
    } else
    {
        lua_pop(lua, 1);
    }
}

void LuaInterface::LUA_EventResourceFound(const unsigned char player, const MapPoint pt, const unsigned char type, const unsigned char quantity)
{
    lua_getglobal(lua, "onResourceFound");

    if(lua_isfunction(lua, -1))
    {
        lua_pushnumber(lua, player);
        lua_pushnumber(lua, pt.x);
        lua_pushnumber(lua, pt.y);
        lua_pushnumber(lua, type);
        lua_pushnumber(lua, quantity);

        // 5 arguments, 0 return values, no error handler
        if(lua_pcall(lua, 5, 0, 0))
        {
            fprintf(stderr, "ERROR: '%s'!\n", lua_tostring(lua, -1));
            lua_pop(lua, 1);
        }
    } else
    {
        lua_pop(lua, 1);
    }
}
