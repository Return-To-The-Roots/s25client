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
#include "LuaPlayer.h"
#include "EventManager.h"
#include "GameClientPlayer.h"
#include "world/GameWorldGame.h"
#include "buildings/nobBaseWarehouse.h"
#include "buildings/nobHQ.h"
#include "gameTypes/BuildingCount.h"
#include "postSystem/PostMsgWithBuilding.h"
#include "notifications/BuildingNote.h"
#include "helpers/converters.h"
#include "libutil/src/Log.h"
#include <stdexcept>

inline void check(bool testValue, const std::string& error)
{
    if(!testValue)
        throw std::runtime_error(error);
}

const BasePlayerInfo& LuaPlayer::GetPlayer() const
{
    return player;
}

void LuaPlayer::Register(kaguya::State& state)
{
    LuaPlayerBase::Register(state);
    state["Player"].setClass(kaguya::ClassMetatable<LuaPlayer, LuaPlayerBase>()
        .addMemberFunction("EnableBuilding", &LuaPlayer::EnableBuilding)
        .addMemberFunction("DisableBuilding", &LuaPlayer::DisableBuilding)
        .addMemberFunction("EnableAllBuildings", &LuaPlayer::EnableAllBuildings)
        .addMemberFunction("DisableAllBuildings", &LuaPlayer::DisableAllBuildings)
        .addMemberFunction("SetRestrictedArea", &LuaPlayer::SetRestrictedArea)
        .addMemberFunction("ClearResources", &LuaPlayer::ClearResources)
        .addMemberFunction("AddWares", &LuaPlayer::AddWares)
        .addMemberFunction("AddPeople", &LuaPlayer::AddPeople)
        .addMemberFunction("GetBuildingCount", &LuaPlayer::GetBuildingCount)
        .addMemberFunction("GetWareCount", &LuaPlayer::GetWareCount)
        .addMemberFunction("GetPeopleCount", &LuaPlayer::GetPeopleCount)
        .addMemberFunction("AIConstructionOrder", &LuaPlayer::AIConstructionOrder)
        .addMemberFunction("ModifyHQ", &LuaPlayer::ModifyHQ)
        .addMemberFunction("GetHQPos", &LuaPlayer::GetHQPos)
        );
}

void LuaPlayer::EnableBuilding(BuildingType bld, bool notify)
{
    check(unsigned(bld) < BUILDING_TYPES_COUNT, "Invalid building type");
    player.EnableBuilding(bld);
    if(notify)
    {
        player.SendPostMessage(new PostMsgWithBuilding(
            player.GetGameWorld().GetEvMgr().GetCurrentGF(),
            _(BUILDING_NAMES[bld]),
            PMC_GENERAL,
            bld,
            player.nation,
            player.GetHQPos()));
    }
}

void LuaPlayer::DisableBuilding(BuildingType bld)
{
    check(unsigned(bld) < BUILDING_TYPES_COUNT, "Invalid building type");
    player.EnableBuilding(bld);
}

void LuaPlayer::EnableAllBuildings()
{
    for(unsigned building_type = 0; building_type < BUILDING_TYPES_COUNT; building_type++)
        player.EnableBuilding(BuildingType(building_type));
}

void LuaPlayer::DisableAllBuildings()
{
    for(unsigned building_type = 0; building_type < BUILDING_TYPES_COUNT; building_type++)
        player.DisableBuilding(BuildingType(building_type));
}

void LuaPlayer::SetRestrictedArea(kaguya::VariadicArgType points)
{
    if(points.size() % 2)
        throw std::runtime_error("Invalid number of points given");
    std::vector<MapPoint> pts;
    for(kaguya::VariadicArgType::const_iterator it = points.begin(); it != points.end(); ++it)
    {
        int x = *it;
        ++it;
        int y = *it;
        if(x < 0 || y < 0)
            throw std::runtime_error("Points must be non-negative");
        pts.push_back(MapPoint(x, y));
    }
    player.GetRestrictedArea() = pts;
}

void LuaPlayer::ClearResources()
{
    const std::list<nobBaseWarehouse*> warehouses = player.GetStorehouses();
    for(std::list<nobBaseWarehouse*>::const_iterator wh = warehouses.begin(); wh != warehouses.end(); ++wh)
        (*wh)->Clear();
}

bool LuaPlayer::AddWares(const std::map<GoodType, unsigned>& wares)
{
    nobBaseWarehouse* warehouse = player.GetFirstWH();

    if(!warehouse)
        return false;

    Inventory goods;

    for(std::map<GoodType, unsigned>::const_iterator it = wares.begin(); it != wares.end(); ++it)
    {
        if(unsigned(it->first) < WARE_TYPES_COUNT)
        {
            goods.Add(it->first, it->second);
            player.IncreaseInventoryWare(it->first, it->second);
        } else
            throw std::runtime_error((std::string("Invalid ware in AddWares: ") + helpers::toString(it->first)).c_str());
    }

    warehouse->AddGoods(goods);
    return true;
}

bool LuaPlayer::AddPeople(const std::map<Job, unsigned>& people)
{
    nobBaseWarehouse* warehouse = player.GetFirstWH();

    if(!warehouse)
        return false;

    Inventory goods;

    for(std::map<Job, unsigned>::const_iterator it = people.begin(); it != people.end(); ++it)
    {
        if(unsigned(it->first) < JOB_TYPES_COUNT)
        {
            goods.Add(it->first, it->second);
            player.IncreaseInventoryJob(it->first, it->second);
        } else
            throw std::runtime_error((std::string("Invalid job in AddPeople: ") + helpers::toString(it->first)).c_str());
    }

    warehouse->AddGoods(goods);
    return true;
}

unsigned LuaPlayer::GetBuildingCount(BuildingType bld)
{
    check(unsigned(bld) < BUILDING_TYPES_COUNT, "Invalid building type");

    return player.GetBuildingCount().buildings[bld];
}

unsigned LuaPlayer::GetWareCount(GoodType ware)
{
    check(unsigned(ware) < WARE_TYPES_COUNT, "Invalid ware");
    return player.GetInventory().goods[ware];
}

unsigned LuaPlayer::GetPeopleCount(Job job)
{
    check(unsigned(job) < JOB_TYPES_COUNT, "Invalid ware");
    return player.GetInventory().people[job];
}

bool LuaPlayer::AIConstructionOrder(unsigned x, unsigned y, BuildingType bld)
{
    check(unsigned(bld) < BUILDING_TYPES_COUNT, "Invalid building type");
    check(x < player.GetGameWorld().GetWidth(), "x coordinate to large");
    check(y < player.GetGameWorld().GetHeight(), "y coordinate to large");
    player.GetGameWorld().GetNotifications().publish(BuildingNote(BuildingNote::LuaOrder, player.GetPlayerId(), MapPoint(x, y), bld));
    return true;
}

void LuaPlayer::ModifyHQ(bool isTent)
{
    const MapPoint hqPos = player.GetHQPos();
    if(hqPos.isValid())
    {
        nobHQ* hq = player.GetGameWorld().GetSpecObj<nobHQ>(hqPos);
        if(hq)
            hq->SetIsTent(isTent);
    }
}

kaguya::standard::tuple<unsigned, unsigned> LuaPlayer::GetHQPos()
{
    return kaguya::standard::tuple<unsigned, unsigned>(player.GetHQPos().x, player.GetHQPos().y);
}
