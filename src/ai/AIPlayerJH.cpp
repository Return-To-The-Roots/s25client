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
#include "AIPlayerJH.h"

#include "GameClientPlayer.h"

#include "buildings/nobMilitary.h"
#include "buildings/nobHQ.h"
#include "buildings/noBuildingSite.h"
#include "buildings/nobHarborBuilding.h"
#include "buildings/nobUsual.h"
#include "nodeObjs/noShip.h"
#include "nodeObjs/noFlag.h"
#include "nodeObjs/noTree.h"
#include "nodeObjs/noAnimal.h"

#include "AIConstruction.h"
#include "gameData/TerrainData.h"
#include "FindWhConditions.h"
#include "GameMessages.h"
#include "GameServer.h"

#include <boost/array.hpp>
#include <list>
#include <algorithm>

// Include last!
#include "DebugNew.h" // IWYU pragma: keep
class GameClientPlayerList;
namespace AIEvent { class Base; }

// from Pathfinding.cpp
bool IsPointOK_RoadPath(const GameWorldBase& gwb, const MapPoint pt, const unsigned char dir, const void* param);

AIPlayerJH::AIPlayerJH(const unsigned char playerid, const GameWorldBase& gwb, const GameClientPlayer& player,
                       const GameClientPlayerList& players, const GlobalGameSettings& ggs,
                       const AI::Level level) : AIBase(playerid, gwb, player, players, ggs, level),
                        UpgradeBldListNumber(-1), isInitGfCompleted(false), defeated(false), UpgradeBldPos(MapPoint::Invalid())
{
    construction = new AIConstruction(aii, *this);
    InitNodes();
    InitResourceMaps();
    SaveResourceMapsToFile();

    switch (level)
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
        default:
            attack_interval = 1;
            build_interval = 1;
            break;
    }
}

AIPlayerJH::~AIPlayerJH()
{
    delete construction;
}

/// Wird jeden GF aufgerufen und die KI kann hier entsprechende Handlungen vollziehen
void AIPlayerJH::RunGF(const unsigned gf, bool gfisnwf)
{
    if (defeated)
        return;

    if (TestDefeat())
        return;
    if (!isInitGfCompleted)
    {
        InitStoreAndMilitarylists();		
		InitDistribution();
		construction->constructionorders.resize(BUILDING_TYPES_COUNT);
    }
	if(isInitGfCompleted<10)
	{
		isInitGfCompleted++;
		return; //  1 init -> 2 test defeat -> 3 do other ai stuff -> goto 2
	}
	if (gfisnwf)//nwf -> now the orders have been executed -> new constructions can be started
	{	
		construction->constructionlocations.clear();
		for(unsigned i=0;i<BUILDING_TYPES_COUNT;i++)
			construction->constructionorders[i]=0;
	}

    if (gf == 100)
    {
        if(aii.GetMilitaryBuildings().empty() && aii.GetStorehouses().size() < 2)
        {
            Chat(_("Hi, I'm an artifical player and I'm not very good yet!"));
            // AI doesn't usually crash the game any more :)
            // Chat(_("And I may crash your game sometimes..."));
        }
	}

    if (!gfisnwf) //try to complete a job on the list
    {
		//LOG.lprintf("ai doing stuff %i \n",playerid);
        if (gf % 100 == 0)
            construction->RefreshBuildingCount();
        ExecuteAIJob();
    }

    if ((gf + playerid * 17) % attack_interval == 0)
    {
        //CheckExistingMilitaryBuildings();
        TryToAttack();
    }
	if (((gf + playerid * 17) % 73 == 0) && (level != AI::EASY))
    {
        MilUpgradeOptim();
    }

    if ((gf + 41 + playerid * 17) % attack_interval == 0)
    {
        if(ggs.getSelection(AddonId::SEA_ATTACK) < 2) //not deactivated by addon? -> go ahead
            TrySeaAttack();
    }

    if ((gf + playerid * 13) % 1500 == 0) 
    {
        CheckExpeditions();
        CheckForester();
        CheckGranitMine();
    }

    if((gf + playerid * 11) % 150 == 0)
    {
        AdjustSettings();
        //check for useless sawmills
        const std::list<nobUsual*>& sawMills = aii.GetBuildings(BLD_SAWMILL);
        if(sawMills.size() > 3)
        {
            int burns = 0;
            for(std::list<nobUsual*>::const_iterator it = sawMills.begin(); it != sawMills.end(); ++it)
            {
                if((*it)->GetProductivity() < 1 && (*it)->HasWorker() && (*it)->GetWares(0) < 1 && (sawMills.size() - burns) > 3 && !(*it)->AreThereAnyOrderedWares())
                {
                    aii.DestroyBuilding((*it));
                    RemoveUnusedRoad(aii.GetSpecObj<noFlag>(aii.GetNeighbour((*it)->GetPos(), Direction::SOUTHEAST)), 1, true);
                    burns++;
                }
            }
        }
    }

    if((gf + playerid * 7) % build_interval == 0) // plan new buildings
    {
        PlanNewBuildings(gf);
    }	
}

void AIPlayerJH::PlanNewBuildings( const unsigned gf )
{
    construction->RefreshBuildingCount();

    //pick a random storehouse and try to build one of these buildings around it (checks if we actually want more of the building type)
    boost::array<BuildingType, 24> bldToTest =
    {{
        BLD_HARBORBUILDING,
        BLD_SHIPYARD,
        BLD_SAWMILL,
        BLD_FORESTER,			
        BLD_FARM,
        BLD_FISHERY,
        BLD_WOODCUTTER,
        BLD_QUARRY,
        BLD_GOLDMINE,
        BLD_IRONMINE,
        BLD_COALMINE,
        BLD_GRANITEMINE,
        BLD_HUNTER,
        BLD_CHARBURNER,
        BLD_IRONSMELTER,
        BLD_MINT,
        BLD_ARMORY,
        BLD_METALWORKS,
        BLD_BREWERY,
        BLD_MILL,
        BLD_PIGFARM,
        BLD_SLAUGHTERHOUSE,
        BLD_BAKERY,
        BLD_DONKEYBREEDER
    }};
    const unsigned resGatherBldCount = 14; /* The first n buildings in the above list, that gather resources */

    //LOG.lprintf("new buildorders %i whs and %i mil for player %i \n",aii.GetStorehouses().size(),aii.GetMilitaryBuildings().size(),playerid);

    const std::list<nobBaseWarehouse*>& storehouses = aii.GetStorehouses();
    if(!storehouses.empty())
    {			
        int randomStore = rand() % (storehouses.size());
        //collect swords,shields,helpers,privates and beer in first storehouse or whatever is closest to the upgradebuilding if we have one!
        nobBaseWarehouse* wh = GetUpgradeBuildingWarehouse();
        SetGatheringForUpgradeWarehouse(wh);

        if (ggs.GetMaxMilitaryRank() > 0) //there is more than 1 rank available -> distribute
            DistributeMaxRankSoldiersByBlocking(5,wh);
        //unlimited when every warehouse has at least that amount
        DistributeGoodsByBlocking(GD_BOARDS, 30); //30 boards for each warehouse - block after that - should speed up expansion
        DistributeGoodsByBlocking(GD_STONES, 50); //50 stones for each warehouse - block after that - should limit losses in case a warehouse is destroyed
        //go to the picked random warehouse and try to build around it
        std::list<nobBaseWarehouse*>::const_iterator it = storehouses.begin();
        std::advance(it, randomStore);
        UpdateNodesAroundNoBorder((*it)->GetPos(), 15); //update the area we want to build in first
        for (unsigned int i = 0; i < bldToTest.size(); i++)
        {
            if (construction->Wanted(bldToTest[i]))
            {
                AddBuildJobAroundEvery(bldToTest[i], true); //add a buildorder for the picked buildingtype at every warehouse
            }
        }
        if(gf > 1500 || aii.GetInventory().goods[GD_BOARDS] > 11)
            AddBuildJob(construction->ChooseMilitaryBuilding((*it)->GetPos()), (*it)->GetPos());
    }
    //end of construction around & orders for warehouses

    //now pick a random military building and try to build around that as well
    const std::list<nobMilitary*>& militaryBuildings = aii.GetMilitaryBuildings();
    if(militaryBuildings.empty())
        return;
    int randomMiliBld = rand() % (militaryBuildings.size());
    std::list<nobMilitary*>::const_iterator it2 = militaryBuildings.begin();
    std::advance(it2, randomMiliBld);
    MapPoint bldPos = (*it2)->GetPos();
    UpdateReachableNodes(bldPos, 15);
    //resource gathering buildings only around military; processing only close to warehouses
    for (unsigned int i = 0; i < resGatherBldCount; i++)
    {
        if (construction->Wanted(bldToTest[i]))
        {
            AddBuildJobAroundEvery(bldToTest[i], false);
        }
    }
    AddBuildJob(construction->ChooseMilitaryBuilding(bldPos), bldPos);
    if((*it2)->IsUseless() && (*it2)->IsDemolitionAllowed() && randomMiliBld!=UpdateUpgradeBuilding())
    {
        aii.DestroyBuilding(bldPos);
    }
}

bool AIPlayerJH::TestDefeat()
{		
    if (isInitGfCompleted>=10 && aii.GetStorehouses().empty())
    {
		//LOG.lprintf("ai defeated player %i \n",playerid);
        defeated = true;
        aii.Surrender();
        Chat(_("You win"));
        return true;
    }
    return false;
}

unsigned AIPlayerJH::GetJobNum() const { return eventManager.GetEventNum() + construction->GetBuildJobNum() + construction->GetConnectJobNum(); }	

/// returns the warehouse closest to the upgradebuilding or if it cant find a way the first warehouse and if there is no warehouse left null
nobBaseWarehouse* AIPlayerJH::GetUpgradeBuildingWarehouse()
{
    const std::list<nobBaseWarehouse*>& storehouses = aii.GetStorehouses();
	if(storehouses.empty())
		return NULL;
	nobBaseWarehouse* wh=(*storehouses.begin());
	int uub=UpdateUpgradeBuilding();
	
	if (uub>=0 && storehouses.size()>1) //upgradebuilding exists and more than 1 warehouse -> find warehouse closest to the upgradebuilding - gather stuff there and deactivate gathering in the previous one
	{		
		std::list<nobMilitary*>::const_iterator upgradeBldIt=aii.GetMilitaryBuildings().begin();
		std::advance(upgradeBldIt,uub);
		//which warehouse is closest to the upgrade building? -> train troops there and block max ranks			
		wh = aii.FindWarehouse(**upgradeBldIt, FW::NoCondition(), false, false);
		if(!wh)
			wh = storehouses.front();
	}
	return wh;
}

void AIPlayerJH::AddBuildJob(BuildingType type, const MapPoint pt, bool front, bool searchPosition)
{
    if(type != BLD_NOTHING)
        construction->AddBuildJob(new AIJH::BuildJob(*this, type, pt, searchPosition ? AIJH::SEARCHMODE_RADIUS : AIJH::SEARCHMODE_NONE), front);
}

void AIPlayerJH::AddBuildJobAroundEvery(BuildingType bt, bool warehouse)
{
	if(warehouse)
	{
		for(std::list<nobBaseWarehouse*>::const_iterator it=aii.GetStorehouses().begin();it!=aii.GetStorehouses().end();++it)
		{
			AddBuildJob(bt, (*it)->GetPos(), false);
		}
	}
	else
	{
		for(std::list<nobMilitary*>::const_iterator it=aii.GetMilitaryBuildings().begin();it!=aii.GetMilitaryBuildings().end();++it)
		{
			AddBuildJob(bt, (*it)->GetPos(), false);
		}
	}
}

void AIPlayerJH::SetGatheringForUpgradeWarehouse(nobBaseWarehouse* upgradewarehouse)
{
	
	for (std::list<nobBaseWarehouse*>::const_iterator it=aii.GetStorehouses().begin();it!=aii.GetStorehouses().end();++it)
	{
		//deactivate gathering for all warehouses that are NOT the one next to the upgradebuilding
        const MapPoint whPos = (*it)->GetPos();
		if(upgradewarehouse->GetPos()!=whPos)
		{
            if((*it)->IsInventorySetting(GD_BEER, EInventorySetting::COLLECT)) //collecting beer? -> stop it
                aii.SetInventorySetting(whPos, GD_BEER, InventorySetting());

            if((*it)->IsInventorySetting(GD_SWORD, EInventorySetting::COLLECT)) //collecting swords? -> stop it
                aii.SetInventorySetting(whPos, GD_SWORD, InventorySetting());

            if((*it)->IsInventorySetting(GD_SHIELDROMANS, EInventorySetting::COLLECT)) //collecting shields? -> stop it
                aii.SetInventorySetting(whPos, GD_SHIELDROMANS, InventorySetting());

            if((*it)->IsInventorySetting(JOB_PRIVATE, EInventorySetting::COLLECT)) //collecting privates? -> stop it
                aii.SetInventorySetting(whPos, JOB_PRIVATE, InventorySetting());

            if((*it)->IsInventorySetting(JOB_HELPER, EInventorySetting::COLLECT)) //collecting helpers? -> stop it
                aii.SetInventorySetting(whPos, JOB_HELPER, InventorySetting());
		}
		else//activate gathering in the closest warehouse
		{
            if(!(*it)->IsInventorySetting(GD_BEER, EInventorySetting::COLLECT)) //not collecting beer? -> start it
                aii.SetInventorySetting(whPos, GD_BEER, EInventorySetting::COLLECT);

            if(!(*it)->IsInventorySetting(GD_SWORD, EInventorySetting::COLLECT)) //not collecting swords? -> start it
                aii.SetInventorySetting(whPos, GD_SWORD, EInventorySetting::COLLECT);

            if(!(*it)->IsInventorySetting(GD_SHIELDROMANS, EInventorySetting::COLLECT)) //not collecting shields? -> start it
                aii.SetInventorySetting(whPos, GD_SHIELDROMANS, EInventorySetting::COLLECT);

            if(!(*it)->IsInventorySetting(JOB_PRIVATE, EInventorySetting::COLLECT) && ggs.GetMaxMilitaryRank() > 0) //not collecting privates AND we can actually upgrade soldiers? -> start it
                aii.SetInventorySetting(whPos, JOB_PRIVATE, EInventorySetting::COLLECT);

            //less than 50 helpers - collect them: more than 50 stop collecting
            if((*it)->GetInventory().people[JOB_HELPER] < 50)
            {
                if(!(*it)->IsInventorySetting(JOB_HELPER, EInventorySetting::COLLECT))
                    aii.SetInventorySetting(whPos, JOB_HELPER, EInventorySetting::COLLECT);
            } else
            {
                if((*it)->IsInventorySetting(JOB_HELPER, EInventorySetting::COLLECT))
                    aii.SetInventorySetting(whPos, JOB_HELPER, InventorySetting());
            }
		}
	}
}

AIJH::Resource AIPlayerJH::CalcResource(const MapPoint pt)
{
    AIJH::Resource subRes = aii.GetSubsurfaceResource(pt);
    AIJH::Resource surfRes = aii.GetSurfaceResource(pt);

    // no resources underground
    if (subRes == AIJH::NOTHING)
    {
        // also no resource on the ground: plant space or unusable?
        if(surfRes == AIJH::NOTHING) 
        {
            // already road, really no resources here
            if(aii.IsRoadPoint(pt))
                return AIJH::NOTHING;
            // check for vital plant space
            for(int i = 0; i < Direction::COUNT; ++i)
            {
                TerrainType t = aii.GetTerrainAround(pt, Direction::fromInt(i));

                // check against valid terrains for planting
                if(!TerrainData::IsVital(t))
                    return AIJH::NOTHING;
            }
            return AIJH::PLANTSPACE;
        }

        return surfRes;
    }
    else // resources in underground
    {
        if (surfRes == AIJH::STONES || surfRes == AIJH::WOOD)
            return AIJH::MULTIPLE;

        if (subRes == AIJH::BLOCKED)
            return AIJH::NOTHING; // nicht so ganz logisch... aber Blocked als res is doof TODO

        return subRes;
    }
}

void AIPlayerJH::InitReachableNodes()
{
    unsigned short width = aii.GetMapWidth();
    unsigned short height = aii.GetMapHeight();

    std::queue<MapPoint> toCheck;

    // Alle auf not reachable setzen
    for (MapPoint pt(0, 0); pt.y < height; ++pt.y)
    {
        for (pt.x = 0; pt.x < width; ++pt.x)
        {
            unsigned i = gwb.GetIdx(pt);
            nodes[i].reachable = false;
            nodes[i].failed_penalty = 0;
            const noFlag* myFlag = aii.GetSpecObj<noFlag>(pt);
            if (myFlag)
            {
                if (myFlag->GetPlayer() == playerid)
                {
                    nodes[i].reachable = true;
                    toCheck.push(pt);
                }
            }
        }
    }

    IterativeReachableNodeChecker(toCheck);
}

void AIPlayerJH::IterativeReachableNodeChecker(std::queue<MapPoint>& toCheck)
{
    // TODO auch mal bootswege bauen können
    //Param_RoadPath prp = { false };

    while(!toCheck.empty())
    {
        // Reachable coordinate
        MapPoint r = toCheck.front();

        // Coordinates to test around this reachable coordinate
        for (int dir = 0; dir < Direction::COUNT; ++dir)
        {
            MapPoint n = aii.GetNeighbour(r, Direction::fromInt(dir));
            unsigned ni = aii.GetIdx(n);

            // already reached, don't test again
            if (nodes[ni].reachable)
                continue;

            bool boat = false;
            // Test whether point is reachable; yes->add to check list
            if (IsPointOK_RoadPath(gwb, n, (dir + 3) % 6, (void*) &boat))
            {
                if (nodes[ni].failed_penalty == 0)
                {
                    nodes[ni].reachable = true;
                    toCheck.push(n);
                }
                else
                {
                    nodes[ni].failed_penalty--;
                }
            }
        }
        toCheck.pop();
    }
}


void AIPlayerJH::UpdateReachableNodes(const MapPoint pt, unsigned radius)
{
    std::vector<MapPoint> pts = aii.GetPointsInRadius(pt, radius);
    std::queue<MapPoint> toCheck;

    for(std::vector<MapPoint>::const_iterator it = pts.begin(); it != pts.end(); ++it)
    {
        const unsigned idx = aii.GetIdx(*it);
        const noFlag* flag = aii.GetSpecObj<noFlag>(*it);
        if (flag && flag->GetPlayer() == playerid)
        {
            nodes[idx].reachable = true;
            toCheck.push(*it);
        }else
            nodes[idx].reachable = false;
    }
    IterativeReachableNodeChecker(toCheck);
}

void AIPlayerJH::InitNodes()
{
    unsigned short width = aii.GetMapWidth();
    unsigned short height = aii.GetMapHeight();

    nodes.resize(width * height);

    InitReachableNodes();

    for (MapPoint pt(0, 0); pt.y < height; ++pt.y)
    {
        for (pt.x = 0; pt.x < width; ++pt.x)
        {
            unsigned i = gwb.GetIdx(pt);

            // if reachable, we'll calc bq
            if (nodes[i].reachable)
            {
                nodes[i].owned = true;
                nodes[i].bq = aii.GetBuildingQuality(pt);
            }
            else
            {
                nodes[i].owned = false;
                nodes[i].bq = BQ_NOTHING;
            }

            nodes[i].res = CalcResource(pt);
            nodes[i].border = aii.IsBorder(pt);
            nodes[i].farmed = false;
        }
    }
}

void AIPlayerJH::UpdateNodes()
{

}

void AIPlayerJH::InitResourceMaps()
{
    for (unsigned res = 0; res < AIJH::RES_TYPE_COUNT; ++res)
    {
        resourceMaps[res] = AIResourceMap(static_cast<AIJH::Resource>(res), aii, nodes);
        resourceMaps[res].Init();
    }
}

namespace{
    struct MapPoint2Idx
    {
        typedef unsigned result_type;
        const AIInterface& aii_;

        MapPoint2Idx(const AIInterface& aii): aii_(aii){}
        result_type operator()(const MapPoint pt, unsigned  /*r*/)
        {
            return aii_.GetIdx(pt);
        }
    };
}

void AIPlayerJH::SetFarmedNodes(const MapPoint pt, bool set)
{
    // Radius in dem Bausplatz für Felder blockiert wird
    const unsigned radius = 3;

    nodes[aii.GetIdx(pt)].farmed = set;
    std::vector<unsigned> ptIdxs = aii.GetPointsInRadius(pt, radius, MapPoint2Idx(aii));
    for(std::vector<unsigned>::const_iterator it = ptIdxs.begin(); it != ptIdxs.end(); ++it)
        nodes[*it].farmed = set;
}

bool AIPlayerJH::FindGoodPosition(MapPoint& pt, AIJH::Resource res, int threshold, BuildingQuality size, int radius, bool inTerritory)
{
    return resourceMaps[res].FindGoodPosition(pt, threshold, size, radius, inTerritory);
}

PositionSearch* AIPlayerJH::CreatePositionSearch(MapPoint& pt, AIJH::Resource res, BuildingQuality size, int minimum, BuildingType  /*bld*/, bool best)
{
    // set some basic parameters
    PositionSearch* p = new PositionSearch(pt, res, minimum, size, BLD_WOODCUTTER, best);
    p->nodesPerStep = 25; // TODO make it dependent on something...
    p->resultValue = 0;

    // allocate memory for the nodes
    unsigned numNodes = aii.GetMapWidth() * aii.GetMapHeight();
    p->tested.clear();
    p->tested.resize(numNodes, false);
    std::queue<MapPoint> emptyQueue;
    p->toTest = emptyQueue;


    // if no useful startpos is given, use headquarter
    if (pt.x >= aii.GetMapWidth() || pt.y >= aii.GetMapHeight())
    {
        pt = aii.GetHeadquarter()->GetPos();
    }

    // insert start position as first node to test
    p->toTest.push(pt);
    p->tested[aii.GetIdx(pt)] = true;

    return p;
}

PositionSearchState AIPlayerJH::FindGoodPosition(PositionSearch* search, bool best)
{
    AIResourceMap& resMap = resourceMaps[search->res];
    // make nodesPerStep tests
    for (int i = 0; i < search->nodesPerStep; i++)
    {
        // no more nodes to test? end this!
        if (search->toTest.empty())
            break;

        // get the node
        MapPoint pt = search->toTest.front();
        search->toTest.pop();
        AIJH::Node& node = nodes[aii.GetIdx(pt)];

        // and test it... TODO exception at res::borderland?
        if (resMap[pt] > search->resultValue // value better
                && node.owned && node.reachable && !node.farmed // available node
                && ((node.bq >= search->size && node.bq < BQ_MINE) || (node.bq == search->size)) // matching size
           )
        {
            // store location & value
            search->resultValue = resMap[pt];
            search->result = pt;
        }

        // now insert neighbouring nodes...
        for (int dir = 0; dir < Direction::COUNT; ++dir)
        {
            MapPoint n = aii.GetNeighbour(pt, Direction::fromInt(dir));
            unsigned ni = aii.GetIdx(n);

            // test if already tested or not in territory
            if (!search->tested[ni] && nodes[ni].owned)
            {
                search->toTest.push(pt);
                search->tested[ni] = true;
            }
        }
    }

    // decide the state of the search

    if (search->toTest.empty() && search->resultValue < search->minimum)
    {
        // no more nodes to test, not reached minimum
        return SEARCH_FAILED;
    }
    else if ( (search->resultValue >= search->minimum && !best)
              || (search->resultValue >= search->minimum && search->toTest.empty()))
    {
        // reached minimal satifiying value or best value, if needed
        return SEARCH_SUCCESSFUL;
    }
    else
    {
        // more to search...
        return SEARCH_IN_PROGRESS;
    }
}


bool AIPlayerJH::FindBestPositionDiminishingResource(MapPoint& pt, AIJH::Resource res, BuildingQuality size, int minimum, int radius, bool inTerritory)
{
    bool fixed = ggs.isEnabled(AddonId::INEXHAUSTIBLE_MINES) && (res == AIJH::IRONORE || res == AIJH::COAL || res == AIJH::GOLD || res == AIJH::GRANITE);
    unsigned short width = aii.GetMapWidth();
    unsigned short height = aii.GetMapHeight();
    int temp = 0;
    bool lastcirclevaluecalculated = false;
    bool lastvaluecalculated = false;
    //to avoid having to calculate a value twice and still move left on the same level without any problems we use this variable to remember the first calculation we did in the circle.
    int circlestartvalue = 0;
    //outside of map bounds? -> search around our main storehouse!
    if (pt.x >= width || pt.y >= height)
    {
        pt = aii.GetStorehouses().front()->GetPos();
    }

    // TODO was besseres wär schön ;)
    if (radius == -1)
        radius = 11;

    MapPoint best(0, 0);
    int best_value = -1;

    for(MapCoord tx = aii.GetXA(pt, Direction::WEST), r = 1; r <= radius; tx = aii.GetXA(tx, pt.y, Direction::WEST), ++r)
    {
        MapPoint t2(tx, pt.y);
        for(unsigned curDir = 2; curDir < 8; ++curDir)
        {
            for(MapCoord step = 0; step < r; ++step)
            {
                unsigned n = aii.GetIdx(t2);
                int& resMapVal = resourceMaps[res][t2];
                if(fixed)
                    temp = resMapVal;
                else
                {
                    //only do a complete calculation for the first point or when moving outward and the last value is unknown
                    if((r < 2 || !lastcirclevaluecalculated) && step < 1 && curDir < 3 && resMapVal)
                    {
                        temp = aii.CalcResourceValue(t2, res);
                        circlestartvalue = temp;
                        lastcirclevaluecalculated = true;
                        lastvaluecalculated = true;
                    }
                    else if(!resMapVal) //was there ever anything? if not skip it!
                    {
                        if(step < 1 && curDir < 3)
                            lastcirclevaluecalculated = false;
                        lastvaluecalculated = false;
                        temp = resMapVal;
                    }
                    else if(step < 1 && curDir < 3)  //circle not yet started? -> last direction was outward (left=0)
                    {
                        temp = aii.CalcResourceValue(t2, res, 0, circlestartvalue);
                        circlestartvalue = temp;
                    }
                    else if(lastvaluecalculated)
                    {
                        if(step > 0) //we moved direction i%6
                            temp = aii.CalcResourceValue(t2, res, curDir % 6, temp);
                        else //last step was the previous direction
                            temp = aii.CalcResourceValue(t2, res, (curDir - 1) % 6, temp);
                    }
                    else
                    {
                        temp = aii.CalcResourceValue(t2, res);
                        lastvaluecalculated = true;
                    }
                    //if(resMapVal)
                    //RTTR_Assert(temp==aii.CalcResourceValue(t2,res));
                    //copy the value to the resource map
                    resMapVal = temp;
                }
                if(res == AIJH::FISH || res == AIJH::STONES)
                {
                    //remove permanently invalid spots to speed up future checks
                    TerrainType t1 = aii.GetTerrain(t2);
                    if(!TerrainData::IsUseable(t1) || TerrainData::IsMineable(t1) || t1 == TT_DESERT)
                        resMapVal = 0;
                }
                else //= granite,gold,iron,coal
                {
                    if(!TerrainData::IsMineable(aii.GetTerrain(t2)))
                        resMapVal = 0;
                }
                if (temp > best_value)
                {
                    if (!nodes[n].reachable || (inTerritory && !aii.IsOwnTerritory(t2)) || nodes[n].farmed)
                    {
                        t2 = aii.GetNeighbour(t2, Direction(curDir));
                        continue;
                    }
                    //special case fish -> check for other fishery buildings
                    if(res == AIJH::FISH && BuildingNearby(t2, BLD_FISHERY, 6))
                    {
                        t2 = aii.GetNeighbour(t2, Direction(curDir));
                        continue;
                    }
                    //dont build next to harborspots
                    if(HarborPosClose(t2, 3, true))
                    {
                        t2 = aii.GetNeighbour(t2, Direction(curDir));
                        continue;
                    }
                    BuildingQuality bq = aii.GetBuildingQuality(t2);
                    if ( (bq >= size && bq < BQ_MINE) // normales Gebäude
                            || (bq == size))    // auch Bergwerke
                    {
                        best = t2;
                        best_value = temp;
                        //TODO: calculate "perfect" rating and instantly return if we got that already
                    }
                }
                t2 = aii.GetNeighbour(t2, Direction(curDir));
            }
        }
    }

    if (best_value >= minimum)
    {
        pt = best;
        return true;
    }
    return false;
}

// TODO: this totally ignores existing buildings of the same type. It should not. Re-introduce the resource maps?
bool AIPlayerJH::FindBestPosition(MapPoint& pt, AIJH::Resource res, BuildingQuality size, int minimum, int radius, bool inTerritory)
{
    if(res == AIJH::IRONORE || res == AIJH::COAL || res == AIJH::GOLD || res == AIJH::GRANITE || res == AIJH::STONES || res == AIJH::FISH)
        return FindBestPositionDiminishingResource(pt, res, size, minimum, radius, inTerritory);
    unsigned short width = aii.GetMapWidth();
    unsigned short height = aii.GetMapHeight();
    //to avoid having to calculate a value twice and still move left on the same level without any problems we use this variable to remember the first calculation we did in the circle.
    int circlestartvalue = 0;
    //outside of map bounds? -> search around our main storehouse!
    if (pt.x >= width || pt.y >= height)
    {
        pt = aii.GetStorehouses().front()->GetPos();
    }

    // TODO was besseres wär schön ;)
    if (radius == -1)
        radius = 11;

    MapPoint best(0, 0);
    int best_value = -1;
    int temp = 0;

    for(MapCoord tx = aii.GetXA(pt, Direction::WEST), r = 1; r <= radius; tx = aii.GetXA(tx, pt.y, Direction::WEST), ++r)
    {
        MapPoint t2(tx, pt.y);
        for(unsigned curDir = 2; curDir < 8; ++curDir)
        {
            for(MapCoord step = 0; step < r; ++step)
            {
                unsigned n = aii.GetIdx(t2);
                if(r == 1 && step == 0 && curDir == 2)
                {
                    //only do a complete calculation for the first point!
                    temp = aii.CalcResourceValue(t2, res);
                    circlestartvalue = temp;
                }
                else if(step == 0 && curDir == 2)
                {
                    //circle not yet started? -> last direction was outward (left=0)
                    temp = aii.CalcResourceValue(t2, res, 0, circlestartvalue);
                    circlestartvalue = temp;
                }
                else if(step > 0) //we moved direction i%6
                    temp = aii.CalcResourceValue(t2, res, curDir % 6, temp);
                else //last step was the previous direction
                    temp = aii.CalcResourceValue(t2, res, (curDir - 1) % 6, temp);
                //copy the value to the resource map (map is only used in the ai debug mode)
                resourceMaps[res][t2] = temp;
                if (temp > best_value)
                {
                    if (!nodes[n].reachable || (inTerritory && !aii.IsOwnTerritory(t2)) || nodes[n].farmed)
                    {
                        t2 = aii.GetNeighbour(t2, Direction(curDir));
                        continue;
                    }
                    if(HarborPosClose(t2, 3, true))
                    {
                        t2 = aii.GetNeighbour(t2, Direction(curDir));
                        continue;
                    }
                    BuildingQuality bq = aii.GetBuildingQuality(t2);
                    if (( (bq >= size && bq < BQ_MINE) // normales Gebäude
                            || (bq == size)) &&     // auch Bergwerke
							(res!=AIJH::BORDERLAND || !aii.IsRoadPoint(aii.GetNeighbour(t2, Direction::SOUTHEAST))))
					//special: military buildings cannot be build next to an existing road as that would have them connected to 2 roads which the ai no longer should do
                    {
                        best = t2;
                        best_value = temp;
                        //TODO: calculate "perfect" rating and instantly return if we got that already
                    }
                }
                t2 = aii.GetNeighbour(t2, Direction(curDir));
            }
        }
    }

    if (best_value >= minimum)
    {
        pt = best;
        return true;
    }
    return false;
}

void AIPlayerJH::UpdateNodesAround(const MapPoint pt, unsigned radius)
{
    UpdateReachableNodes(pt, radius);
}

void AIPlayerJH::UpdateNodesAroundNoBorder(const MapPoint pt, unsigned radius)
{
    UpdateReachableNodes(pt, radius);
}

void AIPlayerJH::ExecuteAIJob()
{
    // Check whether current job is finished...
    /*if (currentJob)
    {
        if (currentJob->GetStatus() == AIJH::JOB_FINISHED)
        {			
            delete currentJob;
            currentJob = 0;
        }
    }

    // ... or it failed
    if (currentJob)
    {
        if (currentJob->GetStatus() == AIJH::JOB_FAILED)
        {
            // TODO fehlerbehandlung?
            //std::cout << "Job failed." << std::endl;
            delete currentJob;
            currentJob = 0;
        }
    }*/
	unsigned quota = 10; //limit the amount of events to handle
	while (eventManager.EventAvailable() && quota) //handle all new events - some will add new orders but they can all be handled instantly
	{
		quota--;
        currentJob.reset(new AIJH::EventJob(*this, eventManager.GetEvent()));
		currentJob->ExecuteJob();
	}
	//how many construction & connect jobs the ai will attempt every gf, the ai gets new orders from events and every 200 gf
	quota = (aii.GetStorehouses().size() + aii.GetMilitaryBuildings().size()) * 1;
	if (quota>40)
		quota=40;

	construction->ExecuteJobs(quota); //try to execute up to quota connect & construction jobs
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

void AIPlayerJH::RecalcBQAround(const MapPoint  /*pt*/)
{
}

void AIPlayerJH::CheckNewMilitaryBuildings()
{
}

void AIPlayerJH::DistributeGoodsByBlocking(const GoodType good, unsigned limit)
{
    bool validgoalexists = false;
    const std::list<nobBaseWarehouse*>& storehouses = aii.GetStorehouses();
    if (aii.GetHarbors().size() < (storehouses.size() / 2)) //dont distribute on maps that are mostly sea maps - harbors are too difficult to defend and have to handle quite a lot of traffic already
    {
        for(std::list<nobBaseWarehouse*>::const_iterator it = storehouses.begin(); it != storehouses.end(); ++it)
        {
            if ((*it)->GetInventory().goods[good] <= limit)
            {
                validgoalexists = true;
                break;
            }
        }
    }
    if (!validgoalexists) // more than limit everywhere (or sea map) -> unblock everywhere
    {
        for(std::list<nobBaseWarehouse*>::const_iterator it = storehouses.begin(); it != storehouses.end(); ++it)
        {
            if((*it)->IsInventorySetting(good, EInventorySetting::STOP)) //not unblocked then issue command to unblock
                aii.SetInventorySetting((*it)->GetPos(), good, (*it)->GetInventorySetting(good).Toggle(EInventorySetting::STOP));
        }
    }
    else // valid goal exists -> block where at least limit goods are stored and unblock the others
    {
        for(std::list<nobBaseWarehouse*>::const_iterator it = storehouses.begin(); it != storehouses.end(); ++it)
        {
            if((*it)->GetInventory().goods[good] <= limit) //not at limit - unblock it
            {
                if((*it)->IsInventorySetting(good, EInventorySetting::STOP)) //not unblocked then issue command to unblock
                    aii.SetInventorySetting((*it)->GetPos(), good, (*it)->GetInventorySetting(good).Toggle(EInventorySetting::STOP));
            }
            else // at limit - block it
            {
                if(!(*it)->IsInventorySetting(good, EInventorySetting::STOP)) //not unblocked then issue command to unblock
                    aii.SetInventorySetting((*it)->GetPos(), good, (*it)->GetInventorySetting(good).Toggle(EInventorySetting::STOP));
            }
        }
    }
}
void AIPlayerJH::DistributeMaxRankSoldiersByBlocking(unsigned limit,nobBaseWarehouse* upwh)
{
    const std::list<nobBaseWarehouse*>& storehouses = aii.GetStorehouses();
	unsigned numCompleteWh=storehouses.size();
	
	if(numCompleteWh<1) //no warehouses -> no job
		return;

	Job maxRankJob = static_cast<Job>(JOB_PRIVATE + ggs.GetMaxMilitaryRank()); //private + general - max rank limiter
	
	if(numCompleteWh==1 ) //only 1 warehouse? dont block max ranks here
	{
        nobBaseWarehouse& wh = *storehouses.front();
		if (wh.IsInventorySetting(maxRankJob, EInventorySetting::STOP))
			aii.SetInventorySetting(wh.GetPos(), maxRankJob, wh.GetInventorySetting(maxRankJob).Toggle(EInventorySetting::STOP));
		return;
	}
	//rest applies for at least 2 complete warehouses!
	std::list<nobMilitary*>frontierMils; //make a list containing frontier military buildings
	for (std::list<nobMilitary*>::const_iterator it = aii.GetMilitaryBuildings().begin();it!=aii.GetMilitaryBuildings().end();++it)
	{
		if ((*it)->GetFrontierDistance()>0 && !(*it)->IsNewBuilt())
			frontierMils.push_back(*it);
	}
	std::list<nobBaseWarehouse*>frontierWhs; //make a list containing all warehouses near frontier military buildings
	for (std::list<nobBaseWarehouse*>::const_iterator it = storehouses.begin(); it!=storehouses.end();++it)
	{
		for (std::list<nobMilitary*>::const_iterator it2 = frontierMils.begin(); it2!=frontierMils.end(); ++it2)
		{	
			if(aii.CalcDistance((*it)->GetPos(),(*it2)->GetPos())<12)
			{
				frontierWhs.push_back(*it);
				break;
			}
		}		
	}
	//have frontier warehouses?
	if(!frontierWhs.empty())
	{
		//LOG.lprintf("distribute maxranks - got frontierwhs for player %i \n",playerid);
		bool hasUnderstaffedWh=false;
		//try to gather limit maxranks in each - if we have that many unblock for all frontier whs, 
		//check if there is at least one with less than limit first
		for (std::list<nobBaseWarehouse*>::const_iterator it=frontierWhs.begin();it!=frontierWhs.end();++it)
		{
			if((*it)->GetInventory().people[maxRankJob]<limit)
			{
				hasUnderstaffedWh=true;
				break;
			}
		}
		//if understaffed was found block in all with >=limit else unblock in all
		for (std::list<nobBaseWarehouse*>::const_iterator it=storehouses.begin();it!=storehouses.end();++it)
		{
            const nobBaseWarehouse& wh = **it;
            bool shouldBlock;
			if(helpers::contains(frontierWhs, *it)) //frontier wh?
			{
				if(hasUnderstaffedWh)
				{
					if(wh.GetInventory().people[maxRankJob]<limit)
                        shouldBlock = false;
					else //more than limit
                        shouldBlock = true;				
				}
				else //no understaffedwh
                    shouldBlock = false;
			}else //not frontier wh! block it
                shouldBlock = true;
            if(shouldBlock != wh.IsInventorySetting(maxRankJob, EInventorySetting::STOP))
                aii.SetInventorySetting(wh.GetPos(), maxRankJob, wh.GetInventorySetting(maxRankJob).Toggle(EInventorySetting::STOP));
		}
	}
	else //there are no frontier whs!
	{
		//LOG.lprintf("distribute maxranks - got NO frontierwhs for player %i \n",playerid);
		bool hasUnderstaffedWh=false;
		//try to gather limit maxranks in each - if we have that many unblock for all  whs, 
		//check if there is at least one with less than limit first
		for (std::list<nobBaseWarehouse*>::const_iterator it=storehouses.begin();it!=storehouses.end();++it)
		{
			if((*it)->GetInventory().people[maxRankJob]<limit && (*it)->GetPos() != upwh->GetPos()) // warehouse next to upgradebuilding is special case
			{
				hasUnderstaffedWh=true;
				break;
			}
		}
		for (std::list<nobBaseWarehouse*>::const_iterator it=storehouses.begin();it!=storehouses.end();++it)
		{
            const nobBaseWarehouse& wh = **it;
            bool shouldBlock;
			if(wh.GetPos() == upwh->GetPos()) // warehouse next to upgradebuilding should block when there is more than 1 wh
			{
				//LOG.lprintf("distribute maxranks - got NO frontierwhs for player %i , block at hq \n",playerid);
                shouldBlock = true;
            } else if(hasUnderstaffedWh)
            {

                if(wh.GetInventory().people[maxRankJob] < limit)
                    shouldBlock = false;
                else //more than limit
                    shouldBlock = true;
            } else //no understaffedwh
                shouldBlock = false;
            if(shouldBlock != wh.IsInventorySetting(maxRankJob, EInventorySetting::STOP))
                aii.SetInventorySetting(wh.GetPos(), maxRankJob, wh.GetInventorySetting(maxRankJob).Toggle(EInventorySetting::STOP));
        }
	}
}
bool AIPlayerJH::SimpleFindPosition(MapPoint& pt, BuildingQuality size, int radius)
{
    unsigned short width = aii.GetMapWidth();
    unsigned short height = aii.GetMapHeight();
    //if(size==BQ_HARBOR)
    //  Chat(_("looking for harbor"));

    if (pt.x >= width || pt.y >= height)
    {
        pt = aii.GetStorehouses().front()->GetPos();
    }

    // TODO was besseres wär schön ;)
    if (radius == -1)
        radius = 30;

    std::vector<MapPoint> pts = aii.GetPointsInRadius(pt, radius);
    for(std::vector<MapPoint>::const_iterator it = pts.begin(); it != pts.end(); ++it)
    {
        unsigned idx = aii.GetIdx(*it);

        if (!nodes[idx].reachable || nodes[idx].farmed || !aii.IsOwnTerritory(*it))
            continue;
        if(HarborPosClose(*it, 3, true))
        {
            if(size != BQ_HARBOR)
                continue;
        }
        BuildingQuality bq = aii.GetBuildingQuality(*it);
        if ( (bq >= size && bq < BQ_MINE) // normales Gebäude
                || (bq == size))    // auch Bergwerke
        {
            pt = *it;
            return true;
        }
    }

    return false;
}

unsigned AIPlayerJH::GetDensity(MapPoint pt, AIJH::Resource res, int radius)
{
    unsigned short width = aii.GetMapWidth();
    unsigned short height = aii.GetMapHeight();

    // TODO: check warum das so ist, und ob das sinn macht! ist so weil der punkt dann außerhalb der karte liegen würde ... könnte trotzdem crashen wenn wir kein hq mehr haben ... mehr checks!
    if (pt.x >= width || pt.y >= height)
    {
        pt = aii.GetStorehouses().front()->GetPos();
    }

    std::vector<unsigned> idxs = aii.GetPointsInRadius(pt, radius, MapPoint2Idx(aii));
    const unsigned all = idxs.size();
    RTTR_Assert(all > 0);

    unsigned good = 0;
    for(std::vector<unsigned>::const_iterator it = idxs.begin(); it != idxs.end(); ++it)
    {
        if (nodes[*it].res == res)
            good++;
    }

    return (good * 100) / all;
}

void AIPlayerJH::HandleNewMilitaryBuilingOccupied(const MapPoint pt)
{
    //kill bad flags we find
    RemoveAllUnusedRoads(pt);
    construction->RefreshBuildingCount();
    const nobMilitary* mil = aii.GetSpecObj<nobMilitary>(pt);
    if (mil)
    {
        if ((mil->GetBuildingType() == BLD_BARRACKS || mil->GetBuildingType() == BLD_GUARDHOUSE) && mil->GetFrontierDistance() == 0 && !mil->IsGoldDisabled())
        {
            aii.SetCoinsAllowed(pt, false);
        }

        // if near border and gold disabled (by addon): enable it
        if (mil->GetFrontierDistance() && mil->IsGoldDisabled())
        {
            aii.SetCoinsAllowed(pt, true);
        }
    }

    AddBuildJob(BLD_HARBORBUILDING, pt);
    if(!IsInvalidShipyardPosition(pt))
        AddBuildJob(BLD_SHIPYARD, pt);
    if (SoldierAvailable())
    {
        AddBuildJob(construction->ChooseMilitaryBuilding(pt), pt);
        AddBuildJob(construction->ChooseMilitaryBuilding(pt), pt);
        AddBuildJob(construction->ChooseMilitaryBuilding(pt), pt);
    }

    // try to build one the following buildings around the new military building

    BuildingType bldToTest[] =
    {
        BLD_STOREHOUSE,
        BLD_WOODCUTTER,
        BLD_QUARRY,
        BLD_GOLDMINE,
        BLD_COALMINE,
        BLD_IRONMINE,
        BLD_GRANITEMINE,
        BLD_FISHERY,
        BLD_FARM,
        BLD_HUNTER,
		BLD_FORESTER
    };
    unsigned numBldToTest = 0;
    //remove the storehouse from the building test list if we are close to another storehouse already
    for (std::list<nobBaseWarehouse*>::const_iterator it = aii.GetStorehouses().begin(); it != aii.GetStorehouses().end(); ++it)
    {
        if (gwb.CalcDistance((*it)->GetPos(), pt) < 20)
        {
            numBldToTest = 1;
            break;
        }
    }
    //same is true for warehouses which are still in production
    for(std::list<noBuildingSite*>::const_iterator it = aii.GetBuildingSites().begin(); it != aii.GetBuildingSites().end(); ++it)
    {
        if((*it)->GetBuildingType() == BLD_STOREHOUSE || (*it)->GetBuildingType() == BLD_HARBORBUILDING)
        {
            if (gwb.CalcDistance((*it)->GetPos(), pt) < 20)
            {
                numBldToTest = 1;
                break;
            }
        }
    }

    for (unsigned int i = numBldToTest; i < 11; ++i)
    {
        if (construction->Wanted(bldToTest[i]))
        {
            AddBuildJob(bldToTest[i], pt);
        }
    }
}

void AIPlayerJH::HandleBuilingDestroyed(MapPoint pt, BuildingType bld)
{
    switch (bld)
    {
        case BLD_FARM:
            SetFarmedNodes(pt, false);
            break;
        case BLD_HARBORBUILDING:
        {
            //destroy all other buildings around the harborspot in range 2 so we can rebuild the harbor ...
            std::vector<MapPoint> pts = aii.GetPointsInRadius(pt, 2);
            for(std::vector<MapPoint>::const_iterator it = pts.begin(); it != pts.end(); ++it)
            {
                const noBaseBuilding* const bb = aii.GetSpecObj<noBaseBuilding>(*it);
                if(bb)
                    aii.DestroyBuilding(*it);
                else
                {
                    const noBuildingSite* const bs = aii.GetSpecObj<noBuildingSite>(*it);
                    if(bs)
                        aii.DestroyFlag(aii.GetNeighbour(*it, Direction::SOUTHEAST));
                }
            }
            break;
        }
        default:
            break;
    }

}

void AIPlayerJH::HandleRoadConstructionComplete(MapPoint pt, unsigned char dir)
{
    //todo: detect "bad" roads and handle them
    const noFlag* flag;
    //does the flag still exist?
    if(!(flag = aii.GetSpecObj<noFlag>(pt)))
        return;
    //does the roadsegment still exist?
    RoadSegment* const roadSeg = flag->routes[dir];
    if(!roadSeg)
        return;
	if(roadSeg->GetLength()<4) //road too short to need flags
		return;
    //check if this road leads to a warehouseflag and if it does start setting flags from the warehouseflag else from the new flag
    //goal is to move roadsegments with a length of more than 2 away from the warehouse
    MapPoint t = gwb.GetNeighbour(roadSeg->GetOtherFlag(flag)->GetPos(), 1);
	construction->constructionlocations.push_back(t);
    if(aii.IsBuildingOnNode(t, BLD_STOREHOUSE) || aii.IsBuildingOnNode(t, BLD_HARBORBUILDING) || aii.IsBuildingOnNode(t, BLD_HEADQUARTERS))
    {
        t = gwb.GetNeighbour(t, 4);
        for(unsigned i = 0; i < roadSeg->GetLength(); ++i)
        {
            t = aii.GetNeighbour(t, Direction::fromInt(roadSeg->GetDir(true, i)));
            aii.SetFlag(t);
        }
    }
    else
    {
        //set flags on our new road starting from the new flag
        for(unsigned i = 0; i < roadSeg->GetLength(); ++i)
        {
            pt = aii.GetNeighbour(pt, Direction::fromInt(roadSeg->GetDir(false, i)));
            aii.SetFlag(pt);
        }
    }
}

void AIPlayerJH::HandleRoadConstructionFailed(const MapPoint pt, unsigned char  /*dir*/)
{
    const noFlag* flag;
    //does the flag still exist?
    if(!(flag = aii.GetSpecObj<noFlag>(pt)))
        return;
    //is it our flag?
    if(flag->GetPlayer() != playerid)
    {
        return;
    }
    //if it isnt a useless flag AND it has no current road connection then retry to build a road.
    if(RemoveUnusedRoad(flag, 255, true, false))
        construction->AddConnectFlagJob(flag);
}

void AIPlayerJH::HandleMilitaryBuilingLost(const MapPoint pt)
{
    if(aii.GetStorehouses().empty()) //check if we have a storehouse left - if we dont have one trying to find a path to one will crash
    {
        return;
    }
    RemoveAllUnusedRoads(pt);

}

void AIPlayerJH::HandleBuildingFinished(const MapPoint pt, BuildingType bld)
{
    switch(bld)
    {
        case BLD_HARBORBUILDING:
            UpdateNodesAround(pt, 8); // todo: fix radius
            RemoveAllUnusedRoads(pt); // repair & reconnect road system - required when a colony gets a new harbor by expedition
            aii.ChangeReserve(pt, 0, 1); //order 1 defender to stay in the harborbuilding

            //if there are positions free start an expedition!
            if(HarborPosRelevant(gwb.GetHarborPointID(pt), true))
            {
                aii.StartExpedition(pt);
            }
            break;

        case BLD_SHIPYARD:
            aii.ToggleShipYardMode(pt);
            break;

        case BLD_STOREHOUSE:
            break;
        case BLD_WOODCUTTER:
            AddBuildJob(BLD_SAWMILL, pt);
            break;
        default:
            break;
    }

}

void AIPlayerJH::HandleNewColonyFounded(const MapPoint pt)
{
    construction->AddConnectFlagJob(aii.GetSpecObj<noFlag>(aii.GetNeighbour(pt, Direction::SOUTHEAST)));
}

void AIPlayerJH::HandleExpedition(const noShip* ship)
{
    if(!ship->IsWaitingForExpeditionInstructions())
        return;
    if (ship->IsAbleToFoundColony())
        aii.FoundColony(ship);
    else
    {
        unsigned char start = rand() % ShipDirection::COUNT;
        for(unsigned char i = start; i < start + ShipDirection::COUNT; ++i)
        {
            if (aii.IsExplorationDirectionPossible(ship->GetPos(), ship->GetCurrentHarbor(), ShipDirection(i)))
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
    std::vector<noBase*> objs = aii.GetDynamicObjects(pt);
    const noShip* ship = NULL;

    for(std::vector<noBase*>::const_iterator it = objs.begin(); it != objs.end(); ++it)
    {
        if((*it)->GetGOT() == GOT_SHIP)
        {
            if(static_cast<noShip*>(*it)->GetPlayer() == playerid)
            {
                if (static_cast<noShip*>(*it)->IsWaitingForExpeditionInstructions())
                {
                    ship = static_cast<noShip*>(*it);
                    break;
                }
            }
        }
    }
    if (ship)
    {
        HandleExpedition(ship);
    }
}

void AIPlayerJH::HandleTreeChopped(const MapPoint pt)
{

    //std::cout << "Tree chopped." << std::endl;

    nodes[aii.GetIdx(pt)].reachable = true;

    UpdateNodesAround(pt, 3);

    int random = rand();

    if (random % 2 == 0)
        AddBuildJob(construction->ChooseMilitaryBuilding(pt), pt);
    else //if (random % 12 == 0)
        AddBuildJob(BLD_WOODCUTTER, pt);
}

void AIPlayerJH::HandleNoMoreResourcesReachable(const MapPoint pt, BuildingType bld)
{

    // Destroy old building (once)

    if (aii.IsObjectTypeOnNode(pt, NOP_BUILDING))
    {
        //keep 2 woodcutters for each forester even if they sometimes run out of trees
        if(bld == BLD_WOODCUTTER)
        {
            for (std::list<nobUsual*>::const_iterator it = aii.GetBuildings(BLD_FORESTER).begin(); it != aii.GetBuildings(BLD_FORESTER).end(); ++it)
            {
                //is the forester somewhat close?
                if(gwb.CalcDistance(pt, (*it)->GetPos()) < 6)
                    //then find it's 2 woodcutters
                {
                    unsigned maxdist = gwb.CalcDistance(pt, (*it)->GetPos());
                    char betterwoodcutters = 0;
                    for (std::list<nobUsual*>::const_iterator it2 = aii.GetBuildings(BLD_WOODCUTTER).begin(); it2 != aii.GetBuildings(BLD_WOODCUTTER).end() && betterwoodcutters < 2; ++it2)
                    {
                        //dont count the woodcutter in question
                        if(pt == (*it2)->GetPos())
                            continue;
                        //closer or equally close to forester than woodcutter in question?
                        if(gwb.CalcDistance((*it2)->GetPos(), (*it)->GetPos()) <= maxdist)
                            betterwoodcutters++;
                    }
                    //couldnt find 2 closer woodcutter -> keep it alive
                    if(betterwoodcutters < 2)
                        return;
                }
            }
        }
        aii.DestroyBuilding(pt);
		if(bld==BLD_FISHERY) //fishery cant find fish? set fish value at location to 0 so we dont have to calculate the value for this location again
			SetResourceMap(AIJH::FISH, pt, 0);
    }
    else
        return;
    UpdateNodesAround(pt, 11); // todo: fix radius
    RemoveUnusedRoad(aii.GetSpecObj<noFlag>(aii.GetNeighbour(pt, Direction::SOUTHEAST)), 1, true);

    // try to expand, maybe res blocked a passage
    AddBuildJob(construction->ChooseMilitaryBuilding(pt), pt);
    AddBuildJob(construction->ChooseMilitaryBuilding(pt), pt);

    // and try to rebuild the same building
    if(bld != BLD_HUNTER)
        AddBuildJob(bld,pt);

    // farm is always good!
    AddBuildJob(BLD_FARM, pt);
}

void AIPlayerJH::HandleShipBuilt(const MapPoint pt)
{
    // Stop building ships if reached a maximum (TODO: make variable)
    const std::list<nobUsual*>& shipyards = aii.GetBuildings(BLD_SHIPYARD);
    if (((aii.GetShipCount() > 6 || aii.GetShipCount() >= (3 * shipyards.size())) && GetCountofAIRelevantSeaIds() > 1) || (GetCountofAIRelevantSeaIds() < 2 && aii.GetShipCount() > gwb.GetHarborPointCount()))
    {
        unsigned mindist = 255;
        nobUsual* shipyard = NULL;
        for (std::list<nobUsual*>::const_iterator it = shipyards.begin(); it != shipyards.end(); ++it)
        {
            if(aii.CalcDistance((*it)->GetPos(), pt) < mindist)
            {
                mindist = aii.CalcDistance((*it)->GetPos(), pt);
                shipyard = *it;
            }
        }
        if(shipyard && mindist < 12)//might have been destroyed by now and anything further away than 12 should be wrong anyways
            aii.SetProductionEnabled(shipyard->GetPos(), false);
    }
}

void AIPlayerJH::HandleBorderChanged(const MapPoint pt)
{
    UpdateNodesAround(pt, 11); // todo: fix radius

    const nobMilitary* mil = aii.GetSpecObj<nobMilitary>(pt);
    if (mil)
    {
        if (mil->GetFrontierDistance() != 0 && mil->IsGoldDisabled())
        {
            aii.SetCoinsAllowed(pt, true);
        }
        if (mil->GetBuildingType() == BLD_BARRACKS || mil->GetBuildingType() == BLD_GUARDHOUSE)
        {
            AddBuildJob(construction->ChooseMilitaryBuilding(pt), pt);
        }
    }
}

void AIPlayerJH::MilUpgradeOptim()
{
	//do we have a upgrade building?
	int upb = UpdateUpgradeBuilding();
	int count=0;	
    const std::list<nobMilitary*>& militaryBuildings = aii.GetMilitaryBuildings();
	for (std::list<nobMilitary*>::const_iterator it = militaryBuildings.begin(); it != militaryBuildings.end(); ++it)
	{
		if (count!=upb) //not upgrade building
		{
			if(upb>=0) //we do have an upgrade building
			{
				if(!(*it)->IsGoldDisabled()) // deactivate gold for all other buildings
				{
					aii.SetCoinsAllowed((*it)->GetPos(), false);
				}
				if ((*it)->GetFrontierDistance()==0 && (((unsigned)count+PlannedConnectedInlandMilitary()) < militaryBuildings.size()) ) //send out troops until 1 private is left, then cancel road
				{
					if ((*it)->GetTroopsCount()>1) //more than 1 soldier remaining? -> send out order
					{
						aii.SendSoldiersHome((*it)->GetPos());
					}
					else if(!(*it)->IsNewBuilt()) //0-1 soldier remains and the building has had at least 1 soldier at some point and the building is not new on the list-> cancel road (and fix roadsystem if necessary)
					{
						RemoveUnusedRoad((*it)->GetFlag(),1,true,true,true);
					}
				}
				else if ((*it)->GetFrontierDistance()>=1) // frontier building - connect to road system
				{
					construction->AddConnectFlagJob((*it)->GetFlag());
				}
			}
			else //no upgrade building? -> activate gold for frontier buildings
			{
				if((*it)->IsGoldDisabled() && (*it)->GetFrontierDistance()>0) 
				{
					aii.SetCoinsAllowed((*it)->GetPos(), true);
				}
			}
		}
		else //upgrade building 
		{
			if(!construction->IsConnectedToRoadSystem((*it)->GetFlag()))
			{
				construction->AddConnectFlagJob((*it)->GetFlag());				
				continue;
			}
			if((*it)->IsGoldDisabled()) // activate gold
			{
				aii.SetCoinsAllowed((*it)->GetPos(), true);
			}
			if((*it)->HasMaxRankSoldier()) // has max rank soldier? send it/them out!
				aii.SendSoldiersHome((*it)->GetPos());
			if(SoldierAvailable(0) && (*it)->GetTroopsCount() < (unsigned)((*it)->GetBuildingType()==BLD_WATCHTOWER?TROOPS_COUNT[aii.GetNation()][2]:TROOPS_COUNT[aii.GetNation()][3])) //building not full and privates in a warehouse? order new!
				aii.OrderNewSoldiers((*it)->GetPos());
		}
		count++;
	}
}


void AIPlayerJH::Chat(const std::string& message)
{
    GameMessage_Server_Chat chat = GameMessage_Server_Chat(playerid, CD_ALL, message);
    GAMESERVER.AIChat(chat);
}

bool AIPlayerJH::HasFrontierBuildings()
{
	for (std::list<nobMilitary*>::const_iterator it = aii.GetMilitaryBuildings().begin(); it!=aii.GetMilitaryBuildings().end();++it)
	{
		if((*it)->GetFrontierDistance()>0)
			return true;
	}
	return false;
}

void AIPlayerJH::CheckExpeditions()
{
    const std::list<nobHarborBuilding*>& harbors = aii.GetHarbors();
    for(std::list<nobHarborBuilding*>::const_iterator it = harbors.begin(); it != harbors.end(); ++it)
    {
        if(((*it)->IsExpeditionActive() && !HarborPosRelevant((*it)->GetHarborPosID(), true)) || (!(*it)->IsExpeditionActive() && HarborPosRelevant((*it)->GetHarborPosID(), true))) //harbor is collecting for expedition and shouldnt OR not collecting and should -> toggle expedition
        {
            aii.StartExpedition((*it)->GetPos()); //command is more of a toggle despite it's name
        }
    }
    //find lost expedition ships - ai should get a notice and catch them all but just in case some fell through the system
    const std::vector<noShip*>& ships = aii.GetShips();
    for(std::vector<noShip*>::const_iterator it = ships.begin(); it != ships.end(); ++it)
    {
        if((*it)->IsWaitingForExpeditionInstructions())
            HandleExpedition(*it);
    }
}

void AIPlayerJH::CheckForester()
{
    const std::list<nobUsual*>& foresters = aii.GetBuildings(BLD_FORESTER);
    if(!foresters.empty() && foresters.size()<2 && aii.GetMilitaryBuildings().size()<3 && aii.GetBuildingSites().size()<3)
        //stop the forester
    {
        if(!(*foresters.begin())->IsProductionDisabled())
            aii.SetProductionEnabled(foresters.front()->GetPos(), false);
    }
    else //activate the forester 
    {
        if(!foresters.empty() && (*foresters.begin())->IsProductionDisabled())
            aii.SetProductionEnabled(foresters.front()->GetPos(), true);
    }
}

void AIPlayerJH::CheckGranitMine()
{
    //stop production in granite mines when the ai has many stones (100+ and at least 15 for each warehouse)
    if(AmountInStorage(GD_STONES,0)<100 || AmountInStorage(GD_STONES,0)<15*aii.GetStorehouses().size())
        //activate
    {
        for(std::list<nobUsual*>::const_iterator it=aii.GetBuildings(BLD_GRANITEMINE).begin();it!=aii.GetBuildings(BLD_GRANITEMINE).end();++it)
        {
            if((*it)->IsProductionDisabled())
                aii.SetProductionEnabled((*it)->GetPos(), true);
        }
    }
    else //deactivate
    {
        for(std::list<nobUsual*>::const_iterator it=aii.GetBuildings(BLD_GRANITEMINE).begin();it!=aii.GetBuildings(BLD_GRANITEMINE).end();++it)
        {
            if(!(*it)->IsProductionDisabled())
                aii.SetProductionEnabled((*it)->GetPos(), false);
        }
    }
}

void AIPlayerJH::TryToAttack()
{
    unsigned hq_or_harbor_without_soldiers = 0;
    std::vector<const nobBaseMilitary*> potentialTargets;

    // use own military buildings (except inland buildings) to search for enemy military buildings
    const std::list<nobMilitary*>& militaryBuildings = aii.GetMilitaryBuildings();
    const unsigned numMilBlds = militaryBuildings.size();
    //when the ai has many buildings the ai will not check the complete list every time
    BOOST_CONSTEXPR_OR_CONST unsigned limit = 40;
    for (std::list<nobMilitary*>::const_iterator it = militaryBuildings.begin(); it != militaryBuildings.end(); ++it)
    {
        // We skip the current building with a probability of limit/numMilBlds
        // -> For twice the number of blds as the limit we will most likely skip every 2nd building
        // This way we check roughly (at most) limit buildings but avoid any preference for one building over an other
        if(rand() % numMilBlds > limit)
            continue;

        const nobMilitary* mil = (*it);
        if (mil->GetFrontierDistance() == 0)  //inland building? -> skip it  
            continue;

        // get nearby enemy buildings and store in set of potential attacking targets
        MapPoint src = (*it)->GetPos();

        sortedMilitaryBlds buildings = aii.GetMilitaryBuildings(src, 2);
        for(sortedMilitaryBlds::iterator target = buildings.begin(); target != buildings.end(); ++target)
        {
            if(helpers::contains(potentialTargets, *target))
                continue;
            if ((*target)->GetGOT() == GOT_NOB_MILITARY && static_cast<const nobMilitary*>(*target)->IsNewBuilt())
                continue;
            MapPoint dest = (*target)->GetPos();
            if (gwb.CalcDistance(src, dest) < BASE_ATTACKING_DISTANCE && aii.IsPlayerAttackable((*target)->GetPlayer()) && aii.IsVisible(dest))
            {
                if ((*target)->GetGOT() != GOT_NOB_MILITARY && !(*target)->DefendersAvailable())
                {
                    // headquarter or harbor without any troops :)
                    hq_or_harbor_without_soldiers++;
                    potentialTargets.insert(potentialTargets.begin(), *target);
                }
                else
                    potentialTargets.push_back(*target);
            }
        }
    }

    // shuffle everything but headquarters and harbors without any troops in them
    std::random_shuffle(potentialTargets.begin() + hq_or_harbor_without_soldiers, potentialTargets.end());

    // check for each potential attacking target the number of available attacking soldiers
    for (std::vector<const nobBaseMilitary*>::iterator target = potentialTargets.begin(); target != potentialTargets.end(); ++target)
    {
        const MapPoint dest = (*target)->GetPos();

        unsigned attackersCount = 0;
        unsigned attackersStrength = 0;

        // ask each of nearby own military buildings for soldiers to contribute to the potential attack
        sortedMilitaryBlds myBuildings = aii.GetMilitaryBuildings(dest, 2);
        for(sortedMilitaryBlds::iterator it3 = myBuildings.begin(); it3 != myBuildings.end(); ++it3)
        {
            if ((*it3)->GetPlayer() == playerid)
            {
                const nobMilitary* myMil = dynamic_cast<const nobMilitary*>(*it3);
                if (!myMil || myMil->IsUnderAttack())
                    continue;

                unsigned newAttackers;
                attackersStrength += myMil->GetSoldiersStrengthForAttack(dest, playerid, newAttackers);
                attackersCount += newAttackers;
            }
        }

        if (attackersCount == 0)
            continue;

        if ((level == AI::HARD) && ((*target)->GetGOT() == GOT_NOB_MILITARY))
        {
            const nobMilitary* enemyTarget = static_cast<const nobMilitary*>(*target);
            if (attackersStrength <= enemyTarget->GetSoldiersStrength() || enemyTarget->GetTroopsCount() == 0)
                continue;
        }

        aii.Attack(dest, attackersCount, true);
        return;
    }
}

void AIPlayerJH::TrySeaAttack()
{
    if(aii.GetShipCount() < 1)
        return;
    if(aii.GetHarbors().empty())
        return;
    std::vector<unsigned short>seaidswithattackers;
    std::vector<unsigned int>attackersatseaid;
    std::vector<int> invalidseas;
    std::deque<const nobBaseMilitary*> potentialTargets;
    std::deque<const nobBaseMilitary*> undefendedTargets;
    std::vector<int> searcharoundharborspots;
    //all seaids with at least 1 ship count available attackers for later checks
    for(std::vector<noShip*>::const_iterator it = aii.GetShips().begin(); it != aii.GetShips().end(); ++it)
    {
        //sea id not already listed as valid or invalid?
        if(!helpers::contains(seaidswithattackers, (*it)->GetSeaID()) && !helpers::contains(invalidseas, (*it)->GetSeaID()))
        {
            unsigned int attackercount = gwb.GetAvailableSoldiersForSeaAttackAtSea(playerid, (*it)->GetSeaID(), false);
            if(attackercount) //got attackers at this sea id? -> add to valid list
            {
                seaidswithattackers.push_back((*it)->GetSeaID());
                attackersatseaid.push_back(attackercount);
            }
            else //not listed but no attackers? ->invalid
            {
                invalidseas.push_back((*it)->GetSeaID());
            }
        }
    }
    if(seaidswithattackers.empty()) //no sea ids with attackers? skip the rest
        return;
    /*else
    {
        for(unsigned i=0;i<seaidswithattackers.size();i++)
            LOG.lprintf("attackers at sea ids for player %i, sea id %i, count %i \n",playerid, seaidswithattackers[i], attackersatseaid[i]);
    }*/
    //first check all harbors there might be some undefended ones - start at 1 to skip the harbor dummy
    for(unsigned i = 1; i < gwb.GetHarborPointCount(); i++)
    {
        const nobHarborBuilding* hb;
        if((hb = aii.GetSpecObj<nobHarborBuilding>(gwb.GetHarborPoint(i))))
        {
            if(aii.IsVisible(hb->GetPos()))
            {
                if(aii.IsPlayerAttackable(hb->GetPlayer()))
                {
                    //attackers for this building?
                    std::vector<unsigned short> testseaidswithattackers(seaidswithattackers);
                    gwb.GetValidSeaIDsAroundMilitaryBuildingForAttackCompare(gwb.GetHarborPoint(i), testseaidswithattackers, playerid);
                    if(!testseaidswithattackers.empty()) //harbor can be attacked?
                    {
                        if(!hb->DefendersAvailable()) //no defenders?
                            undefendedTargets.push_back(hb);
                        else //todo: maybe only attack this when there is a fair win chance for the attackers?
                            potentialTargets.push_back(hb);
                        //LOG.lprintf("found a defended harbor we can attack at %i,%i \n",hb->GetPos());
                    }
                }
                else//cant attack player owning the harbor -> add to list
                {
                    searcharoundharborspots.push_back(i);
                }
            }
            //else: not visible for player no need to look any further here
        }
        else//no harbor -> add to list
        {
            searcharoundharborspots.push_back(i);
            //LOG.lprintf("found an unused harborspot we have to look around of at %i,%i \n",gwb.GetHarborPoint(i).x,gwb.GetHarborPoint(i).y);
        }
    }
    //any undefendedTargets? -> pick one by random
    if(!undefendedTargets.empty())
    {
        std::random_shuffle(undefendedTargets.begin(), undefendedTargets.end());
        for(std::deque<const nobBaseMilitary*>::iterator it = undefendedTargets.begin(); it != undefendedTargets.end(); ++it)
        {
            std::vector<GameWorldBase::PotentialSeaAttacker> attackers = gwb.GetAvailableSoldiersForSeaAttack(playerid, (*it)->GetPos());
            if(!attackers.empty()) //try to attack it!
            {
                aii.SeaAttack((*it)->GetPos(), 1, true);
                return;
            }
        }
    }
    //add all military buildings around still valid harborspots (unused or used by ally)
    unsigned limit = 15;
    unsigned skip = 0;
    if(searcharoundharborspots.size() > 15)
        skip = max<int>(rand() % (searcharoundharborspots.size() / 15 + 1) * 15, 1) - 1;
    for(unsigned i = skip; i < searcharoundharborspots.size() && limit > 0 ; i++)
    {
        limit--;
        //now add all military buildings around the harborspot to our list of potential targets
        sortedMilitaryBlds buildings = aii.GetMilitaryBuildings(gwb.GetHarborPoint(searcharoundharborspots[i]), 2);
        for(sortedMilitaryBlds::const_iterator it = buildings.begin(); it != buildings.end(); ++it)
        {
            if(aii.IsPlayerAttackable((*it)->GetPlayer()) && aii.IsVisible((*it)->GetPos()))
            {
                const nobMilitary* enemyTarget = dynamic_cast<const nobMilitary*>((*it));

                if (enemyTarget && enemyTarget->IsNewBuilt())
                    continue;
                if (((*it)->GetGOT() != GOT_NOB_MILITARY) && (!(*it)->DefendersAvailable())) //undefended headquarter(or unlikely as it is a harbor...) - priority list!
                {
                    std::vector<unsigned short> testseaidswithattackers(seaidswithattackers);
                    gwb.GetValidSeaIDsAroundMilitaryBuildingForAttackCompare((*it)->GetPos(), testseaidswithattackers, playerid);
                    if(!testseaidswithattackers.empty())
                    {
                        undefendedTargets.push_back(*it);
                    }//else - no attackers - do nothing
                }
                else //normal target - check is done after random shuffle so we dont have to check every possible target and instead only enough to get 1 good one
                {
                    potentialTargets.push_back(*it);
                }
            } //not attackable or no vision of region - do nothing
        }
    }
    //now we have a deque full of available and maybe undefended targets that are available for attack -> shuffle and attack the first one we can attack("should" be the first we check...)
    //any undefendedTargets? -> pick one by random
    if(!undefendedTargets.empty())
    {
        std::random_shuffle(undefendedTargets.begin(), undefendedTargets.end());
        for(std::deque<const nobBaseMilitary*>::iterator it = undefendedTargets.begin(); it != undefendedTargets.end(); ++it)
        {
            std::vector<GameWorldBase::PotentialSeaAttacker> attackers = gwb.GetAvailableSoldiersForSeaAttack(playerid, (*it)->GetPos());
            if(!attackers.empty()) //try to attack it!
            {
                aii.SeaAttack((*it)->GetPos(), 1, true);
                return;
            }
        }
    }
    std::random_shuffle(potentialTargets.begin(), potentialTargets.end());
    for(std::deque<const nobBaseMilitary*>::iterator it = potentialTargets.begin(); it != potentialTargets.end(); ++it)
    {
        std::vector<unsigned short> testseaidswithattackers(seaidswithattackers); //TODO: decide if it is worth attacking the target and not just "possible"
        gwb.GetValidSeaIDsAroundMilitaryBuildingForAttackCompare((*it)->GetPos(), testseaidswithattackers, playerid); //test only if we should have attackers from one of our valid sea ids
        if(!testseaidswithattackers.empty()) //only do the final check if it will probably be a good result
        {
            std::vector<GameWorldBase::PotentialSeaAttacker> attackers = gwb.GetAvailableSoldiersForSeaAttack(playerid, (*it)->GetPos()); //now get a final list of attackers and attack it
            if(!attackers.empty())
            {
                aii.SeaAttack((*it)->GetPos(), attackers.size(), true);
                return;
            }
        }
    }
}

void AIPlayerJH::RecalcGround(const MapPoint buildingPos, std::vector<unsigned char> &route_road)
{
    MapPoint pt = buildingPos;

    // building itself
    RecalcBQAround(pt);
    if (GetAINode(pt).res == AIJH::PLANTSPACE)
    {
        resourceMaps[AIJH::PLANTSPACE].Change(pt, -1);
        GetAINode(pt).res = AIJH::NOTHING;
    }

    // flag of building
    pt = aii.GetNeighbour(pt,Direction::SOUTHEAST);
    RecalcBQAround(pt);
    if (GetAINode(pt).res == AIJH::PLANTSPACE)
    {
        resourceMaps[AIJH::PLANTSPACE].Change(pt, -1);
        GetAINode(pt).res = AIJH::NOTHING;
    }

    // along the road
    for (unsigned i = 0; i < route_road.size(); ++i)
    {
        pt = aii.GetNeighbour(pt, Direction::fromInt(route_road[i]));
        RecalcBQAround(pt);
        // Auch Plantspace entsprechend anpassen:
        if (GetAINode(pt).res == AIJH::PLANTSPACE)
        {
            resourceMaps[AIJH::PLANTSPACE].Change(pt, -1);
            GetAINode(pt).res = AIJH::NOTHING;
        }
    }
}

void AIPlayerJH::SaveResourceMapsToFile()
{
#ifdef DEBUG_AI
    for(unsigned i = 0; i < AIJH::RES_TYPE_COUNT; ++i)
    {
        std::stringstream ss;
        ss << "resmap-" << i << ".log";
        FILE* file = fopen(ss.str().c_str(), "w");
        for (unsigned y = 0; y < aii.GetMapHeight(); ++y)
        {
            if (y % 2 == 1)
                fprintf(file, "  ");
            for (unsigned x = 0; x < aii.GetMapWidth(); ++x)
            {
                fprintf(file, "%i   ", resourceMaps[i][aii.GetIdx(MapPoint(x, y))]);
            }
            fprintf(file, "\n");
        }
        fclose(file);
    }
#endif
}

int AIPlayerJH::GetResMapValue(const MapPoint pt, AIJH::Resource res)
{
    return resourceMaps[res][pt];
}

void AIPlayerJH::SendAIEvent(AIEvent::Base* ev)
{
    eventManager.AddAIEvent(ev);
}

bool AIPlayerJH::IsFlagPartofCircle(const noFlag* startFlag, unsigned maxlen, const noFlag* curFlag, unsigned char excludeDir, bool init, std::vector<MapPoint> oldFlags)
{
    if(!init && startFlag == curFlag)
        return true;
    if(maxlen < 1)
        return false;
    bool partofcircle = false;
    unsigned testdir = 0;
    while(testdir < 6 && !partofcircle)
    {
        if (testdir == excludeDir)
        {
            testdir++;
            continue;
        }
        if(testdir == 1 && (aii.IsObjectTypeOnNode(aii.GetNeighbour(curFlag->GetPos(), Direction::NORTHWEST), NOP_BUILDING) || aii.IsObjectTypeOnNode(aii.GetNeighbour(curFlag->GetPos(), Direction::NORTHWEST), NOP_BUILDINGSITE)))
        {
            testdir++;
            continue;
        }
        if(curFlag->routes[testdir])
        {
            const noFlag* flag = curFlag->routes[testdir]->GetOtherFlag(curFlag);
            if (!flag)
                return(false);

            bool alreadyinlist = helpers::contains(oldFlags, flag->GetPos());
            if(!alreadyinlist)
            {
                oldFlags.push_back(flag->GetPos());
                partofcircle = IsFlagPartofCircle(startFlag, maxlen - 1, flag, (curFlag->routes[testdir]->GetOtherFlagDir(curFlag) + 3) % 6, false, oldFlags);
            }
        }
        testdir++;
    }
    return partofcircle;
}

void AIPlayerJH::RemoveAllUnusedRoads(const MapPoint pt)
{
    std::vector<const noFlag*> flags = construction->FindFlags(pt, 25);
    // Jede Flagge testen...
    std::list<const noFlag*>reconnectflags;
    for(unsigned i = 0; i < flags.size(); ++i)
    {
        if(RemoveUnusedRoad(flags[i], 255, true, false))
            reconnectflags.push_back(flags[i]);
    }
    UpdateNodesAroundNoBorder(pt, 25);
    while(!reconnectflags.empty())
    {
        construction->AddConnectFlagJob(reconnectflags.front());
        reconnectflags.pop_front();
    }
}

bool AIPlayerJH::RemoveUnusedRoad(const noFlag* startFlag, unsigned char excludeDir, bool firstflag, bool allowcircle,bool keepstartflag)
{
    unsigned char foundDir = 0xFF;
    unsigned char foundDir2 = 0xFF;
    unsigned char finds = 0;
    // Count roads from this flag...
    for (unsigned char dir = 0; dir < 6; ++dir)
    {
        if (dir == excludeDir)
            continue;
        if(dir == 1 && (aii.IsObjectTypeOnNode(aii.GetNeighbour(startFlag->GetPos(), Direction::NORTHWEST), NOP_BUILDING) || aii.IsObjectTypeOnNode(aii.GetNeighbour(startFlag->GetPos(), Direction::NORTHWEST), NOP_BUILDINGSITE)))
        {
            //the flag belongs to a building - update the pathing map around us and try to reconnect it (if we cant reconnect it -> burn it(burning takes place at the pathfinding job))
            return true;
        }
        if(startFlag->routes[dir])
        {
            finds++;
            if(finds == 1)
                foundDir = dir;
            else if(finds == 2)
                foundDir2 = dir;
        }
    }
    // if we found more than 1 road -> the flag is still in use.
    if (finds > 2)
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
        }
        else
            return false;
    }

    // kill the flag
	if(keepstartflag)
	{
		if(foundDir<6)
			aii.DestroyRoad(startFlag->GetPos(),foundDir);
	}
	else
		aii.DestroyFlag(startFlag);

    // nothing found?
    if (foundDir > 6)
        return false;
    // at least 1 road exists
    RemoveUnusedRoad(startFlag->routes[foundDir]->GetOtherFlag(startFlag), (startFlag->routes[foundDir]->GetOtherFlagDir(startFlag) + 3) % 6, false);
    // 2 roads exist
    if(foundDir2 != 0xFF)
        RemoveUnusedRoad(startFlag->routes[foundDir2]->GetOtherFlag(startFlag), (startFlag->routes[foundDir2]->GetOtherFlagDir(startFlag) + 3) % 6, false);
    return false;
}

unsigned AIPlayerJH::SoldierAvailable(int rank)
{
    unsigned freeSoldiers = 0;
    for (std::list<nobBaseWarehouse*>::const_iterator it = aii.GetStorehouses().begin(); it != aii.GetStorehouses().end(); ++it)
    {
		if(rank<0 || rank>4)
        {
            const Inventory& inventory = (*it)->GetInventory();
            freeSoldiers += (inventory.people[JOB_PRIVATE] + inventory.people[JOB_PRIVATEFIRSTCLASS] + inventory.people[JOB_SERGEANT] + inventory.people[JOB_OFFICER] + inventory.people[JOB_GENERAL]);
        }
		else
			freeSoldiers += ((*it)->GetInventory().people[rank+21]);
    }
    return freeSoldiers;
}

bool AIPlayerJH::HuntablesinRange(const MapPoint pt, unsigned min)
{
    //check first if no other hunter(or hunter buildingsite) is nearby
    if(BuildingNearby(pt, BLD_HUNTER, 15))
        return false;
    unsigned maxrange = 25;
    unsigned short fx, fy, lx, ly;
    const unsigned short SQUARE_SIZE = 19;
    unsigned huntablecount = 0;
    if(pt.x > SQUARE_SIZE) fx = pt.x - SQUARE_SIZE; else fx = 0;
    if(pt.y > SQUARE_SIZE) fy = pt.y - SQUARE_SIZE; else fy = 0;
    if(pt.x + SQUARE_SIZE < aii.GetMapWidth()) lx = pt.x + SQUARE_SIZE; else lx = aii.GetMapWidth() - 1;
    if(pt.y + SQUARE_SIZE < aii.GetMapHeight()) ly = pt.y + SQUARE_SIZE; else ly = aii.GetMapHeight() - 1;
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
            for(std::list<noBase*>::const_iterator it = figures.begin(); it != figures.end(); ++it)
            {
                if((*it)->GetType() == NOP_ANIMAL)
                {
                    // Ist das Tier überhaupt zum Jagen geeignet?
                    if(!static_cast<noAnimal*>(*it)->CanHunted())
                        continue;
                    // Und komme ich hin?
                    if(gwb.FindHumanPath(pt, static_cast<noAnimal*>(*it)->GetPos(), maxrange) != 0xFF)
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
    for(std::list<nobUsual*>::const_iterator it = aii.GetBuildings(BLD_FARM).begin(); it != aii.GetBuildings(BLD_FARM).end(); ++it)
    {
        SetFarmedNodes((*it)->GetPos(), true);
    }
    for(std::list<nobUsual*>::const_iterator it = aii.GetBuildings(BLD_CHARBURNER).begin(); it != aii.GetBuildings(BLD_CHARBURNER).end(); ++it)
    {
        SetFarmedNodes((*it)->GetPos(), true);
    }
	//find the upgradebuilding
    UpgradeBldPos = MapPoint(0, 0);
	UpdateUpgradeBuilding();
}
int AIPlayerJH::UpdateUpgradeBuilding()
{
	std::list<nobMilitary*> backup;
	if(!aii.GetStorehouses().empty())
	{		
		unsigned count=0;
		for (std::list<nobMilitary*>::const_iterator it = aii.GetMilitaryBuildings().begin(); it!=aii.GetMilitaryBuildings().end(); ++it)
		{
			//inland building, tower or fortress, connected to warehouse 1 
			if((*it)->GetBuildingType()>=BLD_WATCHTOWER && (*it)->GetFrontierDistance()<1 && construction->IsConnectedToRoadSystem((*it)->GetFlag()))
			{
				if (construction->IsConnectedToRoadSystem((*it)->GetFlag()))
				{
					//LOG.lprintf("UpdateUpgradeBuilding at %i,%i for player %i (listslot %i) \n",(*it)->GetX(), (*it)->GetY(), playerid, count);
                    UpgradeBldPos = (*it)->GetPos();
					UpgradeBldListNumber=count;
					return count;
				}
				backup.push_back(*it);
				
			}
			count++;
		}
	}
	//no valid upgrade building yet - try to reconnect correctly flagged buildings
	for (std::list<nobMilitary*>::const_iterator it=backup.begin();it!=backup.end();++it)
	{
		construction->AddConnectFlagJob((*it)->GetFlag());
	}
    UpgradeBldPos = MapPoint(0, 0);
	UpgradeBldListNumber=-1;
	return -1;
}
//set default start values for the ai for distribution & military settings
void AIPlayerJH::InitDistribution()
{	
        //set good distribution settings
        Distributions goodSettings;
        goodSettings[0] = 10; //food granite
        goodSettings[1] = 10; //food coal
        goodSettings[2] = 10; //food iron
        goodSettings[3] = 10; //food gold

        goodSettings[4] = 10; //grain mill
        goodSettings[5] = 10; //grain pigfarm
        goodSettings[6] = 10; //grain donkeybreeder
        goodSettings[7] = 10; //grain brewery
        goodSettings[8] = 10; //grain charburner

        goodSettings[9] = 10; //iron armory
        goodSettings[10] = 10; //iron metalworks

        goodSettings[11] = 10; //coal armory
        goodSettings[12] = 10; //coal ironsmelter
        goodSettings[13] = 10; //coal mint

        goodSettings[14] = 10; //wood sawmill
        goodSettings[15] = 10; //wood charburner

        goodSettings[16] = 10; //boards new buildings
        goodSettings[17] = 4; //boards metalworks
        goodSettings[18] = 2; //boards shipyard

        goodSettings[19] = 10; //water bakery
        goodSettings[20] = 10; //water brewery
        goodSettings[21] = 10; //water pigfarm
        goodSettings[22] = 10; //water donkeybreeder
        aii.ChangeDistribution(goodSettings);
}

bool AIPlayerJH::ValidTreeinRange(const MapPoint pt)
{
    unsigned max_radius = 6;
    for(MapCoord tx = gwb.GetXA(pt, 0), r = 1; r <= max_radius; tx = gwb.GetXA(tx, pt.y, 0), ++r)
    {
        MapPoint t2(tx, pt.y);
        for(unsigned i = 2; i < 8; ++i)
        {
            for(MapCoord r2 = 0; r2 < r; t2 = gwb.GetNeighbour(t2, i % 6), ++r2)
            {
                //point has tree & path is available?
                if(gwb.GetNO(t2)->GetType() == NOP_TREE)
                {
                    //not already getting cut down or a freaking pineapple thingy?
                    if (!gwb.GetNode(t2).reserved && (gwb.GetSpecObj<noTree>(t2))->type != 5)
                    {
                        if(gwb.FindHumanPath(pt, t2, 20) != 0xFF)
                            return true;;
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
    for(MapCoord tx = gwb.GetXA(pt, 0), r = 1; r <= max_radius; tx = gwb.GetXA(tx, pt.y, 0), ++r)
    {
        MapPoint t2(tx, pt.y);
        for(unsigned i = 2; i < 8; ++i)
        {
            for(MapCoord r2 = 0; r2 < r; t2 = gwb.GetNeighbour(t2, i % 6), ++r2)
            {
                //point has tree & path is available?
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

void AIPlayerJH::ExecuteLuaConstructionOrder(const MapPoint pt, BuildingType bt,bool forced)
{
	if (!aii.CanBuildBuildingtype(bt)) // not allowed to build this buildingtype? -> do nothing!
		return;
	if (forced) //fixed location - just a direct gamecommand to build buildingtype at location (no checks if this is a valid & good location from the ai)
	{
		aii.SetBuildingSite(pt, bt);
		AIJH::BuildJob* j=new AIJH::BuildJob(*this, bt, pt);
		j->SetStatus(AIJH::JOB_EXECUTING_ROAD1);
		j->SetTarget(pt);
		construction->AddBuildJob(j,true); //connects the buildingsite to roadsystem
	}
	else 
	{
		if (construction->Wanted(bt))
		{
			construction->AddBuildJob(new AIJH::BuildJob(*this, bt, pt), true); //add build job to the front of the list
		}
	}
}

bool AIPlayerJH::BuildingNearby(const MapPoint pt, BuildingType bld, unsigned min)
{
    //assert not a military building
    RTTR_Assert(bld >= 10);
    for(std::list<nobUsual*>::const_iterator it = aii.GetBuildings(bld).begin(); it != aii.GetBuildings(bld).end(); ++it)
    {
        if(gwb.CalcDistance(pt, (*it)->GetPos()) < min)
            return true;
    }
    for(std::list<noBuildingSite*>::const_iterator it = aii.GetBuildingSites().begin(); it != aii.GetBuildingSites().end(); ++it)
    {
        if((*it)->GetBuildingType() == bld)
        {
            if(gwb.CalcDistance(pt, (*it)->GetPos()) < min)
                return true;
        }
    }
    return false;
}

bool AIPlayerJH::HarborPosClose(const MapPoint pt, unsigned range, bool onlyempty)
{
    //skip harbordummy ... ask oliver why there has to be a dummy
    for (unsigned i = 1; i <= gwb.GetHarborPointCount(); i++)
    {
        if(gwb.CalcDistance(pt, gwb.GetHarborPoint(i)) < range && HarborPosRelevant(i)) //in range and valid for ai - as in actually at a sea with more than 1 harbor spot
        {
            if(!onlyempty || !aii.IsBuildingOnNode(gwb.GetHarborPoint(i), BLD_HARBORBUILDING))
                return true;
        }
    }
    return false;
}

/// returns the percentage*100 of possible normal+ building places
unsigned AIPlayerJH::BQsurroundcheck(const MapPoint pt, unsigned range, bool includeexisting,unsigned limit)
{	
	unsigned maxvalue=6*(2<<(range-1))-5; //1,7,19,43,91,... = 6*2^range -5
	unsigned count=0;
	if (( aii.GetBuildingQuality(pt)>=BQ_HUT && aii.GetBuildingQuality(pt) <= BQ_CASTLE) || aii.GetBuildingQuality(pt) == BQ_HARBOR)
    {
		count++;
    }
	NodalObjectType nob = gwb.GetNO(pt)->GetType();
	if (includeexisting)
	{
		if ( nob==NOP_BUILDING || nob==NOP_BUILDINGSITE ||nob==NOP_EXTENSION || nob==NOP_FIRE || nob==NOP_CHARBURNERPILE )
			count++;
	}	
	//first count all the possible building places
    for(MapCoord tx = gwb.GetXA(pt, 0), r = 1; r <= range; tx = gwb.GetXA(tx, pt.y, 0), ++r)
    {
        MapPoint t2(tx, pt.y);
        for(unsigned i = 2; i < 8; ++i)
        {
            for(MapCoord r2 = 0; r2 < r; t2 = gwb.GetNeighbour(t2, i % 6), ++r2)
            {
				if (limit && ((count*100)/maxvalue) > limit)
					return ((count*100)/maxvalue);
                //point can be used for a building
                if (( aii.GetBuildingQualityAnyOwner(t2)>=BQ_HUT && aii.GetBuildingQualityAnyOwner(t2) <= BQ_CASTLE) || aii.GetBuildingQualityAnyOwner(t2) == BQ_HARBOR)
                {
                    count++;
					continue;
                }
				if (includeexisting)
				{
					nob = gwb.GetNO(t2)->GetType();
					if ( nob==NOP_BUILDING || nob==NOP_BUILDINGSITE ||nob==NOP_EXTENSION || nob==NOP_FIRE || nob==NOP_CHARBURNERPILE )
						count++;
				}				
			}
        }
    }	
	//LOG.lprintf("bqcheck at %i,%i r%u result: %u,%u \n",pt,range,count,maxvalue);
    return ((count*100)/maxvalue);
}

bool AIPlayerJH::HarborPosRelevant(unsigned harborid, bool onlyempty)
{
    if(harborid < 1 || harborid > gwb.GetHarborPointCount()) //not a real harbor - shouldnt happen...
    {
        RTTR_Assert(false);
        return false;
    }

    for(unsigned r = 0; r < 6; r++)
    {
        const unsigned short seaId = gwb.GetSeaId(harborid, Direction::fromInt(r));
        if(!seaId)
            continue;

        for(unsigned i = 1; i <= gwb.GetHarborPointCount(); i++) //start at 1 harbor dummy yadayada :>
        {
            if(i != harborid && gwb.IsAtThisSea(i, seaId))
            {
                if(onlyempty) //check if the spot is actually free for colonization?
                {
                    if(gwb.IsHarborPointFree(i, playerid, seaId))
                        return true;
                }
                else
                    return true;
            }
        }
    }
    return false;
}

bool AIPlayerJH::NoEnemyHarbor()
{
    for(unsigned i = 1; i <= gwb.GetHarborPointCount(); i++)
    {
        if(aii.IsBuildingOnNode(gwb.GetHarborPoint(i), BLD_HARBORBUILDING) && !aii.IsOwnTerritory(gwb.GetHarborPoint(i)))
        {
            //LOG.lprintf("found a harbor at spot %i ",i);
            return false;
        }
    }
    return true;
}

bool AIPlayerJH::IsInvalidShipyardPosition(const MapPoint pt)
{
    return BuildingNearby(pt, BLD_SHIPYARD, 20) || !HarborPosClose(pt, 8);

}

unsigned AIPlayerJH::AmountInStorage(unsigned char num,unsigned char page)
{
	unsigned counter=0;
	for(std::list<nobBaseWarehouse*>::const_iterator it=aii.GetStorehouses().begin();it!=aii.GetStorehouses().end();++it)
	{
		if(page==0)
			counter+=(*it)->GetInventory().goods[num];
		else
			counter+=(*it)->GetInventory().people[num];
	}
	return counter;
}

bool AIPlayerJH::ValidFishInRange(const MapPoint pt)
{
    unsigned max_radius = 5;
    for(MapCoord tx = gwb.GetXA(pt, 0), r = 1; r <= max_radius; tx = gwb.GetXA(tx, pt.y, 0), ++r)
    {
        MapPoint t2(tx, pt.y);
        for(unsigned i = 2; i < 8; ++i)
        {
            for(MapCoord r2 = 0; r2 < r; t2 = gwb.GetNeighbour(t2, i % 6), ++r2)
            {
                if(gwb.GetNode(t2).resources > 0x80 && gwb.GetNode(t2).resources < 0x90) //fish on current spot?
                {
                    //LOG.lprintf("found fish at %i,%i ",t2);
                    //try to find a path to a neighboring node on the coast
                    for(int j = 0; j < 6; j++)
                    {
                        if(gwb.FindHumanPath(pt, gwb.GetNeighbour(t2, j), 10) != 0xFF)
                            return true;
                    }
                }
            }
        }
    }
    return false;
}

unsigned AIPlayerJH::GetCountofAIRelevantSeaIds()
{
    std::list<unsigned short>validseaids;
    std::list<unsigned short>onetimeuseseaids;
    for(unsigned i = 1; i <= gwb.GetHarborPointCount(); i++)
    {
        for(unsigned r = 0; r < 6; r++)
        {
            const unsigned short seaId = gwb.GetSeaId(i, Direction::fromInt(r));
            if(!seaId)
                continue;
            //there is a sea id? -> check if it is already a validid or a once found id
            if(!helpers::contains(validseaids, seaId)) //not yet in validseas?
            {
                if(!helpers::contains(onetimeuseseaids, seaId)) //not yet in onetimeuseseaids?
                    onetimeuseseaids.push_back(seaId);
                else
                {
                    //LOG.lprintf("found a second harbor at sea id %i \n",sea_ids[r]);
                    onetimeuseseaids.remove(seaId);
                    validseaids.push_back(seaId);
                }
            }
            else
                continue;
        }
    }
    return validseaids.size();
}

void AIPlayerJH::AdjustSettings()
{
	//update tool creation settings
    ToolSettings toolsettings;
    const Inventory& inventory = aii.GetInventory();
    toolsettings[2] = (inventory.goods[GD_SAW] + inventory.people[JOB_CARPENTER] < 2) ? 4 : inventory.goods[GD_SAW] < 1 ? 1 : 0;                                                                       //saw
    toolsettings[3] = (inventory.goods[GD_PICKAXE] < 1) ? 1 : 0;                                                                                                                     //pickaxe
    toolsettings[4] = (inventory.goods[GD_HAMMER] < 1) ? 1 : 0;                                                                                                                      //hammer
    toolsettings[6] = (inventory.goods[GD_CRUCIBLE] + inventory.people[JOB_IRONFOUNDER] < construction->GetBuildingCount(BLD_IRONSMELTER) + 1) ? 1 : 0;;                   //crucible
    toolsettings[8] = (toolsettings[4] < 1 && toolsettings[3] < 1 && toolsettings[6] < 1 && toolsettings[2] < 1 && (inventory.goods[GD_SCYTHE] < 1)) ? 1 : 0;                        //scythe
    toolsettings[10] = (inventory.goods[GD_ROLLINGPIN] + inventory.people[JOB_BAKER] < construction->GetBuildingCount(BLD_BAKERY) + 1) ? 1 : 0;                            //rollingpin
    toolsettings[5] = (toolsettings[4] < 1 && toolsettings[3] < 1 && toolsettings[6] < 1 && toolsettings[2] < 1 && (inventory.goods[GD_SHOVEL] < 1)) ? 1 : 0 ;                       //shovel
    toolsettings[1] = (toolsettings[4] < 1 && toolsettings[3] < 1 && toolsettings[6] < 1 && toolsettings[2] < 1 && (inventory.goods[GD_AXE] + inventory.people[JOB_WOODCUTTER] < 12) && inventory.goods[GD_AXE] < 1) ? 1 : 0; //axe
    toolsettings[0] = 0; //(toolsettings[4]<1&&toolsettings[3]<1&&toolsettings[6]<1&&toolsettings[2]<1&&(aii.GetInventory().goods[GD_TONGS]<1))?1:0;                                                //Tongs(metalworks)
    toolsettings[9] = 0; //(aii.GetInventory().goods[GD_CLEAVER]+aii.GetInventory().people[JOB_BUTCHER]<construction->GetBuildingCount(BLD_SLAUGHTERHOUSE)+1)?1:0;                                //cleaver
    toolsettings[7] = 0;                                                                                                                                                                        //rod & line
    toolsettings[11] = 0;                                                                                                                                                                       //bow
    aii.ChangeTools(toolsettings);

    // Set military settings to some currently required values
    boost::array<unsigned char, MILITARY_SETTINGS_COUNT> milSettings;
    milSettings[0] = 10;
    milSettings[1] = HasFrontierBuildings()?5:0; //if we have a front send strong soldiers first else weak first to make upgrading easier
    milSettings[2] = 4;
    milSettings[3] = 5;
	//interior 0bar full if we have an upgrade building and gold(or produce gold) else 1 soldier each
	milSettings[4] = UpdateUpgradeBuilding() >= 0 && (inventory.goods[GD_COINS]>0 || (inventory.goods[GD_GOLD]>0 && inventory.goods[GD_COAL]>0 && !aii.GetBuildings(BLD_MINT).empty()) )? 8 : 0;     
    milSettings[6] = ggs.getSelection(AddonId::SEA_ATTACK)==2 ? 0 : 8; //harbor flag: no sea attacks?->no soldiers else 50% to 100%
	milSettings[5] = CalcMilSettings(); //inland 1bar min 50% max 100% depending on how many soldiers are available
	milSettings[7] = 8;                                                     //front: 100%
	if(player.militarySettings_[5] != milSettings[5] || player.militarySettings_[6] != milSettings[6] || player.militarySettings_[4]!=milSettings[4] || player.militarySettings_[1]!=milSettings[1]) //only send the command if we want to change something
		aii.ChangeMilitary(milSettings);
}

unsigned AIPlayerJH::CalcMilSettings()
{
	unsigned InlandTroops[5]= {0,0,0,0,0}; //how many troops are required to fill inland buildings at settings 4,5,6,7,8
	unsigned maxtroops[5][4]= //max troops in the military buildings at settings 4-8
	{
		{1,2,3,5},
		{1,2,4,6},
		{1,2,4,7},
		{1,2,5,8},
		{2,3,6,9}
	};
    ///first sum up all soldiers we have
    unsigned soldierCount=0;
	for (unsigned i=0; i<ggs.GetMaxMilitaryRank(); i++)
        soldierCount += aii.GetInventory().people[JOB_PRIVATE + i];
	
	//now add up all counts of soldiers that are fixed in use and those that depend on whatever we have as a result
    const unsigned numShouldStayConnected = PlannedConnectedInlandMilitary();
    int count=0;	
    unsigned soldierInUseFixed=0; 
    const int uun=UpdateUpgradeBuilding();
    const std::list<nobMilitary*>& militaryBuildings = aii.GetMilitaryBuildings();
	for (std::list<nobMilitary*>::const_iterator it=militaryBuildings.begin(); it!=militaryBuildings.end(); ++it)
	{		
		unsigned convtype=0;
		if((*it)->GetBuildingType()==BLD_BARRACKS)
			convtype=0;
		else if((*it)->GetBuildingType()==BLD_GUARDHOUSE)			
			convtype=1;
		else if((*it)->GetBuildingType()==BLD_WATCHTOWER)
			convtype=2;
		else if((*it)->GetBuildingType()==BLD_FORTRESS)
			convtype=3;
		if((*it)->GetFrontierDistance()==3 ||
            ((*it)->GetFrontierDistance()==2 && ggs.getSelection(AddonId::SEA_ATTACK)!=2) ||
            ((*it)->GetFrontierDistance()==0 && (militaryBuildings.size() < (unsigned)count + numShouldStayConnected || count==uun)))//front or connected interior
		{
			soldierInUseFixed+=maxtroops[4][convtype];
		}
		else if((*it)->GetFrontierDistance()==1) //1 bar (inland)
		{
			for(int i=0;i<5;i++)
				InlandTroops[i]+=maxtroops[i][convtype];	
		}
		else//setting should be 0 so add 1 soldier
			soldierInUseFixed++;

		count++;
	}
	
	//now the current need total and for inland and harbor is ready for use
	unsigned returnValue=8;
	while (returnValue > 4)
	{
        //have more than enough soldiers for this setting or just enough and this is the current setting? -> return it else try the next lower setting down to 4 (50%)
		if(soldierInUseFixed + InlandTroops[returnValue - 4] < soldierCount*10/11 || (player.militarySettings_[5]>=returnValue && soldierInUseFixed + InlandTroops[returnValue - 4] < soldierCount))
			break;
		returnValue--;
	}
	//LOG.lprintf("player %i inland milsetting %i \n",playerid,returnvalue);
	return returnValue;
}
