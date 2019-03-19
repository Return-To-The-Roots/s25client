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

#include "rttrDefines.h" // IWYU pragma: keep
#include "LuaPlayer.h"
#include "EventManager.h"
#include "Game.h"
#include "GamePlayer.h"
#include "ai/AIPlayer.h"
#include "buildings/nobBaseWarehouse.h"
#include "buildings/nobHQ.h"
#include "helpers/strUtils.h"
#include "lua/LuaHelpers.h"
#include "lua/LuaInterfaceBase.h"
#include "notifications/BuildingNote.h"
#include "postSystem/PostMsgWithBuilding.h"
#include "world/GameWorldGame.h"
#include "world/TerritoryRegion.h"
#include "gameTypes/BuildingCount.h"
#include "gameData/BuildingConsts.h"
#include "libutil/Log.h"
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
                               .addFunction("IsInRestrictedArea", &LuaPlayer::IsInRestrictedArea)
                               .addFunction("ClearResources", &LuaPlayer::ClearResources)
                               .addFunction("AddWares", &LuaPlayer::AddWares)
                               .addFunction("AddPeople", &LuaPlayer::AddPeople)
                               .addFunction("GetNumBuildings", &LuaPlayer::GetNumBuildings)
                               .addFunction("GetNumBuildingSites", &LuaPlayer::GetNumBuildingSites)
                               .addFunction("GetNumWares", &LuaPlayer::GetNumWares)
                               .addFunction("GetNumPeople", &LuaPlayer::GetNumPeople)
                               .addFunction("AIConstructionOrder", &LuaPlayer::AIConstructionOrder)
                               .addFunction("ModifyHQ", &LuaPlayer::ModifyHQ)
                               .addFunction("GetHQPos", &LuaPlayer::GetHQPos)
                               .addFunction("IsDefeated", &LuaPlayer::IsDefeated)
                               .addFunction("Surrender", &LuaPlayer::Surrender)
                               .addFunction("IsAlly", &LuaPlayer::IsAlly)
                               .addFunction("IsAttackable", &LuaPlayer::IsAttackable)
                               .addFunction("SuggestPact", &LuaPlayer::SuggestPact)
                               .addFunction("CancelPact", &LuaPlayer::CancelPact)
                               // Old names
                               .addFunction("GetBuildingCount", &LuaPlayer::GetNumBuildings)
                               .addFunction("GetBuildingSitesCount", &LuaPlayer::GetNumBuildingSites)
                               .addFunction("GetWareCount", &LuaPlayer::GetNumWares)
                               .addFunction("GetPeopleCount", &LuaPlayer::GetNumPeople));
}

void LuaPlayer::EnableBuilding(BuildingType bld, bool notify)
{
    lua::assertTrue(unsigned(bld) < NUM_BUILDING_TYPES, "Invalid building type");
    player.EnableBuilding(bld);
    if(notify)
    {
        player.SendPostMessage(new PostMsgWithBuilding(player.GetGameWorld().GetEvMgr().GetCurrentGF(),
                                                       std::string(_("New building type:")) + "\n" + _(BUILDING_NAMES[bld]),
                                                       PostCategory::General, bld, player.nation));
    }
}

void LuaPlayer::DisableBuilding(BuildingType bld)
{
    lua::assertTrue(unsigned(bld) < NUM_BUILDING_TYPES, "Invalid building type");
    player.DisableBuilding(bld);
}

void LuaPlayer::EnableAllBuildings()
{
    for(unsigned building_type = 0; building_type < NUM_BUILDING_TYPES; building_type++)
        player.EnableBuilding(BuildingType(building_type));
}

void LuaPlayer::DisableAllBuildings()
{
    for(unsigned building_type = 0; building_type < NUM_BUILDING_TYPES; building_type++)
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
                LOG.write("You don't need leading nils for SetRestrictedArea\n");
            else if(curPolyStart < 0) // We don't have a current polygon? Can only happen for multiple nils (old style)
                LOG.write("Duplicate nils found in SetRestrictedArea\n");
            else if(pts.size() < static_cast<unsigned>(curPolyStart) + 3)
                throw LuaExecutionError(std::string("Invalid polygon (less than 3 points) found at index ")
                                        + helpers::toString(std::distance(inPoints.cbegin(), it)));
            else if(pts[curPolyStart] != pts.back()) // Close polygon if not already done
                pts.push_back(pts[curPolyStart]);
            curPolyStart = -1;
        } else
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
                throw LuaExecutionError("Points must be positive");
            MapPoint pt(x, y);
            if(pt == MapPoint(0, 0))
            {
                // This might be the (old) separator if: We have a previous 0,0-pair, a valid polygon (>= 3 points) and first pt after 0,0
                // matches last pt
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

bool LuaPlayer::IsInRestrictedArea(unsigned x, unsigned y) const
{
    const GameWorldGame& world = player.GetGameWorld();
    lua::assertTrue(x < world.GetWidth(), "x coordinate to large");
    lua::assertTrue(y < world.GetHeight(), "y coordinate to large");
    return TerritoryRegion::IsPointValid(world.GetSize(), player.GetRestrictedArea(), MapPoint(x, y));
}

void LuaPlayer::ClearResources()
{
    const std::list<nobBaseWarehouse*> warehouses = player.GetBuildingRegister().GetStorehouses();
    for(auto warehouse : warehouses)
        warehouse->Clear();
}

bool LuaPlayer::AddWares(const std::map<GoodType, unsigned>& wares)
{
    nobBaseWarehouse* warehouse = player.GetFirstWH();

    if(!warehouse)
        return false;

    Inventory goods;

    for(auto ware : wares)
    {
        if(unsigned(ware.first) < NUM_WARE_TYPES)
            goods.Add(ware.first, ware.second);
        else
            throw LuaExecutionError(std::string("Invalid ware in AddWares: ") + helpers::toString(ware.first));
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

    for(auto it : people)
    {
        if(unsigned(it.first) < NUM_JOB_TYPES)
            goods.Add(it.first, it.second);
        else
            throw LuaExecutionError(std::string("Invalid job in AddPeople: ") + helpers::toString(it.first));
    }

    warehouse->AddGoods(goods, true);
    return true;
}

unsigned LuaPlayer::GetNumBuildings(BuildingType bld) const
{
    lua::assertTrue(unsigned(bld) < NUM_BUILDING_TYPES, "Invalid building type");

    return player.GetBuildingRegister().GetBuildingNums().buildings[bld];
}

unsigned LuaPlayer::GetNumBuildingSites(BuildingType bld) const
{
    lua::assertTrue(unsigned(bld) < NUM_BUILDING_TYPES, "Invalid building type");

    return player.GetBuildingRegister().GetBuildingNums().buildingSites[bld];
}

unsigned LuaPlayer::GetNumWares(GoodType ware) const
{
    lua::assertTrue(unsigned(ware) < NUM_WARE_TYPES, "Invalid ware");
    return player.GetInventory().goods[ware];
}

unsigned LuaPlayer::GetNumPeople(Job job) const
{
    lua::assertTrue(unsigned(job) < NUM_JOB_TYPES, "Invalid ware");
    return player.GetInventory().people[job];
}

bool LuaPlayer::AIConstructionOrder(unsigned x, unsigned y, BuildingType bld)
{
    // Only for actual AIs
    if(!player.isUsed() || player.isHuman())
        return false;
    lua::assertTrue(unsigned(bld) < NUM_BUILDING_TYPES, "Invalid building type");
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
        auto* hq = player.GetGameWorld().GetSpecObj<nobHQ>(hqPos);
        if(hq)
            hq->SetIsTent(isTent);
    }
}

bool LuaPlayer::IsDefeated() const
{
    return player.IsDefeated();
}

void LuaPlayer::Surrender(bool destroyBlds)
{
    player.Surrender();
    if(destroyBlds)
        player.GetGameWorld().Armageddon(player.GetPlayerId());
}

std::tuple<unsigned, unsigned> LuaPlayer::GetHQPos() const
{
    return std::tuple<unsigned, unsigned>(player.GetHQPos().x, player.GetHQPos().y);
}

bool LuaPlayer::IsAlly(unsigned char otherPlayerId)
{
    return player.IsAlly(otherPlayerId);
}

bool LuaPlayer::IsAttackable(unsigned char otherPlayerId)
{
    return player.IsAttackable(otherPlayerId);
}

void LuaPlayer::SuggestPact(unsigned char otherPlayerId, const PactType pt, const unsigned duration)
{
    auto gameInst = game.lock();
    if(!gameInst)
        return;
    AIPlayer* ai = gameInst->GetAIPlayer(player.GetPlayerId());
    if(ai != nullptr)
    {
        AIInterface aii = ai->getAIInterface();
        aii.SuggestPact(otherPlayerId, pt, duration);
    }
}

void LuaPlayer::CancelPact(const PactType pt, unsigned char otherPlayerId)
{
    auto gameInst = game.lock();
    if(!gameInst)
        return;
    AIPlayer* ai = gameInst->GetAIPlayer(player.GetPlayerId());
    if(ai != nullptr)
    {
        AIInterface aii = ai->getAIInterface();
        aii.CancelPact(pt, otherPlayerId);
    }
}
