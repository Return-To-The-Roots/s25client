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
#include "AIPlayerJH.h"
#include "AIConstruction.h"
#include "BuildingPlanner.h"
#include "FindWhConditions.h"
#include "GamePlayer.h"
#include "Jobs.h"
#include "addons/const_addons.h"
#include "ai/AIEvents.h"
#include "boost/filesystem/fstream.hpp"
#include "buildings/noBuildingSite.h"
#include "buildings/nobHQ.h"
#include "buildings/nobHarborBuilding.h"
#include "buildings/nobMilitary.h"
#include "buildings/nobUsual.h"
#include "helpers/containerUtils.h"
#include "network/GameMessages.h"
#include "network/GameServer.h"
#include "notifications/BuildingNote.h"
#include "notifications/ExpeditionNote.h"
#include "notifications/NodeNote.h"
#include "notifications/ResourceNote.h"
#include "notifications/RoadNote.h"
#include "notifications/ShipNote.h"
#include "pathfinding/PathConditionRoad.h"
#include "nodeObjs/noAnimal.h"
#include "nodeObjs/noFlag.h"
#include "nodeObjs/noShip.h"
#include "nodeObjs/noTree.h"
#include "gameData/BuildingConsts.h"
#include "gameData/BuildingProperties.h"
#include "gameData/GameConsts.h"
#include "gameData/TerrainData.h"
#include <boost/array.hpp>
#include <boost/foreach.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/if.hpp>
#include <boost/lambda/lambda.hpp>
#include <algorithm>
#include <stdexcept>

namespace {
void HandleBuildingNote(AIEventManager& eventMgr, const BuildingNote& note)
{
    AIEvent::Base* ev;
    switch(note.type)
    {
        case BuildingNote::Constructed: ev = new AIEvent::Building(AIEvent::BuildingFinished, note.pos, note.bld); break;
        case BuildingNote::Destroyed: ev = new AIEvent::Building(AIEvent::BuildingDestroyed, note.pos, note.bld); break;
        case BuildingNote::Captured: ev = new AIEvent::Building(AIEvent::BuildingConquered, note.pos, note.bld); break;
        case BuildingNote::Lost: ev = new AIEvent::Building(AIEvent::BuildingLost, note.pos, note.bld); break;
        case BuildingNote::LostLand: ev = new AIEvent::Building(AIEvent::LostLand, note.pos, note.bld); break;
        case BuildingNote::NoRessources: ev = new AIEvent::Building(AIEvent::NoMoreResourcesReachable, note.pos, note.bld); break;
        case BuildingNote::LuaOrder: ev = new AIEvent::Building(AIEvent::LuaConstructionOrder, note.pos, note.bld); break;
        default: RTTR_Assert(false); return;
    }
    eventMgr.AddAIEvent(ev);
}
void HandleExpeditionNote(AIEventManager& eventMgr, const ExpeditionNote& note)
{
    switch(note.type)
    {
        case ExpeditionNote::Waiting: eventMgr.AddAIEvent(new AIEvent::Location(AIEvent::ExpeditionWaiting, note.pos)); break;
        case ExpeditionNote::ColonyFounded: eventMgr.AddAIEvent(new AIEvent::Location(AIEvent::NewColonyFounded, note.pos)); break;
    }
}
void HandleResourceNote(AIEventManager& eventMgr, const ResourceNote& note)
{
    eventMgr.AddAIEvent(new AIEvent::Resource(AIEvent::ResourceFound, note.pos, note.res));
}
void HandleRoadNote(AIEventManager& eventMgr, const RoadNote& note)
{
    switch(note.type)
    {
        case RoadNote::Constructed:
            eventMgr.AddAIEvent(new AIEvent::Direction(AIEvent::RoadConstructionComplete, note.pos, note.route.front()));
            break;
        case RoadNote::ConstructionFailed:
            eventMgr.AddAIEvent(new AIEvent::Direction(AIEvent::RoadConstructionFailed, note.pos, note.route.front()));
            break;
    }
}
void HandleShipNote(AIEventManager& eventMgr, const ShipNote& note)
{
    if(note.type == ShipNote::Constructed)
        eventMgr.AddAIEvent(new AIEvent::Location(AIEvent::ShipBuilt, note.pos));
}
} // namespace

namespace AIJH {

AIPlayerJH::AIPlayerJH(const unsigned char playerId, const GameWorldBase& gwb, const AI::Level level)
    : AIPlayer(playerId, gwb, level), UpgradeBldPos(MapPoint::Invalid()), isInitGfCompleted(false), defeated(player.IsDefeated())
{
    bldPlanner = new BuildingPlanner(*this);
    construction = new AIConstruction(*this);
    InitNodes();
    InitResourceMaps();
#ifdef DEBUG_AI
    SaveResourceMapsToFile();
#endif

    switch(level)
    {
        case AI::EASY:
            attack_interval = 2500;
            build_interval = 1000;
            break;
        case AI::MEDIUM:
            attack_interval = 750;
            build_interval = 400;
            break;
        case AI::HARD:
            attack_interval = 100;
            build_interval = 200;
            break;
        default: throw std::invalid_argument("Invalid AI level!");
    }
    // TODO: Use C++11 lambdas to simplify this
    // TODO: Maybe remove the AIEvents where possible and call the handler functions directly
    namespace bl = boost::lambda;
    using bl::_1;
    NotificationManager& notifications = gwb.GetNotifications();
    subBuilding = notifications.subscribe<BuildingNote>(
      bl::if_(bl::bind(&BuildingNote::player, _1) == playerId)[bl::bind(&HandleBuildingNote, boost::ref(eventManager), _1)]);
    subExpedition = notifications.subscribe<ExpeditionNote>(
      bl::if_(bl::bind(&ExpeditionNote::player, _1) == playerId)[bl::bind(&HandleExpeditionNote, boost::ref(eventManager), _1)]);
    subResource = notifications.subscribe<ResourceNote>(
      bl::if_(bl::bind(&ResourceNote::player, _1) == playerId)[bl::bind(&HandleResourceNote, boost::ref(eventManager), _1)]);
    subRoad = notifications.subscribe<RoadNote>(
      bl::if_(bl::bind(&RoadNote::player, _1) == playerId)[bl::bind(&HandleRoadNote, boost::ref(eventManager), _1)]);
    subShip = notifications.subscribe<ShipNote>(
      bl::if_(bl::bind(&ShipNote::player, _1) == playerId)[bl::bind(&HandleShipNote, boost::ref(eventManager), _1)]);
    subBQ = notifications.subscribe<NodeNote>(
      bl::if_(bl::bind(&NodeNote::type, _1) == NodeNote::BQ)[bl::bind(&AIPlayerJH::UpdateNodeBQ, this, bl::bind(&NodeNote::pos, _1))]);
}

AIPlayerJH::~AIPlayerJH()
{
    delete construction;
    delete bldPlanner;
}

/// Wird jeden GF aufgerufen und die KI kann hier entsprechende Handlungen vollziehen
void AIPlayerJH::RunGF(const unsigned gf, bool gfisnwf)
{
    if(defeated)
        return;

    if(TestDefeat())
        return;
    if(!isInitGfCompleted)
    {
        InitStoreAndMilitarylists();
        InitDistribution();
    }
    if(isInitGfCompleted < 10)
    {
        isInitGfCompleted++;
        return; //  1 init -> 2 test defeat -> 3 do other ai stuff -> goto 2
    }
    bldPlanner->Update(gf, *this);

    if(gfisnwf) // nwf -> now the orders have been executed -> new constructions can be started
        construction->ConstructionsExecuted();

    if(gf == 100)
    {
        if(aii.GetMilitaryBuildings().empty() && aii.GetStorehouses().size() < 2)
        {
            Chat(_("Hi, I'm an artifical player and I'm not very good yet!"));
            // AI doesn't usually crash the game any more :)
            // Chat(_("And I may crash your game sometimes..."));
        }
    }

    if(!gfisnwf) // try to complete a job on the list
    {
        // LOG.write(("ai doing stuff %i \n",playerId);
        if(gf % 100 == 0)
            bldPlanner->UpdateBuildingsWanted(*this);
        ExecuteAIJob();
    }

    if((gf + playerId * 17) % attack_interval == 0)
    {
        // CheckExistingMilitaryBuildings();
        TryToAttack();
    }
    if(((gf + playerId * 17) % 73 == 0) && (level != AI::EASY))
    {
        MilUpgradeOptim();
    }

    if((gf + 41 + playerId * 17) % attack_interval == 0)
    {
        if(ggs.getSelection(AddonId::SEA_ATTACK) < 2) // not deactivated by addon? -> go ahead
            TrySeaAttack();
    }

    if((gf + playerId * 13) % 1500 == 0)
    {
        CheckExpeditions();
        CheckForester();
        CheckGranitMine();
    }

    if((gf + playerId * 11) % 150 == 0)
    {
        AdjustSettings();
        // check for useless sawmills
        const std::list<nobUsual*>& sawMills = aii.GetBuildings(BLD_SAWMILL);
        if(sawMills.size() > 3)
        {
            int burns = 0;
            BOOST_FOREACH(const nobUsual* sawmill, sawMills)
            {
                if(sawmill->GetProductivity() < 1 && sawmill->HasWorker() && sawmill->GetNumWares(0) < 1 && (sawMills.size() - burns) > 3
                   && !sawmill->AreThereAnyOrderedWares())
                {
                    aii.DestroyBuilding(sawmill);
                    RemoveUnusedRoad(*sawmill->GetFlag(), 1, true);
                    burns++;
                }
            }
        }
    }

    if((gf + playerId * 7) % build_interval == 0) // plan new buildings
    {
        CheckForUnconnectedBuildingSites();
        PlanNewBuildings(gf);
    }
}

void AIPlayerJH::PlanNewBuildings(const unsigned gf)
{
    bldPlanner->UpdateBuildingsWanted(*this);

    // pick a random storehouse and try to build one of these buildings around it (checks if we actually want more of the building type)
    boost::array<BuildingType, 24> bldToTest = {{BLD_HARBORBUILDING, BLD_SHIPYARD,       BLD_SAWMILL,    BLD_FORESTER,     BLD_FARM,
                                                 BLD_FISHERY,        BLD_WOODCUTTER,     BLD_QUARRY,     BLD_GOLDMINE,     BLD_IRONMINE,
                                                 BLD_COALMINE,       BLD_GRANITEMINE,    BLD_HUNTER,     BLD_CHARBURNER,   BLD_IRONSMELTER,
                                                 BLD_MINT,           BLD_ARMORY,         BLD_METALWORKS, BLD_BREWERY,      BLD_MILL,
                                                 BLD_PIGFARM,        BLD_SLAUGHTERHOUSE, BLD_BAKERY,     BLD_DONKEYBREEDER}};
    const unsigned numResGatherBlds = 14; /* The first n buildings in the above list, that gather resources */

    // LOG.write(("new buildorders %i whs and %i mil for player %i
    // \n",aii.GetStorehouses().size(),aii.GetMilitaryBuildings().size(),playerId);

    const std::list<nobBaseWarehouse*>& storehouses = aii.GetStorehouses();
    if(!storehouses.empty())
    {
        // collect swords,shields,helpers,privates and beer in first storehouse or whatever is closest to the upgradebuilding if we have
        // one!
        nobBaseWarehouse* wh = GetUpgradeBuildingWarehouse();
        SetGatheringForUpgradeWarehouse(wh);

        if(ggs.GetMaxMilitaryRank() > 0) // there is more than 1 rank available -> distribute
            DistributeMaxRankSoldiersByBlocking(5, wh);
        // 30 boards amd 50 stones for each warehouse - block after that - should speed up expansion and limit losses in case a warehouse is
        // destroyed unlimited when every warehouse has at least that amount
        DistributeGoodsByBlocking(GD_BOARDS, 30);
        DistributeGoodsByBlocking(GD_STONES, 50);
        // go to the picked random warehouse and try to build around it
        int randomStore = rand() % (storehouses.size());
        std::list<nobBaseWarehouse*>::const_iterator it = storehouses.begin();
        std::advance(it, randomStore);
        const MapPoint whPos = (*it)->GetPos();
        UpdateNodesAround(whPos, 15); // update the area we want to build in first
        for(unsigned i = 0; i < bldToTest.size(); i++)
        {
            if(construction->Wanted(bldToTest[i]))
            {
                AddBuildJobAroundEveryWarehouse(bldToTest[i]); // add a buildorder for the picked buildingtype at every warehouse
            }
        }
        if(gf > 1500 || aii.GetInventory().goods[GD_BOARDS] > 11)
            AddBuildJob(construction->ChooseMilitaryBuilding(whPos), whPos);
    }
    // end of construction around & orders for warehouses

    // now pick a random military building and try to build around that as well
    const std::list<nobMilitary*>& militaryBuildings = aii.GetMilitaryBuildings();
    if(militaryBuildings.empty())
        return;
    int randomMiliBld = rand() % (militaryBuildings.size());
    std::list<nobMilitary*>::const_iterator it2 = militaryBuildings.begin();
    std::advance(it2, randomMiliBld);
    MapPoint bldPos = (*it2)->GetPos();
    UpdateNodesAround(bldPos, 15);
    // resource gathering buildings only around military; processing only close to warehouses
    for(unsigned i = 0; i < numResGatherBlds; i++)
    {
        if(construction->Wanted(bldToTest[i]))
        {
            AddBuildJobAroundEveryMilBld(bldToTest[i]);
        }
    }
    AddBuildJob(construction->ChooseMilitaryBuilding(bldPos), bldPos);
    if((*it2)->IsUseless() && (*it2)->IsDemolitionAllowed() && randomMiliBld != UpdateUpgradeBuilding())
    {
        aii.DestroyBuilding(bldPos);
    }
}

bool AIPlayerJH::TestDefeat()
{
    if(isInitGfCompleted >= 10 && aii.GetStorehouses().empty())
    {
        // LOG.write(("ai defeated player %i \n",playerId);
        defeated = true;
        aii.Surrender();
        Chat(_("You win"));
        return true;
    }
    return false;
}

unsigned AIPlayerJH::GetNumJobs() const
{
    return eventManager.GetEventNum() + construction->GetBuildJobNum() + construction->GetConnectJobNum();
}

/// returns the warehouse closest to the upgradebuilding or if it cant find a way the first warehouse and if there is no warehouse left null
nobBaseWarehouse* AIPlayerJH::GetUpgradeBuildingWarehouse()
{
    const std::list<nobBaseWarehouse*>& storehouses = aii.GetStorehouses();
    if(storehouses.empty())
        return NULL;
    nobBaseWarehouse* wh = storehouses.front();
    int uub = UpdateUpgradeBuilding();

    if(uub >= 0 && storehouses.size() > 1) // upgradebuilding exists and more than 1 warehouse -> find warehouse closest to the
                                           // upgradebuilding - gather stuff there and deactivate gathering in the previous one
    {
        std::list<nobMilitary*>::const_iterator upgradeBldIt = aii.GetMilitaryBuildings().begin();
        std::advance(upgradeBldIt, uub);
        // which warehouse is closest to the upgrade building? -> train troops there and block max ranks
        wh = aii.FindWarehouse(**upgradeBldIt, FW::NoCondition(), false, false);
        if(!wh)
            wh = storehouses.front();
    }
    return wh;
}

void AIPlayerJH::AddBuildJob(BuildingType type, const MapPoint pt, bool front, bool searchPosition)
{
    if(type != BLD_NOTHING)
        construction->AddBuildJob(new BuildJob(*this, type, pt, searchPosition ? SEARCHMODE_RADIUS : SEARCHMODE_NONE), front);
}

void AIPlayerJH::AddBuildJobAroundEveryWarehouse(BuildingType bt)
{
    BOOST_FOREACH(const nobBaseWarehouse* wh, aii.GetStorehouses())
    {
        AddBuildJob(bt, wh->GetPos(), false);
    }
}

void AIPlayerJH::AddBuildJobAroundEveryMilBld(BuildingType bt)
{
    BOOST_FOREACH(const nobMilitary* milBld, aii.GetMilitaryBuildings())
    {
        AddBuildJob(bt, milBld->GetPos(), false);
    }
}

void AIPlayerJH::SetGatheringForUpgradeWarehouse(nobBaseWarehouse* upgradewarehouse)
{
    BOOST_FOREACH(const nobBaseWarehouse* wh, aii.GetStorehouses())
    {
        // deactivate gathering for all warehouses that are NOT the one next to the upgradebuilding
        const MapPoint whPos = wh->GetPos();
        if(upgradewarehouse->GetPos() != whPos)
        {
            if(wh->IsInventorySetting(GD_BEER, EInventorySetting::COLLECT)) // collecting beer? -> stop it
                aii.SetInventorySetting(whPos, GD_BEER, InventorySetting());

            if(wh->IsInventorySetting(GD_SWORD, EInventorySetting::COLLECT)) // collecting swords? -> stop it
                aii.SetInventorySetting(whPos, GD_SWORD, InventorySetting());

            if(wh->IsInventorySetting(GD_SHIELDROMANS, EInventorySetting::COLLECT)) // collecting shields? -> stop it
                aii.SetInventorySetting(whPos, GD_SHIELDROMANS, InventorySetting());

            if(wh->IsInventorySetting(JOB_PRIVATE, EInventorySetting::COLLECT)) // collecting privates? -> stop it
                aii.SetInventorySetting(whPos, JOB_PRIVATE, InventorySetting());

            if(wh->IsInventorySetting(JOB_HELPER, EInventorySetting::COLLECT)) // collecting helpers? -> stop it
                aii.SetInventorySetting(whPos, JOB_HELPER, InventorySetting());
        } else // activate gathering in the closest warehouse
        {
            if(!wh->IsInventorySetting(GD_BEER, EInventorySetting::COLLECT)) // not collecting beer? -> start it
                aii.SetInventorySetting(whPos, GD_BEER, EInventorySetting::COLLECT);

            if(!wh->IsInventorySetting(GD_SWORD, EInventorySetting::COLLECT)) // not collecting swords? -> start it
                aii.SetInventorySetting(whPos, GD_SWORD, EInventorySetting::COLLECT);

            if(!wh->IsInventorySetting(GD_SHIELDROMANS, EInventorySetting::COLLECT)) // not collecting shields? -> start it
                aii.SetInventorySetting(whPos, GD_SHIELDROMANS, EInventorySetting::COLLECT);

            if(!wh->IsInventorySetting(JOB_PRIVATE, EInventorySetting::COLLECT)
               && ggs.GetMaxMilitaryRank() > 0) // not collecting privates AND we can actually upgrade soldiers? -> start it
                aii.SetInventorySetting(whPos, JOB_PRIVATE, EInventorySetting::COLLECT);

            // less than 50 helpers - collect them: more than 50 stop collecting
            if(wh->GetInventory().people[JOB_HELPER] < 50)
            {
                if(!wh->IsInventorySetting(JOB_HELPER, EInventorySetting::COLLECT))
                    aii.SetInventorySetting(whPos, JOB_HELPER, EInventorySetting::COLLECT);
            } else
            {
                if(wh->IsInventorySetting(JOB_HELPER, EInventorySetting::COLLECT))
                    aii.SetInventorySetting(whPos, JOB_HELPER, InventorySetting());
            }
        }
    }
}

AIResource AIPlayerJH::CalcResource(const MapPoint pt)
{
    AIResource subRes = aii.GetSubsurfaceResource(pt);
    AIResource surfRes = aii.GetSurfaceResource(pt);

    // no resources underground
    if(subRes == AIResource::NOTHING)
    {
        // also no resource on the ground: plant space or unusable?
        if(surfRes == AIResource::NOTHING)
        {
            // already road, really no resources here
            if(gwb.IsOnRoad(pt))
                return AIResource::NOTHING;
            // check for vital plant space
            for(unsigned i = 0; i < Direction::COUNT; ++i)
            {
                TerrainType t = gwb.GetRightTerrain(pt, Direction::fromInt(i));

                // check against valid terrains for planting
                if(!TerrainData::IsVital(t))
                    return AIResource::NOTHING;
            }
            return AIResource::PLANTSPACE;
        }

        return surfRes;
    } else // resources in underground
    {
        if(surfRes == AIResource::STONES || surfRes == AIResource::WOOD)
            return AIResource::MULTIPLE;

        if(subRes == AIResource::BLOCKED)
            return AIResource::NOTHING; // nicht so ganz logisch... aber Blocked als res is doof TODO

        return subRes;
    }
}

void AIPlayerJH::InitReachableNodes()
{
    std::queue<MapPoint> toCheck;

    // Alle auf not reachable setzen
    RTTR_FOREACH_PT(MapPoint, aiMap.GetSize())
    {
        Node& node = aiMap[pt];
        node.reachable = false;
        node.failed_penalty = 0;
        const noFlag* myFlag = gwb.GetSpecObj<noFlag>(pt);
        if(myFlag && myFlag->GetPlayer() == playerId)
        {
            node.reachable = true;
            toCheck.push(pt);
        }
    }

    IterativeReachableNodeChecker(toCheck);
}

void AIPlayerJH::IterativeReachableNodeChecker(std::queue<MapPoint> toCheck)
{
    // TODO auch mal bootswege bauen können

    PathConditionRoad<GameWorldBase> roadPathChecker(gwb, false);
    while(!toCheck.empty())
    {
        // Reachable coordinate
        MapPoint curPt = toCheck.front();

        // Coordinates to test around this reachable coordinate
        for(unsigned dir = 0; dir < Direction::COUNT; ++dir)
        {
            MapPoint curNeighbour = aiMap.GetNeighbour(curPt, Direction::fromInt(dir));
            Node& node = aiMap[curNeighbour];

            // already reached, don't test again
            if(node.reachable)
                continue;

            // Test whether point is reachable; yes->add to check list
            if(roadPathChecker.IsNodeOk(curNeighbour))
            {
                if(node.failed_penalty == 0)
                {
                    node.reachable = true;
                    toCheck.push(curNeighbour);
                } else
                {
                    node.failed_penalty--;
                }
            }
        }
        toCheck.pop();
    }
}

void AIPlayerJH::UpdateReachableNodes(const std::vector<MapPoint>& pts)
{
    std::queue<MapPoint> toCheck;

    BOOST_FOREACH(const MapPoint& curPt, pts)
    {
        const noFlag* flag = gwb.GetSpecObj<noFlag>(curPt);
        if(flag && flag->GetPlayer() == playerId)
        {
            aiMap[curPt].reachable = true;
            toCheck.push(curPt);
        } else
            aiMap[curPt].reachable = false;
    }
    IterativeReachableNodeChecker(toCheck);
}

void AIPlayerJH::InitNodes()
{
    aiMap.Resize(gwb.GetSize());

    InitReachableNodes();

    RTTR_FOREACH_PT(MapPoint, aiMap.GetSize())
    {
        Node& node = aiMap[pt];

        node.bq = aii.GetBuildingQuality(pt);
        node.res = CalcResource(pt);
        node.owned = aii.IsOwnTerritory(pt);
        node.border = aii.IsBorder(pt);
        node.farmed = false;
    }
}

void AIPlayerJH::UpdateNodesAround(const MapPoint pt, unsigned radius)
{
    std::vector<MapPoint> pts = gwb.GetPointsInRadius(pt, radius);
    UpdateReachableNodes(pts);
    BOOST_FOREACH(const MapPoint& pt, pts)
    {
        Node& node = aiMap[pt];
        // Change of ownership might change bq
        node.bq = aii.GetBuildingQuality(pt);
        node.owned = aii.IsOwnTerritory(pt);
        node.border = aii.IsBorder(pt);
    }
}

void AIPlayerJH::UpdateNodeBQ(const MapPoint& pt)
{
    BuildingQuality newBQ = aii.GetBuildingQuality(pt);
    if(aiMap[pt].bq != newBQ)
    {
        aiMap[pt].bq = newBQ;
        // Neighbour points might change to (flags at borders etc.)
        BOOST_FOREACH(const MapPoint& curPt, gwb.GetPointsInRadius(pt, 1))
            UpdateNodeBQ(curPt);
    }
}

void AIPlayerJH::InitResourceMaps()
{
    resourceMaps.clear();
    for(unsigned res = 0; res < NUM_AIRESOURCES; ++res)
    {
        resourceMaps.push_back(AIResourceMap(static_cast<AIResource>(res), aii, aiMap));
        resourceMaps.back().Init();
    }
}

void AIPlayerJH::SetFarmedNodes(const MapPoint pt, bool set)
{
    // Radius in dem Bausplatz für Felder blockiert wird
    const unsigned radius = 3;

    aiMap[pt].farmed = set;
    std::vector<MapPoint> pts = gwb.GetPointsInRadius(pt, radius);
    BOOST_FOREACH(const MapPoint& curPt, pts)
        aiMap[curPt].farmed = set;
}

MapPoint AIPlayerJH::FindGoodPosition(const MapPoint& pt, AIResource res, int threshold, BuildingQuality size, int radius,
                                      bool inTerritory) const
{
    return resourceMaps[boost::underlying_cast<unsigned>(res)].FindGoodPosition(pt, threshold, size, radius, inTerritory);
}

MapPoint AIPlayerJH::FindBestPositionDiminishingResource(const MapPoint& pt, AIResource res, BuildingQuality size, int minimum, int radius,
                                                         bool inTerritory)
{
    RTTR_Assert(pt.x < aiMap.GetWidth() && pt.y < aiMap.GetHeight());
    bool fixed = ggs.isEnabled(AddonId::INEXHAUSTIBLE_MINES)
                 && (res == AIResource::IRONORE || res == AIResource::COAL || res == AIResource::GOLD || res == AIResource::GRANITE);
    int temp = 0;
    bool lastcirclevaluecalculated = false;
    bool lastvaluecalculated = false;
    // to avoid having to calculate a value twice and still move left on the same level without any problems we use this variable to
    // remember the first calculation we did in the circle.
    int circlestartvalue = 0;

    // TODO was besseres wär schön ;)
    if(radius == -1)
        radius = 11;

    MapPoint best = MapPoint::Invalid();
    int best_value = (minimum == std::numeric_limits<int>::min()) ? minimum : minimum - 1;

    for(MapCoord tx = gwb.GetXA(pt, Direction::WEST), r = 1; r <= radius; tx = gwb.GetXA(MapPoint(tx, pt.y), Direction::WEST), ++r)
    {
        MapPoint curPt(tx, pt.y);
        for(unsigned curDir = 2; curDir < 8; ++curDir)
        {
            for(MapCoord step = 0; step < r; ++step, curPt = aiMap.GetNeighbour(curPt, Direction(curDir)))
            {
                int& resMapVal = resourceMaps[boost::underlying_cast<unsigned>(res)][curPt];
                if(fixed)
                    temp = resMapVal;
                else
                {
                    // only do a complete calculation for the first point or when moving outward and the last value is unknown
                    if((r < 2 || !lastcirclevaluecalculated) && step < 1 && curDir < 3 && resMapVal)
                    {
                        temp = aii.CalcResourceValue(curPt, res);
                        circlestartvalue = temp;
                        lastcirclevaluecalculated = true;
                        lastvaluecalculated = true;
                    } else if(!resMapVal) // was there ever anything? if not skip it!
                    {
                        if(step < 1 && curDir < 3)
                            lastcirclevaluecalculated = false;
                        lastvaluecalculated = false;
                        temp = resMapVal;
                    } else if(step < 1 && curDir < 3) // circle not yet started? -> last direction was outward (left=0)
                    {
                        temp = aii.CalcResourceValue(curPt, res, 0, circlestartvalue);
                        circlestartvalue = temp;
                    } else if(lastvaluecalculated)
                    {
                        if(step > 0) // we moved direction i%6
                            temp = aii.CalcResourceValue(curPt, res, curDir % 6, temp);
                        else // last step was the previous direction
                            temp = aii.CalcResourceValue(curPt, res, (curDir - 1) % 6, temp);
                    } else
                    {
                        temp = aii.CalcResourceValue(curPt, res);
                        lastvaluecalculated = true;
                    }
                    // if(resMapVal)
                    // RTTR_Assert(temp==aii.CalcResourceValue(t2,res));
                    // copy the value to the resource map
                    resMapVal = temp;
                }
                if(res == AIResource::FISH || res == AIResource::STONES)
                {
                    // remove permanently invalid spots to speed up future checks
                    TerrainType t1 = aii.GetTerrain(curPt);
                    if(!TerrainData::IsUseable(t1) || TerrainData::IsMineable(t1) || t1 == TT_DESERT)
                        resMapVal = 0;
                } else //= granite,gold,iron,coal
                {
                    if(!TerrainData::IsMineable(aii.GetTerrain(curPt)))
                        resMapVal = 0;
                }
                if(temp > best_value)
                {
                    if(!aiMap[curPt].reachable || (inTerritory && !aii.IsOwnTerritory(curPt)) || aiMap[curPt].farmed)
                        continue;
                    // special case fish -> check for other fishery buildings
                    if(res == AIResource::FISH && BuildingNearby(curPt, BLD_FISHERY, 6))
                        continue;
                    // dont build next to harborspots
                    if(HarborPosClose(curPt, 3, true))
                        continue;
                    RTTR_Assert(aii.GetBuildingQuality(curPt) == GetAINode(curPt).bq);
                    if(canUseBq(aii.GetBuildingQuality(curPt), size))
                    {
                        best = curPt;
                        best_value = temp;
                        // TODO: calculate "perfect" rating and instantly return if we got that already
                    }
                }
            }
        }
    }

    return best;
}

// TODO: this totally ignores existing buildings of the same type. It should not. Re-introduce the resource maps?
MapPoint AIPlayerJH::FindBestPosition(const MapPoint& pt, AIResource res, BuildingQuality size, int minimum, int radius, bool inTerritory)
{
    if(res == AIResource::IRONORE || res == AIResource::COAL || res == AIResource::GOLD || res == AIResource::GRANITE
       || res == AIResource::STONES || res == AIResource::FISH)
        return FindBestPositionDiminishingResource(pt, res, size, minimum, radius, inTerritory);
    RTTR_Assert(pt.x < aiMap.GetWidth() && pt.y < aiMap.GetHeight());
    // to avoid having to calculate a value twice and still move left on the same level without any problems we use this variable to
    // remember the first calculation we did in the circle.
    int circlestartvalue = 0;

    // TODO was besseres wär schön ;)
    if(radius == -1)
        radius = 11;

    MapPoint best = MapPoint::Invalid();
    int best_value = (minimum == std::numeric_limits<int>::min()) ? minimum : minimum - 1;
    int temp = 0;

    for(MapCoord tx = gwb.GetXA(pt, Direction::WEST), r = 1; r <= radius; tx = gwb.GetXA(MapPoint(tx, pt.y), Direction::WEST), ++r)
    {
        MapPoint curPt(tx, pt.y);
        for(unsigned curDir = 2; curDir < 8; ++curDir)
        {
            for(MapCoord step = 0; step < r; ++step, curPt = gwb.GetNeighbour(curPt, Direction(curDir)))
            {
                if(r == 1 && step == 0 && curDir == 2)
                {
                    // only do a complete calculation for the first point!
                    temp = aii.CalcResourceValue(curPt, res);
                    circlestartvalue = temp;
                } else if(step == 0 && curDir == 2)
                {
                    // circle not yet started? -> last direction was outward (left=0)
                    temp = aii.CalcResourceValue(curPt, res, 0, circlestartvalue);
                    circlestartvalue = temp;
                } else if(step > 0) // we moved direction i%6
                    temp = aii.CalcResourceValue(curPt, res, curDir % 6, temp);
                else // last step was the previous direction
                    temp = aii.CalcResourceValue(curPt, res, (curDir - 1) % 6, temp);
                // copy the value to the resource map (map is only used in the ai debug mode)
                resourceMaps[boost::underlying_cast<unsigned>(res)][curPt] = temp;
                if(temp > best_value)
                {
                    if(!aiMap[curPt].reachable || (inTerritory && !aii.IsOwnTerritory(curPt)) || aiMap[curPt].farmed)
                        continue;
                    if(HarborPosClose(curPt, 3, true))
                        continue;
                    RTTR_Assert(aii.GetBuildingQuality(curPt) == GetAINode(curPt).bq);
                    if(canUseBq(aii.GetBuildingQuality(curPt), size)
                       && (res != AIResource::BORDERLAND || !gwb.IsOnRoad(gwb.GetNeighbour(curPt, Direction::SOUTHEAST))))
                    // special: military buildings cannot be build next to an existing road as that would have them connected to 2 roads
                    // which the ai no longer should do
                    {
                        best = curPt;
                        best_value = temp;
                        // TODO: calculate "perfect" rating and instantly return if we got that already
                    }
                }
            }
        }
    }

    return best;
}

void AIPlayerJH::ExecuteAIJob()
{
    // Check whether current job is finished...
    /*if (currentJob)
    {
        if (currentJob->GetStatus() == JOB_FINISHED)
        {
            delete currentJob;
            currentJob = 0;
        }
    }

    // ... or it failed
    if (currentJob)
    {
        if (currentJob->GetStatus() == JOB_FAILED)
        {
            // TODO fehlerbehandlung?
            //std::cout << "Job failed." << std::endl;
            delete currentJob;
            currentJob = 0;
        }
    }*/
    unsigned quota = 10;                          // limit the amount of events to handle
    while(eventManager.EventAvailable() && quota) // handle all new events - some will add new orders but they can all be handled instantly
    {
        quota--;
        currentJob.reset(new EventJob(*this, eventManager.GetEvent()));
        currentJob->ExecuteJob();
    }
    // how many construction & connect jobs the ai will attempt every gf, the ai gets new orders from events and every 200 gf
    quota = (aii.GetStorehouses().size() + aii.GetMilitaryBuildings().size()) * 1;
    if(quota > 40)
        quota = 40;

    construction->ExecuteJobs(quota); // try to execute up to quota connect & construction jobs
    /*
    // if no current job available, take next one! events first, then constructions
    if (!currentJob)
    {
        if (construction->BuildJobAvailable())
        {
            currentJob = construction->GetBuildJob();
        }
    }
    // Something to do? Do it!
    if (currentJob)
        currentJob->ExecuteJob();
        */
}

void AIPlayerJH::RecalcBQAround(const MapPoint /*pt*/) {}

void AIPlayerJH::CheckNewMilitaryBuildings() {}

void AIPlayerJH::DistributeGoodsByBlocking(const GoodType good, unsigned limit)
{
    const std::list<nobBaseWarehouse*>& storehouses = aii.GetStorehouses();
    if(aii.GetHarbors().size() >= storehouses.size() / 2)
    {
        // dont distribute on maps that are mostly sea maps - harbors are too difficult to defend and have to handle quite a lot of traffic
        // already
        // So unblock everywhere
        BOOST_FOREACH(nobBaseWarehouse* wh, storehouses)
        {
            if(wh->IsInventorySetting(good, EInventorySetting::STOP)) // not unblocked then issue command to unblock
                aii.SetInventorySetting(wh->GetPos(), good, wh->GetInventorySetting(good).Toggle(EInventorySetting::STOP));
        }
        return;
    }

    RTTR_Assert(storehouses.size() >= 2); // Should be assured by condition above
    // We can only distribute between reachable warehouses, so divide them
    std::vector<std::vector<const nobBaseWarehouse*> > whsByReachability;
    BOOST_FOREACH(const nobBaseWarehouse* wh, storehouses)
    {
        // See to which other whs this is connected
        bool foundConnectedWh = false;
        BOOST_FOREACH(std::vector<const nobBaseWarehouse*>& whGroup, whsByReachability)
        {
            if(aii.FindPathOnRoads(*wh, *whGroup.front()))
            {
                whGroup.push_back(wh);
                foundConnectedWh = true;
                break;
            }
        }
        // Not connected to any other -> Add new group
        if(!foundConnectedWh)
            whsByReachability.push_back(std::vector<const nobBaseWarehouse*>(1, wh));
    }

    // Now check each group individually
    BOOST_FOREACH(const std::vector<const nobBaseWarehouse*>& whGroup, whsByReachability)
    {
        // First check if all WHs have more than limit goods (or better: if one does not)
        bool allWHsHaveLimit = true;
        BOOST_FOREACH(const nobBaseWarehouse* wh, whGroup)
        {
            if(wh->GetNumVisualWares(good) <= limit)
            {
                allWHsHaveLimit = false;
                break;
            }
        }
        if(allWHsHaveLimit)
        {
            // So unblock everywhere
            BOOST_FOREACH(const nobBaseWarehouse* wh, whGroup)
            {
                if(wh->IsInventorySetting(good, EInventorySetting::STOP)) // not unblocked then issue command to unblock
                    aii.SetInventorySetting(wh->GetPos(), good, wh->GetInventorySetting(good).Toggle(EInventorySetting::STOP));
            }
        } else
        {
            // At least 1 WH needs wares
            BOOST_FOREACH(const nobBaseWarehouse* wh, whGroup)
            {
                if(wh->GetNumVisualWares(good) <= limit) // not at limit - unblock it
                {
                    if(wh->IsInventorySetting(good, EInventorySetting::STOP)) // not unblocked then issue command to unblock
                        aii.SetInventorySetting(wh->GetPos(), good, wh->GetInventorySetting(good).Toggle(EInventorySetting::STOP));
                } else // at limit - block it
                {
                    if(!wh->IsInventorySetting(good, EInventorySetting::STOP)) // not blocked then issue command to block
                        aii.SetInventorySetting(wh->GetPos(), good, wh->GetInventorySetting(good).Toggle(EInventorySetting::STOP));
                }
            }
        }
    }
}

void AIPlayerJH::DistributeMaxRankSoldiersByBlocking(unsigned limit, nobBaseWarehouse* upwh)
{
    const std::list<nobBaseWarehouse*>& storehouses = aii.GetStorehouses();
    unsigned numCompleteWh = storehouses.size();

    if(numCompleteWh < 1) // no warehouses -> no job
        return;

    ::Job maxRankJob = SOLDIER_JOBS[ggs.GetMaxMilitaryRank()];

    if(numCompleteWh == 1) // only 1 warehouse? dont block max ranks here
    {
        nobBaseWarehouse& wh = *storehouses.front();
        if(wh.IsInventorySetting(maxRankJob, EInventorySetting::STOP))
            aii.SetInventorySetting(wh.GetPos(), maxRankJob, wh.GetInventorySetting(maxRankJob).Toggle(EInventorySetting::STOP));
        return;
    }
    // rest applies for at least 2 complete warehouses!
    std::list<const nobMilitary*> frontierMils; // make a list containing frontier military buildings
    BOOST_FOREACH(const nobMilitary* wh, aii.GetMilitaryBuildings())
    {
        if(wh->GetFrontierDistance() > 0 && !wh->IsNewBuilt())
            frontierMils.push_back(wh);
    }
    std::list<const nobBaseWarehouse*> frontierWhs; // make a list containing all warehouses near frontier military buildings
    BOOST_FOREACH(const nobBaseWarehouse* wh, storehouses)
    {
        BOOST_FOREACH(const nobMilitary* milBld, frontierMils)
        {
            if(gwb.CalcDistance(wh->GetPos(), milBld->GetPos()) < 12)
            {
                frontierWhs.push_back(wh);
                break;
            }
        }
    }
    // have frontier warehouses?
    if(!frontierWhs.empty())
    {
        // LOG.write(("distribute maxranks - got frontierwhs for player %i \n",playerId);
        bool hasUnderstaffedWh = false;
        // try to gather limit maxranks in each - if we have that many unblock for all frontier whs,
        // check if there is at least one with less than limit first
        BOOST_FOREACH(const nobBaseWarehouse* wh, frontierWhs)
        {
            if(wh->GetInventory().people[maxRankJob] < limit)
            {
                hasUnderstaffedWh = true;
                break;
            }
        }
        // if understaffed was found block in all with >=limit else unblock in all
        BOOST_FOREACH(const nobBaseWarehouse* wh, storehouses)
        {
            bool shouldBlock;
            if(helpers::contains(frontierWhs, wh)) // frontier wh?
            {
                if(hasUnderstaffedWh)
                {
                    if(wh->GetInventory().people[maxRankJob] < limit)
                        shouldBlock = false;
                    else // more than limit
                        shouldBlock = true;
                } else // no understaffedwh
                    shouldBlock = false;
            } else // not frontier wh! block it
                shouldBlock = true;
            if(shouldBlock != wh->IsInventorySetting(maxRankJob, EInventorySetting::STOP))
                aii.SetInventorySetting(wh->GetPos(), maxRankJob, wh->GetInventorySetting(maxRankJob).Toggle(EInventorySetting::STOP));
        }
    } else // there are no frontier whs!
    {
        // LOG.write(("distribute maxranks - got NO frontierwhs for player %i \n",playerId);
        bool hasUnderstaffedWh = false;
        // try to gather limit maxranks in each - if we have that many unblock for all  whs,
        // check if there is at least one with less than limit first
        BOOST_FOREACH(const nobBaseWarehouse* wh, storehouses)
        {
            if(wh->GetInventory().people[maxRankJob] < limit
               && wh->GetPos() != upwh->GetPos()) // warehouse next to upgradebuilding is special case
            {
                hasUnderstaffedWh = true;
                break;
            }
        }
        BOOST_FOREACH(const nobBaseWarehouse* wh, storehouses)
        {
            bool shouldBlock;
            if(wh->GetPos() == upwh->GetPos()) // warehouse next to upgradebuilding should block when there is more than 1 wh
            {
                // LOG.write(("distribute maxranks - got NO frontierwhs for player %i , block at hq \n",playerId);
                shouldBlock = true;
            } else if(hasUnderstaffedWh)
            {
                if(wh->GetInventory().people[maxRankJob] < limit)
                    shouldBlock = false;
                else // more than limit
                    shouldBlock = true;
            } else // no understaffedwh
                shouldBlock = false;
            if(shouldBlock != wh->IsInventorySetting(maxRankJob, EInventorySetting::STOP))
                aii.SetInventorySetting(wh->GetPos(), maxRankJob, wh->GetInventorySetting(maxRankJob).Toggle(EInventorySetting::STOP));
        }
    }
}
MapPoint AIPlayerJH::SimpleFindPosition(const MapPoint& pt, BuildingQuality size, int radius) const
{
    RTTR_Assert(pt.x < aiMap.GetWidth() && pt.y < aiMap.GetHeight());

    // TODO was besseres wär schön ;)
    if(radius == -1)
        radius = 30;

    std::vector<MapPoint> pts = gwb.GetPointsInRadius(pt, radius);
    BOOST_FOREACH(const MapPoint& curPt, pts)
    {
        if(!aiMap[curPt].reachable || aiMap[curPt].farmed || !aii.IsOwnTerritory(curPt))
            continue;
        if(HarborPosClose(curPt, 3, true))
        {
            if(size != BQ_HARBOR)
                continue;
        }
        RTTR_Assert(aii.GetBuildingQuality(curPt) == GetAINode(curPt).bq);
        if(canUseBq(aii.GetBuildingQuality(curPt), size)) //(*nodes)[idx].bq; TODO: Update nodes BQ and use that
            return curPt;
    }

    return MapPoint::Invalid();
}

MapPoint AIPlayerJH::FindPositionForBuildingAround(BuildingType type, const MapPoint& around)
{
    MapPoint foundPos = MapPoint::Invalid();
    switch(type)
    {
        case BLD_WOODCUTTER:
        {
            foundPos = FindBestPosition(around, AIResource::WOOD, BUILDING_SIZE[type], 20, 11);
            break;
        }
        case BLD_FORESTER:
            // ensure some distance to other foresters and an minimal amount of plantspace
            if(!construction->OtherUsualBuildingInRadius(around, 12, BLD_FORESTER) && (GetDensity(around, AIResource::PLANTSPACE, 7) > 15))
                foundPos = FindBestPosition(around, AIResource::WOOD, BUILDING_SIZE[type], 0, 11);
            break;
        case BLD_HUNTER:
        {
            // check if there are any animals in range
            if(HuntablesinRange(around, (2 << GetBldPlanner().GetNumBuildings(BLD_HUNTER))))
                foundPos = SimpleFindPosition(around, BUILDING_SIZE[type], 11);
            break;
        }
        case BLD_QUARRY:
        {
            unsigned numQuarries = GetBldPlanner().GetNumBuildings(BLD_QUARRY);
            foundPos = FindBestPosition(around, AIResource::STONES, BUILDING_SIZE[type], std::min(40u, 1 + numQuarries * 10), 11);
            if(foundPos.isValid() && !ValidStoneinRange(foundPos))
            {
                SetResourceMap(AIResource::STONES, foundPos, 0);
                foundPos = MapPoint::Invalid();
            }
            break;
        }
        case BLD_BARRACKS:
        case BLD_GUARDHOUSE:
        case BLD_WATCHTOWER:
        case BLD_FORTRESS: foundPos = FindBestPosition(around, AIResource::BORDERLAND, BUILDING_SIZE[type], 1, 11, true); break;
        case BLD_GOLDMINE: foundPos = FindBestPosition(around, AIResource::GOLD, BQ_MINE, 11, true); break;
        case BLD_COALMINE: foundPos = FindBestPosition(around, AIResource::COAL, BQ_MINE, 11, true); break;
        case BLD_IRONMINE: foundPos = FindBestPosition(around, AIResource::IRONORE, BQ_MINE, 11, true); break;
        case BLD_GRANITEMINE:
            if(!ggs.isEnabled(AddonId::INEXHAUSTIBLE_GRANITEMINES)) // inexhaustible granite mines do not require granite
                foundPos = FindBestPosition(around, AIResource::GRANITE, BQ_MINE, 11, true);
            else
                foundPos = SimpleFindPosition(around, BQ_MINE, 11);
            break;

        case BLD_FISHERY:
            foundPos = FindBestPosition(around, AIResource::FISH, BUILDING_SIZE[type], 11, true);
            if(foundPos.isValid() && !ValidFishInRange(foundPos))
            {
                SetResourceMap(AIResource::FISH, foundPos, 0);
                foundPos = MapPoint::Invalid();
            }
            break;
        case BLD_STOREHOUSE:
            if(!construction->OtherStoreInRadius(around, 15))
                foundPos = SimpleFindPosition(around, BUILDING_SIZE[type], 11);
            break;
        case BLD_HARBORBUILDING:
            foundPos = SimpleFindPosition(around, BUILDING_SIZE[type], 11);
            if(foundPos.isValid() && !HarborPosRelevant(GetWorld().GetHarborPointID(foundPos))) // bad harborspot detected DO NOT USE
                foundPos = MapPoint::Invalid();
            break;
        case BLD_SHIPYARD:
            foundPos = SimpleFindPosition(around, BUILDING_SIZE[type], 11);
            if(foundPos.isValid() && IsInvalidShipyardPosition(foundPos))
                foundPos = MapPoint::Invalid();
            break;
        case BLD_FARM: foundPos = FindBestPosition(around, AIResource::PLANTSPACE, BUILDING_SIZE[type], 85, 11, true); break;
        case BLD_CATAPULT:
            foundPos = SimpleFindPosition(around, BUILDING_SIZE[type], 11);
            if(foundPos.isValid() && BuildingNearby(foundPos, BLD_CATAPULT, 8))
                foundPos = MapPoint::Invalid();
            break;
        default: foundPos = SimpleFindPosition(around, BUILDING_SIZE[type], 11); break;
    }
    return foundPos;
}

unsigned AIPlayerJH::GetDensity(MapPoint pt, AIResource res, int radius)
{
    RTTR_Assert(pt.x < aiMap.GetWidth() && pt.y < aiMap.GetHeight());

    std::vector<MapPoint> pts = gwb.GetPointsInRadius(pt, radius);
    const unsigned all = pts.size();
    RTTR_Assert(all > 0);

    unsigned good = 0;
    BOOST_FOREACH(const MapPoint& curPt, pts)
    {
        if(aiMap[curPt].res == res)
            good++;
    }

    return (good * 100) / all;
}

void AIPlayerJH::HandleNewMilitaryBuilingOccupied(const MapPoint pt)
{
    // kill bad flags we find
    RemoveAllUnusedRoads(pt);
    bldPlanner->UpdateBuildingsWanted(*this);
    const nobMilitary* mil = gwb.GetSpecObj<nobMilitary>(pt);
    if(!mil)
        return;
    // if near border and gold disabled (by addon): enable it
    if(mil->GetFrontierDistance() > 0)
    {
        if(mil->IsGoldDisabled())
            aii.SetCoinsAllowed(pt, true);
    } else if((mil->GetBuildingType() == BLD_BARRACKS || mil->GetBuildingType() == BLD_GUARDHOUSE)
              && mil->GetBuildingType() != construction->GetBiggestAllowedMilBuilding())
    {
        if(!mil->IsGoldDisabled())
            aii.SetCoinsAllowed(pt, false);
    }

    AddBuildJob(BLD_HARBORBUILDING, pt);
    if(!IsInvalidShipyardPosition(pt))
        AddBuildJob(BLD_SHIPYARD, pt);
    if(SoldierAvailable())
    {
        AddBuildJob(construction->ChooseMilitaryBuilding(pt), pt);
        AddBuildJob(construction->ChooseMilitaryBuilding(pt), pt);
        AddBuildJob(construction->ChooseMilitaryBuilding(pt), pt);
    }

    // try to build one the following buildings around the new military building

    BuildingType bldToTest[] = {BLD_STOREHOUSE,  BLD_WOODCUTTER, BLD_QUARRY, BLD_GOLDMINE, BLD_COALMINE, BLD_IRONMINE,
                                BLD_GRANITEMINE, BLD_FISHERY,    BLD_FARM,   BLD_HUNTER,   BLD_FORESTER};
    unsigned bldToTestStartIdx = 0;
    // remove the storehouse from the building test list if we are close to another storehouse already
    BOOST_FOREACH(const nobBaseWarehouse* bldSite, aii.GetStorehouses())
    {
        if(gwb.CalcDistance(bldSite->GetPos(), pt) < 20)
        {
            bldToTestStartIdx = 1;
            break;
        }
    }
    // same is true for warehouses which are still in production
    BOOST_FOREACH(const noBuildingSite* bldSite, aii.GetBuildingSites())
    {
        if(BuildingProperties::IsWareHouse(bldSite->GetBuildingType()))
        {
            if(gwb.CalcDistance(bldSite->GetPos(), pt) < 20)
            {
                bldToTestStartIdx = 1;
                break;
            }
        }
    }

    for(unsigned i = bldToTestStartIdx; i < 11; ++i)
    {
        if(construction->Wanted(bldToTest[i]))
        {
            AddBuildJob(bldToTest[i], pt);
        }
    }
}

void AIPlayerJH::HandleBuilingDestroyed(MapPoint pt, BuildingType bld)
{
    switch(bld)
    {
        case BLD_CHARBURNER:
        case BLD_FARM: SetFarmedNodes(pt, false); break;
        case BLD_HARBORBUILDING:
        {
            // destroy all other buildings around the harborspot in range 2 so we can rebuild the harbor ...
            BOOST_FOREACH(const MapPoint curPt, gwb.GetPointsInRadius(pt, 2))
            {
                const noBaseBuilding* const bb = gwb.GetSpecObj<noBaseBuilding>(curPt);
                if(bb)
                    aii.DestroyBuilding(curPt);
                else
                {
                    const noBuildingSite* const bs = gwb.GetSpecObj<noBuildingSite>(curPt);
                    if(bs)
                        aii.DestroyFlag(gwb.GetNeighbour(curPt, Direction::SOUTHEAST));
                }
            }
            break;
        }
        default: break;
    }
}

void AIPlayerJH::HandleRoadConstructionComplete(MapPoint pt, Direction dir)
{
    // todo: detect "bad" roads and handle them
    const noFlag* flag = gwb.GetSpecObj<noFlag>(pt);
    // does the flag still exist?
    if(!flag)
        return;
    // does the roadsegment still exist?
    const RoadSegment* const roadSeg = flag->GetRoute(dir);
    if(!roadSeg || roadSeg->GetLength() < 4) // road too short to need flags
        return;
    // check if this road leads to a warehouseflag and if it does start setting flags from the warehouseflag else from the new flag
    // goal is to move roadsegments with a length of more than 2 away from the warehouse
    const noFlag& otherFlag = roadSeg->GetOtherFlag(*flag);
    MapPoint bldPos = gwb.GetNeighbour(otherFlag.GetPos(), Direction::NORTHWEST);
    if(aii.IsBuildingOnNode(bldPos, BLD_STOREHOUSE) || aii.IsBuildingOnNode(bldPos, BLD_HARBORBUILDING)
       || aii.IsBuildingOnNode(bldPos, BLD_HEADQUARTERS))
        construction->SetFlagsAlongRoad(otherFlag, roadSeg->GetOtherFlagDir(*flag) + 3u);
    else
    {
        // set flags on our new road starting from the new flag
        construction->SetFlagsAlongRoad(*flag, dir);
    }
}

void AIPlayerJH::HandleRoadConstructionFailed(const MapPoint pt, Direction dir)
{
    const noFlag* flag = gwb.GetSpecObj<noFlag>(pt);
    // does the flag still exist?
    if(!flag)
        return;
    // is it our flag?
    if(flag->GetPlayer() != playerId)
        return;
    // if it isnt a useless flag AND it has no current road connection then retry to build a road.
    if(RemoveUnusedRoad(*flag, INVALID_DIR, true, false))
        construction->AddConnectFlagJob(flag);
}

void AIPlayerJH::HandleMilitaryBuilingLost(const MapPoint pt)
{
    // For now, this is the same as losing land.
    HandleLostLand(pt);
}

void AIPlayerJH::HandleBuildingFinished(const MapPoint pt, BuildingType bld)
{
    switch(bld)
    {
        case BLD_HARBORBUILDING:
            UpdateNodesAround(pt, 8);    // todo: fix radius
            RemoveAllUnusedRoads(pt);    // repair & reconnect road system - required when a colony gets a new harbor by expedition
            aii.ChangeReserve(pt, 0, 1); // order 1 defender to stay in the harborbuilding

            // if there are positions free start an expedition!
            if(HarborPosRelevant(gwb.GetHarborPointID(pt), true))
            {
                aii.StartStopExpedition(pt, true);
            }
            break;

        case BLD_SHIPYARD: aii.SetShipYardMode(pt, true); break;

        case BLD_STOREHOUSE: break;
        case BLD_WOODCUTTER: AddBuildJob(BLD_SAWMILL, pt); break;
        default: break;
    }
}

void AIPlayerJH::HandleNewColonyFounded(const MapPoint pt)
{
    construction->AddConnectFlagJob(gwb.GetSpecObj<noFlag>(gwb.GetNeighbour(pt, Direction::SOUTHEAST)));
}

void AIPlayerJH::HandleExpedition(const noShip* ship)
{
    if(!ship->IsWaitingForExpeditionInstructions())
        return;
    if(ship->IsAbleToFoundColony())
        aii.FoundColony(ship);
    else
    {
        unsigned char start = rand() % ShipDirection::COUNT;
        for(unsigned char i = start; i < start + ShipDirection::COUNT; ++i)
        {
            if(aii.IsExplorationDirectionPossible(ship->GetPos(), ship->GetCurrentHarbor(), ShipDirection(i)))
            {
                aii.TravelToNextSpot(ShipDirection(i), ship);
                return;
            }
        }
        // no direction possible, sad, stop it
        aii.CancelExpedition(ship);
    }
}

void AIPlayerJH::HandleExpedition(const MapPoint pt)
{
    const noShip* ship = NULL;

    BOOST_FOREACH(const noBase* obj, gwb.GetFigures(pt))
    {
        if(obj->GetGOT() == GOT_SHIP)
        {
            if(static_cast<const noShip*>(obj)->GetPlayerId() == playerId)
            {
                if(static_cast<const noShip*>(obj)->IsWaitingForExpeditionInstructions())
                {
                    ship = static_cast<const noShip*>(obj);
                    break;
                }
            }
        }
    }
    if(ship)
    {
        HandleExpedition(ship);
    }
}

void AIPlayerJH::HandleTreeChopped(const MapPoint pt)
{
    // std::cout << "Tree chopped." << std::endl;

    aiMap[pt].reachable = true;

    UpdateNodesAround(pt, 3);

    int random = rand();

    if(random % 2 == 0)
        AddBuildJob(construction->ChooseMilitaryBuilding(pt), pt);
    else // if (random % 12 == 0)
        AddBuildJob(BLD_WOODCUTTER, pt);
}

void AIPlayerJH::HandleNoMoreResourcesReachable(const MapPoint pt, BuildingType bld)
{
    // Destroy old building (once)

    if(aii.IsObjectTypeOnNode(pt, NOP_BUILDING))
    {
        // keep 2 woodcutters for each forester even if they sometimes run out of trees
        if(bld == BLD_WOODCUTTER)
        {
            BOOST_FOREACH(const nobUsual* forester, aii.GetBuildings(BLD_FORESTER))
            {
                // is the forester somewhat close?
                if(gwb.CalcDistance(pt, forester->GetPos()) < 6)
                // then find it's 2 woodcutters
                {
                    unsigned maxdist = gwb.CalcDistance(pt, forester->GetPos());
                    int betterwoodcutters = 0;
                    BOOST_FOREACH(const nobUsual* woodcutter, aii.GetBuildings(BLD_WOODCUTTER))
                    {
                        // dont count the woodcutter in question
                        if(pt == woodcutter->GetPos())
                            continue;
                        // closer or equally close to forester than woodcutter in question?
                        if(gwb.CalcDistance(woodcutter->GetPos(), forester->GetPos()) <= maxdist)
                        {
                            betterwoodcutters++;
                            if(betterwoodcutters >= 2)
                                break;
                        }
                    }
                    // couldnt find 2 closer woodcutter -> keep it alive
                    if(betterwoodcutters < 2)
                        return;
                }
            }
        }
        aii.DestroyBuilding(pt);
        if(bld == BLD_FISHERY) // fishery cant find fish? set fish value at location to 0 so we dont have to calculate the value for this
                               // location again
            SetResourceMap(AIResource::FISH, pt, 0);
    } else
        return;
    UpdateNodesAround(pt, 11); // todo: fix radius
    RemoveUnusedRoad(*gwb.GetSpecObj<noFlag>(gwb.GetNeighbour(pt, Direction::SOUTHEAST)), 1, true);

    // try to expand, maybe res blocked a passage
    AddBuildJob(construction->ChooseMilitaryBuilding(pt), pt);
    AddBuildJob(construction->ChooseMilitaryBuilding(pt), pt);

    // and try to rebuild the same building
    if(bld != BLD_HUNTER)
        AddBuildJob(bld, pt);

    // farm is always good!
    AddBuildJob(BLD_FARM, pt);
}

void AIPlayerJH::HandleShipBuilt(const MapPoint pt)
{
    // Stop building ships if reached a maximum (TODO: make variable)
    const std::list<nobUsual*>& shipyards = aii.GetBuildings(BLD_SHIPYARD);
    bool wantMoreShips;
    unsigned numRelevantSeas = GetNumAIRelevantSeaIds();
    if(numRelevantSeas == 0)
        wantMoreShips = false;
    else if(numRelevantSeas == 1)
        wantMoreShips = aii.GetNumShips() <= gwb.GetNumHarborPoints();
    else
    {
        unsigned wantedShipCt = std::min<unsigned>(7, 3 * shipyards.size());
        wantMoreShips = aii.GetNumShips() < wantedShipCt;
    }
    if(!wantMoreShips)
    {
        // Find shipyard from this ship by getting the closest one. Max distance of <12 nodes
        unsigned mindist = 12;
        const nobUsual* creatingShipyard = NULL;
        BOOST_FOREACH(const nobUsual* shipyard, shipyards)
        {
            unsigned distance = gwb.CalcDistance(shipyard->GetPos(), pt);
            if(distance < mindist)
            {
                mindist = distance;
                creatingShipyard = shipyard;
            }
        }
        if(creatingShipyard) // might have been destroyed by now
            aii.SetProductionEnabled(creatingShipyard->GetPos(), false);
    }
}

void AIPlayerJH::HandleBorderChanged(const MapPoint pt)
{
    UpdateNodesAround(pt, 11); // todo: fix radius

    const nobMilitary* mil = gwb.GetSpecObj<nobMilitary>(pt);
    if(mil)
    {
        if(mil->GetFrontierDistance() != 0 && mil->IsGoldDisabled())
        {
            aii.SetCoinsAllowed(pt, true);
        }
        if(mil->GetBuildingType() != construction->GetBiggestAllowedMilBuilding())
        {
            AddBuildJob(construction->ChooseMilitaryBuilding(pt), pt);
        }
    }
}

void AIPlayerJH::HandleLostLand(const MapPoint pt)
{
    if(aii.GetStorehouses().empty()) // check if we have a storehouse left - if we dont have one trying to find a path to one will crash
    {
        return;
    }
    RemoveAllUnusedRoads(pt);
}

void AIPlayerJH::MilUpgradeOptim()
{
    // do we have a upgrade building?
    int upb = UpdateUpgradeBuilding();
    int count = 0;
    const std::list<nobMilitary*>& militaryBuildings = aii.GetMilitaryBuildings();
    BOOST_FOREACH(const nobMilitary* milBld, militaryBuildings)
    {
        if(count != upb) // not upgrade building
        {
            if(upb >= 0) // we do have an upgrade building
            {
                if(!milBld->IsGoldDisabled()) // deactivate gold for all other buildings
                {
                    aii.SetCoinsAllowed(milBld->GetPos(), false);
                }
                if(milBld->GetFrontierDistance() == 0
                   && (((unsigned)count + GetNumPlannedConnectedInlandMilitaryBlds())
                       < militaryBuildings.size())) // send out troops until 1 private is left, then cancel road
                {
                    if(milBld->GetNumTroops() > 1) // more than 1 soldier remaining? -> send out order
                    {
                        aii.SendSoldiersHome(milBld->GetPos());
                    } else if(!milBld
                                 ->IsNewBuilt()) // 0-1 soldier remains and the building has had at least 1 soldier at some point and the
                                                 // building is not new on the list-> cancel road (and fix roadsystem if necessary)
                    {
                        RemoveUnusedRoad(*milBld->GetFlag(), 1, true, true, true);
                    }
                } else if(milBld->GetFrontierDistance() >= 1) // frontier building - connect to road system
                {
                    construction->AddConnectFlagJob(milBld->GetFlag());
                }
            } else // no upgrade building? -> activate gold for frontier buildings
            {
                if(milBld->IsGoldDisabled() && milBld->GetFrontierDistance() > 0)
                {
                    aii.SetCoinsAllowed(milBld->GetPos(), true);
                }
            }
        } else // upgrade building
        {
            if(!construction->IsConnectedToRoadSystem(milBld->GetFlag()))
            {
                construction->AddConnectFlagJob(milBld->GetFlag());
                continue;
            }
            if(milBld->IsGoldDisabled()) // activate gold
            {
                aii.SetCoinsAllowed(milBld->GetPos(), true);
            }
            if(milBld->HasMaxRankSoldier()) // has max rank soldier? send it/them out!
                aii.SendSoldiersHome(milBld->GetPos());
            if(SoldierAvailable(0) && milBld->GetNumTroops() < milBld->GetMaxTroopsCt()) // building not full and privates in a warehouse?
                aii.OrderNewSoldiers(milBld->GetPos());                                  // order new!
        }
        count++;
    }
}

void AIPlayerJH::Chat(const std::string& message)
{
    GameMessage_Chat chat = GameMessage_Chat(playerId, CD_ALL, message);
    GAMESERVER.AIChat(chat);
}

bool AIPlayerJH::HasFrontierBuildings()
{
    BOOST_FOREACH(const nobMilitary* milBld, aii.GetMilitaryBuildings())
    {
        if(milBld->GetFrontierDistance() > 0)
            return true;
    }
    return false;
}

void AIPlayerJH::CheckExpeditions()
{
    const std::list<nobHarborBuilding*>& harbors = aii.GetHarbors();
    BOOST_FOREACH(const nobHarborBuilding* harbor, harbors)
    {
        bool isHarborRelevant = HarborPosRelevant(harbor->GetHarborPosID(), true);
        if(harbor->IsExpeditionActive()
           != isHarborRelevant) // harbor is collecting for expedition and shouldnt OR not collecting and should -> toggle expedition
        {
            aii.StartStopExpedition(harbor->GetPos(), isHarborRelevant);
        }
    }
    // find lost expedition ships - ai should get a notice and catch them all but just in case some fell through the system
    const std::vector<noShip*>& ships = aii.GetShips();
    BOOST_FOREACH(const noShip* harbor, ships)
    {
        if(harbor->IsWaitingForExpeditionInstructions())
            HandleExpedition(harbor);
    }
}

void AIPlayerJH::CheckForester()
{
    const std::list<nobUsual*>& foresters = aii.GetBuildings(BLD_FORESTER);
    if(!foresters.empty() && foresters.size() < 2 && aii.GetMilitaryBuildings().size() < 3 && aii.GetBuildingSites().size() < 3)
    // stop the forester
    {
        if(!(*foresters.begin())->IsProductionDisabled())
            aii.SetProductionEnabled(foresters.front()->GetPos(), false);
    } else // activate the forester
    {
        if(!foresters.empty() && (*foresters.begin())->IsProductionDisabled())
            aii.SetProductionEnabled(foresters.front()->GetPos(), true);
    }
}

void AIPlayerJH::CheckGranitMine()
{
    // stop production in granite mines when the ai has many stones (100+ and at least 15 for each warehouse)
    bool enableProduction = AmountInStorage(GD_STONES) < 100 || AmountInStorage(GD_STONES) < 15 * aii.GetStorehouses().size();
    BOOST_FOREACH(const nobUsual* mine, aii.GetBuildings(BLD_GRANITEMINE))
    {
        // !productionDisabled != enableProduction
        if(mine->IsProductionDisabled() == enableProduction)
            aii.SetProductionEnabled(mine->GetPos(), enableProduction);
    }
}

void AIPlayerJH::TryToAttack()
{
    unsigned hq_or_harbor_without_soldiers = 0;
    std::vector<const nobBaseMilitary*> potentialTargets;

    // use own military buildings (except inland buildings) to search for enemy military buildings
    const std::list<nobMilitary*>& militaryBuildings = aii.GetMilitaryBuildings();
    const unsigned numMilBlds = militaryBuildings.size();
    // when the ai has many buildings the ai will not check the complete list every time
    BOOST_CONSTEXPR_OR_CONST unsigned limit = 40;
    BOOST_FOREACH(const nobMilitary* milBld, militaryBuildings)
    {
        // We skip the current building with a probability of limit/numMilBlds
        // -> For twice the number of blds as the limit we will most likely skip every 2nd building
        // This way we check roughly (at most) limit buildings but avoid any preference for one building over an other
        if(rand() % numMilBlds > limit)
            continue;

        if(milBld->GetFrontierDistance() == 0) // inland building? -> skip it
            continue;

        // get nearby enemy buildings and store in set of potential attacking targets
        MapPoint src = milBld->GetPos();

        sortedMilitaryBlds buildings = gwb.LookForMilitaryBuildings(src, 2);
        BOOST_FOREACH(const nobBaseMilitary* target, buildings)
        {
            if(helpers::contains(potentialTargets, target))
                continue;
            if(target->GetGOT() == GOT_NOB_MILITARY && static_cast<const nobMilitary*>(target)->IsNewBuilt())
                continue;
            MapPoint dest = target->GetPos();
            if(gwb.CalcDistance(src, dest) < BASE_ATTACKING_DISTANCE && aii.IsPlayerAttackable(target->GetPlayer()) && aii.IsVisible(dest))
            {
                if(target->GetGOT() != GOT_NOB_MILITARY && !target->DefendersAvailable())
                {
                    // headquarter or harbor without any troops :)
                    hq_or_harbor_without_soldiers++;
                    potentialTargets.insert(potentialTargets.begin(), target);
                } else
                    potentialTargets.push_back(target);
            }
        }
    }

    // shuffle everything but headquarters and harbors without any troops in them
    std::random_shuffle(potentialTargets.begin() + hq_or_harbor_without_soldiers, potentialTargets.end());

    // check for each potential attacking target the number of available attacking soldiers
    BOOST_FOREACH(const nobBaseMilitary* target, potentialTargets)
    {
        const MapPoint dest = target->GetPos();

        unsigned attackersCount = 0;
        unsigned attackersStrength = 0;

        // ask each of nearby own military buildings for soldiers to contribute to the potential attack
        sortedMilitaryBlds myBuildings = gwb.LookForMilitaryBuildings(dest, 2);
        BOOST_FOREACH(const nobBaseMilitary* otherMilBld, myBuildings)
        {
            if(otherMilBld->GetPlayer() == playerId)
            {
                const nobMilitary* myMil = dynamic_cast<const nobMilitary*>(otherMilBld);
                if(!myMil || myMil->IsUnderAttack())
                    continue;

                unsigned newAttackers;
                attackersStrength += myMil->GetSoldiersStrengthForAttack(dest, newAttackers);
                attackersCount += newAttackers;
            }
        }

        if(attackersCount == 0)
            continue;

        if((level == AI::HARD) && (target->GetGOT() == GOT_NOB_MILITARY))
        {
            const nobMilitary* enemyTarget = static_cast<const nobMilitary*>(target);
            if(attackersStrength <= enemyTarget->GetSoldiersStrength() || enemyTarget->GetNumTroops() == 0)
                continue;
        }

        aii.Attack(dest, attackersCount, true);
        return;
    }
}

void AIPlayerJH::TrySeaAttack()
{
    if(aii.GetNumShips() < 1)
        return;
    if(aii.GetHarbors().empty())
        return;
    std::vector<unsigned short> seaidswithattackers;
    std::vector<unsigned> attackersatseaid;
    std::vector<int> invalidseas;
    std::deque<const nobBaseMilitary*> potentialTargets;
    std::deque<const nobBaseMilitary*> undefendedTargets;
    std::vector<int> searcharoundharborspots;
    // all seaids with at least 1 ship count available attackers for later checks
    BOOST_FOREACH(const noShip* ship, aii.GetShips())
    {
        // sea id not already listed as valid or invalid?
        if(!helpers::contains(seaidswithattackers, ship->GetSeaID()) && !helpers::contains(invalidseas, ship->GetSeaID()))
        {
            unsigned attackercount = gwb.GetNumSoldiersForSeaAttackAtSea(playerId, ship->GetSeaID(), false);
            if(attackercount) // got attackers at this sea id? -> add to valid list
            {
                seaidswithattackers.push_back(ship->GetSeaID());
                attackersatseaid.push_back(attackercount);
            } else // not listed but no attackers? ->invalid
            {
                invalidseas.push_back(ship->GetSeaID());
            }
        }
    }
    if(seaidswithattackers.empty()) // no sea ids with attackers? skip the rest
        return;
    /*else
    {
        for(unsigned i=0;i<seaidswithattackers.size();i++)
            LOG.write(("attackers at sea ids for player %i, sea id %i, count %i \n",playerId, seaidswithattackers[i], attackersatseaid[i]);
    }*/
    // first check all harbors there might be some undefended ones - start at 1 to skip the harbor dummy
    for(unsigned i = 1; i < gwb.GetNumHarborPoints(); i++)
    {
        const nobHarborBuilding* hb;
        if((hb = gwb.GetSpecObj<nobHarborBuilding>(gwb.GetHarborPoint(i))))
        {
            if(aii.IsVisible(hb->GetPos()))
            {
                if(aii.IsPlayerAttackable(hb->GetPlayer()))
                {
                    // attackers for this building?
                    const std::vector<unsigned short> testseaidswithattackers =
                      gwb.GetFilteredSeaIDsForAttack(gwb.GetHarborPoint(i), seaidswithattackers, playerId);
                    if(!testseaidswithattackers.empty()) // harbor can be attacked?
                    {
                        if(!hb->DefendersAvailable()) // no defenders?
                            undefendedTargets.push_back(hb);
                        else // todo: maybe only attack this when there is a fair win chance for the attackers?
                            potentialTargets.push_back(hb);
                        // LOG.write(("found a defended harbor we can attack at %i,%i \n",hb->GetPos());
                    }
                } else // cant attack player owning the harbor -> add to list
                {
                    searcharoundharborspots.push_back(i);
                }
            }
            // else: not visible for player no need to look any further here
        } else // no harbor -> add to list
        {
            searcharoundharborspots.push_back(i);
            // LOG.write(("found an unused harborspot we have to look around of at %i,%i
            // \n",gwb.GetHarborPoint(i).x,gwb.GetHarborPoint(i).y);
        }
    }
    // any undefendedTargets? -> pick one by random
    if(!undefendedTargets.empty())
    {
        std::random_shuffle(undefendedTargets.begin(), undefendedTargets.end());
        BOOST_FOREACH(const nobBaseMilitary* targetMilBld, undefendedTargets)
        {
            std::vector<GameWorldBase::PotentialSeaAttacker> attackers = gwb.GetSoldiersForSeaAttack(playerId, targetMilBld->GetPos());
            if(!attackers.empty()) // try to attack it!
            {
                aii.SeaAttack(targetMilBld->GetPos(), 1, true);
                return;
            }
        }
    }
    // add all military buildings around still valid harborspots (unused or used by ally)
    unsigned limit = 15;
    unsigned skip = 0;
    if(searcharoundharborspots.size() > 15)
        skip = max<int>(rand() % (searcharoundharborspots.size() / 15 + 1) * 15, 1) - 1;
    for(unsigned i = skip; i < searcharoundharborspots.size() && limit > 0; i++)
    {
        limit--;
        // now add all military buildings around the harborspot to our list of potential targets
        sortedMilitaryBlds buildings = gwb.LookForMilitaryBuildings(gwb.GetHarborPoint(searcharoundharborspots[i]), 2);
        BOOST_FOREACH(const nobBaseMilitary* milBld, buildings)
        {
            if(aii.IsPlayerAttackable(milBld->GetPlayer()) && aii.IsVisible(milBld->GetPos()))
            {
                const nobMilitary* enemyTarget = dynamic_cast<const nobMilitary*>((milBld));

                if(enemyTarget && enemyTarget->IsNewBuilt())
                    continue;
                if((milBld->GetGOT() != GOT_NOB_MILITARY)
                   && (!milBld->DefendersAvailable())) // undefended headquarter(or unlikely as it is a harbor...) - priority list!
                {
                    const std::vector<unsigned short> testseaidswithattackers =
                      gwb.GetFilteredSeaIDsForAttack(milBld->GetPos(), seaidswithattackers, playerId);
                    if(!testseaidswithattackers.empty())
                    {
                        undefendedTargets.push_back(milBld);
                    }  // else - no attackers - do nothing
                } else // normal target - check is done after random shuffle so we dont have to check every possible target and instead only
                       // enough to get 1 good one
                {
                    potentialTargets.push_back(milBld);
                }
            } // not attackable or no vision of region - do nothing
        }
    }
    // now we have a deque full of available and maybe undefended targets that are available for attack -> shuffle and attack the first one
    // we can attack("should" be the first we check...)  any undefendedTargets? -> pick one by random
    if(!undefendedTargets.empty())
    {
        std::random_shuffle(undefendedTargets.begin(), undefendedTargets.end());
        BOOST_FOREACH(const nobBaseMilitary* targetMilBld, undefendedTargets)
        {
            std::vector<GameWorldBase::PotentialSeaAttacker> attackers = gwb.GetSoldiersForSeaAttack(playerId, targetMilBld->GetPos());
            if(!attackers.empty()) // try to attack it!
            {
                aii.SeaAttack(targetMilBld->GetPos(), 1, true);
                return;
            }
        }
    }
    std::random_shuffle(potentialTargets.begin(), potentialTargets.end());
    BOOST_FOREACH(const nobBaseMilitary* ship, potentialTargets)
    {
        // TODO: decide if it is worth attacking the target and not just "possible"
        // test only if we should have attackers from one of our valid sea ids
        const std::vector<unsigned short> testseaidswithattackers =
          gwb.GetFilteredSeaIDsForAttack(ship->GetPos(), seaidswithattackers, playerId);
        if(!testseaidswithattackers.empty()) // only do the final check if it will probably be a good result
        {
            std::vector<GameWorldBase::PotentialSeaAttacker> attackers =
              gwb.GetSoldiersForSeaAttack(playerId, ship->GetPos()); // now get a final list of attackers and attack it
            if(!attackers.empty())
            {
                aii.SeaAttack(ship->GetPos(), attackers.size(), true);
                return;
            }
        }
    }
}

void AIPlayerJH::RecalcGround(const MapPoint buildingPos, std::vector<Direction>& route_road)
{
    MapPoint pt = buildingPos;

    // building itself
    RecalcBQAround(pt);
    if(aiMap[pt].res == AIResource::PLANTSPACE)
    {
        resourceMaps[static_cast<unsigned>(AIResource::PLANTSPACE)].Change(pt, -1);
        aiMap[pt].res = AIResource::NOTHING;
    }

    // flag of building
    pt = gwb.GetNeighbour(pt, Direction::SOUTHEAST);
    RecalcBQAround(pt);
    if(aiMap[pt].res == AIResource::PLANTSPACE)
    {
        resourceMaps[static_cast<unsigned>(AIResource::PLANTSPACE)].Change(pt, -1);
        aiMap[pt].res = AIResource::NOTHING;
    }

    // along the road
    for(unsigned i = 0; i < route_road.size(); ++i)
    {
        pt = gwb.GetNeighbour(pt, route_road[i]);
        RecalcBQAround(pt);
        // Auch Plantspace entsprechend anpassen:
        if(aiMap[pt].res == AIResource::PLANTSPACE)
        {
            resourceMaps[static_cast<unsigned>(AIResource::PLANTSPACE)].Change(pt, -1);
            aiMap[pt].res = AIResource::NOTHING;
        }
    }
}

void AIPlayerJH::SaveResourceMapsToFile()
{
    for(unsigned res = 0; res < NUM_AIRESOURCES; ++res)
    {
        std::stringstream ss;
        ss << "resmap-" << res << ".log";
        bfs::ofstream file(ss.str());
        for(unsigned y = 0; y < aiMap.GetHeight(); ++y)
        {
            if(y % 2 == 1)
                file << "  ";
            for(unsigned x = 0; x < aiMap.GetWidth(); ++x)
                file << resourceMaps[res][MapPoint(x, y)] << "   ";
            file << "\n";
        }
    }
}

int AIPlayerJH::GetResMapValue(const MapPoint pt, AIResource res) const
{
    return GetResMap(res)[pt];
}

const AIResourceMap& AIPlayerJH::GetResMap(AIResource res) const
{
    return resourceMaps[boost::underlying_cast<unsigned>(res)];
}

void AIPlayerJH::SendAIEvent(AIEvent::Base* ev)
{
    eventManager.AddAIEvent(ev);
}

bool AIPlayerJH::IsFlagPartofCircle(const noFlag& startFlag, unsigned maxlen, const noFlag& curFlag, unsigned char excludeDir, bool init,
                                    std::vector<MapPoint> oldFlags)
{
    if(!init && &startFlag == &curFlag)
        return true;
    if(maxlen < 1)
        return false;
    bool partofcircle = false;
    unsigned iTestDir = 0;
    while(iTestDir < 6 && !partofcircle)
    {
        Direction testDir = Direction::fromInt(iTestDir);
        if(iTestDir == excludeDir)
        {
            iTestDir++;
            continue;
        }
        if(iTestDir == 1
           && (aii.IsObjectTypeOnNode(gwb.GetNeighbour(curFlag.GetPos(), Direction::NORTHWEST), NOP_BUILDING)
               || aii.IsObjectTypeOnNode(gwb.GetNeighbour(curFlag.GetPos(), Direction::NORTHWEST), NOP_BUILDINGSITE)))
        {
            iTestDir++;
            continue;
        }
        RoadSegment* route = curFlag.GetRoute(testDir);
        if(route)
        {
            const noFlag& flag = route->GetOtherFlag(curFlag);
            bool alreadyinlist = helpers::contains(oldFlags, flag.GetPos());
            if(!alreadyinlist)
            {
                oldFlags.push_back(flag.GetPos());
                Direction revDir = route->GetOtherFlagDir(curFlag) + 3u;
                partofcircle = IsFlagPartofCircle(startFlag, maxlen - 1, flag, revDir.toUInt(), false, oldFlags);
            }
        }
        iTestDir++;
    }
    return partofcircle;
}

void AIPlayerJH::RemoveAllUnusedRoads(const MapPoint pt)
{
    std::vector<const noFlag*> flags = construction->FindFlags(pt, 25);
    // Jede Flagge testen...
    std::vector<const noFlag*> reconnectflags;
    for(unsigned i = 0; i < flags.size(); ++i)
    {
        if(RemoveUnusedRoad(*flags[i], 255, true, false))
            reconnectflags.push_back(flags[i]);
    }
    UpdateNodesAround(pt, 25);
    BOOST_FOREACH(const noFlag* flag, reconnectflags)
        construction->AddConnectFlagJob(flag);
}

void AIPlayerJH::CheckForUnconnectedBuildingSites()
{
    if(construction->GetConnectJobNum() > 0 || construction->GetBuildJobNum() > 0)
        return;
    BOOST_FOREACH(noBuildingSite* bldSite, player.GetBuildingRegister().GetBuildingSites()) //-V807
    {
        noFlag* flag = bldSite->GetFlag();
        bool foundRoute = false;
        for(unsigned dir = 0; dir < Direction::COUNT; ++dir)
        {
            if(dir == Direction::NORTHWEST)
                continue;
            if(flag->GetRoute(Direction::fromInt(dir)))
            {
                foundRoute = true;
                break;
            }
        }
        if(!foundRoute)
            construction->AddConnectFlagJob(flag);
    }
}

bool AIPlayerJH::RemoveUnusedRoad(const noFlag& startFlag, unsigned char excludeDir /*= 0xFF*/, bool firstflag /*= true*/,
                                  bool allowcircle /*= true*/, bool keepstartflag /*= false*/)
{
    unsigned char foundDir = INVALID_DIR;
    unsigned char foundDir2 = INVALID_DIR;
    unsigned char finds = 0;
    // Count roads from this flag...
    for(unsigned char dir = 0; dir < Direction::COUNT; ++dir)
    {
        if(dir == excludeDir)
            continue;
        if(dir == Direction::NORTHWEST
           && (aii.IsObjectTypeOnNode(gwb.GetNeighbour(startFlag.GetPos(), Direction::NORTHWEST), NOP_BUILDING)
               || aii.IsObjectTypeOnNode(gwb.GetNeighbour(startFlag.GetPos(), Direction::NORTHWEST), NOP_BUILDINGSITE)))
        {
            // the flag belongs to a building - update the pathing map around us and try to reconnect it (if we cant reconnect it -> burn
            // it(burning takes place at the pathfinding job))
            return true;
        }
        if(startFlag.GetRoute(Direction::fromInt(dir)))
        {
            finds++;
            if(finds == 1)
                foundDir = dir;
            else if(finds == 2)
                foundDir2 = dir;
        }
    }
    // if we found more than 1 road -> the flag is still in use.
    if(finds > 2)
        return false;
    else if(finds == 2)
    {
        if(allowcircle)
        {
            std::vector<MapPoint> flagcheck;
            if(!IsFlagPartofCircle(startFlag, 10, startFlag, 7, true, flagcheck))
                return false;
            if(!firstflag)
                return false;
        } else
            return false;
    }

    // kill the flag
    if(keepstartflag)
    {
        if(foundDir != INVALID_DIR)
            aii.DestroyRoad(startFlag.GetPos(), Direction::fromInt(foundDir));
    } else
        aii.DestroyFlag(&startFlag);

    // nothing found?
    if(foundDir == INVALID_DIR)
        return false;
    // at least 1 road exists
    Direction revDir1 = startFlag.GetRoute(Direction::fromInt(foundDir))->GetOtherFlagDir(startFlag) + 3u;
    RemoveUnusedRoad(startFlag.GetRoute(Direction::fromInt(foundDir))->GetOtherFlag(startFlag), revDir1.toUInt(), false);
    // 2 roads exist
    if(foundDir2 != 0xFF)
    {
        Direction revDir2 = startFlag.GetRoute(Direction::fromInt(foundDir2))->GetOtherFlagDir(startFlag) + 3u;
        RemoveUnusedRoad(startFlag.GetRoute(Direction::fromInt(foundDir2))->GetOtherFlag(startFlag), revDir2.toUInt(), false);
    }
    return false;
}

unsigned AIPlayerJH::SoldierAvailable(int rank)
{
    unsigned freeSoldiers = 0;
    BOOST_FOREACH(const nobBaseWarehouse* wh, aii.GetStorehouses())
    {
        if(rank < 0 || rank > 4)
        {
            const Inventory& inventory = wh->GetInventory();
            freeSoldiers += (inventory.people[JOB_PRIVATE] + inventory.people[JOB_PRIVATEFIRSTCLASS] + inventory.people[JOB_SERGEANT]
                             + inventory.people[JOB_OFFICER] + inventory.people[JOB_GENERAL]);
        } else
            freeSoldiers += (wh->GetInventory().people[rank + 21]);
    }
    return freeSoldiers;
}

bool AIPlayerJH::HuntablesinRange(const MapPoint pt, unsigned min)
{
    // check first if no other hunter(or hunter buildingsite) is nearby
    if(BuildingNearby(pt, BLD_HUNTER, 15))
        return false;
    unsigned maxrange = 25;
    unsigned short fx, fy, lx, ly;
    const unsigned short SQUARE_SIZE = 19;
    unsigned huntablecount = 0;
    if(pt.x > SQUARE_SIZE)
        fx = pt.x - SQUARE_SIZE;
    else
        fx = 0;
    if(pt.y > SQUARE_SIZE)
        fy = pt.y - SQUARE_SIZE;
    else
        fy = 0;
    if(pt.x + SQUARE_SIZE < gwb.GetWidth())
        lx = pt.x + SQUARE_SIZE;
    else
        lx = gwb.GetWidth() - 1;
    if(pt.y + SQUARE_SIZE < gwb.GetHeight())
        ly = pt.y + SQUARE_SIZE;
    else
        ly = gwb.GetHeight() - 1;
    // Durchgehen und nach Tieren suchen
    for(MapPoint p2(0, fy); p2.y <= ly; ++p2.y)
    {
        for(p2.x = fx; p2.x <= lx; ++p2.x)
        {
            // Gibts hier was bewegliches?
            if(gwb.GetFigures(p2).empty())
                continue;
            const std::list<noBase*>& figures = gwb.GetFigures(p2);
            // Dann nach Tieren suchen
            BOOST_FOREACH(const noBase* fig, figures)
            {
                if(fig->GetType() == NOP_ANIMAL)
                {
                    // Ist das Tier überhaupt zum Jagen geeignet?
                    if(!static_cast<const noAnimal*>(fig)->CanHunted())
                        continue;
                    // Und komme ich hin?
                    if(gwb.FindHumanPath(pt, static_cast<const noAnimal*>(fig)->GetPos(), maxrange) != 0xFF)
                    // Dann nehmen wir es
                    {
                        if(++huntablecount >= min)
                            return true;
                    }
                }
            }
        }
    }
    return false;
}

void AIPlayerJH::InitStoreAndMilitarylists()
{
    BOOST_FOREACH(const nobUsual* farm, aii.GetBuildings(BLD_FARM))
    {
        SetFarmedNodes(farm->GetPos(), true);
    }
    BOOST_FOREACH(const nobUsual* charburner, aii.GetBuildings(BLD_CHARBURNER))
    {
        SetFarmedNodes(charburner->GetPos(), true);
    }
    // find the upgradebuilding
    UpdateUpgradeBuilding();
}
int AIPlayerJH::UpdateUpgradeBuilding()
{
    std::vector<const nobMilitary*> backup;
    if(!aii.GetStorehouses().empty())
    {
        unsigned count = 0;
        BOOST_FOREACH(const nobMilitary* milBld, aii.GetMilitaryBuildings())
        {
            // inland building, tower or fortress
            BuildingType bld = milBld->GetBuildingType();
            if((bld == BLD_WATCHTOWER || bld == BLD_FORTRESS) && milBld->GetFrontierDistance() < 1)
            {
                if(construction->IsConnectedToRoadSystem(milBld->GetFlag()))
                {
                    // LOG.write(("UpdateUpgradeBuilding at %i,%i for player %i (listslot %i) \n",itObj->GetX(), itObj->GetY(), playerId,
                    // count);
                    UpgradeBldPos = milBld->GetPos();
                    return count;
                }
                backup.push_back(milBld);
            }
            count++;
        }
    }
    // no valid upgrade building yet - try to reconnect correctly flagged buildings
    BOOST_FOREACH(const nobMilitary* milBld, backup)
    {
        construction->AddConnectFlagJob(milBld->GetFlag());
    }
    UpgradeBldPos = MapPoint::Invalid();
    return -1;
}
// set default start values for the ai for distribution & military settings
void AIPlayerJH::InitDistribution()
{
    // set good distribution settings
    Distributions goodSettings;
    goodSettings[0] = 10; // food granite
    goodSettings[1] = 10; // food coal
    goodSettings[2] = 10; // food iron
    goodSettings[3] = 10; // food gold

    goodSettings[4] = 10; // grain mill
    goodSettings[5] = 10; // grain pigfarm
    goodSettings[6] = 10; // grain donkeybreeder
    goodSettings[7] = 10; // grain brewery
    goodSettings[8] = 10; // grain charburner

    goodSettings[9] = 10;  // iron armory
    goodSettings[10] = 10; // iron metalworks

    goodSettings[11] = 10; // coal armory
    goodSettings[12] = 10; // coal ironsmelter
    goodSettings[13] = 10; // coal mint

    goodSettings[14] = 10; // wood sawmill
    goodSettings[15] = 10; // wood charburner

    goodSettings[16] = 10; // boards new buildings
    goodSettings[17] = 4;  // boards metalworks
    goodSettings[18] = 2;  // boards shipyard

    goodSettings[19] = 10; // water bakery
    goodSettings[20] = 10; // water brewery
    goodSettings[21] = 10; // water pigfarm
    goodSettings[22] = 10; // water donkeybreeder
    aii.ChangeDistribution(goodSettings);
}

bool AIPlayerJH::ValidTreeinRange(const MapPoint pt)
{
    unsigned max_radius = 6;
    for(MapCoord tx = gwb.GetXA(pt, Direction::WEST), r = 1; r <= max_radius; tx = gwb.GetXA(MapPoint(tx, pt.y), Direction::WEST), ++r)
    {
        MapPoint t2(tx, pt.y);
        for(unsigned i = 2; i < 8; ++i)
        {
            for(MapCoord r2 = 0; r2 < r; t2 = gwb.GetNeighbour(t2, Direction(i)), ++r2)
            {
                // point has tree & path is available?
                if(gwb.GetNO(t2)->GetType() == NOP_TREE)
                {
                    // not already getting cut down or a freaking pineapple thingy?
                    if(!gwb.GetNode(t2).reserved && gwb.GetSpecObj<noTree>(t2)->ProducesWood())
                    {
                        if(gwb.FindHumanPath(pt, t2, 20) != 0xFF)
                            return true;
                    }
                }
            }
        }
    }
    return false;
}

bool AIPlayerJH::ValidStoneinRange(const MapPoint pt)
{
    unsigned max_radius = 8;
    for(MapCoord tx = gwb.GetXA(pt, Direction::WEST), r = 1; r <= max_radius; tx = gwb.GetXA(MapPoint(tx, pt.y), Direction::WEST), ++r)
    {
        MapPoint t2(tx, pt.y);
        for(unsigned i = 2; i < 8; ++i)
        {
            for(MapCoord r2 = 0; r2 < r; t2 = gwb.GetNeighbour(t2, Direction(i)), ++r2)
            {
                // point has tree & path is available?
                if(gwb.GetNO(t2)->GetType() == NOP_GRANITE)
                {
                    if(gwb.FindHumanPath(pt, t2, 20) != 0xFF)
                        return true;
                }
            }
        }
    }
    return false;
}

void AIPlayerJH::ExecuteLuaConstructionOrder(const MapPoint pt, BuildingType bt, bool forced)
{
    if(!aii.CanBuildBuildingtype(bt)) // not allowed to build this buildingtype? -> do nothing!
        return;
    if(forced) // fixed location - just a direct gamecommand to build buildingtype at location (no checks if this is a valid & good location
               // from the ai)
    {
        aii.SetBuildingSite(pt, bt);
        BuildJob* j = new BuildJob(*this, bt, pt);
        j->SetStatus(JOB_EXECUTING_ROAD1);
        j->SetTarget(pt);
        construction->AddBuildJob(j, true); // connects the buildingsite to roadsystem
    } else
    {
        if(construction->Wanted(bt))
        {
            construction->AddBuildJob(new BuildJob(*this, bt, pt), true); // add build job to the front of the list
        }
    }
}

bool AIPlayerJH::BuildingNearby(const MapPoint pt, BuildingType bldType, unsigned min)
{
    // assert not a military building
    RTTR_Assert(static_cast<unsigned>(bldType) >= FIRST_USUAL_BUILDING);
    BOOST_FOREACH(const nobUsual* bld, aii.GetBuildings(bldType))
    {
        if(gwb.CalcDistance(pt, bld->GetPos()) < min)
            return true;
    }
    BOOST_FOREACH(const noBuildingSite* bldSite, aii.GetBuildingSites())
    {
        if(bldSite->GetBuildingType() == bldType)
        {
            if(gwb.CalcDistance(pt, bldSite->GetPos()) < min)
                return true;
        }
    }
    return false;
}

bool AIPlayerJH::HarborPosClose(const MapPoint pt, unsigned range, bool onlyempty) const
{
    // skip harbordummy
    for(unsigned i = 1; i <= gwb.GetNumHarborPoints(); i++)
    {
        if(gwb.CalcDistance(pt, gwb.GetHarborPoint(i)) < range
           && HarborPosRelevant(i)) // in range and valid for ai - as in actually at a sea with more than 1 harbor spot
        {
            if(!onlyempty || !aii.IsBuildingOnNode(gwb.GetHarborPoint(i), BLD_HARBORBUILDING))
                return true;
        }
    }
    return false;
}

/// returns the percentage*100 of possible normal+ building places
unsigned AIPlayerJH::BQsurroundcheck(const MapPoint pt, unsigned range, bool includeexisting, unsigned limit)
{
    unsigned maxvalue = 6 * (2 << (range - 1)) - 5; // 1,7,19,43,91,... = 6*2^range -5
    unsigned count = 0;
    RTTR_Assert(aii.GetBuildingQuality(pt) == GetAINode(pt).bq);
    if((aii.GetBuildingQuality(pt) >= BQ_HUT && aii.GetBuildingQuality(pt) <= BQ_CASTLE) || aii.GetBuildingQuality(pt) == BQ_HARBOR)
    {
        count++;
    }
    NodalObjectType nob = gwb.GetNO(pt)->GetType();
    if(includeexisting)
    {
        if(nob == NOP_BUILDING || nob == NOP_BUILDINGSITE || nob == NOP_EXTENSION || nob == NOP_FIRE || nob == NOP_CHARBURNERPILE)
            count++;
    }
    // first count all the possible building places
    for(MapCoord tx = gwb.GetXA(pt, Direction::WEST), r = 1; r <= range; tx = gwb.GetXA(MapPoint(tx, pt.y), Direction::WEST), ++r)
    {
        MapPoint t2(tx, pt.y);
        for(unsigned i = 2; i < 8; ++i)
        {
            for(MapCoord r2 = 0; r2 < r; t2 = gwb.GetNeighbour(t2, Direction(i)), ++r2)
            {
                if(limit && ((count * 100) / maxvalue) > limit)
                    return ((count * 100) / maxvalue);
                // point can be used for a building
                if((aii.GetBuildingQualityAnyOwner(t2) >= BQ_HUT && aii.GetBuildingQualityAnyOwner(t2) <= BQ_CASTLE)
                   || aii.GetBuildingQualityAnyOwner(t2) == BQ_HARBOR)
                {
                    count++;
                    continue;
                }
                if(includeexisting)
                {
                    nob = gwb.GetNO(t2)->GetType();
                    if(nob == NOP_BUILDING || nob == NOP_BUILDINGSITE || nob == NOP_EXTENSION || nob == NOP_FIRE
                       || nob == NOP_CHARBURNERPILE)
                        count++;
                }
            }
        }
    }
    // LOG.write(("bqcheck at %i,%i r%u result: %u,%u \n",pt,range,count,maxvalue);
    return ((count * 100) / maxvalue);
}

bool AIPlayerJH::HarborPosRelevant(unsigned harborid, bool onlyempty) const
{
    if(harborid < 1 || harborid > gwb.GetNumHarborPoints()) // not a real harbor - shouldnt happen...
    {
        RTTR_Assert(false);
        return false;
    }

    for(unsigned r = 0; r < 6; r++)
    {
        const unsigned short seaId = gwb.GetSeaId(harborid, Direction::fromInt(r));
        if(!seaId)
            continue;

        for(unsigned curHarborId = 1; curHarborId <= gwb.GetNumHarborPoints(); curHarborId++) // start at 1 harbor dummy yadayada :>
        {
            if(curHarborId != harborid && gwb.IsHarborAtSea(curHarborId, seaId))
            {
                if(onlyempty) // check if the spot is actually free for colonization?
                {
                    if(gwb.IsHarborPointFree(curHarborId, playerId))
                        return true;
                } else
                    return true;
            }
        }
    }
    return false;
}

bool AIPlayerJH::NoEnemyHarbor()
{
    for(unsigned i = 1; i <= gwb.GetNumHarborPoints(); i++)
    {
        if(aii.IsBuildingOnNode(gwb.GetHarborPoint(i), BLD_HARBORBUILDING) && !aii.IsOwnTerritory(gwb.GetHarborPoint(i)))
        {
            // LOG.write(("found a harbor at spot %i ",i);
            return false;
        }
    }
    return true;
}

bool AIPlayerJH::IsInvalidShipyardPosition(const MapPoint pt)
{
    return BuildingNearby(pt, BLD_SHIPYARD, 20) || !HarborPosClose(pt, 8);
}

unsigned AIPlayerJH::AmountInStorage(GoodType good) const
{
    unsigned counter = 0;
    BOOST_FOREACH(const nobBaseWarehouse* wh, aii.GetStorehouses())
        counter += wh->GetInventory().goods[good];
    return counter;
}

unsigned AIPlayerJH::AmountInStorage(::Job job) const
{
    unsigned counter = 0;
    BOOST_FOREACH(const nobBaseWarehouse* wh, aii.GetStorehouses())
        counter += wh->GetInventory().people[job];
    return counter;
}

bool AIPlayerJH::ValidFishInRange(const MapPoint pt)
{
    unsigned max_radius = 5;
    for(MapCoord tx = gwb.GetXA(pt, Direction::WEST), r = 1; r <= max_radius; tx = gwb.GetXA(MapPoint(tx, pt.y), Direction::WEST), ++r)
    {
        MapPoint t2(tx, pt.y);
        for(unsigned i = 2; i < 8; ++i)
        {
            for(MapCoord r2 = 0; r2 < r; t2 = gwb.GetNeighbour(t2, Direction(i)), ++r2)
            {
                if(gwb.GetNode(t2).resources.has(Resource::Fish)) // fish on current spot?
                {
                    // LOG.write(("found fish at %i,%i ",t2);
                    // try to find a path to a neighboring node on the coast
                    for(int j = 0; j < 6; j++)
                    {
                        if(gwb.FindHumanPath(pt, gwb.GetNeighbour(t2, Direction::fromInt(j)), 10) != 0xFF)
                            return true;
                    }
                }
            }
        }
    }
    return false;
}

unsigned AIPlayerJH::GetNumAIRelevantSeaIds() const
{
    std::list<unsigned short> validseaids;
    std::list<unsigned short> onetimeuseseaids;
    for(unsigned i = 1; i <= gwb.GetNumHarborPoints(); i++)
    {
        for(unsigned r = 0; r < 6; r++)
        {
            const unsigned short seaId = gwb.GetSeaId(i, Direction::fromInt(r));
            if(!seaId)
                continue;
            // there is a sea id? -> check if it is already a validid or a once found id
            if(!helpers::contains(validseaids, seaId)) // not yet in validseas?
            {
                if(!helpers::contains(onetimeuseseaids, seaId)) // not yet in onetimeuseseaids?
                    onetimeuseseaids.push_back(seaId);
                else
                {
                    // LOG.write(("found a second harbor at sea id %i \n",seaIds[r]);
                    onetimeuseseaids.remove(seaId);
                    validseaids.push_back(seaId);
                }
            }
        }
    }
    return validseaids.size();
}

void AIPlayerJH::AdjustSettings()
{
    // update tool creation settings
    ToolSettings toolsettings;
    const Inventory& inventory = aii.GetInventory();
    // Saw
    toolsettings[2] = (inventory.goods[GD_SAW] + inventory.people[JOB_CARPENTER] < 2) ? 4 : inventory.goods[GD_SAW] < 1 ? 1 : 0;
    // Pickaxe
    toolsettings[3] = (inventory.goods[GD_PICKAXE] < 1) ? 1 : 0;
    // Hammer
    toolsettings[4] = (inventory.goods[GD_HAMMER] < 1) ? 1 : 0;
    // Crucible
    toolsettings[6] =
      (inventory.goods[GD_CRUCIBLE] + inventory.people[JOB_IRONFOUNDER] < bldPlanner->GetNumBuildings(BLD_IRONSMELTER) + 1) ? 1 : 0;
    // Scythe
    toolsettings[8] =
      (toolsettings[4] < 1 && toolsettings[3] < 1 && toolsettings[6] < 1 && toolsettings[2] < 1 && (inventory.goods[GD_SCYTHE] < 1)) ? 1 :
                                                                                                                                       0;
    // Rollingpin
    toolsettings[10] = (inventory.goods[GD_ROLLINGPIN] + inventory.people[JOB_BAKER] < bldPlanner->GetNumBuildings(BLD_BAKERY) + 1) ? 1 : 0;
    // Shovel
    toolsettings[5] =
      (toolsettings[4] < 1 && toolsettings[3] < 1 && toolsettings[6] < 1 && toolsettings[2] < 1 && (inventory.goods[GD_SHOVEL] < 1)) ? 1 :
                                                                                                                                       0;
    // Axe
    toolsettings[1] = (toolsettings[4] < 1 && toolsettings[3] < 1 && toolsettings[6] < 1 && toolsettings[2] < 1
                       && (inventory.goods[GD_AXE] + inventory.people[JOB_WOODCUTTER] < 12) && inventory.goods[GD_AXE] < 1) ?
                        1 :
                        0;
    // Tongs(metalworks)
    toolsettings[0] =
      0; //(toolsettings[4]<1&&toolsettings[3]<1&&toolsettings[6]<1&&toolsettings[2]<1&&(aii.GetInventory().goods[GD_TONGS]<1))?1:0;
    // cleaver
    toolsettings[9] =
      0; //(aii.GetInventory().goods[GD_CLEAVER]+aii.GetInventory().people[JOB_BUTCHER]<construction->GetNumBuildings(BLD_SLAUGHTERHOUSE)+1)?1:0;
    // rod & line
    toolsettings[7] = 0;
    // bow
    toolsettings[11] = 0;
    for(unsigned i = 0; i < toolsettings.size(); i++)
        if(toolsettings[i] != player.GetToolPriority(i))
        {
            aii.ChangeTools(toolsettings);
            break;
        }

    // Set military settings to some currently required values
    MilitarySettings milSettings;
    milSettings[0] = 10;
    milSettings[1] =
      HasFrontierBuildings() ? 5 : 0; // if we have a front send strong soldiers first else weak first to make upgrading easier
    milSettings[2] = 4;
    milSettings[3] = 5;
    // interior 0bar full if we have an upgrade building and gold(or produce gold) else 1 soldier each
    milSettings[4] = UpdateUpgradeBuilding() >= 0
                         && (inventory.goods[GD_COINS] > 0
                             || (inventory.goods[GD_GOLD] > 0 && inventory.goods[GD_COAL] > 0 && !aii.GetBuildings(BLD_MINT).empty())) ?
                       8 :
                       0;
    milSettings[6] = ggs.getSelection(AddonId::SEA_ATTACK) == 2 ? 0 : 8; // harbor flag: no sea attacks?->no soldiers else 50% to 100%
    milSettings[5] = CalcMilSettings(); // inland 1bar min 50% max 100% depending on how many soldiers are available
    milSettings[7] = 8;                 // front: 100%
    if(player.GetMilitarySetting(5) != milSettings[5] || player.GetMilitarySetting(6) != milSettings[6]
       || player.GetMilitarySetting(4) != milSettings[4]
       || player.GetMilitarySetting(1) != milSettings[1]) // only send the command if we want to change something
        aii.ChangeMilitary(milSettings);
}

unsigned AIPlayerJH::CalcMilSettings()
{
    unsigned InlandTroops[5] = {0, 0, 0, 0, 0}; // how many troops are required to fill inland buildings at settings 4,5,6,7,8
    /// first sum up all soldiers we have
    unsigned numSoldiers = 0;
    for(unsigned i = 0; i < SOLDIER_JOBS.size(); i++)
        numSoldiers += aii.GetInventory().people[SOLDIER_JOBS[i]];

    // now add up all counts of soldiers that are fixed in use and those that depend on whatever we have as a result
    const unsigned numShouldStayConnected = GetNumPlannedConnectedInlandMilitaryBlds();
    int count = 0;
    unsigned soldierInUseFixed = 0;
    const int uun = UpdateUpgradeBuilding();
    const std::list<nobMilitary*>& militaryBuildings = aii.GetMilitaryBuildings();
    BOOST_FOREACH(const nobMilitary* milBld, militaryBuildings)
    {
        if(milBld->GetFrontierDistance() == 3 || (milBld->GetFrontierDistance() == 2 && ggs.getSelection(AddonId::SEA_ATTACK) != 2)
           || (milBld->GetFrontierDistance() == 0
               && (militaryBuildings.size() < (unsigned)count + numShouldStayConnected || count == uun))) // front or connected interior
        {
            soldierInUseFixed += milBld->CalcRequiredNumTroops(1, 8);
        } else if(milBld->GetFrontierDistance() == 1) // 1 bar (inland)
        {
            for(int i = 0; i < 5; i++)
                InlandTroops[i] += milBld->CalcRequiredNumTroops(1, 4 + i);
        } else // setting should be 0 so add 1 soldier
            soldierInUseFixed++;

        count++;
    }

    // now the current need total and for inland and harbor is ready for use
    unsigned returnValue = 8;
    while(returnValue > 4)
    {
        // have more than enough soldiers for this setting or just enough and this is the current setting? -> return it else try the next
        // lower setting down to 4 (50%)
        if(soldierInUseFixed + InlandTroops[returnValue - 4] < numSoldiers * 10 / 11
           || (player.GetMilitarySetting(5) >= returnValue && soldierInUseFixed + InlandTroops[returnValue - 4] < numSoldiers))
            break;
        returnValue--;
    }
    // LOG.write(("player %i inland milsetting %i \n",playerId,returnvalue);
    return returnValue;
}

} // namespace AIJH
