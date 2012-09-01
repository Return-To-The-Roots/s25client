// $Id: AIPlayerJH.cpp 8120 2012-09-01 19:13:00Z jh $
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
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


#include "main.h"
#include "AIPlayerJH.h"

#include "GameClientPlayer.h"
#include "GameWorld.h"
#include "GameCommands.h"
#include "GamePlayerList.h"

#include "nobMilitary.h"
#include "nobHQ.h"
#include "noBuildingSite.h"
#include "noShip.h"
#include "noFlag.h"
#include "noTree.h"
#include "noAnimal.h"

#include "MapGeometry.h"

#include <iostream>
#include <list>

#include "GameMessages.h"
#include "GameServer.h"

// from Pathfinding.cpp
bool IsPointOK_RoadPath(const GameWorldBase& gwb, const MapCoord x, const MapCoord y, const unsigned char dir, const void *param);

AIPlayerJH::AIPlayerJH(const unsigned char playerid, const GameWorldBase * const gwb, const GameClientPlayer * const player,
		const GameClientPlayerList * const players, const GlobalGameSettings * const ggs,
		const AI::Level level) : AIBase(playerid, gwb, player, players, ggs, level), defeated(false), 
		construction(AIConstruction(aii, this))
{
	currentJob = 0;
	InitNodes();
	InitResourceMaps();
	SaveResourceMapsToFile();
}

/// Wird jeden GF aufgerufen und die KI kann hier entsprechende Handlungen vollziehen
void AIPlayerJH::RunGF(const unsigned gf)
{
	if (defeated)
		return;

	if (TestDefeat())
		return;

	if ((gf + (playerid * 11)) % 3 == 0) //try to complete a job on the list
	{
		construction.RefreshBuildingCount();
		ExecuteAIJob();
	}

	if ((gf + playerid * 17) % 1000 == 0)
	{
		//CheckExistingMilitaryBuildings();
		TryToAttack();
	}

	if ((gf + playerid * 13) % 100 == 0)
	{
		CheckNewMilitaryBuildings();
	}
	if((gf+playerid*11)%1500==0) //update tool creation settings
	{
		std::vector<unsigned char> toolsettings;
		toolsettings.resize(12);
		toolsettings[2] = (aii->GetInventory()->goods[GD_SAW] + aii->GetInventory()->people[JOB_CARPENTER]<2)?4:0;																					//saw
		toolsettings[3] = (aii->GetInventory()->goods[GD_PICKAXE]<1)?1:0;																															//pickaxe
		toolsettings[4] = (aii->GetInventory()->goods[GD_HAMMER]<1)?1:0;																															//hammer
		toolsettings[6] = (aii->GetInventory()->goods[GD_CRUCIBLE]+aii->GetInventory()->people[JOB_IRONFOUNDER]<construction.GetBuildingCount(BLD_IRONSMELTER)+1)?1:0;;								//crucible
		toolsettings[8]=(toolsettings[4]<1&&toolsettings[3]<1&&toolsettings[6]<1&&toolsettings[2]<1&&(aii->GetInventory()->goods[GD_SCYTHE] <1))?1:0;												//scythe
		toolsettings[9] = (aii->GetInventory()->goods[GD_CLEAVER]+aii->GetInventory()->people[JOB_BUTCHER]<construction.GetBuildingCount(BLD_SLAUGHTERHOUSE)+1)?1:0;								//cleaver
		toolsettings[10] = (aii->GetInventory()->goods[GD_ROLLINGPIN]+aii->GetInventory()->people[JOB_BAKER]<construction.GetBuildingCount(BLD_BAKERY)+1)?1:0;										//rollingpin
		toolsettings[5] =(toolsettings[4]<1&&toolsettings[3]<1&&toolsettings[6]<1&&toolsettings[2]<1&&(aii->GetInventory()->goods[GD_SHOVEL]<1))?1:0 ;												//shovel
		toolsettings[1] =(toolsettings[4]<1&&toolsettings[3]<1&&toolsettings[6]<1&&toolsettings[2]<1&&(aii->GetInventory()->goods[GD_AXE]+aii->GetInventory()->people[JOB_WOODCUTTER]<12)&&aii->GetInventory()->goods[GD_AXE]<1)?1:0;		//axe
		toolsettings[0] =0;//(toolsettings[4]<1&&toolsettings[3]<1&&toolsettings[6]<1&&toolsettings[2]<1&&(aii->GetInventory()->goods[GD_TONGS]<1))?1:0;												//Tongs(metalworks)
		toolsettings[7] = 0;																																										//rod & line 
		toolsettings[11] = 0;																																										//bow
		aii->SetToolSettings(toolsettings);		
	}
	if((gf+playerid*7)%200==0) // plan new buildings
	{
		UpdateNodes();
		RecalcResource(AIJH::GOLD);
		RecalcResource(AIJH::IRONORE);
		RecalcResource(AIJH::COAL);
		RecalcResource(AIJH::GRANITE);
		RecalcResource(AIJH::BORDERLAND);
		RecalcResource(AIJH::WOOD);
		RecalcResource(AIJH::STONES);
		RecalcResource(AIJH::PLANTSPACE);
		construction.RefreshBuildingCount();		
		//pick a random storehouse and try to build one of these buildings around it (checks if we actually want more of the building type)
		BuildingType bldToTest[] = {
		BLD_SAWMILL,
		BLD_FORESTER,
		BLD_IRONSMELTER,
		BLD_MINT,
		BLD_ARMORY,
		BLD_METALWORKS,
		BLD_BREWERY,
		BLD_MILL,
		BLD_PIGFARM,
		BLD_SLAUGHTERHOUSE,
		BLD_BAKERY,
		BLD_DONKEYBREEDER,
		BLD_FARM,
		BLD_FISHERY,
		BLD_WOODCUTTER,
		BLD_QUARRY,
		BLD_GOLDMINE,
		BLD_IRONMINE,
		BLD_COALMINE,
		BLD_GRANITEMINE,
		BLD_HARBORBUILDING,
		BLD_HUNTER
	};
	unsigned numBldToTest = 22;
	//std::list<AIJH::Coords> bldPoses = construction.GetStoreHousePositions();
	unsigned char randomstore=rand()%construction.GetStoreHousePositions().size();
	bool firsthouse=true;
	bool lostmainstore=false;
	if(construction.GetStoreHousePositions().size()<1)
		return;
	for (std::list<AIJH::Coords>::iterator it = construction.GetStoreHousePositions().begin(); it != construction.GetStoreHousePositions().end(); it++)
	{
		//check if there still is a building if not remove from list
		if(!aii->IsObjectTypeOnNode((*it).x,(*it).y,NOP_BUILDING)&&!aii->IsObjectTypeOnNode((*it).x,(*it).y,NOP_BUILDINGSITE))
		{
			lostmainstore=(firsthouse);
			it=construction.GetStoreHousePositions().erase(it);
			if(it==construction.GetStoreHousePositions().end())
			{
				break;
			}
			else
			{
				continue;
			}
		}
		else
		{
			firsthouse=false;
			if(lostmainstore&&aii->IsObjectTypeOnNode((*it).x,(*it).y,NOP_BUILDING))
			{				
				gcs.push_back(new gc::ChangeInventorySetting((*it).x, (*it).y, 0, 2, 0));
				gcs.push_back(new gc::ChangeInventorySetting((*it).x, (*it).y, 0, 2, 16));
				gcs.push_back(new gc::ChangeInventorySetting((*it).x, (*it).y, 0, 2, 21));
				lostmainstore=false;
			}
		}
		if(randomstore>0)
			randomstore--;	
		else
		{
			UpdateNodesAroundNoBorder((*it).x,(*it).y,15); //update the area we want to build in first 
			for (unsigned int i = 0; i < numBldToTest; i++)
			{
				if (construction.Wanted(bldToTest[i]))
				{
					AddBuildJob(bldToTest[i],(*it).x,(*it).y);
				}
			}
			if(gf>1500||aii->GetInventory()->goods[GD_BOARDS]>11)
				AddBuildJob(construction.ChooseMilitaryBuilding((*it).x, (*it).y),(*it).x, (*it).y);			
			break;
		}
		
	}
	//now pick a random military building and try to build around that
	if(milBuildings.size()<1)return;
	randomstore=rand()%milBuildings.size();	
	numBldToTest = 22;
	//std::list<Coords>::iterator it2 = milBuildings.end();
	for (std::list<Coords>::iterator it = milBuildings.begin(); it != milBuildings.end(); it++)
	{
		//order ai to try building new military buildings close to the latest completed military buildings
		//it2--;
		//AddBuildJob(construction.ChooseMilitaryBuilding((*it2).x, (*it2).y),(*it2).x, (*it2).y);  //faster expansion when we have a huge empire BUT if we have lots of mil buildings this clogs the job queue FIX IT
		if(randomstore>0)
			randomstore--;
		const nobMilitary *mil;
		if (!(mil = aii->GetSpecObj<nobMilitary>((*it).x, (*it).y)))
			continue;
		if(randomstore<=0)
		{			
			UpdateNodesAroundNoBorder((*it).x,(*it).y,15); //update the area we want to build in first 
			for (unsigned int i = 0; i < numBldToTest; i++) 
			{
				if (construction.Wanted(bldToTest[i]))
				{
					AddBuildJob(bldToTest[i],(*it).x,(*it).y);
				}
			}
			AddBuildJob(construction.ChooseMilitaryBuilding((*it).x, (*it).y),(*it).x, (*it).y);
			if(mil->IsUseless()&&mil->IsDemolitionAllowed())
			{
				gcs.push_back(new gc::DestroyBuilding((*it).x, (*it).y));
			}
			break;
		}
	}


	
	}
	if (gf == 99)		
	{
		InitStoreAndMilitarylists();
	}
	if (gf == 100)
	{
		if(milBuildings.size()<1)
		{
			Chat(_("Hi, I'm an artifical player and I'm not very good yet!"));
			Chat(_("And I may crash your game sometimes..."));
		}

		// Set military settings to some nicer default values
		std::vector<unsigned char> milSettings;
		milSettings.resize(8);
		milSettings[0] = 10;
		milSettings[1] = 5;
		milSettings[2] = 5;
		milSettings[3] = 5;
		milSettings[4] = 1;
		milSettings[5] = 8;
		milSettings[6] = 8;
		milSettings[7] = 8;
		aii->SetMilitarySettings(milSettings);
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
		aii->SetDistribution(goodSettings);

	}

	// from time to time give some random build orders to keep alive
	
	/*if ((gf % 3000) == 2999)
	{
		if (construction.Wanted(BLD_WOODCUTTER)) AddBuildJob(new AIJH::BuildJob(this, BLD_WOODCUTTER, AIJH::SEARCHMODE_GLOBAL));
		if (construction.Wanted(BLD_QUARRY)) AddBuildJob(new AIJH::BuildJob(this, BLD_QUARRY, AIJH::SEARCHMODE_GLOBAL));
		if (SoldierAvailable()) AddBuildJob(new AIJH::BuildJob(this, BLD_GUARDHOUSE, AIJH::SEARCHMODE_GLOBAL));
	}*/
}

bool AIPlayerJH::TestDefeat()
{
	if (!aii->GetHeadquarter()&&construction.GetBuildingCount(BLD_STOREHOUSE)+construction.GetBuildingCount(BLD_HARBORBUILDING)-construction.GetBuildingSitesCount(BLD_STOREHOUSE)-construction.GetBuildingSitesCount(BLD_STOREHOUSE)<1)
	{
		defeated = true;
		aii->Surrender();
		Chat(_("You win"));
		return true;
	}
	return false;
}

void AIPlayerJH::AddBuildJob(BuildingType type, MapCoord x, MapCoord y, bool front)
{
	construction.AddBuildJob(new AIJH::BuildJob(this, type, x, y), front);
}

void AIPlayerJH::AddJob(AIJH::Job *job, bool front)
{
	construction.AddJob(job, front);
}


void AIPlayerJH::AddBuildJob(BuildingType type)
{
	construction.AddBuildJob(new AIJH::BuildJob(this, type), false);
}

AIJH::Resource AIPlayerJH::CalcResource(MapCoord x, MapCoord y)
{
	AIJH::Resource res = aii->GetSubsurfaceResource(x, y);

	// resources on surface
	if (res == AIJH::NOTHING)
	{
		res = aii->GetSurfaceResource(x, y);

		if(res == AIJH::NOTHING)
		{
			// check terrain
			unsigned char t;
			bool good = !aii->IsRoadPoint(x, y);
			
			for(unsigned char i = 0;i<6;++i)
			{
				t = aii->GetTerrainAround(x, y, i);

				// check against valid terrains for planting
				if(t != 3 && (t < 8 || t > 12))
				{
					good = false;
				}
			}
			if (good)
			{
				res = AIJH::PLANTSPACE;
			}
		}
		else
		{
			if (res==AIJH::WOOD)
			{
				if((gwb->GetSpecObj<noTree>(x,y))->type==5) //exclude ananas trees (because they are more of a "blocker" than a tree and only count as tree for animation&sound
					res=AIJH::NOTHING;
			}
		}
	}
	else
	{
		if(aii->GetSurfaceResource(x,y)==AIJH::STONES||aii->GetSurfaceResource(x,y)==AIJH::WOOD)
			if(aii->GetSubsurfaceResource(x,y)==AIJH::WOOD)
			{
				if((gwb->GetSpecObj<noTree>(x,y))->type!=5)
					res=AIJH::MULTIPLE;
			}
			else
				res=AIJH::MULTIPLE;
			
	}
	if (res == AIJH::BLOCKED)
	{
		res = AIJH::NOTHING; // nicht so ganz logisch... aber Blocked als res is doof TODO
	}
	return res;
}

void AIPlayerJH::InitReachableNodes()
{
	unsigned short width = aii->GetMapWidth();
	unsigned short height = aii->GetMapHeight();

	std::queue<std::pair<MapCoord, MapCoord> > toCheck;

	// Alle auf not reachable setzen
	for (unsigned short y = 0; y < height; ++y)
	{
		for (unsigned short x = 0; x < width; ++x)
		{
			unsigned i = x + y * width;
			nodes[i].reachable = false;
			const noFlag *myFlag = 0;
			if ( (myFlag = aii->GetSpecObj<noFlag>(x, y)) )
			{
				if (myFlag->GetPlayer() == playerid)
				{
					nodes[i].reachable = true;
					toCheck.push(std::make_pair(x,y));
				}
			}
		}
	}

	IterativeReachableNodeChecker(toCheck);
}

void AIPlayerJH::IterativeReachableNodeChecker(std::queue<std::pair<MapCoord, MapCoord> >& toCheck)
{
	unsigned short width = aii->GetMapWidth();

	// TODO auch mal bootswege bauen können
	//Param_RoadPath prp = { false };

	while(toCheck.size() > 0)
	{
		// Reachable coordinate
		MapCoord rx = toCheck.front().first;
		MapCoord ry = toCheck.front().second;

		// Coordinates to test around this reachable coordinate
		for (unsigned dir = 0; dir < 6; ++dir)
		{
			MapCoord nx = aii->GetXA(rx, ry, dir);
			MapCoord ny = aii->GetYA(rx, ry, dir);
			unsigned ni = nx + ny * width;

			// already reached, don't test again
			if (nodes[ni].reachable)
				continue;

			bool boat = false;
			// Test whether point is reachable; yes->add to check list
			if (IsPointOK_RoadPath(*gwb, nx, ny, (dir+3)%6, (void *) &boat))
			{
				nodes[ni].reachable = true;
				toCheck.push(std::make_pair(nx, ny));
			}
		}
		toCheck.pop();
	}
}


void AIPlayerJH::UpdateReachableNodes(MapCoord x, MapCoord y, unsigned radius)
{
	unsigned short width = aii->GetMapWidth();

	std::queue<std::pair<MapCoord, MapCoord> > toCheck;

	for(MapCoord tx = aii->GetXA(x, y, 0), r = 1; r <= radius; tx = aii->GetXA(tx, y, 0), ++r)
	{
		MapCoord tx2 = tx, ty2 = y;
		for(unsigned i = 2; i < 8; ++i)
		{
			for(MapCoord r2 = 0; r2 < r; aii->GetPointA(tx2, ty2, i%6), ++r2)
			{
				unsigned i = tx2 + ty2 * width;
				nodes[i].reachable = false;
				const noFlag *myFlag = 0;
				if (( myFlag = aii->GetSpecObj<noFlag>(tx2, ty2)))
				{
					if (myFlag->GetPlayer() == playerid)
					{
						nodes[i].reachable = true;
						toCheck.push(std::make_pair(tx2, ty2));
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

	for (unsigned short y = 0; y < height; ++y)
	{
		for (unsigned short x = 0; x < width; ++x)
		{
			unsigned i = x + y * width;

			// if reachable, we'll calc bq
			if (nodes[i].reachable)
			{
				nodes[i].owned = true;
				nodes[i].bq = aii->GetBuildingQuality(x, y);
			}
			else
			{
				nodes[i].owned = false;
				nodes[i].bq = BQ_NOTHING;
			}

			nodes[i].res = CalcResource(x, y);
			nodes[i].border = aii->IsBorder(x, y);
			nodes[i].farmed = false;
		}
	}
}

void AIPlayerJH::UpdateNodes()
{
	unsigned short width = aii->GetMapWidth();
	unsigned short height = aii->GetMapHeight();
	for (unsigned short y = 0; y < height; ++y)
	{
		for (unsigned short x = 0; x < width; ++x)
		{
			unsigned i = x + y * width;

			// if reachable, we'll calc bq
			if (nodes[i].reachable)
			{
				nodes[i].owned = true;
				nodes[i].bq = aii->GetBuildingQuality(x, y);
			}
			else
			{
				nodes[i].owned = false;
				nodes[i].bq = BQ_NOTHING;
			}
			if(nodes[i].res!=AIJH::BLOCKED)
				nodes[i].res = CalcResource(x, y);
			nodes[i].border = aii->IsBorder(x, y);
			nodes[i].farmed = false;
		}
	}
}

void AIPlayerJH::InitResourceMaps()
{
	unsigned short width = aii->GetMapWidth();
	unsigned short height = aii->GetMapHeight();

	resourceMaps.resize(AIJH::RES_TYPE_COUNT);
	for (unsigned res = 0; res < AIJH::RES_TYPE_COUNT; ++res)
	{
		resourceMaps[res].resize(width * height);
		for (unsigned short y=0; y<height; ++y)
		{
			for (unsigned short x=0; x<width; ++x)
			{
				unsigned i = y * width + x;
				//resourceMaps[res][i] = 0;
				if (nodes[i].res == (AIJH::Resource)res && (AIJH::Resource)res != AIJH::BORDERLAND)
				{
					ChangeResourceMap(x, y, AIJH::RES_RADIUS[res], resourceMaps[res], 1);
				}

				// Grenzgebiet"ressource"
				else if (nodes[i].border && (AIJH::Resource)res == AIJH::BORDERLAND)
				{
					ChangeResourceMap(x, y, AIJH::RES_RADIUS[AIJH::BORDERLAND], resourceMaps[AIJH::BORDERLAND], 1);
				}
				if(nodes[i].res==AIJH::MULTIPLE)
				{
					if(aii->GetSubsurfaceResource(x,y)==(AIJH::Resource)res||aii->GetSurfaceResource(x,y)==(AIJH::Resource)res)
						ChangeResourceMap(x, y, AIJH::RES_RADIUS[res], resourceMaps[res], 1);
				}
			}
		}
	}
}

void AIPlayerJH::RecalcResource(AIJH::Resource restype)
{
	unsigned short width = aii->GetMapWidth();
	unsigned short height = aii->GetMapHeight();
	unsigned res=restype;
	std::vector<int> &resmap=resourceMaps[res];
	for (unsigned y=0; y<resmap.size(); ++y)
	{
		resmap[y]=0;
	}
	for (unsigned short y=0; y<height; ++y)
	{
		for (unsigned short x=0; x<width; ++x)
		{
			unsigned i = y * width + x;
			//resourceMaps[res][i] = 0;
			if (nodes[i].res == (AIJH::Resource)res && (AIJH::Resource)res != AIJH::BORDERLAND && gwb->GetNode(x,y).t1!=TT_WATER && gwb->GetNode(x,y).t1!=TT_LAVA && gwb->GetNode(x,y).t1!=TT_SWAMPLAND&& gwb->GetNode(x,y).t1!=TT_SNOW )
			{
				ChangeResourceMap(x, y, AIJH::RES_RADIUS[res], resourceMaps[res], 1);
			}
			// Grenzgebiet"ressource"
			else if (aii->IsBorder(x, y) && (AIJH::Resource)res == AIJH::BORDERLAND)
			{	//only count border area that is actually passable terrain
				if(gwb->GetNode(x,y).t1!=TT_WATER && gwb->GetNode(x,y).t1!=TT_LAVA && gwb->GetNode(x,y).t1!=TT_SWAMPLAND&& gwb->GetNode(x,y).t1!=TT_SNOW)
					ChangeResourceMap(x, y, AIJH::RES_RADIUS[AIJH::BORDERLAND], resourceMaps[AIJH::BORDERLAND], 1);
			}
			if (nodes[i].res == AIJH::MULTIPLE && gwb->GetNode(x,y).t1!=TT_WATER && gwb->GetNode(x,y).t1!=TT_LAVA && gwb->GetNode(x,y).t1!=TT_SWAMPLAND )
			{
				if(aii->GetSubsurfaceResource(x,y)==(AIJH::Resource)res||aii->GetSurfaceResource(x,y)==(AIJH::Resource)res)
						ChangeResourceMap(x, y, AIJH::RES_RADIUS[res], resourceMaps[res], 1);
			}
			if(res==AIJH::WOOD&&aii->IsBuildingOnNode(x,y,BLD_WOODCUTTER)) //existing woodcutters reduce wood rating
				ChangeResourceMap(x, y, 7, resourceMaps[res], -10);
			if(res==AIJH::PLANTSPACE&&aii->IsBuildingOnNode(x,y,BLD_FARM)) //existing farm reduce plantspace rating
				ChangeResourceMap(x, y, 3, resourceMaps[res], -25);
			if(res==AIJH::PLANTSPACE&&aii->IsBuildingOnNode(x,y,BLD_FORESTER)) //existing forester reduce plantspace rating
				ChangeResourceMap(x, y, 6, resourceMaps[res], -25);
		}
	}
}

void AIPlayerJH::SetFarmedNodes(MapCoord x, MapCoord y)
{
	// Radius in dem Bausplatz für Felder blockiert wird
	const unsigned radius = 3;

	unsigned short width = aii->GetMapWidth();
	
	nodes[x + y * width].farmed = true;

	for(MapCoord tx=aii->GetXA(x,y,0), r=1;r<=radius;tx=aii->GetXA(tx,y,0),++r)
	{
		MapCoord tx2 = tx, ty2 = y;
		for(unsigned i = 2;i<8;++i)
		{
			for(MapCoord r2=0;r2<r;aii->GetPointA(tx2,ty2,i%6),++r2)
			{
				unsigned i = tx2 + ty2 * width;
				nodes[i].farmed = true;;
			}
		}
	}
}

void AIPlayerJH::ChangeResourceMap(MapCoord x, MapCoord y, unsigned radius, std::vector<int> &resMap, int value)
{
	unsigned short width = aii->GetMapWidth();

	resMap[x + y * width] += value * radius;

	for(MapCoord tx=aii->GetXA(x,y,0), r=1;r<=radius;tx=aii->GetXA(tx,y,0),++r)
	{
		MapCoord tx2 = tx, ty2 = y;
		for(unsigned i = 2;i<8;++i)
		{
			for(MapCoord r2=0;r2<r;aii->GetPointA(tx2,ty2,i%6),++r2)
			{
				unsigned i = tx2 + ty2 * width;
				resMap[i] += value * (radius-r);
			}
		}
	}


}

bool AIPlayerJH::FindGoodPosition(MapCoord &x, MapCoord &y, AIJH::Resource res, int threshold, BuildingQuality size, int radius, bool inTerritory)
{
	unsigned short width = aii->GetMapWidth();
	unsigned short height = aii->GetMapHeight();

	if (x >= width || y >= height)
	{
		x = aii->GetHeadquarter()->GetX();
		y = aii->GetHeadquarter()->GetY();
	}

	// TODO was besseres wär schön ;)
	if (radius == -1)
		radius = 30;

	for(MapCoord tx=aii->GetXA(x,y,0), r=1;r<=radius;tx=aii->GetXA(tx,y,0),++r)
	{
		MapCoord tx2 = tx, ty2 = y;
		for(unsigned i = 2;i<8;++i)
		{
			for(MapCoord r2=0;r2<r;aii->GetPointA(tx2,ty2,i%6),++r2)
			{
				unsigned i = tx2 + ty2 * width;
				if (resourceMaps[res][i] >= threshold)
				{
					if ((inTerritory && !nodes[i].owned) || nodes[i].farmed)
						continue;
					if ( (nodes[i].bq >= size && nodes[i].bq < BQ_MINE) // normales Gebäude
						|| (nodes[i].bq == size))	// auch Bergwerke
					{
						x = tx2;
						y = ty2;
						return true;
					}
				}
			}
		}
	}
	return false;
}

PositionSearch *AIPlayerJH::CreatePositionSearch(MapCoord &x, MapCoord &y, AIJH::Resource res, BuildingQuality size, int minimum, BuildingType bld, bool best)
{
	// set some basic parameters
	PositionSearch *p = new PositionSearch(x, y, res, minimum, size, BLD_WOODCUTTER, best);
	p->nodesPerStep = 25; // TODO make it dependent on something...
	p->resultValue = 0;

	// allocate memory for the nodes
	unsigned numNodes = aii->GetMapWidth() * aii->GetMapHeight();
	p->tested = new std::vector<bool>(numNodes, false);
	p->toTest = new std::queue<unsigned>;


	// if no useful startpos is given, use headquarter
	if (x >= aii->GetMapWidth() || y >= aii->GetMapHeight())
	{
		x = aii->GetHeadquarter()->GetX();
		y = aii->GetHeadquarter()->GetY();
	}

	// insert start position as first node to test
	p->toTest->push(x + y * aii->GetMapWidth());
	(*p->tested)[x + y * aii->GetMapWidth()] = true;

	return p;
}

PositionSearchState AIPlayerJH::FindGoodPosition(PositionSearch *search, bool best)
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
		AIJH::Node *node = &nodes[nodeIndex];
		unsigned short width = aii->GetMapWidth();
		MapCoord x = nodeIndex % width;
		MapCoord y = nodeIndex / width;
		
		// and test it... TODO exception at res::borderland?
		if (resourceMaps[search->res][nodeIndex] > search->resultValue // value better
			&& node->owned && node->reachable && !node->farmed // available node
			&& ((node->bq >= search->size && node->bq < BQ_MINE) || (node->bq == search->size)) // matching size
			)
		{
			// store location & value
			search->resultValue = resourceMaps[search->res][nodeIndex];
			search->resultX = x;
			search->resultY = y;
		}

		// now insert neighbouring nodes...
		for (unsigned char dir = 0; dir < 6; ++dir)
		{
			MapCoord nx = aii->GetXA(x, y, dir);
			MapCoord ny = aii->GetYA(x, y, dir);
			unsigned ni = nx + ny * width;

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


bool AIPlayerJH::FindBestPosition(MapCoord &x, MapCoord &y, AIJH::Resource res, BuildingQuality size, int minimum, int radius, bool inTerritory)
{
	unsigned short width = aii->GetMapWidth();
	unsigned short height = aii->GetMapHeight();

	if (x >= width || y >= height)
	{
		x = aii->GetHeadquarter()->GetX();
		y = aii->GetHeadquarter()->GetY();
	}

	// TODO was besseres wär schön ;)
	if (radius == -1)
		radius = 30;

	int best_x = 0, best_y = 0, best_value;
	best_value = -1;

	for(MapCoord tx=aii->GetXA(x,y,0), r=1;r<=radius;tx=aii->GetXA(tx,y,0),++r)
	{
		MapCoord tx2 = tx, ty2 = y;
		for(unsigned i = 2;i<8;++i)
		{
			for(MapCoord r2=0;r2<r;aii->GetPointA(tx2,ty2,i%6),++r2)
			{
				unsigned i = tx2 + ty2 * width;
				if (resourceMaps[res][i] > best_value)
				{
					if (!nodes[i].reachable || (inTerritory && !nodes[i].owned) || nodes[i].farmed)
						continue;
					if ( (nodes[i].bq >= size && nodes[i].bq < BQ_MINE) // normales Gebäude
						|| (nodes[i].bq == size))	// auch Bergwerke
					{
						best_x = tx2;
						best_y = ty2;
						best_value = resourceMaps[res][i];
					}
				}
			}
		}
	}

	if (best_value >= minimum)
	{
		x = best_x;
		y = best_y;
		return true;
	}
	return false;
}

void AIPlayerJH::UpdateNodesAround(MapCoord x, MapCoord y, unsigned radius)
{
	unsigned width = aii->GetMapWidth();

	UpdateReachableNodes(x, y, radius);

	for(MapCoord tx=aii->GetXA(x,y,0), r=1;r<=radius;tx=aii->GetXA(tx,y,0),++r)
	{
		MapCoord tx2 = tx, ty2 = y;
		for(unsigned i = 2;i<8;++i)
		{
			for(MapCoord r2=0;r2<r;aii->GetPointA(tx2,ty2,i%6),++r2)
			{
				unsigned i = tx2 + ty2 * width;

				nodes[i].owned = aii->IsOwnTerritory(tx2, ty2);

				if (nodes[i].owned)
				{
					nodes[i].bq = aii->GetBuildingQuality(tx2, ty2);
				}
				else
				{
					nodes[i].owned = false;
					nodes[i].bq = BQ_NOTHING;
				}

				/*AIJH::Resource res = CalcResource(tx2, ty2);
				if (res != nodes[i].res)
				{
					// Altes entfernen:
					if (nodes[i].res != AIJH::NOTHING)
						ChangeResourceMap(tx2, ty2, AIJH::RES_RADIUS[nodes[i].res], resourceMaps[nodes[i].res], -1);
					// Neues Hinzufügen:
					if (res != AIJH::NOTHING)
						ChangeResourceMap(tx2, ty2, AIJH::RES_RADIUS[res], resourceMaps[res], 1);

					nodes[i].res = res;
				}*/

				bool borderland = aii->IsBorder(tx2, ty2);
				if (borderland != nodes[i].border)
				{
					if (borderland)
					{
						//std::cout << tx2 << " / " << ty2 << " Border dazugekommen" << std::endl;
						ChangeResourceMap(tx2, ty2, AIJH::RES_RADIUS[AIJH::BORDERLAND], resourceMaps[AIJH::BORDERLAND], 1);
					}
					else
					{
						//std::cout << tx2 << " / " << ty2 << " Border verschwunden" << std::endl;
						ChangeResourceMap(tx2, ty2, AIJH::RES_RADIUS[AIJH::BORDERLAND], resourceMaps[AIJH::BORDERLAND], -1);
					}
				}

			}
		}
	}
}
void AIPlayerJH::UpdateNodesAroundNoBorder(MapCoord x, MapCoord y, unsigned radius)
{
	unsigned width = aii->GetMapWidth();

	UpdateReachableNodes(x, y, radius);

	for(MapCoord tx=aii->GetXA(x,y,0), r=1;r<=radius;tx=aii->GetXA(tx,y,0),++r)
	{
		MapCoord tx2 = tx, ty2 = y;
		for(unsigned i = 2;i<8;++i)
		{
			for(MapCoord r2=0;r2<r;aii->GetPointA(tx2,ty2,i%6),++r2)
			{
				unsigned i = tx2 + ty2 * width;

				nodes[i].owned = aii->IsOwnTerritory(tx2, ty2);

				if (nodes[i].owned)
				{
					nodes[i].bq = aii->GetBuildingQuality(tx2, ty2);
				}
				else
				{
					nodes[i].owned = false;
					nodes[i].bq = BQ_NOTHING;
				}
				/*
				AIJH::Resource res = CalcResource(tx2, ty2);
				if (res != nodes[i].res)
				{
					// Altes entfernen:
					if (nodes[i].res != AIJH::NOTHING)
						ChangeResourceMap(tx2, ty2, AIJH::RES_RADIUS[nodes[i].res], resourceMaps[nodes[i].res], -1);
					// Neues Hinzufügen:
					if (res != AIJH::NOTHING)
						ChangeResourceMap(tx2, ty2, AIJH::RES_RADIUS[res], resourceMaps[res], 1);

					nodes[i].res = res;
				}*/			

			}
		}
	}
}

void AIPlayerJH::ExecuteAIJob()
{
	// Check whether current job is finished...
	if (currentJob)
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
	}

	// if no current job available, take next one! events first, then constructions
	if (!currentJob)
	{
		if (eventManager.EventAvailable())
		{
			currentJob = new AIJH::EventJob(this, eventManager.GetEvent());
		}
		else if (construction.BuildJobAvailable())
		{
			currentJob = construction.GetBuildJob();
		}
	}

	// Something to do? Do it!
	if (currentJob)
		currentJob->ExecuteJob();
}

void AIPlayerJH::RecalcBQAround(const MapCoord x, const MapCoord y)
{
	unsigned width = aii->GetMapWidth();

	// Drumherum BQ neu berechnen, da diese sich ja jetzt hätten ändern können
	unsigned index = x + y * width;

	nodes[index].bq = aii->GetBuildingQuality(x,y);
	for(unsigned char i = 0;i<6;++i)
	{
		index = aii->GetXA(x,y,i) + aii->GetYA(x,y,i) * width;
		nodes[index].bq = aii->GetBuildingQuality(aii->GetXA(x,y,i), aii->GetYA(x,y,i));
	}
	for(unsigned i = 0;i<12;++i)
	{
		index = aii->GetXA2(x,y,i) + aii->GetYA2(x,y,i) * width;
		nodes[index].bq = aii->GetBuildingQuality(aii->GetXA2(x,y,i),aii->GetYA2(x,y,i));
	}
}

void AIPlayerJH::CheckNewMilitaryBuildings()
{
	for (std::list<Coords>::iterator it = milBuildingSites.begin(); it != milBuildingSites.end(); it++)
	{
		const nobMilitary *mil;
		if ((mil = aii->GetSpecObj<nobMilitary>((*it).x, (*it).y)))
		{
			if (!mil->IsNewBuilt())
			{
				HandleNewMilitaryBuilingOccupied(*it);
				milBuildings.push_back(Coords(mil->GetX(), mil->GetY()));
				milBuildingSites.erase(it);
				break;
			}
		}
	}
}

bool AIPlayerJH::SimpleFindPosition(MapCoord &x, MapCoord &y, BuildingQuality size, int radius)
{
	unsigned short width = aii->GetMapWidth();
	unsigned short height = aii->GetMapHeight();
	//if(size==BQ_HARBOR)
	//	Chat(_("looking for harbor"));

	if (x >= width || y >= height)
	{
		x = aii->GetHeadquarter()->GetX();
		y = aii->GetHeadquarter()->GetY();
	}

	// TODO was besseres wär schön ;)
	if (radius == -1)
		radius = 30;

	for(MapCoord tx=aii->GetXA(x,y,0), r=1;r<=radius;tx=aii->GetXA(tx,y,0),++r)
	{
		MapCoord tx2 = tx, ty2 = y;
		for(unsigned i = 2;i<8;++i)
		{
			for(MapCoord r2=0;r2<r;aii->GetPointA(tx2,ty2,i%6),++r2)
			{
				unsigned i = tx2 + ty2 * width;

				if (!nodes[i].reachable || !nodes[i].owned || nodes[i].farmed)
					continue;
				if ( (nodes[i].bq >= size && nodes[i].bq < BQ_MINE) // normales Gebäude
					|| (nodes[i].bq == size))	// auch Bergwerke
				{
					x = tx2;
					y = ty2;
					return true;
				}
			}
		}
	}

	return false;
}

double AIPlayerJH::GetDensity(MapCoord x, MapCoord y, AIJH::Resource res, int radius)
{
		unsigned short width = aii->GetMapWidth();
		unsigned short height = aii->GetMapHeight();
	

	// TODO: check warum das so ist, und ob das sinn macht! ist so weil der punkt dann außerhalb der karte liegen würde ... könnte trotzdem crashen wenn wir kein hq mehr haben ... mehr checks!
	if (x >= width || y >= height)
	{
		x = aii->GetHeadquarter()->GetX();
		y = aii->GetHeadquarter()->GetY();
	}



	unsigned good = 0;
	unsigned all = 0;

	for(MapCoord tx=aii->GetXA(x,y,0), r=1;r<=radius;tx=aii->GetXA(tx,y,0),++r)
	{
		MapCoord tx2 = tx, ty2 = y;
		for(unsigned i = 2;i<8;++i)
		{
			for(MapCoord r2=0;r2<r;aii->GetPointA(tx2,ty2,i%6),++r2)
			{
				unsigned i = tx2 + ty2 * width;

				if (nodes[i].res == res)
					good++;

				all++;
			}
		}
	}

	return (all != 0) ? good/(double)all : 0.0;
}

void AIPlayerJH::HandleNewMilitaryBuilingOccupied(const Coords& coords)
{	
	MapCoord x = coords.x;
	MapCoord y = coords.y;
	//kill bad flags we find
	std::vector<const noFlag*> flags;
	construction.FindFlags(flags, x, y, 25);	
	// Jede Flagge im umkreis testen auf kaputte wege
	for(unsigned i=0; i<flags.size(); ++i)
	{
		//excluding direction 255 means no excluded direction ... sometimes I used 7 for the same purpose so if you ever change the direction to count to include 7 this will fail in some spots :)
		RemoveUnusedRoad(flags[i],255,true);
	}	
	UpdateNodesAround(x, y, 15); // todo: fix radius
	construction.RefreshBuildingCount();
	//is the captured building in our list(should be if be constructed it)
	bool alreadyinlist=false;
	for (std::list<Coords>::iterator it = milBuildings.begin(); it != milBuildings.end(); it++)
	{		
		if (((*it).x==x&&(*it).y==y))	
		{
			//already in our list break the search
			alreadyinlist=true;
			break;
		}
	}
	//if it wasnt in the list add it to the list
	if(!alreadyinlist)
		milBuildings.push_back(coords);

	const nobMilitary *mil = aii->GetSpecObj<nobMilitary>(x, y);
	if (mil)
	{
		if ((mil->GetBuildingType() == BLD_BARRACKS || mil->GetBuildingType() == BLD_GUARDHOUSE) && mil->GetFrontierDistance() == 0 && !mil->IsGoldDisabled())
		{
			gcs.push_back(new gc::StopGold(x, y));
		}

		// if near border and gold disabled (by addon): enable it
		if (mil->GetFrontierDistance() && mil->IsGoldDisabled())
		{
			gcs.push_back(new gc::StopGold(x, y));
		}
		//should be done by now by the removeunusedroads code
		/*if (!construction.IsConnectedToRoadSystem(mil->GetFlag()))
		{
			construction.AddConnectFlagJob(mil->GetFlag());
		}*/
	}

	AddBuildJob(BLD_HARBORBUILDING, x, y);
	if (SoldierAvailable())
	{
		AddBuildJob(construction.ChooseMilitaryBuilding(x, y), x, y);
		AddBuildJob(construction.ChooseMilitaryBuilding(x, y), x, y);
		AddBuildJob(construction.ChooseMilitaryBuilding(x, y), x, y);
	}

	// try to build one the following buildings around the new military building
	
	BuildingType bldToTest[] = {
		BLD_STOREHOUSE,
		BLD_WOODCUTTER,
		BLD_QUARRY,
		BLD_GOLDMINE,
		BLD_COALMINE,
		BLD_IRONMINE,
		BLD_GRANITEMINE,
		BLD_FISHERY,
		BLD_FARM,
		BLD_HUNTER	
	};
	std::list<AIJH::Coords> storeHousePoses = construction.GetStoreHousePositions();
	//bool storeclose=false;
	unsigned numBldToTest = 0;
	//remove the storehouse from the building test list if we are close to another storehouse already
	for (std::list<AIJH::Coords>::iterator it = storeHousePoses.begin(); it != storeHousePoses.end(); it++)
	{
		if (gwb->CalcDistance((*it).x, (*it).y, x, y) < 20)
		{
			numBldToTest = 1;
			break;
		}
		
	}
	

	for (unsigned int i = numBldToTest; i < 10; ++i)
	{
		if (construction.Wanted(bldToTest[i]))
		{
			AddBuildJob(bldToTest[i], x, y);
		}
	}
}

void AIPlayerJH::HandleBuilingDestroyed(const Coords& coords, BuildingType bld)
{
	MapCoord x = coords.x;
	MapCoord y = coords.y;
	UpdateNodesAroundNoBorder(x, y, 11); // todo: fix radius
	//not sure but maybe we have to update resources if we lost a mine
}

void AIPlayerJH::HandleMilitaryBuilingLost(const Coords& coords)
{
	MapCoord x = coords.x;
	MapCoord y = coords.y;
	UpdateNodesAroundNoBorder(x, y, 15); // todo: fix radius
	//remove from military building list if possible
	for (std::list<Coords>::iterator it = milBuildings.begin(); it != milBuildings.end(); it++)
	{
		const nobMilitary *mil;
		if (!(mil = aii->GetSpecObj<nobMilitary>((*it).x, (*it).y))||((*it).x==x&&(*it).y==y))	
		{
			//this means there is no military building although there should be - probably destroyed or that it is the one we just lost
			it=milBuildings.erase(it);
		}
		if(it==milBuildings.end())
			break;
	}
	if(construction.GetStoreHousePositions().size()<2) //check if we have a storehouse left - if we dont have one trying to find a path to one will crash
	{
		std::list<AIJH::Coords>::iterator it=construction.GetStoreHousePositions().begin()++;
		if(!aii->IsObjectTypeOnNode((*it).x,(*it).y,NOP_BUILDING)&&!aii->IsObjectTypeOnNode((*it).x,(*it).y,NOP_BUILDINGSITE))
			return;
	}
	//find all flags around the lost building and try to reconnect them if necessary 
	std::vector<const noFlag*> flags;
	construction.FindFlags(flags, x, y, 25);	
	// Jede Flagge testen...
	for(unsigned i=0; i<flags.size(); ++i)
	{
		//excluding direction 7 means no excluded direction because there are only 6 valid directions
		RemoveUnusedRoad(flags[i],255,true);
	}	

}

void AIPlayerJH::HandleBuildingFinished(const Coords& coords, BuildingType bld)
{
	switch(bld)
	{
	case BLD_HARBORBUILDING:
		UpdateNodesAround(coords.x, coords.y, 8); // todo: fix radius

		AddBuildJob(BLD_BARRACKS, coords.x, coords.y);
		AddBuildJob(BLD_WOODCUTTER, coords.x, coords.y);
		AddBuildJob(BLD_SAWMILL, coords.x, coords.y);
		AddBuildJob(BLD_QUARRY, coords.x, coords.y);

		// stop beer, swords and shields -> hq only (todo: hq destroyed -> use another storehouse)
		// can't do that on harbors... maybe production is on an island which is not the hq's
		//gcs.push_back(new gc::ChangeInventorySetting(coords.x, coords.y, 0, 2, 0));
		//gcs.push_back(new gc::ChangeInventorySetting(coords.x, coords.y, 0, 2, 16));
		//gcs.push_back(new gc::ChangeInventorySetting(coords.x, coords.y, 0, 2, 21));

		aii->StartExpedition(coords.x, coords.y);
		break;

	case BLD_SHIPYARD:
		aii->ToggleShipyardMode(coords.x, coords.y);
		break;

	case BLD_STOREHOUSE:
		// stop beer, swords and shields -> hq only (todo: hq destroyed -> use another storehouse)
		//aii->ChangeInventorySetting( TODO
		gcs.push_back(new gc::ChangeInventorySetting(coords.x, coords.y, 0, 2, 0));
		gcs.push_back(new gc::ChangeInventorySetting(coords.x, coords.y, 0, 2, 16));
		gcs.push_back(new gc::ChangeInventorySetting(coords.x, coords.y, 0, 2, 21));
		
	default:
		break;
	}

}

void AIPlayerJH::HandleExpedition(const Coords& coords)
{
	list<noBase*> objs;
	aii->GetDynamicObjects(coords.x, coords.y, objs);
	const noShip *ship = NULL;

	for(list<noBase*>::iterator it = objs.begin();it.valid();++it)
	{
		if((*it)->GetGOT() == GOT_SHIP)
		{
			if(static_cast<noShip*>(*it)->GetPlayer() == playerid)
			{
				if (static_cast<noShip*>(*it)->IsOnExpedition())
					ship = static_cast<noShip*>(*it);
			}
		}
	}


	//const noShip *ship = gwb->GetSpecObj<noShip>(coords.x, coords.y);
	if (ship)
	{
		if (ship->IsAbleToFoundColony())
			aii->FoundColony(ship);
		else
		{
			unsigned char start = rand() % 6;

			for(unsigned char i = start; i < start + 6; ++i)
			{
				if (aii->IsExplorationDirectionPossible(coords.x, coords.y, ship->GetCurrentHarbor(), i%6))
				{
					aii->TravelToNextSpot(i%6, ship);
					return;
				}
			}

			// no direction possible, sad, stop it
			aii->CancelExpedition(ship);
		}
	}

}

void AIPlayerJH::HandleTreeChopped(const Coords& coords)
{
	MapCoord x = coords.x;
	MapCoord y = coords.y;

	//std::cout << "Tree chopped." << std::endl;

	nodes[y * aii->GetMapWidth() + x].reachable = true;

	UpdateNodesAround(x, y, 3);

	int random = rand();

	

	if (random % 2 == 0)
		AddBuildJob(construction.ChooseMilitaryBuilding(x, y), x, y);
	else //if (random % 12 == 0)
		AddBuildJob(BLD_WOODCUTTER, x, y);

}

void AIPlayerJH::HandleNoMoreResourcesReachable(const Coords& coords, BuildingType bld)
{
	MapCoord x = coords.x;
	MapCoord y = coords.y;
	UpdateNodesAround(x, y, 11); // todo: fix radius

	// Destroy old building (once)
	
	if (aii->IsObjectTypeOnNode(x, y, NOP_BUILDING))
		gcs.push_back(new gc::DestroyBuilding(x, y));
	else
		return;
	
	RemoveUnusedRoad(aii->GetSpecObj<noFlag>(aii->GetXA(x,y,4),aii->GetYA(x,y,4)), 1,true);

	// try to expand, maybe res blocked a passage
	AddBuildJob(construction.ChooseMilitaryBuilding(x, y), x, y);
	AddBuildJob(construction.ChooseMilitaryBuilding(x, y), x, y);

	// and try to rebuild the same building
	if(bld!=BLD_HUNTER)
		AddBuildJob(bld);

	// farm is always good!
	AddBuildJob(BLD_FARM, x, y);
}

void AIPlayerJH::HandleShipBuilt(const Coords& coords)
{
	// Stop building ships if reached a maximum (TODO: make variable)
	if (aii->GetShipCount() > 5)
	{
		for (std::list<nobUsual *>::const_iterator it = aii->GetBuildings(BLD_SHIPYARD).begin(); it != aii->GetBuildings(BLD_SHIPYARD).end(); it++)
		{
			aii->StopProduction((*it)->GetX(), (*it)->GetY());
		}
	}
}

void AIPlayerJH::HandleBorderChanged(const Coords& coords)
{
	MapCoord x = coords.x;
	MapCoord y = coords.y;
	UpdateNodesAround(x, y, 11); // todo: fix radius

	const nobMilitary *mil = aii->GetSpecObj<nobMilitary>(x, y);
	if (mil)
	{
		if (mil->GetFrontierDistance() != 0 && mil->IsGoldDisabled())
		{
			aii->ToggleCoins(x, y);
		}
		if (mil->GetBuildingType() == BLD_BARRACKS || mil->GetBuildingType() == BLD_GUARDHOUSE)
		{
			AddBuildJob(construction.ChooseMilitaryBuilding(x, y), x, y);
		}
	}	
}


void AIPlayerJH::Chat(std::string message)
{
	GameMessage_Server_Chat chat = GameMessage_Server_Chat(playerid,CD_ALL,message);
	GameServer::inst().AIChat(chat);
}

void AIPlayerJH::TryToAttack() 
{
	std::vector<std::pair<const nobBaseMilitary *, unsigned> > potentialTargets;

	for (std::list<Coords>::iterator it = milBuildings.begin(); it != milBuildings.end(); it++)
	{
		const nobMilitary *mil;
		if (!(mil = aii->GetSpecObj<nobMilitary>((*it).x, (*it).y)))	
		{
			//this means there is no military building although there should be - probably destroyed or just failed to save the right spot - lets try to remove it from the list
			it=milBuildings.erase(it);
			if(it==milBuildings.end())
				break;
			continue;
		}

		if (mil->GetFrontierDistance() == 0)  //inland building? -> deactivate gold & skip it
		{
			if(!mil->IsGoldDisabled())
			{
				gcs.push_back(new gc::StopGold(mil->GetX(), mil->GetY()));
			}
			continue;
		}
		else
		{
			if(mil->IsGoldDisabled())		//combat building? -> activate gold
			{
				gcs.push_back(new gc::StopGold(mil->GetX(), mil->GetY()));
			}
		}
		std::list<nobBaseMilitary *> buildings;
		aii->GetMilitaryBuildings((*it).x, (*it).y, 2, buildings);
		for(std::list<nobBaseMilitary*>::iterator it2 = buildings.begin(); it2 != buildings.end(); ++it2)
		{
			MapCoord dest_x = (*it2)->GetX();
			MapCoord dest_y = (*it2)->GetY();
			if (gwb->CalcDistance((*it).x, (*it).y, dest_x, dest_y) < BASE_ATTACKING_DISTANCE 
				&& aii->IsPlayerAttackable((*it2)->GetPlayer()) && aii->IsVisible(dest_x, dest_y))
			{
				potentialTargets.push_back(std::make_pair((*it2), 0));

				std::list<nobBaseMilitary *> myBuildings;
				aii->GetMilitaryBuildings(dest_x, dest_y, 2, myBuildings);
				for(std::list<nobBaseMilitary*>::iterator it3 = myBuildings.begin(); it3!=myBuildings.end(); ++it3)
				{
					if ((*it3)->GetPlayer() == playerid)
					{
						const nobMilitary *myMil;
						myMil = dynamic_cast<const nobMilitary *>(*it3);
						if (!myMil)
							continue;

						potentialTargets[potentialTargets.size() - 1].second += myMil->GetSoldiersForAttack(dest_x, dest_y, playerid);
					}
				}
			}
		}
	}
	
	unsigned max = 0;
	unsigned maxIndex = 0;

	for (unsigned i=0; i<potentialTargets.size(); ++i)
	{
		if (potentialTargets[i].second > max)
		{
			max = potentialTargets[i].second;
			maxIndex = i;
		}
	}

	if (max > 0)
	{
		const nobMilitary *enemyTarget;
		enemyTarget = dynamic_cast<const nobMilitary *>(potentialTargets[maxIndex].first);
		if (enemyTarget)
		{
			if (potentialTargets[maxIndex].second > enemyTarget->GetTroopsCount() && enemyTarget->GetTroopsCount() > 0)
				gcs.push_back(new gc::Attack(potentialTargets[maxIndex].first->GetX(), potentialTargets[maxIndex].first->GetY(), 
					potentialTargets[maxIndex].second, true));
		}
		else
		{
			// is wohl HQ?
			gcs.push_back(new gc::Attack(potentialTargets[maxIndex].first->GetX(), potentialTargets[maxIndex].first->GetY(), 
				potentialTargets[maxIndex].second, true));
		}
	}
}


void AIPlayerJH::RecalcGround(MapCoord x_building, MapCoord y_building, std::vector<unsigned char> &route_road)
{
	MapCoord x = x_building;
	MapCoord y = y_building;

	// building itself
	RecalcBQAround(x, y);
	if (GetAINode(x, y).res == AIJH::PLANTSPACE)
	{
		ChangeResourceMap(x, y, AIJH::RES_RADIUS[AIJH::PLANTSPACE], resourceMaps[AIJH::PLANTSPACE], -1);
		GetAINode(x, y).res = AIJH::NOTHING;
	}

	// flag of building
	aii->GetPointA(x, y, 4);
	RecalcBQAround(x, y);
	if (GetAINode(x, y).res == AIJH::PLANTSPACE)
	{
		ChangeResourceMap(x, y, AIJH::RES_RADIUS[AIJH::PLANTSPACE], resourceMaps[AIJH::PLANTSPACE], -1);
		GetAINode(x, y).res = AIJH::NOTHING;
	}

	// along the road
	for (unsigned i=0; i<route_road.size(); ++i)
	{
		aii->GetPointA(x, y, route_road[i]);
		RecalcBQAround(x, y);
		// Auch Plantspace entsprechend anpassen:
		if (GetAINode(x, y).res == AIJH::PLANTSPACE)
		{
			ChangeResourceMap(x, y, AIJH::RES_RADIUS[AIJH::PLANTSPACE], resourceMaps[AIJH::PLANTSPACE], -1);
			GetAINode(x, y).res = AIJH::NOTHING;
		}
	}
}

void AIPlayerJH::SaveResourceMapsToFile()
{
#ifdef DEBUG_AI
	for(unsigned i=0; i<AIJH::RES_TYPE_COUNT; ++i)
	{
		std::stringstream ss;
		ss << "resmap-" << i << ".log";
		FILE * file = fopen(ss.str().c_str(),"w");
		for (unsigned y=0; y<aii->GetMapHeight(); ++y)
		{
			if (y % 2 == 1)
				fprintf(file,"  ");
			for (unsigned x=0; x<aii->GetMapWidth(); ++x)
			{
				fprintf(file,"%i   ",resourceMaps[i][x + y * aii->GetMapWidth()]);		
			}
			fprintf(file,"\n");
		}
		fclose(file);
	}
#endif
}

int AIPlayerJH::GetResMapValue(MapCoord x, MapCoord y, AIJH::Resource res)
{
	return resourceMaps[res][x + y * aii->GetMapWidth()];
}

void AIPlayerJH::SendAIEvent(AIEvent::Base *ev) 
{
	eventManager.AddAIEvent(ev);
}

bool AIPlayerJH::IsFlagPartofCircle(const noFlag *startFlag,unsigned maxlen,const noFlag *curFlag,unsigned char excludeDir,bool init,std::vector<int> oldflagsx,std::vector<int> oldflagsy)
{
	if(!init&&startFlag==curFlag)
		return true;
	if(maxlen<1)
		return false;
	bool partofcircle=false;
	unsigned testdir=0;
	while(testdir<6&&!partofcircle)
	{
		if (testdir == excludeDir)
		{
			testdir++;
			continue;
		}
		if (aii->GetPointRoad(curFlag->GetX(), curFlag->GetY(), testdir)) //road towards testdir?
		{ //follow road to next flag and ask again
			unsigned char foundDir=testdir;
			unsigned char prevDir=(testdir+3)%6;
			MapCoord x = aii->GetXA(curFlag->GetX(), curFlag->GetY(), foundDir);
			MapCoord y = aii->GetYA(curFlag->GetX(), curFlag->GetY(), foundDir);
			unsigned char finds=0;
			while(true)
			{
				const noFlag *flag;
				finds=0;
				// flag found?
				if ((flag = aii->GetSpecObj<noFlag>(x, y)))
				{
					bool alreadyinlist=false;
					for(unsigned i=0;i<oldflagsx.size();i++)
					{
						if (flag->GetX()==oldflagsx[i]&&flag->GetY()==oldflagsy[i])
						{
							alreadyinlist=true;
							break;
						}

					}
					if(!alreadyinlist)
					{
						oldflagsx.push_back(flag->GetX());
						oldflagsy.push_back(flag->GetY());						
						partofcircle=IsFlagPartofCircle(startFlag,maxlen-1,flag,prevDir,false,oldflagsx,oldflagsy);
					}
					break;
				}
				else
				{			
					// continue to follow the road
					for (unsigned char nextDir = 0; nextDir < 6; ++nextDir)
					{
						if (aii->GetPointRoad(x, y, nextDir) && nextDir != prevDir)
						{
							x = aii->GetXA(x, y, nextDir);
							y = aii->GetYA(x, y, nextDir);
							prevDir = (nextDir + 3) % 6;
							finds++;
							break;
						}
					}
				}
				if(finds!=1)
				{
					// either found a split in the road (>1) or the road stopped (0) - both things shouldnt happen but just in case they do: break the loop.
					break;
				}
			}
		}
		testdir++;
	}
	return partofcircle;
}


void AIPlayerJH::RemoveUnusedRoad(const noFlag *startFlag, unsigned char excludeDir,bool firstflag)
{
	unsigned char foundDir = 0xFF;
	unsigned char foundDir2= 0xFF;
	unsigned char finds = 0;

	// Count roads from this flag...
	for (unsigned char dir=0; dir < 6; ++dir)
	{
		if (dir == excludeDir)
			continue;
		if (aii->GetPointRoad(startFlag->GetX(), startFlag->GetY(), dir))
		{
			finds++;
			if(finds==1)
				foundDir = dir;
			else
				if(finds==2)
					foundDir2=dir;
			if(dir==1&&(aii->IsObjectTypeOnNode(aii->GetXA(startFlag->GetX(),startFlag->GetY(),1),aii->GetYA(startFlag->GetX(),startFlag->GetY(),1),NOP_BUILDING)||aii->IsObjectTypeOnNode(aii->GetXA(startFlag->GetX(),startFlag->GetY(),1),aii->GetYA(startFlag->GetX(),startFlag->GetY(),1),NOP_BUILDINGSITE)))
			{
				//the flag belongs to a building - update the pathing map around us and try to reconnect it (if we cant reconnect it -> burn it(burning takes place at the pathfinding job))
				finds+=3;
				UpdateNodesAroundNoBorder(startFlag->GetX(),startFlag->GetY(),20);
				construction.AddConnectFlagJob(startFlag);

			}
		}
	}

	// if we found more than 1 road (or a building) the flag is still in use.	
	if (finds>2)
	{	
		return;
	}
	else
	{
		if(finds==2)
		{
			std::vector<int> flagcheck;
			if(!IsFlagPartofCircle(startFlag,10,startFlag,7,true,flagcheck,flagcheck))
				return;
			if(!firstflag)
				return;
		}
	}

	// kill the flag
	aii->DestroyFlag(startFlag);

	// nothing found?
	if (foundDir > 6)
	{
		return;
	}

	MapCoord x = aii->GetXA(startFlag->GetX(), startFlag->GetY(), foundDir);
	MapCoord y = aii->GetYA(startFlag->GetX(), startFlag->GetY(), foundDir);

	unsigned char prevDir = (foundDir + 3) % 6;
	// follow the (only) road to next flag and test it also
	while(true)
	{
		const noFlag *flag;
		finds=0;
		// flag found?
		if ((flag = aii->GetSpecObj<noFlag>(x, y)))
		{
			RemoveUnusedRoad(flag, prevDir,false);
			break;
		}
		else
		{			
			// continue to follow the road
			for (unsigned char nextDir = 0; nextDir < 6; ++nextDir)
			{
				if (aii->GetPointRoad(x, y, nextDir) && nextDir != prevDir)
				{
					x = aii->GetXA(x, y, nextDir);
					y = aii->GetYA(x, y, nextDir);
					prevDir = (nextDir + 3) % 6;
					finds++;
					break;
				}
			}
		}
		if(finds!=1)
		{
			// either found a split in the road (>1) or the road stopped (0) - both things shouldnt happen but just in case they do: break the loop.
			break;
		}
	}
	if(foundDir2!=0xFF)//remove road in the other path as well in case of a 2 way road remove
	{
		x = aii->GetXA(startFlag->GetX(), startFlag->GetY(), foundDir2);
		y = aii->GetYA(startFlag->GetX(), startFlag->GetY(), foundDir2);
		prevDir = (foundDir2 + 3) % 6;
		while(true)
	{
		const noFlag *flag;
		finds=0;
		// flag found?
		if ((flag = aii->GetSpecObj<noFlag>(x, y)))
		{
			RemoveUnusedRoad(flag, prevDir,false);
			break;
		}
		else
		{			
			// continue to follow the road
			for (unsigned char nextDir = 0; nextDir < 6; ++nextDir)
			{
				if (aii->GetPointRoad(x, y, nextDir) && nextDir != prevDir)
				{
					x = aii->GetXA(x, y, nextDir);
					y = aii->GetYA(x, y, nextDir);
					prevDir = (nextDir + 3) % 6;
					finds++;
					break;
				}
			}
		}
		if(finds!=1)
		{
			// either found a split in the road (>1) or the road stopped (0) - both things shouldnt happen but just in case they do: break the loop.
			break;
		}
	}
	}
}

bool AIPlayerJH::SoldierAvailable()
{
	std::list<AIJH::Coords> storeHousePoses = construction.GetStoreHousePositions();
	unsigned freeSoldiers = 0;
	for (std::list<AIJH::Coords>::iterator it = storeHousePoses.begin(); it != storeHousePoses.end(); it++)
	{
		const nobBaseWarehouse *nbw = aii->GetSpecObj<nobBaseWarehouse>(it->x, it->y);
		if (nbw)
		{
			const Goods *g = nbw->GetInventory();
			freeSoldiers += (g->people[JOB_PRIVATE] + g->people[JOB_PRIVATEFIRSTCLASS] + g->people[JOB_SERGEANT] + g->people[JOB_OFFICER] + g->people[JOB_GENERAL]);
		}
	}
	return (freeSoldiers != 0);
}

bool AIPlayerJH::HuntablesinRange(unsigned x,unsigned y,unsigned min)
{
	unsigned maxrange=50;
	unsigned short fx,fy,lx,ly;
	const unsigned short SQUARE_SIZE = 19;
	unsigned huntablecount=0;
	if(x > SQUARE_SIZE) fx = x-SQUARE_SIZE; else fx = 0;
	if(y > SQUARE_SIZE) fy = y-SQUARE_SIZE; else fy = 0;
	if(x+SQUARE_SIZE < aii->GetMapWidth()) lx = x+SQUARE_SIZE; else lx = aii->GetMapWidth()-1;
	if(y+SQUARE_SIZE < aii->GetMapHeight()) ly = y+SQUARE_SIZE; else ly = aii->GetMapHeight()-1;
	// Durchgehen und nach Tieren suchen
	for(unsigned short py = fy;py<=ly;++py)
	{
		for(unsigned short px = fx;px<=lx;++px)
		{
			// Gibts hier was bewegliches?
			if(gwb->GetFigures(px,py).size())
			{
				// Dann nach Tieren suchen
				for(list<noBase*>::iterator it = gwb->GetFigures(px,py).begin();it.valid();++it)
				{
					if((*it)->GetType() == NOP_ANIMAL)
					{
						// Ist das Tier überhaupt zum Jagen geeignet?
						if(!static_cast<noAnimal*>(*it)->CanHunted())
							continue;
						// Und komme ich hin?
						if(gwb->FindHumanPath(x,y,static_cast<noAnimal*>(*it)->GetX(),static_cast<noAnimal*>(*it)->GetY(),maxrange) != 0xFF)
							// Dann nehmen wir es
						{
							if(++huntablecount>=min)
								return true;
						}
						
					}	
				}
			}
		}
	}
	return false;
}

void AIPlayerJH::InitStoreAndMilitarylists()
{
	unsigned short width = aii->GetMapWidth();
	unsigned short height = aii->GetMapHeight();
	
	for (unsigned short y=0; y<height; ++y)
	{
		for (unsigned short x=0; x<width; ++x)
		{
			const nobMilitary *mil;
			if ((mil = aii->GetSpecObj<nobMilitary>(x, y))&&gwb->GetNode(x,y).owner-1==playerid)
			{
				milBuildings.push_back(Coords(x,y));
				continue;
			}
			const nobBaseWarehouse *wh;
			if ((wh=aii->GetSpecObj<nobBaseWarehouse>(x, y))&&gwb->GetNode(x,y).owner-1==playerid)
			{
				//if this building blocks beer/weapons/shields it is not our main storehouse so add it anywhere on our list - else add it in front
				if(wh->CheckRealInventorySettings(0,2,0))
					construction.AddStoreHouse(x,y);
				else
				{
					if(x!=player->hqx||y!=player->hqy) //if it is on our hq spot it should already be in the list
						construction.AddStoreHouseFront(x,y);
				}
				continue;
			}			
		}
	}
	if(milBuildings.size()>0||construction.GetStoreHousePositions().size()>1)
		Chat(_("AI'm back"));
}

