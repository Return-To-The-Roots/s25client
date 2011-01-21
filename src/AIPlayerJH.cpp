// $Id: AIPlayerJH.cpp 7000 2011-01-21 20:51:29Z jh $
//
// Copyright (c) 2005 - 2010 Settlers Freaks (sf-team at siedler25.org)
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

	if ((gf + (playerid * 2)) % 20 == 0)
	{
		construction.RefreshBuildingCount();
		ExecuteAIJob();
	}

	if ((gf + playerid * 1000) % 3000 == 0)
	{
		//CheckExistingMilitaryBuildings();
		TryToAttack();
	}

	if ((gf + playerid * 10) % 100 == 0)
	{
		CheckNewMilitaryBuildings();
	}



	if (gf == 5)
	{
		const nobHQ * hq = aii->GetHeadquarter();
		MapCoord hqx = hq->GetX();
		MapCoord hqy = hq->GetY();

		
		AddBuildJob(BLD_HARBORBUILDING, hqx, hqy);
		AddBuildJob(BLD_SAWMILL);
		AddBuildJob(BLD_FORESTER);
		AddBuildJob(BLD_WOODCUTTER);
		AddBuildJob(BLD_WOODCUTTER);
		AddBuildJob(construction.ChooseMilitaryBuilding(hqx, hqy));
		AddBuildJob(construction.ChooseMilitaryBuilding(hqx, hqy));
		AddBuildJob(construction.ChooseMilitaryBuilding(hqx, hqy));
		AddBuildJob(construction.ChooseMilitaryBuilding(hqx, hqy));
		AddBuildJob(BLD_QUARRY);
		AddBuildJob(BLD_FISHERY);
		
	}

	if (gf == 100)
	{
		Chat(_("Hi, I'm an artifical player and I'm not very good yet!"));
	}
	if (gf == 120)
	{
		Chat(_("And I may crash your game sometimes..."));

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
	}

	if ((gf % 1000) == 0)
	{
		if (construction.Wanted(BLD_SAWMILL))
		{
			AddBuildJob(BLD_SAWMILL);
		}
	}
}

bool AIPlayerJH::TestDefeat()
{
	if (!aii->GetHeadquarter())
	{
		defeated = true;
		aii->Surrender();
		Chat(_("Oh, no, you destroyed my headquarter! I surrender!"));
		return true;
	}
	return false;
}

void AIPlayerJH::AddBuildJob(BuildingType type, MapCoord x, MapCoord y, bool front)
{
	construction.AddBuildJob(new AIJH::BuildJob(this, type, x, y), front);
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
			}
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

PositionSearch *AIPlayerJH::CreatePositionSearch(MapCoord &x, MapCoord &y, AIJH::Resource res, BuildingQuality size, int minimum)
{
	// set some basic parameters
	PositionSearch *p = new PositionSearch(x, y, res, minimum, size);
	p->nodesPerStep = 25; // TODO make it dependent on something...
	p->resultValue = 0;

	// allocate memory for the nodes
	unsigned numNodes = aii->GetMapWidth() * aii->GetMapHeight();
	p->tested = new std::vector<bool>(numNodes, false);
	p->toTest = new std::queue<unsigned>;

	// insert start position as first node to test
	p->toTest->push(x + y * aii->GetMapWidth());

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
	else if (search->resultValue >= search->minimum && !best 
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
				}

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
	

	// TODO: check warum das so ist, und ob das sinn macht!
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
	UpdateNodesAround(x, y, 11); // todo: fix radius
	construction.RefreshBuildingCount();

	const nobMilitary *mil = aii->GetSpecObj<nobMilitary>(x, y);
	if (mil)
	{
		if ((mil->GetBuildingType() == BLD_BARRACKS || mil->GetBuildingType() == BLD_GUARDHOUSE) && mil->GetFrontierDistance() == 0 && !mil->IsGoldDisabled())
		{
			gcs.push_back(new gc::StopGold(x, y));
		}

		// if near border and gold disabled (by addon): enable it
		if (mil->GetFrontierDistance() != 0 && mil->IsGoldDisabled())
		{
			gcs.push_back(new gc::StopGold(x, y));
		}

		if (!construction.IsConnectedToRoadSystem(mil->GetFlag()))
		{
			construction.AddConnectFlagJob(mil->GetFlag());
		}
	}

	AddBuildJob(BLD_HARBORBUILDING, x, y);

	AddBuildJob(construction.ChooseMilitaryBuilding(x, y), x, y);
	AddBuildJob(construction.ChooseMilitaryBuilding(x, y), x, y);
	AddBuildJob(construction.ChooseMilitaryBuilding(x, y), x, y);

	// Temporär only
	AddBuildJob(BLD_FORESTER, x, y);
	AddBuildJob(BLD_WOODCUTTER, x, y);

	AddBuildJob(BLD_QUARRY, x, y);

	AddBuildJob(BLD_GOLDMINE, x, y);
	AddBuildJob(BLD_COALMINE, x, y);
	AddBuildJob(BLD_IRONMINE, x, y);

	AddBuildJob(BLD_SAWMILL, x, y);

	AddBuildJob(BLD_IRONSMELTER, x, y);
	AddBuildJob(BLD_MINT, x, y);
	AddBuildJob(BLD_ARMORY, x, y);

	AddBuildJob(BLD_FISHERY, x, y);

	AddBuildJob(BLD_HUNTER, x, y);

	AddBuildJob(BLD_STOREHOUSE, x, y);


	AddBuildJob(BLD_FARM, x, y);


	AddBuildJob(BLD_BREWERY, x, y);
	AddBuildJob(BLD_MILL, x, y);
	AddBuildJob(BLD_PIGFARM, x, y);
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
					break;
				}
			}
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
	
	RemoveUnusedRoad(aii->GetSpecObj<noFlag>(aii->GetXA(x,y,4),aii->GetYA(x,y,4)), 1);

	// try to expand, maybe res blocked a passage
	AddBuildJob(construction.ChooseMilitaryBuilding(x, y), x, y);
	AddBuildJob(construction.ChooseMilitaryBuilding(x, y), x, y);

	// and try to rebuild the same building
	AddBuildJob(bld);

	// farm is always good!
	AddBuildJob(BLD_FARM, x, y);
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
			continue;

		if (mil->GetFrontierDistance() == 0)
			continue;

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

void AIPlayerJH::RemoveUnusedRoad(const noFlag *startFlag, unsigned char excludeDir)
{
	unsigned char foundDir = 0xFF;
	unsigned char finds = 0;

	// Count roads from this flag...
	for (unsigned char dir=0; dir < 6; ++dir)
	{
		if (dir == excludeDir)
			continue;
		if (aii->GetPointRoad(startFlag->GetX(), startFlag->GetY(), dir))
		{
			finds++;
			foundDir = dir;
		}
	}

	// if its not equal to 1, it's not an useless flag
	if (finds != 1)
		return;

	// kill it
	aii->DestroyFlag(startFlag);

	MapCoord x = aii->GetXA(startFlag->GetX(), startFlag->GetY(), foundDir);
	MapCoord y = aii->GetYA(startFlag->GetX(), startFlag->GetY(), foundDir);

	unsigned char prevDir = (foundDir + 3) % 6;
	// follow the (only) road to next flag and test it also
	while(true)
	{
		const noFlag *flag;

		// flag found?
		if ((flag = aii->GetSpecObj<noFlag>(x, y)))
		{
			RemoveUnusedRoad(flag, prevDir);
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
					break;
				}
			}
		}
	}
}
