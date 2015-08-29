// $Id: AIPlayerJH.cpp 9586 2015-02-01 09:36:43Z marcus $
//
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


#include "defines.h"
#include "AIPlayerJH.h"

#include "GameClientPlayer.h"
#include "GameWorld.h"
#include "GamePlayerList.h"

#include "buildings/nobMilitary.h"
#include "buildings/nobHQ.h"
#include "buildings/noBuildingSite.h"
#include "nodeObjs/noShip.h"
#include "nodeObjs/noFlag.h"
#include "nodeObjs/noTree.h"
#include "nodeObjs/noAnimal.h"

#include "MapGeometry.h"
#include "AIConstruction.h"
#include "gameData/TerrainData.h"

#include <iostream>
#include <list>

#include "GameMessages.h"
#include "GameServer.h"
#include <set>
#include <algorithm>

// from Pathfinding.cpp
bool IsPointOK_RoadPath(const GameWorldBase& gwb, const MapPoint pt, const unsigned char dir, const void* param);

AIPlayerJH::AIPlayerJH(const unsigned char playerid, const GameWorldBase& gwb, const GameClientPlayer& player,
                       const GameClientPlayerList& players, const GlobalGameSettings& ggs,
                       const AI::Level level) : AIBase(playerid, gwb, player, players, ggs, level), defeated(false)
{
    construction = new AIConstruction(aii, this);
	initgfcomplete=0;
    currentJob = 0;
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
    if (!initgfcomplete)
    {
        InitStoreAndMilitarylists();		
		InitDistribution();
		construction->constructionorders.resize(BUILDING_TYPES_COUNT);
    }
	if(initgfcomplete<10)
	{
		initgfcomplete++;
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
        if(aii->GetMilitaryBuildings().size() < 1 && aii->GetStorehouses().size() < 2)
        {
            Chat(_("Hi, I'm an artifical player and I'm not very good yet!"));
            // AI doesn't usually crash the game any more :)
            // Chat(_("And I may crash your game sometimes..."));
        }
	}

    if (!gfisnwf) //try to complete a job on the list
    {
		//LOG.lprintf("ai doing stuff %i \n",playerid);
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
        if(ggs.getSelection(ADDON_SEA_ATTACK) < 2) //not deactivated by addon? -> go ahead
            TrySeaAttack();
    }
	// check expeditions (order new / cancel) and if we have 1 complete forester but less than 1 military building and less than 2 buildingsites stop production
	// stop/resume granitemine production
    if ((gf + playerid * 13) % 1500 == 0) 
    {
        for(std::list<nobHarborBuilding*>::const_iterator it = aii->GetHarbors().begin(); it != aii->GetHarbors().end(); it++)
        {
            if(((*it)->IsExpeditionActive() && !HarborPosRelevant((*it)->GetHarborPosID(), true)) || (!(*it)->IsExpeditionActive() && HarborPosRelevant((*it)->GetHarborPosID(), true))) //harbor is collecting for expedition and shouldnt OR not collecting and should -> toggle expedition
            {
                aii->StartExpedition((*it)->GetPos()); //command is more of a toggle despite it's name
            }
        }
        //find lost expedition ships - ai should get a notice and catch them all but just in case some fell through the system
        for(std::vector<noShip*>::const_iterator it = aii->GetShips().begin(); it != aii->GetShips().end(); it++)
        {
            if((*it)->IsWaitingForExpeditionInstructions())
                HandleExpedition(*it);
        }
		if(aii->GetBuildings(BLD_FORESTER).size()>0 && aii->GetBuildings(BLD_FORESTER).size()<2 && aii->GetMilitaryBuildings().size()<3 && aii->GetBuildingSites().size()<3)
			//stop the forester
		{
			if(!(*aii->GetBuildings(BLD_FORESTER).begin())->IsProductionDisabled())
				aii->ToggleProduction(aii->GetBuildings(BLD_FORESTER).front()->GetPos());
		}
		else //activate the forester 
		{
			if(aii->GetBuildings(BLD_FORESTER).size()>0 && (*aii->GetBuildings(BLD_FORESTER).begin())->IsProductionDisabled())
				aii->ToggleProduction(aii->GetBuildings(BLD_FORESTER).front()->GetPos());
		}
		//stop production in granite mines when the ai has many stones (100+ and at least 15 for each warehouse)
		if(AmountInStorage(GD_STONES,0)<100 || AmountInStorage(GD_STONES,0)<15*aii->GetStorehouses().size())
			//activate
		{
			for(std::list<nobUsual*>::const_iterator it=aii->GetBuildings(BLD_GRANITEMINE).begin();it!=aii->GetBuildings(BLD_GRANITEMINE).end();it++)
			{
				if((*it)->IsProductionDisabled())
					aii->ToggleProduction((*it)->GetPos());
			}
		}
		else //deactivate
		{
			for(std::list<nobUsual*>::const_iterator it=aii->GetBuildings(BLD_GRANITEMINE).begin();it!=aii->GetBuildings(BLD_GRANITEMINE).end();it++)
			{
				if(!(*it)->IsProductionDisabled())
					aii->ToggleProduction((*it)->GetPos());
			}
		}
    }
    if((gf + playerid * 11) % 150 == 0)
    {
        AdjustSettings();
        //check for useless sawmills
        if(aii->GetBuildings(BLD_SAWMILL).size() > 3)
        {
            int burns = 0;
            for(std::list<nobUsual*>::const_iterator it = aii->GetBuildings(BLD_SAWMILL).begin(); it != aii->GetBuildings(BLD_SAWMILL).end(); it++)
            {
                if(*(*it)->GetProduktivityPointer() < 1 && (*it)->HasWorker() && (*it)->GetWares(0) < 1 && (aii->GetBuildings(BLD_SAWMILL).size() - burns) > 3 && !(*it)->AreThereAnyOrderedWares())
                {
                    aii->DestroyBuilding((*it));
                    RemoveUnusedRoad(aii->GetSpecObj<noFlag>(aii->GetNeighbour((*it)->GetPos(), Direction::SOUTHWEST)), 1, true);
                    burns++;
                }
            }
        }
    }
    if((gf + playerid * 7) % build_interval == 0) // plan new buildings
    {
        construction->RefreshBuildingCount();
		
        //pick a random storehouse and try to build one of these buildings around it (checks if we actually want more of the building type)
        BuildingType bldToTest[] =
        {
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
        };
        unsigned numBldToTest = 24;
        int randomstore=0;
		//LOG.lprintf("new buildorders %i whs and %i mil for player %i \n",aii->GetStorehouses().size(),aii->GetMilitaryBuildings().size(),playerid);
		
        if(aii->GetStorehouses().size() > 0)
		{			
			randomstore = rand() % (aii->GetStorehouses().size());
			//collect swords,shields,helpers,privates and beer in first storehouse or whatever is closest to the upgradebuilding if we have one!
			nobBaseWarehouse* wh = GetUpgradeBuildingWarehouse();
			SetGatheringForUpgradeWarehouse(wh);
		
			if (MAX_MILITARY_RANK - ggs.getSelection(ADDON_MAX_RANK) > 0) //there is more than 1 rank available -> distribute
				DistributeMaxRankSoldiersByBlocking(5,wh);
			//unlimited when every warehouse has at least that amount
			DistributeGoodsByBlocking(23, 30); //30 boards for each warehouse - block after that - should speed up expansion
			DistributeGoodsByBlocking(24, 50); //50 stones for each warehouse - block after that - should limit losses in case a warehouse is destroyed
			//go to the picked random warehouse and try to build around it
			std::list<nobBaseWarehouse*>::const_iterator it = aii->GetStorehouses().begin();
			std::advance(it, randomstore);
			UpdateNodesAroundNoBorder((*it)->GetPos(), 15); //update the area we want to build in first
			for (unsigned int i = 0; i < numBldToTest; i++)
			{
				if (construction->Wanted(bldToTest[i]))
				{
					AddBuildJobAroundEvery(bldToTest[i], true); //add a buildorder for the picked buildingtype at every warehouse
				}
			}
			if(gf > 1500 || aii->GetInventory()->goods[GD_BOARDS] > 11)
	            AddBuildJob(construction->ChooseMilitaryBuilding((*it)->GetPos()), (*it)->GetPos());
		}
		//end of construction around & orders for warehouses

        //now pick a random military building and try to build around that as well
        if(aii->GetMilitaryBuildings().size() < 1)return;
        randomstore = rand() % (aii->GetMilitaryBuildings().size());
        std::list<nobMilitary*>::const_iterator it2 = aii->GetMilitaryBuildings().begin();
        std::advance(it2, randomstore);
        MapPoint t = (*it2)->GetPos();
        UpdateReachableNodes(t, 15);
		numBldToTest=14; //resource gathering buildings only around military; processing only close to warehouses
        for (unsigned int i = 0; i < numBldToTest; i++)
        {
            if (construction->Wanted(bldToTest[i]))
            {
                AddBuildJobAroundEvery(bldToTest[i], false);
            }
        }
        AddBuildJob(construction->ChooseMilitaryBuilding(t), t);
        if((*it2)->IsUseless() && (*it2)->IsDemolitionAllowed() && randomstore!=UpdateUpgradeBuilding())
        {
            aii->DestroyBuilding(t);
        }
    }	
}

bool AIPlayerJH::TestDefeat()
{		
    if (initgfcomplete>=10 && aii->GetStorehouses().empty())
    {
		//LOG.lprintf("ai defeated player %i \n",playerid);
        defeated = true;
        aii->Surrender();
        Chat(_("You win"));
        return true;
    }
    return false;
}

unsigned AIPlayerJH::GetJobNum() const { return eventManager.GetEventNum() + construction->GetBuildJobNum() + construction->GetConnectJobNum(); }	

/// returns the warehouse closest to the upgradebuilding or if it cant find a way the first warehouse and if there is no warehouse left null
nobBaseWarehouse* AIPlayerJH::GetUpgradeBuildingWarehouse()
{
	if(aii->GetStorehouses().size()<1)
		return 0;
	nobBaseWarehouse* wh=(*aii->GetStorehouses().begin());
	int uub=UpdateUpgradeBuilding();
	
	if (uub>=0 && aii->GetStorehouses().size()>1) //upgradebuilding exists and more than 1 warehouse -> find warehouse closest to the upgradebuilding - gather stuff there and deactivate gathering in the previous one
	{		
		std::list<nobMilitary*>::const_iterator ubldit=aii->GetMilitaryBuildings().begin();
		std::advance(ubldit,uub);
		//which warehouse is closest to the upgrade building? -> train troops there and block max ranks			
		unsigned param_count = 0; //at least 0 soldiers
		wh = aii->FindWarehouse((*ubldit), FW::Condition_Troops, 0, false, &param_count, false);
		if(!wh)
		{
			wh = (*aii->GetStorehouses().begin());
		}
	}
	return wh;
}

void AIPlayerJH::AddBuildJob(AIJH::BuildJob* job, bool front /*= false*/){ construction->AddBuildJob(job, front); }

void AIPlayerJH::AddBuildJob(BuildingType type, const MapPoint pt, bool front)
{
    construction->AddBuildJob(new AIJH::BuildJob(this, type, pt), front);
}

void AIPlayerJH::AddBuildJobAroundEvery(BuildingType bt, bool warehouse)
{
	if(warehouse)
	{
		for(std::list<nobBaseWarehouse*>::const_iterator it=aii->GetStorehouses().begin();it!=aii->GetStorehouses().end();it++)
		{
			construction->AddBuildJob(new AIJH::BuildJob(this,bt,(*it)->GetPos()),false);
		}
	}
	else
	{
		for(std::list<nobMilitary*>::const_iterator it=aii->GetMilitaryBuildings().begin();it!=aii->GetMilitaryBuildings().end();it++)
		{
			construction->AddBuildJob(new AIJH::BuildJob(this,bt,(*it)->GetPos()),false);
		}
	}
}

void AIPlayerJH::SetGatheringForUpgradeWarehouse(nobBaseWarehouse* upgradewarehouse)
{
	
	for (std::list<nobBaseWarehouse*>::const_iterator it=aii->GetStorehouses().begin();it!=aii->GetStorehouses().end();it++)
	{
		//deactivate gathering for all warehouses that are NOT the one next to the upgradebuilding
		if(upgradewarehouse->GetX()!=(*it)->GetX() || upgradewarehouse->GetY()!=(*it)->GetY())
		{
			if((*it)->CheckRealInventorySettings(0, 8, GD_BEER)) //collecting beer? -> stop it
				aii->ChangeInventorySetting((*it)->GetPos(), 0, 8, 0);

			if((*it)->CheckRealInventorySettings(0, 8, GD_SWORD)) //collecting swords? -> stop it
				aii->ChangeInventorySetting((*it)->GetPos(), 0, 8, 16);

			if((*it)->CheckRealInventorySettings(0, 8, 21)) //collecting shields? -> stop it
				aii->ChangeInventorySetting((*it)->GetPos(), 0, 8, 21);

			if((*it)->CheckRealInventorySettings(1, 8, JOB_PRIVATE)) //collecting privates? -> stop it
				aii->ChangeInventorySetting((*it)->GetPos(), 1, 8, JOB_PRIVATE);

			if((*it)->CheckRealInventorySettings(1, 8, JOB_HELPER)) //collecting helpers? -> stop it
				aii->ChangeInventorySetting((*it)->GetPos(), 1, 8, JOB_HELPER);
		}
		else//activate gathering in the closest warehouse
		{
			if(!(*it)->CheckRealInventorySettings(0, 8, GD_BEER)) //not collecting beer? -> start it
				aii->ChangeInventorySetting((*it)->GetPos(), 0, 8, 0);

			if(!(*it)->CheckRealInventorySettings(0, 8, GD_SWORD)) //not collecting swords? -> start it
				aii->ChangeInventorySetting((*it)->GetPos(), 0, 8, 16);

			if(!(*it)->CheckRealInventorySettings(0, 8, 21)) //not collecting shields? -> start it
				aii->ChangeInventorySetting((*it)->GetPos(), 0, 8, 21);

			if(!(*it)->CheckRealInventorySettings(1, 8, JOB_PRIVATE) && MAX_MILITARY_RANK - ggs.getSelection(ADDON_MAX_RANK) > 0) //not collecting privates AND we can actually upgrade soldiers? -> start it
				aii->ChangeInventorySetting((*it)->GetPos(), 1, 8, JOB_PRIVATE);

			if(((*it)->CheckRealInventorySettings(1, 8, 0) && ((*it)->GetInventory()->people[JOB_HELPER] > 50)) || (!(*it)->CheckRealInventorySettings(1, 8, 0) && !((*it)->GetInventory()->people[JOB_HELPER] > 50)))
			{
				aii->ChangeInventorySetting((*it)->GetPos(), 1, 8, 0); //less than 50 helpers - collect them: more than 50 stop collecting
			}
		}
	}
}

AIJH::Resource AIPlayerJH::CalcResource(const MapPoint pt)
{
    AIJH::Resource subRes = aii->GetSubsurfaceResource(pt);

    // resources on surface
    if (subRes == AIJH::NOTHING)
    {
        AIJH::Resource res = aii->GetSurfaceResource(pt);

        if(res == AIJH::NOTHING)
        {
            if(aii->IsRoadPoint(pt))
                return AIJH::NOTHING;

            for(unsigned char i = 0; i < Direction::COUNT; ++i)
            {
                TerrainType t = aii->GetTerrainAround(pt, Direction::fromUInt(i));

                // check against valid terrains for planting
                if(!TerrainData::IsVital(t))
                    return AIJH::NOTHING;
            }
            return AIJH::PLANTSPACE;
        }
        else if (res == AIJH::WOOD)
        {
            if((gwb.GetSpecObj<noTree>(pt))->type == 5) //exclude pineapple (because they are more of a "blocker" than a tree and only count as tree for animation&sound)
                return AIJH::NOTHING;
        }
    }
    else
    {
        AIJH::Resource res = aii->GetSurfaceResource(pt);
        if (res == AIJH::STONES || res == AIJH::WOOD)
        {
            if (subRes == AIJH::WOOD)
            {
                if ((gwb.GetSpecObj<noTree>(pt))->type != 5)
                    return AIJH::MULTIPLE;
            }
            else
                return AIJH::MULTIPLE;
        }
    }
    if (subRes == AIJH::BLOCKED)
        subRes = AIJH::NOTHING; // nicht so ganz logisch... aber Blocked als res is doof TODO

    return subRes;
}

void AIPlayerJH::InitReachableNodes()
{
    unsigned short width = aii->GetMapWidth();
    unsigned short height = aii->GetMapHeight();

    std::queue<MapPoint> toCheck;

    // Alle auf not reachable setzen
    for (MapPoint pt(0, 0); pt.y < height; ++pt.y)
    {
        for (pt.x = 0; pt.x < width; ++pt.x)
        {
            unsigned i = gwb.GetIdx(pt);
            nodes[i].reachable = false;
            const noFlag* myFlag = 0;
            if ( (myFlag = aii->GetSpecObj<noFlag>(pt)) )
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

    while(toCheck.size() > 0)
    {
        // Reachable coordinate
        MapPoint r = toCheck.front();

        // Coordinates to test around this reachable coordinate
        for (unsigned dir = 0; dir < Direction::COUNT; ++dir)
        {
            MapPoint n = aii->GetNeighbour(r, Direction::fromUInt(dir));
            unsigned ni = aii->GetIdx(n);

            // already reached, don't test again
            if (nodes[ni].reachable)
                continue;

            bool boat = false;
            // Test whether point is reachable; yes->add to check list
            if (IsPointOK_RoadPath(gwb, n, (dir + 3) % 6, (void*) &boat))
            {
                nodes[ni].reachable = true;
                toCheck.push(n);
            }
        }
        toCheck.pop();
    }
}


void AIPlayerJH::UpdateReachableNodes(const MapPoint pt, unsigned radius)
{
    std::queue<MapPoint> toCheck;

    for(MapCoord tx = aii->GetXA(pt, Direction::NORTH), r = 1; r <= radius; tx = aii->GetXA(tx, pt.y, Direction::NORTH), ++r)
    {
        MapPoint t2(tx, pt.y);
        for(unsigned i = 2; i < 8; ++i)
        {
            for(MapCoord r2 = 0; r2 < r; t2 = aii->GetNeighbour(t2, Direction(i)), ++r2)
            {
                unsigned i = aii->GetIdx(t2);
                nodes[i].reachable = false;
                const noFlag* myFlag = 0;
                if (( myFlag = aii->GetSpecObj<noFlag>(t2)))
                {
                    if (myFlag->GetPlayer() == playerid)
                    {
                        nodes[i].reachable = true;
                        toCheck.push(t2);
                    }
                }
            }
        }
    }
    IterativeReachableNodeChecker(toCheck);
}

void AIPlayerJH::InitNodes()
{
    unsigned short width = aii->GetMapWidth();
    unsigned short height = aii->GetMapHeight();

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
                nodes[i].bq = aii->GetBuildingQuality(pt);
            }
            else
            {
                nodes[i].owned = false;
                nodes[i].bq = BQ_NOTHING;
            }

            nodes[i].res = CalcResource(pt);
            nodes[i].border = aii->IsBorder(pt);
            nodes[i].farmed = false;
        }
    }
}

void AIPlayerJH::UpdateNodes()
{

}

void AIPlayerJH::InitResourceMaps()
{
    unsigned short width = aii->GetMapWidth();
    unsigned short height = aii->GetMapHeight();

    resourceMaps.resize(AIJH::RES_TYPE_COUNT);
    for (unsigned res = 0; res < AIJH::RES_TYPE_COUNT; ++res)
    {
        resourceMaps[res].resize(width * height);
        for (MapPoint pt(0, 0); pt.y < height; ++pt.y)
        {
            for (pt.x = 0; pt.x < width; ++pt.x)
            {
                unsigned i = aii->GetIdx(pt);
                //resourceMaps[res][i] = 0;
                if (nodes[i].res == (AIJH::Resource)res && (AIJH::Resource)res != AIJH::BORDERLAND)
                {
                    ChangeResourceMap(pt, AIJH::RES_RADIUS[res], resourceMaps[res], 1);
                }

                // Grenzgebiet"ressource"
                else if (nodes[i].border && (AIJH::Resource)res == AIJH::BORDERLAND)
                {
                    ChangeResourceMap(pt, AIJH::RES_RADIUS[AIJH::BORDERLAND], resourceMaps[AIJH::BORDERLAND], 1);
                }
                if(nodes[i].res == AIJH::MULTIPLE)
                {
                    if(aii->GetSubsurfaceResource(pt) == (AIJH::Resource)res || aii->GetSurfaceResource(pt) == (AIJH::Resource)res)
                        ChangeResourceMap(pt, AIJH::RES_RADIUS[res], resourceMaps[res], 1);
                }
            }
        }
    }
}

void AIPlayerJH::RecalcResource(AIJH::Resource res)
{
    unsigned short width = aii->GetMapWidth();
    unsigned short height = aii->GetMapHeight();
    std::vector<int> &resmap = resourceMaps[res];
    for (unsigned y = 0; y < resmap.size(); ++y)
    {
        resmap[y] = 0;
    }
    for (MapPoint pt(0, 0); pt.y < height; ++pt.y)
    {
        for (pt.x = 0; pt.x < width; ++pt.x)
        {
            unsigned i = aii->GetIdx(pt);
            //resourceMaps[res][i] = 0;
            if (nodes[i].res == res && res != AIJH::BORDERLAND && TerrainData::IsUseable(gwb.GetNode(pt).t1))
            {
                ChangeResourceMap(pt, AIJH::RES_RADIUS[res], resmap, 1);
            }
            // Grenzgebiet"ressource"
            else if (aii->IsBorder(pt) && res == AIJH::BORDERLAND)
            {
                //only count border area that is actually passable terrain
                if(TerrainData::IsUseable(gwb.GetNode(pt).t1))
                    ChangeResourceMap(pt, AIJH::RES_RADIUS[AIJH::BORDERLAND], resmap, 1);
            }
            if (nodes[i].res == AIJH::MULTIPLE && TerrainData::IsUseable(gwb.GetNode(pt).t1) )
            {
                if(aii->GetSubsurfaceResource(pt) == res || aii->GetSurfaceResource(pt) == res)
                    ChangeResourceMap(pt, AIJH::RES_RADIUS[res], resmap, 1);
            }
            if(res == AIJH::WOOD && aii->IsBuildingOnNode(pt, BLD_WOODCUTTER)) //existing woodcutters reduce wood rating
                ChangeResourceMap(pt, 7, resmap, -10);
            if(res == AIJH::PLANTSPACE && aii->IsBuildingOnNode(pt, BLD_FARM)) //existing farm reduce plantspace rating
                ChangeResourceMap(pt, 3, resmap, -25);
            if(res == AIJH::PLANTSPACE && aii->IsBuildingOnNode(pt, BLD_FORESTER)) //existing forester reduce plantspace rating
                ChangeResourceMap(pt, 6, resmap, -25);
        }
    }
}

void AIPlayerJH::SetFarmedNodes(const MapPoint pt, bool set)
{
    // Radius in dem Bausplatz für Felder blockiert wird
    const unsigned radius = 3;

    nodes[aii->GetIdx(pt)].farmed = set;

    for(MapCoord tx = aii->GetXA(pt, Direction::NORTH), r = 1; r <= radius; tx = aii->GetXA(tx, pt.y, Direction::NORTH), ++r)
    {
        MapPoint t2(tx, pt.y);
        for(unsigned i = 2; i < 8; ++i)
        {
            for(MapCoord r2 = 0; r2 < r; t2 = aii->GetNeighbour(t2, Direction(i)), ++r2)
            {
                unsigned i = aii->GetIdx(t2);
                nodes[i].farmed = set;
            }
        }
    }
}

void AIPlayerJH::ChangeResourceMap(const MapPoint pt, unsigned radius, std::vector<int> &resMap, int value)
{
    resMap[aii->GetIdx(pt)] += value * radius;

    for(MapCoord tx = aii->GetXA(pt, Direction::NORTH), r = 1; r <= radius; tx = aii->GetXA(tx, pt.y, Direction::NORTH), ++r)
    {
        MapPoint t2(tx, pt.y);
        for(unsigned i = 2; i < 8; ++i)
        {
            for(MapCoord r2 = 0; r2 < r; t2 = aii->GetNeighbour(t2, Direction(i)), ++r2)
            {
                unsigned i = aii->GetIdx(t2);
                resMap[i] += value * (radius - r);
            }
        }
    }


}

bool AIPlayerJH::FindGoodPosition(MapPoint& pt, AIJH::Resource res, int threshold, BuildingQuality size, int radius, bool inTerritory)
{
    unsigned short width = aii->GetMapWidth();
    unsigned short height = aii->GetMapHeight();

    if (pt.x >= width || pt.y >= height)
    {
        pt.x = aii->GetHeadquarter()->GetX();
        pt.y = aii->GetHeadquarter()->GetY();
    }

    // TODO was besseres wär schön ;)
    if (radius == -1)
        radius = 30;

    for(MapCoord tx = aii->GetXA(pt, Direction::NORTH), r = 1; r <= radius; tx = aii->GetXA(tx, pt.y, Direction::NORTH), ++r)
    {
        MapPoint t2(tx, pt.y);
        for(unsigned i = 2; i < 8; ++i)
        {
            for(MapCoord r2 = 0; r2 < r; t2 = aii->GetNeighbour(t2, Direction(i)), ++r2)
            {
                unsigned i = aii->GetIdx(t2);
                if (resourceMaps[res][i] >= threshold)
                {
                    if ((inTerritory && !aii->IsOwnTerritory(t2)) || nodes[i].farmed)
                        continue;
                    if ( (aii->GetBuildingQuality(t2) >= size && aii->GetBuildingQuality(t2) < BQ_MINE) // normales Gebäude
                            || (aii->GetBuildingQuality(t2) == size)) // auch Bergwerke
                    {
                        pt = t2;
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

PositionSearch* AIPlayerJH::CreatePositionSearch(MapPoint& pt, AIJH::Resource res, BuildingQuality size, int minimum, BuildingType bld, bool best)
{
    // set some basic parameters
    PositionSearch* p = new PositionSearch(pt, res, minimum, size, BLD_WOODCUTTER, best);
    p->nodesPerStep = 25; // TODO make it dependent on something...
    p->resultValue = 0;

    // allocate memory for the nodes
    unsigned numNodes = aii->GetMapWidth() * aii->GetMapHeight();
    p->tested = new std::vector<bool>(numNodes, false);
    p->toTest = new std::queue<unsigned>;


    // if no useful startpos is given, use headquarter
    if (pt.x >= aii->GetMapWidth() || pt.y >= aii->GetMapHeight())
    {
        pt.x = aii->GetHeadquarter()->GetX();
        pt.y = aii->GetHeadquarter()->GetY();
    }

    // insert start position as first node to test
    p->toTest->push(aii->GetIdx(pt));
    (*p->tested)[aii->GetIdx(pt)] = true;

    return p;
}

PositionSearchState AIPlayerJH::FindGoodPosition(PositionSearch* search, bool best)
{
    // make nodesPerStep tests
    for (int i = 0; i < search->nodesPerStep; i++)
    {
        // no more nodes to test? end this!
        if (search->toTest->empty())
            break;

        // get the node
        unsigned nodeIndex = search->toTest->front();
        search->toTest->pop();
        AIJH::Node* node = &nodes[nodeIndex];
        unsigned short width = aii->GetMapWidth();
        MapPoint pt(nodeIndex % width, nodeIndex / width);

        // and test it... TODO exception at res::borderland?
        if (resourceMaps[search->res][nodeIndex] > search->resultValue // value better
                && node->owned && node->reachable && !node->farmed // available node
                && ((node->bq >= search->size && node->bq < BQ_MINE) || (node->bq == search->size)) // matching size
           )
        {
            // store location & value
            search->resultValue = resourceMaps[search->res][nodeIndex];
            search->result = pt;
        }

        // now insert neighbouring nodes...
        for (unsigned char dir = 0; dir < Direction::COUNT; ++dir)
        {
            MapPoint n = aii->GetNeighbour(pt, Direction::fromUInt(dir));
            unsigned ni = aii->GetIdx(n);

            // test if already tested or not in territory
            if (!(*search->tested)[ni] && nodes[ni].owned)
            {
                search->toTest->push(ni);
                (*search->tested)[ni] = true;
            }
        }
    }

    // decide the state of the search

    // no more nodes to test, not reached minimum
    if (search->toTest->empty() && search->resultValue < search->minimum)
    {
        return SEARCH_FAILED;
    }

    // reached minimal satifiying value or best value, if needed
    else if ( (search->resultValue >= search->minimum && !best)
              || (search->resultValue >= search->minimum && search->toTest->empty()))
    {
        return SEARCH_SUCCESSFUL;
    }

    // more to search...
    else
    {
        return SEARCH_IN_PROGRESS;
    }
}


bool AIPlayerJH::FindBestPositionDiminishingResource(MapPoint& pt, AIJH::Resource res, BuildingQuality size, int minimum, int radius, bool inTerritory)
{
    bool fixed = ggs.isEnabled(ADDON_INEXHAUSTIBLE_MINES) && (res == AIJH::IRONORE || res == AIJH::COAL || res == AIJH::GOLD || res == AIJH::GRANITE);
    unsigned short width = aii->GetMapWidth();
    unsigned short height = aii->GetMapHeight();
    int temp = 0;
    bool lastcirclevaluecalculated = false;
    bool lastvaluecalculated = false;
    //to avoid having to calculate a value twice and still move left on the same level without any problems we use this variable to remember the first calculation we did in the circle.
    int circlestartvalue = 0;
    //outside of map bounds? -> search around our main storehouse!
    if (pt.x >= width || pt.y >= height)
    {
        pt.x = aii->GetStorehouses().front()->GetX();
        pt.y = aii->GetStorehouses().front()->GetY();
    }

    // TODO was besseres wär schön ;)
    if (radius == -1)
        radius = 11;

    MapPoint best(0, 0);
    int best_value = -1;

    for(MapCoord tx = aii->GetXA(pt, Direction::NORTH), r = 1; r <= radius; tx = aii->GetXA(tx, pt.y, Direction::NORTH), ++r)
    {
        MapPoint t2(tx, pt.y);
        for(unsigned i = 2; i < 8; ++i)
        {
            for(MapCoord r2 = 0; r2 < r; ++r2)
            {
                unsigned n = aii->GetIdx(t2);
                if(fixed)
                    temp = resourceMaps[res][n];
                else
                {
                    //only do a complete calculation for the first point or when moving outward and the last value is unknown
                    if((r < 2 || !lastcirclevaluecalculated) && r2 < 1 && i < 3 && resourceMaps[res][n])
                    {
                        temp = aii->CalcResourceValue(t2, res);
                        circlestartvalue = temp;
                        lastcirclevaluecalculated = true;
                        lastvaluecalculated = true;
                    }
                    else
                    {
                        //was there ever anything? if not skip it!
                        if(!resourceMaps[res][n])
                        {
                            if(r2 < 1 && i < 3)
                                lastcirclevaluecalculated = false;
                            lastvaluecalculated = false;
                            temp = resourceMaps[res][n];
                        }
                        else
                        {
                            //temp=aii->CalcResourceValue(t2,res);
                            //circle not yet started? -> last direction was outward (left=0)
                            if(r2 < 1 && i < 3)
                            {
                                temp = aii->CalcResourceValue(t2, res, 0, circlestartvalue);
                                circlestartvalue = temp;
                            }
                            else
                            {
                                if(lastvaluecalculated)
                                {
                                    if(r2 > 0) //we moved direction i%6
                                        temp = aii->CalcResourceValue(t2, res, i % 6, temp);
                                    else //last step was the previous direction
                                        temp = aii->CalcResourceValue(t2, res, (i - 1) % 6, temp);
                                }
                                else
                                {
                                    temp = aii->CalcResourceValue(t2, res);
                                    lastvaluecalculated = true;
                                }
                            }
                        }
                    }
                    //if(resourceMaps[res][n])
                    //assert(temp==aii->CalcResourceValue(t2,res));
                    //copy the value to the resource map
                    resourceMaps[res][n] = temp;
                }
                if(res == AIJH::FISH || res == AIJH::STONES)
                {
                    //remove permanently invalid spots to speed up future checks
                    TerrainType t1 = gwb.GetNode(t2).t1;
                    if(!TerrainData::IsUseable(t1) || TerrainData::IsMineable(t1))
                        resourceMaps[res][n] = 0;
                }
                else //= granite,gold,iron,coal
                {
                    if(!TerrainData::IsMineable(gwb.GetNode(t2).t1))
                        resourceMaps[res][n] = 0;
                }
                if (temp > best_value)
                {
                    if (!nodes[n].reachable || (inTerritory && !aii->IsOwnTerritory(t2)) || nodes[n].farmed)
                    {
                        t2 = aii->GetNeighbour(t2, Direction(i));
                        continue;
                    }
                    //special case fish -> check for other fishery buildings
                    if(res == AIJH::FISH)
                    {
                        if(BuildingNearby(t2, BLD_FISHERY, 6))
                        {
                            t2 = aii->GetNeighbour(t2, Direction(i));
                            continue;
                        }
                    }
                    //dont build next to harborspots
                    if(HarborPosClose(t2, 3, true))
                    {
                        t2 = aii->GetNeighbour(t2, Direction(i));
                        continue;
                    }
                    BuildingQuality bq = aii->GetBuildingQuality(t2);
                    if ( (bq >= size && bq < BQ_MINE) // normales Gebäude
                            || (bq == size))    // auch Bergwerke
                    {
                        best = t2;
                        best_value = temp;
                        //TODO: calculate "perfect" rating and instantly return if we got that already
                    }
                }
                t2 = aii->GetNeighbour(t2, Direction(i));
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

bool AIPlayerJH::FindBestPosition(MapPoint& pt, AIJH::Resource res, BuildingQuality size, int minimum, int radius, bool inTerritory)
{
    if(res == AIJH::IRONORE || res == AIJH::COAL || res == AIJH::GOLD || res == AIJH::GRANITE || res == AIJH::STONES || res == AIJH::FISH)
        return FindBestPositionDiminishingResource(pt, res, size, minimum, radius, inTerritory);
    unsigned short width = aii->GetMapWidth();
    unsigned short height = aii->GetMapHeight();
    int temp = 0;
    //to avoid having to calculate a value twice and still move left on the same level without any problems we use this variable to remember the first calculation we did in the circle.
    int circlestartvalue = 0;
    //outside of map bounds? -> search around our main storehouse!
    if (pt.x >= width || pt.y >= height)
    {
        pt = aii->GetStorehouses().front()->GetPos();
    }

    // TODO was besseres wär schön ;)
    if (radius == -1)
        radius = 11;

    MapPoint best(0, 0);
    int best_value = -1;

    for(MapCoord tx = aii->GetXA(pt, Direction::NORTH), r = 1; r <= radius; tx = aii->GetXA(tx, pt.y, Direction::NORTH), ++r)
    {
        MapPoint t2(tx, pt.y);
        for(unsigned i = 2; i < 8; ++i)
        {
            for(MapCoord r2 = 0; r2 < r; ++r2)
            {
                unsigned n = aii->GetIdx(t2);
                //only do a complete calculation for the first point!
                if(r < 2 && r2 < 1 && i < 3)
                {
                    temp = aii->CalcResourceValue(t2, res);
                    circlestartvalue = temp;
                }
                else
                {
                    //temp=aii->CalcResourceValue(t2,res);
                    //circle not yet started? -> last direction was outward (left=0)
                    if(r2 < 1 && i < 3)
                    {
                        temp = aii->CalcResourceValue(t2, res, 0, circlestartvalue);
                        circlestartvalue = temp;
                    }
                    else
                    {
                        if(r2 > 0) //we moved direction i%6
                            temp = aii->CalcResourceValue(t2, res, i % 6, temp);
                        else //last step was the previous direction
                            temp = aii->CalcResourceValue(t2, res, (i - 1) % 6, temp);
                    }
                }
                //copy the value to the resource map (map is only used in the ai debug mode)
                resourceMaps[res][n] = temp;
                if (temp > best_value)
                {
                    if (!nodes[n].reachable || (inTerritory && !aii->IsOwnTerritory(t2)) || nodes[n].farmed)
                    {
                        t2 = aii->GetNeighbour(t2, Direction(i));
                        continue;
                    }
                    if(HarborPosClose(t2, 3, true))
                    {
                        t2 = aii->GetNeighbour(t2, Direction(i));
                        continue;
                    }
                    BuildingQuality bq = aii->GetBuildingQuality(t2);
                    if (( (bq >= size && bq < BQ_MINE) // normales Gebäude
                            || (bq == size)) &&     // auch Bergwerke
							(res!=AIJH::BORDERLAND || !aii->IsRoadPoint(aii->GetNeighbour(t2, Direction::SOUTHWEST))))
					//special: military buildings cannot be build next to an existing road as that would have them connected to 2 roads which the ai no longer should do
                    {
                        best = t2;
                        best_value = temp;
                        //TODO: calculate "perfect" rating and instantly return if we got that already
                    }
                }
                t2 = aii->GetNeighbour(t2, Direction(i));
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
		if(currentJob)
			delete currentJob;
		currentJob = new AIJH::EventJob(this,eventManager.GetEvent());
		currentJob->ExecuteJob();
	}
	//how many construction & connect jobs the ai will attempt every gf, the ai gets new orders from events and every 200 gf
	quota = (aii->GetStorehouses().size() + aii->GetMilitaryBuildings().size()) * 1;
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

void AIPlayerJH::RecalcBQAround(const MapPoint pt)
{
}

void AIPlayerJH::CheckNewMilitaryBuildings()
{
}

void AIPlayerJH::DistributeGoodsByBlocking(unsigned char goodnumber, unsigned limit)
{
    bool validgoalexists = false;
    if (aii->GetHarbors().size() < (aii->GetStorehouses().size() / 2)) //dont distribute on maps that are mostly sea maps - harbors are too difficult to defend and have to handle quite a lot of traffic already
    {
        for(std::list<nobBaseWarehouse*>::const_iterator it = aii->GetStorehouses().begin(); it != aii->GetStorehouses().end(); it++)
        {
            if ((*it)->GetInventory()->goods[goodnumber] <= limit)
            {
                validgoalexists = true;
                break;
            }
        }
    }
    if (!validgoalexists) // more than limit everywhere (or sea map) -> unblock everywhere
    {
        for(std::list<nobBaseWarehouse*>::const_iterator it = aii->GetStorehouses().begin(); it != aii->GetStorehouses().end(); it++)
        {
            if((*it)->CheckRealInventorySettings(0, 2, goodnumber)) //page,setting,goodnumber - not unblocked then issue command to unblock
                aii->ChangeInventorySetting((*it)->GetPos(), 0, 2, goodnumber); //page,setting,goodnumber (settings: 0 nothing,2 block,8collect)
        }
    }
    else // valid goal exists -> block where at least limit goods are stored and unblock the others
    {
        for(std::list<nobBaseWarehouse*>::const_iterator it = aii->GetStorehouses().begin(); it != aii->GetStorehouses().end(); it++)
        {
            if((*it)->GetInventory()->goods[goodnumber] <= limit) //not at limit - unblock it
            {
                if((*it)->CheckRealInventorySettings(0, 2, goodnumber)) //page,setting,goodnumber - not unblocked then issue command to unblock
                {
                    aii->ChangeInventorySetting((*it)->GetPos(), 0, 2, goodnumber); //page,setting,goodnumber (settings: 2 block,8collect)
                }
            }
            else // at limit - block it
            {
                if(!(*it)->CheckRealInventorySettings(0, 2, goodnumber)) //already blocked?
                    aii->ChangeInventorySetting((*it)->GetPos(), 0, 2, goodnumber);
            }
        }
    }
}
void AIPlayerJH::DistributeMaxRankSoldiersByBlocking(unsigned limit,nobBaseWarehouse* upwh)
{
	unsigned completewh=aii->GetStorehouses().size();
	
	if(completewh<1) //no warehouses -> no job
		return;

	unsigned char maxrankjobnr=JOB_PRIVATE + MAX_MILITARY_RANK  - ggs.getSelection(ADDON_MAX_RANK); //private + general - max rank limiter
	
	if(completewh==1 ) //only 1 warehouse? dont block max ranks here
	{
		if ((*(aii->GetStorehouses().begin()))->CheckRealInventorySettings(1,2,maxrankjobnr))
			aii->ChangeInventorySetting(aii->GetStorehouses().front()->GetPos(),1,2,maxrankjobnr);
		return;
	}
	//rest applies for at least 2 complete warehouses!
	std::list<nobMilitary*>frontiermils; //make a list containing frontier military buildings
	for (std::list<nobMilitary*>::const_iterator it = aii->GetMilitaryBuildings().begin();it!=aii->GetMilitaryBuildings().end();it++)
	{
		if ((*it)->GetFrontierDistance()>0 && !(*it)->IsNewBuilt())
			frontiermils.push_back(*it);
	}
	std::list<nobBaseWarehouse*>frontierwhs; //make a list containing all warehouses near frontier military buildings
	for (std::list<nobBaseWarehouse*>::const_iterator it = aii->GetStorehouses().begin(); it!=aii->GetStorehouses().end();it++)
	{
		for (std::list<nobMilitary*>::const_iterator it2 = frontiermils.begin(); it2!=frontiermils.end();it2++)
		{	
			if(aii->CalcDistance((*it)->GetPos(),(*it2)->GetPos())<12)
			{
				frontierwhs.push_back(*it);
				break;
			}
		}		
	}
	//have frontier warehouses?
	if(!frontierwhs.empty())
	{
		//LOG.lprintf("distribute maxranks - got frontierwhs for player %i \n",playerid);
		bool understaffedwh=false;
		//try to gather limit maxranks in each - if we have that many unblock for all frontier whs, 
		//check if there is at least one with less than limit first
		for (std::list<nobBaseWarehouse*>::const_iterator it=frontierwhs.begin();it!=frontierwhs.end();it++)
		{
			if((*it)->GetInventory()->people[maxrankjobnr]<limit)
			{
				understaffedwh=true;
				break;
			}
		}
		//if understaffed was found block in all with >=limit else unblock in all
		for (std::list<nobBaseWarehouse*>::const_iterator it=aii->GetStorehouses().begin();it!=aii->GetStorehouses().end();it++)
		{
			if(helpers::contains(frontierwhs, (*it))) //frontier wh?
			{
				if(understaffedwh)
				{
					if((*it)->GetInventory()->people[maxrankjobnr]<limit)
					{
						if ((*it)->CheckRealInventorySettings(1,2,maxrankjobnr)) //maxranks blocked? -> unblock
							aii->ChangeInventorySetting((*it)->GetPos(),1,2,maxrankjobnr);
					}
					else //more than limit
					{
						if (!(*it)->CheckRealInventorySettings(1,2,maxrankjobnr)) //maxranks not blocked -> block
							aii->ChangeInventorySetting((*it)->GetPos(),1,2,maxrankjobnr);
					}					
				}
				else //no understaffedwh
				{
					if ((*it)->CheckRealInventorySettings(1,2,maxrankjobnr)) //maxranks blocked? -> unblock
							aii->ChangeInventorySetting((*it)->GetPos(),1,2,maxrankjobnr);
				}
			}
			else //not frontier wh! block it
			{
				if (!(*it)->CheckRealInventorySettings(1,2,maxrankjobnr)) //maxranks not blocked -> block
							aii->ChangeInventorySetting((*it)->GetPos(),1,2,maxrankjobnr);
			}
		}
	}
	else //there are no frontier whs!
	{
		//LOG.lprintf("distribute maxranks - got NO frontierwhs for player %i \n",playerid);
		bool understaffedwh=false;
		//try to gather limit maxranks in each - if we have that many unblock for all  whs, 
		//check if there is at least one with less than limit first
		for (std::list<nobBaseWarehouse*>::const_iterator it=aii->GetStorehouses().begin();it!=aii->GetStorehouses().end();it++)
		{
			if((*it)->GetInventory()->people[maxrankjobnr]<limit && (*it)->GetPos() != upwh->GetPos()) // warehouse next to upgradebuilding is special case
			{
				understaffedwh=true;
				break;
			}
		}
		for (std::list<nobBaseWarehouse*>::const_iterator it=aii->GetStorehouses().begin();it!=aii->GetStorehouses().end();it++)
		{
			if((*it)->GetPos() == upwh->GetPos()) // warehouse next to upgradebuilding should block when there is more than 1 wh
			{
				//LOG.lprintf("distribute maxranks - got NO frontierwhs for player %i , block at hq \n",playerid);
				if (!(*it)->CheckRealInventorySettings(1,2,maxrankjobnr))
					aii->ChangeInventorySetting((*it)->GetPos(),1,2,maxrankjobnr);
				continue;
			}
			if(understaffedwh)
			{
				
				if((*it)->GetInventory()->people[maxrankjobnr]<limit )
				{
					if ((*it)->CheckRealInventorySettings(1,2,maxrankjobnr)) //maxranks blocked? -> unblock
						aii->ChangeInventorySetting((*it)->GetPos(),1,2,maxrankjobnr);
				}
				else //more than limit
				{
					if (!(*it)->CheckRealInventorySettings(1,2,maxrankjobnr)) //maxranks not blocked -> block
						aii->ChangeInventorySetting((*it)->GetPos(),1,2,maxrankjobnr);
				}					
			}
			else //no understaffedwh
			{
				if ((*it)->CheckRealInventorySettings(1,2,maxrankjobnr)) //maxranks blocked? -> unblock
						aii->ChangeInventorySetting((*it)->GetPos(),1,2,maxrankjobnr);
			}
		}
	}
}
bool AIPlayerJH::SimpleFindPosition(MapPoint& pt, BuildingQuality size, int radius)
{
    unsigned short width = aii->GetMapWidth();
    unsigned short height = aii->GetMapHeight();
    //if(size==BQ_HARBOR)
    //  Chat(_("looking for harbor"));

    if (pt.x >= width || pt.y >= height)
    {
        pt = aii->GetStorehouses().front()->GetPos();
    }

    // TODO was besseres wär schön ;)
    if (radius == -1)
        radius = 30;

    for(MapCoord tx = aii->GetXA(pt, Direction::NORTH), r = 1; r <= radius; tx = aii->GetXA(tx, pt.y, Direction::NORTH), ++r)
    {
        MapPoint t2(tx, pt.y);
        for(unsigned i = 2; i < 8; ++i)
        {
            for(MapCoord r2 = 0; r2 < r; t2 = aii->GetNeighbour(t2, Direction(i)), ++r2)
            {
                unsigned i = aii->GetIdx(t2);

                if (!nodes[i].reachable || !aii->IsOwnTerritory(t2) || nodes[i].farmed)
                    continue;
                if(HarborPosClose(t2, 3, true))
                {
                    if(size != BQ_HARBOR)
                        continue;
                }
                BuildingQuality bq = aii->GetBuildingQuality(t2);
                if ( (bq >= size && bq < BQ_MINE) // normales Gebäude
                        || (bq == size))    // auch Bergwerke
                {
                    pt = t2;
                    return true;
                }
            }
        }
    }

    return false;
}

unsigned AIPlayerJH::GetDensity(MapPoint pt, AIJH::Resource res, int radius)
{
    unsigned short width = aii->GetMapWidth();
    unsigned short height = aii->GetMapHeight();


    // TODO: check warum das so ist, und ob das sinn macht! ist so weil der punkt dann außerhalb der karte liegen würde ... könnte trotzdem crashen wenn wir kein hq mehr haben ... mehr checks!
    if (pt.x >= width || pt.y >= height)
    {
        pt = aii->GetStorehouses().front()->GetPos();
    }



    unsigned good = 0;
    unsigned all = 0;

    for(MapCoord tx = aii->GetXA(pt, Direction::NORTH), r = 1; r <= radius; tx = aii->GetXA(tx, pt.y, Direction::NORTH), ++r)
    {
        MapPoint t2(tx, pt.y);
        for(unsigned i = 2; i < 8; ++i)
        {
            for(MapCoord r2 = 0; r2 < r; t2 = aii->GetNeighbour(t2, Direction(i)), ++r2)
            {
                unsigned i = aii->GetIdx(t2);

                if (nodes[i].res == res)
                    good++;

                all++;
            }
        }
    }

    return (all != 0) ? (good * 100) / all : 0;
}

void AIPlayerJH::HandleNewMilitaryBuilingOccupied(const MapPoint pt)
{
    //kill bad flags we find
    RemoveAllUnusedRoads(pt);
    construction->RefreshBuildingCount();
    const nobMilitary* mil = aii->GetSpecObj<nobMilitary>(pt);
    if (mil)
    {
        if ((mil->GetBuildingType() == BLD_BARRACKS || mil->GetBuildingType() == BLD_GUARDHOUSE) && mil->GetFrontierDistance() == 0 && !mil->IsGoldDisabled())
        {
            aii->ToggleCoins(pt);
        }

        // if near border and gold disabled (by addon): enable it
        if (mil->GetFrontierDistance() && mil->IsGoldDisabled())
        {
            aii->ToggleCoins(pt);
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
    for (std::list<nobBaseWarehouse*>::const_iterator it = aii->GetStorehouses().begin(); it != aii->GetStorehouses().end(); it++)
    {
        if (gwb.CalcDistance((*it)->GetPos(), pt) < 20)
        {
            numBldToTest = 1;
            break;
        }
    }
    //same is true for warehouses which are still in production
    for(std::list<noBuildingSite*>::const_iterator it = aii->GetBuildingSites().begin(); it != aii->GetBuildingSites().end(); it++)
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
            pt = aii->GetNeighbour(pt, Direction::NORTH);
            for(int i = 2; i < 8; i++) //range 1
            {
                const noBaseBuilding* bb;
                const noBuildingSite* bs;
                if((bb = aii->GetSpecObj<noBaseBuilding>(pt)))
                {
                    aii->DestroyBuilding(pt);
                }
                if((bs = aii->GetSpecObj<noBuildingSite>(pt)))
                {
                    aii->DestroyFlag(gwb.GetNeighbour(pt, 4));
                }
                pt = aii->GetNeighbour(pt, Direction(i));
            }
            pt = aii->GetNeighbour(pt, Direction::NORTH);
            for(int i = 2; i < 8; i++) //range 2
            {
                for(int r = 0; r < 2; r++)
                {
                    const noBaseBuilding* bb;
                    const noBuildingSite* bs;
                    if((bb = aii->GetSpecObj<noBaseBuilding>(pt)))
                    {
                        aii->DestroyBuilding(pt);
                    }
                    if((bs = aii->GetSpecObj<noBuildingSite>(pt)))
                    {
                        aii->DestroyFlag(gwb.GetNeighbour(pt, 4));
                    }
                    pt = aii->GetNeighbour(pt, Direction(i));
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
    if(!(flag = aii->GetSpecObj<noFlag>(pt)))
        return;
    //does the roadsegment still exist?
    if(!flag->routes[dir])
        return;
	if(flag->routes[dir]->GetLength()<4) //road too short to need flags
		return;
    //check if this road leads to a warehouseflag and if it does start setting flags from the warehouseflag else from the new flag
    //goal is to move roadsegments with a length of more than 2 away from the warehouse
    MapPoint t = flag->routes[dir]->GetOtherFlag(flag)->GetPos();
    t = gwb.GetNeighbour(t, 1);
	construction->constructionlocations.push_back(t);
    if(aii->IsBuildingOnNode(t, BLD_STOREHOUSE) || aii->IsBuildingOnNode(t, BLD_HARBORBUILDING) || aii->IsBuildingOnNode(t, BLD_HEADQUARTERS))
    {
        t = gwb.GetNeighbour(t, 4);
        for(unsigned i = 0; i < flag->routes[dir]->GetLength(); ++i)
        {
            t = aii->GetNeighbour(t, Direction::fromUInt(flag->routes[dir]->GetDir(true, i)));
            {
                aii->SetFlag(t);
            }
        }
    }
    else
    {
        //set flags on our new road starting from the new flag
        for(unsigned i = 0; i < flag->routes[dir]->GetLength(); ++i)
        {
            pt = aii->GetNeighbour(pt, Direction::fromUInt(flag->routes[dir]->GetDir(false, i)));
            {
                aii->SetFlag(pt);
            }
        }
    }
}

void AIPlayerJH::HandleRoadConstructionFailed(const MapPoint pt, unsigned char dir)
{
    const noFlag* flag;
    //does the flag still exist?
    if(!(flag = aii->GetSpecObj<noFlag>(pt)))
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
    if(aii->GetStorehouses().empty()) //check if we have a storehouse left - if we dont have one trying to find a path to one will crash
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
            aii->ChangeReserve(pt, 0, 1); //order 1 defender to stay in the harborbuilding

            //if there are positions free start an expedition!
            if(HarborPosRelevant(gwb.GetHarborPointID(pt), true))
            {
                aii->StartExpedition(pt);
            }
            break;

        case BLD_SHIPYARD:
            aii->ToggleShipYardMode(pt);
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
    construction->AddConnectFlagJob(aii->GetSpecObj<noFlag>(aii->GetNeighbour(pt, Direction::SOUTHWEST)));
}

void AIPlayerJH::HandleExpedition(const noShip* ship)
{
    if(!ship->IsWaitingForExpeditionInstructions())
        return;
    if (ship->IsAbleToFoundColony())
        aii->FoundColony(ship);
    else
    {
        unsigned char start = rand() % Direction::COUNT;
        for(unsigned char i = start; i < start + Direction::COUNT; ++i)
        {
            if (aii->IsExplorationDirectionPossible(ship->GetPos(), ship->GetCurrentHarbor(), Direction(i)))
            {
                aii->TravelToNextSpot(Direction(i), ship);
                return;
            }
        }
        // no direction possible, sad, stop it
        aii->CancelExpedition(ship);
    }
}

void AIPlayerJH::HandleExpedition(const MapPoint pt)
{
    std::vector<noBase*> objs = aii->GetDynamicObjects(pt);
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

    nodes[aii->GetIdx(pt)].reachable = true;

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

    if (aii->IsObjectTypeOnNode(pt, NOP_BUILDING))
    {
        //keep 2 woodcutters for each forester even if they sometimes run out of trees
        if(bld == BLD_WOODCUTTER)
        {
            for (std::list<nobUsual*>::const_iterator it = aii->GetBuildings(BLD_FORESTER).begin(); it != aii->GetBuildings(BLD_FORESTER).end(); it++)
            {
                //is the forester somewhat close?
                if(gwb.CalcDistance(pt, (*it)->GetPos()) < 6)
                    //then find it's 2 woodcutters
                {
                    unsigned maxdist = gwb.CalcDistance(pt, (*it)->GetPos());
                    char betterwoodcutters = 0;
                    for (std::list<nobUsual*>::const_iterator it2 = aii->GetBuildings(BLD_WOODCUTTER).begin(); it2 != aii->GetBuildings(BLD_WOODCUTTER).end() && betterwoodcutters < 2; it2++)
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
        aii->DestroyBuilding(pt);
		if(bld==BLD_FISHERY) //fishery cant find fish? set fish value at location to 0 so we dont have to calculate the value for this location again
			SetResourceMap(AIJH::FISH, aii->GetIdx(pt), 0);
    }
    else
        return;
    UpdateNodesAround(pt, 11); // todo: fix radius
    RemoveUnusedRoad(aii->GetSpecObj<noFlag>(aii->GetNeighbour(pt, Direction::SOUTHWEST)), 1, true);

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
    if (((aii->GetShipCount() > 6 || aii->GetShipCount() >= (3 * aii->GetBuildings(BLD_SHIPYARD).size())) && GetCountofAIRelevantSeaIds() > 1) || (GetCountofAIRelevantSeaIds() < 2 && aii->GetShipCount() > gwb.GetHarborPointCount()))
    {
        unsigned mindist = 255;
        nobUsual* shipyard = NULL;
        for (std::list<nobUsual*>::const_iterator it = aii->GetBuildings(BLD_SHIPYARD).begin(); it != aii->GetBuildings(BLD_SHIPYARD).end(); it++)
        {
            if(aii->CalcDistance((*it)->GetPos(), pt) < mindist)
            {
                mindist = aii->CalcDistance((*it)->GetPos(), pt);
                shipyard = *it;
            }
        }
        if(shipyard && mindist < 12)//might have been destroyed by now and anything further away than 12 should be wrong anyways
            aii->ToggleProduction( shipyard->GetPos() );
    }
}

void AIPlayerJH::HandleBorderChanged(const MapPoint pt)
{
    UpdateNodesAround(pt, 11); // todo: fix radius

    const nobMilitary* mil = aii->GetSpecObj<nobMilitary>(pt);
    if (mil)
    {
        if (mil->GetFrontierDistance() != 0 && mil->IsGoldDisabled())
        {
            aii->ToggleCoins(pt);
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
	for (std::list<nobMilitary*>::const_iterator it = aii->GetMilitaryBuildings().begin(); it != aii->GetMilitaryBuildings().end(); it++)
	{
		if (count!=upb) //not upgrade building
		{
			if(upb>=0) //we do have an upgrade building
			{
				if(!(*it)->IsGoldDisabled()) // deactivate gold for all other buildings
				{
					aii->ToggleCoins((*it)->GetPos());
				}
				if ((*it)->GetFrontierDistance()==0 && (((unsigned)count+PlannedConnectedInlandMilitary()) < aii->GetMilitaryBuildings().size()) ) //send out troops until 1 private is left, then cancel road
				{
					if ((*it)->GetTroopsCount()>1) //more than 1 soldier remaining? -> send out order
					{
						aii->SendSoldiersHome((*it)->GetPos());
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
					aii->ToggleCoins((*it)->GetPos());
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
				aii->ToggleCoins((*it)->GetPos());
			}
			if((*it)->HasMaxRankSoldier()) // has max rank soldier? send it/them out!
				aii->SendSoldiersHome((*it)->GetPos());
			if(SoldierAvailable(0) && (*it)->GetTroopsCount() < (unsigned)((*it)->GetBuildingType()==BLD_WATCHTOWER?TROOPS_COUNT[aii->GetNation()][2]:TROOPS_COUNT[aii->GetNation()][3])) //building not full and privates in a warehouse? order new!
				aii->OrderNewSoldiers((*it)->GetPos());
		}
		count++;
	}
}


void AIPlayerJH::Chat(std::string message)
{
    GameMessage_Server_Chat chat = GameMessage_Server_Chat(playerid, CD_ALL, message);
    GAMESERVER.AIChat(chat);
}

bool AIPlayerJH::HasFrontierBuildings()
{
	for (std::list<nobMilitary*>::const_iterator it = aii->GetMilitaryBuildings().begin(); it!=aii->GetMilitaryBuildings().end();it++)
	{
		if((*it)->GetFrontierDistance()>0)
			return true;
	}
	return false;
}

void AIPlayerJH::TryToAttack()
{
    unsigned hq_or_harbor_without_soldiers = 0;
    std::deque<const nobBaseMilitary*> potentialTargets;

    // use own military buildings (except inland buildings) to search for enemy military buildings
    unsigned skip = 0; //when the ai has many buildings the ai will not check the complete list every time
    unsigned limit = 40;
    if(aii->GetMilitaryBuildings().size() > 40)
    {
        skip = max<unsigned>((rand() % ((aii->GetMilitaryBuildings().size() / 40) + 1)) * 40, 1) - 1;
    }
    for (std::list<nobMilitary*>::const_iterator it = aii->GetMilitaryBuildings().begin(); limit > 0 && it != aii->GetMilitaryBuildings().end(); it++)
    {
        limit--;
        if(skip > 0)
            std::advance(it, skip);
        skip = 0;
        const nobMilitary* mil = (*it);
        if (mil->GetFrontierDistance() == 0)  //inland building? -> skip it
        {            
            continue;
        }

        // get nearby enemy buildings and store in set of potential attacking targets
        MapPoint src = (*it)->GetPos();

        sortedMilitaryBlds buildings = aii->GetMilitaryBuildings(src, 2);
        for(sortedMilitaryBlds::iterator target = buildings.begin(); target != buildings.end(); ++target)
        {
            if ((*target)->GetGOT() == GOT_NOB_MILITARY)
            {
                const nobMilitary* enemyTarget = dynamic_cast<const nobMilitary*>((*target));

                if (enemyTarget && enemyTarget->IsNewBuilt())
                    continue;
            }
            MapPoint dest = (*target)->GetPos();
            if (gwb.CalcDistance(src, dest) < BASE_ATTACKING_DISTANCE
                    && aii->IsPlayerAttackable((*target)->GetPlayer()) && aii->IsVisible(dest))
            {
                if (((*target)->GetGOT() != GOT_NOB_MILITARY) && (!(*target)->DefendersAvailable()))
                {
                    // headquarter or harbor without any troops :)
                    hq_or_harbor_without_soldiers++;
                    potentialTargets.push_front(*target);
                }
                else
                {
                    potentialTargets.push_back(*target);
                }
            }
        }
    }

    // shuffle everything but headquarters and harbors without any troops in them
    std::random_shuffle(potentialTargets.begin() + hq_or_harbor_without_soldiers, potentialTargets.end());

    // check for each potential attacking target the number of available attacking soldiers
    for (std::deque<const nobBaseMilitary*>::iterator target = potentialTargets.begin(); target != potentialTargets.end(); target++)
    {
        const MapPoint dest = (*target)->GetPos();

        unsigned attackersCount = 0;
        unsigned attackersStrength = 0;

        // ask each of nearby own military buildings for soldiers to contribute to the potential attack
        sortedMilitaryBlds myBuildings = aii->GetMilitaryBuildings(dest, 2);
        for(sortedMilitaryBlds::iterator it3 = myBuildings.begin(); it3 != myBuildings.end(); ++it3)
        {
            if ((*it3)->GetPlayer() == playerid)
            {
                const nobMilitary* myMil;
                myMil = dynamic_cast<const nobMilitary*>(*it3);
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
            const nobMilitary* enemyTarget = dynamic_cast<const nobMilitary*>((*target));

            if (enemyTarget && ((attackersStrength <= enemyTarget->GetSoldiersStrength()) || (enemyTarget->GetTroopsCount() == 0)))
            {
                continue;
            }
        }

        aii->Attack(dest, attackersCount, true);

        return;
    }
}

void AIPlayerJH::TrySeaAttack()
{
    if(aii->GetShipCount() < 1)
        return;
    if(aii->GetHarbors().size() < 1)
        return;
    std::vector<unsigned short>seaidswithattackers;
    std::vector<unsigned int>attackersatseaid;
    std::vector<int> invalidseas;
    std::deque<const nobBaseMilitary*> potentialTargets;
    std::deque<const nobBaseMilitary*> undefendedTargets;
    std::vector<int> searcharoundharborspots;
    //all seaids with at least 1 ship count available attackers for later checks
    for(std::vector<noShip*>::const_iterator it = aii->GetShips().begin(); it != aii->GetShips().end(); it++)
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
    if(seaidswithattackers.size() < 1) //no sea ids with attackers? skip the rest
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
        if((hb = aii->GetSpecObj<nobHarborBuilding>(gwb.GetHarborPoint(i))))
        {
            if(aii->IsVisible(hb->GetPos()))
            {
                if(aii->IsPlayerAttackable(hb->GetPlayer()))
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
    if(undefendedTargets.size() > 0)
    {
        std::random_shuffle(undefendedTargets.begin(), undefendedTargets.end());
        for(std::deque<const nobBaseMilitary*>::iterator it = undefendedTargets.begin(); it != undefendedTargets.end(); it++)
        {
            std::vector<GameWorldBase::PotentialSeaAttacker> attackers = gwb.GetAvailableSoldiersForSeaAttack(playerid, (*it)->GetPos());
            if(!attackers.empty()) //try to attack it!
            {
                aii->SeaAttack((*it)->GetPos(), 1, true);
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
        sortedMilitaryBlds buildings = aii->GetMilitaryBuildings(gwb.GetHarborPoint(searcharoundharborspots[i]), 2);
        for(sortedMilitaryBlds::const_iterator it = buildings.begin(); it != buildings.end(); it++)
        {
            if(aii->IsPlayerAttackable((*it)->GetPlayer()) && aii->IsVisible((*it)->GetPos()))
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
    if(undefendedTargets.size() > 0)
    {
        std::random_shuffle(undefendedTargets.begin(), undefendedTargets.end());
        for(std::deque<const nobBaseMilitary*>::iterator it = undefendedTargets.begin(); it != undefendedTargets.end(); it++)
        {
            std::vector<GameWorldBase::PotentialSeaAttacker> attackers = gwb.GetAvailableSoldiersForSeaAttack(playerid, (*it)->GetPos());
            if(!attackers.empty()) //try to attack it!
            {
                aii->SeaAttack((*it)->GetPos(), 1, true);
                return;
            }
        }
    }
    std::random_shuffle(potentialTargets.begin(), potentialTargets.end());
    for(std::deque<const nobBaseMilitary*>::iterator it = potentialTargets.begin(); it != potentialTargets.end(); it++)
    {
        std::vector<unsigned short> testseaidswithattackers(seaidswithattackers); //TODO: decide if it is worth attacking the target and not just "possible"
        gwb.GetValidSeaIDsAroundMilitaryBuildingForAttackCompare((*it)->GetPos(), testseaidswithattackers, playerid); //test only if we should have attackers from one of our valid sea ids
        if(testseaidswithattackers.size() > 0) //only do the final check if it will probably be a good result
        {
            std::vector<GameWorldBase::PotentialSeaAttacker> attackers = gwb.GetAvailableSoldiersForSeaAttack(playerid, (*it)->GetPos()); //now get a final list of attackers and attack it
            if(!attackers.empty())
            {
                aii->SeaAttack((*it)->GetPos(), attackers.size(), true);
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
        ChangeResourceMap(pt, AIJH::RES_RADIUS[AIJH::PLANTSPACE], resourceMaps[AIJH::PLANTSPACE], -1);
        GetAINode(pt).res = AIJH::NOTHING;
    }

    // flag of building
    pt = aii->GetNeighbour(pt,Direction::SOUTHWEST);
    RecalcBQAround(pt);
    if (GetAINode(pt).res == AIJH::PLANTSPACE)
    {
        ChangeResourceMap(pt, AIJH::RES_RADIUS[AIJH::PLANTSPACE], resourceMaps[AIJH::PLANTSPACE], -1);
        GetAINode(pt).res = AIJH::NOTHING;
    }

    // along the road
    for (unsigned i = 0; i < route_road.size(); ++i)
    {
        pt = aii->GetNeighbour(pt, Direction::fromUInt(route_road[i]));
        RecalcBQAround(pt);
        // Auch Plantspace entsprechend anpassen:
        if (GetAINode(pt).res == AIJH::PLANTSPACE)
        {
            ChangeResourceMap(pt, AIJH::RES_RADIUS[AIJH::PLANTSPACE], resourceMaps[AIJH::PLANTSPACE], -1);
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
        for (unsigned y = 0; y < aii->GetMapHeight(); ++y)
        {
            if (y % 2 == 1)
                fprintf(file, "  ");
            for (unsigned x = 0; x < aii->GetMapWidth(); ++x)
            {
                fprintf(file, "%i   ", resourceMaps[i][aii->GetIdx(MapPoint(x, y))]);
            }
            fprintf(file, "\n");
        }
        fclose(file);
    }
#endif
}

int AIPlayerJH::GetResMapValue(const MapPoint pt, AIJH::Resource res)
{
    return resourceMaps[res][aii->GetIdx(pt)];
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
        if(testdir == 1 && (aii->IsObjectTypeOnNode(aii->GetNeighbour(curFlag->GetPos(), Direction::NORTHEAST), NOP_BUILDING) || aii->IsObjectTypeOnNode(aii->GetNeighbour(curFlag->GetPos(), Direction::NORTHEAST), NOP_BUILDINGSITE)))
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
    while(reconnectflags.size() > 0)
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
        if(dir == 1 && (aii->IsObjectTypeOnNode(aii->GetNeighbour(startFlag->GetPos(), Direction::NORTHEAST), NOP_BUILDING) || aii->IsObjectTypeOnNode(aii->GetNeighbour(startFlag->GetPos(), Direction::NORTHEAST), NOP_BUILDINGSITE)))
        {
            //the flag belongs to a building - update the pathing map around us and try to reconnect it (if we cant reconnect it -> burn it(burning takes place at the pathfinding job))
            finds += 3;
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
    {
        return false;
    }
    else
    {
        if(finds == 2)
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
    }

    // kill the flag
	if(keepstartflag)
	{
		if(foundDir<6)
			aii->DestroyRoad(startFlag->GetPos(),foundDir);
	}
	else
		aii->DestroyFlag(startFlag);

    // nothing found?
    if (foundDir > 6)
    {
        return false;
    }
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
    for (std::list<nobBaseWarehouse*>::const_iterator it = aii->GetStorehouses().begin(); it != aii->GetStorehouses().end(); it++)
    {
		if(rank<0 || rank>4)
			freeSoldiers += ((*it)->GetInventory()->people[JOB_PRIVATE] + (*it)->GetInventory()->people[JOB_PRIVATEFIRSTCLASS] + (*it)->GetInventory()->people[JOB_SERGEANT] + (*it)->GetInventory()->people[JOB_OFFICER] + (*it)->GetInventory()->people[JOB_GENERAL]);
		else
			freeSoldiers += ((*it)->GetInventory()->people[rank+21]);
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
    if(pt.x + SQUARE_SIZE < aii->GetMapWidth()) lx = pt.x + SQUARE_SIZE; else lx = aii->GetMapWidth() - 1;
    if(pt.y + SQUARE_SIZE < aii->GetMapHeight()) ly = pt.y + SQUARE_SIZE; else ly = aii->GetMapHeight() - 1;
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
    for(std::list<nobUsual*>::const_iterator it = aii->GetBuildings(BLD_FARM).begin(); it != aii->GetBuildings(BLD_FARM).end(); it++)
    {
        SetFarmedNodes((*it)->GetPos(), true);
    }
    for(std::list<nobUsual*>::const_iterator it = aii->GetBuildings(BLD_CHARBURNER).begin(); it != aii->GetBuildings(BLD_CHARBURNER).end(); it++)
    {
        SetFarmedNodes((*it)->GetPos(), true);
    }
	//find the upgradebuilding
	UpgradeBldX=0;
	UpgradeBldY=0;
	UpdateUpgradeBuilding();
}
int AIPlayerJH::UpdateUpgradeBuilding()
{
	std::list<nobMilitary*> backup;
	if(!aii->GetStorehouses().empty())
	{		
		unsigned count=0;
		for (std::list<nobMilitary*>::const_iterator it = aii->GetMilitaryBuildings().begin(); it!=aii->GetMilitaryBuildings().end(); it++)
		{
			//inland building, tower or fortress, connected to warehouse 1 
			if((*it)->GetBuildingType()>=BLD_WATCHTOWER && (*it)->GetFrontierDistance()<1 && construction->IsConnectedToRoadSystem((*it)->GetFlag()))
			{
				if (construction->IsConnectedToRoadSystem((*it)->GetFlag()))
				{
					//LOG.lprintf("UpdateUpgradeBuilding at %i,%i for player %i (listslot %i) \n",(*it)->GetX(), (*it)->GetY(), playerid, count);
					UpgradeBldX=(*it)->GetX();
					UpgradeBldY=(*it)->GetY();
					UpgradeBldListNumber=count;
					return count;
				}
				backup.push_back(*it);
				
			}
			count++;
		}
	}
	//no valid upgrade building yet - try to reconnect correctly flagged buildings
	for (std::list<nobMilitary*>::const_iterator it=backup.begin();it!=backup.end();it++)
	{
		construction->AddConnectFlagJob((*it)->GetFlag());
	}
	UpgradeBldX=0;
	UpgradeBldY=0;
	UpgradeBldListNumber=-1;
	return -1;
}
//set default start values for the ai for distribution & military settings
void AIPlayerJH::InitDistribution()
{	
        //set good distribution settings
        std::vector<unsigned char> goodSettings;
        goodSettings.resize(23);
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
        aii->ChangeDistribution(goodSettings);
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
	if (!aii->CanBuildBuildingtype(bt)) // not allowed to build this buildingtype? -> do nothing!
		return;
	if (forced) //fixed location - just a direct gamecommand to build buildingtype at location (no checks if this is a valid & good location from the ai)
	{
		aii->SetBuildingSite(pt, bt);
		AIJH::BuildJob* j=new AIJH::BuildJob(this, bt, pt);
		j->SetStatus(AIJH::JOB_EXECUTING_ROAD1);
		j->SetTarget(pt);
		construction->AddBuildJob(j,true); //connects the buildingsite to roadsystem
	}
	else 
	{
		if (construction->Wanted(bt))
		{
			construction->AddBuildJob(new AIJH::BuildJob(this, bt, pt), true); //add build job to the front of the list
		}
	}
}

bool AIPlayerJH::BuildingNearby(const MapPoint pt, BuildingType bld, unsigned min)
{
    //assert not a military building
    assert(bld >= 10);
    for(std::list<nobUsual*>::const_iterator it = aii->GetBuildings(bld).begin(); it != aii->GetBuildings(bld).end(); it++)
    {
        if(gwb.CalcDistance(pt, (*it)->GetPos()) < min)
            return true;
    }
    for(std::list<noBuildingSite*>::const_iterator it = aii->GetBuildingSites().begin(); it != aii->GetBuildingSites().end(); it++)
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
            if(!onlyempty || !aii->IsBuildingOnNode(gwb.GetHarborPoint(i), BLD_HARBORBUILDING))
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
	if (( aii->GetBuildingQuality(pt)>=BQ_HUT && aii->GetBuildingQuality(pt) <= BQ_CASTLE) || aii->GetBuildingQuality(pt) == BQ_HARBOR)
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
                if (( aii->GetBuildingQualityAnyOwner(t2)>=BQ_HUT && aii->GetBuildingQualityAnyOwner(t2) <= BQ_CASTLE) || aii->GetBuildingQualityAnyOwner(t2) == BQ_HARBOR)
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
        assert(false);
        return false;
    }
    //get sea ids of harbor id given - is there at least 1 sea id? if so check for other harbors with the same id!
    unsigned short sea_ids[6];
    gwb.GetSeaIDs(harborid, sea_ids);
    for(unsigned r = 0; r < 6; r++)
    {
        if(sea_ids[r] > 0) //there is a sea id? -> check all other harbors to find if there is another at the same id!
        {
            for(unsigned i = 1; i <= gwb.GetHarborPointCount(); i++) //start at 1 harbor dummy yadayada :>
            {
                if(i != harborid && gwb.IsAtThisSea(i, sea_ids[r]))
                {
                    if(onlyempty) //check if the spot is actually free for colonization?
                    {
                        if(gwb.IsHarborPointFree(i, playerid, sea_ids[r]))
                        {
                            return true;
                        }
                    }
                    else
                        return true;
                }
            }
        }
    }
    return false;
}

bool AIPlayerJH::NoEnemyHarbor()
{
    for(unsigned i = 1; i <= gwb.GetHarborPointCount(); i++)
    {
        if(aii->IsBuildingOnNode(gwb.GetHarborPoint(i), BLD_HARBORBUILDING) && !aii->IsOwnTerritory(gwb.GetHarborPoint(i)))
        {
            //LOG.lprintf("found a harbor at spot %i ",i);
            return false;
        }
    }
    return true;
}

bool AIPlayerJH::IsInvalidShipyardPosition(const MapPoint pt)
{
    if (BuildingNearby(pt, BLD_SHIPYARD, 20) || !HarborPosClose(pt, 8))
        return true;
    return false;

}

unsigned AIPlayerJH::AmountInStorage(unsigned char num,unsigned char page)
{
	unsigned counter=0;
	for(std::list<nobBaseWarehouse*>::const_iterator it=aii->GetStorehouses().begin();it!=aii->GetStorehouses().end();it++)
	{
		if(page==0)
			counter+=(*it)->GetInventory()->goods[num];
		else
			counter+=(*it)->GetInventory()->people[num];
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
    unsigned short sea_ids[6];
    for(unsigned i = 1; i <= gwb.GetHarborPointCount(); i++)
    {
        //get sea ids of harbor id given
        gwb.GetSeaIDs(i, sea_ids);
        for(unsigned r = 0; r < 6; r++)
        {
            if(sea_ids[r] > 0) //there is a sea id? -> check if it is already a validid or a once found id
            {
                if(!helpers::contains(validseaids, sea_ids[r])) //not yet in validseas?
                {
                    if(!helpers::contains(onetimeuseseaids, sea_ids[r])) //not yet in onetimeuseseaids?
                        onetimeuseseaids.push_back(sea_ids[r]);
                    else
                    {
                        //LOG.lprintf("found a second harbor at sea id %i \n",sea_ids[r]);
                        onetimeuseseaids.remove(sea_ids[r]);
                        validseaids.push_back(sea_ids[r]);
                    }
                }
                else
                    continue;
            }
        }
    }
    return validseaids.size();
}

void AIPlayerJH::AdjustSettings()
{
	//update tool creation settings
    std::vector<unsigned char> toolsettings;
    toolsettings.resize(12);
    toolsettings[2] = (aii->GetInventory()->goods[GD_SAW] + aii->GetInventory()->people[JOB_CARPENTER] < 2) ? 4 : aii->GetInventory()->goods[GD_SAW] < 1 ? 1 : 0;                                                                       //saw
    toolsettings[3] = (aii->GetInventory()->goods[GD_PICKAXE] < 1) ? 1 : 0;                                                                                                                     //pickaxe
    toolsettings[4] = (aii->GetInventory()->goods[GD_HAMMER] < 1) ? 1 : 0;                                                                                                                      //hammer
    toolsettings[6] = (aii->GetInventory()->goods[GD_CRUCIBLE] + aii->GetInventory()->people[JOB_IRONFOUNDER] < construction->GetBuildingCount(BLD_IRONSMELTER) + 1) ? 1 : 0;;                   //crucible
    toolsettings[8] = (toolsettings[4] < 1 && toolsettings[3] < 1 && toolsettings[6] < 1 && toolsettings[2] < 1 && (aii->GetInventory()->goods[GD_SCYTHE] < 1)) ? 1 : 0;                        //scythe
    toolsettings[10] = (aii->GetInventory()->goods[GD_ROLLINGPIN] + aii->GetInventory()->people[JOB_BAKER] < construction->GetBuildingCount(BLD_BAKERY) + 1) ? 1 : 0;                            //rollingpin
    toolsettings[5] = (toolsettings[4] < 1 && toolsettings[3] < 1 && toolsettings[6] < 1 && toolsettings[2] < 1 && (aii->GetInventory()->goods[GD_SHOVEL] < 1)) ? 1 : 0 ;                       //shovel
    toolsettings[1] = (toolsettings[4] < 1 && toolsettings[3] < 1 && toolsettings[6] < 1 && toolsettings[2] < 1 && (aii->GetInventory()->goods[GD_AXE] + aii->GetInventory()->people[JOB_WOODCUTTER] < 12) && aii->GetInventory()->goods[GD_AXE] < 1) ? 1 : 0; //axe
    toolsettings[0] = 0; //(toolsettings[4]<1&&toolsettings[3]<1&&toolsettings[6]<1&&toolsettings[2]<1&&(aii->GetInventory()->goods[GD_TONGS]<1))?1:0;                                                //Tongs(metalworks)
    toolsettings[9] = 0; //(aii->GetInventory()->goods[GD_CLEAVER]+aii->GetInventory()->people[JOB_BUTCHER]<construction->GetBuildingCount(BLD_SLAUGHTERHOUSE)+1)?1:0;                                //cleaver
    toolsettings[7] = 0;                                                                                                                                                                        //rod & line
    toolsettings[11] = 0;                                                                                                                                                                       //bow
    aii->ChangeTools(toolsettings);

    // Set military settings to some currently required values
    std::vector<unsigned char> milSettings;
    milSettings.resize(8);
    milSettings[0] = 10;
    milSettings[1] = HasFrontierBuildings()?5:0; //if we have a front send strong soldiers first else weak first to make upgrading easier
    milSettings[2] = 4;
    milSettings[3] = 5;
	//interior 0bar full if we have an upgrade building and gold(or produce gold) else 1 soldier each
	milSettings[4] = UpdateUpgradeBuilding() >= 0 && (aii->GetInventory()->goods[GD_COINS]>0 || (aii->GetInventory()->goods[GD_GOLD]>0 && aii->GetInventory()->goods[GD_COAL]>0 && aii->GetBuildings(BLD_MINT).size()) )? 8 : 0;     
    milSettings[6] = ggs.getSelection(ADDON_SEA_ATTACK)==2 ? 0 : 8; //harbor flag: no sea attacks?->no soldiers else 50% to 100%
	milSettings[5] = CalcMilSettings(); //inland 1bar min 50% max 100% depending on how many soldiers are available
	milSettings[7] = 8;                                                     //front: 100%
	if(player.military_settings[5] != milSettings[5] || player.military_settings[6] != milSettings[6] || player.military_settings[4]!=milSettings[4] || player.military_settings[1]!=milSettings[1]) //only send the command if we want to change something
		aii->ChangeMilitary(milSettings);
}

unsigned AIPlayerJH::CalcMilSettings()
{
	///first sum up all soldiers we have
	unsigned milrank=JOB_PRIVATE;
	unsigned soldiercount=0;
	unsigned soldierinusefixed=0; 
	int uun=UpdateUpgradeBuilding();
	int count=0;	
	unsigned InlandTroops[5]= {0,0,0,0,0}; //how many troops are required to fill inland buildings at settings 4,5,6,7,8
	unsigned maxtroops[5][4]= //max troops in the military buildings at settings 4-8
	{
		{1,2,3,5},
		{1,2,4,6},
		{1,2,4,7},
		{1,2,5,8},
		{2,3,6,9}
	};
	unsigned howmanyshouldstayconnected=PlannedConnectedInlandMilitary();
	for (unsigned i=0;i<5;i++){soldiercount += aii->GetInventory()->people[milrank++];}
	
	//now add up all counts of soldiers that are fixed in use and those that depend on whatever we have as a result
	for (std::list<nobMilitary*>::const_iterator it=aii->GetMilitaryBuildings().begin();it!=aii->GetMilitaryBuildings().end();it++)
	{		
		unsigned convtype=0;
		if((*it)->GetBuildingType()==BLD_BARRACKS)
			convtype=0;
		if((*it)->GetBuildingType()==BLD_GUARDHOUSE)			
			convtype=1;
		if((*it)->GetBuildingType()==BLD_WATCHTOWER)
			convtype=2;
		if((*it)->GetBuildingType()==BLD_FORTRESS)
			convtype=3;
		if((*it)->GetFrontierDistance()==3 || ((*it)->GetFrontierDistance()==2 && ggs.getSelection(ADDON_SEA_ATTACK)!=2) || ((*it)->GetFrontierDistance()==0 && (aii->GetMilitaryBuildings().size()-howmanyshouldstayconnected < (unsigned)count || count==uun)))//front or connected interior
		{
			soldierinusefixed+=maxtroops[4][convtype];
		}
		else if((*it)->GetFrontierDistance()==1) //1 bar (inland)
		{
			for(int i=0;i<5;i++)
				InlandTroops[i]+=maxtroops[i][convtype];	
		}
		else//setting should be 0 so add 1 soldier
			soldierinusefixed++;

		count++;
	}
	
	//now the current need total and for inland and harbor is ready for use
	unsigned returnvalue=8;
	while (returnvalue - 4 > 0)
	{//have more than enough soldiers for this setting or just enough and this is the current setting? -> return it else try the next lower setting down to 4 (50%)
		if(soldierinusefixed + InlandTroops[returnvalue - 4] < soldiercount*10/11 || (player.military_settings[5]>=returnvalue && soldierinusefixed + InlandTroops[returnvalue - 4] < soldiercount))
			break;
		returnvalue--;
	}
	//LOG.lprintf("player %i inland milsetting %i \n",playerid,returnvalue);
	return returnvalue;
}
