// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "LuaPlayer.h"
#include "EventManager.h"
#include "Game.h"
#include "GamePlayer.h"
#include "ai/AIPlayer.h"
#include "buildings/nobBaseWarehouse.h"
#include "buildings/nobHQ.h"
#include "helpers/EnumRange.h"
#include "helpers/toString.h"
#include "lua/LuaHelpers.h"
#include "lua/LuaInterfaceBase.h"
#include "notifications/BuildingNote.h"
#include "postSystem/PostMsgWithBuilding.h"
#include "world/GameWorld.h"
#include "world/TerritoryRegion.h"
#include "gameTypes/BuildingCount.h"
#include "gameData/BuildingConsts.h"
#include "s25util/Log.h"
#include <kaguya/kaguya.hpp>

const BasePlayerInfo& LuaPlayer::getPlayer() const
{
    return player;
}

void LuaPlayer::Register(kaguya::State& state)
{
    LuaPlayerBase::Register(state);
    state["Player"].setClass(kaguya::UserdataMetatable<LuaPlayer, LuaPlayerBase>()
                               .addFunction("EnableBuilding", &LuaPlayer::enableBuilding)
                               .addFunction("DisableBuilding", &LuaPlayer::disableBuilding)
                               .addFunction("EnableAllBuildings", &LuaPlayer::enableAllBuildings)
                               .addFunction("DisableAllBuildings", &LuaPlayer::disableAllBuildings)
                               .addFunction("SetRestrictedArea", &LuaPlayer::setRestrictedArea)
                               .addFunction("IsInRestrictedArea", &LuaPlayer::isInRestrictedArea)
                               .addFunction("ClearResources", &LuaPlayer::clearResources)
                               .addFunction("AddWares", &LuaPlayer::addWares)
                               .addFunction("AddPeople", &LuaPlayer::addPeople)
                               .addFunction("GetNumBuildings", &LuaPlayer::getNumBuildings)
                               .addFunction("GetNumBuildingSites", &LuaPlayer::getNumBuildingSites)
                               .addFunction("GetNumWares", &LuaPlayer::getNumWares)
                               .addFunction("GetNumPeople", &LuaPlayer::getNumPeople)
                               .addFunction("GetStatisticsValue", &LuaPlayer::getStatisticsValue)
                               .addFunction("AIConstructionOrder", &LuaPlayer::aIConstructionOrder)
                               .addFunction("ModifyHQ", &LuaPlayer::modifyHQ)
                               .addFunction("GetHQPos", &LuaPlayer::getHQPos)
                               .addFunction("IsDefeated", &LuaPlayer::IsDefeated)
                               .addFunction("Surrender", &LuaPlayer::Surrender)
                               .addFunction("IsAlly", &LuaPlayer::isAlly)
                               .addFunction("IsAttackable", &LuaPlayer::isAttackable)
                               .addFunction("SuggestPact", &LuaPlayer::SuggestPact)
                               .addFunction("CancelPact", &LuaPlayer::CancelPact)
                               // Old names
                               .addFunction("GetBuildingCount", &LuaPlayer::getNumBuildings)
                               .addFunction("GetBuildingSitesCount", &LuaPlayer::getNumBuildingSites)
                               .addFunction("GetWareCount", &LuaPlayer::getNumWares)
                               .addFunction("GetPeopleCount", &LuaPlayer::getNumPeople));
}

void LuaPlayer::enableBuilding(lua::SafeEnum<BuildingType> bld, bool notify)
{
    player.enableBuilding(bld);
    if(notify)
    {
        player.sendPostMessage(
          std::make_unique<PostMsgWithBuilding>(player.getGameWorld().getEvMgr().getCurrentGF(),
                                                std::string(_("New building type:")) + "\n" + _(BUILDING_NAMES[bld]),
                                                PostCategory::General, bld, player.nation));
    }
}

void LuaPlayer::disableBuilding(lua::SafeEnum<BuildingType> bld)
{
    player.disableBuilding(bld);
}

void LuaPlayer::enableAllBuildings()
{
    for(const auto bld : helpers::enumRange<BuildingType>())
        player.enableBuilding(bld);
}

void LuaPlayer::disableAllBuildings()
{
    for(const auto bld : helpers::enumRange<BuildingType>())
        player.disableBuilding(bld);
}

void LuaPlayer::setRestrictedArea(kaguya::VariadicArgType inPoints)
{
    if(inPoints.size() == 0)
    {
        // Skip everything else if we only want to lift the restrictions
        player.getRestrictedArea().clear();
        return;
    }

    std::vector<MapPoint> pts;
    // Index where the current polygon started
    int curPolyStart = -1;
    // Detect old-style separators for multi-polygon detection
    int lastNullPt = -1;
    // Do we have multiple polygons?
    bool isMultiPoly = false;
    for(auto it = inPoints.cbegin(); it != inPoints.cend(); ++it)
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
                // This might be the (old) separator if: We have a previous 0,0-pair, a valid polygon (>= 3 points) and
                // first pt after 0,0 matches last pt
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
    player.getRestrictedArea() = pts;
}

bool LuaPlayer::isInRestrictedArea(unsigned x, unsigned y) const
{
    const GameWorld& world = player.getGameWorld();
    lua::assertTrue(x < world.getWidth(), "x coordinate to large");
    lua::assertTrue(y < world.getHeight(), "y coordinate to large");
    return TerritoryRegion::isPointValid(world.getSize(), player.getRestrictedArea(), MapPoint(x, y));
}

void LuaPlayer::clearResources()
{
    const std::list<nobBaseWarehouse*> warehouses = player.getBuildingRegister().getStorehouses();
    for(auto* warehouse : warehouses)
        warehouse->clear();
}

bool LuaPlayer::addWares(const std::map<lua::SafeEnum<GoodType>, unsigned>& wares)
{
    nobBaseWarehouse* warehouse = player.getFirstWH();

    if(!warehouse)
        return false;

    GoodCounts counts;
    for(const auto& ware : wares)
        counts[ware.first] += ware.second;

    warehouse->addToInventory(counts, true);
    return true;
}

bool LuaPlayer::addPeople(const std::map<lua::SafeEnum<Job>, unsigned>& people)
{
    nobBaseWarehouse* warehouse = player.getFirstWH();

    if(!warehouse)
        return false;

    GoodsAndPeopleCounts counts;

    for(const auto& it : people)
    {
        const Job job = it.first;
        if(job == Job::BoatCarrier)
        {
            counts[Job::Helper] += it.second;
            counts[GoodType::Boat] += it.second;
        } else
            counts[it.first] += it.second;
    }

    warehouse->addToInventory(counts, true);
    return true;
}

unsigned LuaPlayer::getNumBuildings(lua::SafeEnum<BuildingType> bld) const
{
    return player.getBuildingRegister().getBuildingNums().buildings[bld];
}

unsigned LuaPlayer::getNumBuildingSites(lua::SafeEnum<BuildingType> bld) const
{
    return player.getBuildingRegister().getBuildingNums().buildingSites[bld];
}

unsigned LuaPlayer::getNumWares(lua::SafeEnum<GoodType> ware) const
{
    return player.getInventory().goods[ware];
}

unsigned LuaPlayer::getNumPeople(lua::SafeEnum<Job> job) const
{
    return player.getInventory().people[job];
}

unsigned LuaPlayer::getStatisticsValue(lua::SafeEnum<StatisticType> stat) const
{
    return player.getStatisticCurrentValue(stat);
}

bool LuaPlayer::aIConstructionOrder(unsigned x, unsigned y, lua::SafeEnum<BuildingType> bld)
{
    // Only for actual AIs
    if(!player.isUsed() || player.isHuman())
        return false;
    GameWorld& world = player.getGameWorld();
    lua::assertTrue(x < world.getWidth(), "x coordinate to large");
    lua::assertTrue(y < world.getHeight(), "y coordinate to large");
    world.getNotifications().publish(BuildingNote(BuildingNote::LuaOrder, player.getPlayerId(), MapPoint(x, y), bld));
    return true;
}

void LuaPlayer::modifyHQ(bool isTent)
{
    player.setHQIsTent(isTent);
}

bool LuaPlayer::IsDefeated() const
{
    return player.IsDefeated();
}

void LuaPlayer::Surrender(bool destroyBlds)
{
    player.Surrender();
    if(destroyBlds)
        player.getGameWorld().armageddon(player.getPlayerId());
}

std::pair<unsigned, unsigned> LuaPlayer::getHQPos() const
{
    return std::pair<unsigned, unsigned>(player.getHQPos().x, player.getHQPos().y);
}

bool LuaPlayer::isAlly(unsigned char otherPlayerId)
{
    return player.isAlly(otherPlayerId);
}

bool LuaPlayer::isAttackable(unsigned char otherPlayerId)
{
    return player.isAttackable(otherPlayerId);
}

void LuaPlayer::SuggestPact(unsigned char otherPlayerId, const lua::SafeEnum<PactType> pt, const unsigned duration)
{
    AIPlayer* ai = game.getAIPlayer(player.getPlayerId());
    if(ai != nullptr)
        ai->getAIInterface().SuggestPact(otherPlayerId, pt, duration);
}

void LuaPlayer::CancelPact(const lua::SafeEnum<PactType> pt, unsigned char otherPlayerId)
{
    AIPlayer* ai = game.getAIPlayer(player.getPlayerId());
    if(ai != nullptr)
        ai->getAIInterface().CancelPact(pt, otherPlayerId);
}
