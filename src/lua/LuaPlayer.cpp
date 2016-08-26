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
#include "GamePlayer.h"
#include "world/GameWorldGame.h"
#include "buildings/nobBaseWarehouse.h"
#include "buildings/nobHQ.h"
#include "gameTypes/BuildingCount.h"
#include "postSystem/PostMsgWithBuilding.h"
#include "notifications/BuildingNote.h"
#include "lua/LuaHelpers.h"
#include "helpers/converters.h"
#include "libutil/src/Log.h"
#include <stdexcept>

const BasePlayerInfo& LuaPlayer::GetPlayer() const
{
    return player;
}

void LuaPlayer::Register(kaguya::State& state)
{
    LuaPlayerBase::Register(state);
    state["Player"].setClass(kaguya::UserdataMetatable<LuaPlayer, LuaPlayerBase>()
        .addFunction("EnableBuilding", &LuaPlayer::EnableBuilding)
        .addFunction("DisableBuilding", &LuaPlayer::DisableBuilding)
        .addFunction("EnableAllBuildings", &LuaPlayer::EnableAllBuildings)
        .addFunction("DisableAllBuildings", &LuaPlayer::DisableAllBuildings)
        .addFunction("SetRestrictedArea", &LuaPlayer::SetRestrictedArea)
        .addFunction("ClearResources", &LuaPlayer::ClearResources)
        .addFunction("AddWares", &LuaPlayer::AddWares)
        .addFunction("AddPeople", &LuaPlayer::AddPeople)
        .addFunction("GetBuildingCount", &LuaPlayer::GetBuildingCount)
        .addFunction("GetBuildingSitesCount", &LuaPlayer::GetBuildingSitesCount)
        .addFunction("GetWareCount", &LuaPlayer::GetWareCount)
        .addFunction("GetPeopleCount", &LuaPlayer::GetPeopleCount)
        .addFunction("AIConstructionOrder", &LuaPlayer::AIConstructionOrder)
        .addFunction("ModifyHQ", &LuaPlayer::ModifyHQ)
        .addFunction("GetHQPos", &LuaPlayer::GetHQPos)
        .addFunction("Surrender", &LuaPlayer::Surrender)
    );
}

void LuaPlayer::EnableBuilding(BuildingType bld, bool notify)
{
    lua::assertTrue(unsigned(bld) < BUILDING_TYPES_COUNT, "Invalid building type");
    player.EnableBuilding(bld);
    if(notify)
    {
        player.SendPostMessage(new PostMsgWithBuilding(
            player.GetGameWorld().GetEvMgr().GetCurrentGF(),
            std::string(_("New building type:")) + "\n" + _(BUILDING_NAMES[bld]),
            PostCategory::General,
            bld,
            player.nation));
    }
}

void LuaPlayer::DisableBuilding(BuildingType bld)
{
    lua::assertTrue(unsigned(bld) < BUILDING_TYPES_COUNT, "Invalid building type");
    player.DisableBuilding(bld);
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

void LuaPlayer::SetRestrictedArea(kaguya::VariadicArgType inPoints)
{
    if(inPoints.size() == 0)
    {
        // Skip everything else if we only want to lift the restrictions
        player.GetRestrictedArea().clear();
        return;
    }

    std::vector<MapPoint> pts;
    // Index where the current polygon started
    int curPolyStart = -1;
    // Detect old-style separators for multi-polygon detection
    int lastNullPt = -1;
    // Do we have multiple polygons?
    bool isMultiPoly = false;
    for(kaguya::VariadicArgType::const_iterator it = inPoints.begin(); it != inPoints.end(); ++it)
    {
        // Is this the separator between polygons?
        if(it->isNilref())
        {
            if(pts.empty()) // Start separator (old style)
                LOG.write("You don't need leading nils for SetRestrictedArea");
            else if(curPolyStart < 0) // We don't have a current polygon? Can only happen for multiple nils (old style)
                LOG.write("Duplicate nils found in SetRestrictedArea");
            else if(pts.size() - static_cast<unsigned>(curPolyStart) < 3)
                throw std::runtime_error(std::string("Invalid polygon (less than 3 points) found at index ") + helpers::toString(std::distance(inPoints.cbegin(), it)));
            else if(pts[curPolyStart] != pts.back()) // Close polygon if not already done
                pts.push_back(pts[curPolyStart]);
            curPolyStart = -1;
        }else
        {
            // Do we start a new polygon?
            if(curPolyStart < 0)
            {
                if(!pts.empty())
                {
                    // We have multiple polygons -> Add separator
                    isMultiPoly = true;
                    if(pts.back() != MapPoint(0, 0))
                        pts.push_back(MapPoint(0, 0));
                }
                curPolyStart = pts.size();
            }
            int x = *it;
            ++it;
            int y = *it;
            if(x < 0 || y < 0)
                throw std::runtime_error("Points must be non-negative");
            MapPoint pt(x, y);
            if(pt == MapPoint(0, 0))
            {
                // This might be the (old) separator if: We have a previous 0,0-pair, a valid polygon (>= 3 points) and first pt after 0,0 matches last pt
                if(lastNullPt >= 0 && pts.size() - lastNullPt >= 3 && pts[lastNullPt + 1] == pts.back())
                    isMultiPoly = true;
                lastNullPt = pts.size();
            }
            pts.push_back(pt);
        }
    }
    if(isMultiPoly)
    {
        if(curPolyStart >= 0 && pts[curPolyStart] != pts.back()) // Close polygon if not already done
            pts.push_back(pts[curPolyStart]);
        if(pts.front() != MapPoint(0, 0))
            pts.insert(pts.begin(), MapPoint(0, 0));
        if(pts.back() != MapPoint(0, 0))
            pts.push_back(MapPoint(0, 0));
    } else if(pts.front() == pts.back())
        pts.pop_back();
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
            goods.Add(it->first, it->second);
        else
            throw std::runtime_error((std::string("Invalid ware in AddWares: ") + helpers::toString(it->first)).c_str());
    }

    warehouse->AddGoods(goods, true);
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
            goods.Add(it->first, it->second);
        else
            throw std::runtime_error((std::string("Invalid job in AddPeople: ") + helpers::toString(it->first)).c_str());
    }

    warehouse->AddGoods(goods, true);
    return true;
}

unsigned LuaPlayer::GetBuildingCount(BuildingType bld)
{
    lua::assertTrue(unsigned(bld) < BUILDING_TYPES_COUNT, "Invalid building type");

    return player.GetBuildingCount().buildings[bld];
}

unsigned LuaPlayer::GetBuildingSitesCount(BuildingType bld)
{
    lua::assertTrue(unsigned(bld) < BUILDING_TYPES_COUNT, "Invalid building type");

    return player.GetBuildingCount().buildingSites[bld];
}

unsigned LuaPlayer::GetWareCount(GoodType ware)
{
    lua::assertTrue(unsigned(ware) < WARE_TYPES_COUNT, "Invalid ware");
    return player.GetInventory().goods[ware];
}

unsigned LuaPlayer::GetPeopleCount(Job job)
{
    lua::assertTrue(unsigned(job) < JOB_TYPES_COUNT, "Invalid ware");
    return player.GetInventory().people[job];
}

bool LuaPlayer::AIConstructionOrder(unsigned x, unsigned y, BuildingType bld)
{
    // Only for actual AIs
    if(!player.isUsed() || player.isHuman())
        return false;
    lua::assertTrue(unsigned(bld) < BUILDING_TYPES_COUNT, "Invalid building type");
    GameWorldGame& world = player.GetGameWorld();
    lua::assertTrue(x < world.GetWidth(), "x coordinate to large");
    lua::assertTrue(y < world.GetHeight(), "y coordinate to large");
    world.GetNotifications().publish(BuildingNote(BuildingNote::LuaOrder, player.GetPlayerId(), MapPoint(x, y), bld));
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

void LuaPlayer::Surrender(bool destroyBlds)
{
    player.Surrender();
    if(destroyBlds)
        player.GetGameWorld().Armageddon(player.GetPlayerId());
}

kaguya::standard::tuple<unsigned, unsigned> LuaPlayer::GetHQPos()
{
    return kaguya::standard::tuple<unsigned, unsigned>(player.GetHQPos().x, player.GetHQPos().y);
}
