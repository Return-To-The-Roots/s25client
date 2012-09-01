// $Id: AIConstruction.cpp 8119 2012-09-01 19:12:36Z jh $
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

///////////////////////////////////////////////////////////////////////////////
// Header
#include "main.h"
#include "AIConstruction.h"

#include "GameCommands.h"
#include "nobBaseMilitary.h"
#include "MapGeometry.h"
#include "nobHQ.h"
#include "AIPlayerJH.h"

// from Pathfinding.cpp
bool IsPointOK_RoadPath(const GameWorldBase& gwb, const MapCoord x, const MapCoord y, const unsigned char dir, const void *param);

AIConstruction::AIConstruction(AIInterface *aii, AIPlayerJH *aijh)
: aii(aii), aijh(aijh)
{
	playerID = aii->GetPlayerID();
	buildingsWanted.resize(BUILDING_TYPES_COUNT);
	RefreshBuildingCount();
	InitBuildingsWanted();
	if (!aijh->TestDefeat())
	{
		AddStoreHouseFront(aii->GetHeadquarter()->GetX(), aii->GetHeadquarter()->GetY());
	}
}

AIConstruction::~AIConstruction(void)
{
}

void AIConstruction::AddBuildJob(AIJH::BuildJob *job, bool front)
{
	if (front)
		buildJobs.push_front(job);
	else
		buildJobs.push_back(job);
}

void AIConstruction::AddJob(AIJH::Job *job, bool front) 
{
	if (front)
		buildJobs.push_front(job);
	else
		buildJobs.push_back(job);

}

AIJH::Job *AIConstruction::GetBuildJob()
{
	if (buildJobs.empty())
		return NULL;
	
	AIJH::Job *job = buildJobs.front();
	buildJobs.pop_front();
	return job;
}

void AIConstruction::AddConnectFlagJob(const noFlag *flag)
{
	buildJobs.push_front(new AIJH::ConnectJob(aijh, flag->GetX(), flag->GetY()));
}


void AIConstruction::FindFlags(std::vector<const noFlag*>& flags, unsigned short x, unsigned short y, unsigned short radius, 
															 unsigned short real_x, unsigned short real_y, unsigned short real_radius, bool clear)
{
	if (clear)
		flags.clear();

	for(MapCoord tx=aii->GetXA(x,y,0), r=1;r<=radius;tx=aii->GetXA(tx,y,0),++r)
	{
		MapCoord tx2 = tx, ty2 = y;
		for(unsigned i = 2;i<8;++i)
		{
			for(MapCoord r2=0;r2<r;aii->GetPointA(tx2,ty2,i%6),++r2)
			{
				if(aii->GetDistance(tx2, ty2, real_x, real_y) <= real_radius && aii->GetSpecObj<noFlag>(tx2,ty2))
				{
					flags.push_back(aii->GetSpecObj<noFlag>(tx2,ty2));
				}
			}
		}
	}
}

void AIConstruction::FindFlags(std::vector<const noFlag*>& flags, unsigned short x, unsigned short y, unsigned short radius, bool clear)
{
	if (clear)
		flags.clear();

	// TODO Performance Killer!
	/*
	if (radius > 10)
	{
		list<nobBaseMilitary*> military;
		gwb->LookForMilitaryBuildings(military, x, y, 2);
		for(list<nobBaseMilitary*>::iterator it = military.begin();it.valid();++it)
		{
			unsigned distance = gwb->CalcDistance((*it)->GetX(), (*it)->GetY(), x, y);
			if (distance < radius && (*it)->GetPlayer() == player->getPlayerID())
			{
				FindFlags(flags, (*it)->GetX(), (*it)->GetY(), 10, x, y, radius, false);
			}
		}
	}
	*/

	for(MapCoord tx=aii->GetXA(x,y,0), r=1;r<=radius;tx=aii->GetXA(tx,y,0),++r)
	{
		MapCoord tx2 = tx, ty2 = y;
		for(unsigned i = 2;i<8;++i)
		{
			for(MapCoord r2=0;r2<r;aii->GetPointA(tx2,ty2,i%6),++r2)
			{
				const noFlag *flag = aii->GetSpecObj<noFlag>(tx2,ty2);
				if(flag && flag->GetPlayer() == playerID)
				{
					flags.push_back(flag);
					if (flags.size() > 30)
					{
						return;
					}
				}
			}
		}
	}
}

bool AIConstruction::ConnectFlagToRoadSytem(const noFlag *flag, std::vector<unsigned char>& route, unsigned int maxSearchRadius)
{
	// TODO: die methode kann  ganz schön böse Laufzeiten bekommen... Optimieren?

	// Radius in dem nach würdigen Fahnen gesucht wird
	//const unsigned short maxSearchRadius = 10;

	// Ziel, das möglichst schnell erreichbar sein soll 
	//noFlag *targetFlag = gwb->GetSpecObj<nobHQ>(player->hqx, player->hqy)->GetFlag();
	noFlag *targetFlag = FindTargetStoreHouseFlag(flag->GetX(), flag->GetY());

	// Falls kein Lager mehr vorhanden ist, brauchen wir auch keinen Weg suchen
	if (!targetFlag)
		return false;

	// Flaggen in der Umgebung holen
	std::vector<const noFlag*> flags;
	FindFlags(flags, flag->GetX(), flag->GetY(), maxSearchRadius);

#ifdef DEBUG_AI
	std::cout << "FindFlagsNum: " << flags.size() << std::endl;
#endif

	unsigned shortest = 0;
	unsigned int shortestLength = 99999;
	std::vector<unsigned char> tmpRoute;
	bool found = false;
	
	// Jede Flagge testen...
	for(unsigned i=0; i<flags.size(); ++i)
	{
		tmpRoute.clear();
		unsigned int length;
		
		// Gibts überhaupt einen Pfad zu dieser Flagge
		bool pathFound = aii->FindFreePathForNewRoad(flag->GetX(), flag->GetY(), flags[i]->GetX(), flags[i]->GetY(), &tmpRoute, &length);

		// Wenn ja, dann gucken ob dieser Pfad möglichst kurz zum "höheren" Ziel (allgemeines Lager im Moment) ist
		if (pathFound)
		{
			unsigned int distance = 0;

			// Strecke von der potenziellen Zielfahne bis zum Lager
			bool pathFound = aii->FindPathOnRoads(flags[i], targetFlag, &distance);

			// Gewählte Fahne hat leider auch kein Anschluß an ein Lager, zu schade!
			if (!pathFound)
				// Und ist auch nicht zufällig die Lager-Flagge selber...
				if (flags[i]->GetX() != targetFlag->GetX() || flags[i]->GetY() != targetFlag->GetY())
					continue;

			// Sind wir mit der Fahne schon verbunden? Einmal reicht!
			if (aii->FindPathOnRoads(flags[i], flag))
				continue;
			
			// Ansonsten haben wir einen Pfad!
			found = true;
			
			// Kürzer als der letzte? Nehmen! Existierende Strecke höher gewichten (2), damit möglichst kurze Baustrecken
			// bevorzugt werden bei ähnlich langen Wegmöglichkeiten
			if (2 * length + distance < shortestLength)
			{
				shortest = i;
				shortestLength = 2 * length + distance;
				route = tmpRoute;
			}
		}
	}

	if (found)
	{
		return BuildRoad(flag, flags[shortest], route);
	}
	return false;
}

bool AIConstruction::BuildRoad(const noRoadNode *start, const noRoadNode *target, std::vector<unsigned char> &route)
{
	bool foundPath;
		
	// Gucken obs einen Weg gibt
	if (route.empty())
	{
		foundPath = aii->FindFreePathForNewRoad(start->GetX(), start->GetY(), target->GetX(), target->GetY(), &route);
	}
	else
	{
		// Wenn Route übergeben wurde, davon ausgehen dass diese auch existiert
		foundPath = true;
	}

	// Wenn Pfad gefunden, Befehl zum Straße bauen und Flagen setzen geben
	if (foundPath)
	{
		MapCoord x = start->GetX();
		MapCoord y = start->GetY();
		aii->BuildRoad(x, y, route);

		// Flaggen auf der Straße setzen
		for(unsigned i=0; i<route.size(); ++i)
		{
			aii->GetPointA(x, y, route[i]);
			// Alle zwei Teilstücke versuchen eine Flagge zu bauen
			if (i % 2 == 1)
			{
				aii->SetFlag(x, y);
			}
		}
		return true;
	}
	return false;
}

bool AIConstruction::IsConnectedToRoadSystem(const noFlag *flag)
{
	noFlag *targetFlag = this->FindTargetStoreHouseFlag(flag->GetX(), flag->GetY());
	if (targetFlag)
		return aii->FindPathOnRoads(flag, targetFlag);
	else 
		return false;
}

BuildingType AIConstruction::ChooseMilitaryBuilding(MapCoord x, MapCoord y)
{
	BuildingType bld = BLD_BARRACKS;

	if (((rand() % 3) == 0||aii->GetInventory()->people[JOB_PRIVATE]<15)&&(aii->GetInventory()->goods[GD_STONES]>6||GetBuildingCount(BLD_QUARRY)>0))
		bld = BLD_GUARDHOUSE;

	std::list<nobBaseMilitary*> military;
	aii->GetMilitaryBuildings(x, y, 3, military);
	for(std::list<nobBaseMilitary*>::iterator it = military.begin();it!=military.end();++it)
	{
		unsigned distance = aii->GetDistance((*it)->GetX(), (*it)->GetY(), x, y);

		// Prüfen ob Feind in der Nähe
		if ((*it)->GetPlayer() != playerID && distance < 35)
		{
			int randmil = rand();

			// avoid to build catapults in the beginning (no expansion)
			unsigned  militaryBuildingCount = GetBuildingCount(BLD_BARRACKS) + GetBuildingCount(BLD_GUARDHOUSE)
				+ GetBuildingCount(BLD_WATCHTOWER) + GetBuildingCount(BLD_FORTRESS);

			if (randmil % 8 == 0 && aii->CanBuildCatapult() && militaryBuildingCount > 3)
				bld = BLD_CATAPULT;
			else if (randmil % 2 == 0)
				bld = BLD_FORTRESS;
			else
				bld = BLD_WATCHTOWER;
			//slim chance for a guardhouse instead of tower or fortress so we can expand towards an enemy even if there are no big building spots in that direction
			if(randmil%10==0)
				bld=BLD_GUARDHOUSE;
			break;
		}
	}

	return bld;
}

unsigned AIConstruction::GetBuildingCount(BuildingType type)
{
	return buildingCounts.building_counts[type] + buildingCounts.building_site_counts[type];
}

unsigned AIConstruction::GetBuildingSitesCount(BuildingType type)
{
	return buildingCounts.building_site_counts[type];
}

bool AIConstruction::Wanted(BuildingType type)
{
	if (type == BLD_CATAPULT && !aii->CanBuildCatapult())
		return false;
	if ((type >= BLD_BARRACKS && type <= BLD_FORTRESS) || type == BLD_STOREHOUSE)
		//todo: find a better way to determine that there is no risk in expanding than sawmill up and complete (everything else complete as well)
		return (GetBuildingCount(BLD_BARRACKS)+GetBuildingCount(BLD_GUARDHOUSE)+GetBuildingCount(BLD_FORTRESS)+GetBuildingCount(BLD_WATCHTOWER)>0 || (GetBuildingCount(BLD_SAWMILL)>1&&aii->GetBuildingSites().size()<2));		
	return GetBuildingCount(type) < buildingsWanted[type];
}

void AIConstruction::RefreshBuildingCount()
{
	aii->GetBuildingCount(buildingCounts);
	//no military buildings -> usually start only
	if(GetBuildingCount(BLD_BARRACKS)+GetBuildingCount(BLD_GUARDHOUSE)+GetBuildingCount(BLD_FORTRESS)+GetBuildingCount(BLD_WATCHTOWER)<1)
	{
		buildingsWanted[BLD_FORESTER] = 1;
		buildingsWanted[BLD_SAWMILL] = 2; //probably only has 1 saw+carpenter but if that is the case the ai will try to produce 1 additional saw very quickly
		buildingsWanted[BLD_WOODCUTTER] = 1;
		buildingsWanted[BLD_QUARRY] = 2;
		buildingsWanted[BLD_GRANITEMINE] = 0;
		buildingsWanted[BLD_COALMINE] = 0;
		buildingsWanted[BLD_IRONMINE] = 0;
		buildingsWanted[BLD_GOLDMINE] = 0;
		buildingsWanted[BLD_CATAPULT] = 0;
		buildingsWanted[BLD_FISHERY] = 0;		
		buildingsWanted[BLD_HUNTER] = 0;
		buildingsWanted[BLD_FARM] = 0;
	}
	else
	{
		buildingsWanted[BLD_FORESTER]=(GetBuildingCount(BLD_SAWMILL)>2&&GetBuildingCount(BLD_SAWMILL)>GetBuildingCount(BLD_WOODCUTTER))||(aii->GetInventory()->goods[GD_BOARDS]<20&&GetBuildingCount(BLD_STOREHOUSE)>0)?2:1;
	//building types usually limited by profession+tool for profession with some arbitrary limit. Some buildings which are linked to others in a chain / profession-tool-rivalry have additional limits.
	buildingsWanted[BLD_WOODCUTTER]=(aii->GetInventory()->goods[GD_AXE] + aii->GetInventory()->people[JOB_WOODCUTTER]<12) ? aii->GetInventory()->goods[GD_AXE] + aii->GetInventory()->people[JOB_WOODCUTTER] : 12;
	if(GetBuildingCount(BLD_SAWMILL)*2<buildingsWanted[BLD_WOODCUTTER]&&GetBuildingCount(BLD_SAWMILL)<4)
		buildingsWanted[BLD_WOODCUTTER]=GetBuildingCount(BLD_SAWMILL)*2;

	buildingsWanted[BLD_FISHERY]=(aii->GetInventory()->goods[GD_RODANDLINE] + aii->GetInventory()->people[JOB_FISHER]<10) ? aii->GetInventory()->goods[GD_RODANDLINE] + aii->GetInventory()->people[JOB_FISHER] : 10;
	buildingsWanted[BLD_HUNTER]=(aii->GetInventory()->goods[GD_BOW] + aii->GetInventory()->people[JOB_HUNTER]<4)?aii->GetInventory()->goods[GD_BOW] + aii->GetInventory()->people[JOB_HUNTER]:4;

	
	if(aii->GetInventory()->goods[GD_PICKAXE]+aii->GetInventory()->people[JOB_MINER]<7 && aii->GetInventory()->people[JOB_STONEMASON]>0 && aii->GetInventory()->people[JOB_MINER]<3)
	{
		buildingsWanted[BLD_QUARRY] = aii->GetInventory()->people[JOB_STONEMASON]; //dont use pickaxes if there are only 2 miners!
	}
	else
	{
		//>6miners = build up to 6 depending on resources, else max out at miners/2
		buildingsWanted[BLD_QUARRY]=(aii->GetInventory()->goods[GD_PICKAXE] + aii->GetInventory()->people[JOB_STONEMASON]<6) ? ((aii->GetInventory()->people[JOB_MINER]>6)?aii->GetInventory()->goods[GD_PICKAXE] + aii->GetInventory()->people[JOB_STONEMASON]:aii->GetInventory()->people[JOB_MINER]/2) : 6;		
	}
	//sawmills limited by woodcutters and carpenter+saws
	if(GetBuildingCount(BLD_WOODCUTTER) < 6)
	{
		if (aii->GetInventory()->goods[GD_SAW] + aii->GetInventory()->people[JOB_CARPENTER]>1 + GetBuildingCount(BLD_WOODCUTTER) / 2)
			buildingsWanted[BLD_SAWMILL] = 1 + GetBuildingCount(BLD_WOODCUTTER) / 2;
		else
			buildingsWanted[BLD_SAWMILL] = aii->GetInventory()->goods[GD_SAW] + aii->GetInventory()->people[JOB_CARPENTER];
	}
	else
	{
		if (aii->GetInventory()->goods[GD_SAW] + aii->GetInventory()->people[JOB_CARPENTER]>3)
			buildingsWanted[BLD_SAWMILL] = 4;
		else
			buildingsWanted[BLD_SAWMILL] = aii->GetInventory()->goods[GD_SAW] + aii->GetInventory()->people[JOB_CARPENTER];
	}
	//ironsmelters limited by ironmines or crucibles
	buildingsWanted[BLD_IRONSMELTER]=(aii->GetInventory()->goods[GD_CRUCIBLE] + aii->GetInventory()->people[JOB_IRONFOUNDER]>=GetBuildingCount(BLD_IRONMINE))?GetBuildingCount(BLD_IRONMINE) : aii->GetInventory()->goods[GD_CRUCIBLE] + aii->GetInventory()->people[JOB_IRONFOUNDER];
	
	buildingsWanted[BLD_MINT] = GetBuildingCount(BLD_GOLDMINE);
	//armory count = smelter -metalworks if there is more than 1 smelter or 1 if there is just 1.
	buildingsWanted[BLD_ARMORY] = (GetBuildingCount(BLD_IRONSMELTER)>1)?GetBuildingCount(BLD_IRONSMELTER)-GetBuildingCount(BLD_METALWORKS):GetBuildingCount(BLD_IRONSMELTER);
	//brewery count = 1+(armory/4) if there is at least 1 armory
	buildingsWanted[BLD_BREWERY] = (GetBuildingCount(BLD_ARMORY) > 0 && GetBuildingCount(BLD_FARM) > 0) ? 1+(GetBuildingCount(BLD_ARMORY)/5) : 0;
	//metalworks is 1 if there is at least 1 smelter, 2 if mines are inexhaustible and we have at least 4 ironsmelters
	buildingsWanted[BLD_METALWORKS] = (GetBuildingCount(BLD_IRONSMELTER) > 0) ? 1 : 0 ;

	buildingsWanted[BLD_MILL] = (buildingCounts.building_counts[BLD_FARM]<8)?(buildingCounts.building_counts[BLD_FARM] + 2) / 4:GetBuildingCount(BLD_FARM)<20?(buildingCounts.building_counts[BLD_FARM] ) / 4:(buildingCounts.building_counts[BLD_FARM] ) / 3;
	if (buildingsWanted[BLD_MILL]>GetBuildingCount(BLD_BAKERY)+1) buildingsWanted[BLD_MILL]=GetBuildingCount(BLD_BAKERY)+1;
	buildingsWanted[BLD_BAKERY] = (GetBuildingCount(BLD_MILL)>aii->GetInventory()->goods[GD_ROLLINGPIN] + aii->GetInventory()->people[JOB_BAKER])?aii->GetInventory()->goods[GD_ROLLINGPIN] + aii->GetInventory()->people[JOB_BAKER]:(GetBuildingCount(BLD_MILL));

	buildingsWanted[BLD_PIGFARM] = (buildingCounts.building_counts[BLD_FARM]<8)?buildingCounts.building_counts[BLD_FARM] / 4:(buildingCounts.building_counts[BLD_FARM]-2) / 4;
	if (buildingsWanted[BLD_PIGFARM]>GetBuildingCount(BLD_SLAUGHTERHOUSE)+1)buildingsWanted[BLD_PIGFARM]=GetBuildingCount(BLD_SLAUGHTERHOUSE)+1;
	buildingsWanted[BLD_SLAUGHTERHOUSE] = (GetBuildingCount(BLD_PIGFARM)>aii->GetInventory()->goods[GD_CLEAVER] + aii->GetInventory()->people[JOB_BUTCHER])?aii->GetInventory()->goods[GD_CLEAVER] + aii->GetInventory()->people[JOB_BUTCHER]:(GetBuildingCount(BLD_PIGFARM));

	buildingsWanted[BLD_WELL] = buildingsWanted[BLD_BAKERY] + buildingsWanted[BLD_PIGFARM]
		+ buildingsWanted[BLD_DONKEYBREEDER] + buildingsWanted[BLD_BREWERY];

	buildingsWanted[BLD_FARM] = aii->GetInventory()->goods[GD_SCYTHE] + aii->GetInventory()->people[JOB_FARMER];
	
	if(aii->GetInventory()->goods[GD_PICKAXE]+aii->GetInventory()->people[JOB_MINER]<3){
		//almost out of new pickaxes and miners - emergency program: get coal,iron,smelter&metalworks
		buildingsWanted[BLD_COALMINE] = 1;
		buildingsWanted[BLD_IRONMINE] = 1;
		buildingsWanted[BLD_GOLDMINE] = 0;
		buildingsWanted[BLD_IRONSMELTER] = 1;
		buildingsWanted[BLD_METALWORKS]=1;
		buildingsWanted[BLD_ARMORY]=0;	
		buildingsWanted[BLD_GRANITEMINE]=0;
		buildingsWanted[BLD_MINT]=0;
	}
	else  //more than 2 miners
	{ 
		//coalmine count now depends on iron & gold not linked to food or material supply - might have to add a material check if this makes problems
		buildingsWanted[BLD_COALMINE]=(GetBuildingCount(BLD_IRONMINE)>0)?(GetBuildingCount(BLD_IRONMINE)*2)-1+GetBuildingCount(BLD_GOLDMINE):(GetBuildingCount(BLD_GOLDMINE)>0)?GetBuildingCount(BLD_GOLDMINE):1;
		if (GetBuildingCount(BLD_FARM) > 8) //quite the empire just scale mines with farms
		{
			buildingsWanted[BLD_IRONMINE] = (GetBuildingCount(BLD_FARM)/3>GetBuildingCount(BLD_IRONSMELTER)+1)?GetBuildingCount(BLD_IRONSMELTER)+1:GetBuildingCount(BLD_FARM)/3;
			buildingsWanted[BLD_GOLDMINE] = (GetBuildingCount(BLD_MINT)+1>1)?2:1;
			buildingsWanted[BLD_DONKEYBREEDER]=1;
		}
		else
		{		//probably still limited in food supply	go up to 4 coal 1 gold 2 iron (gold+coal->coin, iron+coal->tool, iron+coal+coal->weapon)				
			buildingsWanted[BLD_IRONMINE]=(aii->GetInventory()->people[JOB_MINER]+aii->GetInventory()->goods[GD_PICKAXE]-(GetBuildingCount(BLD_COALMINE)+GetBuildingCount(BLD_GOLDMINE))>1&&GetBuildingCount(BLD_BAKERY)+GetBuildingCount(BLD_SLAUGHTERHOUSE)+GetBuildingCount(BLD_HUNTER)+GetBuildingCount(BLD_FISHERY)>4)?2:1;
			buildingsWanted[BLD_GOLDMINE]=(aii->GetInventory()->people[JOB_MINER]>2)?1:0;			
		}
		if(aii->GetInventory()->goods[GD_STONES]<50 && GetBuildingCount(BLD_QUARRY)<1) //no more stones and no quarry -> try emergency granitemines.
			buildingsWanted[BLD_GRANITEMINE] =(aii->GetInventory()->people[JOB_MINER]>6)? 2:1;
		else
			buildingsWanted[BLD_GRANITEMINE]=0;
	}
	}
}

void AIConstruction::InitBuildingsWanted()
{
	buildingsWanted[BLD_FORESTER] = 1;
	buildingsWanted[BLD_SAWMILL] = 1;
	buildingsWanted[BLD_WOODCUTTER] = 12;
	buildingsWanted[BLD_QUARRY] = 6;
	buildingsWanted[BLD_GRANITEMINE] = 0;
	buildingsWanted[BLD_COALMINE] = 4;
	buildingsWanted[BLD_IRONMINE] = 2;
	buildingsWanted[BLD_GOLDMINE] = 1;
	buildingsWanted[BLD_CATAPULT] = 5;
	buildingsWanted[BLD_FISHERY] = 6;
	buildingsWanted[BLD_QUARRY] = 6;
	buildingsWanted[BLD_HUNTER] = 2;
	buildingsWanted[BLD_FARM] = aii->GetInventory()->goods[GD_SCYTHE] + aii->GetInventory()->people[JOB_FARMER];
	buildingsWanted[BLD_HARBORBUILDING] = 99;
	buildingsWanted[BLD_SHIPYARD] = 1;
}

bool AIConstruction::BuildAlternativeRoad(const noFlag *flag, std::vector<unsigned char> &route)
{
	// Radius in dem nach würdigen Fahnen gesucht wird
	const unsigned short maxRoadLength = 10;
	// Faktor um den der Weg kürzer sein muss als ein vorhander Pfad, um gebaut zu werden
	const unsigned short lengthFactor = 5;


	// Flaggen in der Umgebung holen
	std::vector<const noFlag*> flags;
	FindFlags(flags, flag->GetX(), flag->GetY(), maxRoadLength);

	// Jede Flagge testen...
	for(unsigned i=0; i<flags.size(); ++i)
	{
		//std::vector<unsigned char> new_route;
		route.clear();
		unsigned int newLength;
		
		// Gibts überhaupt einen Pfad zu dieser Flagge
		bool pathFound = aii->FindFreePathForNewRoad(flag->GetX(), flag->GetY(), flags[i]->GetX(), flags[i]->GetY(),&route,&newLength);

		// Wenn ja, dann gucken ob unser momentaner Weg zu dieser Flagge vielleicht voll weit ist und sich eine Straße lohnt
		if (pathFound)
		{
			unsigned int oldLength = 0;

			// Aktuelle Strecke zu der Flagge
			bool pathAvailable = aii->FindPathOnRoads(flags[i], flag, &oldLength);

			// Lohnt sich die Straße?
			if (!pathAvailable || newLength * lengthFactor < oldLength)
			{
				return BuildRoad(flag, flags[i], route);
			}
		}
	}

	return false;
}

bool AIConstruction::FindStoreHousePosition(MapCoord &x, MapCoord &y, unsigned radius)
{
	// max distance to warehouse/hq
	const unsigned maxDistance = 20;

	MapCoord fx = aii->GetXA(x,y,4);
	MapCoord fy = aii->GetYA(x,y,4);

	unsigned minDist = std::numeric_limits<unsigned>::max();
	for (std::list<AIJH::Coords>::iterator it = storeHouses.begin(); it != storeHouses.end(); it++)
	{
		const noBaseBuilding *bld;
		if ((bld = aii->GetSpecObj<noBaseBuilding>((*it).x, (*it).y)))
		{
			if (bld->GetBuildingType() != BLD_STOREHOUSE && bld->GetBuildingType() != BLD_HEADQUARTERS && bld->GetBuildingType()!=BLD_HARBORBUILDING)
				continue;

			const noFlag *targetFlag = aii->GetSpecObj<noFlag>(fx,fy);
			unsigned dist;
			bool pathAvailable = aii->FindPathOnRoads(bld->GetFlag(), targetFlag, &dist);

			if (!pathAvailable)
				continue;

			if (dist < minDist)
			{
				minDist = dist;
			}
			if (minDist <= maxDistance)
			{
				return false;
			}
		}
	}
	//assert(false); // fix pos berechnung für lagerhaus beim aufruf, nicht hier
	return true; //SimpleFindPosition(x, y, BUILDING_SIZE[BLD_STOREHOUSE], radius);
}

noFlag *AIConstruction::FindTargetStoreHouseFlag(MapCoord x, MapCoord y)
{
	unsigned minDistance = std::numeric_limits<unsigned>::max();
	AIJH::Coords minTarget(0xFF,0xFF);
	bool found = false;
	const noBaseBuilding *bld;
	for (std::list<AIJH::Coords>::iterator it = storeHouses.begin(); it != storeHouses.end(); it++)
	{
		if ((bld = aii->GetSpecObj<noBaseBuilding>((*it).x, (*it).y)))
		{
			if (bld->GetBuildingType() != BLD_STOREHOUSE && bld->GetBuildingType() != BLD_HEADQUARTERS)
				continue;
		
			unsigned dist = aii->GetDistance(x, y, (*it).x, (*it).y);

			if (dist < minDistance)
			{
				minDistance = dist;
				minTarget = *it;
				found = true;
			}
		}
	}
	if (!found)
		return NULL;
	else
	{
		bld = aii->GetSpecObj<noBaseBuilding>(minTarget.x, minTarget.y);
		return bld->GetFlag();
	}
}
