// $Id: AIConstruction.cpp 8305 2012-09-22 12:34:54Z marcus $
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
#include "noBuildingSite.h"

// from Pathfinding.cpp
bool IsPointOK_RoadPath(const GameWorldBase& gwb, const MapCoord x, const MapCoord y, const unsigned char dir, const void *param);

AIConstruction::AIConstruction(AIInterface *aii, AIPlayerJH *aijh)
: aii(aii), aijh(aijh)
{
	playerID = aii->GetPlayerID();
	buildingsWanted.resize(BUILDING_TYPES_COUNT);
	RefreshBuildingCount();
	InitBuildingsWanted();
	if (!aijh->TestDefeat() && (aii->GetHeadquarter() != NULL))
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
			unsigned size=0;
			//check for non-flag points on planned route: more than 2 nonflaggable spaces on the route -> not really valid path 
			unsigned temp=0;
			MapCoord tx=flag->GetX();
			MapCoord ty=flag->GetY();
			for(unsigned j=0;j<tmpRoute.size();++j)
			{
				aii->GetPointA(tx,ty,tmpRoute[j]);
				if(aii->GetBuildingQuality(tx,ty)<1)
					temp++;
				else
				{
					if(size<temp)
						size=temp;
					temp=0;
				}
			}
			if(size>2)
				continue;

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
			if (2 * length + distance +10*size < shortestLength)
			{
				shortest = i;
				shortestLength = 2 * length + distance + 10*size;
				route = tmpRoute;
			}
		}
	}

	if (found)
	{
		//LOG.lprintf("ai build main road player %i at %i %i\n", flag->GetPlayer(), flag->GetX(),flag->GetY());
		return MinorRoadImprovements(flag, flags[shortest], route);
	}
	return false;
}

bool AIConstruction::MinorRoadImprovements(const noRoadNode *start,const noRoadNode *target,std::vector<unsigned char>&route)
{	
	//bool done=false;
	MapCoord x=start->GetX(),y=start->GetY();
	/*for(unsigned i=0;i<route.size();i++)
	{
		LOG.lprintf(" %i",route[i]);
	}
	LOG.lprintf("\n");*/
	for(unsigned i=0;i<(route.size()-1);i++)
	{
		if(((route[i]+1)%6==route[i+1])||((route[i]+5)%6==route[i+1])) //switching current and next route element will result in the same position after building both
		{
			MapCoord tx=x,ty=y;			
			aii->GetPointA(tx,ty,route[i+1]);
			aii->GetPointA(x,y,route[i]);
			if(aii->RoadAvailable(tx,ty,route[i+1])&&aii->IsOwnTerritory(tx,ty)) //can the alternative road be build?
			{
				if(aii->CalcBQSumDifference(x,y,tx,ty)) //does the alternative road block a lower buildingquality point than the normal planned route?
				{
					//LOG.lprintf("AIConstruction::road improvements p%i from %i,%i moved node %i,%i to %i,%i i:%i, i+1:%i\n",playerID, start->GetX(),start->GetY(),x,y,tx,ty,route[i],route[i+1]);
					x=tx; //we move the alternative path so move x&y and switch the route entries
					y=ty;
					if((route[i]+1)%6==route[i+1])
					{	
						route[i]=(route[i]+1)%6;
						route[i+1]=(route[i+1]+5)%6;
					}
					else
					{						
						route[i]=(route[i]+5)%6;
						route[i+1]=(route[i+1]+1)%6;
					}
					//done=true;
				}
			}
		}
		else
			aii->GetPointA(x,y,route[i]);
	}
	/*if(done)
	{
		LOG.lprintf("final road\n");
		for(unsigned i=0;i<route.size();i++)
		{
			LOG.lprintf(" %i",route[i]);
		}
		LOG.lprintf("\n");
	}*/
	return BuildRoad(start,target,route);
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
		aii->SetFlag(target->GetX(),target->GetY());
		aii->BuildRoad(x, y, route);
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
			if (randmil % 8 == 0 && aii->CanBuildCatapult() && militaryBuildingCount > 3&&distance<15)
				bld = BLD_CATAPULT;
			else
			{
				if (randmil % 2 == 0)
					bld = BLD_FORTRESS;			
				else
					bld = BLD_WATCHTOWER;
			}
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
	if (type == BLD_CATAPULT)
		return aii->CanBuildCatapult()&&(aii->GetInventory()->goods[GD_STONES]>50);
	if ((type >= BLD_BARRACKS && type <= BLD_FORTRESS) || type == BLD_STOREHOUSE)
		//todo: find a better way to determine that there is no risk in expanding than sawmill up and complete
		return (GetBuildingCount(BLD_BARRACKS)+GetBuildingCount(BLD_GUARDHOUSE)+GetBuildingCount(BLD_FORTRESS)+GetBuildingCount(BLD_WATCHTOWER)>0 || buildingCounts.building_counts[BLD_SAWMILL]>0 || (aii->GetInventory()->goods[GD_BOARDS]>30&&GetBuildingCount(BLD_SAWMILL)>0));		
	return GetBuildingCount(type) < buildingsWanted[type];
}

void AIConstruction::RefreshBuildingCount()
{
	aii->GetBuildingCount(buildingCounts);
	//no military buildings -> usually start only
	if(aii->GetMilitaryBuildings().size()<1&&aii->GetStorehouses().size()<2)
	{
		buildingsWanted[BLD_FORESTER]=1;
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
		buildingsWanted[BLD_FORESTER]=(GetBuildingCount(BLD_SAWMILL)>2&&GetBuildingCount(BLD_SAWMILL)>GetBuildingCount(BLD_WOODCUTTER))||(aii->GetInventory()->goods[GD_BOARDS]<40&&GetBuildingCount(BLD_STOREHOUSE)>0)?aii->GetInventory()->goods[GD_BOARDS]<50&&GetBuildingCount(BLD_WOODCUTTER)<9&&buildingsWanted[BLD_WOODCUTTER]>8&&((GetBuildingCount(BLD_BARRACKS)+GetBuildingCount(BLD_GUARDHOUSE)+GetBuildingCount(BLD_FORTRESS)+GetBuildingCount(BLD_WATCHTOWER))>15)?3:2:1;
		if(buildingsWanted[BLD_FORESTER]>(aii->GetInventory()->people[JOB_FORESTER]+aii->GetInventory()->goods[GD_SHOVEL]))buildingsWanted[BLD_FORESTER]=(aii->GetInventory()->people[JOB_FORESTER]+aii->GetInventory()->goods[GD_SHOVEL]);
	//building types usually limited by profession+tool for profession with some arbitrary limit. Some buildings which are linked to others in a chain / profession-tool-rivalry have additional limits.
	buildingsWanted[BLD_WOODCUTTER]=aii->GetInventory()->goods[GD_AXE] + aii->GetInventory()->people[JOB_WOODCUTTER] ;
	

	buildingsWanted[BLD_FISHERY]=(aii->GetInventory()->goods[GD_RODANDLINE] + aii->GetInventory()->people[JOB_FISHER]<10) ? aii->GetInventory()->goods[GD_RODANDLINE] + aii->GetInventory()->people[JOB_FISHER] : 10;
	buildingsWanted[BLD_HUNTER]=(aii->GetInventory()->goods[GD_BOW] + aii->GetInventory()->people[JOB_HUNTER]<4)?aii->GetInventory()->goods[GD_BOW] + aii->GetInventory()->people[JOB_HUNTER]:4;

	//quarry: low ware games start at 2 otherwise build as many as we have stonemasons, higher ware games up to 6 quarries
	if(aii->GetInventory()->goods[GD_PICKAXE]+aii->GetInventory()->people[JOB_MINER]<7 && aii->GetInventory()->people[JOB_STONEMASON]>0 && aii->GetInventory()->people[JOB_MINER]<3)
	{
		buildingsWanted[BLD_QUARRY]=aii->GetInventory()->people[JOB_STONEMASON]>2?aii->GetInventory()->people[JOB_STONEMASON]:2; 
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
		if(aii->GetMilitaryBuildings().size()>10)
			buildingsWanted[BLD_SAWMILL]=min<int>((aii->GetInventory()->goods[GD_SAW] + aii->GetInventory()->people[JOB_CARPENTER]),(aii->GetMilitaryBuildings().size()-5));
		else
			buildingsWanted[BLD_SAWMILL]=min<int>((aii->GetInventory()->goods[GD_SAW] + aii->GetInventory()->people[JOB_CARPENTER]),(aii->GetMilitaryBuildings().size()));
		if(aii->GetInventory()->goods[GD_WOOD]<30)
			buildingsWanted[BLD_SAWMILL]=GetBuildingCount(BLD_SAWMILL);
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

	buildingsWanted[BLD_MILL] = (buildingCounts.building_counts[BLD_FARM]<8)?(buildingCounts.building_counts[BLD_FARM] + 2) / 4:(buildingCounts.building_counts[BLD_FARM] ) / 4;
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
			buildingsWanted[BLD_GOLDMINE] = (GetBuildingCount(BLD_MINT)>0)?GetBuildingCount(BLD_IRONSMELTER)>10&&GetBuildingCount(BLD_MINT)>1?3:2:1;
			buildingsWanted[BLD_DONKEYBREEDER]=1;
		}
		else
		{		//probably still limited in food supply	go up to 4 coal 1 gold 2 iron (gold+coal->coin, iron+coal->tool, iron+coal+coal->weapon)				
			buildingsWanted[BLD_IRONMINE]=(aii->GetInventory()->people[JOB_MINER]+aii->GetInventory()->goods[GD_PICKAXE]-(GetBuildingCount(BLD_COALMINE)+GetBuildingCount(BLD_GOLDMINE))>1&&GetBuildingCount(BLD_BAKERY)+GetBuildingCount(BLD_SLAUGHTERHOUSE)+GetBuildingCount(BLD_HUNTER)+GetBuildingCount(BLD_FISHERY)>4)?2:1;
			buildingsWanted[BLD_GOLDMINE]=(aii->GetInventory()->people[JOB_MINER]>2)?1:0;			
		}
		if(aii->GetInventory()->goods[GD_STONES]<50 && GetBuildingCount(BLD_QUARRY)<4) //no more stones and no quarry -> try emergency granitemines.
			buildingsWanted[BLD_GRANITEMINE] =(aii->GetInventory()->people[JOB_MINER]>6)? 4-GetBuildingCount(BLD_QUARRY):1;
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
	buildingsWanted[BLD_SHIPYARD] = 99;
}

bool AIConstruction::BuildAlternativeRoad(const noFlag *flag, std::vector<unsigned char> &route)
{
	//LOG.lprintf("ai build alt road player %i at %i %i\n", flag->GetPlayer(), flag->GetX(),flag->GetY());
	// Radius in dem nach würdigen Fahnen gesucht wird
	const unsigned short maxRoadLength = 10;
	// Faktor um den der Weg kürzer sein muss als ein vorhander Pfad, um gebaut zu werden
	const unsigned short lengthFactor = 5;


	// Flaggen in der Umgebung holen
	std::vector<const noFlag*> flags;
	FindFlags(flags, flag->GetX(), flag->GetY(), maxRoadLength);
	std::vector<unsigned char>mainroad=route;
	//targetflag for mainroad
	MapCoord tx=flag->GetX(),tx2;
	MapCoord ty=flag->GetY(),ty2;
	for(unsigned i=0;i<mainroad.size();i++)
	{
		aii->GetPointA(tx,ty,mainroad[i]);
	}	
	const noFlag*mainflag=aii->GetSpecObj<noFlag>(tx,ty);

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
			if(!pathAvailable&&mainflag)
			{
				pathAvailable=aii->FindPathOnRoads(flags[i],mainflag,&oldLength);
				if(pathAvailable)
					oldLength+=mainroad.size();
			}
			bool crossmainpath=false;
			unsigned size=0;
			//more than 5 nonflaggable spaces on the route -> not really valid path 
			unsigned temp=0;
			tx=flag->GetX();
			ty=flag->GetY();
			for(unsigned j=0;j<route.size();++j)
			{
				aii->GetPointA(tx,ty,route[j]);
				tx2=flag->GetX();
				ty2=flag->GetY();
				//check if we cross the planned main road
				for(unsigned k=0;k<mainroad.size();++k)
				{
					aii->GetPointA(tx2,ty2,mainroad[k]);
					if(tx2==tx&&ty2==ty)
					{
						crossmainpath=true;
						break;
					}
				}
				if(aii->GetBuildingQuality(tx,ty)<1)
					temp++;
				else
				{
					if(size<temp)
						size=temp;
					temp=0;
				}
			}
			if(size>2||crossmainpath)
				continue;

			// Lohnt sich die Straße?
			if (!pathAvailable || newLength * lengthFactor < oldLength)
			{
				return BuildRoad(flag, flags[i], route);
			}
		}
	}

	return false;
}

bool AIConstruction::OtherStoreInRadius(MapCoord &x, MapCoord &y, unsigned radius)
{
	for (std::list<nobBaseWarehouse*>::const_iterator it = aii->GetStorehouses().begin(); it != aii->GetStorehouses().end(); it++)
	{
		if(aii->CalcDistance((*it)->GetX(),(*it)->GetY(),x,y)<radius)
			return true;
	}
	for(std::list<noBuildingSite*>::const_iterator it=aii->GetBuildingSites().begin();it!=aii->GetBuildingSites().end();it++)
	{
		if((*it)->GetBuildingType()==BLD_STOREHOUSE||(*it)->GetBuildingType()==BLD_HARBORBUILDING)
		{
			if(aii->CalcDistance((*it)->GetX(),(*it)->GetY(),x,y)<radius)
				return true;
		}
	}
	return false;
}

noFlag *AIConstruction::FindTargetStoreHouseFlag(MapCoord x, MapCoord y)
{
	unsigned minDistance = std::numeric_limits<unsigned>::max();
	nobBaseWarehouse* minTarget;
	bool found = false;
	for (std::list<nobBaseWarehouse*>::const_iterator it = aii->GetStorehouses().begin(); it != aii->GetStorehouses().end(); it++)
	{
		unsigned dist = aii->GetDistance(x, y, (*it)->GetX(), (*it)->GetY());
		if (dist < minDistance)
		{
			minDistance = dist;
			minTarget = *it;
			found = true;
		}		
	}
	if (!found)
		return NULL;
	else
	{
		return minTarget->GetFlag();
	}
}
